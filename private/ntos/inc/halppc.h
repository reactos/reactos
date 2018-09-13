#include "kxppc.h"

//
// Wait Reason and Wait Type Enumerated Type Values
//

#define WrExecutive 0x0

//
// Bug Check Code Definitions
//

#define APC_INDEX_MISMATCH 0x1
#define DATA_BUS_ERROR 0x2e
#define DATA_COHERENCY_EXCEPTION 0x55
#define HAL1_INITIALIZATION_FAILED 0x61
#define INSTRUCTION_BUS_ERROR 0x2f
#define INSTRUCTION_COHERENCY_EXCEPTION 0x56
#define INTERRUPT_EXCEPTION_NOT_HANDLED 0x3d
#define INTERRUPT_UNWIND_ATTEMPTED 0x3c
#define INVALID_AFFINITY_SET 0x3
#define INVALID_DATA_ACCESS_TRAP 0x4
#define IRQL_GT_ZERO_AT_SYSTEM_SERVICE 0x4a
#define IRQL_NOT_LESS_OR_EQUAL 0xa
#define KMODE_EXCEPTION_NOT_HANDLED 0x1e
#define NMI_HARDWARE_FAILURE 0x80
#define NO_USER_MODE_CONTEXT 0xe
#define PAGE_FAULT_WITH_INTERRUPTS_OFF 0x49
#define PANIC_STACK_SWITCH 0x2b
#define SPIN_LOCK_INIT_FAILURE 0x81
#define SYSTEM_EXIT_OWNED_MUTEX 0x39
#define SYSTEM_SERVICE_EXCEPTION 0x3b
#define SYSTEM_UNWIND_PREVIOUS_USER 0x3a
#define TRAP_CAUSE_UNKNOWN 0x12
#define UNEXPECTED_KERNEL_MODE_TRAP 0x7f

//
// Breakpoint type definitions
//

#define DBG_STATUS_CONTROL_C 0x1

//
// Exception Record Offset, Flag, and Enumerated Type Definitions
//

#define EXCEPTION_NONCONTINUABLE 0x1
#define EXCEPTION_UNWINDING 0x2
#define EXCEPTION_EXIT_UNWIND 0x4
#define EXCEPTION_STACK_INVALID 0x8
#define EXCEPTION_NESTED_CALL 0x10
#define EXCEPTION_TARGET_UNWIND 0x20
#define EXCEPTION_COLLIDED_UNWIND 0x40
#define EXCEPTION_UNWIND 0x66
#define EXCEPTION_EXECUTE_HANDLER 0x1
#define EXCEPTION_CONTINUE_SEARCH 0x0
#define EXCEPTION_CONTINUE_EXECUTION 0xffffffff

#define ExceptionContinueExecution 0x0
#define ExceptionContinueSearch 0x1
#define ExceptionNestedException 0x2
#define ExceptionCollidedUnwind 0x3

#define ErExceptionCode 0x0
#define ErExceptionFlags 0x4
#define ErExceptionRecord 0x8
#define ErExceptionAddress 0xc
#define ErNumberParameters 0x10
#define ErExceptionInformation 0x14
#define ExceptionRecordLength 0x50

//
// Fast Mutex Structure Offset Definitions
//

#define FmCount 0x0
#define FmOwner 0x4
#define FmContention 0x8
#define FmEvent 0xc
#define FmOldIrql 0x1c

//
// Interrupt Priority Request Level Definitions
//

#define APC_LEVEL 0x1
#define DISPATCH_LEVEL 0x2
#define IPI_LEVEL 0x1d
#define POWER_LEVEL 0x1e
#define PROFILE_LEVEL 0x1b
#define HIGH_LEVEL 0x1f
#define SYNCH_LEVEL 0x2

//
// Large Integer Structure Offset Definitions
//

#define LiLowPart 0x0
#define LiHighPart 0x4

//
// List Entry Structure Offset Definitions
//

#define LsFlink 0x0
#define LsBlink 0x4

//
// String Structure Offset Definitions
//

#define StrLength 0x0
#define StrMaximumLength 0x2
#define StrBuffer 0x4

//
// System Time Structure Offset Definitions
//

#define StLowTime 0x0
#define StHigh1Time 0x4
#define StHigh2Time 0x8

//
// Time Structure Offset Definitions
//

#define TmLowTime 0x0
#define TmHighTime 0x4

//
// DPC object Structure Offset Definitions
//

#define DpType 0x0
#define DpNumber 0x2
#define DpImportance 0x3
#define DpDpcListEntry 0x4
#define DpDeferredRoutine 0xc
#define DpDeferredContext 0x10
#define DpSystemArgument1 0x14
#define DpSystemArgument2 0x18
#define DpLock 0x1c
#define DpcObjectLength 0x20

//
// Interrupt Object Structure Offset Definitions
//

#define InLevelSensitive 0x0
#define InLatched 0x1

#define InType 0x0
#define InSize 0x2
#define InInterruptListEntry 0x4
#define InServiceRoutine 0xc
#define InServiceContext 0x10
#define InSpinLock 0x14
#define InActualLock 0x1c
#define InDispatchAddress 0x20
#define InVector 0x24
#define InIrql 0x28
#define InSynchronizeIrql 0x29
#define InFloatingSave 0x2a
#define InConnected 0x2b
#define InNumber 0x2c
#define InMode 0x30
#define InShareVector 0x2d
#define InDispatchCode 0x3c
#define InterruptObjectLength 0x4c

//
// Processor Control Registers Structure Offset Definitions
//

#define PCR_MINOR_VERSION 0x1
#define PCR_MAJOR_VERSION 0x1

#define PcMinorVersion 0x0
#define PcMajorVersion 0x2
#define PcInterruptRoutine 0x4
#define PcPcrPage2 0x404
#define PcKseg0Top 0x408
#define PcFirstLevelDcacheSize 0x484
#define PcFirstLevelDcacheFillSize 0x488
#define PcFirstLevelIcacheSize 0x48c
#define PcFirstLevelIcacheFillSize 0x490
#define PcSecondLevelDcacheSize 0x494
#define PcSecondLevelDcacheFillSize 0x498
#define PcSecondLevelIcacheSize 0x49c
#define PcSecondLevelIcacheFillSize 0x4a0
#define PcPrcb 0x4a4
#define PcTeb 0x4a8
#define PcDcacheAlignment 0x4ac
#define PcDcacheFillSize 0x4b0
#define PcIcacheAlignment 0x4b4
#define PcIcacheFillSize 0x4b8
#define PcProcessorVersion 0x4bc
#define PcProcessorRevision 0x4c0
#define PcProfileInterval 0x4c4
#define PcProfileCount 0x4c8
#define PcStallExecutionCount 0x4cc
#define PcStallScaleFactor 0x4d0
#define PcCachePolicy 0x4d8
#define PcIcacheMode 0x4d8
#define PcDcacheMode 0x4d9
#define PcIrqlMask 0x4dc
#define PcIrqlTable 0x4fc
#define PcCurrentIrql 0x505
#define PcNumber 0x506
#define PcSetMember 0x508
#define PcCurrentThread 0x510
#define PcAlignedCachePolicy 0x514
#define PcSoftwareInterrupt 0x518
#define PcApcInterrupt 0x518
#define PcDispatchInterrupt 0x519
#define PcNotMember 0x51c
#define PcSystemReserved 0x520
#define PcHalReserved 0x560

//
// Processor Block Structure Offset Definitions
//

#define PRCB_MINOR_VERSION 0x1
#define PRCB_MAJOR_VERSION 0x1

#define PbMinorVersion 0x0
#define PbMajorVersion 0x2
#define PbCurrentThread 0x4
#define PbNextThread 0x8
#define PbIdleThread 0xc
#define PbNumber 0x10
#define PbSetMember 0x14
#define PbRestartBlock 0x18
#define PbPcrPage 0x1c
#define PbSystemReserved 0x24
#define PbHalReserved 0x60

//
// Context Frame Offset and Flag Definitions
//

#define CONTEXT_FULL 0x7
#define CONTEXT_CONTROL 0x1
#define CONTEXT_FLOATING_POINT 0x2
#define CONTEXT_INTEGER 0x4

#define CxFpr0 0x0
#define CxFpr1 0x8
#define CxFpr2 0x10
#define CxFpr3 0x18
#define CxFpr4 0x20
#define CxFpr5 0x28
#define CxFpr6 0x30
#define CxFpr7 0x38
#define CxFpr8 0x40
#define CxFpr9 0x48
#define CxFpr10 0x50
#define CxFpr11 0x58
#define CxFpr12 0x60
#define CxFpr13 0x68
#define CxFpr14 0x70
#define CxFpr15 0x78
#define CxFpr16 0x80
#define CxFpr17 0x88
#define CxFpr18 0x90
#define CxFpr19 0x98
#define CxFpr20 0xa0
#define CxFpr21 0xa8
#define CxFpr22 0xb0
#define CxFpr23 0xb8
#define CxFpr24 0xc0
#define CxFpr25 0xc8
#define CxFpr26 0xd0
#define CxFpr27 0xd8
#define CxFpr28 0xe0
#define CxFpr29 0xe8
#define CxFpr30 0xf0
#define CxFpr31 0xf8
#define CxFpscr 0x100
#define CxGpr0 0x108
#define CxGpr1 0x10c
#define CxGpr2 0x110
#define CxGpr3 0x114
#define CxGpr4 0x118
#define CxGpr5 0x11c
#define CxGpr6 0x120
#define CxGpr7 0x124
#define CxGpr8 0x128
#define CxGpr9 0x12c
#define CxGpr10 0x130
#define CxGpr11 0x134
#define CxGpr12 0x138
#define CxGpr13 0x13c
#define CxGpr14 0x140
#define CxGpr15 0x144
#define CxGpr16 0x148
#define CxGpr17 0x14c
#define CxGpr18 0x150
#define CxGpr19 0x154
#define CxGpr20 0x158
#define CxGpr21 0x15c
#define CxGpr22 0x160
#define CxGpr23 0x164
#define CxGpr24 0x168
#define CxGpr25 0x16c
#define CxGpr26 0x170
#define CxGpr27 0x174
#define CxGpr28 0x178
#define CxGpr29 0x17c
#define CxGpr30 0x180
#define CxGpr31 0x184
#define CxCr 0x188
#define CxXer 0x18c
#define CxMsr 0x190
#define CxIar 0x194
#define CxLr 0x198
#define CxCtr 0x19c
#define CxContextFlags 0x1a0
#define CxDr0 0x1b0
#define CxDr1 0x1b4
#define CxDr2 0x1b8
#define CxDr3 0x1bc
#define CxDr4 0x1c0
#define CxDr5 0x1c4
#define CxDr6 0x1c8
#define CxDr7 0x1cc
#define ContextFrameLength 0x1d0

//
// Call/Return Stack Frame Header Offset Definitions and Length
//

#define CrBackChain 0x0
#define CrGlueSaved1 0x4
#define CrGlueSaved2 0x8
#define CrReserved1 0xc
#define CrSpare1 0x10
#define CrSpare2 0x14
#define CrParameter0 0x18
#define CrParameter1 0x1c
#define CrParameter2 0x20
#define CrParameter3 0x24
#define CrParameter4 0x28
#define CrParameter5 0x2c
#define CrParameter6 0x30
#define CrParameter7 0x34
#define StackFrameHeaderLength 0x38

//
// Exception Frame Offset Definitions and Length
//

#define ExGpr13 0x4
#define ExGpr14 0x8
#define ExGpr15 0xc
#define ExGpr16 0x10
#define ExGpr17 0x14
#define ExGpr18 0x18
#define ExGpr19 0x1c
#define ExGpr20 0x20
#define ExGpr21 0x24
#define ExGpr22 0x28
#define ExGpr23 0x2c
#define ExGpr24 0x30
#define ExGpr25 0x34
#define ExGpr26 0x38
#define ExGpr27 0x3c
#define ExGpr28 0x40
#define ExGpr29 0x44
#define ExGpr30 0x48
#define ExGpr31 0x4c
#define ExFpr14 0x50
#define ExFpr15 0x58
#define ExFpr16 0x60
#define ExFpr17 0x68
#define ExFpr18 0x70
#define ExFpr19 0x78
#define ExFpr20 0x80
#define ExFpr21 0x88
#define ExFpr22 0x90
#define ExFpr23 0x98
#define ExFpr24 0xa0
#define ExFpr25 0xa8
#define ExFpr26 0xb0
#define ExFpr27 0xb8
#define ExFpr28 0xc0
#define ExFpr29 0xc8
#define ExFpr30 0xd0
#define ExFpr31 0xd8
#define ExceptionFrameLength 0xe0

//
// Trap Frame Offset Definitions and Length
//

#define TrTrapFrame 0x0
#define TrOldIrql 0x4
#define TrPreviousMode 0x5
#define TrSavedApcStateIndex 0x6
#define TrSavedKernelApcDisable 0x7
#define TrExceptionRecord 0x8
#define TrGpr0 0x5c
#define TrGpr1 0x60
#define TrGpr2 0x64
#define TrGpr3 0x68
#define TrGpr4 0x6c
#define TrGpr5 0x70
#define TrGpr6 0x74
#define TrGpr7 0x78
#define TrGpr8 0x7c
#define TrGpr9 0x80
#define TrGpr10 0x84
#define TrGpr11 0x88
#define TrGpr12 0x8c
#define TrFpr0 0x90
#define TrFpr1 0x98
#define TrFpr2 0xa0
#define TrFpr3 0xa8
#define TrFpr4 0xb0
#define TrFpr5 0xb8
#define TrFpr6 0xc0
#define TrFpr7 0xc8
#define TrFpr8 0xd0
#define TrFpr9 0xd8
#define TrFpr10 0xe0
#define TrFpr11 0xe8
#define TrFpr12 0xf0
#define TrFpr13 0xf8
#define TrFpscr 0x100
#define TrCr 0x108
#define TrXer 0x10c
#define TrMsr 0x110
#define TrIar 0x114
#define TrLr 0x118
#define TrCtr 0x11c
#define TrDr0 0x120
#define TrDr1 0x124
#define TrDr2 0x128
#define TrDr3 0x12c
#define TrDr4 0x130
#define TrDr5 0x134
#define TrDr6 0x138
#define TrDr7 0x13c
#define TrapFrameLength 0x140

//
// Processor State Frame Offset Definitions
//

#define PsContextFrame 0x0
#define PsSpecialRegisters 0x1d0
#define SrKernelDr0 0x0
#define SrKernelDr1 0x4
#define SrKernelDr2 0x8
#define SrKernelDr3 0xc
#define SrKernelDr4 0x10
#define SrKernelDr5 0x14
#define SrKernelDr6 0x18
#define SrKernelDr7 0x1c
#define SrSprg0 0x20
#define SrSprg1 0x24
#define SrSr0 0x28
#define SrSr1 0x2c
#define SrSr2 0x30
#define SrSr3 0x34
#define SrSr4 0x38
#define SrSr5 0x3c
#define SrSr6 0x40
#define SrSr7 0x44
#define SrSr8 0x48
#define SrSr9 0x4c
#define SrSr10 0x50
#define SrSr11 0x54
#define SrSr12 0x58
#define SrSr13 0x5c
#define SrSr14 0x60
#define SrSr15 0x64
#define SrDBAT0L 0x68
#define SrDBAT0U 0x6c
#define SrDBAT1L 0x70
#define SrDBAT1U 0x74
#define SrDBAT2L 0x78
#define SrDBAT2U 0x7c
#define SrDBAT3L 0x80
#define SrDBAT3U 0x84
#define SrIBAT0L 0x88
#define SrIBAT0U 0x8c
#define SrIBAT1L 0x90
#define SrIBAT1U 0x94
#define SrIBAT2L 0x98
#define SrIBAT2U 0x9c
#define SrIBAT3L 0xa0
#define SrIBAT3U 0xa4
#define SrSdr1 0xa8
#define ProcessorStateLength 0x2a0

//
// Loader Parameter Block Offset Definitions
//

#define LpbLoadOrderListHead 0x0
#define LpbMemoryDescriptorListHead 0x8
#define LpbKernelStack 0x18
#define LpbPrcb 0x1c
#define LpbProcess 0x20
#define LpbThread 0x24
#define LpbRegistryLength 0x28
#define LpbRegistryBase 0x2c
#define LpbInterruptStack 0x5c
#define LpbFirstLevelDcacheSize 0x60
#define LpbFirstLevelDcacheFillSize 0x64
#define LpbFirstLevelIcacheSize 0x68
#define LpbFirstLevelIcacheFillSize 0x6c
#define LpbHashedPageTable 0x70
#define LpbPanicStack 0x74
#define LpbPcrPage 0x78
#define LpbPdrPage 0x7c
#define LpbSecondLevelDcacheSize 0x80
#define LpbSecondLevelDcacheFillSize 0x84
#define LpbSecondLevelIcacheSize 0x88
#define LpbSecondLevelIcacheFillSize 0x8c
#define LpbPcrPage2 0x90
#define LpbIcacheMode 0x94
#define LpbDcacheMode 0x95
#define LpbNumberCongruenceClasses 0x96
#define LpbKseg0Top 0x98
#define LpbMemoryManagementModel 0x9e
#define LpbHashedPageTableSize 0xa0
#define LpbKernelKseg0PagesDescriptor 0xa8
#define LpbMinimumBlockLength 0xac
#define LpbMaximumBlockLength 0xb0

//
// Memory Allocation Descriptor Offset Definitions
//

#define MadListEntry 0x0
#define MadMemoryType 0x8
#define MadBasePage 0xc
#define MadPageCount 0x10

//
// Address Space Layout Definitions
//

#define KUSEG_BASE 0x0
#define KSEG0_BASE 0x80000000
#define KSEG1_BASE PCR->Kseg0Top
#define KSEG2_BASE KSEG1_BASE

//
// Page Table and Directory Entry Definitions
//

#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 0xc
#define PDI_SHIFT 0x16
#define PTI_SHIFT 0xc

//
// Breakpoint Definitions
//

#define USER_BREAKPOINT 0x0
#define KERNEL_BREAKPOINT 0x1
#define BREAKIN_BREAKPOINT 0x2

//
// Miscellaneous Definitions
//

#define Executive 0x0
#define KernelMode 0x0
#define FALSE 0x0
#define TRUE 0x1
#define UNCACHED_POLICY 0x2
#define KiPcr 0xffffd000
#define KiPcr2 0xffffe000
