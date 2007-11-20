/*
 * Copyright (c) 2007 by Aleksey Bragin
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
#ifndef __OHCI_H__
#define __OHCI_H__

#define OHCI_DEVICE_NAME "\\Device\\OHCI"
#define OHCI_DOS_DEVICE_NAME "\\DosDevices\\OHCI"

/* Host Controller Operational Registers */

#define OHCI_REVISION       0x0
#define OHCI_CONTROL        0x4
#define OHCI_CMDSTATUS      0x8
#define OHCI_INTRSTATUS     0xc
#define OHCI_INTRENABLE     0x10
#define OHCI_INTRDISABLE    0x14

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

/*
 * OHCI Endpoint Descriptor (ED) ... holds TD queue
 * See OHCI spec, section 4.2
 *
 * This is a "Queue Head" for those transfers, which is why
 * both EHCI and UHCI call similar structures a "QH".
 */
typedef struct _OHCI_ED {
	/* first fields are hardware-specified */
	ULONG			hwINFO;       	/* endpoint config bitmap */
	/* info bits defined by hcd */
#define ED_DEQUEUE	(1 << 27)
	/* info bits defined by the hardware */
#define ED_ISO		(1 << 15)
#define ED_SKIP		(1 << 14)
#define ED_LOWSPEED	(1 << 13)
#define ED_OUT		(0x01 << 11)
#define ED_IN		(0x02 << 11)
	ULONG			hwTailP;	/* tail of TD list */
	ULONG			hwHeadP;	/* head of TD list (hc r/w) */
#define ED_C		(0x02)			/* toggle carry */
#define ED_H		(0x01)			/* halted */
	ULONG			hwNextED;	/* next ED in list */

	/* rest are purely for the driver's use */
#if 0
	dma_addr_t		dma;		/* addr of ED */
	struct _OHCI_TD		*dummy;		/* next TD to activate */

	/* host's view of schedule */
	struct _OHCI_ED		*ed_next;	/* on schedule or rm_list */
	struct _OHCI_ED		*ed_prev;	/* for non-interrupt EDs */
	struct list_head	td_list;	/* "shadow list" of our TDs */

	/* create --> IDLE --> OPER --> ... --> IDLE --> destroy
	 * usually:  OPER --> UNLINK --> (IDLE | OPER) --> ...
	 */
	UCHAR			state;		/* ED_{IDLE,UNLINK,OPER} */
#define ED_IDLE 	0x00		/* NOT linked to HC */
#define ED_UNLINK 	0x01		/* being unlinked from hc */
#define ED_OPER		0x02		/* IS linked to hc */

	UCHAR			type; 		/* PIPE_{BULK,...} */

	/* periodic scheduling params (for intr and iso) */
	UCHAR			branch;
	USHORT			interval;
	USHORT			load;
	USHORT			last_iso;	/* iso only */

	/* HC may see EDs on rm_list until next frame (frame_no == tick) */
	USHORT			tick;
#endif
} OHCI_ED, *POHCI_ED;

#define ED_MASK	((u32)~0x0f)		/* strip hw status in low addr bits */

 
/*
 * OHCI Transfer Descriptor (TD) ... one per transfer segment
 * See OHCI spec, sections 4.3.1 (general = control/bulk/interrupt)
 * and 4.3.2 (iso)
 */
typedef struct _OHCI_TD {
	/* first fields are hardware-specified */
	ULONG		hwINFO;		/* transfer info bitmask */

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

  	ULONG		hwCBP;		/* Current Buffer Pointer (or 0) */
  	ULONG		hwNextTD;	/* Next TD Pointer */
  	ULONG		hwBE;		/* Memory Buffer End Pointer */

	/* PSW is only for ISO.  Only 1 PSW entry is used, but on
	 * big-endian PPC hardware that's the second entry.
	 */
#define MAXPSW	2
  	USHORT		hwPSW [MAXPSW];

	/* rest are purely for the driver's use */
#if 0
  	UCHAR		index;
  	struct ed	*ed;
  	struct td	*td_hash;	/* dma-->td hashtable */
  	struct td	*next_dl_td;
  	struct urb	*urb;

	dma_addr_t	td_dma;		/* addr of this TD */
	dma_addr_t	data_dma;	/* addr of data it points to */

	struct list_head td_list;	/* "shadow list", TDs on same ED */
#endif
} OHCI_TD, *POHCI_TD;

/*
 * The HCCA (Host Controller Communications Area) is a 256 byte
 * structure defined section 4.4.1 of the OHCI spec. The HC is
 * told the base address of it.  It must be 256-byte aligned.
 */
typedef struct _OHCI_HCCA
{
#define NUM_INTS 32
	ULONG	int_table [NUM_INTS];	/* periodic schedule */

	/* 
	 * OHCI defines u16 frame_no, followed by u16 zero pad.
	 * Since some processors can't do 16 bit bus accesses,
	 * portable access must be a 32 bits wide.
	 */
	ULONG	frame_no;		/* current frame number */
	ULONG	done_head;		/* info returned for an interrupt */
	UCHAR	reserved_for_hc [116];
	UCHAR	what [4];		/* spec only identifies 252 bytes :) */
} OHCI_HCCA, *POHCI_HCCA;

/*
 * This is the structure of the OHCI controller's memory mapped I/O region.
 * You must use readl() and writel() (in <asm/io.h>) to access these fields!!
 * Layout is in section 7 (and appendix B) of the spec.
 */
typedef struct _OHCI_REGS
{
	/* control and status registers (section 7.1) */
	ULONG	revision;
	ULONG	control;
	ULONG	cmdstatus;
	ULONG	intrstatus;
	ULONG	intrenable;
	ULONG	intrdisable;

	/* memory pointers (section 7.2) */
	ULONG	hcca;
	ULONG	ed_periodcurrent;
	ULONG	ed_controlhead;
	ULONG	ed_controlcurrent;
	ULONG	ed_bulkhead;
	ULONG	ed_bulkcurrent;
	ULONG	donehead;

	/* frame counters (section 7.3) */
	ULONG	fminterval;
	ULONG	fmremaining;
	ULONG	fmnumber;
	ULONG	periodicstart;
	ULONG	lsthresh;

	/* Root hub ports (section 7.4) */
	struct	ohci_roothub_regs {
		ULONG	a;
		ULONG	b;
		ULONG	status;
#define MAX_ROOT_PORTS	15	/* maximum OHCI root hub ports (RH_A_NDP) */
		ULONG	portstatus [MAX_ROOT_PORTS];
	} roothub;

	/* and optional "legacy support" registers (appendix B) at 0x0100 */
} OHCI_REGS, *POHCI_REGS;

typedef struct _OHCI_DEV
{
    HCD     hcd_interf;

    PHYSICAL_ADDRESS   	ohci_reg_base;						// io space
    BOOLEAN				port_mapped;
    PBYTE				port_base;							// note: added by ehci_caps.length, operational regs base addr, not the actural base
    struct _OHCI_REGS   *regs;
    struct _OHCI_HCCA   *hcca;
    PVOID               td_cache;
    PVOID               ed_cache;

    PHYSICAL_ADDRESS	hcca_logic_addr;
    PHYSICAL_ADDRESS	td_logic_addr;
    PHYSICAL_ADDRESS	ed_logic_addr;

    USHORT              num_ports;

    KTIMER				reset_timer;						//used to reset the host controller
    struct _OHCI_DEVICE_EXTENSION    *pdev_ext;
    PUSB_DEV            root_hub;							//root hub
} OHCI_DEV, *POHCI_DEV;

typedef struct _OHCI_DEVICE_EXTENSION
{
    DEVEXT_HEADER		dev_ext_hdr;
    PDEVICE_OBJECT     	pdev_obj;
    PDRIVER_OBJECT  	pdrvr_obj;
    POHCI_DEV 			ohci;

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

    PKINTERRUPT			ohci_int;
    KDPC   				ohci_dpc;
} OHCI_DEVICE_EXTENSION, *POHCI_DEVICE_EXTENSION;

#define ohci_from_hcd( hCD ) ( struct_ptr( ( hCD ), OHCI_DEV, hcd_interf ) )

#endif /* __OHCI_H__ */
