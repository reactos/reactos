#ifndef __INCLUDE_HAL_MPS
#define __INCLUDE_HAL_MPS

/* 
 * FIXME: This does not work if we have more than 24 IRQs (ie. more than one 
 * I/O APIC) 
 */
#define IRQL2VECTOR(irql)   (IRQ2VECTOR(PROFILE_LEVEL - (irql)))

#define IRQL2TPR(irql)	    ((irql) >= IPI_LEVEL ? IPI_VECTOR : ((irql) >= PROFILE_LEVEL ? LOCAL_TIMER_VECTOR : ((irql) > DISPATCH_LEVEL ? IRQL2VECTOR(irql) : 0)))

typedef struct _KIRQ_TRAPFRAME
{
   ULONG Magic;
   ULONG Gs;
   ULONG Fs;
   ULONG Es;
   ULONG Ds;
   ULONG Eax;
   ULONG Ecx;
   ULONG Edx;
   ULONG Ebx;
   ULONG Esp;
   ULONG Ebp;
   ULONG Esi;
   ULONG Edi;
   ULONG Eip;
   ULONG Cs;
   ULONG Eflags;
} KIRQ_TRAPFRAME, *PKIRQ_TRAPFRAME;

#if 0
/* This values are defined in halirql.h */
#define FIRST_DEVICE_VECTOR	    0x30
#define FIRST_SYSTEM_VECTOR	    0xEF
#endif

#define NUMBER_DEVICE_VECTORS	    (FIRST_SYSTEM_VECTOR - FIRST_DEVICE_VECTOR)


/* MP Floating Pointer Structure */
#define MPF_SIGNATURE (('_' << 24) | ('P' << 16) | ('M' << 8) | '_')

#include <pshpack1.h>
typedef struct _MP_FLOATING_POINTER
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
} MP_FLOATING_POINTER, *PMP_FLOATING_POINTER;


#define FEATURE2_IMCRP  0x80

/* MP Configuration Table Header */
#define MPC_SIGNATURE (('P' << 24) | ('M' << 16) | ('C' << 8) | 'P')

typedef struct _MP_CONFIGURATION_TABLE
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
} MP_CONFIGURATION_TABLE, *PMP_CONFIGURATION_TABLE;

/* MP Configuration Table Entries */
#define MPCTE_PROCESSOR 0   /* One entry per processor */
#define MPCTE_BUS       1   /* One entry per bus */
#define MPCTE_IOAPIC    2   /* One entry per I/O APIC */
#define MPCTE_INTSRC    3   /* One entry per bus interrupt source */
#define MPCTE_LINTSRC   4   /* One entry per system interrupt source */


typedef struct _MP_CONFIGURATION_PROCESSOR
{
  UCHAR Type;         /* 0 */
  UCHAR ApicId;       /* Local APIC ID for the processor */
  UCHAR ApicVersion;  /* Local APIC version */
  UCHAR CpuFlags;     /* CPU flags */
  ULONG CpuSignature; /* CPU signature */
  ULONG FeatureFlags; /* CPUID feature value */
  ULONG Reserved[2];  /* Reserved (0) */
}  MP_CONFIGURATION_PROCESSOR, *PMP_CONFIGURATION_PROCESSOR;



typedef struct  _MP_CONFIGURATION_BUS
{
	UCHAR Type;         /* 1 */
	UCHAR BusId;        /* Bus ID */
	CHAR BusType[6];   /* Bus type */
}  MP_CONFIGURATION_BUS, *PMP_CONFIGURATION_BUS;

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


typedef struct _MP_CONFIGURATION_IOAPIC
{
	UCHAR Type;         /* 2 */
	UCHAR ApicId;       /* I/O APIC ID */
	UCHAR ApicVersion;  /* I/O APIC version */
	UCHAR ApicFlags;    /* I/O APIC flags */
	ULONG ApicAddress;  /* I/O APIC base address */
} MP_CONFIGURATION_IOAPIC, *PMP_CONFIGURATION_IOAPIC;

#define MAX_IOAPIC  2

#define MP_IOAPIC_USABLE  0x01


typedef struct _MP_CONFIGURATION_INTSRC
{
	UCHAR Type;         /* 3 */
	UCHAR IrqType;      /* Interrupt type */
	USHORT IrqFlag;     /* Interrupt flags */
	UCHAR SrcBusId;     /* Source bus ID */
	UCHAR SrcBusIrq;    /* Source bus interrupt */
	UCHAR DstApicId;    /* Destination APIC ID */
	UCHAR DstApicInt;   /* Destination interrupt */
}  MP_CONFIGURATION_INTSRC, *PMP_CONFIGURATION_INTSRC;

#define MAX_IRQ_SOURCE  128

#define INT_VECTORED    0
#define INT_NMI         1
#define INT_SMI         2
#define INT_EXTINT      3

#define IRQDIR_DEFAULT  0
#define IRQDIR_HIGH     1
#define IRQDIR_LOW      3


typedef struct _MP_CONFIGURATION_INTLOCAL
{
	UCHAR Type;         /* 4 */
	UCHAR IrqType;      /* Interrupt type */
	USHORT IrqFlag;     /* Interrupt flags */
	UCHAR SrcBusId;     /* Source bus ID */
	UCHAR SrcBusIrq;    /* Source bus interrupt */
	UCHAR DstApicId;    /* Destination local APIC ID */
	UCHAR DstApicLInt;  /* Destination local APIC interrupt */
} MP_CONFIGURATION_INTLOCAL, *PMP_CONFIGURATION_INTLOCAL;
#include <poppack.h>

#define MP_APIC_ALL	0xFF
  
#define CPU_FLAG_ENABLED         1  /* Processor is available */
#define CPU_FLAG_BSP             2  /* Processor is the bootstrap processor */

#define CPU_STEPPING_MASK  0x0F
#define CPU_MODEL_MASK	   0xF0
#define CPU_FAMILY_MASK	   0xF00

#define PIC_IRQS  16

/* Prototypes */

VOID HalpInitMPS(VOID);


#endif /* __INCLUDE_HAL_MPS */

/* EOF */
