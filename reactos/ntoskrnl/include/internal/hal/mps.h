#ifndef __INCLUDE_HAL_MPS
#define __INCLUDE_HAL_MPS

#define APIC_DEFAULT_BASE     0xFEE00000    /* Default Local APIC Base Register Address */
#define IOAPIC_DEFAULT_BASE   0xFEC00000    /* Default I/O APIC Base Register Address */

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
#define APIC_LVTPC   0x0340 /* Performance Counter LVT (R/W) */
#define APIC_LINT0   0x0350 /* Local Vector Table (LINT0) (R/W) */
#define APIC_LINT1   0x0360 /* Local Vector Table (LINT1) (R/W) */
#define APIC_LVT3    0x0370 /* Local Vector Table (Error) (R/W) */
#define APIC_ICRT    0x0380 /* Initial Count Register for Timer (R/W) */
#define APIC_CCRT    0x0390 /* Current Count Register for Timer (R) */
#define APIC_TDCR    0x03E0 /* Timer Divide Configuration Register (R/W) */

#define APIC_ID_MASK       (0xF << 24)
#define GET_APIC_ID(x)	   (((x) & APIC_ID_MASK) >> 24)
#define APIC_VER_MASK      0xFF00FF
#define GET_APIC_VERSION(x)((x) & 0xFF)
#define GET_APIC_MAXLVT(x) (((x) >> 16) & 0xFF)

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
#define APIC_DM_FIXED		  (0x0 << 8)
#define APIC_DM_LOWEST  	(0x1 << 8)
#define APIC_DM_SMI		    (0x2 << 8)
#define APIC_DM_REMRD		  (0x3 << 8)
#define APIC_DM_NMI		    (0x4 << 8)
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
#define APIC_LVT_REMOTE_IRR		  (0x1 << 14)		/* Remote IRR */
#define APIC_LVT_LEVEL_TRIGGER	(0x1 << 15)		/* Lvel Triggered */
#define APIC_LVT_MASKED   	    (0x1 << 16)   /* Mask */
#define APIC_LVT_PERIODIC 	  	(0x1 << 17)   /* Timer Mode */

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

#define IPI_CACHE_FLUSH    0x40
#define IPI_INV_TLB        0x41
#define IPI_INV_PTE        0x42
#define IPI_INV_RESCHED    0x43
#define IPI_STOP           0x44


#define APIC_INTEGRATED(version) (version & 0xF0)


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

typedef struct _IOAPIC_ROUTE_ENTRY {
	ULONG	vector		:  8,
		delivery_mode	:  3,	/* 000: FIXED
					               * 001: lowest priority
					               * 111: ExtINT
					               */
		dest_mode	:  1,	    /* 0: physical, 1: logical */
		delivery_status	:  1,
		polarity	:  1,
		irr		:  1,
		trigger		:  1,	    /* 0: edge, 1: level */
		mask		:  1,	      /* 0: enabled, 1: disabled */
		__reserved_2	: 15;

	union {		struct { ULONG
					__reserved_1	: 24,
					physical_dest	:  4,
					__reserved_2	:  4;
			} physical;

			struct { ULONG
					__reserved_1	: 24,
					logical_dest	:  8;
			} logical;
	} dest;
} __attribute__ ((packed)) IOAPIC_ROUTE_ENTRY, *PIOAPIC_ROUTE_ENTRY;

typedef struct _IOAPIC_INFO
{
   ULONG  ApicId;         /* APIC ID */
   ULONG  ApicVersion;    /* APIC version */
   ULONG  ApicAddress;    /* APIC address */
   ULONG  EntryCount;     /* Number of redirection entries */
} IOAPIC_INFO, *PIOAPIC_INFO;


/*
 * Local APIC timer IRQ vector is on a different priority level,
 * to work around the 'lost local interrupt if more than 2 IRQ
 * sources per level' errata.
 */
#define LOCAL_TIMER_VECTOR 	  0xEF

#define CALL_FUNCTION_VECTOR	0xFB
#define RESCHEDULE_VECTOR		  0xFC
#define INVALIDATE_TLB_VECTOR	0xFD
#define ERROR_VECTOR				  0xFE
#define SPURIOUS_VECTOR			  0xFF  /* Must be 0xXF */

/*
 * First APIC vector available to drivers: (vectors 0x30-0xEE)
 * we start at 0x31 to spread out vectors evenly between priority
 * levels.
 */
#define FIRST_DEVICE_VECTOR	  0x31
#define FIRST_SYSTEM_VECTOR	  0xEF
#define NUMBER_DEVICE_VECTORS (FIRST_SYSTEM_VECTOR - FIRST_DEVICE_VECTOR)


/* MP Floating Pointer Structure */
#define MPF_SIGNATURE (('_' << 24) | ('P' << 16) | ('M' << 8) | '_')

typedef struct __attribute__((packed)) _MP_FLOATING_POINTER
{
	ULONG Signature[4];     /* _MP_ */
	ULONG Address;          /* Physical Address Pointer (0 means no configuration table exist) */
	UCHAR Length;           /* Structure length in 16-byte paragraphs */
	UCHAR Specification;    /* Specification revision	*/
	UCHAR Checksum;         /* Checksum */
	UCHAR Feature1;         /* MP System Configuration Type */
	UCHAR Feature2;         /* Bit 7 set for IMCR|PIC */
	UCHAR Feature3;         /* Unused (0) */
	UCHAR Feature4;         /* Unused (0) */
	UCHAR Feature5;         /* Unused (0) */
} MP_FLOATING_POINTER, *PMP_FLOATING_POINTER;

#define FEATURE2_IMCRP  0x80

/* MP Configuration Table Header */
#define MPC_SIGNATURE (('P' << 24) | ('M' << 16) | ('C' << 8) | 'P')

typedef struct __attribute__((packed)) _MP_CONFIGURATION_TABLE
{
	ULONG Signature[4];     /* PCMP */
	USHORT Length;	        /* Size of configuration table */
	CHAR  Specification;    /* Specification Revision */
	CHAR Checksum;          /* Checksum */
	CHAR Oem[8];            /* OEM ID */
	CHAR ProductId[12];     /* Product ID */
	ULONG OemTable;         /* 0 if not present */
	USHORT OemTableSize;    /* 0 if not present */
	USHORT EntryCount;      /* Number of entries */
	ULONG LocalAPICAddress; /* Local APIC address */
  USHORT ExtTableLength;  /* Extended Table Length */
  UCHAR ExtTableChecksum; /* Extended Table Checksum */
	UCHAR Reserved;         /* Reserved */
} MP_CONFIGURATION_TABLE, *PMP_CONFIGURATION_TABLE;

/* MP Configuration Table Entries */
#define MPCTE_PROCESSOR 0   /* One entry per processor */
#define MPCTE_BUS       1   /* One entry per bus */
#define MPCTE_IOAPIC    2   /* One entry per I/O APIC */
#define MPCTE_INTSRC    3   /* One entry per bus interrupt source */
#define MPCTE_LINTSRC   4   /* One entry per system interrupt source */


typedef struct __attribute__((packed)) _MP_CONFIGURATION_PROCESSOR
{
	UCHAR Type;         /* 0 */
	UCHAR ApicId;       /* Local APIC ID for the processor */
	UCHAR ApicVersion;  /* Local APIC version */
	UCHAR CpuFlags;     /* CPU flags */
	ULONG CpuSignature; /* CPU signature */
  ULONG FeatureFlags; /* CPUID feature value */
	ULONG Reserved[2];  /* Reserved (0) */
} MP_CONFIGURATION_PROCESSOR, *PMP_CONFIGURATION_PROCESSOR;

#define CPU_FLAG_ENABLED         1  /* Processor is available */
#define CPU_FLAG_BSP             2  /* Processor is the bootstrap processor */

#define CPU_STEPPING_MASK  0x0F
#define CPU_MODEL_MASK	   0xF0
#define CPU_FAMILY_MASK	   0xF00


typedef struct __attribute__((packed)) _MP_CONFIGURATION_BUS
{
	UCHAR Type;         /* 1 */
	UCHAR BusId;        /* Bus ID */
	UCHAR BusType[6];   /* Bus type */
} MP_CONFIGURATION_BUS, *PMP_CONFIGURATION_BUS;

#define MAX_BUS 32

#define MP_BUS_ISA  1
#define MP_BUS_EISA 2
#define MP_BUS_PCI  3
#define MP_BUS_MCA  4

#define BUSTYPE_EISA	  "EISA"
#define BUSTYPE_ISA	    "ISA"
#define BUSTYPE_INTERN	"INTERN"	/* Internal BUS */
#define BUSTYPE_MCA	    "MCA"
#define BUSTYPE_VL	    "VL"		  /* Local bus */
#define BUSTYPE_PCI	    "PCI"
#define BUSTYPE_PCMCIA	"PCMCIA"
#define BUSTYPE_CBUS	  "CBUS"
#define BUSTYPE_CBUSII	"CBUSII"
#define BUSTYPE_FUTURE	"FUTURE"
#define BUSTYPE_MBI	    "MBI"
#define BUSTYPE_MBII	  "MBII"
#define BUSTYPE_MPI	    "MPI"
#define BUSTYPE_MPSA	  "MPSA"
#define BUSTYPE_NUBUS	  "NUBUS"
#define BUSTYPE_TC	    "TC"
#define BUSTYPE_VME	    "VME"
#define BUSTYPE_XPRESS	"XPRESS"


typedef struct __attribute__((packed)) _MP_CONFIGURATION_IOAPIC
{
	UCHAR Type;         /* 2 */
	UCHAR ApicId;       /* I/O APIC ID */
	UCHAR ApicVersion;  /* I/O APIC version */
	UCHAR ApicFlags;    /* I/O APIC flags */
	ULONG ApicAddress;  /* I/O APIC base address */
} MP_CONFIGURATION_IOAPIC, *PMP_CONFIGURATION_IOAPIC;

#define MAX_IOAPIC  2

#define MP_IOAPIC_USABLE  0x01


typedef struct __attribute__((packed)) _MP_CONFIGURATION_INTSRC
{
	UCHAR Type;         /* 3 */
	UCHAR IrqType;      /* Interrupt type */
	USHORT IrqFlag;     /* Interrupt flags */
	UCHAR SrcBusId;     /* Source bus ID */
	UCHAR SrcBusIrq;    /* Source bus interrupt */
	UCHAR DstApicId;    /* Destination APIC ID */
	UCHAR DstApicInt;   /* Destination interrupt */
} MP_CONFIGURATION_INTSRC, *PMP_CONFIGURATION_INTSRC;

#define MAX_IRQ_SOURCE  128

#define INT_VECTORED    0
#define INT_NMI         1
#define INT_SMI         2
#define INT_EXTINT      3

#define IRQDIR_DEFAULT  0
#define IRQDIR_HIGH     1
#define IRQDIR_LOW      3


typedef struct __attribute__((packed)) _MP_CONFIGURATION_INTLOCAL
{
	UCHAR Type;         /* 4 */
	UCHAR IrqType;      /* Interrupt type */
	USHORT IrqFlag;     /* Interrupt flags */
	UCHAR SrcBusId;     /* Source bus ID */
	UCHAR SrcBusIrq;    /* Source bus interrupt */
	UCHAR DstApicId;    /* Destination local APIC ID */
	UCHAR DstApicLInt;  /* Destination local APIC interrupt */
} MP_CONFIGURATION_INTLOCAL, *PMP_CONFIGURATION_INTLOCAL;

#define MP_APIC_ALL	0xFF


static inline VOID ReadPentiumClock(PULARGE_INTEGER Count)
{
   register ULONG nLow;
   register ULONG nHigh;
  
   __asm__ __volatile__ ("rdtsc" : "=a" (nLow), "=d" (nHigh));

   Count->u.LowPart = nLow;
   Count->u.HighPart = nHigh;
}


#define MAX_CPU   32


typedef struct _CPU_INFO
{
   UCHAR    Flags;            /* CPU flags */
   UCHAR    APICId;           /* Local APIC ID */
   UCHAR    APICVersion;      /* Local APIC version */
   UCHAR    MaxLVT;           /* Number of LVT registers */
   ULONG    BusSpeed;         /* BUS speed */
   ULONG    CoreSpeed;        /* Core speed */
   UCHAR    Padding[16-12];   /* Padding to 16-byte */
} CPU_INFO, *PCPU_INFO;

/* CPU flags */
#define CPU_USABLE   0x01  /* 1 if the CPU is usable (ie. can be used) */
#define CPU_ENABLED  0x02  /* 1 if the CPU is enabled */
#define CPU_BSP      0x04  /* 1 if the CPU is the bootstrap processor */


typedef enum {
  amPIC = 0,    /* IMCR and PIC compatibility mode */
  amVWIRE       /* Virtual Wire compatibility mode */
} APIC_MODE;


#define pushfl(x) __asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define popfl(x) __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")


#define PIC_IRQS  16

/* Prototypes */

VOID HalpInitMPS(VOID);
volatile ULONG IOAPICRead(ULONG Apic, ULONG Offset);
VOID IOAPICWrite(ULONG Apic, ULONG Offset, ULONG Value);
VOID IOAPICMaskIrq(ULONG Apic, ULONG Irq);
VOID IOAPICUnmaskIrq(ULONG Apic, ULONG Irq);
volatile inline ULONG APICRead(ULONG Offset);
inline VOID APICWrite(ULONG Offset, ULONG Value);
inline VOID APICSendEOI(VOID);
inline ULONG ThisCPU(VOID);
VOID APICSendIPI(ULONG Target,
                 ULONG DeliveryMode,
                 ULONG IntNum,
                 ULONG Level);
/* For debugging */
VOID IOAPICDump(VOID);
VOID APICDump(VOID);

#endif /* __INCLUDE_HAL_MPS */