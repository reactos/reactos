/*
 * OHCI HCD (Host Controller Driver) for USB.
 * 
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * 
 * This file is licenced under the GPL.
 */
 
/*
 * OHCI Endpoint Descriptor (ED) ... holds TD queue
 * See OHCI spec, section 4.2
 *
 * This is a "Queue Head" for those transfers, which is why
 * both EHCI and UHCI call similar structures a "QH".
 */
struct ed {
	/* first fields are hardware-specified, le32 */
	__u32			hwINFO;       	/* endpoint config bitmap */
	/* info bits defined by hcd */
#define ED_DEQUEUE	__constant_cpu_to_le32(1 << 27)
	/* info bits defined by the hardware */
#define ED_ISO		__constant_cpu_to_le32(1 << 15)
#define ED_SKIP		__constant_cpu_to_le32(1 << 14)
#define ED_LOWSPEED	__constant_cpu_to_le32(1 << 13)
#define ED_OUT		__constant_cpu_to_le32(0x01 << 11)
#define ED_IN		__constant_cpu_to_le32(0x02 << 11)
	__u32			hwTailP;	/* tail of TD list */
	__u32			hwHeadP;	/* head of TD list (hc r/w) */
#define ED_C		__constant_cpu_to_le32(0x02)	/* toggle carry */
#define ED_H		__constant_cpu_to_le32(0x01)	/* halted */
	__u32			hwNextED;	/* next ED in list */

	/* rest are purely for the driver's use */
	dma_addr_t		dma;		/* addr of ED */
	struct td		*dummy;		/* next TD to activate */

	/* host's view of schedule */
	struct ed		*ed_next;	/* on schedule or rm_list */
	struct ed		*ed_prev;	/* for non-interrupt EDs */
	struct list_head	td_list;	/* "shadow list" of our TDs */

	/* create --> IDLE --> OPER --> ... --> IDLE --> destroy
	 * usually:  OPER --> UNLINK --> (IDLE | OPER) --> ...
	 * some special cases :  OPER --> IDLE ...
	 */
	u8			state;		/* ED_{IDLE,UNLINK,OPER} */
#define ED_IDLE 	0x00		/* NOT linked to HC */
#define ED_UNLINK 	0x01		/* being unlinked from hc */
#define ED_OPER		0x02		/* IS linked to hc */

	u8			type; 		/* PIPE_{BULK,...} */

	/* periodic scheduling params (for intr and iso) */
	u8			branch;
	u16			interval;
	u16			load;
	u16			last_iso;	/* iso only */

	/* HC may see EDs on rm_list until next frame (frame_no == tick) */
	u16			tick;
} __attribute__ ((aligned(16)));

#define ED_MASK	((u32)~0x0f)		/* strip hw status in low addr bits */

 
/*
 * OHCI Transfer Descriptor (TD) ... one per transfer segment
 * See OHCI spec, sections 4.3.1 (general = control/bulk/interrupt)
 * and 4.3.2 (iso)
 */
struct td {
	/* first fields are hardware-specified, le32 */
	__u32		hwINFO;		/* transfer info bitmask */

	/* hwINFO bits for both general and iso tds: */
#define TD_CC       0xf0000000			/* condition code */
#define TD_CC_GET(td_p) ((td_p >>28) & 0x0f)
//#define TD_CC_SET(td_p, cc) (td_p) = ((td_p) & 0x0fffffff) | (((cc) & 0x0f) << 28)
#define TD_DI       0x00E00000			/* frames before interrupt */
#define TD_DI_SET(X) (((X) & 0x07)<< 21)
	/* these two bits are available for definition/use by HCDs in both
	 * general and iso tds ... others are available for only one type
	 */
#define TD_DONE     0x00020000			/* retired to donelist */
#define TD_ISO      0x00010000			/* copy of ED_ISO */

	/* hwINFO bits for general tds: */
#define TD_EC       0x0C000000			/* error count */
#define TD_T        0x03000000			/* data toggle state */
#define TD_T_DATA0  0x02000000				/* DATA0 */
#define TD_T_DATA1  0x03000000				/* DATA1 */
#define TD_T_TOGGLE 0x00000000				/* uses ED_C */
#define TD_DP       0x00180000			/* direction/pid */
#define TD_DP_SETUP 0x00000000			/* SETUP pid */
#define TD_DP_IN    0x00100000				/* IN pid */
#define TD_DP_OUT   0x00080000				/* OUT pid */
							/* 0x00180000 rsvd */
#define TD_R        0x00040000			/* round: short packets OK? */

	/* (no hwINFO #defines yet for iso tds) */

  	__u32		hwCBP;		/* Current Buffer Pointer (or 0) */
  	__u32		hwNextTD;	/* Next TD Pointer */
  	__u32		hwBE;		/* Memory Buffer End Pointer */

	/* PSW is only for ISO */
#define MAXPSW 1		/* hardware allows 8 */
  	__u16		hwPSW [MAXPSW];

	/* rest are purely for the driver's use */
  	__u8		index;
  	struct ed	*ed;
  	struct td	*td_hash;	/* dma-->td hashtable */
  	struct td	*next_dl_td;
  	struct urb	*urb;

	dma_addr_t	td_dma;		/* addr of this TD */
	dma_addr_t	data_dma;	/* addr of data it points to */

	struct list_head td_list;	/* "shadow list", TDs on same ED */
} __attribute__ ((aligned(32)));	/* c/b/i need 16; only iso needs 32 */

#define TD_MASK	((u32)~0x1f)		/* strip hw status in low addr bits */

/*
 * Hardware transfer status codes -- CC from td->hwINFO or td->hwPSW
 */
#define TD_CC_NOERROR      0x00
#define TD_CC_CRC          0x01
#define TD_CC_BITSTUFFING  0x02
#define TD_CC_DATATOGGLEM  0x03
#define TD_CC_STALL        0x04
#define TD_DEVNOTRESP      0x05
#define TD_PIDCHECKFAIL    0x06
#define TD_UNEXPECTEDPID   0x07
#define TD_DATAOVERRUN     0x08
#define TD_DATAUNDERRUN    0x09
    /* 0x0A, 0x0B reserved for hardware */
#define TD_BUFFEROVERRUN   0x0C
#define TD_BUFFERUNDERRUN  0x0D
    /* 0x0E, 0x0F reserved for HCD */
#define TD_NOTACCESSED     0x0F


/* map OHCI TD status codes (CC) to errno values */ 
static const int cc_to_error [16] = { 
	/* No  Error  */               0,
	/* CRC Error  */               -EILSEQ,
	/* Bit Stuff  */               -EPROTO,
	/* Data Togg  */               -EILSEQ,
	/* Stall      */               -EPIPE,
	/* DevNotResp */               -ETIMEDOUT,
	/* PIDCheck   */               -EPROTO,
	/* UnExpPID   */               -EPROTO,
	/* DataOver   */               -EOVERFLOW,
	/* DataUnder  */               -EREMOTEIO,
	/* (for hw)   */               -EIO,
	/* (for hw)   */               -EIO,
	/* BufferOver */               -ECOMM,
	/* BuffUnder  */               -ENOSR,
	/* (for HCD)  */               -EALREADY,
	/* (for HCD)  */               -EALREADY 
};


/*
 * The HCCA (Host Controller Communications Area) is a 256 byte
 * structure defined section 4.4.1 of the OHCI spec. The HC is
 * told the base address of it.  It must be 256-byte aligned.
 */
struct ohci_hcca {
#define NUM_INTS 32
	__u32	int_table [NUM_INTS];	/* periodic schedule */
	__u16	frame_no;		/* current frame number */
	__u16	pad1;			/* set to 0 on each frame_no change */
	__u32	done_head;		/* info returned for an interrupt */
	u8	reserved_for_hc [116];
	u8	what [4];		/* spec only identifies 252 bytes :) */
} __attribute__ ((aligned(256)));

  
/*
 * This is the structure of the OHCI controller's memory mapped I/O region.
 * You must use readl() and writel() (in <asm/io.h>) to access these fields!!
 * Layout is in section 7 (and appendix B) of the spec.
 */
struct ohci_regs {
	/* control and status registers (section 7.1) */
	__u32	revision;
	__u32	control;
	__u32	cmdstatus;
	__u32	intrstatus;
	__u32	intrenable;
	__u32	intrdisable;

	/* memory pointers (section 7.2) */
	__u32	hcca;
	__u32	ed_periodcurrent;
	__u32	ed_controlhead;
	__u32	ed_controlcurrent;
	__u32	ed_bulkhead;
	__u32	ed_bulkcurrent;
	__u32	donehead;

	/* frame counters (section 7.3) */
	__u32	fminterval;
	__u32	fmremaining;
	__u32	fmnumber;
	__u32	periodicstart;
	__u32	lsthresh;

	/* Root hub ports (section 7.4) */
	struct	ohci_roothub_regs {
		__u32	a;
		__u32	b;
		__u32	status;
#define MAX_ROOT_PORTS	15	/* maximum OHCI root hub ports (RH_A_NDP) */
		__u32	portstatus [MAX_ROOT_PORTS];
	} roothub;

	/* and optional "legacy support" registers (appendix B) at 0x0100 */

} __attribute__ ((aligned(32)));


/* OHCI CONTROL AND STATUS REGISTER MASKS */

/*
 * HcControl (control) register masks
 */
#define OHCI_CTRL_CBSR	(3 << 0)	/* control/bulk service ratio */
#define OHCI_CTRL_PLE	(1 << 2)	/* periodic list enable */
#define OHCI_CTRL_IE	(1 << 3)	/* isochronous enable */
#define OHCI_CTRL_CLE	(1 << 4)	/* control list enable */
#define OHCI_CTRL_BLE	(1 << 5)	/* bulk list enable */
#define OHCI_CTRL_HCFS	(3 << 6)	/* host controller functional state */
#define OHCI_CTRL_IR	(1 << 8)	/* interrupt routing */
#define OHCI_CTRL_RWC	(1 << 9)	/* remote wakeup connected */
#define OHCI_CTRL_RWE	(1 << 10)	/* remote wakeup enable */

/* pre-shifted values for HCFS */
#	define OHCI_USB_RESET	(0 << 6)
#	define OHCI_USB_RESUME	(1 << 6)
#	define OHCI_USB_OPER	(2 << 6)
#	define OHCI_USB_SUSPEND	(3 << 6)

/*
 * HcCommandStatus (cmdstatus) register masks
 */
#define OHCI_HCR	(1 << 0)	/* host controller reset */
#define OHCI_CLF  	(1 << 1)	/* control list filled */
#define OHCI_BLF  	(1 << 2)	/* bulk list filled */
#define OHCI_OCR  	(1 << 3)	/* ownership change request */
#define OHCI_SOC  	(3 << 16)	/* scheduling overrun count */

/*
 * masks used with interrupt registers:
 * HcInterruptStatus (intrstatus)
 * HcInterruptEnable (intrenable)
 * HcInterruptDisable (intrdisable)
 */
#define OHCI_INTR_SO	(1 << 0)	/* scheduling overrun */
#define OHCI_INTR_WDH	(1 << 1)	/* writeback of done_head */
#define OHCI_INTR_SF	(1 << 2)	/* start frame */
#define OHCI_INTR_RD	(1 << 3)	/* resume detect */
#define OHCI_INTR_UE	(1 << 4)	/* unrecoverable error */
#define OHCI_INTR_FNO	(1 << 5)	/* frame number overflow */
#define OHCI_INTR_RHSC	(1 << 6)	/* root hub status change */
#define OHCI_INTR_OC	(1 << 30)	/* ownership change */
#define OHCI_INTR_MIE	(1 << 31)	/* master interrupt enable */


/* OHCI ROOT HUB REGISTER MASKS */
 
/* roothub.portstatus [i] bits */
#define RH_PS_CCS            0x00000001   	/* current connect status */
#define RH_PS_PES            0x00000002   	/* port enable status*/
#define RH_PS_PSS            0x00000004   	/* port suspend status */
#define RH_PS_POCI           0x00000008   	/* port over current indicator */
#define RH_PS_PRS            0x00000010  	/* port reset status */
#define RH_PS_PPS            0x00000100   	/* port power status */
#define RH_PS_LSDA           0x00000200    	/* low speed device attached */
#define RH_PS_CSC            0x00010000 	/* connect status change */
#define RH_PS_PESC           0x00020000   	/* port enable status change */
#define RH_PS_PSSC           0x00040000    	/* port suspend status change */
#define RH_PS_OCIC           0x00080000    	/* over current indicator change */
#define RH_PS_PRSC           0x00100000   	/* port reset status change */

/* roothub.status bits */
#define RH_HS_LPS	     0x00000001		/* local power status */
#define RH_HS_OCI	     0x00000002		/* over current indicator */
#define RH_HS_DRWE	     0x00008000		/* device remote wakeup enable */
#define RH_HS_LPSC	     0x00010000		/* local power status change */
#define RH_HS_OCIC	     0x00020000		/* over current indicator change */
#define RH_HS_CRWE	     0x80000000		/* clear remote wakeup enable */

/* roothub.b masks */
#define RH_B_DR		0x0000ffff		/* device removable flags */
#define RH_B_PPCM	0xffff0000		/* port power control mask */

/* roothub.a masks */
#define	RH_A_NDP	(0xff << 0)		/* number of downstream ports */
#define	RH_A_PSM	(1 << 8)		/* power switching mode */
#define	RH_A_NPS	(1 << 9)		/* no power switching */
#define	RH_A_DT		(1 << 10)		/* device type (mbz) */
#define	RH_A_OCPM	(1 << 11)		/* over current protection mode */
#define	RH_A_NOCP	(1 << 12)		/* no over current protection */
#define	RH_A_POTPGT	(0xff << 24)		/* power on to power good time */


/* hcd-private per-urb state */
typedef struct urb_priv {
	struct ed		*ed;
	__u16			length;		// # tds in this request
	__u16			td_cnt;		// tds already serviced
	int   			state;
	struct td		*td [0];	// all TDs in this request

} urb_priv_t;

#define URB_DEL 1

#define TD_HASH_SIZE    64    /* power'o'two */
// sizeof (struct td) ~= 64 == 2^6 ... 
#define TD_HASH_FUNC(td_dma) ((td_dma ^ (td_dma >> 6)) % TD_HASH_SIZE)


/*
 * This is the full ohci controller description
 *
 * Note how the "proper" USB information is just
 * a subset of what the full implementation needs. (Linus)
 */

struct ohci_hcd {
	spinlock_t		lock;

	/*
	 * I/O memory used to communicate with the HC (dma-consistent)
	 */
	struct ohci_regs	*regs;

	/*
	 * main memory used to communicate with the HC (dma-consistent).
	 * hcd adds to schedule for a live hc any time, but removals finish
	 * only at the start of the next frame.
	 */
	struct ohci_hcca	*hcca;
	dma_addr_t		hcca_dma;

	struct ed		*ed_rm_list;		/* to be removed */

	struct ed		*ed_bulktail;		/* last in bulk list */
	struct ed		*ed_controltail;	/* last in ctrl list */
 	struct ed		*periodic [NUM_INTS];	/* shadow int_table */

	/*
	 * memory management for queue data structures
	 */
	struct pci_pool		*td_cache;
	struct pci_pool		*ed_cache;
	struct td		*td_hash [TD_HASH_SIZE];

	/*
	 * driver state
	 */
	int			disabled;	/* e.g. got a UE, we're hung */
	int			sleeping;
	int			load [NUM_INTS];
	u32 			hc_control;	/* copy of hc control reg */

	unsigned long		flags;		/* for HC bugs */
#define	OHCI_QUIRK_AMD756	0x01			/* erratum #4 */
#define	OHCI_QUIRK_SUPERIO	0x02			/* natsemi */
	// there are also chip quirks/bugs in init logic

	/*
	 * framework state
	 */
	struct usb_hcd		hcd;
};

#define hcd_to_ohci(hcd_ptr) container_of(hcd_ptr, struct ohci_hcd, hcd)

/*-------------------------------------------------------------------------*/

#ifndef DEBUG
#define STUB_DEBUG_FILES
#endif	/* DEBUG */

#define ohci_dbg(ohci, fmt, args...) \
	dev_dbg ((ohci)->hcd.controller , fmt , ## args )
#define ohci_err(ohci, fmt, args...) \
	dev_err ((ohci)->hcd.controller , fmt , ## args )
#define ohci_info(ohci, fmt, args...) \
	dev_info ((ohci)->hcd.controller , fmt , ## args )
#define ohci_warn(ohci, fmt, args...) \
	dev_warn ((ohci)->hcd.controller , fmt , ## args )

#ifdef OHCI_VERBOSE_DEBUG
#	define ohci_vdbg ohci_dbg
#else
#	define ohci_vdbg(ohci, fmt, args...) do { } while (0)
#endif

