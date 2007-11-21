/*
 * Copyright (c) 2001-2002 by David Brownell
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __EHCI_H__
#define __EHCI_H__

#define EHCI_QH_SIZE				sizeof( EHCI_QH )
#define EHCI_ITD_SIZE				sizeof( EHCI_ITD )
#define EHCI_SITD_SIZE				sizeof( EHCI_SITD )
#define EHCI_FSTN_SIZE				sizeof( EHCI_FSTN )
#define EHCI_QTD_SIZE				sizeof( EHCI_QTD )

#define INIT_LIST_FLAG_TYPE			0x0f
#define INIT_LIST_FLAG_ITD			0x00
#define INIT_LIST_FLAG_QH			0x01
#define INIT_LIST_FLAG_SITD			0x02
#define INIT_LIST_FLAG_FSTN			0x03
#define INIT_LIST_FLAG_QTD			0x04

#define EHCI_MAX_SIZE_TRANSFER		0x100000 	// 1 MB per transfer
#define EHCI_MAX_ELEMS_POOL			1024
#define EHCI_MAX_LISTS_POOL			0x08
#define EHCI_MAX_QHS_LIST			256			// 4 pages
#define EHCI_MAX_ITDS_LIST			( PAGE_SIZE / EHCI_ITD_SIZE * 2 ) 	// 2 pages
#define EHCI_MAX_SITDS_LIST			( PAGE_SIZE / EHCI_SITD_SIZE )// 1 page
#define EHCI_MAX_FSTNS_LIST			( PAGE_SIZE / EHCI_FSTN_SIZE )// 1 page
#define EHCI_MAX_QTDS_LIST			256			// 4 pages

#define EHCI_PTR_TERM       		0x01
#define EHCI_NAK_RL_COUNT			0x04

#define EHCI_QTD_MAX_TRANS_SIZE		20480
#define PHYS_PART_TYPE_MASK			0x01f
#define PHYS_PART_ADDR_MASK			( ~PHYS_PART_TYPE_MASK )

typedef struct _EHCI_ELEM_LINKS
{
	LIST_ENTRY 			elem_link;		// link for one urb request
	LIST_ENTRY			sched_link;		// to shadow link of schedule
	struct _EHCI_ELEM_POOL*  	pool_link;
	struct _EHCI_ELEM_LIST*		list_link;		// opaque to client, which list this td belongs to
	PVOID				phys_part;		// point to EHCI_XXX, the lower 5 bits is used to indicate the type of the element
	PURB				purb;

} EHCI_ELEM_LINKS, *PEHCI_ELEM_LINKS;

typedef struct _INIT_ELEM_LIST_CONTEXT
{
	PADAPTER_OBJECT padapter;
	struct _EHCI_ELEM_POOL *pool;

} INIT_ELEM_LIST_CONTEXT, *PINIT_ELEM_LIST_CONTEXT;

typedef VOID ( *PDESTROY_ELEM_LIST )( struct _EHCI_ELEM_LIST* plist );
typedef PLIST_ENTRY ( *PGET_LIST_HEAD )( struct _EHCI_ELEM_LIST* plist );
typedef LONG ( *PGET_TOTAL_COUNT )( struct _EHCI_ELEM_LIST* plist );
typedef LONG ( *PGET_ELEM_SIZE )( struct _EHCI_ELEM_LIST* plist );
typedef LONG ( *PGET_LINK_OFFSET )( struct _EHCI_ELEM_LIST* plist );
typedef LONG ( *PADD_REF )( struct _EHCI_ELEM_LIST* plist );
typedef LONG ( *PRELEASE_REF )( struct _EHCI_ELEM_LIST* plist );
typedef LONG ( *PGET_REF )( struct _EHCI_ELEM_LIST* plist );

typedef struct _EHCI_ELEM_LIST
{
	PDESTROY_ELEM_LIST	destroy_list;
	PGET_LIST_HEAD		get_list_head;
	PGET_TOTAL_COUNT	get_total_count;
	PGET_ELEM_SIZE		get_elem_size;
	PGET_LINK_OFFSET	get_link_offset;
	PADD_REF			add_ref;
	PRELEASE_REF		release_ref;
	PGET_REF			get_ref;

	// private area
	LONG				flags;			// contails element's type info
	LONG				total_count;
	LONG				elem_size;
	LIST_HEAD			free_list;		// chains of all the elements
	struct _EHCI_ELEM_POOL		*parent_pool;
	LONG				reference;

	// used to represent the physical memory
	PPHYSICAL_ADDRESS   phys_addrs;		// array of non-contigous memory
	PBYTE				*phys_bufs;
	PEHCI_ELEM_LINKS	elem_head_buf;
	PADAPTER_OBJECT 	padapter;

} EHCI_ELEM_LIST, *PEHCI_ELEM_LIST;

BOOLEAN elem_pool_init_pool( struct _EHCI_ELEM_POOL* pool, LONG flags, PVOID context);
VOID elem_pool_destroy_pool ( struct _EHCI_ELEM_POOL* pool );
PEHCI_ELEM_LINKS elem_pool_alloc_elem ( struct _EHCI_ELEM_POOL* pool );
VOID elem_pool_free_elem ( PEHCI_ELEM_LINKS elem_link );
LONG elem_pool_get_total_count ( struct _EHCI_ELEM_POOL* pool );
BOOLEAN elem_pool_is_empty ( struct _EHCI_ELEM_POOL* pool );
LONG elem_pool_get_free_count ( struct _EHCI_ELEM_POOL* pool );
LONG elem_pool_get_link_offset ( struct _EHCI_ELEM_POOL* pool );
PEHCI_ELEM_LINKS elem_pool_alloc_elems ( struct _EHCI_ELEM_POOL* pool, LONG count );
BOOLEAN elem_pool_free_elems( PEHCI_ELEM_LINKS elem_chains );
LONG elem_pool_get_type( struct _EHCI_ELEM_POOL* pool );

// the following are private functions
BOOLEAN elem_pool_expand_pool( struct _EHCI_ELEM_POOL* pool, LONG elem_count );
BOOLEAN elem_pool_collect_garbage( struct _EHCI_ELEM_POOL* pool );

// lock operations
BOOLEAN elem_pool_lock( struct _EHCI_ELEM_POOL* pool, BOOLEAN at_dpc );
BOOLEAN elem_pool_unlock( struct _EHCI_ELEM_POOL* pool, BOOLEAN at_dpc );

// helper
LONG get_elem_phys_part_size( ULONG type );

typedef struct _EHCI_ELEM_POOL
{
	LONG 				flags;
	LONG				free_count;		// free count of all the lists currently allocated
	LONG				link_offset;	// a cache for elem_list->get_link_offset
	LONG				list_count;		// lists currently allocated
	PEHCI_ELEM_LIST		elem_lists[ EHCI_MAX_LISTS_POOL ];

} EHCI_ELEM_POOL, *PEHCI_ELEM_POOL;

/*-------------------------------------------------------------------------*/

#define	QTD_NEXT(dma)	cpu_to_le32((u32)dma)

/*
 * EHCI Specification 0.95 Section 3.5
 * QTD: describe data transfer components (buffer, direction, ...)
 * See Fig 3-6 "Queue Element Transfer Descriptor Block Diagram".
 *
 * These are associated only with "QH" (Queue Head) structures,
 * used with control, bulk, and interrupt transfers.
 */
#define	QTD_TOGGLE	(1 << 31)	/* data toggle */
#define	QTD_LENGTH(tok)	(((tok)>>16) & 0x7fff)
#define	QTD_IOC		(1 << 15)	/* interrupt on complete */
#define	QTD_CERR(tok)	(((tok)>>10) & 0x3)
#define	QTD_PID(tok)	(((tok)>>8) & 0x3)
#define	QTD_STS_ACTIVE	(1 << 7)	/* HC may execute this */
#define	QTD_STS_HALT	(1 << 6)	/* halted on error */
#define	QTD_STS_DBE	(1 << 5)	/* data buffer error (in HC) */
#define	QTD_STS_BABBLE	(1 << 4)	/* device was babbling (qtd halted) */
#define	QTD_STS_XACT	(1 << 3)	/* device gave illegal response */
#define	QTD_STS_MMF	(1 << 2)	/* incomplete split transaction */
#define	QTD_STS_STS	(1 << 1)	/* split transaction state */
#define	QTD_STS_PING	(1 << 0)	/* issue PING? */
#define QTD_ANY_ERROR ( QTD_STS_HALT | QTD_STS_DBE | QTD_STS_XACT | QTD_STS_MMF | QTD_STS_BABBLE )

#define QTD_PID_SETUP 	0x02
#define QTD_PID_IN		0x01
#define QTD_PID_OUT 	0x00

#pragma pack( push, usb_align, 1 )

typedef struct _EHCI_QTD_CONTENT
{
	// DWORD 0
	ULONG terminal : 1;
	ULONG reserved : 4;
	ULONG next_qtd : 27;

	// DWORD 1
	ULONG alt_terminal : 1;
	ULONG reserved1 : 4;
	ULONG alt_qtd : 27;

	// DWORD 2
	ULONG status : 8;
	ULONG pid : 2;
	ULONG err_count : 2;
	ULONG cur_page : 3;
	ULONG ioc : 1;
	ULONG bytes_to_transfer : 15;
	ULONG data_toggle : 1;

	// DWORD 3
	ULONG cur_offset : 12;
	ULONG page0 : 20;

	// DWORD 4-
	ULONG pages[ 4 ];

} EHCI_QTD_CONTENT, *PEHCI_QTD_CONTENT;

typedef struct _EHCI_QTD
{
	/* first part defined by EHCI spec */
	ULONG			hw_next;		  	/* see EHCI 3.5.1 */
	ULONG			hw_alt_next;	  	/* see EHCI 3.5.2 */
	ULONG			hw_token;		  	/* see EHCI 3.5.3 */
	ULONG			hw_buf [5];		  	/* see EHCI 3.5.4 */

	/* the rest is HCD-private */
	PEHCI_ELEM_LINKS	elem_head_link;
	ULONG			phys_addr;		/* qtd address */
	USHORT			bytes_to_transfer; /* the bytes_to_transfer in hw_token will drop to zero when completed*/
	USHORT			reserved2;
	ULONG			reserved[ 5 ];

} EHCI_QTD, *PEHCI_QTD;           //__attribute__ ((aligned (32)));

#define QTD_MASK cpu_to_le32 (~0x1f)	/* mask NakCnt+T in qh->hw_alt_next */

/*-------------------------------------------------------------------------*/

/* type tag from {qh,itd,sitd,fstn}->hw_next */
#define Q_NEXT_TYPE(dma) ((dma) & __constant_cpu_to_le32 (3 << 1))

/* values for that type tag */
#define Q_TYPE_ITD	(0 << 1)
#define Q_TYPE_QH	(1 << 1)
#define Q_TYPE_SITD (2 << 1)
#define Q_TYPE_FSTN (3 << 1)

/* next async queue entry, or pointer to interrupt/periodic QH */
#define	QH_NEXT(dma)	(cpu_to_le32(((u32)dma)&~0x01f)|Q_TYPE_QH)

/* for periodic/async schedules and qtd lists, mark end of list */
#define	EHCI_LIST_END	__constant_cpu_to_le32(1) /* "null pointer" to hw */

/*-------------------------------------------------------------------------*/

/*
 * EHCI Specification 0.95 Section 3.6
 * QH: describes control/bulk/interrupt endpoints
 * See Fig 3-7 "Queue Head Structure Layout".
 *
 * These appear in both the async and (for interrupt) periodic schedules.
 */

#define	QH_HEAD		0x00008000
#define	QH_STATE_LINKED		1			/* HC sees this */
#define	QH_STATE_UNLINK		2			/* HC may still see this */
#define	QH_STATE_IDLE		3			/* HC doesn't see this */
#define	QH_STATE_UNLINK_WAIT	4		/* LINKED and on reclaim q */
#define	QH_STATE_COMPLETING	5			/* don't touch token.HALT */
#define NO_FRAME ((unsigned short)~0)	/* pick new start */

typedef struct _EHCI_QH_CONTENT
{
	// DWORD 0
	ULONG terminal : 1;
	ULONG ptr_type : 2;
	ULONG reserved : 2;
	ULONG next_link : 27;

	// DWORD 1
	ULONG dev_addr : 7;
	ULONG inactive : 1;
	ULONG endp_addr : 4;
	ULONG endp_spd : 2;
	ULONG data_toggle : 1;
	ULONG is_async_head	: 1;
	ULONG max_packet_size : 11;
	ULONG is_ctrl_endp : 1;
	ULONG reload_counter : 4;

	// DWORD 2
    ULONG s_mask : 8;
	ULONG c_mask : 8;
	ULONG hub_addr : 7;
	ULONG port_idx : 7;
	ULONG mult : 2;

	// DWORD 3
	ULONG cur_qtd_ptr;

	// overlay
	EHCI_QTD_CONTENT cur_qtd;

} EHCI_QH_CONTENT, *PEHCI_QH_CONTENT;

typedef struct _EHCI_QH
{
	/* first part defined by EHCI spec */
	ULONG			hw_next;			 /* see EHCI 3.6.1 */
	ULONG			hw_info1;			 /* see EHCI 3.6.2 */
	ULONG			hw_info2;			 /* see EHCI 3.6.2 */
	ULONG			hw_current;			 /* qtd list - see EHCI 3.6.4 */

	/* qtd overlay (hardware parts of a struct ehci_qtd) */
	ULONG			hw_qtd_next;
	ULONG			hw_alt_next;
	ULONG			hw_token;
	ULONG			hw_buf [5];

	/* the rest is HCD-private */
	PEHCI_ELEM_LINKS	elem_head_link;
	ULONG		    phys_addr;			/* address of qh */
	ULONG			reserved[ 2 ];

} EHCI_QH, *PEHCI_QH;					//  __attribute__ ((aligned (32)));

/*-------------------------------------------------------------------------*/

/*
 * EHCI Specification 0.95 Section 3.3
 * Fig 3-4 "Isochronous Transaction Descriptor (iTD)"
 *
 * Schedule records for high speed iso xfers
 */

#define EHCI_ISOC_ACTIVE        (1<<31)        /* activate transfer this slot */
#define EHCI_ISOC_BUF_ERR       (1<<30)        /* Data buffer error */
#define EHCI_ISOC_BABBLE        (1<<29)        /* babble detected */
#define EHCI_ISOC_XACTERR       (1<<28)        /* XactErr - transaction error */
#define	EHCI_ITD_LENGTH(tok)	(((tok)>>16) & 0x7fff)
#define	EHCI_ITD_IOC		(1 << 15)			/* interrupt on complete */

#define ITD_STS_ACTIVE			8
#define ITD_STS_BUFERR			4
#define ITD_STS_BABBLE			2
#define ITD_STS_XACTERR			1

#define ITD_ANY_ERROR  ( ITD_STS_BUFERR	| ITD_STS_BABBLE | ITD_STS_XACTERR )

typedef struct _EHCI_ITD_STATUS_SLOT
{
	ULONG offset : 12;
	ULONG page_sel : 3;
	ULONG ioc : 1;
	ULONG trans_length : 12;
	ULONG status : 4;

} EHCI_ITD_STATUS_SLOT, *PEHCI_ITD_STATUS_SLOT;

typedef struct _EHCI_ITD_CONTENT
{
	// DWORD 0;
	ULONG terminal : 1;
	ULONG ptr_type : 2;
	ULONG reserved : 2;
	ULONG next_link : 27;
	// DWORD 1-8;
	EHCI_ITD_STATUS_SLOT status_slot[ 8 ];
	// DWORD 9;
	ULONG dev_addr : 7;
	ULONG reserved1 : 1;
	ULONG endp_num : 4;
	ULONG page0 : 20;
	// DWORD 10;
	ULONG max_packet_size : 11;
	ULONG io_dir : 1;
	ULONG page1 : 20;
	// DWORD 11;
	ULONG mult : 2;
	ULONG reserved2 : 10;
	ULONG page2 : 20;
	// DWORD 12-;
	ULONG pages[ 4 ];

} EHCI_ITD_CONTENT, *PEHCI_ITD_CONTENT;

typedef struct _EHCI_ITD {

	/* first part defined by EHCI spec */
	ULONG			hw_next;           			/* see EHCI 3.3.1 */
	ULONG			hw_transaction [8];			/* see EHCI 3.3.2 */
	ULONG			hw_bufp [7];	   			/* see EHCI 3.3.3 */

	/* the rest is EHCI-private */
	PEHCI_ELEM_LINKS elem_head_link;
	ULONG			phys_addr;	    			/* for this itd */
	ULONG 			buf_phys_addr;	    		/* buffer address */
	ULONG			reserved[ 5 ];

} EHCI_ITD, *PEHCI_ITD;                 // __attribute__ ((aligned (32)));

/*-------------------------------------------------------------------------*/

/*
 * EHCI Specification 0.95 Section 3.4
 * siTD, aka split-transaction isochronous Transfer Descriptor
 *       ... describe low/full speed iso xfers through TT in hubs
 * see Figure 3-5 "Split-transaction Isochronous Transaction Descriptor (siTD)
 */
#define SITD_STS_ACTIVE		0x80
#define SITD_STS_ERR		0x40	// error from device
#define SITD_STS_DBE		0x20	// data buffer error
#define SITD_STS_BABBLE		0x10	// data more than expected
#define SITD_STS_XACTERR	0x08	// general error on hc
#define SITD_STS_MISSFRM	0x04	// missd micro frames
#define SITD_STS_SC			0x02	// state is whether start-split( 0 ) or complete-split( 1 )
#define SITD_STS_RESERVED	0x01
#define SITD_ANY_ERROR ( SITD_STS_ERR | SITD_STS_DBE | SITD_STS_BABBLE | SITD_STS_XACTERR | SITD_STS_MISSFRM )

typedef struct _EHCI_SITD_CONTENT
{
	// DWORD 0;
	ULONG terminal : 1;
	ULONG ptr_type : 2;
	ULONG reserved : 2;
	ULONG next_link : 27;
	// DWORD 1;
	ULONG dev_addr : 7;
	ULONG reserved1 : 1;
	ULONG endp_num : 4;
	ULONG reserved2 : 4;
	ULONG hub_addr : 7;
	ULONG reserved3 : 1;
	ULONG port_idx : 7;
	ULONG io_dir : 1;
	// DWORD 2;
	ULONG s_mask : 8;
	ULONG c_mask : 8;
	ULONG reserved4 : 16;
	// DWORD 3:
	ULONG status : 8;
	ULONG c_prog_mask : 8;
	ULONG bytes_to_transfer : 10;
	ULONG reserved5 : 4;
	ULONG page_sel : 1;
	ULONG ioc : 1;
	// DWORD 4;
	ULONG cur_offset : 12;
	ULONG page0 : 20;
	// DWORD 5;
	ULONG trans_count : 3;
	ULONG trans_pos : 2;
	ULONG reserved6 : 7;
	ULONG page1 : 20;
	// DWORD 6;
	ULONG back_terminal : 1;
	ULONG reserved7 : 4;
	ULONG back_ptr : 27;

} EHCI_SITD_CONTENT, *PEHCI_SITD_CONTENT;

typedef struct _EHCI_SITD
{
	/* first part defined by EHCI spec */
	ULONG			hw_next;
	/* uses bit field macros above - see EHCI 0.95 Table 3-8 */

	ULONG			hw_fullspeed_ep;	/* see EHCI table 3-9 */
	ULONG           hw_uframe;			/* see EHCI table 3-10 */
    ULONG       	hw_tx_results1;		/* see EHCI table 3-11 */
	ULONG           hw_tx_results2;		/* see EHCI table 3-12 */
	ULONG           hw_tx_results3;		/* see EHCI table 3-12 */
    ULONG       	hw_backpointer;		/* see EHCI table 3-13 */

	/* the rest is HCD-private */
	PEHCI_ELEM_LINKS elem_head_link;
	ULONG			phys_addr;
	ULONG			buf_phys_addr;	  	/* buffer address */

	USHORT			usecs;				/* start bandwidth */
	USHORT			c_usecs;			/* completion bandwidth */
	ULONG			reserved[ 5 ];

} EHCI_SITD, *PEHCI_SITD; // __attribute__ ((aligned (32)));

/*-------------------------------------------------------------------------*/

/*
 * EHCI Specification 0.96 Section 3.7
 * Periodic Frame Span Traversal Node (FSTN)
 *
 * Manages split interrupt transactions (using TT) that span frame boundaries
 * into uframes 0/1; see 4.12.2.2.  In those uframes, a "save place" FSTN
 * makes the HC jump (back) to a QH to scan for fs/ls QH completions until
 * it hits a "restore" FSTN; then it returns to finish other uframe 0/1 work.
 */
typedef struct _EHCI_FSTN
{
	ULONG			hw_next;	/* any periodic q entry */
	ULONG			hw_prev;	/* qh or EHCI_LIST_END */

	/* the rest is HCD-private */
	PEHCI_ELEM_LINKS elem_head_link;
	ULONG	    	phys_addr;
	ULONG			reserved[ 4 ];

} EHCI_FSTN, *PEHCI_FSTN;		// __attribute__ ((aligned (32)));


/* NOTE:  urb->transfer_flags expected to not use this bit !!! */
#define EHCI_STATE_UNLINK	0x8000		/* urb being unlinked */

/*-------------------------------------------------------------------------*/

/* EHCI register interface, corresponds to EHCI Revision 0.95 specification */

/* Section 2.2 Host Controller Capability Registers */
#define HCS_DEBUG_PORT(p)	(((p)>>20)&0xf)	/* bits 23:20, debug port? */
#define HCS_INDICATOR(p)	((p)&(1 << 16))	/* true: has port indicators */
#define HCS_N_CC(p)			(((p)>>12)&0xf)	/* bits 15:12, #companion HCs */
#define HCS_N_PCC(p)		(((p)>>8)&0xf)	/* bits 11:8, ports per CC */
#define HCS_PORTROUTED(p)	((p)&(1 << 7))	/* true: port routing */
#define HCS_PPC(p)		((p)&(1 << 4))	/* true: port power control */
#define HCS_N_PORTS(p)		(((p)>>0)&0xf)	/* bits 3:0, ports on HC */

#define HCC_EXT_CAPS(p)		(((p)>>8)&0xff)	/* for pci extended caps */
#define HCC_ISOC_CACHE(p)	((p)&(1 << 7))  /* true: can cache isoc frame */
#define HCC_ISOC_THRES(p)	(((p)>>4)&0x7)  /* bits 6:4, uframes cached */
#define HCC_CANPARK(p)		((p)&(1 << 2))  /* true: can park on async qh */
#define HCC_PGM_FRAMELISTLEN(p) ((p)&(1 << 1))  /* true: periodic_size changes*/
#define HCC_64BIT_ADDR(p)	((p)&(1))       /* true: can use 64-bit addr */

/* 23:16 is r/w intr rate, in microframes; default "8" == 1/msec */
#define CMD_PARK	(1<<11)		/* enable "park" on async qh */
#define CMD_PARK_CNT(c)	(((c)>>8)&3)	/* how many transfers to park for */
#define CMD_LRESET	(1<<7)		/* partial reset (no ports, etc) */
#define CMD_IAAD	(1<<6)		/* "doorbell" interrupt async advance */
#define CMD_ASE		(1<<5)		/* async schedule enable */
#define CMD_PSE  	(1<<4)		/* periodic schedule enable */
/* 3:2 is periodic frame list size */
#define CMD_RESET	(1<<1)		/* reset HC not bus */
#define CMD_RUN		(1<<0)		/* start/stop HC */

/* these STS_* flags are also intr_enable bits (USBINTR) */
#define STS_IAA		(1<<5)		/* Interrupted on async advance */
#define STS_FATAL	(1<<4)		/* such as some PCI access errors */
#define STS_FLR		(1<<3)		/* frame list rolled over */
#define STS_PCD		(1<<2)		/* port change detect */
#define STS_ERR		(1<<1)		/* "error" completion (overflow, ...) */
#define STS_INT		(1<<0)		/* "normal" completion (short, ...) */

#define STS_ASS		(1<<15)		/* Async Schedule Status */
#define STS_PSS		(1<<14)		/* Periodic Schedule Status */
#define STS_RECL	(1<<13)		/* Reclamation */
#define STS_HALT	(1<<12)		/* Not running (any reason) */

/* 31:23 reserved */
#define PORT_WKOC_E	(1<<22)		/* wake on overcurrent (enable) */
#define PORT_WKDISC_E	(1<<21)		/* wake on disconnect (enable) */
#define PORT_WKCONN_E	(1<<20)		/* wake on connect (enable) */
/* 19:16 for port testing */
/* 15:14 for using port indicator leds (if HCS_INDICATOR allows) */
#define PORT_OWNER	(1<<13)		/* true: companion hc owns this port */
#define PORT_POWER	(1<<12)		/* true: has power (see PPC) */
#define PORT_USB11(x) (((x)&(3<<10))==(1<<10))	/* USB 1.1 device */
/* 11:10 for detecting lowspeed devices (reset vs release ownership) */
/* 9 reserved */
#define PORT_PR		(1<<8)			/* reset port */
#define PORT_SUSP	(1<<7)		/* suspend port */
#define PORT_RESUME	(1<<6)			/* resume it */
#define PORT_OCC	(1<<5)			/* over current change */
#define PORT_OC		(1<<4)			/* over current active */
#define PORT_PEC	(1<<3)			/* port enable change */
#define PORT_PE		(1<<2)			/* port enable */
#define PORT_CSC	(1<<1)			/* connect status change */
#define PORT_CCS	(1<<0)		/* device connected */

#define FLAG_CF		(1<<0)			/* true: we'll support "high speed" */

typedef struct _EHCI_HCS_CONTENT
{
	ULONG port_count : 4;
	ULONG port_power_control : 1;
	ULONG reserved : 2;
	ULONG port_rout_rules : 1;
	ULONG port_per_chc : 4;
	ULONG chc_count : 4;
	ULONG port_indicator : 1;
	ULONG reserved2 : 3;
	ULONG dbg_port_num : 4;
	ULONG reserved3 : 8;

} EHCI_HCS_CONTENT, *PEHCI_HCS_CONTENT;

typedef struct _EHCI_HCC_CONTENT
{
	ULONG cur_addr_bits : 1;        /* 0: 32 bit addressing  1: 64 bit addressing */
	ULONG var_frame_list : 1;		/* 0: 1024 frames, 1: support other number of frames */
	ULONG park_mode : 1;
	ULONG reserved : 1;
	ULONG iso_sched_threshold : 4;
	ULONG eecp_capable : 8;
	ULONG reserved2 : 16;

} EHCI_HCC_CONTENT, *PEHCI_HCC_CONTENT;

typedef struct _EHCI_CAPS {
	UCHAR		length;				/* CAPLENGTH - size of this struct */
	UCHAR		reserved;       	/* offset 0x1 */
	USHORT		hci_version;    	/* HCIVERSION - offset 0x2 */
	ULONG		hcs_params;     	/* HCSPARAMS - offset 0x4 */

	ULONG		hcc_params;      	/* HCCPARAMS - offset 0x8 */
	UCHAR		portroute [8];	 	/* nibbles for routing - offset 0xC */

} EHCI_CAPS, *PEHCI_CAPS;

/* Section 2.3 Host Controller Operational Registers */

#define EHCI_USBCMD  		0x00
#define EHCI_USBSTS  		0x04
#define EHCI_USBINTR		0x08
#define EHCI_FRINDEX		0x0c
#define EHCI_CTRLDSSEGMENT  0x10
#define EHCI_PERIODICLISTBASE 0x14
#define EHCI_ASYNCLISTBASE	0x18
#define EHCI_CONFIGFLAG		0x40
#define EHCI_PORTSC			0x44

#define EHCI_USBINTR_INTE   0x01
#define EHCI_USBINTR_ERR	0x02
#define EHCI_USBINTR_PC		0x04
#define EHCI_USBINTR_FLROVR 0x08
#define EHCI_USBINTR_HSERR	0x10
#define EHCI_USBINTR_ASYNC  0x20

typedef struct _EHCI_USBCMD_CONTENT
{
	ULONG run_stop : 1;
	ULONG hcreset : 1;
	ULONG frame_list_size : 2;
	ULONG periodic_enable : 1;
	ULONG async_enable : 1;
	ULONG door_bell : 1;
	ULONG light_reset : 1;
	ULONG async_park_count : 2;
	ULONG reserved : 1;
	ULONG async_park_enable : 1;
	ULONG reserved1 : 4;
	ULONG int_threshold : 8;
	ULONG reserved2 : 8;

} EHCI_USBCMD_CONTENT, *PEHCI_USBCMD_CONTENT;

typedef struct _EHCI_USBSTS_CONTENT
{
	ULONG ioc : 1;
	ULONG trasac_error : 1;
	ULONG port_change : 1;
	ULONG fl_rollover : 1;
	ULONG host_system_error : 1;
	ULONG async_advance : 1;
	ULONG reserved : 6;
	ULONG hc_halted : 1;
	ULONG reclaimation : 1;
	ULONG periodic_status : 1;
	ULONG async_status : 1;
	ULONG reserved1 : 16;

} EHCI_USBSTS_CONTENT, *PEHCI_USBSTS_CONTENT;

typedef struct _EHCI_RHPORTSC_CONTENT
{
	ULONG cur_connect : 1;
	ULONG cur_connect_change : 1;
	ULONG port_enable : 1;
	ULONG port_enable_change : 1;
	ULONG over_current : 1;
	ULONG over_current_change : 1;
	ULONG force_port_resume : 1;
	ULONG suspend : 1;
	ULONG port_reset : 1;
	ULONG reserved : 1;
	ULONG line_status : 2;
	ULONG port_power : 1;
	ULONG port_owner : 1;
	ULONG port_indicator : 2;
	ULONG port_test : 4;
	ULONG we_connect : 1;
	ULONG we_disconnect : 1;
	ULONG we_over_current : 1;
	ULONG reserved1 : 9;

} EHCI_RHPORTSC_CONTENT, *PEHCI_RHPORTSC_CONTENT;

typedef struct _EHCI_REGS {

	ULONG		command;
	ULONG		status;
	ULONG		intr_enable;
	ULONG		frame_index;		/* current microframe number */
	ULONG		segment; 			/* address bits 63:32 if needed */
	ULONG		frame_list; 		/* points to periodic list */
	ULONG		async_next;			/* address of next async queue head */
	ULONG		reserved [9];
	ULONG		configured_flag;
	ULONG		port_status [0];	/* up to N_PORTS */

} EHCI_REGS, *PEHCI_REGS;

#pragma pack( pop, usb_align )

/* ehci_hcd->lock guards shared data against other CPUs:
 *   ehci_hcd:	async, reclaim, periodic (and shadow), ...
 *   hcd_dev:	ep[]
 *   ehci_qh:	qh_next, qtd_list
 *   ehci_qtd:	qtd_list
 *
 * Also, hold this lock when talking to HC registers or
 * when updating hw_* fields in shared qh/qtd/... structures.
 */

#define	EHCI_MAX_ROOT_PORTS	15		/* see HCS_N_PORTS */

#define EHCI_DEVICE_NAME "\\Device\\EHCI"

#define EHCI_DOS_DEVICE_NAME "\\DosDevices\\EHCI"

#define EHCI_ITD_POOL_IDX 			INIT_LIST_FLAG_ITD
#define EHCI_QH_POOL_IDX  			INIT_LIST_FLAG_QH
#define EHCI_SITD_POOL_IDX 			INIT_LIST_FLAG_SITD
#define EHCI_FSTN_POOL_IDX 			INIT_LIST_FLAG_FSTN
#define EHCI_QTD_POOL_IDX 			INIT_LIST_FLAG_QTD

#define EHCI_DEFAULT_FRAMES			UHCI_MAX_FRAMES
#define EHCI_MAX_SYNC_BUS_TIME		50000					// stands for 100000 ns, only to get wrapped within one word

#define EHCI_SCHED_INT8_INDEX		0
#define EHCI_SCHED_INT4_INDEX		1
#define EHCI_SCHED_INT2_INDEX		2
#define EHCI_SCHED_FSTN_INDEX		3
#define EHCI_SCHED_INT1_INDEX		4

#define qtd_pool 	( &ehci->elem_pools[ EHCI_QTD_POOL_IDX ] )
#define qh_pool 	( &ehci->elem_pools[ EHCI_QH_POOL_IDX ] )
#define fstn_pool 	( &ehci->elem_pools[ EHCI_FSTN_POOL_IDX ] )
#define itd_pool 	( &ehci->elem_pools[ EHCI_ITD_POOL_IDX ] )
#define sitd_pool 	( &ehci->elem_pools[ EHCI_SITD_POOL_IDX ] )


typedef struct _EHCI_DEV
{
	HCD					hcd_interf;

	EHCI_CAPS			ehci_caps;

	PHYSICAL_ADDRESS   	ehci_reg_base;						// io space
	BOOLEAN				port_mapped;
	PBYTE				port_base;							// note: added by ehci_caps.length, operational regs base addr, not the actural base

	ULONG				frame_count;
	PHYSICAL_ADDRESS	frame_list_phys_addr;

	KSPIN_LOCK          frame_list_lock;    				// run at DIRQL
    PULONG              frame_list;							// periodic schedule

	PFRAME_LIST_CPU_ENTRY frame_list_cpu;					// periodic schedule shadow

    LIST_HEAD           urb_list;                   	    // active urb-list

	LIST_HEAD			async_list_cpu;
	LIST_HEAD			periodic_list_cpu[ 8 ];			// each slot for one periodic
	PEHCI_QH			skel_async_qh;


	//
    // pools for device specific data
	//
	EHCI_ELEM_POOL 		elem_pools[ 5 ];

	//
    //for iso and int bandwidth claim, bandwidth schedule
	//
	KSPIN_LOCK 			pending_endp_list_lock;				//lock to access the following two
	LIST_HEAD 			pending_endp_list;
	UHCI_PENDING_ENDP_POOL  pending_endp_pool;
	PUSHORT 	        frame_bw;							//unit uFrame
	USHORT				min_bw;								//the bottle-neck of the bandwidths across frame-list

	KTIMER				reset_timer;						//used to reset the host controller
	struct _EHCI_DEVICE_EXTENSION    *pdev_ext;
    PUSB_DEV            root_hub;							//root hub

} EHCI_DEV, *PEHCI_DEV;

typedef UHCI_PORT EHCI_MEMORY;

typedef struct _EHCI_DEVICE_EXTENSION
{
	//struct _USB_DEV_MANAGER 	*pdev_mgr;
	DEVEXT_HEADER		dev_ext_hdr;
	PDEVICE_OBJECT     	pdev_obj;
	PDRIVER_OBJECT  	pdrvr_obj;
	PEHCI_DEV 			ehci;

	//device resources
    PADAPTER_OBJECT     padapter;
	ULONG 				map_regs;
	PCM_RESOURCE_LIST 	res_list;
    ULONG               pci_addr;	// bus number | slot number | funciton number
	UHCI_INTERRUPT   	res_interrupt;
	union
	{
		UHCI_PORT 			res_port;
		EHCI_MEMORY 		res_memory;
	};

	PKINTERRUPT			ehci_int;
	KDPC   				ehci_dpc;

} EHCI_DEVICE_EXTENSION, *PEHCI_DEVICE_EXTENSION;

/*-------------------------------------------------------------------------*/

#endif /* __EHCI_H__ */
