#ifndef __INCLUDE_HAL_MPS
#define __INCLUDE_HAL_MPS

/* 
 * FIXME: This does not work if we have more than 24 IRQs (ie. more than one 
 * I/O APIC) 
 */
#define IRQL2VECTOR(irql)   (IRQ2VECTOR(PROFILE_LEVEL - (irql)))

#define IRQL2TPR(irql)	    ((irql) >= IPI_LEVEL ? IPI_VECTOR : ((irql) >= PROFILE_LEVEL ? LOCAL_TIMER_VECTOR : ((irql) > DISPATCH_LEVEL ? IRQL2VECTOR(irql) : 0)))

#define IOAPIC_DEFAULT_BASE   0xFEC00000    /* Default I/O APIC Base Register Address */

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



#if 0
/* This values are defined in halirql.h */
#define FIRST_DEVICE_VECTOR	    0x30
#define FIRST_SYSTEM_VECTOR	    0xEF
#endif

#define NUMBER_DEVICE_VECTORS	    (FIRST_SYSTEM_VECTOR - FIRST_DEVICE_VECTOR)


/* MP Floating Pointer Structure */
#define MPF_SIGNATURE (('_' << 24) | ('P' << 16) | ('M' << 8) | '_')

typedef struct __attribute__((packed)) _MP_FLOATING_POINTER
{
	ULONG Signature;     /* _MP_ */
	ULONG Address;          /* Physical Address Pointer (0 means no configuration table exist) */
	UCHAR Length;           /* Structure length in 16-byte paragraphs */
	UCHAR Specification;    /* Specification revision	*/
	UCHAR Checksum;         /* Checksum */
	UCHAR Feature1;         /* MP System Configuration Type */
	UCHAR Feature2;         /* Bit 7 set for IMCR|PIC */
	UCHAR Feature3;         /* Unused (0) */
	UCHAR Feature4;         /* Unused (0) */
	UCHAR Feature5;         /* Unused (0) */
} __attribute__((packed)) MP_FLOATING_POINTER, *PMP_FLOATING_POINTER;

#define FEATURE2_IMCRP  0x80

/* MP Configuration Table Header */
#define MPC_SIGNATURE (('P' << 24) | ('M' << 16) | ('C' << 8) | 'P')

typedef struct __attribute__((packed)) _MP_CONFIGURATION_TABLE
{
  ULONG Signature;     /* PCMP */
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
} __attribute__((packed)) MP_CONFIGURATION_TABLE, *PMP_CONFIGURATION_TABLE;

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
} __attribute__((packed)) MP_CONFIGURATION_PROCESSOR, 
  *PMP_CONFIGURATION_PROCESSOR;

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
} __attribute__((packed)) MP_CONFIGURATION_BUS, *PMP_CONFIGURATION_BUS;

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
} __attribute__((packed)) MP_CONFIGURATION_IOAPIC, *PMP_CONFIGURATION_IOAPIC;

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
} __attribute__((packed)) MP_CONFIGURATION_INTSRC, *PMP_CONFIGURATION_INTSRC;

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
  
#if defined(__GNUC__)
   __asm__ __volatile__ ("rdtsc" : "=a" (nLow), "=d" (nHigh));
#elif defined(_MSC_VER)
   __asm rdtsc
   __asm mov nLow, eax
   __asm mov nHigh, edx
#else
#error Unknown compiler for inline assembler
#endif

   Count->u.LowPart = nLow;
   Count->u.HighPart = nHigh;
}




/* CPU flags */
#define CPU_USABLE   0x01  /* 1 if the CPU is usable (ie. can be used) */
#define CPU_ENABLED  0x02  /* 1 if the CPU is enabled */
#define CPU_BSP      0x04  /* 1 if the CPU is the bootstrap processor */


typedef enum {
  amPIC = 0,    /* IMCR and PIC compatibility mode */
  amVWIRE       /* Virtual Wire compatibility mode */
} APIC_MODE;


#define PIC_IRQS  16

/* Prototypes */

VOID HalpInitMPS(VOID);
volatile ULONG IOAPICRead(ULONG Apic, ULONG Offset);
VOID IOAPICWrite(ULONG Apic, ULONG Offset, ULONG Value);
VOID IOAPICMaskIrq(ULONG Irq);
VOID IOAPICUnmaskIrq(ULONG Irq);

/* For debugging */
VOID IOAPICDump(VOID);

#endif /* __INCLUDE_HAL_MPS */

/* EOF */
