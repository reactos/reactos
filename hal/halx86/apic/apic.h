
#ifdef _M_AMD64
#define IOAPIC_BASE 0xFFFFFFFFFFFE1000ULL // checkme
#define ZERO_VECTOR          0x00 // IRQL 00
#define APC_VECTOR           0x3D // IRQL 01
#define APIC_SPURIOUS_VECTOR 0x3f
#define DISPATCH_VECTOR      0x41 // IRQL 02
#define APIC_GENERIC_VECTOR  0xC1 // IRQL 27
#define APIC_CLOCK_VECTOR    0xD1 // IRQL 28
#define APIC_SYNCH_VECTOR    0xD1 // IRQL 28
#define APIC_IPI_VECTOR      0xE1 // IRQL 29
#define APIC_ERROR_VECTOR    0xE3
#define POWERFAIL_VECTOR     0xEF // IRQL 30
#define APIC_PROFILE_VECTOR  0xFD // IRQL 31
#define APIC_NMI_VECTOR      0xFF
#define IrqlToTpr(Irql) (Irql << 4)
#define IrqlToSoftVector(Irql) ((Irql << 4)|0xf)
#define TprToIrql(Tpr) ((KIRQL)(Tpr >> 4))
#define CLOCK2_LEVEL CLOCK_LEVEL
#else
#define IOAPIC_BASE 0xFFFE1000 // checkme
#define ZERO_VECTOR          0x00 // IRQL 00
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
#define APIC_NMI_VECTOR      0xFF
#define IrqlToTpr(Irql) (HalpIRQLtoTPR[Irql])
#define IrqlToSoftVector(Irql) IrqlToTpr(Irql)
#define TprToIrql(Tpr)  (HalVectorToIRQL[Tpr >> 4])
#endif

#define MSR_APIC_BASE 0x0000001B
#define IOAPIC_PHYS_BASE 0xFEC00000
#define APIC_CLOCK_INDEX 8

#define ApicLogicalId(Cpu) ((UCHAR)(1<< Cpu))

/* APIC Register Address Map */
#define APIC_ID       0x0020 /* Local APIC ID Register (R/W) */
#define APIC_VER      0x0030 /* Local APIC Version Register (R) */
#define APIC_TPR      0x0080 /* Task Priority Register (R/W) */
#define APIC_APR      0x0090 /* Arbitration Priority Register (R) */
#define APIC_PPR      0x00A0 /* Processor Priority Register (R) */
#define APIC_EOI      0x00B0 /* EOI Register (W) */
#define APIC_RRR      0x00C0 /* Remote Read Register () */
#define APIC_LDR      0x00D0 /* Logical Destination Register (R/W) */
#define APIC_DFR      0x00E0 /* Destination Format Register (0-27 R, 28-31 R/W) */
#define APIC_SIVR     0x00F0 /* Spurious Interrupt Vector Register (0-3 R, 4-9 R/W) */
#define APIC_ISR      0x0100 /* Interrupt Service Register 0-255 (R) */
#define APIC_TMR      0x0180 /* Trigger Mode Register 0-255 (R) */
#define APIC_IRR      0x0200 /* Interrupt Request Register 0-255 (r) */
#define APIC_ESR      0x0280 /* Error Status Register (R) */
#define APIC_ICR0     0x0300 /* Interrupt Command Register 0-31 (R/W) */
#define APIC_ICR1     0x0310 /* Interrupt Command Register 32-63 (R/W) */
#define APIC_TMRLVTR  0x0320 /* Timer Local Vector Table (R/W) */
#define	APIC_THRMLVTR 0x0330 /* Thermal Local Vector Table */
#define APIC_PCLVTR   0x0340 /* Performance Counter Local Vector Table (R/W) */
#define APIC_LINT0    0x0350 /* LINT0 Local Vector Table (R/W) */
#define APIC_LINT1    0x0360 /* LINT1 Local Vector Table (R/W) */
#define APIC_ERRLVTR  0x0370 /* Error Local Vector Table (R/W) */
#define APIC_TICR     0x0380 /* Initial Count Register for Timer (R/W) */
#define APIC_TCCR     0x0390 /* Current Count Register for Timer (R) */
#define APIC_TDCR     0x03E0 /* Timer Divide Configuration Register (R/W) */
#define APIC_EAFR     0x0400 /* extended APIC Feature register (R/W) */
#define APIC_EACR     0x0410 /* Extended APIC Control Register (R/W) */
#define APIC_SEOI     0x0420 /* Specific End Of Interrupt Register (W) */
#define APIC_EXT0LVTR 0x0500 /* Extended Interrupt 0 Local Vector Table */
#define APIC_EXT1LVTR 0x0510 /* Extended Interrupt 1 Local Vector Table */
#define APIC_EXT2LVTR 0x0520 /* Extended Interrupt 2 Local Vector Table */
#define APIC_EXT3LVTR 0x0530 /* Extended Interrupt 3 Local Vector Table */

enum
{
    APIC_MT_Fixed = 0,
    APIC_MT_LowestPriority = 1,
    APIC_MT_SMI = 2,
    APIC_MT_RemoteRead = 3,
    APIC_MT_NMI = 4,
    APIC_MT_INIT = 5,
    APIC_MT_Startup = 6,
    APIC_MT_ExtInt = 7,
};

enum
{
    APIC_TGM_Edge,
    APIC_TGM_Level
};

enum
{
    APIC_DM_Physical,
    APIC_DM_Logical
};

enum
{
    APIC_DSH_Destination,
    APIC_DSH_Self,
    APIC_DSH_AllIncludingSelf,
    APIC_DSH_AllExclusingSelf
};

enum
{
    APIC_DF_Flat = 0xFFFFFFFF,
    APIC_DF_Cluster = 0x0FFFFFFF
};

enum
{
    TIMER_DV_DivideBy2 = 0,
    TIMER_DV_DivideBy4 = 1,
    TIMER_DV_DivideBy8 = 2,
    TIMER_DV_DivideBy16 = 3,
    TIMER_DV_DivideBy32 = 8,
    TIMER_DV_DivideBy64 = 9,
    TIMER_DV_DivideBy128 = 10,
    TIMER_DV_DivideBy1 = 11,
};


typedef union _APIC_BASE_ADRESS_REGISTER
{
    ULONG64 Long;
    struct
    {
        ULONG64 Reserved1:8;
        ULONG64 BootStrapCPUCore:1;
        ULONG64 Reserved2:2;
        ULONG64 Enable:1;
        ULONG64 BaseAddress:40;
        ULONG64 ReservedMBZ:12;
    };
} APIC_BASE_ADRESS_REGISTER;

typedef union _APIC_SPURIOUS_INERRUPT_REGISTER
{
    ULONG Long;
    struct
    {
        ULONG Vector:8;
        ULONG SoftwareEnable:1;
        ULONG FocusCPUCoreChecking:1;
        ULONG ReservedMBZ:22;
    };
} APIC_SPURIOUS_INERRUPT_REGISTER;

typedef union
{
    ULONG Long;
    struct
    {
        ULONG Version:8;
        ULONG ReservedMBZ:8;
        ULONG MaxLVT:8;
        ULONG ReservedMBZ1:7;
        ULONG ExtRegSpacePresent:1;
    };
} APIC_VERSION_REGISTER;

typedef union
{
    ULONG Long;
    struct
    {
        ULONG Version:1;
        ULONG SEOIEnable:1;
        ULONG ExtApicIdEnable:1;
        ULONG ReservedMBZ:29;
    };
} APIC_EXTENDED_CONTROL_REGISTER;

typedef union _APIC_COMMAND_REGISTER
{
    ULONGLONG LongLong;
    struct
    {
        ULONG Long0;
        ULONG Long1;
    };
    struct
    {
        ULONGLONG Vector:8;
        ULONGLONG MessageType:3;
        ULONGLONG DestinationMode:1;
        ULONGLONG DeliveryStatus:1;
        ULONGLONG ReservedMBZ:1;
        ULONGLONG Level:1;
        ULONGLONG TriggerMode:1;
        ULONGLONG RemoteReadStatus:2;
        ULONGLONG DestinationShortHand:2;
        ULONGLONG Reserved2MBZ:36;
        ULONGLONG Destination:8;
    };
} APIC_COMMAND_REGISTER;

typedef union
{
    ULONG Long;
    struct
    {
        ULONG Vector:8;
        ULONG MessageType:3;
        ULONG ReservedMBZ:1;
        ULONG DeliveryStatus:1;
        ULONG Reserved1MBZ:1;
        ULONG RemoteIRR:1;
        ULONG TriggerMode:1;
        ULONG Mask:1;
        ULONG TimerMode:1;
        ULONG Reserved2MBZ:13;
    };
} LVT_REGISTER;


enum
{
    IOAPIC_IOREGSEL = 0x00,
    IOAPIC_IOWIN    = 0x10
};

enum
{
    IOAPIC_ID  = 0x00,
    IOAPIC_VER = 0x01,
    IOAPIC_ARB = 0x02,
    IOAPIC_REDTBL = 0x10
};

typedef union _IOAPIC_REDIRECTION_REGISTER
{
    ULONGLONG LongLong;
    struct
    {
        ULONG Long0;
        ULONG Long1;
    };
    struct
    {
        ULONGLONG Vector:8;
        ULONGLONG DeliveryMode:3;
        ULONGLONG DestinationMode:1;
        ULONGLONG DeliveryStatus:1;
        ULONGLONG Polarity:1;
        ULONGLONG RemoteIRR:1;
        ULONGLONG TriggerMode:1;
        ULONGLONG Mask:1;
        ULONGLONG Reserved:39;
        ULONGLONG Destination:8;
    };
} IOAPIC_REDIRECTION_REGISTER;

FORCEINLINE
ULONG
ApicRead(ULONG Offset)
{
    return *(volatile ULONG *)(APIC_BASE + Offset);
}

FORCEINLINE
VOID
ApicWrite(ULONG Offset, ULONG Value)
{
    *(volatile ULONG *)(APIC_BASE + Offset) = Value;
}

VOID
NTAPI
ApicInitializeTimer(ULONG Cpu);

VOID __cdecl ApicSpuriousService(VOID);

