/*
 *
 */

#ifndef __INTERNAL_HAL_APIC_H
#define __INTERNAL_HAL_APIC_H

#define APIC_DEFAULT_BASE     0xFEE00000    /* Default Local APIC Base Register Address */

/* APIC Register Address Map */
#define APIC_ID      0x0020 /* Local APIC ID Register (R/W) */
#define APIC_VER     0x0030 /* Local APIC Version Register (R) */
#define APIC_TPR     0x0080 /* Task Priority Register (R/W) */
#define APIC_APR     0x0090 /* Arbitration Priority Register (R) */
#define APIC_PPR     0x00A0 /* Processor Priority Register (R) */
#define APIC_EOI     0x00B0 /* EOI Register (W) */
#define APIC_LDR     0x00D0 /* Logical Destination Register (R/W) */
#define APIC_DFR     0x00E0 /* Destination Format Register (0-27 R, 28-31 R/W) */
#define APIC_SIVR    0x00F0 /* Spurious Interrupt Vector Register (0-3 R, 4-9 R/W) */
#define APIC_ISR     0x0100 /* Interrupt Service Register 0-255 (R) */
#define APIC_TMR     0x0180 /* Trigger Mode Register 0-255 (R) */
#define APIC_IRR     0x0200 /* Interrupt Request Register 0-255 (r) */
#define APIC_ESR     0x0280 /* Error Status Register (R) */
#define APIC_ICR0    0x0300 /* Interrupt Command Register 0-31 (R/W) */
#define APIC_ICR1    0x0310 /* Interrupt Command Register 32-63 (R/W) */
#define APIC_LVTT    0x0320 /* Local Vector Table (Timer) (R/W) */
#define	APIC_LVTTHMR 0x0330
#define APIC_LVTPC   0x0340 /* Performance Counter LVT (R/W) */
#define APIC_LINT0   0x0350 /* Local Vector Table (LINT0) (R/W) */
#define APIC_LINT1   0x0360 /* Local Vector Table (LINT1) (R/W) */
#define APIC_LVT3    0x0370 /* Local Vector Table (Error) (R/W) */
#define APIC_ICRT    0x0380 /* Initial Count Register for Timer (R/W) */
#define APIC_CCRT    0x0390 /* Current Count Register for Timer (R) */
#define APIC_TDCR    0x03E0 /* Timer Divide Configuration Register (R/W) */

#define APIC_ID_MASK		(0xF << 24)
#define GET_APIC_ID(x)		(((x) & APIC_ID_MASK) >> 24)
#define	GET_APIC_LOGICAL_ID(x)	(((x)>>24)&0xFF)
#define APIC_VER_MASK		0xFF00FF
#define GET_APIC_VERSION(x)	((x) & 0xFF)
#define GET_APIC_MAXLVT(x)	(((x) >> 16) & 0xFF)

#define APIC_TPR_PRI       0xFF
#define APIC_TPR_INT       0xF0
#define APIC_TPR_SUB       0xF
#define APIC_TPR_MAX       0xFF           /* Maximum priority */
#define APIC_TPR_MIN       0x20           /* Minimum priority */

#define APIC_LDR_MASK      (0xFF << 24)

#define APIC_SIVR_ENABLE   (0x1 << 8)
#define APIC_SIVR_FOCUS    (0x1 << 9)

#define APIC_ESR_MASK      (0xFE << 0)    /* Error Mask */

#define APIC_ICR0_VECTOR   (0xFF << 0)    /* Vector */
#define APIC_ICR0_DM       (0x7 << 8)     /* Delivery Mode */
#define APIC_ICR0_DESTM    (0x1 << 11)    /* Destination Mode */
#define APIC_ICR0_DS       (0x1 << 12)    /* Delivery Status */
#define APIC_ICR0_LEVEL    (0x1 << 14)    /* Level */
#define APIC_ICR0_TM       (0x1 << 15)    /* Trigger Mode */
#define APIC_ICR0_DESTS    (0x3 << 18)    /* Destination Shorthand */

/* Delivery Modes */
#define APIC_DM_FIXED	  (0x0 << 8)
#define APIC_DM_LOWEST    (0x1 << 8)
#define APIC_DM_SMI       (0x2 << 8)
#define APIC_DM_REMRD     (0x3 << 8)
#define APIC_DM_NMI       (0x4 << 8)
#define APIC_DM_INIT      (0x5 << 8)
#define APIC_DM_STARTUP   (0x6 << 8)
#define APIC_DM_EXTINT	  (0x7 << 8)
#define GET_APIC_DELIVERY_MODE(x)	(((x) >> 8) & 0x7)
#define SET_APIC_DELIVERY_MODE(x,y)	(((x) & ~0x700) | ((y) << 8))

/* Destination Shorthand values */
#define APIC_ICR0_DESTS_FIELD          (0x0 << 0)
#define APIC_ICR0_DESTS_SELF           (0x1 << 18)
#define APIC_ICR0_DESTS_ALL            (0x2 << 18)
#define APIC_ICR0_DESTS_ALL_BUT_SELF   (0x3 << 18)

#define APIC_ICR0_LEVEL_DEASSERT (0x0 << 14) /* Deassert level */
#define APIC_ICR0_LEVEL_ASSERT   (0x1 << 14) /* Assert level */

#define GET_APIC_DEST_FIELD(x)   (((x) >> 24) & 0xFF)
#define SET_APIC_DEST_FIELD(x)   (((x) & 0xFF) << 24)

#define GET_APIC_TIMER_BASE(x)   (((x) >> 18) & 0x3)
#define SET_APIC_TIMER_BASE(x)   ((x) << 18)
#define APIC_TIMER_BASE_CLKIN    0x0
#define APIC_TIMER_BASE_TMBASE   0x1
#define APIC_TIMER_BASE_DIV      0x2

#define APIC_LVT_VECTOR   		  (0xFF << 0)   /* Vector */
#define APIC_LVT_DS       		  (0x1 << 12)   /* Delivery Status */
#define APIC_LVT_REMOTE_IRR		  (0x1 << 14)	/* Remote IRR */
#define APIC_LVT_LEVEL_TRIGGER		  (0x1 << 15)	/* Lvel Triggered */
#define APIC_LVT_MASKED			  (0x1 << 16)   /* Mask */
#define APIC_LVT_PERIODIC		  (0x1 << 17)   /* Timer Mode */

#define APIC_LVT3_DM        (0x7 << 8)
#define APIC_LVT3_IIPP      (0x1 << 13)
#define APIC_LVT3_TM        (0x1 << 15)
#define APIC_LVT3_MASKED    (0x1 << 16)
#define APIC_LVT3_OS        (0x1 << 17)

#define APIC_TDCR_TMBASE   (0x1 << 2)
#define APIC_TDCR_MASK     0x0F
#define APIC_TDCR_2        0x00
#define APIC_TDCR_4        0x01
#define APIC_TDCR_8        0x02
#define APIC_TDCR_16       0x03
#define APIC_TDCR_32       0x08
#define APIC_TDCR_64       0x09
#define APIC_TDCR_128      0x0A
#define APIC_TDCR_1        0x0B

#define APIC_LVT_VECTOR   		  (0xFF << 0)   /* Vector */
#define APIC_LVT_DS       		  (0x1 << 12)   /* Delivery Status */
#define APIC_LVT_REMOTE_IRR		  (0x1 << 14)	/* Remote IRR */
#define APIC_LVT_LEVEL_TRIGGER		  (0x1 << 15)	/* Lvel Triggered */
#define APIC_LVT_MASKED			  (0x1 << 16)   /* Mask */
#define APIC_LVT_PERIODIC	 	  (0x1 << 17)   /* Timer Mode */

#define APIC_LVT3_DM        (0x7 << 8)
#define APIC_LVT3_IIPP      (0x1 << 13)
#define APIC_LVT3_TM        (0x1 << 15)
#define APIC_LVT3_MASKED    (0x1 << 16)
#define APIC_LVT3_OS        (0x1 << 17)

#define APIC_TDCR_TMBASE   (0x1 << 2)
#define APIC_TDCR_MASK     0x0F
#define APIC_TDCR_2        0x00
#define APIC_TDCR_4        0x01
#define APIC_TDCR_8        0x02
#define APIC_TDCR_16       0x03
#define APIC_TDCR_32       0x08
#define APIC_TDCR_64       0x09
#define APIC_TDCR_128      0x0A
#define APIC_TDCR_1        0x0B

#define APIC_TARGET_SELF         0x100
#define APIC_TARGET_ALL          0x200
#define APIC_TARGET_ALL_BUT_SELF 0x300

#define APIC_INTEGRATED(version) (version & 0xF0)

typedef enum {
  amPIC = 0,    /* IMCR and PIC compatibility mode */
  amVWIRE       /* Virtual Wire compatibility mode */
} APIC_MODE;

#ifdef CONFIG_SMP
#define MAX_CPU   32
#else
#define MAX_CPU	  1
#endif

/*
 * Local APIC timer IRQ vector is on a different priority level,
 * to work around the 'lost local interrupt if more than 2 IRQ
 * sources per level' errata.
 */
#define LOCAL_TIMER_VECTOR 	    0xEF

#define IPI_VECTOR		    0xFB
#define ERROR_VECTOR		    0xFE
#define SPURIOUS_VECTOR		    0xFF  /* Must be 0xXF */

/* CPU flags */
#define CPU_USABLE   0x01  /* 1 if the CPU is usable (ie. can be used) */
#define CPU_ENABLED  0x02  /* 1 if the CPU is enabled */
#define CPU_BSP      0x04  /* 1 if the CPU is the bootstrap processor */
#define CPU_TSC      0x08  /* 1 if the CPU has a time stamp counter */

typedef struct _CPU_INFO
{
   UCHAR    Flags;            /* CPU flags */
   UCHAR    APICId;           /* Local APIC ID */
   UCHAR    APICVersion;      /* Local APIC version */
//   UCHAR    MaxLVT;           /* Number of LVT registers */
   ULONG    BusSpeed;         /* BUS speed */
   ULONG    CoreSpeed;        /* Core speed */
   UCHAR    Padding[16-12];   /* Padding to 16-byte */
} CPU_INFO, *PCPU_INFO;

extern ULONG CPUCount;			/* Total number of CPUs */
extern ULONG BootCPU;			/* Bootstrap processor */
extern ULONG OnlineCPUs;		/* Bitmask of online CPUs */
extern CPU_INFO CPUMap[MAX_CPU];	/* Map of all CPUs in the system */
extern PULONG APICBase;			/* Virtual address of local APIC */
extern ULONG lastregr[MAX_CPU];		/* For debugging */
extern ULONG lastvalr[MAX_CPU];
extern ULONG lastregw[MAX_CPU];
extern ULONG lastvalw[MAX_CPU];

/* Prototypes */
VOID APICSendIPI(ULONG Target, ULONG Mode);
VOID APICSetup(VOID);
VOID HaliInitBSP(VOID);
VOID APICSyncArbIDs(VOID);
VOID APICCalibrateTimer(ULONG CPU);
VOID HaliStartApplicationProcessor(ULONG Cpu, ULONG Stack);

static __inline ULONG _APICRead(ULONG Offset)
{
    PULONG p;

    p = (PULONG)((ULONG)APICBase + Offset);
    return *p;
}

#if 0
static __inline VOID APICWrite(ULONG Offset,
                               ULONG Value)
{
    PULONG p;

    p = (PULONG)((ULONG)APICBase + Offset);

    *p = Value;
}
#else
static __inline VOID APICWrite(ULONG Offset,
                               ULONG Value)
{
    PULONG p;
    ULONG CPU = (_APICRead(APIC_ID) & APIC_ID_MASK) >> 24;

    lastregw[CPU] = Offset;
    lastvalw[CPU] = Value;

    p = (PULONG)((ULONG)APICBase + Offset);

    *p = Value;
}
#endif

#if 0 
static __inline ULONG APICRead(ULONG Offset)
{
    PULONG p;

    p = (PULONG)((ULONG)APICBase + Offset);
    return *p;
}
#else
static __inline ULONG APICRead(ULONG Offset)
{
    PULONG p;
    ULONG CPU = (_APICRead(APIC_ID) & APIC_ID_MASK) >> 24;

    lastregr[CPU] = Offset;
    lastvalr[CPU] = 0;

    p = (PULONG)((ULONG)APICBase + Offset);

    lastvalr[CPU] = *p;
    return lastvalr[CPU];
}
#endif

static __inline ULONG ThisCPU(VOID)
{
    return (APICRead(APIC_ID) & APIC_ID_MASK) >> 24;
}

static __inline VOID APICSendEOI(VOID)
{
    // Send the EOI
    APICWrite(APIC_EOI, 0);
}

#endif /* __INTERNAL_HAL_APIC_H */

/* EOF */
