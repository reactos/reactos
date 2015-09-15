/*
 *
 */

#pragma once

#if defined(__GNUC__) && !defined(_MINIHAL_)
#define INIT_SECTION __attribute__((section ("INIT")))
#else
#define INIT_SECTION /* Done via alloc_text for MSC */
#endif


#ifdef CONFIG_SMP
#define HAL_BUILD_TYPE (DBG ? PRCB_BUILD_DEBUG : 0)
#else
#define HAL_BUILD_TYPE ((DBG ? PRCB_BUILD_DEBUG : 0) | PRCB_BUILD_UNIPROCESSOR)
#endif

typedef struct _HAL_BIOS_FRAME
{
    ULONG SegSs;
    ULONG Esp;
    ULONG EFlags;
    ULONG SegCs;
    ULONG Eip;
    PKTRAP_FRAME TrapFrame;
    ULONG CsLimit;
    ULONG CsBase;
    ULONG CsFlags;
    ULONG SsLimit;
    ULONG SsBase;
    ULONG SsFlags;
    ULONG Prefix;
} HAL_BIOS_FRAME, *PHAL_BIOS_FRAME;

typedef
VOID
(__cdecl *PHAL_SW_INTERRUPT_HANDLER)(
    VOID
);

typedef
VOID
(FASTCALL *PHAL_SW_INTERRUPT_HANDLER_2ND_ENTRY)(
    IN PKTRAP_FRAME TrapFrame
);

#define HAL_APC_REQUEST         0
#define HAL_DPC_REQUEST         1

/* CMOS Registers and Ports */
#define CMOS_CONTROL_PORT       (PUCHAR)0x70
#define CMOS_DATA_PORT          (PUCHAR)0x71
#define RTC_REGISTER_A          0x0A
#define   RTC_REG_A_UIP         0x80
#define RTC_REGISTER_B          0x0B
#define   RTC_REG_B_PI          0x40
#define RTC_REGISTER_C          0x0C
#define   RTC_REG_C_IRQ         0x80
#define RTC_REGISTER_D          0x0D
#define RTC_REGISTER_CENTURY    0x32

/* Usage flags */
#define IDT_REGISTERED          0x01
#define IDT_LATCHED             0x02
#define IDT_READ_ONLY           0x04
#define IDT_INTERNAL            0x11
#define IDT_DEVICE              0x21

/* Conversion functions */
#define BCD_INT(bcd)            \
    (((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F))
#define INT_BCD(int)            \
    (UCHAR)(((int / 10) << 4) + (int % 10))

//
// BIOS Interrupts
//
#define VIDEO_SERVICES   0x10

//
// Operations for INT 10h (in AH)
//
#define SET_VIDEO_MODE   0x00

//
// Video Modes for INT10h AH=00 (in AL)
//
#define GRAPHICS_MODE_12 0x12           /* 80x30	 8x16  640x480	 16/256K */

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
        UCHAR OperatingMode:3;
        UCHAR AccessMode:2;
        UCHAR Channel:2;
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

typedef enum _I8259_READ_REQUEST
{
    InvalidRequest,
    InvalidRequest2,
    ReadIdr,
    ReadIsr
} I8259_READ_REQUEST;

typedef enum _I8259_EOI_MODE
{
    RotateAutoEoiClear,
    NonSpecificEoi,
    InvalidEoiMode,
    SpecificEoi,
    RotateAutoEoiSet,
    RotateNonSpecific,
    SetPriority,
    RotateSpecific
} I8259_EOI_MODE;

//
// Definitions for ICW Registers
//
typedef union _I8259_ICW1
{
    struct
    {
        UCHAR NeedIcw4:1;
        UCHAR OperatingMode:1;
        UCHAR Interval:1;
        UCHAR InterruptMode:1;
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
        UCHAR SystemMode:1;
        UCHAR EoiMode:1;
        UCHAR BufferedMode:2;
        UCHAR SpecialFullyNestedMode:1;
        UCHAR Reserved:3;
    };
    UCHAR Bits;
} I8259_ICW4, *PI8259_ICW4;

typedef union _I8259_OCW2
{
    struct
    {
        UCHAR IrqNumber:3;
        UCHAR Sbz:2;
        UCHAR EoiMode:3;
    };
    UCHAR Bits;
} I8259_OCW2, *PI8259_OCW2;

typedef union _I8259_OCW3
{
    struct
    {
        UCHAR ReadRequest:2;
        UCHAR PollCommand:1;
        UCHAR Sbo:1;
        UCHAR Sbz:1;
        UCHAR SpecialMaskMode:2;
        UCHAR Reserved:1;
    };
    UCHAR Bits;
} I8259_OCW3, *PI8259_OCW3;

typedef union _I8259_ISR
{
    union
    {
        struct
        {
            UCHAR Irq0:1;
            UCHAR Irq1:1;
            UCHAR Irq2:1;
            UCHAR Irq3:1;
            UCHAR Irq4:1;
            UCHAR Irq5:1;
            UCHAR Irq6:1;
            UCHAR Irq7:1;
        };
    };
    UCHAR Bits;
} I8259_ISR, *PI8259_ISR;

typedef I8259_ISR I8259_IDR, *PI8259_IDR;

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

typedef
BOOLEAN
(NTAPI *PHAL_DISMISS_INTERRUPT)(
    IN KIRQL Irql,
    IN ULONG Irq,
    OUT PKIRQL OldIrql
);

BOOLEAN
NTAPI
HalpDismissIrqGeneric(
    IN KIRQL Irql,
    IN ULONG Irq,
    OUT PKIRQL OldIrql
);

BOOLEAN
NTAPI
HalpDismissIrq15(
    IN KIRQL Irql,
    IN ULONG Irq,
    OUT PKIRQL OldIrql
);

BOOLEAN
NTAPI
HalpDismissIrq13(
    IN KIRQL Irql,
    IN ULONG Irq,
    OUT PKIRQL OldIrql
);

BOOLEAN
NTAPI
HalpDismissIrq07(
    IN KIRQL Irql,
    IN ULONG Irq,
    OUT PKIRQL OldIrql
);

BOOLEAN
NTAPI
HalpDismissIrqLevel(
    IN KIRQL Irql,
    IN ULONG Irq,
    OUT PKIRQL OldIrql
);

BOOLEAN
NTAPI
HalpDismissIrq15Level(
    IN KIRQL Irql,
    IN ULONG Irq,
    OUT PKIRQL OldIrql
);

BOOLEAN
NTAPI
HalpDismissIrq13Level(
    IN KIRQL Irql,
    IN ULONG Irq,
    OUT PKIRQL OldIrql
);

BOOLEAN
NTAPI
HalpDismissIrq07Level(
    IN KIRQL Irql,
    IN ULONG Irq,
    OUT PKIRQL OldIrql
);

VOID
__cdecl
HalpHardwareInterruptLevel(
    VOID
);

//
// Hack Flags
//
#define HALP_REVISION_FROM_HACK_FLAGS(x)    ((x) >> 24)
#define HALP_REVISION_HACK_FLAGS(x)         ((x) >> 12)
#define HALP_HACK_FLAGS(x)                  ((x) & 0xFFF)

//
// Feature flags
//
#define HALP_CARD_FEATURE_FULL_DECODE   0x0001

//
// Match Flags
//
#define HALP_CHECK_CARD_REVISION_ID     0x10000
#define HALP_CHECK_CARD_SUBVENDOR_ID    0x20000
#define HALP_CHECK_CARD_SUBSYSTEM_ID    0x40000

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
VOID __cdecl HalpApcInterrupt(VOID);
VOID __cdecl HalpDispatchInterrupt(VOID);
VOID __cdecl HalpDispatchInterrupt2(VOID);
DECLSPEC_NORETURN VOID FASTCALL HalpApcInterrupt2ndEntry(IN PKTRAP_FRAME TrapFrame);
DECLSPEC_NORETURN VOID FASTCALL HalpDispatchInterrupt2ndEntry(IN PKTRAP_FRAME TrapFrame);

/* profil.c */
extern BOOLEAN HalpProfilingStopped;

/* timer.c */
VOID NTAPI HalpInitializeClock(VOID);
VOID __cdecl HalpClockInterrupt(VOID);
VOID __cdecl HalpProfileInterrupt(VOID);

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
ULONG_PTR
NTAPI
HalpAllocPhysicalMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG_PTR MaxAddress,
    IN PFN_NUMBER PageCount,
    IN BOOLEAN Aligned
);

PVOID
NTAPI
HalpMapPhysicalMemory64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN PFN_COUNT PageCount
);

VOID
NTAPI
HalpUnmapVirtualAddress(
    IN PVOID VirtualAddress,
    IN PFN_COUNT NumberPages
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
FASTCALL
HalpExitToV86(
    PKTRAP_FRAME TrapFrame
);

VOID
__cdecl
HalpRealModeStart(
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
// CMOS Routines
//
VOID
NTAPI
HalpInitializeCmos(
    VOID
);

UCHAR
NTAPI
HalpReadCmos(
    IN UCHAR Reg
);

VOID
NTAPI
HalpWriteCmos(
    IN UCHAR Reg,
    IN UCHAR Value
);

//
// Spinlock for protecting CMOS access
//
VOID
NTAPI
HalpAcquireCmosSpinLock(
    VOID
);

VOID
NTAPI
HalpReleaseCmosSpinLock(
    VOID
);

NTSTATUS
NTAPI
HalpOpenRegistryKey(
    IN PHANDLE KeyHandle,
    IN HANDLE RootKey,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
);

VOID
NTAPI
HalpGetNMICrashFlag(
    VOID
);

BOOLEAN
NTAPI
HalpGetDebugPortTable(
    VOID
);

VOID
NTAPI
HalpReportSerialNumber(
    VOID
);

NTSTATUS
NTAPI
HalpMarkAcpiHal(
    VOID
);

VOID
NTAPI
HalpBuildAddressMap(
    VOID
);

VOID
NTAPI
HalpReportResourceUsage(
    IN PUNICODE_STRING HalName,
    IN INTERFACE_TYPE InterfaceType
);

ULONG
NTAPI
HalpIs16BitPortDecodeSupported(
    VOID
);

NTSTATUS
NTAPI
HalpQueryAcpiResourceRequirements(
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *Requirements
);

VOID
FASTCALL
KeUpdateSystemTime(
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG Increment,
    IN KIRQL OldIrql
);

VOID
NTAPI
HalpInitBusHandlers(
    VOID
);

NTSTATUS
NTAPI
HaliInitPnpDriver(
    VOID
);

VOID
NTAPI
HalpDebugPciDumpBus(
    IN ULONG i,
    IN ULONG j,
    IN ULONG k,
    IN PPCI_COMMON_CONFIG PciData
);

VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

#ifdef _M_AMD64
#define KfLowerIrql KeLowerIrql
#define KiEnterInterruptTrap(TrapFrame) /* We do all neccessary in asm code */
#define KiEoiHelper(TrapFrame) return /* Just return to the caller */
#define HalBeginSystemInterrupt(Irql, Vector, OldIrql) ((*(OldIrql) = PASSIVE_LEVEL), TRUE)
#ifndef CONFIG_SMP
/* On UP builds, spinlocks don't exist at IRQL >= DISPATCH */
#define KiAcquireSpinLock(SpinLock)
#define KiReleaseSpinLock(SpinLock)
#define KfAcquireSpinLock(SpinLock) KfRaiseIrql(DISPATCH_LEVEL);
#define KfReleaseSpinLock(SpinLock, OldIrql) KeLowerIrql(OldIrql);
#endif // !CONFIG_SMP
#endif // _M_AMD64

extern BOOLEAN HalpNMIInProgress;

extern ADDRESS_USAGE HalpDefaultIoSpace;

extern KSPIN_LOCK HalpSystemHardwareLock;

extern PADDRESS_USAGE HalpAddressUsageList;

extern LARGE_INTEGER HalpPerfCounter;

extern KAFFINITY HalpActiveProcessors;

extern BOOLEAN HalDisableFirmwareMapper;
extern PWCHAR HalHardwareIdString;
extern PWCHAR HalName;

extern KAFFINITY HalpDefaultInterruptAffinity;

extern IDTUsageFlags HalpIDTUsageFlags[MAXIMUM_IDTVECTOR+1];

extern const USHORT HalpBuildType;
