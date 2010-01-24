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
// See ISA System Architecture 3rd Edition (Tom Shanley, Don Anderson, John Swindle)
// P. 396, 397
//
// These ports are controlled by the i8259 Programmable Interrupt Controller (PIC)
//
#define PIC1_CONTROL_PORT      0x20
#define PIC1_DATA_PORT         0x21
#define PIC2_CONTROL_PORT      0xA0
#define PIC2_DATA_PORT         0xA1

//
// Definitions for ICW/OCW Bits
//
typedef enum _I8259_ICW1_OPERATING_MODE
{
    Cascade,
    Single
} I8259_ICW1_OPERATING_MODE;

typedef enum _I8259_ICW1_INTERRUPT_MODE
{
    EdgeTriggered,
    LevelTriggered
} I8259_ICW1_INTERRUPT_MODE;

typedef enum _I8259_ICW1_INTERVAL
{
    Interval8,
    Interval4
} I8259_ICW1_INTERVAL;

typedef enum _I8259_ICW4_SYSTEM_MODE
{
    Mcs8085Mode,
    New8086Mode
} I8259_ICW4_SYSTEM_MODE;

typedef enum _I8259_ICW4_EOI_MODE
{
    NormalEoi,
    AutomaticEoi
} I8259_ICW4_EOI_MODE;

typedef enum _I8259_ICW4_BUFFERED_MODE
{
    NonBuffered,
    NonBuffered2,
    BufferedSlave,
    BufferedMaster
} I8259_ICW4_BUFFERED_MODE;

//
// Definitions for ICW Registers
//
typedef union _I8259_ICW1
{
    struct 
    {
        UCHAR NeedIcw4:1;
        I8259_ICW1_OPERATING_MODE OperatingMode:1;
        I8259_ICW1_INTERVAL Interval:1;
        I8259_ICW1_INTERRUPT_MODE InterruptMode:1;
        UCHAR Init:1;
        UCHAR InterruptVectorAddress:3;
    };
    UCHAR Bits;
} I8259_ICW1, *PI8259_ICW1;

typedef union _I8259_ICW2
{
    struct 
    {
        UCHAR Sbz:3;
        UCHAR InterruptVector:5;
    };
    UCHAR Bits;
} I8259_ICW2, *PI8259_ICW2;

typedef union _I8259_ICW3
{
    union
    {
        struct 
        {
            UCHAR SlaveIrq0:1;
            UCHAR SlaveIrq1:1;
            UCHAR SlaveIrq2:1;
            UCHAR SlaveIrq3:1;
            UCHAR SlaveIrq4:1;
            UCHAR SlaveIrq5:1;
            UCHAR SlaveIrq6:1;
            UCHAR SlaveIrq7:1;
        };
        struct 
        {
            UCHAR SlaveId:3;
            UCHAR Reserved:5;
        };
    };
    UCHAR Bits;
} I8259_ICW3, *PI8259_ICW3;

typedef union _I8259_ICW4
{
    struct 
    {
        I8259_ICW4_SYSTEM_MODE SystemMode:1;
        I8259_ICW4_EOI_MODE EoiMode:1;
        I8259_ICW4_BUFFERED_MODE BufferedMode:2;
        UCHAR SpecialFullyNestedMode:1;
        UCHAR Reserved:3;
    };
    UCHAR Bits;
} I8259_ICW4, *PI8259_ICW4;

//
// See EISA System Architecture 2nd Edition (Tom Shanley, Don Anderson, John Swindle)
// P. 34, 35
//
// These ports are controlled by the i8259A Programmable Interrupt Controller (PIC)
//
#define EISA_ELCR_MASTER       0x4D0
#define EISA_ELCR_SLAVE        0x4D1

typedef union _EISA_ELCR
{
    struct
    {
        struct
        {
            UCHAR Irq0Level:1;
            UCHAR Irq1Level:1;
            UCHAR Irq2Level:1;
            UCHAR Irq3Level:1;
            UCHAR Irq4Level:1;
            UCHAR Irq5Level:1;
            UCHAR Irq6Level:1;
            UCHAR Irq7Level:1;
        } Master;
        struct
        {
            UCHAR Irq8Level:1;
            UCHAR Irq9Level:1;
            UCHAR Irq10Level:1;
            UCHAR Irq11Level:1;
            UCHAR Irq12Level:1;
            UCHAR Irq13Level:1;
            UCHAR Irq14Level:1;
            UCHAR Irq15Level:1;
        } Slave;
    };
    USHORT Bits;
} EISA_ELCR, *PEISA_ELCR;

typedef struct _PIC_MASK
{
    union
    {
        struct
        {
            UCHAR Master;
            UCHAR Slave;
        };
        USHORT Both;
    };    
} PIC_MASK, *PPIC_MASK;

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

/* pic.c */
VOID NTAPI HalpInitializePICs(IN BOOLEAN EnableInterrupts);

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
