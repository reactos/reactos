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
