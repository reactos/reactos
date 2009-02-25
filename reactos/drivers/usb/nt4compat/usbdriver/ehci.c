/**
 * ehci.c - USB driver stack project for Windows NT 4.0
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
#include "ehci.h"

//----------------------------------------------------------
// ehci routines
//#define DEMO

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
  : ( ( enDP )->pusb_endp_desc->bEndpointAddress & USB_DIR_IN ) ? 1 : 0 )

#define dev_set_state( pdEV, staTE ) \
( pdEV->flags = ( ( pdEV )->flags & ( ~USB_DEV_STATE_MASK ) ) | ( staTE ) )

#define endp_max_packet_size( enDP ) \
( DEFAULT_ENDP( enDP )\
  ? ( ( ( PUSB_DEV )enDP->pusb_if )->pusb_dev_desc ? \
	  ( ( PUSB_DEV )enDP->pusb_if )->pusb_dev_desc->bMaxPacketSize0\
	  : 8 )\
  : ( enDP->pusb_endp_desc->wMaxPacketSize & 0x7ff ) )

#define endp_mult_count( endp ) ( ( ( endp->pusb_endp_desc->wMaxPacketSize & 0x1800 ) >> 11 ) + 1 )

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

#define ehci_insert_urb_to_schedule( eHCI, pURB, rET ) \
{\
	SYNC_PARAM sync_param;\
	sync_param.ehci = eHCI;\
	sync_param.context = ( pURB );\
	sync_param.ret = FALSE;\
\
	rET = KeSynchronizeExecution( eHCI->pdev_ext->ehci_int, ehci_sync_insert_urb_schedule, &sync_param );\
}

#define EHCI_ERROR_INT ( STS_FATAL | STS_ERR )
#define EHCI_QH_ERROR( qh_contENT ) ( ( qh_contENT )->cur_qtd.status & ( QTD_STS_HALT | QTD_STS_DBE | QTD_STS_BABBLE | QTD_STS_XACT | QTD_STS_MMF ) )
#define EHCI_QTD_ERROR( qtd_contENT ) ( ( qtd_contENT )->status & ( QTD_STS_HALT | QTD_STS_DBE | QTD_STS_BABBLE | QTD_STS_XACT | QTD_STS_MMF ) )

#define EHCI_READ_PORT_ULONG( pul ) ( *pul )
#define EHCI_WRITE_PORT_ULONG( pul, src ) \
{\
	ULONG cmd_reg;\
   	*pul = ( ULONG )src;\
	cmd_reg = EHCI_READ_PORT_ULONG( ehci->port_base + EHCI_USBCMD );\
	if( cmd_reg == 0 )\
		cmd_reg++;\
}

#define EHCI_READ_PORT_UCHAR( pch ) ( *pch )
#define EHCI_WRITE_PORT_UCHAR( pch, src ) ( *pch = ( UCHAR )src )

#define EHCI_READ_PORT_USHORT( psh ) ( *psh )
#define EHCI_WRITE_PORT_USHORT( psh, src ) ( *psh = ( USHORT )src )

#define press_doorbell( eHCI ) \
{\
	ULONG tmp;\
	tmp = EHCI_READ_PORT_ULONG( ( PULONG )( ( eHCI )->port_base + EHCI_USBCMD ) );\
	tmp |= CMD_IAAD;\
	EHCI_WRITE_PORT_ULONG( ( PULONG )( ( eHCI )->port_base +  EHCI_USBCMD ), tmp );\
}
#define ehci_from_hcd( hCD ) ( struct_ptr( ( hCD ), EHCI_DEV, hcd_interf ) )

#define qh_from_list_entry( pentry ) ( ( PEHCI_QH )( ( ( ULONG )struct_ptr( pentry, EHCI_ELEM_LINKS, elem_link )->phys_part ) & PHYS_PART_ADDR_MASK ) )
#define qtd_from_list_entry( pentry ) ( ( PEHCI_QTD )( ( ( ULONG )struct_ptr( pentry, EHCI_ELEM_LINKS, elem_link )->phys_part ) & PHYS_PART_ADDR_MASK ) )
#define itd_from_list_entry( pentry ) ( ( PEHCI_ITD )( ( ( ULONG )struct_ptr( pentry, EHCI_ELEM_LINKS, elem_link )->phys_part ) & PHYS_PART_ADDR_MASK ) )
#define sitd_from_list_entry( pentry ) ( ( PEHCI_SITD )( ( ( ULONG )struct_ptr( pentry, EHCI_ELEM_LINKS, elem_link )->phys_part ) & PHYS_PART_ADDR_MASK ) )
#define fstn_from_list_entry( pentry ) ( ( PEHCI_FSTN )( ( ( ULONG )struct_ptr( pentry, EHCI_ELEM_LINKS, elem_link )->phys_part ) & PHYS_PART_ADDR_MASK ) )

#define qh_from_schedule( pentry ) ( ( PEHCI_QH )( ( ( ULONG )struct_ptr( pentry, EHCI_ELEM_LINKS, sched_link )->phys_part ) & PHYS_PART_ADDR_MASK ) )
#define itd_from_schedule( pentry ) ( ( PEHCI_ITD )( ( ( ULONG )struct_ptr( pentry, EHCI_ELEM_LINKS, sched_link )->phys_part ) & PHYS_PART_ADDR_MASK ) )
#define sitd_from_schedule( pentry ) ( ( PEHCI_SITD )( ( ( ULONG )struct_ptr( pentry, EHCI_ELEM_LINKS, sched_link )->phys_part ) & PHYS_PART_ADDR_MASK ) )
#define fstn_from_schedule( pentry ) ( ( PEHCI_FSTN )( ( ( ULONG )struct_ptr( pentry, EHCI_ELEM_LINKS, sched_link )->phys_part ) & PHYS_PART_ADDR_MASK ) )

#define elem_type( ptr, from_list ) ( from_list ? ( ( ( ( ULONG )struct_ptr( ptr, EHCI_ELEM_LINKS, elem_link)->phys_part ) & PHYS_PART_TYPE_MASK ) >> 1 ) \
				: ( ( ( ( ULONG )struct_ptr( ptr, EHCI_ELEM_LINKS, sched_link)->phys_part ) & PHYS_PART_TYPE_MASK ) >> 1 ) )

// #define elem_type_list_entry( pentry ) ( ( qh_from_schedule( pentry )->hw_next & 0x06 ) >> 1 )
#define elem_type_list_entry( pentry ) ( elem_type( pentry, TRUE ) )

#define get_parent_hs_hub( pDEV, parent_HUB, port_IDX ) \
{\
	parent_HUB = pDEV->parent_dev;\
	port_IDX = pdev->port_idx;\
	while( parent_HUB )\
	{\
		if( ( parent_HUB->flags & USB_DEV_CLASS_MASK ) != USB_DEV_CLASS_HUB )\
		{\
			parent_HUB = NULL;\
			break;\
		}\
		if( ( parent_HUB->flags & USB_DEV_FLAG_HIGH_SPEED ) == 0 )\
		{\
			port_IDX = parent_HUB->port_idx;\
			parent_HUB = parent_HUB->parent_dev;\
			continue;\
		}\
		break;\
	}\
}

#define init_elem_phys_part( pelnk ) RtlZeroMemory( ( PVOID )( ( ( ULONG )( pelnk )->phys_part ) & PHYS_PART_ADDR_MASK ), get_elem_phys_part_size( ( ( ( ULONG )( pelnk )->phys_part ) & 0x06 ) >> 1 ) )
#define REAL_INTERVAL	( 1 << pipe_content->interval  )

#define elem_safe_free( ptHIS, single ) \
{\
	UCHAR em_type; \
	em_type = ( UCHAR )elem_type( ptHIS, TRUE ); \
	if( ptHIS )\
	{\
		if( em_type == INIT_LIST_FLAG_QTD )\
		{\
			elem_pool_lock( qtd_pool, TRUE );\
			if( single )\
				elem_pool_free_elem( qtd_from_list_entry( ptHIS )->elem_head_link );\
			else \
				elem_pool_free_elems( qtd_from_list_entry( ptHIS )->elem_head_link );\
			elem_pool_unlock( qtd_pool, TRUE );\
		}\
		else if( em_type == INIT_LIST_FLAG_ITD )\
		{\
			elem_pool_lock( itd_pool, TRUE );\
			if( single )\
				elem_pool_free_elem( itd_from_list_entry( ptHIS )->elem_head_link );\
			else \
				elem_pool_free_elems( itd_from_list_entry( ptHIS )->elem_head_link );\
			elem_pool_unlock( itd_pool, TRUE );\
		}\
		else if( em_type == INIT_LIST_FLAG_SITD )\
		{\
			elem_pool_lock( sitd_pool, TRUE );\
			if( single )\
				elem_pool_free_elem( sitd_from_list_entry( ptHIS )->elem_head_link );\
			else \
				elem_pool_free_elems( sitd_from_list_entry( ptHIS )->elem_head_link );\
			elem_pool_unlock( sitd_pool, TRUE );\
		}\
		else if( em_type == INIT_LIST_FLAG_FSTN )\
		{\
			elem_pool_lock( fstn_pool, TRUE );\
			if( single )\
				elem_pool_free_elem( fstn_from_list_entry( ptHIS )->elem_head_link );\
			else \
				elem_pool_free_elems( fstn_from_list_entry( ptHIS )->elem_head_link );\
			elem_pool_unlock( fstn_pool, TRUE );\
		}\
		else if( em_type == INIT_LIST_FLAG_QH )\
		{\
			elem_pool_lock( qh_pool, TRUE );\
			if( single )\
				elem_pool_free_elem( qh_from_list_entry( ptHIS )->elem_head_link );\
			else \
				elem_pool_free_elems( qh_from_list_entry( ptHIS )->elem_head_link );\
			elem_pool_unlock( qh_pool, TRUE );\
		}\
	}\
}

#ifndef min
#define min( a, b ) ( ( a ) > ( b ) ? ( b ) : ( a ) )
#endif
#ifndef max
#define max( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#endif

#define CLR_RH2_PORTSTAT( port_idx, x ) \
{\
	PULONG addr; \
	addr = ( PULONG )( ehci->port_base + port_idx ); \
	status = EHCI_READ_PORT_ULONG( addr ); \
	status = ( status & 0xfffffd5 ) & ~( x ); \
	EHCI_WRITE_PORT_ULONG( addr, ( ULONG )status ); \
}

#define SET_RH2_PORTSTAT( port_idx, x ) \
{\
	PULONG addr; \
	addr = ( PULONG )( ehci->port_base + port_idx ); \
	status = EHCI_READ_PORT_ULONG( addr ); \
	if( x & PORT_PR ) \
		status = ( status & 0xffffffd1 ) | ( x ); \
	else \
		status = ( status & 0xffffffd5 ) | ( x ); \
	EHCI_WRITE_PORT_ULONG( addr, ( ULONG )status ); \
}

#define ehci_from_hcd( hCD ) ( struct_ptr( ( hCD ), EHCI_DEV, hcd_interf ) )
#define ehci_from_dev( dEV ) ( ehci_from_hcd( dEV->hcd ) )

#define ehci_copy_overlay( pQHC, pTDC ) \
{\
	LONG td_size;\
	PEHCI_QH pqh1;\
	PEHCI_QTD ptd1;\
	pqh1 = ( PEHCI_QH )( pQHC );\
	ptd1 = ( PEHCI_QTD )( pTDC );\
	td_size = get_elem_phys_part_size( INIT_LIST_FLAG_QTD );\
	( pQHC )->cur_qtd_ptr = ptd1->phys_addr;\
	RtlZeroMemory( &( pQHC )->cur_qtd, td_size );\
	( pQHC )->cur_qtd.data_toggle = ( pTDC )->data_toggle;\
	pqh1->hw_qtd_next = ptd1->phys_addr;\
	pqh1->hw_alt_next = EHCI_PTR_TERM;\
}

//declarations
typedef struct
{
    union
    {
        PUHCI_DEV uhci;
        PEHCI_DEV ehci;
    };
    PVOID context;
    ULONG ret;

} SYNC_PARAM, *PSYNC_PARAM;

PDEVICE_OBJECT
ehci_alloc(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path, ULONG bus_addr, PUSB_DEV_MANAGER dev_mgr);

BOOLEAN ehci_init_schedule(PEHCI_DEV ehci, PADAPTER_OBJECT padapter);

BOOLEAN ehci_release(PDEVICE_OBJECT pdev);

static VOID ehci_stop(PEHCI_DEV ehci);

BOOLEAN ehci_destroy_schedule(PEHCI_DEV ehci);

BOOLEAN NTAPI ehci_sync_insert_urb_schedule(PVOID context);

VOID ehci_init_hcd_interface(PEHCI_DEV ehci);

NTSTATUS ehci_rh_submit_urb(PUSB_DEV rh, PURB purb);

NTSTATUS ehci_dispatch_irp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);

VOID ehci_generic_urb_completion(PURB purb, PVOID context);

static NTSTATUS ehci_internal_submit_bulk(PEHCI_DEV ehci, PURB purb);

static NTSTATUS ehci_internal_submit_int(PEHCI_DEV ehci, PURB purb);

static NTSTATUS ehci_internal_submit_ctrl(PEHCI_DEV ehci, PURB purb);

static NTSTATUS ehci_internal_submit_iso(PEHCI_DEV ehci, PURB purb);

static ULONG ehci_scan_iso_error(PEHCI_DEV ehci, PURB purb);

BOOLEAN ehci_claim_bandwidth(PEHCI_DEV ehci, PURB purb, BOOLEAN claim_bw);    //true to claim band-width, false to free band-width

static VOID ehci_insert_bulk_schedule(PEHCI_DEV ehci, PURB purb);

#define ehci_insert_control_schedule ehci_insert_bulk_schedule

static VOID ehci_insert_int_schedule(PEHCI_DEV ehci, PURB purb);

static VOID ehci_insert_iso_schedule(PEHCI_DEV ehci, PURB purb);

#define ehci_remove_control_from_schedule ehci_remove_bulk_from_schedule

PDEVICE_OBJECT ehci_probe(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path, PUSB_DEV_MANAGER dev_mgr);

PDEVICE_OBJECT ehci_create_device(PDRIVER_OBJECT drvr_obj, PUSB_DEV_MANAGER dev_mgr);

BOOLEAN ehci_delete_device(PDEVICE_OBJECT pdev);

VOID ehci_get_capabilities(PEHCI_DEV ehci, PBYTE base);

BOOLEAN NTAPI ehci_isr(PKINTERRUPT interrupt, PVOID context);

BOOLEAN ehci_start(PHCD hcd);

extern VOID rh_timer_svc_reset_port_completion(PUSB_DEV dev, PVOID context);

extern VOID rh_timer_svc_int_completion(PUSB_DEV dev, PVOID context);

extern USB_DEV_MANAGER g_dev_mgr;

#ifndef INCLUDE_EHCI
ULONG debug_level = DBGLVL_MAXIMUM;
PDRIVER_OBJECT usb_driver_obj = NULL;

//pending endpoint pool funcs
VOID
ehci_wait_ms(PEHCI_DEV ehci, LONG ms)
{
    LARGE_INTEGER lms;
    if (ms <= 0)
        return;

    lms.QuadPart = -10 * ms;
    KeSetTimer(&ehci->reset_timer, lms, NULL);

    KeWaitForSingleObject(&ehci->reset_timer, Executive, KernelMode, FALSE, NULL);

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
#else
#define ehci_wait_ms uhci_wait_ms
extern VOID uhci_wait_ms(PEHCI_DEV ehci, LONG ms);

extern BOOLEAN init_pending_endp_pool(PUHCI_PENDING_ENDP_POOL pool);

extern BOOLEAN free_pending_endp(PUHCI_PENDING_ENDP_POOL pool, PUHCI_PENDING_ENDP pending_endp);

extern PUHCI_PENDING_ENDP alloc_pending_endp(PUHCI_PENDING_ENDP_POOL pool, LONG count);

extern BOOLEAN destroy_pending_endp_pool(PUHCI_PENDING_ENDP_POOL pool);

#endif

//end of pending endpoint pool funcs

static VOID NTAPI
ehci_cancel_pending_endp_urb(IN PVOID Parameter)
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
        //these devs are protected by purb's ref-count
        purb = (PURB) RemoveHeadList(abort_list);
        pdev = purb->pdev;
        // purb->status is set when they are added to abort_list

        ehci_generic_urb_completion(purb, purb->context);

        lock_dev(pdev, FALSE);
        pdev->ref_count--;
        unlock_dev(pdev, FALSE);
    }
    usb_free_mem(abort_list);
    return;
}

static BOOLEAN
ehci_process_pending_endp(PEHCI_DEV ehci)
{
    PUSB_DEV pdev;
    LIST_ENTRY temp_list, abort_list;
    PLIST_ENTRY pthis;
    PURB purb;
    PUSB_ENDPOINT pendp;
    NTSTATUS can_submit = STATUS_SUCCESS;
    PWORK_QUEUE_ITEM pwork_item;
    PLIST_ENTRY cancel_list;
    PUSB_DEV pparent = NULL;
    UCHAR port_idx = 0;
    BOOLEAN tt_needed;
    UCHAR hub_addr = 0;
    USE_BASIC_IRQL;

    if (ehci == NULL)
        return FALSE;

    InitializeListHead(&temp_list);
    InitializeListHead(&abort_list);

    purb = NULL;
    ehci_dbg_print(DBGLVL_MEDIUM, ("ehci_process_pending_endp(): entering..., ehci=0x%x\n", ehci));

    lock_pending_endp_list(&ehci->pending_endp_list_lock);
    while (IsListEmpty(&ehci->pending_endp_list) == FALSE)
    {

        ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_process_pending_endp(): pending_endp_list=0x%x\n",
                                        &ehci->pending_endp_list));

        tt_needed = FALSE;
        pthis = RemoveHeadList(&ehci->pending_endp_list);
        pendp = ((PUHCI_PENDING_ENDP) pthis)->pendp;
        pdev = dev_from_endp(pendp);
        lock_dev(pdev, TRUE);

        if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
        {
            unlock_dev(pdev, TRUE);
            free_pending_endp(&ehci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
            //delegate to ehci_remove_device for remiving the purb queue on the endpoint
            continue;
        }
        if ((pdev->flags & USB_DEV_FLAG_HIGH_SPEED) == 0)
        {
            // prepare split transaction
            unlock_dev(pdev, TRUE);

            // pparent won't be removed when pending_endp_list_lock is acquired.
            get_parent_hs_hub(pdev, pparent, port_idx);

            if (pparent == NULL)
            {
                TRAP();
                ehci_dbg_print(DBGLVL_MEDIUM,
                               ("ehci_process_pending_endp(): full/low speed device with no parent!!!\n"));
                free_pending_endp(&ehci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
                continue;
            }

            if (hub_lock_tt(pparent, port_idx, (UCHAR) endp_type(pendp)) == FALSE)
            {
                lock_dev(pdev, TRUE);
                if (dev_state(pdev) != USB_DEV_STATE_ZOMB)
                {
                    // reinsert the pending-endp to the list
                    InsertTailList(&temp_list, pthis);
                    unlock_dev(pdev, TRUE);
                }
                else
                {
                    // delegate to ehci_remove_device for purb removal
                    unlock_dev(pdev, TRUE);
                    free_pending_endp(&ehci->pending_endp_pool,
                                      struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
                }
                continue;
            }

            // backup the hub address for future use
            hub_addr = pparent->dev_addr;

            lock_dev(pdev, TRUE);
            if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
            {
                unlock_dev(pdev, TRUE);
                free_pending_endp(&ehci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
                hub_unlock_tt(pparent, port_idx, (UCHAR) endp_type(pendp));
                continue;
            }
            tt_needed = TRUE;
            // go on processing
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
            free_pending_endp(&ehci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
            if (tt_needed)
                hub_unlock_tt(pparent, port_idx, (UCHAR) endp_type(pendp));
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
            free_pending_endp(&ehci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
            if (tt_needed)
                hub_unlock_tt(pparent, port_idx, (UCHAR) endp_type(pendp));
            continue;
        }

        if (tt_needed)
        {
            ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->hub_addr = hub_addr;
            ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->port_idx = port_idx;
        }

        // if can_submit is STATUS_SUCCESS, the purb is inserted into the schedule
        switch (endp_type(pendp))
        {
            case USB_ENDPOINT_XFER_BULK:
            {
                can_submit = ehci_internal_submit_bulk(ehci, purb);
                break;
            }
            case USB_ENDPOINT_XFER_CONTROL:
            {
                can_submit = ehci_internal_submit_ctrl(ehci, purb);
                break;
            }
            case USB_ENDPOINT_XFER_INT:
            {
                can_submit = ehci_internal_submit_int(ehci, purb);
                break;
            }
            case USB_ENDPOINT_XFER_ISOC:
            {
                can_submit = ehci_internal_submit_iso(ehci, purb);
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
            // otherwise error or success
            free_pending_endp(&ehci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));

            if (can_submit != STATUS_SUCCESS)
            {
                //abort these URBs
                InsertTailList(&abort_list, (LIST_ENTRY *) purb);
                purb->status = can_submit;
            }
        }
        unlock_dev(pdev, TRUE);
        if (can_submit != STATUS_SUCCESS && tt_needed)
        {
            hub_unlock_tt(pparent, port_idx, (UCHAR) endp_type(pendp));
        }
    }

    if (IsListEmpty(&temp_list) == FALSE)
    {
        //re-append them to the pending_endp_list
        ListFirst(&temp_list, pthis);
        RemoveEntryList(&temp_list);
        MergeList(&ehci->pending_endp_list, pthis);
    }
    unlock_pending_endp_list(&ehci->pending_endp_list_lock);

    if (IsListEmpty(&abort_list) == FALSE)
    {
        PLIST_ENTRY pthis;
        cancel_list = (PLIST_ENTRY) usb_alloc_mem(NonPagedPool, sizeof(WORK_QUEUE_ITEM) + sizeof(LIST_ENTRY));
        ASSERT(cancel_list);

        ListFirst(&abort_list, pthis);
        RemoveEntryList(&abort_list);
        InsertTailList(pthis, cancel_list);

        pwork_item = (PWORK_QUEUE_ITEM) & cancel_list[1];

        // we do not need to worry the ehci_cancel_pending_endp_urb running when the
        // driver is unloading since purb-reference count will prevent the dev_mgr to
        // quit till all the reference count to the dev drop to zero.
        ExInitializeWorkItem(pwork_item, ehci_cancel_pending_endp_urb, (PVOID) cancel_list);
        ExQueueWorkItem(pwork_item, DelayedWorkQueue);
    }
    return TRUE;
}

NTSTATUS
ehci_submit_urb(PEHCI_DEV ehci, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
{
    int i;
    PUHCI_PENDING_ENDP pending_endp;
    NTSTATUS status;
    USE_BASIC_IRQL;

    if (ehci == NULL)
        return STATUS_INVALID_PARAMETER;

    if (pdev == NULL || pendp == NULL || purb == NULL)
    {
        // give a chance to those pending urb, especially for clearing hub tt
        ehci_process_pending_endp(ehci);
        return STATUS_INVALID_PARAMETER;
    }

    lock_pending_endp_list(&ehci->pending_endp_list_lock);
    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        status = purb->status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto LBL_OUT;
    }

    if (dev_class(pdev) == USB_DEV_CLASS_ROOT_HUB)
    {
        unlock_dev(pdev, TRUE);
        unlock_pending_endp_list(&ehci->pending_endp_list_lock);
        status = ehci_rh_submit_urb(pdev, purb);
        return status;
    }

    if (pendp)
        purb->pendp = pendp;
    else
        purb->pendp = &pdev->default_endp;

    if (dev_from_endp(purb->pendp) != pdev)
    {
        status = purb->status = STATUS_INVALID_PARAMETER;
        goto LBL_OUT;
    }

    if (endp_state(purb->pendp) == USB_ENDP_FLAG_STALL)
    {
        status = purb->status = USB_STATUS_ENDPOINT_HALTED;
        goto LBL_OUT;
    }

    if ((pdev->flags & USB_DEV_FLAG_HIGH_SPEED) == 0)
    {
        // wait one ms
        usb_wait_ms_dpc(1);
    }

    purb->pdev = pdev;
    purb->rest_bytes = purb->data_length;

    if (endp_type(purb->pendp) == USB_ENDPOINT_XFER_BULK)
        purb->bytes_to_transfer = (purb->data_length > EHCI_MAX_SIZE_TRANSFER ? EHCI_MAX_SIZE_TRANSFER : purb->data_length);    //multiple transfer for large data block
    else
        purb->bytes_to_transfer = purb->data_length;

    ehci_dbg_print(DBGLVL_MEDIUM, ("ehci_submit_urb(): bytes_to_transfer=0x%x\n", purb->bytes_to_transfer));

    purb->bytes_transfered = 0;
    InitializeListHead(&purb->trasac_list);
    purb->last_finished_td = &purb->trasac_list;
    purb->flags &= ~(URB_FLAG_STATE_MASK | URB_FLAG_IN_SCHEDULE | URB_FLAG_FORCE_CANCEL);
    purb->flags |= URB_FLAG_STATE_PENDING;


    i = IsListEmpty(&pendp->urb_list);
    InsertTailList(&pendp->urb_list, &purb->urb_link);

    pdev->ref_count++;          //for purb reference

    if (i == FALSE)
    {
        //there is purb pending, simply queue it and return
        status = purb->status = STATUS_PENDING;
        goto LBL_OUT;
    }
    else if (usb_endp_busy_count(purb->pendp) && endp_type(purb->pendp) != USB_ENDPOINT_XFER_ISOC)
    {
        //
        //No purb waiting but purb overlap not allowed,
        //so leave it in queue and return, will be scheduled
        //later
        //
        status = purb->status = STATUS_PENDING;
        goto LBL_OUT;
    }

    pending_endp = alloc_pending_endp(&ehci->pending_endp_pool, 1);
    if (pending_endp == NULL)
    {
        //panic
        status = purb->status = STATUS_UNSUCCESSFUL;
        goto LBL_OUT2;
    }

    pending_endp->pendp = purb->pendp;
    InsertTailList(&ehci->pending_endp_list, (PLIST_ENTRY) pending_endp);

    unlock_dev(pdev, TRUE);
    unlock_pending_endp_list(&ehci->pending_endp_list_lock);

    ehci_process_pending_endp(ehci);
    return STATUS_PENDING;

  LBL_OUT2:
    pdev->ref_count--;
    RemoveEntryList((PLIST_ENTRY) purb);

  LBL_OUT:
    unlock_dev(pdev, TRUE);
    unlock_pending_endp_list(&ehci->pending_endp_list_lock);
    ehci_process_pending_endp(ehci);
    return status;
}

static NTSTATUS
ehci_set_error_code(PURB purb, ULONG raw_status)
{
    PURB_HS_PIPE_CONTENT pipe_content;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;

    //test if the purb is canceled
    if (purb->flags & URB_FLAG_FORCE_CANCEL)
    {
        purb->status = STATUS_CANCELLED;
    }
    else if (raw_status == 0)
        purb->status = STATUS_SUCCESS;

    else if (pipe_content->trans_type == USB_ENDPOINT_XFER_INT ||
             pipe_content->trans_type == USB_ENDPOINT_XFER_BULK ||
             pipe_content->trans_type == USB_ENDPOINT_XFER_CONTROL)
    {

        if (raw_status & QTD_STS_BABBLE)
            purb->status = USB_STATUS_DATA_OVERRUN;

        else if (raw_status & QTD_STS_HALT)
            purb->status = USB_STATUS_ENDPOINT_HALTED;

        else if (raw_status & QTD_STS_DBE)
            purb->status = USB_STATUS_BUFFER_OVERRUN;

        else if (raw_status & QTD_STS_XACT)
            purb->status = USB_STATUS_CRC;      // crc is included in xact err.

        else if (raw_status & QTD_STS_MMF)
            purb->status = USB_STATUS_BTSTUFF;

        else
            purb->status = STATUS_UNSUCCESSFUL;
    }
    else if (pipe_content->trans_type == USB_ENDPOINT_XFER_ISOC)
    {
        if (pipe_content->speed_high)
        {
            if (raw_status & ITD_STS_BUFERR)
                purb->status = USB_STATUS_BUFFER_OVERRUN;

            else if (raw_status & ITD_STS_BABBLE)
                purb->status = USB_STATUS_BABBLE_DETECTED;

            else if (raw_status & ITD_STS_XACTERR)      // Xact Err
                purb->status = USB_STATUS_CRC;

            else
                purb->status = STATUS_UNSUCCESSFUL;

        }
        else
        {
            if (raw_status & SITD_STS_ERR)      // ERR is received from hub's tt
                purb->status = USB_STATUS_ERROR;

            else if (raw_status & SITD_STS_DBE)
                purb->status = USB_STATUS_BUFFER_OVERRUN;

            else if (raw_status & SITD_STS_BABBLE)
                purb->status = USB_STATUS_BABBLE_DETECTED;

            else if (raw_status & SITD_STS_XACTERR)     // Xact Error
                purb->status = USB_STATUS_CRC;

            else if (raw_status & SITD_STS_MISSFRM)     // missing microframe
                purb->status = USB_STATUS_DATA_TOGGLE_MISMATCH;

            else
                purb->status = STATUS_UNSUCCESSFUL;
        }
    }
    if (purb->status != STATUS_SUCCESS)
    {
        hcd_dbg_print(DBGLVL_MEDIUM, ("ehci_set_error_code(): error status 0x%x\n", raw_status));
    }
    return purb->status;
}

static BOOLEAN NTAPI
ehci_sync_remove_urb_finished(PVOID context)
{
    PEHCI_DEV ehci;
    PLIST_ENTRY pthis, pnext, ptemp;
    PURB purb;
    PSYNC_PARAM pparam;

    pparam = (PSYNC_PARAM) context;
    ehci = pparam->ehci;
    ptemp = (PLIST_ENTRY) pparam->context;

    if (ehci == NULL)
    {
        return (UCHAR) (pparam->ret = FALSE);
    }

    ListFirst(&ehci->urb_list, pthis);
    while (pthis)
    {
        //remove urbs not in the schedule
        ListNext(&ehci->urb_list, pthis, pnext);
        purb = (PURB) pthis;

        if ((purb->flags & URB_FLAG_IN_SCHEDULE) == 0)
        {
            //finished or canceled( not applied for split bulk ).
            RemoveEntryList(pthis);
            InsertTailList(ptemp, pthis);
        }
        pthis = pnext;
    }
    pparam->ret = TRUE;
    return (UCHAR) TRUE;
}

VOID NTAPI
ehci_dpc_callback(PKDPC dpc, PVOID context, PVOID sysarg1, PVOID sysarg2)
{
    PEHCI_DEV ehci;

    LIST_HEAD temp_list;
    PLIST_ENTRY pthis, pnext;
    PURB purb;
    PEHCI_QH pqh;
    PEHCI_QTD ptd;
    PUHCI_PENDING_ENDP pending_endp;
    PUSB_DEV pdev;
    PUSB_ENDPOINT pendp;

    BOOLEAN finished;
    LONG i;
    ULONG ehci_status, urb_status;

    SYNC_PARAM sync_param;
    UCHAR ep_type;
    USE_BASIC_NON_PENDING_IRQL;

    ehci = (PEHCI_DEV) context;
    if (ehci == NULL)
        return;

    ehci_status = (ULONG) sysarg1;

    InitializeListHead(&temp_list);

    sync_param.ehci = ehci;
    sync_param.context = (PVOID) & temp_list;

    ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_dpc_callback(): entering..., ehci=0x%x\n", ehci));
    //remove finished purb from ehci's purb-list
    KeSynchronizeExecution(ehci->pdev_ext->ehci_int, ehci_sync_remove_urb_finished, &sync_param);

    //release resources( itds, sitds, fstns, tds, and qhs ) allocated for the purb
    while (IsListEmpty(&temp_list) == FALSE)
    {
        //not in any public queue, if do not access into dev, no race
        //condition will occur
        purb = (PURB) RemoveHeadList(&temp_list);
        urb_status = purb->status;
        ep_type = endp_type(purb->pendp);

        if (ep_type == USB_ENDPOINT_XFER_ISOC)
        {
            // collect error for iso transfer
            urb_status = ehci_scan_iso_error(ehci, purb);
        }

        //the only place we do not use this lock on non-pending-endp-list data
        KeAcquireSpinLockAtDpcLevel(&ehci->pending_endp_list_lock);
        while (IsListEmpty(&purb->trasac_list) == FALSE)
        {
            UCHAR em_type;
            pthis = RemoveHeadList(&purb->trasac_list);
            em_type = (UCHAR) elem_type(pthis, TRUE);

            if (em_type == INIT_LIST_FLAG_QH)
            {
                pqh = qh_from_list_entry(pthis);
                elem_safe_free(pthis, TRUE);
            }
            else
            {
                //must be an itd, sitd chain
                InsertHeadList(&purb->trasac_list, pthis);
                for(i = 0, purb->bytes_transfered = 0; i < purb->td_count; i++)
                {
                    PEHCI_QTD_CONTENT ptdc = NULL;
                    PEHCI_ITD_CONTENT pitdc;
                    PEHCI_SITD_CONTENT psitdc;

                    em_type = (UCHAR) elem_type(pthis, TRUE);

                    // accumulate data transfered in tds
                    if (em_type == INIT_LIST_FLAG_QTD)
                    {
                        ptd = qtd_from_list_entry(pthis);
                        ptdc = (PEHCI_QTD_CONTENT) ptd;
                        if ((ptdc->status & QTD_STS_ACTIVE) == 0 && ((ptdc->status & QTD_ANY_ERROR) == 0))
                            purb->bytes_transfered += ptd->bytes_to_transfer;
                    }
                    else if (em_type == INIT_LIST_FLAG_ITD)
                    {
                        int j;
                        pitdc = (PEHCI_ITD_CONTENT) itd_from_list_entry(pthis);
                        for(j = 0; j < 8; j++)
                        {
                            if ((pitdc->status_slot[j].status & ITD_STS_ACTIVE) == 0
                                && (pitdc->status_slot[j].status & ITD_ANY_ERROR) == 0)
                                purb->bytes_transfered += ptdc->bytes_to_transfer;
                        }
                    }
                    else if (em_type == INIT_LIST_FLAG_SITD)
                    {
                        psitdc = (PEHCI_SITD_CONTENT) sitd_from_list_entry(pthis);
                        if ((psitdc->status & SITD_STS_ACTIVE) == 0 && (psitdc->status & SITD_ANY_ERROR) == 0)
                            purb->bytes_transfered += ptdc->bytes_to_transfer;
                    }
                    ListNext(&purb->trasac_list, pthis, pnext);
                    pthis = pnext;
                }

                // check to see if an fstn is there
                ListFirstPrev(&purb->trasac_list, pthis);
                if (elem_type(pthis, TRUE) == INIT_LIST_FLAG_FSTN)
                {
                    RemoveEntryList(pthis);
                    elem_safe_free(pthis, TRUE);
                }

                ListFirst(&purb->trasac_list, pthis);
                RemoveEntryList(&purb->trasac_list);

                // free the tds
                elem_safe_free(pthis, FALSE);

                //termination condition
                InitializeListHead(&purb->trasac_list);
                purb->last_finished_td = NULL;
            }
        }

        if (ep_type == USB_ENDPOINT_XFER_ISOC || ep_type == USB_ENDPOINT_XFER_INT)
            ehci_claim_bandwidth(ehci, purb, FALSE);    //release band-width

        KeReleaseSpinLockFromDpcLevel(&ehci->pending_endp_list_lock);

        ehci_set_error_code(purb, urb_status);

        pdev = dev_from_endp(purb->pendp);
        pendp = purb->pendp;

        // perform clear tt buffer if error on full/low bulk/control pipe
        if (ep_type == USB_ENDPOINT_XFER_BULK || ep_type == USB_ENDPOINT_XFER_CONTROL)
        {
            PURB_HS_PIPE_CONTENT pipe_content;
            PUSB_DEV phub;
            UCHAR port_idx;

            get_parent_hs_hub(pdev, phub, port_idx);
            pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;

            if (pipe_content->speed_high == 0 && purb->status != STATUS_SUCCESS)
            {
                // lets schedule an event to clear the tt buffer
                hub_post_clear_tt_event(phub, port_idx, purb->pipe);
            }
            else if (pipe_content->speed_high == 0)
            {
                if (phub == NULL)
                    TRAP();
                else
                {
                    // release tt if no error
                    hub_unlock_tt(phub, (UCHAR) port_idx, (UCHAR) pipe_content->trans_type);
                }
            }
        }

        finished = TRUE;

        //since the ref_count for the purb is not released, we can safely have one
        //pointer to dev

        if (purb->status == USB_STATUS_BABBLE_DETECTED)
        {
            usb_dbg_print(DBGLVL_MEDIUM,
                          ("ehci_dpc_callback(): alert!!!, babble detected, severe error, reset the whole bus\n"));
            // ehci_start( ehci );
        }

        if (ehci_status & STS_HALT)     //&& !ehci->is_suspended
        {
            ehci_start(&ehci->hcd_interf);
        }

        //this will let the new request in ehci_generic_urb_completion to this endp
        //be processed rather than queued in the pending_endp_list
        lock_dev(pdev, TRUE);
        usb_endp_busy_count_dec(pendp);
        unlock_dev(pdev, TRUE);

        if (usb_success(purb->status) == FALSE)
        {
            // set error code and complete the purb and purb is invalid from this point
            ehci_generic_urb_completion(purb, purb->context);
        }
        else
        {
            if (ep_type == USB_ENDPOINT_XFER_BULK)
            {
                purb->rest_bytes -= purb->bytes_transfered;
                if (purb->rest_bytes)
                {
                    finished = FALSE;
                }
                else
                {
                    ehci_generic_urb_completion(purb, purb->context);
                }
            }
            else
            {
                ehci_generic_urb_completion(purb, purb->context);
                // DbgBreakPoint();
                //purb is now invalid
            }
        }

        KeAcquireSpinLockAtDpcLevel(&ehci->pending_endp_list_lock);
        lock_dev(pdev, TRUE);

        if (finished)
            pdev->ref_count--;

        if (urb_status && ((ep_type == USB_ENDPOINT_XFER_BULK) || (ep_type == USB_ENDPOINT_XFER_INT)))
        {
            // error on int or bulk pipe, cleared in usb_reset_pipe_completion
            pendp->flags &= ~USB_ENDP_FLAG_STAT_MASK;
            pendp->flags |= USB_ENDP_FLAG_STALL;
        }

        if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
        {
            unlock_dev(pdev, TRUE);
            KeReleaseSpinLockFromDpcLevel(&ehci->pending_endp_list_lock);
            if (finished == FALSE)
            {

                purb->status = STATUS_DEVICE_DOES_NOT_EXIST;
                ehci_generic_urb_completion(purb, purb->context);

                lock_dev(pdev, TRUE);
                pdev->ref_count--;
                unlock_dev(pdev, TRUE);
            }
            continue;
        }

        if (finished && IsListEmpty(&pendp->urb_list) == TRUE)
        {
            unlock_dev(pdev, TRUE);
            KeReleaseSpinLockFromDpcLevel(&ehci->pending_endp_list_lock);
            continue;
        }
        else if (finished == TRUE)
        {
            //has purb in the endp's purb-list
            if (usb_endp_busy_count(pendp) > 0)
            {
                //the urbs still have chance to be sheduled but not this time
                unlock_dev(pdev, TRUE);
                KeReleaseSpinLockFromDpcLevel(&ehci->pending_endp_list_lock);
                continue;
            }
        }

        if (finished == FALSE)
        {
            //a split bulk transfer, ( not the high speed split transfer )
            purb->bytes_transfered = 0;
            purb->bytes_to_transfer =
                EHCI_MAX_SIZE_TRANSFER > purb->rest_bytes ? purb->rest_bytes : EHCI_MAX_SIZE_TRANSFER;

            //the purb is not finished
            purb->flags &= ~URB_FLAG_STATE_MASK;
            purb->flags |= URB_FLAG_STATE_PENDING;

            InsertHeadList(&pendp->urb_list, (PLIST_ENTRY) purb);
        }

        pending_endp = alloc_pending_endp(&ehci->pending_endp_pool, 1);
        pending_endp->pendp = pendp;
        InsertTailList(&ehci->pending_endp_list, &pending_endp->endp_link);

        unlock_dev(pdev, TRUE);
        KeReleaseSpinLockFromDpcLevel(&ehci->pending_endp_list_lock);
    }

    //ah...exhausted, let's find some in the pending_endp_list to rock
    ehci_process_pending_endp(ehci);
    return;
}

static BOOLEAN NTAPI
ehci_sync_cancel_urbs_dev(PVOID context)
{
    //cancel all the urbs on one dev
    PEHCI_DEV ehci;
    PUSB_DEV pdev, dest_dev;
    PSYNC_PARAM sync_param;
    PLIST_ENTRY pthis, pnext;
    LONG count;

    sync_param = (PSYNC_PARAM) context;
    dest_dev = (PUSB_DEV) sync_param->context;
    ehci = sync_param->ehci;

    if (ehci == NULL || dest_dev == NULL)
    {
        return (UCHAR) (sync_param->ret = FALSE);
    }
    count = 0;
    ListFirst(&ehci->urb_list, pthis);
    while (pthis)
    {
        pdev = dev_from_endp(((PURB) pthis)->pendp);
        if (pdev == dest_dev)
        {
            ((PURB) pthis)->flags |= URB_FLAG_FORCE_CANCEL;
        }
        ListNext(&ehci->urb_list, pthis, pnext);
        pthis = pnext;
        count++;
    }

    if (count)
    {
        // signal an int for further process
        press_doorbell(ehci);
    }
    return (UCHAR) (sync_param->ret = TRUE);
}

BOOLEAN
ehci_remove_device(PEHCI_DEV ehci, PUSB_DEV dev)
{
    PUHCI_PENDING_ENDP ppending_endp;
    PLIST_ENTRY pthis, pnext;
    PURB purb;
    LIST_HEAD temp_list;
    int i, j, k;
    SYNC_PARAM sync_param;

    USE_BASIC_IRQL;

    if (ehci == NULL || dev == NULL)
        return FALSE;

    InitializeListHead(&temp_list);

    //free pending endp that has purb queued from pending endp list
    lock_pending_endp_list(&ehci->pending_endp_list_lock);

    ListFirst(&ehci->pending_endp_list, pthis);

    while (pthis)
    {
        ppending_endp = (PUHCI_PENDING_ENDP) pthis;
        ListNext(&ehci->pending_endp_list, pthis, pnext);
        if (dev_from_endp(ppending_endp->pendp) == dev)
        {
            RemoveEntryList(pthis);
            free_pending_endp(&ehci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
        }
        pthis = pnext;
    }
    unlock_pending_endp_list(&ehci->pending_endp_list_lock);

    //cancel all the urbs in the purb-list
    sync_param.ehci = ehci;
    sync_param.context = (PVOID) dev;

    KeSynchronizeExecution(ehci->pdev_ext->ehci_int, ehci_sync_cancel_urbs_dev, &sync_param);

    //cancel all the purb in the endp's purb-list
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
                ehci_generic_urb_completion(purb, purb->context);
            }
        }
    }

    lock_dev(dev, FALSE) dev->ref_count -= k;
    unlock_dev(dev, FALSE);

    return TRUE;
}

static BOOLEAN
ehci_insert_urb_schedule(PEHCI_DEV ehci, PURB purb)
// must have dev_lock( ehci_process_pending_endp ) and frame_list_lock acquired
{
    PURB_HS_PIPE_CONTENT pipe_content;

    if (ehci == NULL || purb == NULL)
        return FALSE;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    switch (pipe_content->trans_type)
    {
        case USB_ENDPOINT_XFER_CONTROL:
            ehci_insert_control_schedule(ehci, purb);
            break;
        case USB_ENDPOINT_XFER_BULK:
            ehci_insert_bulk_schedule(ehci, purb);
            break;
        case USB_ENDPOINT_XFER_INT:
            ehci_insert_int_schedule(ehci, purb);
            break;
        case USB_ENDPOINT_XFER_ISOC:
            ehci_insert_iso_schedule(ehci, purb);
            break;
        default:
            return FALSE;
    }

    purb->flags &= ~URB_FLAG_STATE_MASK;
    purb->flags |= URB_FLAG_STATE_IN_PROCESS | URB_FLAG_IN_SCHEDULE;
    InsertTailList(&ehci->urb_list, (PLIST_ENTRY) purb);

    return TRUE;
}

static BOOLEAN
ehci_insert_tds_qh(PEHCI_DEV ehci, PEHCI_QH pqh, PEHCI_QTD td_chain)
{
    if (pqh == NULL || td_chain == NULL)
        return FALSE;

    UNREFERENCED_PARAMETER(ehci);

    ehci_copy_overlay((PEHCI_QH_CONTENT) pqh, (PEHCI_QTD_CONTENT) td_chain);
    InsertTailList(&td_chain->elem_head_link->elem_link, &pqh->elem_head_link->elem_link);
    return TRUE;
}

static BOOLEAN
ehci_insert_qh_urb(PURB purb, PEHCI_QH pqh)
{
    PLIST_ENTRY pthis, pnext;
    if (pqh == NULL || purb == NULL)
        return FALSE;

    InsertTailList(&pqh->elem_head_link->elem_link, &purb->trasac_list);
    ListFirst(&purb->trasac_list, pthis) while (pthis)
    {
        // note: fstn may in this chain
        struct_ptr(pthis, EHCI_ELEM_LINKS, elem_link)->purb = purb;
        ListNext(&purb->trasac_list, pthis, pnext);
        pthis = pnext;
    }
    return TRUE;
}

#define calc_td_count( pURB, start_aDDR, td_coUNT ) \
{\
	LONG i, j, k;\
	td_coUNT = 0;\
	k = ( ( pURB )->bytes_to_transfer + max_packet_size - 1 ) / max_packet_size;\
	if( k != 0 )\
	{\
		LONG packets_per_td, packets_per_page;\
		packets_per_td = EHCI_QTD_MAX_TRANS_SIZE / max_packet_size;\
		packets_per_page = PAGE_SIZE / max_packet_size;\
		i = ( ( LONG )&( pURB )->data_buffer[ ( start_aDDR ) ] ) & ( PAGE_SIZE - 1 );\
		if( i )\
		{\
			i = PAGE_SIZE - i;\
			j = i & ( max_packet_size - 1 );\
			k -= ( EHCI_QTD_MAX_TRANS_SIZE - PAGE_SIZE + i - j ) / max_packet_size;\
			if( k < 0 )\
				td_coUNT = 1;\
			else\
			{\
				if( j )\
					i = packets_per_td - packets_per_page;\
				else\
					i = packets_per_td;\
				td_coUNT = 1 + ( k + i - 1 ) / i; \
			}\
		}\
		else\
		{\
			td_coUNT = ( k + packets_per_td - 1 ) / packets_per_td;\
		}\
	}\
}

static BOOLEAN
ehci_fill_td_buf_ptr(PURB purb, LONG start_addr,        // start idx into purb->data_buffer
                     PLIST_ENTRY td_list, LONG td_count, ULONG toggle)
// fill the tds' bytes_to_transfer and hw_buf, return next toggle value: true 1, false 0
{
    LONG i, j, k, data_load;
    LONG packets_per_td, packets_per_page, bytes_to_transfer, max_packet_size;
    PLIST_ENTRY pthis, pnext;
    PEHCI_QTD_CONTENT ptdc;
    PEHCI_QTD ptd;
    PVOID ptr;

    if (purb == NULL || td_list == NULL || td_count == 0)
        return toggle;

    max_packet_size = 1 << ((PURB_HS_PIPE_CONTENT) & purb->pipe)->max_packet_size;
    packets_per_td = EHCI_QTD_MAX_TRANS_SIZE / max_packet_size;
    packets_per_page = PAGE_SIZE / max_packet_size;

    pthis = td_list;
    bytes_to_transfer = purb->bytes_to_transfer;

    i = ((LONG) & (purb)->data_buffer[(start_addr)]) & (PAGE_SIZE - 1);
    if (i)
    {
        i = PAGE_SIZE - i;
        j = i & (max_packet_size - 1);
    }
    else
    {
        i = j = 0;
    }

    while (bytes_to_transfer)
    {
        ptd = qtd_from_list_entry(pthis);
        ptd->hw_buf[0] = MmGetPhysicalAddress(&purb->data_buffer[start_addr]).LowPart;
        ptdc = (PEHCI_QTD_CONTENT) ptd;

        if (i != 0)
        {
            data_load = (LONG) (EHCI_QTD_MAX_TRANS_SIZE - PAGE_SIZE + i - j) < bytes_to_transfer
                ? (LONG) (EHCI_QTD_MAX_TRANS_SIZE - PAGE_SIZE + i - j) : bytes_to_transfer;

            ptdc->bytes_to_transfer = (USHORT) data_load;
            ptd->bytes_to_transfer = (USHORT) data_load;

            // subtract the header part
            data_load -= (i < data_load ? i : data_load);

            for(k = 1; data_load > 0; k++)
            {
                ptr = &purb->data_buffer[start_addr + i + (k - 1) * PAGE_SIZE];
                ptr = (PVOID) (((ULONG) ptr) & ~(PAGE_SIZE - 1));
                ptd->hw_buf[k] = MmGetPhysicalAddress(ptr).LowPart;
                data_load -= PAGE_SIZE < data_load ? PAGE_SIZE : data_load;
            }
        }
        else
        {
            // aligned on page boundary
            data_load = EHCI_QTD_MAX_TRANS_SIZE < bytes_to_transfer
                ? EHCI_QTD_MAX_TRANS_SIZE : bytes_to_transfer;

            ptdc->bytes_to_transfer = (USHORT) data_load;
            ptd->bytes_to_transfer = (USHORT) data_load;

            data_load -= (PAGE_SIZE < data_load ? PAGE_SIZE : data_load);

            for(k = 1; data_load > 0; k++)
            {
                ptr = &purb->data_buffer[start_addr + k * PAGE_SIZE];
                ptr = (PVOID) (((ULONG) ptr) & ~(PAGE_SIZE - 1));
                ptd->hw_buf[k] = MmGetPhysicalAddress(ptr).LowPart;
                data_load -= PAGE_SIZE < data_load ? PAGE_SIZE : data_load;
            }
        }
        ptdc->data_toggle = toggle;
        if (((ptdc->bytes_to_transfer + max_packet_size - 1) / max_packet_size) & 1)
        {
            //only odd num of transactions has effect
            toggle ^= 1;
        }
        start_addr += ptdc->bytes_to_transfer;
        bytes_to_transfer -= ptdc->bytes_to_transfer;
        ListNext(td_list, pthis, pnext);
        pthis = pnext;
        i = j;
    }
    return toggle;
}

static NTSTATUS
ehci_internal_submit_bulk(PEHCI_DEV ehci, PURB purb)
//
// assume that the purb has its rest_bytes and bytes_to_transfer set
// and bytes_transfered is zeroed.
// dev_lock must be acquired outside
// purb comes from dev's endpoint purb-list. it is already removed from
// the endpoint purb-list.
//
{

    LONG max_packet_size, td_count, offset, bytes_to_transfer;
    PBYTE start_addr;
    PEHCI_QTD ptd;
    PEHCI_QH pqh;
    LIST_ENTRY td_list, *pthis, *pnext;
    BOOLEAN old_toggle, toggle, ret;
    UCHAR pid;
    LONG i, j;
    PURB_HS_PIPE_CONTENT pipe_content;
    PEHCI_QTD_CONTENT ptdc;
    PEHCI_QH_CONTENT pqhc;
    PEHCI_ELEM_LINKS pelnk;

    if (ehci == NULL || purb == NULL)
        return STATUS_INVALID_PARAMETER;

    max_packet_size = endp_max_packet_size(purb->pendp);
    if (purb->bytes_to_transfer == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    start_addr = &purb->data_buffer[purb->data_length - purb->rest_bytes];
    calc_td_count(purb, purb->data_length - purb->rest_bytes, td_count);

    elem_pool_lock(qtd_pool, TRUE);
    pelnk = elem_pool_alloc_elems(qtd_pool, td_count);
    elem_pool_unlock(qtd_pool, TRUE);

    if (pelnk == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }
    ptd = (PEHCI_QTD) ((ULONG) pelnk->phys_part & PHYS_PART_ADDR_MASK);

    InitializeListHead(&td_list);
    InsertTailList(&ptd->elem_head_link->elem_link, &td_list);

    ListFirst(&td_list, pthis);
    ListNext(&td_list, pthis, pnext);

    offset = 0;

    old_toggle = toggle = (purb->pendp->flags & USB_ENDP_FLAG_DATATOGGLE) ? TRUE : FALSE;
    bytes_to_transfer = purb->bytes_to_transfer;
    ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_internal_submit_bulk():dev toggle=%d\n", toggle));

    for(i = 1; i < 16; i++)
    {
        if ((max_packet_size >> i) == 0)
            break;
    }
    i--;
    i &= 0xf;

    purb->pipe = 0;
    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    pipe_content->max_packet_size = i;
    pipe_content->endp_addr = endp_num(purb->pendp);
    pipe_content->dev_addr = dev_from_endp(purb->pendp)->dev_addr;
    pipe_content->trans_dir = endp_dir(purb->pendp);
    pipe_content->trans_type = USB_ENDPOINT_XFER_BULK;
    pipe_content->data_toggle = toggle;
    pipe_content->speed_high = (dev_from_endp(purb->pendp)->flags & USB_DEV_FLAG_HIGH_SPEED) ? 1 : 0;
    pipe_content->speed_low = (dev_from_endp(purb->pendp)->flags & USB_DEV_FLAG_LOW_SPEED) ? 1 : 0;

    pid = (((ULONG) purb->pendp->pusb_endp_desc->bEndpointAddress & USB_DIR_IN) ? QTD_PID_IN : QTD_PID_OUT);

    i = ((ULONG) start_addr) & (PAGE_SIZE - 1); // header part within first page
    if (i)
    {
        i = PAGE_SIZE - i;
        if (i < purb->bytes_to_transfer)
            j = i & (max_packet_size - 1);
        else
            j = 0;
    }
    else
        j = 0;

    // fill the page pointer and toggle

    toggle = ehci_fill_td_buf_ptr(purb, purb->data_length - purb->rest_bytes, pthis, td_count, toggle);
    while (pthis)
    {
        ptd = qtd_from_list_entry(pthis);
        ptdc = (PEHCI_QTD_CONTENT) ptd;

        // ptdc->alt_terminal = 1;
        // ptdc->alt_qtd = 0;
        ptd->hw_alt_next = EHCI_PTR_TERM;
        ptdc->pid = pid;

        // ptd->elem_head_link->purb = purb; will be filled later
        ptdc->err_count = 3;
        ptdc->status = 0x80;    // active, and do_start_split for split transfer
        ptdc->cur_page = 0;
        // ptdc->data_toggle = toggle;

        if (pnext)
        {
            ptd->hw_next = qtd_from_list_entry(pnext)->phys_addr;
        }
        else
        {
            //Last one, enable ioc and short packet detect if necessary
            ptd->hw_next = EHCI_PTR_TERM;
            ptdc->ioc = TRUE;
            if (bytes_to_transfer < max_packet_size && (pid == QTD_PID_IN))
            {
                //ptd->status |= TD_CTRL_SPD;
            }
        }

        pthis = pnext;

        if (pthis)
            ListNext(&td_list, pthis, pnext);
    }

    ListFirst(&td_list, pthis);
    RemoveEntryList(&td_list);

    elem_pool_lock(qh_pool, TRUE);
    pqh = (PEHCI_QH) ((ULONG) elem_pool_alloc_elem(qh_pool)->phys_part & PHYS_PART_ADDR_MASK);
    elem_pool_unlock(qh_pool, TRUE);

    if (pqh == NULL)
    {
        // free the qtds
        elem_safe_free(pthis, TRUE);
        return STATUS_NO_MORE_ENTRIES;

    }

    purb->td_count = td_count;
    pqhc = (PEHCI_QH_CONTENT) pqh;
    pqh->hw_next = EHCI_PTR_TERM;       // filled later
    pqhc->dev_addr = pipe_content->dev_addr;
    pqhc->inactive = 0;
    pqhc->endp_addr = pipe_content->endp_addr;
    pqhc->data_toggle = 0;      //pipe_content->data_toggle;
    pqhc->is_async_head = 0;
    pqhc->max_packet_size = (1 << pipe_content->max_packet_size);
    pqhc->is_ctrl_endp = 0;
    pqhc->reload_counter = EHCI_NAK_RL_COUNT;

    if (pipe_content->speed_high)
        pqhc->endp_spd = USB_SPEED_HIGH;
    else if (pipe_content->speed_low)
        pqhc->endp_spd = USB_SPEED_LOW;
    else
        pqhc->endp_spd = USB_SPEED_FULL;

    pqh->hw_info2 = 0;
    pqhc->mult = 1;
    pqh->hw_current = 0;
    pqh->hw_qtd_next = 0;       // filled later
    pqh->hw_alt_next = EHCI_PTR_TERM;
    pqh->hw_token = 0;          //indicate to advance queue before execution

    if (!pipe_content->speed_high)
    {
        pqhc->hub_addr = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->hub_addr;
        pqhc->port_idx = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->port_idx;
    }

    ptd = qtd_from_list_entry(pthis);
    ehci_insert_tds_qh(ehci, pqh, ptd);
    ehci_insert_qh_urb(purb, pqh);
    purb->pendp->flags =
        (purb->pendp->flags & ~USB_ENDP_FLAG_DATATOGGLE) | (toggle ? USB_ENDP_FLAG_DATATOGGLE : 0);
    usb_endp_busy_count_inc(purb->pendp);
    ehci_insert_urb_to_schedule(ehci, purb, ret);

    if (ret == FALSE)
    {
        // undo all we have done
        ListFirst(&pqh->elem_head_link->elem_link, pthis);

        RemoveEntryList(&purb->trasac_list);
        RemoveEntryList(&pqh->elem_head_link->elem_link);       //remove qh from td_chain

        elem_safe_free(pthis, FALSE);
        elem_safe_free(&pqh->elem_head_link->elem_link, TRUE);

        InitializeListHead(&purb->trasac_list);
        // usb_endp_busy_count_dec( purb->pendp ); // the decrement is done in the dpc callback
        purb->pendp->flags =
            (purb->pendp->flags & ~USB_ENDP_FLAG_DATATOGGLE) | (old_toggle ? USB_ENDP_FLAG_DATATOGGLE : 0);
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
ehci_internal_submit_ctrl(PEHCI_DEV ehci, PURB purb)
{

    LIST_ENTRY td_list, *pthis, *pnext;
    LONG i, td_count;
    LONG toggle;
    LONG max_packet_size, bytes_to_transfer, bytes_rest, start_idx;

    PEHCI_QTD ptd;
    PEHCI_QH pqh;
    PEHCI_QH_CONTENT pqhc;
    UCHAR dev_addr;
    BOOLEAN ret;
    PURB_HS_PIPE_CONTENT pipe_content;
    PEHCI_QTD_CONTENT ptdc;
    PEHCI_ELEM_LINKS pelnk;
    PUSB_DEV pdev;

    if (ehci == NULL || purb == NULL)
        return STATUS_INVALID_PARAMETER;

    bytes_rest = purb->rest_bytes;
    bytes_to_transfer = purb->bytes_to_transfer;
    max_packet_size = endp_max_packet_size(purb->pendp);
    start_idx = purb->data_length - purb->rest_bytes;

    calc_td_count(purb, start_idx, td_count);
    td_count += 2;              // add setup td and handshake td

    elem_pool_lock(qtd_pool, TRUE);
    pelnk = elem_pool_alloc_elems(qtd_pool, td_count);
    elem_pool_unlock(qtd_pool, TRUE);

    if (pelnk == NULL)
    {
        return STATUS_NO_MORE_ENTRIES;
    }

    InsertTailList(&pelnk->elem_link, &td_list);
    ListFirst(&td_list, pthis);
    ListNext(&td_list, pthis, pnext);

    ptd = qtd_from_list_entry(pthis);

    pdev = dev_from_endp(purb->pendp);
    dev_addr = pdev->dev_addr;

    if (dev_state(pdev) <= USB_DEV_STATE_RESET) //only valid for control transfer
        dev_addr = 0;

    usb_dbg_print(DBGLVL_MAXIMUM, ("ehci_internal_submit_ctrl(): dev_addr =0x%x\n", dev_addr));

    // fill the setup packet
    ptdc = (PEHCI_QTD_CONTENT) ptd;
    ptd->hw_next = qtd_from_list_entry(pnext)->phys_addr;
    ptd->hw_alt_next = EHCI_PTR_TERM;
    ptdc->status = 0x80;        // active
    ptdc->pid = QTD_PID_SETUP;
    ptdc->err_count = 3;
    ptdc->cur_page = 0;
    ptdc->ioc = 0;
    ptdc->bytes_to_transfer = sizeof(USB_CTRL_SETUP_PACKET);
    ptdc->data_toggle = 0;
    ptd->hw_buf[0] = MmGetPhysicalAddress(purb->setup_packet).LowPart;

    for(i = 1; i < 16; i++)
    {
        if ((max_packet_size >> i) == 0)
            break;
    }
    i--;
    i &= 0xf;

    purb->pipe = 0;
    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    pipe_content->max_packet_size = i;
    pipe_content->endp_addr = endp_num(purb->pendp);
    pipe_content->dev_addr = dev_addr;
    pipe_content->speed_low = (pdev->flags & USB_DEV_FLAG_LOW_SPEED) ? 1 : 0;
    pipe_content->speed_high = (pdev->flags & USB_DEV_FLAG_HIGH_SPEED) ? 1 : 0;
    pipe_content->trans_type = USB_ENDPOINT_XFER_CONTROL;

    pthis = pnext;
    ListNext(&td_list, pthis, pnext);

    // all the tds's toggle and data_buffer pointer is filled here
    toggle = 1;
    ehci_fill_td_buf_ptr(purb, start_idx, pthis, td_count - 2, toggle);

    for(i = 0; ((i < td_count - 2) && pthis); i++)
    {
        //construct tds for DATA packets of data stage.
        ptd = qtd_from_list_entry(pthis);
        ptdc = (PEHCI_QTD_CONTENT) ptd;
        ptd->hw_alt_next = EHCI_PTR_TERM;
        ptdc->status = 0x80;    // active and startXSplit
        ptdc->pid = ((purb->setup_packet[0] & USB_DIR_IN) ? QTD_PID_IN : QTD_PID_OUT);
        ptdc->err_count = 3;
        ptdc->cur_page = 0;
        ptdc->ioc = 0;

        if (pnext)
            ptd->hw_next = qtd_from_list_entry(pnext)->phys_addr;
        else
            ptd->hw_next = EHCI_PTR_TERM;

        pthis = pnext;
        if (pthis)
            ListNext(&td_list, pthis, pnext);
    }

    if (pthis)
        ptd->hw_next = qtd_from_list_entry(pthis)->phys_addr;
    else
        TRAP();

    // ListFirstPrev( &td_list, pthis );
    ptd = qtd_from_list_entry(pthis);

    //the last is an IN transaction
    ptdc = (PEHCI_QTD_CONTENT) ptd;
    ptd->hw_alt_next = EHCI_PTR_TERM;
    ptdc->status = 0x80;
    ptdc->pid = ((td_count > 2)
                 ? ((purb->setup_packet[0] & USB_DIR_IN) ? QTD_PID_OUT : QTD_PID_IN) : QTD_PID_IN);

    ptdc->err_count = 3;
    ptdc->cur_page = 0;
    ptdc->ioc = 1;
    ptdc->bytes_to_transfer = 0;
    ptdc->data_toggle = 1;
    ptd->hw_next = EHCI_PTR_TERM;

    ListFirst(&td_list, pthis);
    RemoveEntryList(&td_list);

    ptd = qtd_from_list_entry(pthis);
    elem_pool_lock(qh_pool, TRUE);
    pelnk = elem_pool_alloc_elem(qh_pool);
    elem_pool_unlock(qh_pool, TRUE);

    if (pelnk == NULL)
    {
        elem_safe_free(pthis, FALSE);
        return STATUS_NO_MORE_ENTRIES;

    }
    pqh = (PEHCI_QH) ((ULONG) pelnk->phys_part & PHYS_PART_ADDR_MASK);
    pqhc = (PEHCI_QH_CONTENT) pqh;

    pqh->hw_alt_next = pqh->hw_next = EHCI_PTR_TERM;

    pqhc->dev_addr = dev_addr;
    pqhc->inactive = 0;
    pqhc->endp_addr = endp_num(purb->pendp);

    if (pipe_content->speed_high)
        pqhc->endp_spd = USB_SPEED_HIGH;
    else if (pipe_content->speed_low)
        pqhc->endp_spd = USB_SPEED_LOW;
    else
        pqhc->endp_spd = USB_SPEED_FULL;

    pqhc->data_toggle = 1;      // use dt from qtd
    pqhc->is_async_head = 0;
    pqhc->max_packet_size = endp_max_packet_size(purb->pendp);

    if (pipe_content->speed_high == 0)
        pqhc->is_ctrl_endp = 1;
    else
        pqhc->is_ctrl_endp = 0;

    pqhc->reload_counter = EHCI_NAK_RL_COUNT;

    // DWORD 2
    pqh->hw_info2 = 0;
    pqhc->mult = 1;

    if (!pipe_content->speed_high)
    {
        pqhc->hub_addr = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->hub_addr;
        pqhc->port_idx = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->port_idx;
    }

    purb->td_count = td_count;

    ehci_insert_tds_qh(ehci, pqh, ptd);
    ehci_insert_qh_urb(purb, pqh);

    usb_endp_busy_count_inc(purb->pendp);
    ehci_insert_urb_to_schedule(ehci, purb, ret);

    if (ret == FALSE)
    {
        RemoveEntryList(&purb->trasac_list);
        RemoveEntryList(&pqh->elem_head_link->elem_link);

        elem_safe_free(&pqh->elem_head_link->elem_link, TRUE);
        elem_safe_free(pthis, FALSE);

        InitializeListHead(&purb->trasac_list);
        // usb_endp_busy_count_dec( purb->pendp );
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
ehci_internal_submit_int(PEHCI_DEV ehci, PURB purb)
{
    LONG i, max_packet_size;
    PEHCI_QTD ptd;
    BOOLEAN ret;
    PUSB_DEV pdev;
    PURB_HS_PIPE_CONTENT pipe_content;
    UCHAR mult_trans, toggle, old_toggle;
    PEHCI_ELEM_LINKS pelnk;
    PEHCI_QTD_CONTENT ptdc;
    PEHCI_QH pqh;
    PEHCI_QH_CONTENT pqhc;
    PEHCI_FSTN pfstn;

    if (ehci == NULL || purb == NULL)
        return STATUS_INVALID_PARAMETER;

    old_toggle = toggle = (purb->pendp->flags & USB_ENDP_FLAG_DATATOGGLE) ? TRUE : FALSE;
    max_packet_size = endp_max_packet_size(purb->pendp);
    pdev = dev_from_endp(purb->pendp);

    if (max_packet_size == 0 || max_packet_size > 64)
        return STATUS_INVALID_PARAMETER;

    if ((pdev->flags & USB_DEV_FLAG_HIGH_SPEED) == 0)
    {
        if (max_packet_size < purb->data_length)
            return STATUS_INVALID_PARAMETER;

        for(i = 1; i < 16; i++)
        {
            if ((((ULONG) purb->pendp->pusb_endp_desc->bInterval) >> i) == 0)
                break;
        }
        i--;
        mult_trans = 1;
    }
    else
    {
        mult_trans = endp_mult_count(purb->pendp);
        if (max_packet_size * endp_mult_count(purb->pendp) < purb->data_length)
            return STATUS_INVALID_PARAMETER;
        i = purb->pendp->pusb_endp_desc->bInterval - 1;
    }

    purb->pipe = 0;
    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    pipe_content->interval = i;
    pipe_content->trans_type = USB_ENDPOINT_XFER_INT;   // bit 0-1
    pipe_content->speed_high = (pdev->flags & USB_DEV_FLAG_HIGH_SPEED) ? 1 : 0; // bit 5
    pipe_content->speed_low = (pdev->flags & USB_DEV_FLAG_LOW_SPEED) ? 1 : 0;   // bit 6
    pipe_content->trans_dir = endp_dir(purb->pendp);    // bit 7
    pipe_content->dev_addr = pdev->dev_addr;    // bit 8-14
    pipe_content->endp_addr = endp_num(purb->pendp);    // bit 15-18
    pipe_content->data_toggle = 1;      // bit 19
    pipe_content->mult_count = mult_trans;

    // pipe_content->start_uframe : 3; // bit 28-30 will be filled later

    for(i = 1; i <= 16; i++)
    {
        if (((ULONG) max_packet_size) >> i)
            continue;
        else
            break;
    }
    i--;
    i &= 0xf;

    pipe_content->max_packet_size = i;  // bit 20-23 log2( max_packet_size )

    if (ehci_claim_bandwidth(ehci, purb, TRUE) == FALSE)
    {
        // can not allocate bandwidth for it
        return STATUS_UNSUCCESSFUL;
    }

    // one qtd is enough
    elem_pool_lock(qtd_pool, TRUE);
    pelnk = elem_pool_alloc_elem(qtd_pool);
    elem_pool_unlock(qtd_pool, TRUE);

    if (pelnk == NULL)
    {
        ehci_claim_bandwidth(ehci, purb, FALSE);
        return STATUS_NO_MORE_ENTRIES;
    }

    ptd = (PEHCI_QTD) ((ULONG) pelnk->phys_part & PHYS_PART_ADDR_MASK);
    ptdc = (PEHCI_QTD_CONTENT) ptd;
    ptd->hw_next = EHCI_PTR_TERM;
    // DWORD 1
    ptd->hw_alt_next = EHCI_PTR_TERM;
    // DWORD 2
    ptdc->status = 0x80;
    ptdc->pid = pipe_content->trans_dir ? QTD_PID_IN : QTD_PID_OUT;
    ptdc->err_count = 3;
    ptdc->cur_page = 0;
    ptdc->ioc = 1;
    ptdc->bytes_to_transfer = purb->data_length;
    toggle = (UCHAR) ehci_fill_td_buf_ptr(purb, 0, &pelnk->elem_link, 1, toggle);

    elem_pool_lock(qh_pool, TRUE);
    pelnk = elem_pool_alloc_elem(qh_pool);
    elem_pool_unlock(qh_pool, TRUE);
    if (pelnk == NULL)
    {
        elem_safe_free(&ptd->elem_head_link->elem_link, TRUE);
        InitializeListHead(&purb->trasac_list);
        ehci_claim_bandwidth(ehci, purb, FALSE);
        return STATUS_NO_MORE_ENTRIES;
    }
    pqh = (PEHCI_QH) ((ULONG) pelnk->phys_part & PHYS_PART_ADDR_MASK);
    pqhc = (PEHCI_QH_CONTENT) pqh;

    pqh->hw_next = EHCI_PTR_TERM;
    pqhc->dev_addr = pdev->dev_addr;
    pqhc->inactive = 0;
    pqhc->endp_addr = endp_num(purb->pendp);

    if (pipe_content->speed_high)
        pqhc->endp_spd = USB_SPEED_HIGH;
    else if (pipe_content->speed_low)
        pqhc->endp_spd = USB_SPEED_LOW;
    else
        pqhc->endp_spd = USB_SPEED_FULL;

    pqhc->data_toggle = 0;
    pqhc->is_async_head = 0;
    pqhc->max_packet_size = endp_max_packet_size(purb->pendp);
    pqhc->is_ctrl_endp = 0;
    pqhc->reload_counter = 0;

    // DWORD 2
    pqh->hw_info2 = 0;
    pqhc->mult = mult_trans;

    if (pipe_content->speed_high)
    {
        if (pipe_content->interval == 0)        // one poll per uframe
            pqhc->s_mask = 0xff;
        else if (pipe_content->interval == 1)   // one poll every 2 uframe
            pqhc->s_mask = pipe_content->start_uframe == 0 ? 0x55 : 0xbb;
        else if (pipe_content->interval == 2)
        {
            pqhc->s_mask = 0x11;
            pqhc->s_mask <<= pipe_content->start_uframe;
        }
        else
        {
            pqhc->s_mask = 1 << (pipe_content->start_uframe);
        }
        pqhc->c_mask = 0;
    }
    else                        // full/low speed
    {
        pqhc->s_mask = 1 << pipe_content->start_uframe;
        if (pipe_content->start_uframe < 4)
        {
            pqhc->c_mask = 0x07 << (pipe_content->start_uframe + 2);
        }
        else if (pipe_content->start_uframe == 4)
        {
            pqhc->c_mask = 0xc1;
        }
        else if (pipe_content->start_uframe >= 5)
        {
            // we need fstn
            pqhc->c_mask = 0x03;
            if (pipe_content->start_uframe == 5)
            {
                pqhc->c_mask |= 0x80;
            }
        }
        if (pipe_content->start_uframe >= 4)
        {
            // chain an fstn
            elem_pool_lock(fstn_pool, TRUE);
            pelnk = elem_pool_alloc_elem(fstn_pool);
            elem_pool_unlock(fstn_pool, TRUE);
            if (pelnk == NULL)
            {
                elem_safe_free(&pqh->elem_head_link->elem_link, TRUE);
                elem_safe_free(&ptd->elem_head_link->elem_link, TRUE);
                InitializeListHead(&purb->trasac_list);
                ehci_claim_bandwidth(ehci, purb, FALSE);
                return STATUS_NO_MORE_ENTRIES;
            }
            pfstn = (PEHCI_FSTN) ((ULONG) pelnk->phys_part & PHYS_PART_ADDR_MASK);
            pfstn->hw_prev = ptd->phys_addr;
            pfstn->elem_head_link->purb = purb;
            InsertTailList(&ptd->elem_head_link->elem_link, &pfstn->elem_head_link->elem_link);
        }
        pqhc->hub_addr = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->hub_addr;
        pqhc->port_idx = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->port_idx;
    }

    // DWORD 3
    purb->td_count = 1;

    InitializeListHead(&purb->trasac_list);
    ehci_insert_tds_qh(ehci, pqh, ptd);
    ehci_insert_qh_urb(purb, pqh);

    purb->pendp->flags = (purb->pendp->flags & ~USB_ENDP_FLAG_DATATOGGLE) | (toggle << 31);
    usb_endp_busy_count_inc(purb->pendp);

    ehci_insert_urb_to_schedule(ehci, purb, ret);

    if (ret == FALSE)
    {
        RemoveEntryList(&purb->trasac_list);
        RemoveEntryList(&pqh->elem_head_link->elem_link);

        elem_safe_free(&pqh->elem_head_link->elem_link, TRUE);
        // an fstn may follow the td
        elem_safe_free(&ptd->elem_head_link->elem_link, FALSE);

        InitializeListHead(&purb->trasac_list);
        ehci_claim_bandwidth(ehci, purb, FALSE);

        purb->pendp->flags = (purb->pendp->flags & ~USB_ENDP_FLAG_DATATOGGLE) | ((toggle ^ 1) << 31);
        // usb_endp_busy_count_dec( purb->pendp );

        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


static NTSTATUS
ehci_internal_submit_iso(PEHCI_DEV ehci, PURB purb)
{
    LONG i, j, td_count, temp;
    PEHCI_ITD pitd;
    PEHCI_SITD psitd;
    PEHCI_SITD_CONTENT psitdc;
    PEHCI_ITD_CONTENT pitdc;
    LIST_ENTRY td_list, *pthis, *pnext, *pprev;
    BOOLEAN ret;
    PURB_HS_PIPE_CONTENT pipe_content;
    PUSB_DEV pdev;
    PEHCI_ELEM_LINKS pelnk;

    if (ehci == NULL || purb == NULL)
        return STATUS_INVALID_PARAMETER;

    if (purb->iso_frame_count == 0)
        return STATUS_INVALID_PARAMETER;

    pdev = dev_from_endp(purb->pendp);
    purb->pipe = 0;
    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    pipe_content->trans_type = USB_ENDPOINT_XFER_ISOC;  // bit 0-1
    pipe_content->speed_high = (pdev->flags & USB_DEV_FLAG_HIGH_SPEED) ? 1 : 0; // bit 5
    pipe_content->speed_low = 0;        // bit 6
    pipe_content->trans_dir = endp_dir(purb->pendp);    // bit 7
    pipe_content->dev_addr = pdev->dev_addr;    // bit 8-14
    pipe_content->endp_addr = endp_num(purb->pendp);    // bit 15-18
    pipe_content->data_toggle = 0;      // bit 19

    ret = FALSE;
    purb->params[0] = j = endp_max_packet_size(purb->pendp);

    if (pipe_content->speed_high == 0)
    {
        // check to see if the frame data is too long to transfer
        if (purb->iso_frame_count >= (LONG) ehci->frame_count)
            return STATUS_INVALID_PARAMETER;

        for(i = 0; i < (LONG) purb->iso_frame_count; i++)
        {
            if (purb->iso_packet_desc[i].length > j)
                return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        // excess the frame count limit
        if (purb->iso_frame_count >= (LONG) (ehci->frame_count << 3))
            return STATUS_INVALID_PARAMETER;

        for(i = 0; i < (LONG) purb->iso_frame_count; i++)
        {
            if (purb->iso_packet_desc[i].length > j * endp_mult_count(purb->pendp))     // 3 is max mult-transaction count
                return STATUS_INVALID_PARAMETER;
        }

        pipe_content->mult_count = endp_mult_count(purb->pendp);
    }

    pipe_content->max_packet_size = 0;  // bit 20-23 log( max_packet_size ), not correct, should not be used

    if (pipe_content->speed_high == 0)
    {
        for(i = 1; i < 16; i++)
        {
            if ((((ULONG) purb->pendp->pusb_endp_desc->bInterval) >> i) == 0)
                break;
        }
        i--;
    }
    else
    {
        i = purb->pendp->pusb_endp_desc->bInterval - 1;
    }

    pipe_content->interval = i; // bit 24-27 the same definition as in USB2.0 spec, for high or full/low speed

    if (ehci_claim_bandwidth(ehci, purb, TRUE) == FALSE)
        return STATUS_UNSUCCESSFUL;

    if (pipe_content->speed_high == 0)
    {
        td_count = purb->iso_frame_count;

        // test to see if the last td needs one more sitd for pure complete-split
        if (pipe_content->trans_dir == 0)
        {
            j = (purb->iso_packet_desc[purb->iso_frame_count - 1].length + 187) / 188;
            if (purb->iso_packet_desc[purb->iso_frame_count - 1].params.start_uframe + 1 + j >= 8)
            {
                td_count++;
                ret = TRUE;
            }
        }
        elem_pool_lock(itd_pool, TRUE);
        pelnk = elem_pool_alloc_elems(itd_pool, td_count);
        elem_pool_unlock(itd_pool, TRUE);

    }
    else
    {
        i = REAL_INTERVAL;
        if (pipe_content->interval >= 3)
        {
            td_count = purb->iso_frame_count;
            j = 0;
        }
        else
        {
            j = purb->iso_start_frame & 0x07;
            if (j == 0)
            {
                td_count = (purb->iso_frame_count + 8 / i - 1) * i / 8;
            }
            else
            {
                j = 1 + (7 - j) / i;    // the leading packets from the 8-trans boundary
                td_count = (j >= (LONG) purb->iso_frame_count ?
                            1 : 1 + (purb->iso_frame_count - j + 8 / i - 1) * i / 8);
            }
        }

        elem_pool_lock(sitd_pool, TRUE);
        pelnk = elem_pool_alloc_elems(sitd_pool, td_count);
        elem_pool_unlock(sitd_pool, TRUE);
    }

    if (pelnk == NULL)
    {
        ehci_claim_bandwidth(ehci, purb, FALSE);
        return STATUS_NO_MORE_ENTRIES;
    }

    InsertTailList(&pelnk->elem_link, &td_list);
    ListFirst(&td_list, pthis);
    pprev = pthis;
    purb->td_count = td_count;

    //set up offset for high speed and interval == 1
    if (pipe_content->speed_high && pipe_content->interval == 0)
    {
        for(i = 0; i < (LONG) purb->iso_frame_count; i++)
        {
            if (i == 0)
                purb->iso_packet_desc[i].offset = 0;
            else
                purb->iso_packet_desc[i].offset = purb->iso_packet_desc[i - 1].offset +
                    purb->iso_packet_desc[i].length;
        }
    }

    i = 0, temp = 0;

    while (pthis)
    {
        init_elem_phys_part(struct_ptr(pthis, EHCI_ELEM_LINKS, elem_link));
        if (pipe_content->speed_high)
        {
            LONG start_uframe, k;
            LONG l, pk_idx, offset, start_uf, td_length;
            PULONG pbuf;
            ULONG phys_addr[8];

            pitd = itd_from_list_entry(pthis);
            pitdc = (PEHCI_ITD_CONTENT) pitd;
            start_uframe = purb->iso_start_frame & 0x07;

            // will be filled later
            pitd->hw_next = EHCI_PTR_TERM;

            // DWORD 9;
            pitdc->dev_addr = pdev->dev_addr;
            pitdc->endp_num = endp_num(purb->pendp);

            pitdc->max_packet_size = endp_max_packet_size(purb->pendp);
            pitdc->io_dir = pipe_content->trans_dir;
            pitdc->mult = endp_mult_count(purb->pendp);

            pbuf = pitd->hw_bufp;
            RtlZeroMemory(phys_addr, sizeof(phys_addr));

            if (pipe_content->interval < 3)
            {
                // this indicates one itd schedules more than one uframes
                // for multiple transactions described by iso_packet_desc
                if (i == 0)
                    k = td_count == 1 ? purb->iso_frame_count : j;      // the first itd
                else
                    k = (LONG) (purb->iso_frame_count - i) <= 8 / REAL_INTERVAL
                        ? (purb->iso_frame_count - i) : 8 / REAL_INTERVAL;

                // j is the header transactions out of the interval
                // aligned transactions per td
                if (j > 0 && i == 0)    // handle the first itd
                    start_uf = start_uframe;
                else
                    start_uf = start_uframe % REAL_INTERVAL;
            }
            else
            {
                k = 1, start_uf = start_uframe & 0x07;
            }


            // calculate the data to transfer with this td
            td_length = 0;
            for(l = start_uf, pk_idx = i; pk_idx < i + k; pk_idx++, l += REAL_INTERVAL)
            {
                td_length += purb->iso_packet_desc[pk_idx].length;
                phys_addr[l] =
                    MmGetPhysicalAddress(&purb->data_buffer[purb->iso_packet_desc[pk_idx].offset]).LowPart;
            }

            // fill the page pointer, and offset
            if (pipe_content->interval != 0)
            {
                for(l = start_uf, pk_idx = i; pk_idx < i + k; pk_idx++, l += REAL_INTERVAL)
                {
                    pitdc->status_slot[l].offset = phys_addr[l] & (PAGE_SIZE - 1);
                    pbuf[l >> pipe_content->interval] |= phys_addr[l] & (~(PAGE_SIZE - 1));
                    pitdc->status_slot[l].page_sel = l >> pipe_content->interval;
                    pitdc->status_slot[l].status = 0x08;
                    pitdc->status_slot[l].trans_length = purb->iso_packet_desc[pk_idx].length;
                    if (PAGE_SIZE - pitdc->status_slot[l].offset <
                        (ULONG) purb->iso_packet_desc[pk_idx].length)
                    {
                        // fill the next page buf, we can not simply add
                        // PAGE_SIZE to the phys_addr[ l ].
                        pbuf[(l >> pipe_content->interval) + 1] |=
                            MmGetPhysicalAddress((PBYTE)
                                                 (((ULONG) & purb->
                                                   data_buffer[purb->iso_packet_desc[pk_idx].
                                                               offset]) & (~(PAGE_SIZE - 1))) +
                                                 PAGE_SIZE).LowPart;
                    }
                }
            }
            else                // interval == 0
            {
                LONG m, n = 0, n2 = 0;
                // fill the page buffer first
                // calculate the page buffer needed
                offset = phys_addr[0] & (PAGE_SIZE - 1);
                if (offset != 0)
                {
                    offset = PAGE_SIZE - offset;
                    l = 1 + (td_length - offset + PAGE_SIZE - 1) / PAGE_SIZE;
                }
                else
                {
                    l = (td_length + PAGE_SIZE - 1) / PAGE_SIZE;
                }

                if (l > 7)
                    TRAP();

                // fill the hw_bufp array and PG field, pk_idx is index into hw_bufp
                for(pk_idx = 0; pk_idx < l; pk_idx++)
                {
                    if (pk_idx == 0)
                    {
                        offset = phys_addr[start_uf] & (~(PAGE_SIZE - 1));
                        pbuf[pk_idx] |= offset;
                        n = pk_idx;
                        pitdc->status_slot[0].page_sel = n;
                        n2 = start_uf;
                    }
                    else
                    {
                        // scan to find if the buf pointer already filled in the td
                        // since interval = 1, we do not need k * REAL_INTERVAL
                        // k is transaction count for current td,
                        // n is hw_bufp( pbuf ) index
                        // n2 is the last phys_addr index we stopped
                        for(m = n2; m < start_uf + k; m++)
                        {
                            // we can not determine the phys_addr[ x ] is piror
                            // to offset if it is less than offset.
                            // because phys_addr is discrete.
                            // if( ( phys_addr[ m ] & ( ~( PAGE_SIZE - 1 ) ) ) < offset )
                            // continue;

                            if ((phys_addr[m] & (~(PAGE_SIZE - 1))) == (ULONG) offset)
                            {
                                pitdc->status_slot[m].page_sel = n;
                                continue;
                            }
                            break;
                        }

                        if (m == start_uf + k)
                            TRAP();

                        offset = phys_addr[m] & (~(PAGE_SIZE - 1));
                        pbuf[pk_idx] |= offset;
                        n = pk_idx;
                        n2 = m;
                        pitdc->status_slot[m].page_sel = n;
                    }
                }
                // fill offset and others
                for(l = start_uf, pk_idx = i; l < start_uf + k; l++, pk_idx++)
                {
                    pitdc->status_slot[l].offset = (phys_addr[l] & (PAGE_SIZE - 1));
                    pitdc->status_slot[l].status = 0x08;
                    pitdc->status_slot[l].trans_length = purb->iso_packet_desc[pk_idx].length;
                }
                // exhausted
            }
            i += k;
        }
        else                    // full/low speed
        {
            psitd = sitd_from_list_entry(pthis);
            psitdc = (PEHCI_SITD_CONTENT) psitd;
            psitd->hw_next = EHCI_PTR_TERM;

            // DWORD 1;
            psitdc->dev_addr = pdev->dev_addr;
            psitdc->endp_num = endp_num(purb->pendp);
            psitdc->hub_addr = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->hub_addr;
            psitdc->port_idx = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->port_idx;
            psitdc->io_dir = endp_dir(purb->pendp);

            psitdc->status &= 0x80;     // in DWORD 3

            // DWORD 2;
            j = (purb->iso_packet_desc[i].length + 187) / 188;

            if (psitdc->io_dir == 0)
            {
                for(; j > 0; j--)
                {
                    psitdc->s_mask |= (1 << (j - 1));
                }
                psitdc->s_mask <<= purb->iso_packet_desc[i].params.start_uframe & 0x07;
                psitdc->c_mask = 0;
            }
            else
            {
                LONG k;

                psitdc->s_mask = 1 << purb->iso_packet_desc[i].params.start_uframe & 0x07;
                // iso split case 2b: ehci spec 1.0
                if (j == 6)
                    j = 5;

                j = j - 1 + 2;  // actual complete-split count

                psitdc->c_mask |= temp >> 8;    // the previous sitd's complete split
                if (temp >> 8)  // link back for sitd split completion
                {
                    psitd->hw_backpointer = sitd_from_list_entry(pprev)->phys_addr;
                    psitdc->status &= 0x82;
                }
                else
                {
                    psitd->hw_backpointer = EHCI_PTR_TERM;
                }

                for(k = temp = 0; k < j; k++)
                {
                    temp |= 1 << k;
                }

                temp <<= ((purb->iso_packet_desc[i].params.start_uframe & 0x07) + 2);

                // only uframe zero and one have complete split for prev sitd
                if ((temp >> 8) > 3)
                    TRAP();

                psitdc->c_mask |= temp & 0xff;
            }

            // DWORD 3:
            psitdc->c_prog_mask = 0;
            psitdc->bytes_to_transfer = purb->iso_packet_desc[i].length;
            psitdc->page_sel = 0;
            psitdc->ioc = 0;

            // DWORD 4;
            j = (ULONG) ((PBYTE) purb->data_buffer + purb->iso_packet_desc[i].offset);
            psitd->hw_tx_results2 = MmGetPhysicalAddress((PVOID) j).LowPart;

            // DWORD 5;
            if (PAGE_SIZE - (j & (PAGE_SIZE - 1)) < (ULONG) purb->iso_packet_desc[i].length)
            {
                // need to fill another slot
                psitdc->page1 =
                    MmGetPhysicalAddress((PVOID) ((j & ~(PAGE_SIZE - 1)) + PAGE_SIZE)).LowPart >> 12;
            }

            if (purb->iso_packet_desc[i].length > 188)
                psitdc->trans_pos = 0x00;
            else if (purb->iso_packet_desc[i].length <= 188)
                psitdc->trans_pos = 0x01;

            if (psitdc->io_dir == 0)
                psitdc->trans_count = (purb->iso_packet_desc[i].length + 187) / 188;

        }
        ListNext(&td_list, pthis, pnext);
        pprev = pthis;
        pthis = pnext;

    }

    if (pipe_content->speed_high == 0)
    {
        // has an extra sitd to fill at the tail
        if (ret)
        {
            ListFirstPrev(&td_list, pthis);
            init_elem_phys_part(struct_ptr(pthis, EHCI_ELEM_LINKS, elem_link));

            psitd = sitd_from_list_entry(pthis);
            psitdc = (PEHCI_SITD_CONTENT) psitd;
            psitd->hw_next = EHCI_PTR_TERM;

            // DWORD 1;
            psitdc->dev_addr = pdev->dev_addr;
            psitdc->endp_num = endp_num(purb->pendp);
            psitdc->hub_addr = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->hub_addr;
            psitdc->port_idx = ((PURB_HS_CONTEXT_CONTENT) & purb->hs_context)->port_idx;
            psitdc->io_dir = endp_dir(purb->pendp);

            psitdc->status &= 0x80;     // in DWORD 3

            // DWORD 2;
            psitdc->s_mask = 0x04;      // uframe 2, random selection

            psitdc->c_mask = 0x70;      // complete split at uframe 4, 5, 6
            ListFirstPrev(pthis, pprev);
            psitd->hw_backpointer = sitd_from_list_entry(pprev)->phys_addr;
            psitdc->status &= 0x82;

            // DWORD 3:
            psitdc->c_prog_mask = 0;
            psitdc->bytes_to_transfer = 1;      // purb->iso_packet_desc[ purb->iso_frame_count - 1 ].length;
            psitdc->page_sel = 0;

            j = (ULONG) ((PBYTE) purb->data_buffer + purb->iso_packet_desc[purb->iso_frame_count - 1].offset);
            // the last byte is overridden.
            j += purb->iso_packet_desc[purb->iso_frame_count - 1].length - 1;
            psitd->hw_tx_results2 = MmGetPhysicalAddress((PVOID) j).LowPart;
        }

        // set the interrupt
        ListFirstPrev(&td_list, pthis);
        psitdc = (PEHCI_SITD_CONTENT) sitd_from_list_entry(pthis);
        psitdc->ioc = 1;
    }
    else
    {
        // set the ioc
        ListFirstPrev(&td_list, pthis);
        pitdc = (PEHCI_ITD_CONTENT) itd_from_list_entry(pthis);
        for(i = 7; i >= 0; i--)
        {
            if (pitdc->status_slot[i].status == 0x08)
            {
                pitdc->status_slot[i].ioc = 1;
                break;
            }
        }
        if (i < 0)
            TRAP();
    }

    ListFirst(&td_list, pthis);
    // ListFirst( &purb->trasac_list, pthis )
    RemoveEntryList(&td_list);
    InsertTailList(pthis, &purb->trasac_list);

    while (pthis)
    {
        // fill the purb ptr
        struct_ptr(pthis, EHCI_ELEM_LINKS, elem_link)->purb = purb;
        ListNext(&purb->trasac_list, pthis, pnext);
        pthis = pnext;
    }

    //indirectly guarded by pending_endp_list_lock
    usb_endp_busy_count_inc(purb->pendp);
    ehci_insert_urb_to_schedule(ehci, purb, ret);

    if (ret == FALSE)
    {
        // usb_endp_busy_count_dec( purb->pendp );

        ListFirst(&purb->trasac_list, pthis);
        RemoveEntryList(&purb->trasac_list);

        elem_safe_free(pthis, FALSE);
        ehci_claim_bandwidth(ehci, purb, FALSE);
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

BOOLEAN NTAPI
//this function used as the KeSynchronizeExecution param to delegate control to ehci_insert_urb_schedule
ehci_sync_insert_urb_schedule(PVOID context)
{
    PSYNC_PARAM sync_param;
    PEHCI_DEV ehci;
    PURB purb;

    sync_param = (PSYNC_PARAM) context;
    if (sync_param == NULL)
        return FALSE;

    ehci = sync_param->ehci;
    purb = (PURB) sync_param->context;

    if (ehci == NULL || purb == NULL)
        return (UCHAR) (sync_param->ret = FALSE);

    return (UCHAR) (sync_param->ret = ehci_insert_urb_schedule(ehci, purb));
}

static BOOLEAN NTAPI
ehci_sync_cancel_urb(PVOID context)
{
    //cancel a single purb
    PEHCI_DEV ehci;
    PSYNC_PARAM sync_param;
    PURB purb2, dest_urb;
    PLIST_ENTRY pthis, pnext;
    BOOLEAN found = FALSE;

    if (context == NULL)
        return FALSE;

    sync_param = (PSYNC_PARAM) context;
    ehci = sync_param->ehci;
    dest_urb = (PURB) sync_param->context;

    if (ehci == NULL || dest_urb == NULL)
        return (UCHAR) (sync_param->ret = FALSE);

    ListFirst(&ehci->urb_list, pthis);
    while (pthis)
    {
        purb2 = (PURB) pthis;
        if (purb2 == dest_urb)
        {
            found = TRUE;
            purb2->flags |= URB_FLAG_FORCE_CANCEL;
            break;
        }
        ListNext(&ehci->urb_list, pthis, pnext);
        pthis = pnext;
    }

    if (found)
    {
        press_doorbell(ehci);
    }
    return (UCHAR) (sync_param->ret = found);
}

NTSTATUS
ehci_cancel_urb(PEHCI_DEV ehci, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
//note any fields of the purb can not be referenced unless it is found in some queue
{
    PLIST_ENTRY pthis, pnext;
    BOOLEAN found;
    PURB purb2;

    SYNC_PARAM sync_param;

    USE_BASIC_NON_PENDING_IRQL;

    if (ehci == NULL || purb == NULL || pdev == NULL || pendp == NULL)
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
        //it will be canceled in ehci_process_pending_endp
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

        ehci_generic_urb_completion(purb, purb->context);

        lock_dev(pdev, FALSE);
        pdev->ref_count--;
        unlock_dev(pdev, FALSE);
        return STATUS_SUCCESS;
    }

    //      search the purb in the purb-list and try to cancel
    sync_param.ehci = ehci;
    sync_param.context = purb;

    KeSynchronizeExecution(ehci->pdev_ext->ehci_int, ehci_sync_cancel_urb, &sync_param);

    found = sync_param.ret;

    if (found)
        return USB_STATUS_CANCELING;

    return STATUS_INVALID_PARAMETER;
}

VOID
ehci_generic_urb_completion(PURB purb, PVOID context)
{
    PUSB_DEV pdev;
    BOOLEAN is_ctrl = FALSE;
    USE_NON_PENDING_IRQL;

    old_irql = KeGetCurrentIrql();
    if (old_irql > DISPATCH_LEVEL)
        TRAP();

    if (old_irql < DISPATCH_LEVEL)
        KeRaiseIrql(DISPATCH_LEVEL, &old_irql);

    pdev = purb->pdev;
    if (purb == NULL)
        goto LBL_LOWER_IRQL;

    if (pdev == NULL)
        goto LBL_LOWER_IRQL;

    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        // no need to do following statistics
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
                ehci_dbg_print(DBGLVL_MAXIMUM,
                               ("ehci_generic_urb_completion(): contiguous error 3 times, dev 0x%x is deactivated\n",
                                pdev));
            }
        }
        else
            pdev->time_out_count = 0;

    }

    if (endp_type(purb->pendp) == USB_ENDPOINT_XFER_CONTROL)
        is_ctrl = TRUE;

    unlock_dev(pdev, TRUE);

  LBL_CLIENT_PROCESS:
    if (!is_ctrl)
    {
        if (purb->completion)
            purb->completion(purb, context);
    }
    else
    {
        if (purb->ctrl_req_context.ctrl_stack_count == 0)
        {
            if (purb->completion)
                purb->completion(purb, context);
        }
        else
        {
            // pstack = &purb->ctrl_req_stack[ purb->ctrl_req_context.ctrl_cur_stack ];
            // if( pstack->urb_completion )
            //              pstack->urb_completion( purb, pstack->context );
            usb_call_ctrl_completion(purb);
        }
    }

  LBL_LOWER_IRQL:
    if (old_irql < DISPATCH_LEVEL)
        KeLowerIrql(old_irql);

    return;
}

NTSTATUS
ehci_rh_submit_urb(PUSB_DEV pdev, PURB purb)
{
    PUSB_DEV_MANAGER dev_mgr;
    PTIMER_SVC ptimer;
    PUSB_CTRL_SETUP_PACKET psetup;
    PEHCI_DEV ehci;
    NTSTATUS status;
    PHUB2_EXTENSION hub_ext;
    PUSB_PORT_STATUS ps, psret;
    LONG i;
    UCHAR port_count;

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

    ehci = ehci_from_hcd(pdev->hcd);
    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

    hub_ext = ((PHUB2_EXTENSION) pdev->dev_ext);
    port_count = (UCHAR) ((PEHCI_HCS_CONTENT) & ehci->ehci_caps.hcs_params)->port_count;

    switch (endp_type(purb->pendp))
    {
        case USB_ENDPOINT_XFER_CONTROL:
        {
            if (psetup->bmRequestType == 0xa3 && psetup->bRequest == USB_REQ_GET_STATUS)
            {
                //get-port-status
                if (psetup->wIndex == 0 || psetup->wIndex > port_count || psetup->wLength < 4)
                {
                    purb->status = STATUS_INVALID_PARAMETER;
                    break;
                }

                i = EHCI_PORTSC + 4 * (psetup->wIndex - 1);     // USBPORTSC1;
                status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + i));
                ps = &hub_ext->rh_port_status[psetup->wIndex];

                psret = (PUSB_PORT_STATUS) purb->data_buffer;
                ps->wPortStatus = 0;

                if (status & PORT_CCS)
                {
                    ps->wPortStatus |= USB_PORT_STAT_CONNECTION;
                }
                if (status & PORT_PE)
                {
                    ps->wPortStatus |= USB_PORT_STAT_ENABLE;
                    ps->wPortStatus |= USB_PORT_STAT_HIGH_SPEED;        // ehci spec
                }
                if (status & PORT_PR)
                {
                    ps->wPortStatus |= USB_PORT_STAT_RESET;
                }
                if (status & PORT_SUSP)
                {
                    ps->wPortStatus |= USB_PORT_STAT_SUSPEND;
                }
                if (PORT_USB11(status))
                {
                    ps->wPortStatus |= USB_PORT_STAT_LOW_SPEED;
                }

                //always power on
                ps->wPortStatus |= USB_PORT_STAT_POWER;

                //now set change field
                if ((status & PORT_CSC) && !(ps->wPortStatus & USB_PORT_STAT_LOW_SPEED))
                {
                    ps->wPortChange |= USB_PORT_STAT_C_CONNECTION;
                }
                if ((status & PORT_PEC) && !(ps->wPortStatus & USB_PORT_STAT_LOW_SPEED))
                {
                    ps->wPortChange |= USB_PORT_STAT_C_ENABLE;
                }

                //don't touch other fields, might be filled by
                //other function

                usb_dbg_print(DBGLVL_MAXIMUM,
                              ("ehci_rh_submit_urb(): get port status, wPortStatus=0x%x, wPortChange=0x%x, address=0x%x\n",
                               ps->wPortStatus, ps->wPortChange, ps));

                psret->wPortChange = ps->wPortChange;
                psret->wPortStatus = ps->wPortStatus;

                purb->status = STATUS_SUCCESS;

                break;
            }
            else if (psetup->bmRequestType == 0x23 && psetup->bRequest == USB_REQ_CLEAR_FEATURE)
            {
                //clear-port-feature
                if (psetup->wIndex == 0 || psetup->wIndex > port_count)
                {
                    purb->status = STATUS_INVALID_PARAMETER;
                    break;
                }

                i = EHCI_PORTSC + 4 * (psetup->wIndex - 1);     // USBPORTSC1;
                ps = &hub_ext->rh_port_status[psetup->wIndex];

                purb->status = STATUS_SUCCESS;
                switch (psetup->wValue)
                {
                    case USB_PORT_FEAT_C_CONNECTION:
                    {
                        SET_RH2_PORTSTAT(i, USBPORTSC_CSC);
                        status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + i));
                        usb_dbg_print(DBGLVL_MAXIMUM,
                                      ("ehci_rh_submit_urb(): clear csc, port%d=0x%x\n", psetup->wIndex));
                        ps->wPortChange &= ~USB_PORT_STAT_C_CONNECTION;
                        break;
                    }
                    case USB_PORT_FEAT_C_ENABLE:
                    {
                        SET_RH2_PORTSTAT(i, USBPORTSC_PEC);
                        status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + i));
                        usb_dbg_print(DBGLVL_MAXIMUM,
                                      ("ehci_rh_submit_urb(): clear pec, port%d=0x%x\n", psetup->wIndex));
                        ps->wPortChange &= ~USB_PORT_STAT_C_ENABLE;
                        break;
                    }
                    case USB_PORT_FEAT_C_RESET:
                    {
                        ps->wPortChange &= ~USB_PORT_STAT_C_RESET;
                        //the reset signal is down in rh_timer_svc_reset_port_completion
                        // enable or not is set by host controller
                        // status = EHCI_READ_PORT_ULONG( ( PUSHORT ) ( ehci->port_base + i ) );
                        usb_dbg_print(DBGLVL_MAXIMUM,
                                      ("ehci_rh_submit_urb(): clear pr, enable pe, port%d=0x%x\n",
                                       psetup->wIndex));
                        break;
                    }
                    case USB_PORT_FEAT_ENABLE:
                    {
                        ps->wPortStatus &= ~USB_PORT_STAT_ENABLE;
                        CLR_RH2_PORTSTAT(i, USBPORTSC_PE);
                        status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + i));
                        usb_dbg_print(DBGLVL_MAXIMUM,
                                      ("ehci_rh_submit_urb(): clear pe, port%d=0x%x\n", psetup->wIndex));
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
                if (psetup->wIndex == 0 || psetup->wIndex > port_count || psetup->wLength == 0)
                {
                    purb->status = STATUS_INVALID_PARAMETER;
                    break;
                }

                i = EHCI_PORTSC + 4 * (psetup->wIndex - 1);     // USBPORTSC1;
                status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + i));
                purb->data_buffer[0] = (status & USBPORTSC_LS);

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
                    ehci_dbg_print(DBGLVL_MAXIMUM,
                                   ("ehci_rh_submit_urb(): set feature with wValue=0x%x\n", psetup->wValue));
                    break;
                }

                i = EHCI_PORTSC + 4 * (psetup->wIndex - 1);     // USBPORTSC1;

                ptimer = alloc_timer_svc(&dev_mgr->timer_svc_pool, 1);
                ptimer->threshold = 0;  // within [ 50ms, 60ms ], one tick is 10 ms
                ptimer->context = (ULONG) purb;
                ptimer->pdev = pdev;
                ptimer->func = rh_timer_svc_reset_port_completion;

                //start the timer
                pdev->ref_count += 2;   //one for timer and one for purb

                status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + i));
                usb_dbg_print(DBGLVL_MAXIMUM,
                              ("ehci_rh_submit_urb(): reset port, port%d=0x%x\n", psetup->wIndex, status));
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

            usb_dbg_print(DBGLVL_MAXIMUM,
                          ("ehci_rh_submit_urb(): current rh's ref_count=0x%x\n", pdev->ref_count));
            pdev->ref_count += 2;       //one for timer and one for purb

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
ehci_rh_reset_port(PHCD hcd, UCHAR port_idx)
{
    ULONG i;
    PEHCI_DEV ehci;
    ULONG status;
    UCHAR port_count;

    if (hcd == NULL)
        return FALSE;

    ehci = ehci_from_hcd(hcd);
    port_count = (UCHAR) ((PEHCI_HCS_CONTENT) & ehci->ehci_caps.hcs_params)->port_count;

    if (port_idx < 1 || port_idx > port_count)
        return FALSE;

    i = (ULONG) (EHCI_PORTSC + 4 * (port_idx - 1));

    // assert the reset signal,(implicitly disable the port)
    SET_RH2_PORTSTAT(i, PORT_PR);

    usb_wait_ms_dpc(50);
    // clear the reset signal, delay port enable till clearing port feature
    CLR_RH2_PORTSTAT(i, PORT_PR);

    // wait the port stable
    usb_wait_ms_dpc(2);

    status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + i));
    if (!(status & PORT_PE))
    {
        // release the ownership from ehci to companion hc
        status |= PORT_OWNER;
        EHCI_WRITE_PORT_ULONG((PULONG) (ehci->port_base + i), status);
        // the host controller will set PORTSC automatically
        return FALSE;
    }
    usb_wait_us_dpc(10);
    // SET_RH_PORTSTAT( i, PORT_PE );

    //recovery time 10ms
    usb_wait_ms_dpc(10);

    // clear PORT_PEC and PORT_PCC
    SET_RH2_PORTSTAT(i, 0x0a);

    status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + i));
    usb_dbg_print(DBGLVL_MAXIMUM, ("ehci_rh_reset_port(): status after written=0x%x\n", status));
    return TRUE;
}

NTSTATUS
ehci_dispatch_irp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
    PEHCI_DEVICE_EXTENSION pdev_ext;
    PUSB_DEV_MANAGER dev_mgr;
    PEHCI_DEV ehci;

    pdev_ext = DeviceObject->DeviceExtension;
    ehci = pdev_ext->ehci;

    dev_mgr = ehci->hcd_interf.hcd_get_dev_mgr(&ehci->hcd_interf);
    return dev_mgr_dispatch(dev_mgr, irp);
}

//the following are for hcd interface methods
VOID
ehci_set_dev_mgr(PHCD hcd, PUSB_DEV_MANAGER dev_mgr)
{
    hcd->dev_mgr = dev_mgr;
}

PUSB_DEV_MANAGER
ehci_get_dev_mgr(PHCD hcd)
{
    return hcd->dev_mgr;
}

ULONG
ehci_get_type(PHCD hcd)
{
    return HCD_TYPE_EHCI;       // ( hcd->flags & HCD_TYPE_MASK );
}

VOID
ehci_set_id(PHCD hcd, UCHAR id)
{
    hcd->flags &= ~HCD_ID_MASK;
    hcd->flags |= (HCD_ID_MASK & id);
}

UCHAR
ehci_get_id(PHCD hcd)
{
    return (UCHAR) (hcd->flags & HCD_ID_MASK);
}


UCHAR
ehci_alloc_addr(PHCD hcd)
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
ehci_free_addr(PHCD hcd, UCHAR addr)
{
    if (addr & 0x80)
        return;

    if (hcd == NULL)
        return;

    hcd->dev_addr_map[addr >> 3] &= ~(1 << (addr & 7));
    return;

}

NTSTATUS
ehci_submit_urb2(PHCD hcd, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
{
    return ehci_submit_urb(ehci_from_hcd(hcd), pdev, pendp, purb);
}

PUSB_DEV
ehci_get_root_hub(PHCD hcd)
{
    return ehci_from_hcd(hcd)->root_hub;
}

VOID
ehci_set_root_hub(PHCD hcd, PUSB_DEV root_hub)
{
    if (hcd == NULL || root_hub == NULL)
        return;
    ehci_from_hcd(hcd)->root_hub = root_hub;
    return;
}

BOOLEAN
ehci_remove_device2(PHCD hcd, PUSB_DEV pdev)
{
    if (hcd == NULL || pdev == NULL)
        return FALSE;

    return ehci_remove_device(ehci_from_hcd(hcd), pdev);
}

BOOLEAN
ehci_hcd_release(PHCD hcd)
{
    PEHCI_DEV ehci;
    PEHCI_DEVICE_EXTENSION pdev_ext;

    if (hcd == NULL)
        return FALSE;

    ehci = ehci_from_hcd(hcd);
    pdev_ext = ehci->pdev_ext;
    return ehci_release(pdev_ext->pdev_obj);
}

NTSTATUS
ehci_cancel_urb2(PHCD hcd, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
{
    PEHCI_DEV ehci;
    if (hcd == NULL)
        return STATUS_INVALID_PARAMETER;

    ehci = ehci_from_hcd(hcd);
    return ehci_cancel_urb(ehci, pdev, pendp, purb);
}

BOOLEAN
ehci_rh_get_dev_change(PHCD hcd, PBYTE buf)     //must have the rh dev_lock acquired
{
    PEHCI_DEV ehci;
    LONG port_count, i;
    ULONG status;

    if (hcd == NULL)
        return FALSE;

    ehci = ehci_from_hcd(hcd);
    port_count = HCS_N_PORTS(ehci->ehci_caps.hcs_params);
    for(i = 0; i < port_count; i++)
    {
        status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + EHCI_PORTSC + (i << 2)));
        ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_rh_get_dev_change(): erh port%d status=0x%x\n", i, status));

        if (status & (PORT_PEC | PORT_CSC | PORT_OCC))
        {
            buf[(i + 1) >> 3] |= (1 << ((i + 1) & 7));
        }
    }
    return TRUE;
}

NTSTATUS
ehci_hcd_dispatch(PHCD hcd, LONG disp_code, PVOID param)
{
    PEHCI_DEV ehci;

    if (hcd == NULL)
        return STATUS_INVALID_PARAMETER;
    ehci = ehci_from_hcd(hcd);
    switch (disp_code)
    {
        case HCD_DISP_READ_PORT_COUNT:
        {
            if (param == NULL)
                return STATUS_INVALID_PARAMETER;
            *((PUCHAR) param) = (UCHAR) HCS_N_PORTS(ehci->ehci_caps.hcs_params);
            return STATUS_SUCCESS;
        }
        case HCD_DISP_READ_RH_DEV_CHANGE:
        {
            if (ehci_rh_get_dev_change(hcd, param) == FALSE)
                return STATUS_INVALID_PARAMETER;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_IMPLEMENTED;
}

//------------------------------------------------------------------------------
//              EHCI routines follows
//
VOID ehci_init_int8_qh(PEHCI_QH_CONTENT qh);

BOOLEAN NTAPI
ehci_cal_cpu_freq(PVOID context)
{
    usb_cal_cpu_freq();
    return TRUE;
}

PDEVICE_OBJECT
ehci_probe(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path, PUSB_DEV_MANAGER dev_mgr)
{
    LONG bus, i, j, ret = 0;
    PCI_SLOT_NUMBER slot_num;
    PPCI_COMMON_CONFIG pci_config;
    PDEVICE_OBJECT pdev;
    BYTE buffer[sizeof(PCI_COMMON_CONFIG)];
    PEHCI_DEVICE_EXTENSION pdev_ext;

    slot_num.u.AsULONG = 0;
    pci_config = (PPCI_COMMON_CONFIG) buffer;
    pdev = NULL;

    //scan the bus to find ehci controller
    for(bus = 0; bus < 2; bus++)        /*enum only bus0 and bus1 */
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

                if (pci_config->BaseClass == 0x0c && pci_config->SubClass == 0x03
                    && pci_config->ProgIf == 0x20)
                {
                    //well, we find our usb host controller( EHCI ), create device
                    pdev = ehci_alloc(drvr_obj, reg_path, ((bus << 8) | (i << 3) | j), dev_mgr);

                    if (!pdev)
                        continue;
                }
            }

            if (ret == 0)
                break;
        }
    }

    if (pdev)
    {
        pdev_ext = pdev->DeviceExtension;
        if (pdev_ext)
        {
            // acquire higher irql to eliminate pre-empty
            KeSynchronizeExecution(pdev_ext->ehci_int, ehci_cal_cpu_freq, NULL);
        }
    }
    return NULL;
}

PDEVICE_OBJECT
ehci_alloc(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path, ULONG bus_addr, PUSB_DEV_MANAGER dev_mgr)
{

    LONG frd_num, prd_num;
    PDEVICE_OBJECT pdev;
    PEHCI_DEVICE_EXTENSION pdev_ext;
    ULONG vector, addr_space;
    LONG bus, i;
    KIRQL irql;
    KAFFINITY affinity;

    DEVICE_DESCRIPTION dev_desc;
    CM_PARTIAL_RESOURCE_DESCRIPTOR *pprd;
    PCI_SLOT_NUMBER slot_num;
    NTSTATUS status;


    pdev = ehci_create_device(drvr_obj, dev_mgr);

    if (pdev == NULL)
        return NULL;

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
    dev_desc.MaximumLength = EHCI_MAX_SIZE_TRANSFER;

    pdev_ext->map_regs = 2;     // we do not use it seriously

    pdev_ext->padapter = HalGetAdapter(&dev_desc, &pdev_ext->map_regs);

    ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_alloc(): padapter=0x%x\n", pdev_ext->padapter));
    if (pdev_ext->padapter == NULL)
    {
        //fatal error
        ehci_delete_device(pdev);
        return NULL;
    }

    DbgPrint("ehci_alloc(): reg_path=0x%x, \n \
			ehci_alloc(): PCIBus=0x%x, bus=0x%x, bus_addr=0x%x \n \
			ehci_alloc(): slot_num=0x%x, &res_list=0x%x \n \
			ehci_alloc(): adapter=0x%x \n", (DWORD) reg_path, (DWORD) PCIBus, (DWORD) bus, (DWORD) bus_addr, (DWORD) slot_num.u.AsULONG, (DWORD) & pdev_ext->res_list, pdev_ext->padapter);

    //let's allocate resources for this device
    DbgPrint("ehci_alloc(): about to assign slot res\n");
    if ((status = HalAssignSlotResources(reg_path, NULL,        //no class name yet
                                         drvr_obj, NULL,        //no support of another ehci controller
                                         PCIBus,
                                         bus, slot_num.u.AsULONG, &pdev_ext->res_list)) != STATUS_SUCCESS)
    {
        DbgPrint("ehci_alloc(): error assign slot res, 0x%x\n", status);
        release_adapter(pdev_ext->padapter);
        pdev_ext->padapter = NULL;
        ehci_delete_device(pdev);
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
            else if (pprd->Type == CmResourceTypeMemory)
            {
                RtlCopyMemory(&pdev_ext->res_memory, &pprd->u.Memory, sizeof(pprd->u.Memory));
            }
        }
    }

    //for port, translate them to system address
    addr_space = 0;
    if (HalTranslateBusAddress(PCIBus, bus, pdev_ext->res_port.Start, &addr_space,      //io space
                               &pdev_ext->ehci->ehci_reg_base) != (BOOLEAN) TRUE)
    {
        DbgPrint("ehci_alloc(): error, can not translate bus address\n");
        release_adapter(pdev_ext->padapter);
        pdev_ext->padapter = NULL;
        ehci_delete_device(pdev);
        return NULL;
    }

    DbgPrint("ehci_alloc(): address space=0x%x\n, reg_base=0x%x\n",
             addr_space, pdev_ext->ehci->ehci_reg_base.u.LowPart);

    if (addr_space == 0)
    {
        //port has been mapped to memory space
        pdev_ext->ehci->port_mapped = TRUE;
        pdev_ext->ehci->port_base = (PBYTE) MmMapIoSpace(pdev_ext->ehci->ehci_reg_base,
                                                         pdev_ext->res_port.Length, FALSE);

        //fatal error can not map the registers
        if (pdev_ext->ehci->port_base == NULL)
        {
            release_adapter(pdev_ext->padapter);
            pdev_ext->padapter = NULL;
            ehci_delete_device(pdev);
            return NULL;
        }
    }
    else
    {
        //io space
        pdev_ext->ehci->port_mapped = FALSE;
        pdev_ext->ehci->port_base = (PBYTE) pdev_ext->ehci->ehci_reg_base.LowPart;
    }

    //before we connect the interrupt, we have to init ehci
    pdev_ext->ehci->pdev_ext = pdev_ext;

    //init ehci_caps
    // i = ( ( PEHCI_HCS_CONTENT )( &pdev_ext->ehci->ehci_caps.hcs_params ) )->length;

    ehci_get_capabilities(pdev_ext->ehci, pdev_ext->ehci->port_base);
    i = pdev_ext->ehci->ehci_caps.length;
    pdev_ext->ehci->port_base += i;

    if (ehci_init_schedule(pdev_ext->ehci, pdev_ext->padapter) == FALSE)
    {
        release_adapter(pdev_ext->padapter);
        pdev_ext->padapter = NULL;
        ehci_delete_device(pdev);
        return NULL;
    }

    InitializeListHead(&pdev_ext->ehci->urb_list);
    KeInitializeSpinLock(&pdev_ext->ehci->pending_endp_list_lock);
    InitializeListHead(&pdev_ext->ehci->pending_endp_list);

    ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_alloc(): pending_endp_list=0x%x\n",
                                    &pdev_ext->ehci->pending_endp_list));

    init_pending_endp_pool(&pdev_ext->ehci->pending_endp_pool);

    KeInitializeTimer(&pdev_ext->ehci->reset_timer);

    vector = HalGetInterruptVector(PCIBus,
                                   bus,
                                   pdev_ext->res_interrupt.level,
                                   pdev_ext->res_interrupt.vector, &irql, &affinity);

    //connect the interrupt
    DbgPrint("ehci_alloc(): the int=0x%x\n", vector);
    if (IoConnectInterrupt(&pdev_ext->ehci_int, ehci_isr, pdev_ext->ehci, NULL, //&pdev_ext->ehci->frame_list_lock,
                           vector, irql, irql, LevelSensitive, TRUE,    //share the vector
                           affinity, FALSE)     //No float save
        != STATUS_SUCCESS)
    {
        ehci_release(pdev);
        return NULL;
    }

    KeInitializeDpc(&pdev_ext->ehci_dpc, ehci_dpc_callback, (PVOID) pdev_ext->ehci);

    return pdev;
}

PDEVICE_OBJECT
ehci_create_device(PDRIVER_OBJECT drvr_obj, PUSB_DEV_MANAGER dev_mgr)
{
    NTSTATUS status;
    PDEVICE_OBJECT pdev;
    PEHCI_DEVICE_EXTENSION pdev_ext;

    UNICODE_STRING dev_name;
    UNICODE_STRING symb_name;

    STRING string, another_string;
    CHAR str_dev_name[64], str_symb_name[64];
    UCHAR hcd_id;

    if (drvr_obj == NULL)
        return NULL;

    //note: hcd count wont increment till the hcd is registered in dev_mgr
    sprintf(str_dev_name, "%s%d", EHCI_DEVICE_NAME, dev_mgr->hcd_count);
    sprintf(str_symb_name, "%s%d", EHCI_DOS_DEVICE_NAME, dev_mgr->hcd_count);

    RtlInitString(&string, str_dev_name);
    RtlAnsiStringToUnicodeString(&dev_name, &string, TRUE);

    pdev = NULL;
    status = IoCreateDevice(drvr_obj,
                            sizeof(EHCI_DEVICE_EXTENSION) + sizeof(EHCI_DEV),
                            &dev_name, FILE_EHCI_DEV_TYPE, 0, FALSE, &pdev);

    if (status != STATUS_SUCCESS || pdev == NULL)
    {
        RtlFreeUnicodeString(&dev_name);
        ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_create_device(): error create device 0x%x\n", status));
        return NULL;
    }

    pdev_ext = pdev->DeviceExtension;
    RtlZeroMemory(pdev_ext, sizeof(EHCI_DEVICE_EXTENSION) + sizeof(EHCI_DEV));

    pdev_ext->dev_ext_hdr.type = NTDEV_TYPE_HCD;
    pdev_ext->dev_ext_hdr.dispatch = ehci_dispatch_irp;
    pdev_ext->dev_ext_hdr.start_io = NULL;      //we do not support startio
    pdev_ext->dev_ext_hdr.dev_mgr = dev_mgr;

    pdev_ext->pdev_obj = pdev;
    pdev_ext->pdrvr_obj = drvr_obj;

    pdev_ext->ehci = (PEHCI_DEV) & (pdev_ext[1]);

    RtlInitString(&another_string, str_symb_name);
    RtlAnsiStringToUnicodeString(&symb_name, &another_string, TRUE);
    //RtlInitUnicodeString( &symb_name, DOS_DEVICE_NAME );

    IoCreateSymbolicLink(&symb_name, &dev_name);

    ehci_dbg_print(DBGLVL_MAXIMUM,
                   ("ehci_create_device(): dev=0x%x\n, pdev_ext= 0x%x, ehci=0x%x, dev_mgr=0x%x\n", pdev,
                    pdev_ext, pdev_ext->ehci, dev_mgr));

    RtlFreeUnicodeString(&dev_name);
    RtlFreeUnicodeString(&symb_name);

    //register with dev_mgr though it is not initilized
    ehci_init_hcd_interface(pdev_ext->ehci);
    hcd_id = dev_mgr_register_hcd(dev_mgr, &pdev_ext->ehci->hcd_interf);

    pdev_ext->ehci->hcd_interf.hcd_set_id(&pdev_ext->ehci->hcd_interf, hcd_id);
    pdev_ext->ehci->hcd_interf.hcd_set_dev_mgr(&pdev_ext->ehci->hcd_interf, dev_mgr);

    return pdev;

}

VOID
ehci_init_hcd_interface(PEHCI_DEV ehci)
{
    ehci->hcd_interf.hcd_set_dev_mgr = ehci_set_dev_mgr;
    ehci->hcd_interf.hcd_get_dev_mgr = ehci_get_dev_mgr;
    ehci->hcd_interf.hcd_get_type = ehci_get_type;
    ehci->hcd_interf.hcd_set_id = ehci_set_id;
    ehci->hcd_interf.hcd_get_id = ehci_get_id;
    ehci->hcd_interf.hcd_alloc_addr = ehci_alloc_addr;
    ehci->hcd_interf.hcd_free_addr = ehci_free_addr;
    ehci->hcd_interf.hcd_submit_urb = ehci_submit_urb2;
    ehci->hcd_interf.hcd_generic_urb_completion = ehci_generic_urb_completion;
    ehci->hcd_interf.hcd_get_root_hub = ehci_get_root_hub;
    ehci->hcd_interf.hcd_set_root_hub = ehci_set_root_hub;
    ehci->hcd_interf.hcd_remove_device = ehci_remove_device2;
    ehci->hcd_interf.hcd_rh_reset_port = ehci_rh_reset_port;
    ehci->hcd_interf.hcd_release = ehci_hcd_release;
    ehci->hcd_interf.hcd_cancel_urb = ehci_cancel_urb2;
    ehci->hcd_interf.hcd_start = ehci_start;
    ehci->hcd_interf.hcd_dispatch = ehci_hcd_dispatch;

    ehci->hcd_interf.flags = HCD_TYPE_EHCI;     //hcd types | hcd id
}

BOOLEAN
ehci_init_schedule(PEHCI_DEV ehci, PADAPTER_OBJECT padapter)
{
    PEHCI_DEVICE_EXTENSION pdev_ext;
    BOOLEAN ret = TRUE;
    LONG i;
    PEHCI_QH_CONTENT pqh_content;
    PEHCI_QH pqh;
    PEHCI_ELEM_LINKS pelnk;

    if (ehci == NULL)
        return FALSE;

    pdev_ext = ehci->pdev_ext;
    if (pdev_ext == NULL)
        return FALSE;

    // padapter = pdev_ext->padapter;
    if (ehci->frame_count == 0)
        ehci->frame_count = EHCI_DEFAULT_FRAMES;

    // allocate pools
    for(i = 0; i < 5; i++)
    {
        ret = elem_pool_init_pool(&ehci->elem_pools[i], i, padapter);
        if (ret == FALSE)
            break;
    }

    if (ret == FALSE)
    {
        i--;
        for(; i >= 0; i--)
            elem_pool_destroy_pool(&ehci->elem_pools[i]);
        return FALSE;
    }

    // allocate periodic frame list
    ehci->frame_list =
        HalAllocateCommonBuffer(padapter,
                                sizeof(ULONG) * ehci->frame_count, &ehci->frame_list_phys_addr, FALSE);
    if (ehci->frame_list == NULL)
        goto ERROR_OUT;

    RtlZeroMemory(ehci->frame_list, sizeof(ULONG) * ehci->frame_count);
    ehci->frame_list_cpu = usb_alloc_mem(NonPagedPool, sizeof(LIST_HEAD) * ehci->frame_count);

    if (ehci->frame_list_cpu == NULL)
        goto ERROR_OUT;

    for(i = 0; i < (LONG) ehci->frame_count; i++)
    {
        InitializeListHead(&ehci->frame_list_cpu[i].td_link);
    }

    for(i = 0; i < 8; i++)
    {
        InitializeListHead(&ehci->periodic_list_cpu[i]);
    }

    InitializeListHead(&ehci->async_list_cpu);

    // init frame band budget array
    ehci->frame_bw = usb_alloc_mem(NonPagedPool, sizeof(USHORT) * ehci->frame_count * 8);
    if (ehci->frame_bw == NULL)
        goto ERROR_OUT;

    for(i = 0; i < (LONG) ehci->frame_count * 8; i++)
    {
        ehci->frame_bw[i] = EHCI_MAX_SYNC_BUS_TIME;
    }
    ehci->min_bw = EHCI_MAX_SYNC_BUS_TIME;

    // chain the 1ms interrupt qh to the schedule
    if ((pelnk = elem_pool_alloc_elem(&ehci->elem_pools[EHCI_QH_POOL_IDX])) == NULL)
        goto ERROR_OUT;

    pqh_content = (PEHCI_QH_CONTENT) ((ULONG) pelnk->phys_part & PHYS_PART_ADDR_MASK);
    ehci_init_int8_qh(pqh_content);

    // chain qh to the shadow list
    InsertTailList(&ehci->periodic_list_cpu[EHCI_SCHED_INT8_INDEX], &pelnk->sched_link);

    // chain it to the periodic schedule, we use it as a docking point
    // for req of 8- uframes request
    pqh = (PEHCI_QH) pqh_content;

    for(i = 0; i < (LONG) ehci->frame_count; i++)
    {
        ehci->frame_list[i] = pqh->phys_addr;
    }

    // allocate fstn
    /*if( ( pelnk = elem_pool_alloc_elem( &ehci->elem_pools[ EHCI_FSTN_POOL_IDX ] ) ) == NULL )
       goto ERROR_OUT;

       pfstn = ( PEHCI_FSTN )( ( ULONG )pelnk->phys_part & PHYS_PART_ADDR_MASK );
       pfstn->hw_next = EHCI_PTR_TERM;
       pfstn->hw_prev = EHCI_PTR_TERM | ( INIT_LIST_FLAG_QH << 1 );
       InsertTailList( &ehci->periodic_list_cpu[ EHCI_SCHED_FSTN_INDEX ], &pelnk->sched_link );
       pqh->hw_next = pfstn->phys_addr; */

    // allocate for async list head
    if ((pelnk = elem_pool_alloc_elem(&ehci->elem_pools[EHCI_QH_POOL_IDX])) == NULL)
        goto ERROR_OUT;

    // init the async list head
    pqh = (PEHCI_QH) ((ULONG) pelnk->phys_part & PHYS_PART_ADDR_MASK);
    pqh_content = (PEHCI_QH_CONTENT) pqh;
    ehci_init_int8_qh(pqh_content);
    pqh->hw_next = pqh->phys_addr;
    pqh_content->s_mask = 0;
    pqh_content->is_async_head = 1;
    pqh_content->endp_addr = 0;
    ehci->skel_async_qh = pqh;
    InsertTailList(&ehci->async_list_cpu, &pqh->elem_head_link->sched_link);
    return TRUE;

  ERROR_OUT:
    ehci_destroy_schedule(ehci);
    return FALSE;
}

BOOLEAN
ehci_destroy_schedule(PEHCI_DEV ehci)
{
    LONG i;
    if (ehci == NULL)
        return FALSE;

    if (ehci->frame_bw)
        usb_free_mem(ehci->frame_bw);
    ehci->frame_bw = NULL;

    if (ehci->frame_list_cpu)
        usb_free_mem(ehci->frame_list_cpu);
    ehci->frame_list_cpu = NULL;

    if (ehci->frame_list)
        HalFreeCommonBuffer(ehci->pdev_ext->padapter,
                            sizeof(ULONG) * ehci->frame_count,
                            ehci->frame_list_phys_addr, ehci->frame_list, FALSE);

    ehci->frame_list = NULL;
    ehci->frame_list_phys_addr.LowPart = 0;
    ehci->frame_list_phys_addr.HighPart = 0;

    for(i = 0; i < 5; i++)
        elem_pool_destroy_pool(&ehci->elem_pools[i]);

    return TRUE;
}

VOID
ehci_init_int8_qh(PEHCI_QH_CONTENT qh)
{
    if (qh == NULL)
        return;
    // DWORD 0
    qh->terminal = EHCI_PTR_TERM;
    qh->ptr_type = 0;
    qh->reserved = 0;
    qh->next_link = 0;

    // DWORD 1
    qh->dev_addr = 126;         // a fake addr
    qh->inactive = 0;
    qh->endp_addr = 1;          // a fake endp
    qh->endp_spd = USB_SPEED_HIGH;
    qh->data_toggle = 0;
    qh->is_async_head = 0;
    qh->max_packet_size = 64;
    qh->is_ctrl_endp = 0;
    qh->reload_counter = 0;

    // DWORD 2
    qh->s_mask = 0x80;          // we are interrupt qh
    qh->c_mask = 0;
    qh->hub_addr = 0;
    qh->port_idx = 0;
    qh->mult = 1;

    // DWORD 3
    qh->cur_qtd_ptr = 0;        // a terminal

    // overlay
    // !active and !halted
    RtlZeroMemory(&qh->cur_qtd, get_elem_phys_part_size(INIT_LIST_FLAG_QTD));
    qh->cur_qtd.alt_terminal = 1;       // don't use this
    qh->cur_qtd.terminal = 1;
    qh->cur_qtd.status = QTD_STS_HALT;
}

VOID
ehci_get_capabilities(PEHCI_DEV ehci, PBYTE base)
// fetch capabilities register from ehci
{
    PEHCI_CAPS pcap;
    PEHCI_HCS_CONTENT phcs;
    LONG i;

    if (ehci == NULL)
        return;

    pcap = &ehci->ehci_caps;
    pcap->length = EHCI_READ_PORT_UCHAR((PUCHAR) (base + 0));
    pcap->reserved = EHCI_READ_PORT_UCHAR((PUCHAR) (base + 1));
    pcap->hci_version = EHCI_READ_PORT_USHORT((PUSHORT) (base + 2));
    pcap->hcs_params = EHCI_READ_PORT_ULONG((PULONG) (base + 4));
    pcap->hcc_params = EHCI_READ_PORT_ULONG((PULONG) (base + 8));

    phcs = (PEHCI_HCS_CONTENT) & pcap->hcs_params;
    if (phcs->port_rout_rules)
    {
        for(i = 0; i < 8; i++)
            pcap->portroute[i] = EHCI_READ_PORT_UCHAR((PUCHAR) (base + 12 + i));
    }
    return;
}

BOOLEAN
ehci_delete_device(PDEVICE_OBJECT pdev)
{
    STRING string;
    UNICODE_STRING symb_name;
    CHAR str_symb_name[64];
    PEHCI_DEVICE_EXTENSION pdev_ext;

    if (pdev == NULL)
        return FALSE;

    pdev_ext = pdev->DeviceExtension;

    sprintf(str_symb_name,
            "%s%d", EHCI_DOS_DEVICE_NAME, pdev_ext->ehci->hcd_interf.hcd_get_id(&pdev_ext->ehci->hcd_interf));

    RtlInitString(&string, str_symb_name);
    RtlAnsiStringToUnicodeString(&symb_name, &string, TRUE);
    IoDeleteSymbolicLink(&symb_name);
    RtlFreeUnicodeString(&symb_name);

    if (pdev_ext->res_list)
        ExFreePool(pdev_ext->res_list); //      not allocated by usb_alloc_mem

    IoDeleteDevice(pdev);
    ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_delete_device(): device deleted\n"));
    return TRUE;
}

VOID
ehci_stop(PEHCI_DEV ehci)
{
    PBYTE base;
    PEHCI_USBCMD_CONTENT usbcmd;
    LONG tmp;

    base = ehci->port_base;
    // turn off all the interrupt
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_USBINTR), 0);
    tmp = EHCI_READ_PORT_ULONG((PULONG) (base + EHCI_USBCMD));
    usbcmd = (PEHCI_USBCMD_CONTENT) & tmp;
    usbcmd->run_stop = 0;
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_USBCMD), tmp);
}

BOOLEAN
ehci_release(PDEVICE_OBJECT pdev)
{
    PEHCI_DEVICE_EXTENSION pdev_ext;
    PEHCI_DEV ehci;

    if (pdev == NULL)
        return FALSE;

    pdev_ext = pdev->DeviceExtension;

    if (pdev_ext == NULL)
        return FALSE;

    ehci = pdev_ext->ehci;
    if (ehci == NULL)
        return FALSE;

    ehci_stop(ehci);

    if (pdev_ext->ehci_int)
    {
        IoDisconnectInterrupt(pdev_ext->ehci_int);
        pdev_ext->ehci_int = NULL;
    }
    else
        TRAP();
    destroy_pending_endp_pool(&pdev_ext->ehci->pending_endp_pool);

    ehci_destroy_schedule(ehci);

    release_adapter(pdev_ext->padapter);
    pdev_ext->padapter = NULL;

    ehci_delete_device(pdev);

    return FALSE;

}

BOOLEAN
ehci_start(PHCD hcd)
{
    ULONG tmp;
    PBYTE base;
    PEHCI_USBCMD_CONTENT usbcmd;
    PEHCI_DEV ehci;

    if (hcd == NULL)
        return FALSE;

    ehci = struct_ptr(hcd, EHCI_DEV, hcd_interf);
    base = ehci->port_base;

    // stop the controller
    tmp = EHCI_READ_PORT_ULONG((PULONG) (base + EHCI_USBCMD));
    usbcmd = (PEHCI_USBCMD_CONTENT) & tmp;
    usbcmd->run_stop = 0;
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_USBCMD), tmp);

    // wait the controller stop( ehci spec, 16 microframe )
    usb_wait_ms_dpc(2);

    // reset the controller
    usbcmd = (PEHCI_USBCMD_CONTENT) & tmp;
    usbcmd->hcreset = TRUE;
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_USBCMD), tmp);

    for(;;)
    {
        // interval.QuadPart = -100 * 10000; // 10 ms
        // KeDelayExecutionThread( KernelMode, FALSE, &interval );
        KeStallExecutionProcessor(10);
        tmp = EHCI_READ_PORT_ULONG((PULONG) (base + EHCI_USBCMD));
        if (!usbcmd->hcreset)
            break;
    }

    // prepare the registers
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_CTRLDSSEGMENT), 0);

    // turn on all the int
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_USBINTR),
                          EHCI_USBINTR_INTE | EHCI_USBINTR_ERR | EHCI_USBINTR_ASYNC | EHCI_USBINTR_HSERR
                          // EHCI_USBINTR_FLROVR | \  // it is noisy
                          // EHCI_USBINTR_PC |    // we detect it by polling
        );
    // write the list base reg
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_PERIODICLISTBASE), ehci->frame_list_phys_addr.LowPart);

    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_ASYNCLISTBASE), ehci->skel_async_qh->phys_addr & ~(0x1f));

    usbcmd->int_threshold = 1;
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_USBCMD), tmp);

    // let's rock
    usbcmd->run_stop = 1;
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_USBCMD), tmp);

    // set the configuration flag
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_CONFIGFLAG), 1);

    // enable the list traversaling
    usbcmd->async_enable = 1;
    usbcmd->periodic_enable = 1;
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_USBCMD), tmp);

    return TRUE;
}

VOID
ehci_run_stop(PEHCI_DEV ehci, BOOLEAN start)
{
    PEHCI_USBCMD_CONTENT usbcmd;
    PEHCI_USBSTS_CONTENT usbsts;
    ULONG tmp;
    PBYTE base;

    if (ehci == NULL)
        return;

    base = ehci->port_base;
    usbcmd = (PEHCI_USBCMD_CONTENT) & tmp;
    usbsts = (PEHCI_USBSTS_CONTENT) & tmp;

    tmp = EHCI_READ_PORT_ULONG((PULONG) (base + EHCI_USBSTS));
    if (start && usbsts->hc_halted == 0)
    {
        TRAP();
        usb_dbg_print(DBGLVL_MEDIUM, ("ehci_run_stop(): WARNING: hc running, can not start again\n"));
        return;
    }
    tmp = EHCI_READ_PORT_ULONG((PULONG) (base + EHCI_USBCMD));
    usbcmd->run_stop = start ? 1 : 0;
    EHCI_WRITE_PORT_ULONG((PULONG) (base + EHCI_USBCMD), tmp);
    if (start)
        usb_wait_ms_dpc(2);     //ehci spec 16 microframes
}

VOID
ehci_find_min_bandwidth(PEHCI_DEV ehci)
{
    LONG i;
    if (ehci == NULL)
        return;

    for(i = 0; i < (LONG) (ehci->frame_count << 3); i++)
    {
        ehci->min_bw = ehci->min_bw < ehci->frame_bw[i] ? ehci->min_bw : ehci->frame_bw[i];
    }
    return;
}

BOOLEAN
ehci_claim_bw_for_int(PEHCI_DEV ehci, PURB purb, BOOLEAN release)
// should have pending_endp_list_lock acquired, and purb->pipe prepared
{
    PURB_HS_PIPE_CONTENT pipe_content;
    LONG i, j;
    ULONG interval, bus_time, ss_time, cs_time;
    BOOLEAN bw_avail;
    ULONG max_packet_size;

    if (ehci == NULL || purb == NULL)
        return FALSE;

    if (purb->pipe == 0)
        return FALSE;

    bw_avail = FALSE;
    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    interval = REAL_INTERVAL;
    if (pipe_content->speed_high == 0)
    {
        // translate to high speed uframe count
        interval <<= 3;
    }

    if (interval > (ehci->frame_count << 3))
        interval = (ehci->frame_count << 3);

    max_packet_size = (1 << pipe_content->max_packet_size);
    if (pipe_content->speed_high)
    {
        // this is a high speed endp

        bus_time = usb_calc_bus_time(USB_SPEED_HIGH,
                                     (pipe_content->trans_dir) << 7,
                                     FALSE, min(purb->data_length, (LONG) max_packet_size));

        // multiple transactions per uframe
        if (purb->data_length > (LONG) max_packet_size)
        {
            bus_time *= (purb->data_length) / max_packet_size;
            bus_time += usb_calc_bus_time(USB_SPEED_HIGH,
                                          (pipe_content->trans_dir) << 7,
                                          FALSE, purb->data_length % max_packet_size);
        }
        bus_time = (bus_time + 1) >> 1;

        if (release)
        {
            for(i = pipe_content->start_uframe + (purb->int_start_frame << 3);
                i < (LONG) (ehci->frame_count << 3); i += interval)
            {
                ehci->frame_bw[i] += (USHORT) bus_time;
            }
            goto LBL_OUT;
        }
        if (bus_time < ehci->min_bw)
        {
            // start the poll from uframe zero of each frame
            bw_avail = TRUE;
            pipe_content->start_uframe = 0;
            for(i = 0; i < (LONG) (ehci->frame_count << 3); i += interval)
            {
                ehci->frame_bw[i] -= (USHORT) bus_time;
                if (ehci->frame_bw[i] < ehci->min_bw)
                    ehci->min_bw = ehci->frame_bw[i];
            }
        }
        else                    // bother to find a pattern
        {
            for(j = 0; j < (LONG) interval; j++)
            {
                for(i = j; i < (LONG) (ehci->frame_count << 3); i += interval)
                {
                    if (ehci->frame_bw[i] < bus_time)
                        break;
                }
                if (i > (LONG) (ehci->frame_count << 3))
                {
                    bw_avail = TRUE;
                    break;
                }
            }
            if (bw_avail)
            {
                for(i = j; i < (LONG) (ehci->frame_count << 3); i += interval)
                {
                    ehci->frame_bw[i] -= (USHORT) bus_time;
                    if (ehci->frame_bw[i] < ehci->min_bw)
                        ehci->min_bw = ehci->frame_bw[i];
                }
                pipe_content->start_uframe = j & 7;
                purb->int_start_frame = j >> 3;
            }
        }
    }
    else                        // full/low speed pipe
    {
        // split condition is considered
        if (pipe_content->trans_dir)
        {
            // an input interrupt, with handshake
            // 55 is 144 - 90 + 1, turnaround time is one byte not the worst case 90 bytes,
            // refer to ehci-1.0 table 4-5 p64
            ss_time = 231 * 25 / 12;
            // cs_time = ( 55 * 8 + ( LONG )( ( ( 19 + 7 * 8 * purb->data_length ) / 6 ) ) ) * 25 / 12;
            cs_time = (55 * 8 + (LONG) (((7 * 8 * purb->data_length) / 6))) * 25 / 12;
        }
        else
        {
            // according to ehci-1.0 table 4-5 p64
            ss_time = (49 * 8 + (LONG) (((7 * 8 * purb->data_length) / 6))) * 25 / 12;
            // 287 = 237 + 48( handshake ) + 8( turn around time )
            cs_time = 287 * 25 / 12;
        }
        ss_time >>= 1, cs_time >>= 1;
        if (release)
        {
            for(i = pipe_content->start_uframe + (purb->int_start_frame << 3);
                i < (LONG) (ehci->frame_count << 3); i += interval)
            {
                ehci->frame_bw[i] += (USHORT) ss_time;
                ehci->frame_bw[i + 2] += (USHORT) cs_time;
                ehci->frame_bw[i + 3] += (USHORT) cs_time;
                if ((i & 0x07) != 0x06)
                    ehci->frame_bw[i + 4] += (USHORT) cs_time;
            }
            goto LBL_OUT;
        }
        if (ss_time < ehci->min_bw && cs_time < ehci->min_bw)
        {
            pipe_content->start_uframe = 0;
            bw_avail = TRUE;
            for(i = 0; i < (LONG) (ehci->frame_count << 3); i += interval)
            {
                ehci->frame_bw[i] -= (USHORT) ss_time;
                ehci->min_bw = min(ehci->frame_bw[i], ehci->min_bw);
                ehci->frame_bw[i + 2] -= (USHORT) cs_time;
                ehci->min_bw = min(ehci->frame_bw[i + 2], ehci->min_bw);
                ehci->frame_bw[i + 3] -= (USHORT) cs_time;
                ehci->min_bw = min(ehci->frame_bw[i + 3], ehci->min_bw);
                if ((i & 0x07) != 0x06)
                {
                    ehci->frame_bw[i + 4] -= (USHORT) cs_time;
                    ehci->min_bw = min(ehci->frame_bw[i + 4], ehci->min_bw);
                }
            }
        }
        else
        {
            for(j = 0; j < (LONG) interval; j++)
            {
                if ((j & 0x7) == 7)     // start-split not allowed at this uframe
                    continue;

                for(i = j; i < (LONG) (ehci->frame_count << 3); i += interval)
                {
                    if (ehci->frame_bw[i] < ss_time)
                        break;
                    if (ehci->frame_bw[i + 2] < cs_time)
                        break;
                    if (ehci->frame_bw[i + 3] < cs_time)
                        break;
                    if ((i & 0x7) != 6)
                        if (ehci->frame_bw[i + 4] < cs_time)
                            break;
                }
                if (i > (LONG) (ehci->frame_count << 3))
                {
                    bw_avail = TRUE;
                    break;
                }
            }

            pipe_content->start_uframe = j & 7;
            purb->int_start_frame = j >> 3;

            for(i = j; i < (LONG) (ehci->frame_count << 3); i += interval)
            {
                ehci->frame_bw[i] -= (USHORT) ss_time;
                ehci->min_bw = min(ehci->frame_bw[i], ehci->min_bw);
                ehci->frame_bw[i + 2] -= (USHORT) cs_time;
                ehci->min_bw = min(ehci->frame_bw[i + 2], ehci->min_bw);
                ehci->frame_bw[i + 3] -= (USHORT) cs_time;
                ehci->min_bw = min(ehci->frame_bw[i + 3], ehci->min_bw);
                if ((i & 0x7) != 6)
                {
                    ehci->frame_bw[i + 4] -= (USHORT) cs_time;
                    ehci->min_bw = min(ehci->frame_bw[i + 4], ehci->min_bw);
                }
            }
        }
    }

  LBL_OUT:
    if (!release)
        return bw_avail;
    else
    {
        ehci_find_min_bandwidth(ehci);
        return TRUE;
    }
}

LONG
ehci_get_cache_policy(PEHCI_DEV ehci)
{
    PEHCI_HCC_CONTENT hcc_content;

    hcc_content = (PEHCI_HCC_CONTENT) & ehci->ehci_caps;
    if (hcc_content->iso_sched_threshold >= 8)
        return 16;
    if (hcc_content->iso_sched_threshold == 0)
        return 2;
    return hcc_content->iso_sched_threshold + 2;
}

// 25/12 is bus-time per bit ( 1000 / 480 )
#define BEST_BUDGET_TIME_UFRAME  ( ( 188 * 7 / 6 ) * 25 / 12 )

//  in: 231 is sum of split token + host ipg + token, 8 is bus turn-around time, 67 is full speed data token in DATA packet
// out: 49 byte is sum of split token+ host ipg + token + host ipg + data packet
#define iso_max_data_load( dir ) ( dir == USB_DIR_IN ? \
		( ( 188 * 8 - 231 - 8 - 67 + ( 8 - 1 ) ) / 8 ) : ( 188 - 49 ) )

#define iso_uframes_data_load( dir, data_load, uf_cnt )

BOOLEAN
ehci_claim_bw_for_iso(PEHCI_DEV ehci, PURB purb, BOOLEAN release)
{
    PURB_HS_PIPE_CONTENT pipe_content;
    LONG i, j, k;
    ULONG interval, bus_time, ss_time, cs_time, remainder;
    BOOLEAN bw_avail;
    ULONG cur_uframe, start_uframe = 0, max_time, max_packet_size;
    PBYTE base;
    if (ehci == NULL || purb == NULL)
        return FALSE;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    base = ehci->port_base;
    cur_uframe = EHCI_READ_PORT_ULONG((PULONG) (base + EHCI_FRINDEX)) + 1;
    bw_avail = FALSE;

    max_packet_size = purb->params[0];  //( 1 << pipe_content->max_packet_size );

    if (pipe_content->speed_high)
    {
        interval = REAL_INTERVAL;
        if (purb->iso_frame_count == 0 || purb->iso_frame_count + 2 * 8 > (LONG) (ehci->frame_count << 3))
            return FALSE;

        for(i = 0, max_time = 0; i < (LONG) purb->iso_frame_count; i++)
        {
            bus_time = usb_calc_bus_time(USB_SPEED_HIGH,
                                         (pipe_content->trans_dir) << 7,
                                         TRUE, min(purb->iso_packet_desc[i].length, (LONG) max_packet_size));
            // NOTE: we did not use endp_mult_count here, because the comparison is enough
            // to calculate the bandwidth
            if (purb->iso_packet_desc[i].length > (LONG) max_packet_size)
            {
                // multiple transactions per uframe
                bus_time *= purb->iso_packet_desc[i].length / max_packet_size;
                bus_time += usb_calc_bus_time(USB_SPEED_HIGH,
                                              (pipe_content->trans_dir) << 7,
                                              TRUE, purb->iso_packet_desc[i].length % max_packet_size);
            }
            bus_time = (bus_time + 1) >> 1;
            max_time = max(bus_time, max_time);
            purb->iso_packet_desc[i].bus_time = bus_time;
        }

        if (release)
        {
            // it is a release operation
            for(i = purb->iso_start_frame, k = 0; k < (LONG) purb->iso_frame_count;
                k++, i = (i + interval) % (ehci->frame_count << 3))
            {
                ehci->frame_bw[i] += (USHORT) purb->iso_packet_desc[k].bus_time;
            }
            ehci_find_min_bandwidth(ehci);
            return TRUE;
        }

        if (max_time < ehci->min_bw)
        {
            start_uframe = cur_uframe + ehci_get_cache_policy(ehci);    // avoid cache
            for(i = start_uframe, j = 0; j < (LONG) purb->iso_frame_count;
                i = (i + interval) % (ehci->frame_count << 3), j++)
            {
                ehci->frame_bw[i] -= (USHORT) purb->iso_packet_desc[j].bus_time;
                ehci->min_bw = min(ehci->frame_bw[j], ehci->min_bw);
            }
            purb->iso_start_frame = start_uframe;
            return TRUE;
        }
        else                    // max_time >= ehci->min_bw
        {
            for(j = 0; j < (LONG) interval; j++)
            {
                start_uframe = cur_uframe + ehci_get_cache_policy(ehci) + j;
                for(i = start_uframe, k = 0; k < (LONG) purb->iso_frame_count;
                    k++, i = (i + interval) % (ehci->frame_count << 3))
                {
                    if (ehci->frame_bw[i] < (USHORT) purb->iso_packet_desc[k].bus_time)
                    {
                        break;
                    }
                }

                if (k < (LONG) purb->iso_frame_count)
                    continue;
                bw_avail = TRUE;
                break;
            }
            if (bw_avail)
            {
                // allocate the bandwidth
                for(i = start_uframe, k = 0; k < (LONG) purb->iso_frame_count;
                    k++, i = (i + interval) % (ehci->frame_count << 3))
                {
                    ehci->frame_bw[i] -= (USHORT) purb->iso_packet_desc[k].bus_time;
                    ehci->min_bw = min(ehci->min_bw, ehci->frame_bw[i]);
                }
                purb->iso_start_frame = start_uframe;
            }
        }
    }
    else                        // not high speed endpoint
    {
        // split transfer
        if (purb->iso_frame_count == 0 || purb->iso_frame_count + 2 > (LONG) ehci->frame_count)
            return FALSE;

        if (max_packet_size > 1023)
            return FALSE;

        remainder = 0;

        //
        // calculate for each frame
        // in: 231 is sum of split token + host ipg + token, 8 is bus turn-around time, 67 is full speed data token in DATA packet
        // out: 49 byte is sum of split token+ host ipg + token + host ipg + data packet
        // bit-stuffing is for high speed bus transfer
        //

        if (pipe_content->trans_dir)
        {
            // an input transfer, no handshake
            ss_time = 231 * 25 / 12;
            // cs_time = ( 231 + 8 + 67 + ( LONG )( ( ( 19 + 7 * 8 * 188 ) / 6 ) ) ) * 25 / 12;
            cs_time = (231 + 8 + 67 + (LONG) (((7 * 8 * 188) / 6))) * 25 / 12;
        }
        else
        {
            // an output transfer according to ehci-1.0 table 4-5 p64
            // ss_time = ( 49 * 8 + ( LONG )( ( ( 19 + 7 * 8 * 188 ) / 6 ) ) ) * 25 / 12;
            ss_time = (49 * 8 + (LONG) (((7 * 8 * 188) / 6))) * 25 / 12;
            cs_time = 0;
            for(i = 0; i < (LONG) purb->iso_frame_count; i++)
            {
                // remainder = ( 49 * 8 + ( LONG )( ( ( 19 + 7 * 8 * ( purb->iso_packet_desc[ i ].length % 188 ) ) / 6 ) ) ) * 25 / 12;
                remainder =
                    (49 * 8 + (LONG) (((7 * 8 * (purb->iso_packet_desc[i].length % 188)) / 6))) * 25 / 12;
                remainder >>= 1;
                purb->iso_packet_desc[i].params.bus_time = (USHORT) remainder;
            }
        }

        ss_time >>= 1;
        cs_time >>= 1;
        cur_uframe = (cur_uframe + 7) & (~0x07);

        j = ehci->frame_count << 3;
        if (release)
        {
            if (pipe_content->trans_dir)
            {
                for(i = 0; i < (LONG) purb->iso_frame_count; i++)
                {
                    ehci->frame_bw[purb->iso_packet_desc[i].params.start_uframe] += (USHORT) ss_time;
                    for(k = 0; k < (purb->iso_packet_desc[i].length + 187) / 188; k++)
                    {
                        ehci->frame_bw[(cur_uframe + 0x12 + (i << 3) + k) % j] += (USHORT) cs_time;
                    }

                    // two extra complete-split
                    ehci->frame_bw[(cur_uframe + 0x12 + (i << 3) + k) % j] += (USHORT) cs_time;
                    ehci->frame_bw[(cur_uframe + 0x13 + (i << 3) + k) % j] += (USHORT) cs_time;
                }
            }
            else
            {
                for(i = 0; i < (LONG) purb->iso_frame_count; i++)
                {
                    for(k = 0; k < (purb->iso_packet_desc[i].length + 187) / 188; k++)
                    {
                        ehci->frame_bw[(cur_uframe + 0x10 + (i << 3) + k) % j] += (USHORT) ss_time;
                    }
                }
            }
            ehci_find_min_bandwidth(ehci);
        }

        // search for available bw
        if (ss_time < ehci->min_bw && cs_time < ehci->min_bw)
        {
            if (pipe_content->trans_dir)
            {
                for(i = 0; i < (LONG) purb->iso_frame_count; i++)
                {
                    ehci->frame_bw[(cur_uframe + 0x10 + (i << 3)) % j] -= (USHORT) ss_time;
                    ehci->min_bw = min(ehci->frame_bw[(cur_uframe + 0x10 + (i << 3)) % j], ehci->min_bw);

                    for(k = 0; k < (purb->iso_packet_desc[i].length + 187) / 188; k++)
                    {
                        ehci->frame_bw[(cur_uframe + 0x12 + (i << 3) + k) % j] -= (USHORT) cs_time;
                        ehci->min_bw =
                            min(ehci->frame_bw[(cur_uframe + 0x12 + (i << 3) + k) % j], ehci->min_bw);
                    }

                    // two extra complete-split
                    ehci->frame_bw[(cur_uframe + 0x12 + (i << 3) + k) % j] -= (USHORT) cs_time;
                    ehci->min_bw = min(ehci->frame_bw[cur_uframe + 0x12 + (i << 3) + k], ehci->min_bw);
                    ehci->frame_bw[(cur_uframe + 0x13 + (i << 3) + k) % j] -= (USHORT) cs_time;
                    ehci->min_bw = min(ehci->frame_bw[cur_uframe + 0x13 + (i << 3) + k], ehci->min_bw);
                }
            }
            else                // iso output
            {
                for(i = 0; i < (LONG) purb->iso_frame_count; i++)
                {
                    for(k = 0; k < (purb->iso_packet_desc[i].length + 187) / 188; k++)
                    {
                        ehci->frame_bw[(cur_uframe + 0x10 + (i << 3) + k) % j] -= (USHORT) ss_time;
                        ehci->min_bw =
                            min(ehci->frame_bw[(cur_uframe + 0x11 + (i << 3) + k) % j], ehci->min_bw);
                    }
                }
            }
            purb->iso_start_frame = 0;
            for(i = 0; i < (LONG) purb->iso_frame_count; i++)
            {
                if (i == 0)
                    purb->iso_packet_desc[i].params.start_uframe = (USHORT) (cur_uframe + 0x10);
                else
                    purb->iso_packet_desc[i].params.start_uframe =
                        purb->iso_packet_desc[i - 1].params.start_uframe + 0x8;
            }
            bw_avail = TRUE;
        }
        else                    // take the pain to find one
        {
            BOOLEAN large;
            long temp, base;

            for(j = (cur_uframe >> 3) + 2; j != (LONG) (cur_uframe >> 3); j = (j + 1) % ehci->frame_count)
            {
                temp = 0;

                for(i = 0; i < (LONG) purb->iso_frame_count; i++)
                {
                    large = purb->iso_packet_desc[i].length > 579;
                    base = (purb->iso_packet_desc[i].length + 187) / 188;

                    if (base > 6)
                        return FALSE;

                    if (pipe_content->trans_dir)
                    {
                        // input split iso, for those large than 579, schedule it at the uframe boundary
                        for(temp = 0; temp < (large == FALSE) ? (8 - base - 1) : 1; temp++)
                        {
                            k = (((j + i) << 3) + temp) % (ehci->frame_count << 3);
                            if (ehci->frame_bw[k] > ss_time)
                                continue;

                            k = base;
                            while (k != 0)
                            {
                                if (ehci->
                                    frame_bw[(((j + i) << 3) + 1 + temp + k) % (ehci->frame_count << 3)] <
                                    cs_time)
                                    break;
                                k--;
                            }
                            if (k > 0)  // not available
                                continue;

                            // the first following extra cs
                            k = (((j + i) << 3) + 2 + temp + base) % (ehci->frame_count << 3);
                            if (ehci->frame_bw[k] < cs_time)
                                continue;

                            if (base < 6)
                            {
                                // very large one does not have this second extra cs
                                if (ehci->frame_bw[(k + 1) % (ehci->frame_count << 3)] < cs_time)
                                    continue;
                            }
                        }

                        if (temp == 8 - 1 - base)       // no bandwidth for ss
                            break;
                    }
                    else        // output
                    {
                        // note: 8 - 1 - base has different meaning from the above
                        // it is to avoid the ss on H-Frame 7, but the above one is
                        // the latency of the classic bus.
                        for(temp = 0; temp < 8 - 1 - base; temp++)
                        {
                            if (ehci->frame_bw[((j + i) << 3) % (ehci->frame_count << 3) + temp] > ss_time)
                                continue;

                            for(k = temp; k < temp + base; k++)
                            {
                                if (ehci->frame_bw[((j + i) << 3) % (ehci->frame_count << 3) + k] < ss_time)
                                    break;
                            }
                        }

                        if (temp == 8 - 1 - base)
                            break;
                    }

                    purb->iso_packet_desc[i].params.start_uframe =
                        (USHORT) ((((j + i) << 3) + temp) % (ehci->frame_count << 3));
                    // next frame
                }
                if (i < (LONG) purb->iso_frame_count)
                {
                    // skip to the next section
                    j = (j + i) % (ehci->frame_count << 3);
                    continue;
                }
                bw_avail = TRUE;
                break;
            }
            // FIXME: Should we claim bw for the last complete split sitd? this is not done
            // yet.
            if (bw_avail)
            {
                if (pipe_content->trans_dir)
                {
                    // input iso
                    for(i = 0; i < (LONG) purb->iso_frame_count; i++)
                    {
                        j = purb->iso_packet_desc[i].length;
                        temp = (purb->iso_packet_desc[i].params.start_uframe) % (ehci->frame_count << 3);
                        ehci->frame_bw[temp] -= (USHORT) ss_time;
                        for(k = 0; k < (j + 187) / 188; j++)
                        {
                            ehci->frame_bw[temp + 2 + k] -= (USHORT) cs_time;
                        }
                        ehci->frame_bw[temp + 2 + k] -= (USHORT) cs_time;
                        if ((j + 187) / 188 < 6)        //ehci restriction
                        {
                            ehci->frame_bw[temp + 3 + k] -= (USHORT) cs_time;
                        }
                    }
                }
                else            //output iso
                {
                    for(i = 0; i < (LONG) purb->iso_frame_count; i++)
                    {
                        j = purb->iso_packet_desc[i].length;
                        temp = (purb->iso_packet_desc[i].params.start_uframe) % (ehci->frame_count << 3);
                        for(k = 0; k < j / 188; j++)
                        {
                            ehci->frame_bw[temp + k] -= (USHORT) ss_time;
                        }
                        if (j % 188)
                            ehci->frame_bw[temp + k] -= purb->iso_packet_desc[i].params.bus_time;
                    }
                }
            }
        }
    }
    return bw_avail;
}

BOOLEAN
ehci_claim_bandwidth(PEHCI_DEV ehci, PURB purb, BOOLEAN claim_bw)  //true to claim band-width, false to free band-width
// should have pending_endp_list_lock acquired, and purb->pipe prepared
{
    PURB_HS_PIPE_CONTENT pipe_content;
    BOOLEAN ret;

    if (ehci == NULL || purb == NULL)
        return FALSE;

    ret = FALSE;
    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    if (pipe_content->trans_type == USB_ENDPOINT_XFER_ISOC)
    {
        ret = ehci_claim_bw_for_iso(ehci, purb, claim_bw ? FALSE : TRUE);
    }
    else if (pipe_content->trans_type == USB_ENDPOINT_XFER_INT)
    {
        ret = ehci_claim_bw_for_int(ehci, purb, claim_bw ? FALSE : TRUE);
    }
    else
        TRAP();
    return ret;
}

BOOLEAN
ehci_can_remove(PURB purb, BOOLEAN door_bell_rings, ULONG cur_frame)
// test if the purb can be removed from the schedule, by current frame index and
// purb states
{
    PURB_HS_PIPE_CONTENT pipe_content;
    ULONG interval;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    interval = REAL_INTERVAL;

    switch (purb->flags & URB_FLAG_STATE_MASK)
    {
        case URB_FLAG_STATE_PENDING:
        {
            // not impossible
            TRAP();
            break;
        }
        case URB_FLAG_STATE_IN_PROCESS:
        {
            break;
        }
        case URB_FLAG_STATE_DOORBELL:
        {
            if ((pipe_content->trans_type == USB_ENDPOINT_XFER_BULK ||
                 pipe_content->trans_type == USB_ENDPOINT_XFER_CONTROL) && door_bell_rings == TRUE)
            {
                return TRUE;
            }
            else if ((pipe_content->trans_type == USB_ENDPOINT_XFER_BULK ||
                      pipe_content->trans_type == USB_ENDPOINT_XFER_CONTROL))
            {
                break;
            }
            else
            {
                TRAP();
                break;
            }
        }
        case URB_FLAG_STATE_WAIT_FRAME:
        {
            // need more processing
            if ((purb->flags & URB_FLAG_FORCE_CANCEL) == 0)
            {
                TRAP();
                break;
            }
            if (pipe_content->trans_type == USB_ENDPOINT_XFER_INT)
            {
                return door_bell_rings;
            }
            else                // isochronous can not be canceled
            {
                TRAP();
                break;
            }
        }
    }
    return FALSE;
}

NTSTATUS ehci_remove_urb_from_schedule(PEHCI_DEV ehci, PURB purb);

static VOID
ehci_deactivate_urb(PURB purb)
{
    PURB_HS_PIPE_CONTENT pipe_content;
    PLIST_ENTRY pthis, pnext;
    PEHCI_QH_CONTENT pqh_content;
    PEHCI_QTD_CONTENT pqtd_content;

    if (purb == NULL)
        return;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    switch (pipe_content->trans_type)
    {
        case USB_ENDPOINT_XFER_CONTROL:
        case USB_ENDPOINT_XFER_BULK:
        case USB_ENDPOINT_XFER_INT:
        {
            ListFirst(&purb->trasac_list, pthis);
            pqh_content = (PEHCI_QH_CONTENT) qh_from_list_entry(pthis);
            ListNext(&purb->trasac_list, pthis, pnext);
            do
            {
                pqtd_content = (PEHCI_QTD_CONTENT) qtd_from_list_entry(pthis);
                if (pqtd_content->status & QTD_STS_ACTIVE)
                {
                    pqtd_content->status &= ~QTD_STS_ACTIVE;
                }
                ListNext(&purb->trasac_list, pthis, pnext);
                pthis = pnext;

            }
            while (pthis);
            break;
        }
        case USB_ENDPOINT_XFER_ISOC:
        {
            // fall through
        }
        default:
            TRAP();
    }
    return;
}

static VOID
ehci_insert_bulk_schedule(PEHCI_DEV ehci, PURB purb)
// list head is only a handle, the qh and qtd are following it.
{
    PLIST_ENTRY list_head;
    PEHCI_QH pqh, pqhprev, pqhnext;
    PLIST_ENTRY pthis, pprev, pnext;

    if (ehci == NULL || purb == NULL)
        return;

    list_head = &purb->trasac_list;
    ListFirst(list_head, pthis);
    if (pthis == NULL)
        return;

    if (elem_type_list_entry(pthis) != INIT_LIST_FLAG_QH)
        return;

    pqh = qh_from_list_entry(pthis);
    // the last qh
    ListFirstPrev(&ehci->async_list_cpu, pprev);
    pqhprev = qh_from_schedule(pprev);

    // the first qh
    ListFirst(&ehci->async_list_cpu, pnext);
    pqhnext = qh_from_schedule(pnext);

    if (pprev == &ehci->async_list_cpu)
    {
        // always a qh in async list
        TRAP();
        return;
    }
    pqh->hw_next = pqhnext->phys_addr;
    InsertTailList(&ehci->async_list_cpu, &pqh->elem_head_link->sched_link);
    pqhprev->hw_next = pqh->phys_addr;
    return;
}

static VOID
ehci_remove_bulk_from_schedule(PEHCI_DEV ehci, PURB purb)
// executed in isr, and have frame_list_lock acquired, so
// never try to acquire any spin-lock
// remove the bulk purb from schedule, and mark it not in
// the schedule
{
    PLIST_ENTRY list_head;
    PEHCI_QH pqh, pqhprev, pqhnext;
    PEHCI_QH_CONTENT pqhc;
    PLIST_ENTRY pthis, pprev, pnext;

    if (ehci == NULL || purb == NULL)
        return;

    list_head = &purb->trasac_list;
    ListFirst(list_head, pthis);
    if (pthis == NULL)
    {
        TRAP();
        return;
    }
    pqh = qh_from_list_entry(pthis);
    pqhc = (PEHCI_QH_CONTENT) pqh;

    if (pqhc->is_async_head)
        TRAP();

    ListFirst(&pqh->elem_head_link->sched_link, pnext);
    ListFirstPrev(&pqh->elem_head_link->sched_link, pprev);

    if (pprev == &ehci->async_list_cpu)
    {
        // we will at least have a qh with H-bit 1 in the async-list
        TRAP();
    }
    else if (pnext == &ehci->async_list_cpu)
    {
        // remove the last one
        pqhprev = qh_from_schedule(pprev);
        ListFirst(&ehci->async_list_cpu, pnext);
        pqhnext = qh_from_schedule(pnext);
        pqhprev->hw_next = pqhnext->phys_addr;
    }
    else
    {
        pqhprev = qh_from_schedule(pprev);
        pqhnext = qh_from_schedule(pnext);
        pqhprev->hw_next = pqhnext->phys_addr;
    }
    RemoveEntryList(&pqh->elem_head_link->sched_link);
    return;
}


static VOID
ehci_insert_fstn_schedule(PEHCI_DEV ehci, PURB purb)
{

    PURB_HS_PIPE_CONTENT pipe_content, pc;
    PLIST_ENTRY pthis, list_head, pnext = NULL, pprev;
    PEHCI_QH pqhnext;
    PEHCI_FSTN pfstn;
    PURB purb1;

    ULONG interval, start_frame, start_uframe;
    LONG i;

    if (ehci == NULL || purb == NULL)
        return;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    interval = (1 << (pipe_content->interval + 3));
    list_head = &purb->trasac_list;
    start_frame = purb->int_start_frame;
    start_uframe = (start_frame << 3) + 1;      //( start_frame << 3 ) + pipe_content->start_uframe;

    if ((start_frame << 3) >= interval)
        TRAP();

    ListFirstPrev(list_head, pprev);

    if (elem_type_list_entry(pprev) != INIT_LIST_FLAG_FSTN)
    {
        TRAP();
        return;
    }

    pfstn = fstn_from_list_entry(pprev);

    if (interval == 8)
    {
        ListFirst(&ehci->periodic_list_cpu[EHCI_SCHED_INT8_INDEX], pthis);

        // skip the first one
        ListNext(&ehci->periodic_list_cpu[EHCI_SCHED_INT8_INDEX], pthis, pnext);
        pprev = pthis;
        pthis = pnext;

        while (pthis)
        {
            purb1 = qh_from_schedule(pthis)->elem_head_link->purb;
            pc = (PURB_HS_PIPE_CONTENT) & purb1->pipe;
            if (pc->speed_high)
            {
                TRAP();
                return;
            }
            if ((1 << (pc->interval + 3)) > (LONG) interval)
            {
                TRAP();
                continue;
            }
            else if ((1 << (pc->interval + 3) < (LONG) interval))
            {
                break;
            }
            else if (elem_type(pthis, FALSE) == INIT_LIST_FLAG_FSTN)
            {
                ListNext(&ehci->periodic_list_cpu[EHCI_SCHED_INT8_INDEX], pthis, pnext);
                pprev = pthis;
                pthis = pnext;
            }
            else if (pc->start_uframe <= 1)
            {
                ListNext(&ehci->periodic_list_cpu[EHCI_SCHED_INT8_INDEX], pthis, pnext);
                pprev = pthis;
                pthis = pnext;
            }
            break;
        }
        if (pprev == NULL)
        {
            TRAP();
            return;
        }
        if (pthis == NULL)
        {
            //the last one
            InsertTailList(&ehci->periodic_list_cpu[EHCI_SCHED_INT8_INDEX],
                           &pfstn->elem_head_link->sched_link);
        }
        else
        {
            if (elem_type(pprev, FALSE) == INIT_LIST_FLAG_FSTN)
            {
                InsertHeadList(&fstn_from_schedule(pprev)->elem_head_link->sched_link,
                               &pfstn->elem_head_link->sched_link);
            }
            else
            {
                InsertHeadList(&qh_from_schedule(pprev)->elem_head_link->sched_link,
                               &pfstn->elem_head_link->sched_link);
            }
        }
        pfstn->hw_next = qh_from_schedule(pprev)->hw_next;
        qh_from_schedule(pprev)->hw_next = pfstn->phys_addr;
    }
    else
    {
        start_frame++;
        for(i = start_frame; i < (LONG) start_frame + 1; i += (interval >> 3))
        {
            list_head = &ehci->frame_list_cpu[i].td_link;
            ListFirst(list_head, pthis);

            pprev = list_head;
            while (pthis)
            {
                // skip itds and sitds
                if (elem_type(pthis, FALSE) == INIT_LIST_FLAG_ITD ||
                    elem_type(pthis, FALSE) == INIT_LIST_FLAG_SITD)
                {
                    ListNext(list_head, pthis, pnext);
                    pprev = pthis;
                    pthis = pnext;
                    continue;
                }
                break;
            }

            while (pthis)
            {
                // find the insertion point
                ULONG u;

                pqhnext = qh_from_schedule(pthis);
                if (elem_type(pthis, FALSE) == INIT_LIST_FLAG_FSTN)
                    purb1 = fstn_from_schedule(pthis)->elem_head_link->purb;
                else
                    purb1 = pqhnext->elem_head_link->purb;

                if (purb1 == NULL)
                    TRAP();

                pc = (PURB_HS_PIPE_CONTENT) & purb1->pipe;
                u = 1 << (pc->speed_high ? (1 << pc->interval) : (1 << (pc->interval + 3)));

                if (u > interval)
                {
                    ListNext(list_head, pthis, pnext);
                    pprev = pthis;
                    pthis = pnext;
                    continue;
                }
                else if (u == interval)
                {
                    if (start_uframe >=
                        (elem_type(pthis, FALSE) == INIT_LIST_FLAG_FSTN ?
                         1 : pc->start_uframe) + (purb1->int_start_frame << 3))
                    {
                        ListNext(list_head, pthis, pnext);
                        pprev = pthis;
                        pthis = pnext;
                        continue;
                    }
                    else
                        break;
                }
                else if (u < interval)
                {
                    break;
                }
            }

            if (pprev == list_head)
            {
                // insert to the list head
                pnext = pfstn->elem_head_link->sched_link.Flink = list_head->Flink;
                list_head->Flink = &pfstn->elem_head_link->sched_link;
                pfstn->hw_next = ehci->frame_list[i];   // point to following node
                ehci->frame_list[i] = pfstn->phys_addr;
            }
            else
            {
                pnext = pfstn->elem_head_link->sched_link.Flink = pprev->Flink;
                pprev->Flink = &pfstn->elem_head_link->sched_link;

                // fstn can be handled correctly
                pfstn->hw_next = qh_from_schedule(pprev)->hw_next;
                qh_from_schedule(pprev)->hw_next = pfstn->phys_addr;
            }
        }
        // the pointer to next node of this fstn is alway same across the frame list.
        for(i = start_frame + (interval >> 3); i < (LONG) ehci->frame_count; i += (interval >> 3))
        {
            pprev = list_head = &ehci->frame_list_cpu[i].td_link;
            ListFirst(list_head, pthis);

            while (pthis)
            {
                if (pthis == pnext)
                {
                    break;
                }
                pprev = pthis;
                ListNext(list_head, pthis, pthis);
            }

            pprev->Flink = &pfstn->elem_head_link->sched_link;
            if (pprev == list_head)
                ehci->frame_list[i] = pfstn->phys_addr;
            else
                qh_from_schedule(pprev)->hw_next = pfstn->phys_addr;
        }
    }
}

static VOID
ehci_remove_fstn_from_schedule(PEHCI_DEV ehci, PURB purb)
{
    PURB_HS_PIPE_CONTENT pipe_content;
    PLIST_ENTRY pthis, list_head, pnext, pprev;
    PEHCI_FSTN pfstn;

    ULONG interval, start_frame, start_uframe;
    LONG i;

    if (ehci == NULL || purb == NULL)
        return;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    interval = (1 << (pipe_content->interval + 3));
    list_head = &purb->trasac_list;
    start_frame = purb->int_start_frame;
    start_uframe = 1;

    if ((start_frame << 3) >= interval)
        TRAP();
    start_frame++;

    ListFirstPrev(list_head, pprev);
    if (elem_type_list_entry(pprev) != INIT_LIST_FLAG_FSTN)
    {
        TRAP();
        return;
    }

    pfstn = fstn_from_list_entry(pprev);
    if (interval < 8)
    {
        TRAP();
        return;
    }
    if (interval == 8)
    {
        ListFirstPrev(&pfstn->elem_head_link->sched_link, pprev);
        qh_from_schedule(pprev)->hw_next = pfstn->hw_next;
        RemoveEntryList(&pfstn->elem_head_link->sched_link);
    }
    else
    {
        for(i = start_frame; i < (LONG) ehci->frame_count; i++)
        {
            ListFirst(&ehci->frame_list_cpu[i].td_link, pthis);
            if (pthis == NULL)
            {
                TRAP();
                return;
            }
            pprev = &ehci->frame_list_cpu[i].td_link;
            while (pthis && pthis != &pfstn->elem_head_link->sched_link)
            {
                pprev = pthis;
                ListNext(&ehci->frame_list_cpu[i].td_link, pthis, pnext);
                pthis = pnext;
            }
            if (pthis == NULL)
            {
                TRAP();
                return;
            }
            qh_from_schedule(pprev)->hw_next = pfstn->hw_next;
            pprev->Flink = pfstn->elem_head_link->sched_link.Flink;
        }
    }
    return;
}

static VOID
ehci_insert_int_schedule(PEHCI_DEV ehci, PURB purb)
{
    PURB_HS_PIPE_CONTENT pipe_content, pc;
    PLIST_ENTRY pthis, list_head, pnext = NULL, pprev;
    PEHCI_ELEM_LINKS elem_link;
    PEHCI_QH pqh, pqhprev, pqhnext;
    PURB purb1;

    ULONG interval, u, start_frame, start_uframe;
    LONG i;
    UCHAR need_fstn;

    if (ehci == NULL || purb == NULL)
        return;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    interval = REAL_INTERVAL;
    start_uframe = (purb->int_start_frame << 3) + pipe_content->start_uframe;
    start_frame = purb->int_start_frame;
    need_fstn = FALSE;
    list_head = &purb->trasac_list;

    ListFirst(list_head, pthis);
    if (pthis == NULL)
        return;

    pqh = qh_from_list_entry(pthis);

    if (!pipe_content->speed_high)
    {
        interval = (interval << 3);
        ListFirstPrev(list_head, pprev);
        if (elem_type_list_entry(pprev) == INIT_LIST_FLAG_FSTN)
            need_fstn = TRUE;
    }

    if (interval < 16)
    {
        pqhprev = pqhnext = NULL;
        if (interval == 1)
        {
            list_head = &ehci->periodic_list_cpu[EHCI_SCHED_FSTN_INDEX];
            ListFirst(list_head, pthis);
            InsertTailList(list_head, &pqh->elem_head_link->sched_link);
            ListFirstPrev(&pqh->elem_head_link->sched_link, pprev);
            pqh->hw_next = EHCI_PTR_TERM;
            if (pprev == pthis)
            {
                fstn_from_schedule(pthis)->hw_next = pqh->phys_addr;
            }
            else
            {
                qh_from_schedule(pthis)->hw_next = pqh->phys_addr;
            }
        }
        else                    // interval == 2 or 4 or 8
        {
            list_head = &ehci->periodic_list_cpu[EHCI_SCHED_INT8_INDEX];
            ListFirst(list_head, pthis);
            pprev = NULL;
            while (pthis)
            {
                elem_link = struct_ptr(pthis, EHCI_ELEM_LINKS, sched_link);
                purb1 = elem_link->purb;
                pc = (PURB_HS_PIPE_CONTENT) purb1->pipe;
                u = (pc->speed_high ? (1 << pc->interval) : (1 << (pc->interval + 3)));

                if (interval < u)
                {
                    ListFirstPrev(pthis, pprev);
                    break;
                }
                else if (interval > u)
                {
                    ListNext(list_head, pthis, pnext);
                    pprev = pthis;
                    pthis = pnext;
                    continue;
                }
                // FIXME: is this right to fix fstn's start_uf 1???
                else if (start_uframe <=
                         (elem_type(pthis, FALSE) == INIT_LIST_FLAG_FSTN ?
                          1 : pc->start_uframe) + (purb1->int_start_frame << 3))
                {
                    ListNext(list_head, pthis, pnext);
                    pprev = pthis;
                    pthis = pnext;
                    continue;
                }
                else            // interval is equal, and start_uframe is greater
                {
                    ListFirstPrev(pthis, pprev);
                    break;
                }
            }
            if (pprev == NULL)
            {
                // at least one dummy qh is there
                TRAP();
            }
            else if (pnext == NULL)
            {
                // the last one in this chain, fstn can be handled correctly
                InsertTailList(list_head, &pqh->elem_head_link->sched_link);
                pqhprev = qh_from_schedule(pprev);
                pqh->hw_next = pqhprev->hw_next;
                pqhprev->hw_next = pqh->phys_addr;
            }
            else
            {
                pqhprev = qh_from_schedule(pprev);
                if (elem_type(pprev, FALSE) == INIT_LIST_FLAG_QH)
                {
                    InsertHeadList(&pqhprev->elem_head_link->sched_link, &pqh->elem_head_link->sched_link);
                }
                else if (elem_type(pprev, FALSE) == INIT_LIST_FLAG_FSTN)
                {
                    InsertHeadList(&fstn_from_schedule(pprev)->elem_head_link->sched_link,
                                   &pqh->elem_head_link->sched_link);
                }
                pqh->hw_next = pqhprev->hw_next;
                pqhprev->hw_next = pqh->phys_addr;
            }
        }
    }
    else                        // interval >= 16
    {
        if ((start_frame << 3) >= interval)
            TRAP();

        for(i = start_frame; i < (LONG) start_frame + 1; i += (interval >> 3))
        {
            list_head = &ehci->frame_list_cpu[i].td_link;
            ListFirst(list_head, pthis);

            pprev = list_head;
            while (pthis)
            {
                // skip itds and sitds
                if (elem_type(pthis, FALSE) == INIT_LIST_FLAG_ITD ||
                    elem_type(pthis, FALSE) == INIT_LIST_FLAG_SITD)
                {
                    ListNext(list_head, pthis, pnext);
                    pprev = pthis;
                    pthis = pnext;
                    continue;
                }
                break;
            }

            while (pthis)
            {
                // find the insertion point

                pqhnext = qh_from_schedule(pthis);
                if (elem_type(pthis, FALSE) == INIT_LIST_FLAG_FSTN)
                    purb1 = fstn_from_schedule(pthis)->elem_head_link->purb;
                else
                    purb1 = pqhnext->elem_head_link->purb;

                if (purb1 == NULL)
                    TRAP();

                pc = (PURB_HS_PIPE_CONTENT) & purb1->pipe;
                u = 1 << (pc->speed_high ? (1 << pc->interval) : (1 << (pc->interval + 3)));

                if (u > interval)
                {
                    ListNext(list_head, pthis, pnext);
                    pprev = pthis;
                    pthis = pnext;
                    continue;
                }
                else if (u == interval)
                {
                    if (start_uframe >=
                        (elem_type(pthis, FALSE) == INIT_LIST_FLAG_FSTN ?
                         1 : pc->start_uframe) + (purb1->int_start_frame << 3))
                    {
                        ListNext(list_head, pthis, pnext);
                        pprev = pthis;
                        pthis = pnext;
                        continue;
                    }
                    else
                        break;
                }
                else if (u < interval)
                {
                    break;
                }
            }
            if (pprev == list_head)
            {
                // insert to the list head
                pnext = pqh->elem_head_link->sched_link.Flink = list_head->Flink;
                list_head->Flink = &pqh->elem_head_link->sched_link;
                pqh->hw_next = ehci->frame_list[i];     // point to following node
                ehci->frame_list[i] = pqh->phys_addr;
            }
            else
            {
                pnext = pqh->elem_head_link->sched_link.Flink = pprev->Flink;
                pprev->Flink = &pqh->elem_head_link->sched_link;

                // fstn can be handled correctly
                pqh->hw_next = qh_from_schedule(pprev)->hw_next;
                qh_from_schedule(pprev)->hw_next = pqh->phys_addr;
            }
        }
        for(i = start_frame + (interval >> 3); i < (LONG) ehci->frame_count; i += (interval >> 3))
        {
            pprev = list_head = &ehci->frame_list_cpu[i].td_link;
            ListFirst(list_head, pthis);

            while (pthis)
            {
                if (pthis == pnext)
                {
                    break;
                }
                pprev = pthis;
                ListNext(list_head, pthis, pthis);
            }

            pprev->Flink = &pqh->elem_head_link->sched_link;
            if (pprev == list_head)
                ehci->frame_list[i] = pqh->phys_addr;
            else
                qh_from_schedule(pprev)->hw_next = pqh->phys_addr;
        }
    }

    if (need_fstn)
        ehci_insert_fstn_schedule(ehci, purb);

    return;
}

static VOID
ehci_remove_int_from_schedule(PEHCI_DEV ehci, PURB purb)
{
    PURB_HS_PIPE_CONTENT pipe_content;
    PLIST_ENTRY pthis, list_head, pnext, pprev, pcur;
    PEHCI_QH pqh, pqhprev;

    ULONG interval, start_frame, start_uframe;
    LONG i;

    if (ehci == NULL || purb == NULL)
        return;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    interval = REAL_INTERVAL;
    start_uframe = (purb->int_start_frame << 3) + pipe_content->start_uframe;
    start_frame = purb->int_start_frame;

    ListFirst(&purb->trasac_list, pthis);
    if (pthis == NULL)
        return;

    pqh = qh_from_list_entry(pthis);
    list_head = &purb->trasac_list;

    if (IsListEmpty(list_head))
    {
        TRAP();
        return;
    }

    if (!pipe_content->speed_high)
    {
        interval = (interval << 3);
    }

    if (interval >= 256 * 8)
        return;

    if (interval < 16)
    {
        ListFirstPrev(&pqh->elem_head_link->sched_link, pprev);
        RemoveEntryList(&pqh->elem_head_link->sched_link);
        if (interval > 1)
        {
            pqhprev = qh_from_schedule(pprev);
            pqhprev->hw_next = pqh->hw_next;
        }
        else
        {
            ListFirst(&ehci->frame_list_cpu[EHCI_SCHED_FSTN_INDEX].td_link, list_head);
            if (elem_type(pprev, FALSE) == INIT_LIST_FLAG_FSTN)
            {
                fstn_from_schedule(list_head)->hw_next = pqh->hw_next;
            }
            else
            {
                qh_from_schedule(pprev)->hw_next = pqh->hw_next;
            }
        }
    }
    else if (interval >= 16)
    {
        ListFirst(list_head, pthis);
        pthis = &pqh->elem_head_link->sched_link;

        for(i = start_uframe; i < (LONG) (ehci->frame_count << 3); i += interval)
        {
            ListFirst(&ehci->frame_list_cpu[i].td_link, pcur);
            pprev = NULL;
            while (pthis != pcur && pcur)
            {
                ListNext(&ehci->frame_list_cpu[i].td_link, pcur, pnext);
                pprev = pcur;
                pcur = pnext;
            }

            if (pcur == NULL)
            {
                TRAP();
                continue;
            }
            else if (pprev == NULL)
            {
                // the first one in the frame list
                ehci->frame_list_cpu[i].td_link.Flink = pthis->Flink;
                ehci->frame_list[i] = qh_from_schedule(pthis)->hw_next;
            }
            else
            {
                if (elem_type(pprev, FALSE) == INIT_LIST_FLAG_QH)
                {
                    qh_from_schedule(pprev)->elem_head_link->sched_link.Flink =
                        pqh->elem_head_link->sched_link.Flink;
                    qh_from_schedule(pprev)->hw_next = pqh->hw_next;
                }
                else if (elem_type(pprev, FALSE) == INIT_LIST_FLAG_ITD)
                {
                    itd_from_schedule(pprev)->elem_head_link->sched_link.Flink =
                        pqh->elem_head_link->sched_link.Flink;
                    itd_from_schedule(pprev)->hw_next = pqh->hw_next;
                }
                else if (elem_type(pprev, FALSE) == INIT_LIST_FLAG_SITD)
                {
                    sitd_from_schedule(pprev)->elem_head_link->sched_link.Flink =
                        pqh->elem_head_link->sched_link.Flink;
                    sitd_from_schedule(pprev)->hw_next = pqh->hw_next;
                }
                else if (elem_type(pprev, FALSE) == INIT_LIST_FLAG_FSTN)
                {
                    fstn_from_schedule(pprev)->elem_head_link->sched_link.Flink =
                        pqh->elem_head_link->sched_link.Flink;
                    fstn_from_schedule(pprev)->hw_next = pqh->hw_next;
                }
                else
                    TRAP();
            }
        }
    }

    ListFirstPrev(&purb->trasac_list, pprev);
    if (elem_type_list_entry(pprev) == INIT_LIST_FLAG_FSTN)
        ehci_remove_fstn_from_schedule(ehci, purb);
    return;
}

static VOID
ehci_insert_iso_schedule(PEHCI_DEV ehci, PURB purb)
{
    PURB_HS_PIPE_CONTENT pipe_content;
    PLIST_ENTRY pthis, list_head, pnext;

    ULONG interval, start_frame;
    LONG i;

    if (ehci == NULL || purb == NULL)
        return;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;

    interval = 8;
    if (pipe_content->speed_high)
        interval = REAL_INTERVAL;

    start_frame = purb->iso_start_frame;

    ListFirst(&purb->trasac_list, pthis);
    if (pthis == NULL)
    {
        TRAP();
        return;
    }

    list_head = &purb->trasac_list;
    if (IsListEmpty(list_head))
    {
        TRAP();
        return;
    }

    i = start_frame;
    while (pthis)
    {
        if (pipe_content->speed_high)
        {
            itd_from_list_entry(pthis)->elem_head_link->sched_link.Flink =
                ehci->frame_list_cpu[i].td_link.Flink;
            itd_from_list_entry(pthis)->hw_next = ehci->frame_list[i];

            ehci->frame_list[i] = itd_from_list_entry(pthis)->phys_addr;
            ehci->frame_list_cpu[i].td_link.Flink = pthis;
        }
        else
        {
            sitd_from_list_entry(pthis)->elem_head_link->sched_link.Flink =
                ehci->frame_list_cpu[i].td_link.Flink;
            sitd_from_list_entry(pthis)->hw_next = ehci->frame_list[i];

            ehci->frame_list[i] = sitd_from_list_entry(pthis)->phys_addr;
            ehci->frame_list_cpu[i].td_link.Flink = pthis;
        }

        ListNext(list_head, pthis, pnext);
        pthis = pnext;

        if (interval <= 8)
            i++;
        else
            i += (interval >> 3);
    }
    return;
}

static VOID
ehci_remove_iso_from_schedule(PEHCI_DEV ehci, PURB purb)
{
    PURB_HS_PIPE_CONTENT pipe_content;
    PLIST_ENTRY pthis, list_head, pnext, pprev, pcur;

    ULONG interval, start_frame;
    LONG i;

    if (ehci == NULL || purb == NULL)
        return;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;

    interval = 8;
    if (pipe_content->speed_high)
        interval = REAL_INTERVAL;

    start_frame = purb->iso_start_frame;

    ListFirst(&purb->trasac_list, pthis);
    if (pthis == NULL)
    {
        TRAP();
        return;
    }

    list_head = &purb->trasac_list;
    if (IsListEmpty(list_head))
    {
        TRAP();
        return;
    }

    i = start_frame;
    while (pthis)
    {
        // for the possible existance of sitd back pointer, we can not use for(...)
        ListFirst(&ehci->frame_list_cpu[i].td_link, pcur);
        pprev = &ehci->frame_list_cpu[i].td_link;
        while (pcur)
        {
            if (pcur != pthis)
            {
                ListNext(&ehci->frame_list_cpu[i].td_link, pcur, pnext);
                pprev = pcur;
                pcur = pnext;
                continue;
            }
            break;
        }

        if (pcur == NULL)
        {
            TRAP();
        }
        pprev->Flink = pcur->Flink;
        if (pprev != &ehci->frame_list_cpu[i].td_link)
            qh_from_schedule(pprev)->hw_next = qh_from_schedule(pcur)->hw_next;
        else
            ehci->frame_list[i] = qh_from_schedule(pcur)->hw_next;

        ListNext(list_head, pthis, pnext);
        pthis = pnext;

        if (interval <= 8)
            i++;
        else
            i += (interval >> 3);
    }
    return;
}

NTSTATUS
ehci_isr_removing_urb(PEHCI_DEV ehci, PURB purb, BOOLEAN doorbell_rings, ULONG cur_frame)
{
    UCHAR type;
    PLIST_ENTRY pthis;
    PURB_HS_PIPE_CONTENT pipe_content;
    PEHCI_ITD_CONTENT pitd_content;
    PEHCI_SITD_CONTENT psitd_content;
    PEHCI_QH_CONTENT pqh_content;
    PEHCI_QTD_CONTENT pqtd_content;
    LONG i;

    if (purb == NULL || ehci == NULL)
        return STATUS_INVALID_PARAMETER;

    if ((purb->flags & URB_FLAG_STATE_MASK) == URB_FLAG_STATE_FINISHED)
        return STATUS_SUCCESS;

    type = 0;
    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;

    switch (purb->flags & URB_FLAG_STATE_MASK)
    {
        case URB_FLAG_STATE_IN_PROCESS:
        {
            // determine the removal type: complete, error or cancel
            ListFirst(&purb->trasac_list, pthis);
            if (purb->flags & URB_FLAG_FORCE_CANCEL)
            {
                type = 3;
            }
            else
            {
                if (pipe_content->trans_type == USB_ENDPOINT_XFER_BULK ||
                    pipe_content->trans_type == USB_ENDPOINT_XFER_INT ||
                    pipe_content->trans_type == USB_ENDPOINT_XFER_CONTROL)
                {
                    pqh_content =
                        (PEHCI_QH_CONTENT) ((ULONG) struct_ptr(pthis, EHCI_ELEM_LINKS, elem_link)->
                                            phys_part & PHYS_PART_ADDR_MASK);
                    if (EHCI_QH_ERROR(pqh_content))
                    {
                        purb->status = pqh_content->cur_qtd.status;
                        type = 2;
                    }
                    else
                    {
                        pqtd_content = &pqh_content->cur_qtd;
                        if (pqtd_content->terminal && ((pqtd_content->status & QTD_STS_ACTIVE) == 0))
                        {
                            type = 1;
                        }
                        // else, not finished
                    }
                }
                else if (pipe_content->trans_type == USB_ENDPOINT_XFER_ISOC)
                {
                    // FIXME: do we need to check if current frame falls out of the
                    // frame range of iso transfer
                    // inspect the last td to determine if finished
                    ListFirstPrev(&purb->trasac_list, pthis);
                    if (pthis)
                    {
                        if (pipe_content->speed_high)
                        {
                            pitd_content =
                                (PEHCI_ITD_CONTENT) ((ULONG) struct_ptr(pthis, EHCI_ELEM_LINKS, elem_link)->
                                                     phys_part & PHYS_PART_ADDR_MASK);
                            for(i = 0; i < 8; i++)
                            {
                                if (pitd_content->status_slot[i].trans_length &&
                                    pitd_content->status_slot[i].status & 0x08)
                                {
                                    break;
                                }
                            }
                            if (i == 8)
                            {
                                // the itds are all inactive
                                type = 1;
                            }
                        }
                        else
                        {
                            psitd_content =
                                (PEHCI_SITD_CONTENT) ((ULONG) struct_ptr(pthis, EHCI_ELEM_LINKS, elem_link)->
                                                      phys_part & PHYS_PART_ADDR_MASK);
                            if ((psitd_content->status & 0x80) == 0)
                            {
                                type = 1;
                            }
                        }
                    }
                    else        // empty transaction list in purb
                        TRAP();
                }
                else            // unknown transfer type
                    TRAP();

            }                   // end of not force cancel

            if (type == 0)
                return STATUS_SUCCESS;

            switch (type)
            {
                case 1:
                {
                    if (pipe_content->trans_type == USB_ENDPOINT_XFER_CONTROL ||
                        pipe_content->trans_type == USB_ENDPOINT_XFER_BULK)
                    {
                        ehci_remove_bulk_from_schedule(ehci, purb);
                        purb->flags &= ~URB_FLAG_STATE_MASK;
                        purb->flags |= URB_FLAG_STATE_DOORBELL;
                        purb->status = 0;
                        press_doorbell(ehci);
                        return STATUS_SUCCESS;
                    }
                    else if (pipe_content->trans_type == USB_ENDPOINT_XFER_ISOC)
                    {
                        ehci_remove_iso_from_schedule(ehci, purb);
                    }
                    else if (pipe_content->trans_type == USB_ENDPOINT_XFER_INT)
                    {
                        ehci_remove_int_from_schedule(ehci, purb);
                    }
                    else        // unknown transfer type
                        TRAP();

                    purb->flags &= ~URB_FLAG_STATE_MASK;
                    purb->flags |= URB_FLAG_STATE_FINISHED;

                    // notify dpc the purb can be completed;
                    purb->flags &= ~URB_FLAG_IN_SCHEDULE;
                    purb->status = 0;

                    return STATUS_SUCCESS;
                }
                case 2:
                {
                    if (pipe_content->trans_type == USB_ENDPOINT_XFER_CONTROL ||
                        pipe_content->trans_type == USB_ENDPOINT_XFER_BULK)
                    {
                        ehci_deactivate_urb(purb);
                        ehci_remove_bulk_from_schedule(ehci, purb);
                        purb->flags &= ~URB_FLAG_STATE_MASK;
                        purb->flags |= URB_FLAG_STATE_DOORBELL;
                        press_doorbell(ehci);
                    }
                    else if (pipe_content->trans_type == USB_ENDPOINT_XFER_INT)
                    {
                        ehci_remove_int_from_schedule(ehci, purb);

                        purb->flags &= ~URB_FLAG_STATE_MASK;
                        purb->flags |= URB_FLAG_STATE_FINISHED;
                        purb->flags &= ~URB_FLAG_IN_SCHEDULE;
                    }
                    else        // unknown transfer or iso transfer
                        TRAP();
                    return STATUS_SUCCESS;
                }
                case 3:
                {
                    if (pipe_content->trans_type == USB_ENDPOINT_XFER_CONTROL ||
                        pipe_content->trans_type == USB_ENDPOINT_XFER_BULK ||
                        pipe_content->trans_type == USB_ENDPOINT_XFER_INT)
                    {
                        ehci_deactivate_urb(purb);
                        if (pipe_content->trans_type == USB_ENDPOINT_XFER_BULK ||
                            pipe_content->trans_type == USB_ENDPOINT_XFER_CONTROL)
                            ehci_remove_bulk_from_schedule(ehci, purb);
                        else
                            ehci_remove_int_from_schedule(ehci, purb);

                        purb->flags &= ~URB_FLAG_STATE_MASK;
                        purb->flags |= URB_FLAG_STATE_DOORBELL;

                        press_doorbell(ehci);

                    }
                    else        // unknown transfer or iso transfer
                        DO_NOTHING;
                    purb->status = 0;
                    return STATUS_SUCCESS;
                }
                default:
                    TRAP();
            }
        }
        case URB_FLAG_STATE_DOORBELL:
        {
            if (doorbell_rings == FALSE)
                return STATUS_SUCCESS;

            purb->flags &= ~URB_FLAG_STATE_MASK;
            purb->flags |= URB_FLAG_STATE_FINISHED;
            purb->flags &= ~URB_FLAG_IN_SCHEDULE;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_SUCCESS;
}

static ULONG
ehci_scan_iso_error(PEHCI_DEV ehci, PURB purb)
// we only report the first error of the ITDs, purb->status is the status code
// return the raw status for ehci_set_error_code
{
    PURB_HS_PIPE_CONTENT pipe_content;
    PEHCI_SITD_CONTENT psitd_content;
    PEHCI_ITD_CONTENT pitd_content;
    PLIST_ENTRY pthis, pnext;
    LONG i;

    if (ehci == NULL || purb == NULL)
        return 0;

    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->pipe;
    if (pipe_content->trans_type != USB_ENDPOINT_XFER_ISOC)
    {
        return 0;
    }

    ListFirst(&purb->trasac_list, pthis);
    if (pipe_content->speed_high)
    {
        while (pthis)
        {
            pitd_content = (PEHCI_ITD_CONTENT) itd_from_list_entry(pthis);
            for(i = 0; i < 8; i++)
            {
                if (pitd_content->status_slot[i].status & ITD_ANY_ERROR)
                    break;
            }
            if (i < 8)
            {
                // error occured
                return purb->status = pitd_content->status_slot[i].status;
            }
            ListNext(&purb->trasac_list, pthis, pnext);
            pthis = pnext;
        }
    }
    else
    {
        while (pthis)
        {
            psitd_content = (PEHCI_SITD_CONTENT) sitd_from_list_entry(pthis);
            if (psitd_content->status & SITD_ANY_ERROR)
            {
                // error occured
                if (psitd_content->s_mask == 0x04 &&
                    psitd_content->c_mask == 0x70 && psitd_content->bytes_to_transfer == 1)
                    return purb->status = 0;

                return purb->status = psitd_content->status;
            }
            ListNext(&purb->trasac_list, pthis, pnext);
            pthis = pnext;
        }
    }
    return 0;
}

BOOLEAN NTAPI
ehci_isr(PKINTERRUPT interrupt, PVOID context)
    // we can not use endp here for it is within the dev scope, and
    // we can not acquire the dev-lock, fortunately we saved some
    // info in purb->pipe in ehci_internal_submit_XXX.
{

    PEHCI_DEV ehci;
    ULONG status;
#ifdef DBG
    ULONG urb_count;
#endif
    PLIST_ENTRY pthis, pnext;
    PURB purb;
    BOOLEAN door_bell_rings;
    ULONG cur_frame;
    /*
     * Read the interrupt status, and write it back to clear the
     * interrupt cause
     */
    ehci = (PEHCI_DEV) context;
    if (ehci == NULL)
        return FALSE;

    status = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + EHCI_USBSTS));
    cur_frame = EHCI_READ_PORT_ULONG((PULONG) (ehci->port_base + EHCI_FRINDEX));

    status &= (EHCI_ERROR_INT | STS_INT | STS_IAA);
    if (!status)                /* shared interrupt, not mine */
    {
        ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_isr():  not our int\n"));
        return FALSE;
    }

    /* clear it */
    EHCI_WRITE_PORT_ULONG((PULONG) (ehci->port_base + EHCI_USBSTS), status);

    if (status & EHCI_ERROR_INT)
    {
        ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_isr():  current ehci status=0x%x\n", status));
    }
    else
    {
        ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_isr():  congratulations, no error occurs\n"));
    }


    if (status & STS_FATAL)
    {
        DbgPrint("ehci_isr(): host system error, PCI problems?\n");
        for(;;);

    }

    if (status & STS_HALT)      //&& !ehci->is_suspended
    {
        DbgPrint("ehci_isr(): host controller halted. very bad\n");
        /* FIXME: Reset the controller, fix the offending TD */
        // reset is performed in dpc
    }


    door_bell_rings = ((status & STS_IAA) != 0);

    // scan to remove those due
#ifdef DBG
    urb_count = dbg_count_list(&ehci->urb_list);
    ehci_dbg_print(DBGLVL_MAXIMUM, ("ehci_isr(): urb# in process is %d\n", urb_count));
#endif

    ListFirst(&ehci->urb_list, pthis);
    while (pthis)
    {
        purb = (PURB) pthis;
        ehci_isr_removing_urb(ehci, purb, door_bell_rings, cur_frame);
        ListNext(&ehci->urb_list, pthis, pnext);
        pthis = pnext;
    }

    KeInsertQueueDpc(&ehci->pdev_ext->ehci_dpc, (PVOID) status, 0);
    return TRUE;
}

#ifndef INCLUDE_EHCI
VOID
ehci_unload(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT pdev;
    PEHCI_DEVICE_EXTENSION pdev_ext;
    PUSB_DEV_MANAGER dev_mgr;
    LONG i;

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

    dev_mgr_release_hcd(dev_mgr);
    return;
}

NTSTATUS
generic_dispatch_irp(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    PDEVEXT_HEADER dev_ext;

    dev_ext = (PDEVEXT_HEADER) dev_obj->DeviceExtension;

    if (dev_ext && dev_ext->dispatch)
        return dev_ext->dispatch(dev_obj, irp);

    irp->IoStatus.Information = 0;

    EXIT_DISPATCH(STATUS_UNSUCCESSFUL, irp);
}

VOID
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
        IoSetCancelRoutine(irp, NULL);
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN fRes;

#if DBG
    // should be done before any debug output is done.
    // read our debug verbosity level from the registry
    //NetacOD_GetRegistryDword( NetacOD_REGISTRY_PARAMETERS_PATH, //absolute registry path
    //                           L"DebugLevel",     // REG_DWORD ValueName
    //                           &gDebugLevel );    // Value receiver

    // debug_level = DBGLVL_MAXIMUM;
#endif

    ehci_dbg_print_cond(DBGLVL_MINIMUM, DEBUG_UHCI,
                        ("Entering DriverEntry(), RegistryPath=\n    %ws\n", RegistryPath->Buffer));

    // Remember our driver object, for when we create our child PDO
    usb_driver_obj = DriverObject;

    //
    // Create dispatch points for create, close, unload
    DriverObject->MajorFunction[IRP_MJ_CREATE] = generic_dispatch_irp;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = generic_dispatch_irp;
    DriverObject->DriverUnload = ehci_unload;

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

    ehci_probe(DriverObject, RegistryPath, &g_dev_mgr);

    if (dev_mgr_strobe(&g_dev_mgr) == FALSE)
    {
        dev_mgr_release_hcd(&g_dev_mgr);
        return STATUS_UNSUCCESSFUL;
    }

    dev_mgr_start_hcd(&g_dev_mgr);
    ehci_dbg_print_cond(DBGLVL_DEFAULT, DEBUG_UHCI, ("DriverEntry(): exiting... (%x)\n", ntStatus));
    return STATUS_SUCCESS;
}
#endif

//note: the initialization will be in the following order
//      dev_mgr_strobe
//      ehci_start

//      to  kill dev_mgr_thread:
//      dev_mgr->term_flag = TRUE;
//      KeSetEvent( &dev_mgr->wake_up_event );
//      this piece of code must run at passive-level
//
//
