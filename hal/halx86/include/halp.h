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

//
// Commonly stated as being 1.19318MHz
//
// See ISA System Architecture 3rd Edition (Tom Shanley, Don Anderson, John Swindle)
// P. 471
//
// However, the true value is closer to 1.19318181[...]81MHz since this is 1/3rd
// of the NTSC color subcarrier frequency which runs at 3.57954545[...]45MHz.
//
// Note that Windows uses 1.193167MHz which seems to have no basis. However, if
// one takes the NTSC color subcarrier frequency as being 3.579545 (trimming the
// infinite series) and divides it by three, one obtains 1.19318167.
//
// It may be that the original NT HAL source code introduced a typo and turned
// 119318167 into 1193167 by ommitting the "18". This is very plausible as the
// number is quite long.
//
#define PIT_FREQUENCY 1193182

//
// These ports are controlled by the i8254 Programmable Interrupt Timer (PIT)
//
#define TIMER_CHANNEL0_DATA_PORT 0x40
#define TIMER_CHANNEL1_DATA_PORT 0x41
#define TIMER_CHANNEL2_DATA_PORT 0x42
#define TIMER_CONTROL_PORT       0x43

//
// Mode 0 - Interrupt On Terminal Count
// Mode 1 - Hardware Re-triggerable One-Shot
// Mode 2 - Rate Generator
// Mode 3 - Square Wave Generator
// Mode 4 - Software Triggered Strobe
// Mode 5 - Hardware Triggered Strobe
//
typedef enum _TIMER_OPERATING_MODES
{
    PitOperatingMode0,
    PitOperatingMode1,
    PitOperatingMode2,
    PitOperatingMode3,
    PitOperatingMode4,
    PitOperatingMode5,
    PitOperatingMode2Reserved,
    PitOperatingMode5Reserved
} TIMER_OPERATING_MODES;

typedef enum _TIMER_ACCESS_MODES
{
    PitAccessModeCounterLatch,
    PitAccessModeLow,
    PitAccessModeHigh,
    PitAccessModeLowHigh
} TIMER_ACCESS_MODES;

typedef enum _TIMER_CHANNELS
{
    PitChannel0,
    PitChannel1,
    PitChannel2,
    PitReadBack
} TIMER_CHANNELS;

typedef union _TIMER_CONTROL_PORT_REGISTER
{
    struct 
    {
        UCHAR BcdMode:1;
        TIMER_OPERATING_MODES OperatingMode:3;
        TIMER_ACCESS_MODES AccessMode:2;
        TIMER_CHANNELS Channel:2;
    };
    UCHAR Bits;
} TIMER_CONTROL_PORT_REGISTER, *PTIMER_CONTROL_PORT_REGISTER;

//
// See ISA System Architecture 3rd Edition (Tom Shanley, Don Anderson, John Swindle)
// P. 400
//
// This port is controled by the i8255 Programmable Peripheral Interface (PPI)
//
#define SYSTEM_CONTROL_PORT_A   0x92
#define SYSTEM_CONTROL_PORT_B   0x61
typedef union _SYSTEM_CONTROL_PORT_B_REGISTER
{
    struct 
    {
        UCHAR Timer2GateToSpeaker:1;
        UCHAR SpeakerDataEnable:1;
        UCHAR ParityCheckEnable:1;
        UCHAR ChannelCheckEnable:1;
        UCHAR RefreshRequest:1;
        UCHAR Timer2Output:1;
        UCHAR ChannelCheck:1;
        UCHAR ParityCheck:1;
    };
    UCHAR Bits;
} SYSTEM_CONTROL_PORT_B_REGISTER, *PSYSTEM_CONTROL_PORT_B_REGISTER;

//
// Mm PTE/PDE to Hal PTE/PDE
//
#define HalAddressToPde(x) (PHARDWARE_PTE)MiAddressToPde(x)
#define HalAddressToPte(x) (PHARDWARE_PTE)MiAddressToPte(x)
    
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

VOID
NTAPI
HalpFlushTLB(VOID);

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

extern BOOLEAN HalpNMIInProgress;

extern PVOID HalpRealModeStart;
extern PVOID HalpRealModeEnd;

extern ADDRESS_USAGE HalpDefaultIoSpace;

extern KSPIN_LOCK HalpSystemHardwareLock;

extern PADDRESS_USAGE HalpAddressUsageList;

#endif /* __INTERNAL_HAL_HAL_H */
