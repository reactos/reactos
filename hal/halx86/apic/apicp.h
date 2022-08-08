/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Header file for APIC hal
 * COPYRIGHT:   Copyright 2011 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#ifdef _M_AMD64
    #define LOCAL_APIC_BASE 0xFFFFFFFFFFFE0000ULL
    #define IOAPIC_BASE 0xFFFFFFFFFFFE1000ULL

    /* Vectors */
    #define APC_VECTOR           0x1F // IRQL 01 (APC_LEVEL) - KiApcInterrupt
    #define DISPATCH_VECTOR      0x2F // IRQL 02 (DISPATCH_LEVEL) - KiDpcInterrupt
    #define CMCI_VECTOR          0x35 // IRQL 05 (CMCI_LEVEL) - HalpInterruptCmciService
    #define APIC_CLOCK_VECTOR    0xD1 // IRQL 13 (CLOCK_LEVEL), IRQ 8 - HalpTimerClockInterrupt
    #define CLOCK_IPI_VECTOR     0xD2 // IRQL 13 (CLOCK_LEVEL) - HalpTimerClockIpiRoutine
    #define REBOOT_VECTOR        0xD7 // IRQL 15 (PROFILE_LEVEL) - HalpInterruptRebootService
    #define STUB_VECTOR          0xD8 // IRQL 15 (PROFILE_LEVEL) - HalpInterruptStubService
    #define APIC_SPURIOUS_VECTOR 0xDF // IRQL 13 (CLOCK_LEVEL) - HalpInterruptSpuriousService
    #define APIC_IPI_VECTOR      0xE1 // IRQL 14 (IPI_LEVEL) - KiIpiInterrupt
    #define APIC_ERROR_VECTOR    0xE2 // IRQL 14 (IPI_LEVEL) - HalpInterruptLocalErrorService
    #define POWERFAIL_VECTOR     0xE3 // IRQL 14 (POWER_LEVEL) : HalpInterruptDeferredRecoveryService
    #define APIC_PROFILE_VECTOR  0xFD // IRQL 15 (PROFILE_LEVEL) - HalpTimerProfileInterrupt
    #define APIC_PERF_VECTOR     0xFE // IRQL 15 (PROFILE_LEVEL) - HalpPerfInterrupt
    #define APIC_NMI_VECTOR      0xFF

    #define IrqlToTpr(Irql) (Irql << 4)
    #define IrqlToSoftVector(Irql) ((Irql << 4)|0xf)
    #define TprToIrql(Tpr) ((KIRQL)(Tpr >> 4))
    #define CLOCK2_LEVEL CLOCK_LEVEL
#else
    #define LOCAL_APIC_BASE  0xFFFE0000
    #define IOAPIC_BASE 0xFFFE1000

    /* Vectors */
    #define APIC_SPURIOUS_VECTOR 0x1f
    #define APC_VECTOR           0x3D // IRQL 01
    #define DISPATCH_VECTOR      0x41 // IRQL 02
    #define APIC_GENERIC_VECTOR  0xC1 // IRQL 27
    #define APIC_CLOCK_VECTOR    0xD1 // IRQL 28
    #define APIC_SYNCH_VECTOR    0xD1 // IRQL 28
    #define APIC_IPI_VECTOR      0xE1 // IRQL 29
    #define APIC_ERROR_VECTOR    0xE3
    #define POWERFAIL_VECTOR     0xEF // IRQL 30
    #define APIC_PROFILE_VECTOR  0xFD // IRQL 31
    #define APIC_PERF_VECTOR     0xFE
    #define APIC_NMI_VECTOR      0xFF

    #define IrqlToTpr(Irql) (HalpIRQLtoTPR[Irql])
    #define IrqlToSoftVector(Irql) IrqlToTpr(Irql)
    #define TprToIrql(Tpr)  (HalVectorToIRQL[Tpr >> 4])
#endif

#define APIC_MAX_IRQ 24
#define APIC_FREE_VECTOR 0xFF
#define APIC_RESERVED_VECTOR 0xFE

/* The IMCR is supported by two read/writable or write-only I/O ports,
   22h and 23h, which receive address and data respectively.
   To access the IMCR, write a value of 70h to I/O port 22h, which selects the IMCR.
   Then write the data to I/O port 23h. The power-on default value is zero,
   which connects the NMI and 8259 INTR lines directly to the BSP.
   Writing a value of 01h forces the NMI and 8259 INTR signals to pass through the APIC.
*/
#define IMCR_ADDRESS_PORT  (PUCHAR)0x0022
#define IMCR_DATA_PORT     (PUCHAR)0x0023
#define IMCR_SELECT        0x70
#define IMCR_PIC_DIRECT    0x00
#define IMCR_PIC_VIA_APIC  0x01


/* APIC Register Address Map */
typedef enum _APIC_REGISTER
{
    APIC_ID       = 0x0020, /* Local APIC ID Register (R/W) */
    APIC_VER      = 0x0030, /* Local APIC Version Register (R) */
    APIC_TPR      = 0x0080, /* Task Priority Register (R/W) */
    APIC_APR      = 0x0090, /* Arbitration Priority Register (R) */
    APIC_PPR      = 0x00A0, /* Processor Priority Register (R) */
    APIC_EOI      = 0x00B0, /* EOI Register (W) */
    APIC_RRR      = 0x00C0, /* Remote Read Register () */
    APIC_LDR      = 0x00D0, /* Logical Destination Register (R/W) */
    APIC_DFR      = 0x00E0, /* Destination Format Register (0-27 R, 28-31 R/W) */
    APIC_SIVR     = 0x00F0, /* Spurious Interrupt Vector Register (0-3 R, 4-9 R/W) */
    APIC_ISR      = 0x0100, /* Interrupt Service Register 0-255 (R) */
    APIC_TMR      = 0x0180, /* Trigger Mode Register 0-255 (R) */
    APIC_IRR      = 0x0200, /* Interrupt Request Register 0-255 (r) */
    APIC_ESR      = 0x0280, /* Error Status Register (R) */
    APIC_ICR0     = 0x0300, /* Interrupt Command Register 0-31 (R/W) */
    APIC_ICR1     = 0x0310, /* Interrupt Command Register 32-63 (R/W) */
    APIC_TMRLVTR  = 0x0320, /* Timer Local Vector Table (R/W) */
    APIC_THRMLVTR = 0x0330, /* Thermal Local Vector Table */
    APIC_PCLVTR   = 0x0340, /* Performance Counter Local Vector Table (R/W) */
    APIC_LINT0    = 0x0350, /* LINT0 Local Vector Table (R/W) */
    APIC_LINT1    = 0x0360, /* LINT1 Local Vector Table (R/W) */
    APIC_ERRLVTR  = 0x0370, /* Error Local Vector Table (R/W) */
    APIC_TICR     = 0x0380, /* Initial Count Register for Timer (R/W) */
    APIC_TCCR     = 0x0390, /* Current Count Register for Timer (R) */
    APIC_TDCR     = 0x03E0, /* Timer Divide Configuration Register (R/W) */
    APIC_EAFR     = 0x0400, /* extended APIC Feature register (R/W) */
    APIC_EACR     = 0x0410, /* Extended APIC Control Register (R/W) */
    APIC_SEOI     = 0x0420, /* Specific End Of Interrupt Register (W) */
    APIC_EXT0LVTR = 0x0500, /* Extended Interrupt 0 Local Vector Table */
    APIC_EXT1LVTR = 0x0510, /* Extended Interrupt 1 Local Vector Table */
    APIC_EXT2LVTR = 0x0520, /* Extended Interrupt 2 Local Vector Table */
    APIC_EXT3LVTR = 0x0530  /* Extended Interrupt 3 Local Vector Table */
} APIC_REGISTER;

#define MSR_APIC_BASE 0x0000001B
#define IOAPIC_PHYS_BASE 0xFEC00000
#define APIC_CLOCK_INDEX 8
#define ApicLogicalId(Cpu) ((UCHAR)(1<< Cpu))

/* The following definitions are based on AMD documentation.
   They differ slightly in Intel documentation. */

/* Message Type (Intel: "Delivery Mode") */
typedef enum _APIC_MT
{
    APIC_MT_Fixed = 0,
    APIC_MT_LowestPriority = 1,
    APIC_MT_SMI = 2,
    APIC_MT_RemoteRead = 3,
    APIC_MT_NMI = 4,
    APIC_MT_INIT = 5,
    APIC_MT_Startup = 6,
    APIC_MT_ExtInt = 7,
} APIC_MT;

/* Trigger Mode */
typedef enum _APIC_TGM
{
    APIC_TGM_Edge,
    APIC_TGM_Level
} APIC_TGM;

/* Destination Mode */
typedef enum _APIC_DM
{
    APIC_DM_Physical,
    APIC_DM_Logical
} APIC_DM;

/* Destination Short Hand */
typedef enum _APIC_DSH
{
    APIC_DSH_Destination,
    APIC_DSH_Self,
    APIC_DSH_AllIncludingSelf,
    APIC_DSH_AllExclusingSelf
} APIC_DSH;

/* Write Constants */
typedef enum _APIC_DF
{
    APIC_DF_Flat = 0xFFFFFFFF,
    APIC_DF_Cluster = 0x0FFFFFFF
} APIC_DF;

/* Remote Read Status */
typedef enum _APIC_RRS
{
    APIC_RRS_Invalid = 0,
    APIC_RRS_Pending = 1,
    APIC_RRS_Done = 2
} APIC_RRS;

/* Timer Constants */
typedef enum _TIMER_DV
{
    TIMER_DV_DivideBy2 = 0,
    TIMER_DV_DivideBy4 = 1,
    TIMER_DV_DivideBy8 = 2,
    TIMER_DV_DivideBy16 = 3,
    TIMER_DV_DivideBy32 = 8,
    TIMER_DV_DivideBy64 = 9,
    TIMER_DV_DivideBy128 = 10,
    TIMER_DV_DivideBy1 = 11,
} TIMER_DV;

#include <pshpack1.h>
typedef union _APIC_BASE_ADRESS_REGISTER
{
    UINT64 LongLong;
    struct
    {
        UINT64 Reserved1:8;
        UINT64 BootStrapCPUCore:1;
        UINT64 Reserved2:2;
        UINT64 Enable:1;
        UINT64 BaseAddress:40;
        UINT64 ReservedMBZ:12;
    };
} APIC_BASE_ADRESS_REGISTER;

typedef union _APIC_SPURIOUS_INERRUPT_REGISTER
{
    UINT32 Long;
    struct
    {
        UINT32 Vector:8;
        UINT32 SoftwareEnable:1;
        UINT32 FocusCPUCoreChecking:1;
        UINT32 ReservedMBZ:22;
    };
} APIC_SPURIOUS_INERRUPT_REGISTER;

typedef union _APIC_VERSION_REGISTER
{
    UINT32 Long;
    struct
    {
        UINT32 Version:8;
        UINT32 ReservedMBZ:8;
        UINT32 MaxLVT:8;
        UINT32 ReservedMBZ1:7;
        UINT32 ExtRegSpacePresent:1;
    };
} APIC_VERSION_REGISTER;

typedef union _APIC_EXTENDED_CONTROL_REGISTER
{
    UINT32 Long;
    struct
    {
        UINT32 Version:1;
        UINT32 SEOIEnable:1;
        UINT32 ExtApicIdEnable:1;
        UINT32 ReservedMBZ:29;
    };
} APIC_EXTENDED_CONTROL_REGISTER;

typedef union _APIC_INTERRUPT_COMMAND_REGISTER
{
    UINT64 LongLong;
    struct
    {
        UINT32 Long0;
        UINT32 Long1;
    };
    struct
    {
        UINT64 Vector:8;
        UINT64 MessageType:3;
        UINT64 DestinationMode:1;
        UINT64 DeliveryStatus:1;
        UINT64 ReservedMBZ:1;
        UINT64 Level:1;
        UINT64 TriggerMode:1;
        UINT64 RemoteReadStatus:2; /* Intel: Reserved */
        UINT64 DestinationShortHand:2;
        UINT64 Reserved2MBZ:36;
        UINT64 Destination:8;
    };
} APIC_INTERRUPT_COMMAND_REGISTER;

typedef union _LVT_REGISTER
{
    UINT32 Long;
    struct
    {
        UINT32 Vector:8;
        UINT32 MessageType:3;
        UINT32 ReservedMBZ:1;
        UINT32 DeliveryStatus:1;
        UINT32 Reserved1MBZ:1;
        UINT32 RemoteIRR:1;
        UINT32 TriggerMode:1;
        UINT32 Mask:1;
        UINT32 TimerMode:1;
        UINT32 Reserved2MBZ:13;
    };
} LVT_REGISTER;

/* IOAPIC offsets */
#define IOAPIC_IOREGSEL 0x00
#define IOAPIC_IOWIN    0x10

/* IOAPIC Constants */
#define IOAPIC_ID     0x00
#define IOAPIC_VER    0x01
#define IOAPIC_ARB    0x02
#define IOAPIC_REDTBL 0x10

typedef union _IOAPIC_REDIRECTION_REGISTER
{
    UINT64 LongLong;
    struct
    {
        UINT32 Long0;
        UINT32 Long1;
    };
    struct
    {
        UINT64 Vector:8;
        UINT64 DeliveryMode:3;
        UINT64 DestinationMode:1;
        UINT64 DeliveryStatus:1;
        UINT64 Polarity:1;
        UINT64 RemoteIRR:1;
        UINT64 TriggerMode:1;
        UINT64 Mask:1;
        UINT64 Reserved:39;
        UINT64 Destination:8;
    };
} IOAPIC_REDIRECTION_REGISTER;
#include <poppack.h>

FORCEINLINE
ULONG
ApicRead(APIC_REGISTER Register)
{
    return READ_REGISTER_ULONG((PULONG)(APIC_BASE + Register));
}

FORCEINLINE
VOID
ApicWrite(APIC_REGISTER Register, ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)(APIC_BASE + Register), Value);
}

VOID
NTAPI
ApicInitializeTimer(ULONG Cpu);

VOID
NTAPI
HalInitializeProfiling(VOID);

VOID
NTAPI
HalpInitApicInfo(IN PLOADER_PARAMETER_BLOCK KeLoaderBlock);

VOID __cdecl ApicSpuriousService(VOID);
