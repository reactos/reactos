

#ifndef _ARM64_KETYPES_H
#define _ARM64_KETYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Interrupt request levels */
#define PASSIVE_LEVEL           0
#define LOW_LEVEL               0
#define APC_LEVEL               1
#define DISPATCH_LEVEL          2
#define CMCI_LEVEL              5
#define CLOCK_LEVEL             13
#define IPI_LEVEL               14
#define DRS_LEVEL               14
#define POWER_LEVEL             14
#define PROFILE_LEVEL           15
#define HIGH_LEVEL              15

//
// IPI Types
//
#define IPI_APC                 1
#define IPI_DPC                 2
#define IPI_FREEZE              4
#define IPI_PACKET_READY        6
#define IPI_SYNCH_REQUEST       16

//
// PRCB Flags
//
#define PRCB_MAJOR_VERSION      1
#define PRCB_BUILD_DEBUG        1
#define PRCB_BUILD_UNIPROCESSOR 2

//
// No LDTs on ARM64
//
#define LDT_ENTRY              ULONG


//
// HAL Variables
//
#define INITIAL_STALL_COUNT     100
#define MM_HAL_VA_START         0xFFFFFFFFFFC00000ULL
#define MM_HAL_VA_END           0xFFFFFFFFFFFFFFFFULL

//
// Structure for CPUID info
//
typedef union _CPU_INFO
{
    ULONG dummy;
} CPU_INFO, *PCPU_INFO;

typedef struct _KTRAP_FRAME
{
    UCHAR ExceptionActive;
    UCHAR ContextFromKFramesUnwound;
    UCHAR DebugRegistersValid;
    union
    {
        struct
        {
            CHAR PreviousMode;
            UCHAR PreviousIrql;
        };
    };
    ULONG Reserved;
    union
    {
        struct
        {
            ULONG64 FaultAddress;
            ULONG64 TrapFrame;
        };
    };
    //struct PKARM64_VFP_STATE VfpState;
    ULONG VfpState;
    ULONG Bcr[8];
    ULONG64 Bvr[8];
    ULONG Wcr[2];
    ULONG64 Wvr[2];
    ULONG Spsr;
    ULONG Esr;
    ULONG64 Sp;
    union
    {
        ULONG64 X[19];
        struct
        {
            ULONG64 X0;
            ULONG64 X1;
            ULONG64 X2;
            ULONG64 X3;
            ULONG64 X4;
            ULONG64 X5;
            ULONG64 X6;
            ULONG64 X7;
            ULONG64 X8;
            ULONG64 X9;
            ULONG64 X10;
            ULONG64 X11;
            ULONG64 X12;
            ULONG64 X13;
            ULONG64 X14;
            ULONG64 X15;
            ULONG64 X16;
            ULONG64 X17;
            ULONG64 X18;
        };
    };
    ULONG64 Lr;
    ULONG64 Fp;
    ULONG64 Pc;
} KTRAP_FRAME, *PKTRAP_FRAME;

typedef struct _KEXCEPTION_FRAME
{
    ULONG dummy;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

#ifndef NTOS_MODE_USER

typedef struct _TRAPFRAME_LOG_ENTRY
{
    ULONG64 Thread;
    UCHAR CpuNumber;
    UCHAR TrapType;
    USHORT Padding;
    ULONG Cpsrl;
    ULONG64 X0;
    ULONG64 X1;
    ULONG64 X2;
    ULONG64 X3;
    ULONG64 X4;
    ULONG64 X5;
    ULONG64 X6;
    ULONG64 X7;
    ULONG64 Fp;
    ULONG64 Lr;
    ULONG64 Sp;
    ULONG64 Pc;
    ULONG64 Far;
    ULONG Esr;
    ULONG Reserved1;
} TRAPFRAME_LOG_ENTRY, *PTRAPFRAME_LOG_ENTRY;

//
// Processor Region Control Block
// Based on WoA
//
typedef struct _KPRCB
{
    ULONG dummy;
} KPRCB, *PKPRCB;

//
// Processor Control Region
// Based on WoA
//
typedef struct _KIPCR
{
    union
    {
        struct
        {
            ULONG TibPad0[2];
            PVOID Spare1;
            struct _KPCR *Self;
            PVOID  PcrReserved0;
            struct _KSPIN_LOCK_QUEUE* LockArray;
            PVOID Used_Self;
        };
    };
    KIRQL CurrentIrql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR Pad1[2];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    ULONG SecondLevelCacheSize;
    struct
    {
        UCHAR ApcInterrupt;
        UCHAR DispatchInterrupt;
    };
    USHORT InterruptPad;
    UCHAR BtiMitigation;
    struct
    {
        UCHAR SsbMitigationFirmware:1;
        UCHAR SsbMitigationDynamic:1;
        UCHAR SsbMitigationKernel:1;
        UCHAR SsbMitigationUser:1;
        UCHAR SsbMitigationReserved:4;
    };
    UCHAR Pad2[2];
    ULONG64 PanicStorage[6];
    PVOID KdVersionBlock;
    PVOID HalReserved[134];
    PVOID KvaUserModeTtbr1;

    /* Private members, not in ntddk.h */
    PVOID Idt[256];
    PVOID* IdtExt;
    PVOID PcrAlign[15];
    KPRCB Prcb;
} KIPCR, *PKIPCR;

//
// Special Registers Structure (outside of CONTEXT)
// Based on WoA symbols
//
typedef struct _KSPECIAL_REGISTERS
{
    ULONG64 Elr_El1;
    UINT32  Spsr_El1;
    ULONG64 Tpidr_El0;
    ULONG64 Tpidrro_El0;
    ULONG64 Tpidr_El1;
    ULONG64 KernelBvr[8];
    ULONG   KernelBcr[8];
    ULONG64 KernelWvr[2];
    ULONG   KernelWcr[2];
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// ARM64 Architecture State
// Based on WoA symbols
//
typedef struct _KARM64_ARCH_STATE
{
    ULONG64 Midr_El1;
    ULONG64 Sctlr_El1;
    ULONG64 Actlr_El1;
    ULONG64 Cpacr_El1;
    ULONG64 Tcr_El1;
    ULONG64 Ttbr0_El1;
    ULONG64 Ttbr1_El1;
    ULONG64 Esr_El1;
    ULONG64 Far_El1;
    ULONG64 Pmcr_El0;
    ULONG64 Pmcntenset_El0;
    ULONG64 Pmccntr_El0;
    ULONG64 Pmxevcntr_El0[31];
    ULONG64 Pmxevtyper_El0[31];
    ULONG64 Pmovsclr_El0;
    ULONG64 Pmselr_El0;
    ULONG64 Pmuserenr_El0;
    ULONG64 Mair_El1;
    ULONG64 Vbar_El1;
} KARM64_ARCH_STATE, *PKARM64_ARCH_STATE;

typedef struct _KPROCESSOR_STATE
{
    KSPECIAL_REGISTERS SpecialRegisters; // 0
    KARM64_ARCH_STATE ArchState;         // 160
    CONTEXT ContextFrame;                // 800
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

//
// Macro to get current KPRCB
//
FORCEINLINE
struct _KPRCB *
KeGetCurrentPrcb(VOID)
{  
    //UNIMPLEMENTED;
    return 0;
}

//
// Just read it from the PCR
//
#define KeGetCurrentIrql()             KeGetPcr()->CurrentIrql
#define _KeGetCurrentThread()          KeGetCurrentPrcb()->CurrentThread
#define _KeGetPreviousMode()           KeGetCurrentPrcb()->CurrentThread->PreviousMode
#define _KeIsExecutingDpc()            (KeGetCurrentPrcb()->DpcRoutineActive != 0)
#define KeGetCurrentThread()           _KeGetCurrentThread()
#define KeGetPreviousMode()            _KeGetPreviousMode()

#endif // !NTOS_MODE_USER

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // !_ARM64_KETYPES_H
