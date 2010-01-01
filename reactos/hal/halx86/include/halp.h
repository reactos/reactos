/*
 *
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

#define HAL_APC_REQUEST         0
#define HAL_DPC_REQUEST         1

/* CMOS Registers and Ports */
#define CMOS_CONTROL_PORT       (PUCHAR)0x70
#define CMOS_DATA_PORT          (PUCHAR)0x71
#define RTC_REGISTER_A          0x0A
#define RTC_REGISTER_B          0x0B
#define RTC_REG_A_UIP           0x80
#define RTC_REGISTER_CENTURY    0x32

/* Timer Registers and Ports */
#define TIMER_CONTROL_PORT      0x43
#define TIMER_DATA_PORT0        0x40
#define TIMER_SC0               0
#define TIMER_BOTH              0x30
#define TIMER_MD2               0x4

/* Usage flags */
#define IDT_REGISTERED          0x01
#define IDT_LATCHED             0x02
#define IDT_INTERNAL            0x11
#define IDT_DEVICE              0x21

/* Conversion functions */
#define BCD_INT(bcd)            \
    (((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F))
#define INT_BCD(int)            \
    (UCHAR)(((int / 10) << 4) + (int % 10))

typedef struct _IDTUsageFlags
{
    UCHAR Flags;
} IDTUsageFlags;

typedef struct
{
    KIRQL Irql;
    UCHAR BusReleativeVector;
} IDTUsage;

typedef struct _HalAddressUsage
{
    struct _HalAddressUsage *Next;
    CM_RESOURCE_TYPE Type;
    UCHAR Flags;
    struct
    {
        ULONG Start;
        ULONG Length;
    } Element[];
} ADDRESS_USAGE, *PADDRESS_USAGE;

/* adapter.c */
PADAPTER_OBJECT NTAPI HalpAllocateAdapterEx(ULONG NumberOfMapRegisters,BOOLEAN IsMaster, BOOLEAN Dma32BitAddresses);

/* sysinfo.c */
VOID
NTAPI
HalpRegisterVector(IN UCHAR Flags,
                   IN ULONG BusVector,
                   IN ULONG SystemVector,
                   IN KIRQL Irql);

VOID
NTAPI
HalpEnableInterruptHandler(IN UCHAR Flags,
                           IN ULONG BusVector,
                           IN ULONG SystemVector,
                           IN KIRQL Irql,
                           IN PVOID Handler,
                           IN KINTERRUPT_MODE Mode);

/* irql.c */
VOID NTAPI HalpInitPICs(VOID);

/* udelay.c */
VOID NTAPI HalpInitializeClock(VOID);

VOID
NTAPI
HalpCalibrateStallExecution(VOID);

/* pci.c */
VOID HalpInitPciBus (VOID);

/* dma.c */
VOID HalpInitDma (VOID);

/* Non-generic initialization */
VOID HalpInitPhase0 (PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID HalpInitPhase1(VOID);
VOID NTAPI HalpClockInterrupt(VOID);
VOID NTAPI HalpProfileInterrupt(VOID);

//
// KD Support
//
VOID
NTAPI
HalpCheckPowerButton(
    VOID
);

VOID
NTAPI
HalpRegisterKdSupportFunctions(
    VOID
);

NTSTATUS
NTAPI
HalpSetupPciDeviceForDebugging(
    IN PVOID LoaderBlock,
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
);

NTSTATUS
NTAPI
HalpReleasePciDeviceForDebugging(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
);

//
// Memory routines
//
PVOID
NTAPI
HalpMapPhysicalMemory64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPage
);

VOID
NTAPI
HalpUnmapVirtualAddress(
    IN PVOID VirtualAddress,
    IN ULONG NumberPages
);

/* sysinfo.c */
NTSTATUS
NTAPI
HaliQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS InformationClass,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG ReturnedLength
);

NTSTATUS
NTAPI
HaliSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS InformationClass,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer
);

//
// BIOS Routines
//
BOOLEAN
NTAPI
HalpBiosDisplayReset(
    VOID
);

VOID
NTAPI
HalpBiosCall(
    VOID
);

VOID
NTAPI
HalpTrap0D(
    VOID
);

VOID
NTAPI
HalpTrap06(
    VOID
);

//
// Processor Halt Routine
//
VOID
NTAPI
HaliHaltSystem(
    VOID
);

//
// CMOS initialization
//
VOID
NTAPI
HalpInitializeCmos(
    VOID
);

//
// Spinlock for protecting CMOS access
//
VOID
NTAPI
HalpAcquireSystemHardwareSpinLock(
    VOID
);

VOID
NTAPI
HalpReleaseCmosSpinLock(
    VOID
);

#ifdef _M_AMD64
#define KfLowerIrql KeLowerIrql
#ifndef CONFIG_SMP
/* On UP builds, spinlocks don't exist at IRQL >= DISPATCH */
#define KiAcquireSpinLock(SpinLock)
#define KiReleaseSpinLock(SpinLock)
#define KfAcquireSpinLock(SpinLock) KfRaiseIrql(DISPATCH_LEVEL);
#define KfReleaseSpinLock(SpinLock, OldIrql) KeLowerIrql(OldIrql);
#endif // !CONFIG_SMP
#endif // _M_AMD64

extern PVOID HalpRealModeStart;
extern PVOID HalpRealModeEnd;

extern ADDRESS_USAGE HalpDefaultIoSpace;

extern KSPIN_LOCK HalpSystemHardwareLock;

extern PADDRESS_USAGE HalpAddressUsageList;

#endif /* __INTERNAL_HAL_HAL_H */
