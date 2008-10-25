/*
 *
 */

#ifndef __INTERNAL_HAL_IOAPIC_H
#define __INTERNAL_HAL_IOAPIC_H

/* I/O APIC Register Address Map */
#define IOAPIC_IOREGSEL 0x0000  /* I/O Register Select (index) (R/W) */
#define IOAPIC_IOWIN    0x0010  /* I/O window (data) (R/W) */

#define IOAPIC_ID       0x0000  /* IO APIC ID (R/W) */
#define IOAPIC_VER      0x0001  /* IO APIC Version (R) */
#define IOAPIC_ARB      0x0002  /* IO APIC Arbitration ID (R) */
#define IOAPIC_REDTBL   0x0010  /* Redirection Table (0-23 64-bit registers) (R/W) */

#define IOAPIC_ID_MASK        (0xF << 24)
#define GET_IOAPIC_ID(x)	    (((x) & IOAPIC_ID_MASK) >> 24)
#define SET_IOAPIC_ID(x)	    ((x) << 24)

#define IOAPIC_VER_MASK       (0xFF)
#define GET_IOAPIC_VERSION(x) (((x) & IOAPIC_VER_MASK))
#define IOAPIC_MRE_MASK       (0xFF << 16)  /* Maximum Redirection Entry */
#define GET_IOAPIC_MRE(x)     (((x) & IOAPIC_MRE_MASK) >> 16)

#define IOAPIC_ARB_MASK       (0xF << 24)
#define GET_IOAPIC_ARB(x)	    (((x) & IOAPIC_ARB_MASK) >> 24)

#define IOAPIC_TBL_DELMOD   (0x7 << 10) /* Delivery Mode (see APIC_DM_*) */
#define IOAPIC_TBL_DM       (0x1 << 11) /* Destination Mode */
#define IOAPIC_TBL_DS       (0x1 << 12) /* Delivery Status */
#define IOAPIC_TBL_INTPOL   (0x1 << 13) /* Interrupt Input Pin Polarity */
#define IOAPIC_TBL_RIRR     (0x1 << 14) /* Remote IRR */
#define IOAPIC_TBL_TM       (0x1 << 15) /* Trigger Mode */
#define IOAPIC_TBL_IM       (0x1 << 16) /* Interrupt Mask */
#define IOAPIC_TBL_DF0      (0xF << 56) /* Destination Field (physical mode) */
#define IOAPIC_TBL_DF1      (0xFF<< 56) /* Destination Field (logical mode) */
#define IOAPIC_TBL_VECTOR   (0xFF << 0) /* Vector (10h - FEh) */

#include <pshpack1.h>
typedef struct _IOAPIC_ROUTE_ENTRY {
   ULONG vector	    :  8,
   delivery_mode    :  3,   /* 000: FIXED
			     * 001: lowest priority
			     * 111: ExtINT
			     */
   dest_mode	    :  1,   /* 0: physical, 1: logical */
   delivery_status  :  1,
   polarity	    :  1,
   irr		    :  1,
   trigger	    :  1,   /* 0: edge, 1: level */
   mask		    :  1,   /* 0: enabled, 1: disabled */
   __reserved_2	    : 15;

   union {
      struct {
         ULONG __reserved_1  : 24,
               physical_dest :  4,
               __reserved_2  :  4;
      } physical;
      struct {
         ULONG __reserved_1  : 24,
               logical_dest  :  8;
      } logical;
   } dest;
} IOAPIC_ROUTE_ENTRY, *PIOAPIC_ROUTE_ENTRY;
#include <poppack.h>

typedef struct _IOAPIC_INFO
{
   ULONG  ApicId;         /* APIC ID */
   ULONG  ApicVersion;    /* APIC version */
   ULONG  ApicAddress;    /* APIC address */
   ULONG  EntryCount;     /* Number of redirection entries */
} IOAPIC_INFO, *PIOAPIC_INFO;

#define IOAPIC_DEFAULT_BASE   0xFEC00000    /* Default I/O APIC Base Register Address */

extern ULONG IRQCount;					/* Number of IRQs  */
extern UCHAR BUSMap[MAX_BUS];				/* Map of all buses in the system */
extern UCHAR PCIBUSMap[MAX_BUS];			/* Map of all PCI buses in the system */
extern IOAPIC_INFO IOAPICMap[MAX_IOAPIC];		/* Map of all I/O APICs in the system */
extern ULONG IOAPICCount;				/* Number of I/O APICs in the system */
extern ULONG APICMode;					/* APIC mode at startup */
extern MP_CONFIGURATION_INTSRC IRQMap[MAX_IRQ_SOURCE];	/* Map of all IRQs */

VOID IOAPICSetupIrqs(VOID);
VOID IOAPICEnable(VOID);
VOID IOAPICSetupIds(VOID);
VOID IOAPICMaskIrq(ULONG Irq);
VOID IOAPICUnmaskIrq(ULONG Irq);

VOID HaliReconfigurePciInterrupts(VOID);

/* For debugging */
VOID IOAPICDump(VOID);

#endif



/* EOF */

