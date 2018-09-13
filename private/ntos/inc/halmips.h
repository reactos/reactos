#include "kxmips.h"

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
#define IPI_LEVEL 0x7
#define POWER_LEVEL 0x7
#define PROFILE_LEVEL 0x8
#define HIGH_LEVEL 0x8
#define SYNCH_LEVEL 0x6

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
#define PbSystemReserved 0x20
#define PbHalReserved 0x60

//
// Processor Control Registers Structure Offset Definitions
//

#define PCR_MINOR_VERSION 0x1
#define PCR_MAJOR_VERSION 0x1

#define PcMinorVersion 0x0
#define PcMajorVersion 0x2
#define PcInterruptRoutine 0x4
#define PcXcodeDispatch 0x404
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
#define PcTlsArray 0x4ac
#define PcDcacheFillSize 0x4b0
#define PcIcacheAlignment 0x4b4
#define PcIcacheFillSize 0x4b8
#define PcProcessorId 0x4bc
#define PcProfileInterval 0x4c0
#define PcProfileCount 0x4c4
#define PcStallExecutionCount 0x4c8
#define PcStallScaleFactor 0x4cc
#define PcNumber 0x4d0
#define PcDataBusError 0x4d4
#define PcInstructionBusError 0x4d8
#define PcCachePolicy 0x4dc
#define PcIrqlMask 0x4e0
#define PcIrqlTable 0x500
#define PcCurrentIrql 0x509
#define PcSetMember 0x50c
#define PcCurrentThread 0x514
#define PcAlignedCachePolicy 0x518
#define PcNotMember 0x51c
#define PcSystemReserved 0x520
#define PcDcacheAlignment 0x55c
#define PcHalReserved 0x560

//
// Context Frame Offset and Flag Definitions
//

#define CONTEXT_FULL 0x10017
#define CONTEXT_CONTROL 0x10001
#define CONTEXT_FLOATING_POINT 0x10002
#define CONTEXT_INTEGER 0x10004
#define CONTEXT_EXTENDED_FLOAT 0x1000a
#define CONTEXT_EXTENDED_INTEGER 0x10014

//
// 32-bit Context Frame Offset Definitions
//

#define CxFltF0 0x10
#define CxFltF1 0x14
#define CxFltF2 0x18
#define CxFltF3 0x1c
#define CxFltF4 0x20
#define CxFltF5 0x24
#define CxFltF6 0x28
#define CxFltF7 0x2c
#define CxFltF8 0x30
#define CxFltF9 0x34
#define CxFltF10 0x38
#define CxFltF11 0x3c
#define CxFltF12 0x40
#define CxFltF13 0x44
#define CxFltF14 0x48
#define CxFltF15 0x4c
#define CxFltF16 0x50
#define CxFltF17 0x54
#define CxFltF18 0x58
#define CxFltF19 0x5c
#define CxFltF20 0x60
#define CxFltF21 0x64
#define CxFltF22 0x68
#define CxFltF23 0x6c
#define CxFltF24 0x70
#define CxFltF25 0x74
#define CxFltF26 0x78
#define CxFltF27 0x7c
#define CxFltF28 0x80
#define CxFltF29 0x84
#define CxFltF30 0x88
#define CxFltF31 0x8c
#define CxIntZero 0x90
#define CxIntAt 0x94
#define CxIntV0 0x98
#define CxIntV1 0x9c
#define CxIntA0 0xa0
#define CxIntA1 0xa4
#define CxIntA2 0xa8
#define CxIntA3 0xac
#define CxIntT0 0xb0
#define CxIntT1 0xb4
#define CxIntT2 0xb8
#define CxIntT3 0xbc
#define CxIntT4 0xc0
#define CxIntT5 0xc4
#define CxIntT6 0xc8
#define CxIntT7 0xcc
#define CxIntS0 0xd0
#define CxIntS1 0xd4
#define CxIntS2 0xd8
#define CxIntS3 0xdc
#define CxIntS4 0xe0
#define CxIntS5 0xe4
#define CxIntS6 0xe8
#define CxIntS7 0xec
#define CxIntT8 0xf0
#define CxIntT9 0xf4
#define CxIntK0 0xf8
#define CxIntK1 0xfc
#define CxIntGp 0x100
#define CxIntSp 0x104
#define CxIntS8 0x108
#define CxIntRa 0x10c
#define CxIntLo 0x110
#define CxIntHi 0x114
#define CxFsr 0x118
#define CxFir 0x11c
#define CxPsr 0x120
#define CxContextFlags 0x124

//
// 64-bit Context Frame Offset Definitions
//

#define CxXFltF0 0x10
#define CxXFltF1 0x18
#define CxXFltF2 0x20
#define CxXFltF3 0x28
#define CxXFltF4 0x30
#define CxXFltF5 0x38
#define CxXFltF6 0x40
#define CxXFltF7 0x48
#define CxXFltF8 0x50
#define CxXFltF9 0x58
#define CxXFltF10 0x60
#define CxXFltF11 0x68
#define CxXFltF12 0x70
#define CxXFltF13 0x78
#define CxXFltF14 0x80
#define CxXFltF15 0x88
#define CxXFltF16 0x90
#define CxXFltF17 0x98
#define CxXFltF18 0xa0
#define CxXFltF19 0xa8
#define CxXFltF20 0xb0
#define CxXFltF21 0xb8
#define CxXFltF22 0xc0
#define CxXFltF23 0xc8
#define CxXFltF24 0xd0
#define CxXFltF25 0xd8
#define CxXFltF26 0xe0
#define CxXFltF27 0xe8
#define CxXFltF28 0xf0
#define CxXFltF29 0xf8
#define CxXFltF30 0x100
#define CxXFltF31 0x108
#define CxXFsr 0x118
#define CxXFir 0x11c
#define CxXPsr 0x120
#define CxXContextFlags 0x124
#define CxXIntZero 0x128
#define CxXIntAt 0x130
#define CxXIntV0 0x138
#define CxXIntV1 0x140
#define CxXIntA0 0x148
#define CxXIntA1 0x150
#define CxXIntA2 0x158
#define CxXIntA3 0x160
#define CxXIntT0 0x168
#define CxXIntT1 0x170
#define CxXIntT2 0x178
#define CxXIntT3 0x180
#define CxXIntT4 0x188
#define CxXIntT5 0x190
#define CxXIntT6 0x198
#define CxXIntT7 0x1a0
#define CxXIntS0 0x1a8
#define CxXIntS1 0x1b0
#define CxXIntS2 0x1b8
#define CxXIntS3 0x1c0
#define CxXIntS4 0x1c8
#define CxXIntS5 0x1d0
#define CxXIntS6 0x1d8
#define CxXIntS7 0x1e0
#define CxXIntT8 0x1e8
#define CxXIntT9 0x1f0
#define CxXIntK0 0x1f8
#define CxXIntK1 0x200
#define CxXIntGp 0x208
#define CxXIntSp 0x210
#define CxXIntS8 0x218
#define CxXIntRa 0x220
#define CxXIntLo 0x228
#define CxXIntHi 0x230
#define ContextFrameLength 0x238

//
// Exception Frame Offset Definitions and Length
//

#define ExArgs 0x0

//
// 32-bit Nonvolatile Floating State
//

#define ExFltF20 0x20
#define ExFltF21 0x24
#define ExFltF22 0x28
#define ExFltF23 0x2c
#define ExFltF24 0x30
#define ExFltF25 0x34
#define ExFltF26 0x38
#define ExFltF27 0x3c
#define ExFltF28 0x40
#define ExFltF29 0x44
#define ExFltF30 0x48
#define ExFltF31 0x4c

//
// 64-bit Nonvolatile Floating State
//

#define ExXFltF20 0x20
#define ExXFltF22 0x28
#define ExXFltF24 0x30
#define ExXFltF26 0x38
#define ExXFltF28 0x40
#define ExXFltF30 0x48

//
// 32-bit Nonvolatile Integer State
//

#define ExIntS0 0x50
#define ExIntS1 0x54
#define ExIntS2 0x58
#define ExIntS3 0x5c
#define ExIntS4 0x60
#define ExIntS5 0x64
#define ExIntS6 0x68
#define ExIntS7 0x6c
#define ExIntS8 0x70
#define ExSwapReturn 0x74
#define ExIntRa 0x78
#define ExceptionFrameLength 0x80

//
// Trap Frame Offset Definitions and Length
//

#define TrArgs 0x0

//
// 32-bit Volatile Floating State
//

#define TrFltF0 0x10
#define TrFltF1 0x14
#define TrFltF2 0x18
#define TrFltF3 0x1c
#define TrFltF4 0x20
#define TrFltF5 0x24
#define TrFltF6 0x28
#define TrFltF7 0x2c
#define TrFltF8 0x30
#define TrFltF9 0x34
#define TrFltF10 0x38
#define TrFltF11 0x3c
#define TrFltF12 0x40
#define TrFltF13 0x44
#define TrFltF14 0x48
#define TrFltF15 0x4c
#define TrFltF16 0x50
#define TrFltF17 0x54
#define TrFltF18 0x58
#define TrFltF19 0x5c

//
// 64-bit Volatile Floating State
//

#define TrXFltF0 0x10
#define TrXFltF1 0x18
#define TrXFltF2 0x20
#define TrXFltF3 0x28
#define TrXFltF4 0x30
#define TrXFltF5 0x38
#define TrXFltF6 0x40
#define TrXFltF7 0x48
#define TrXFltF8 0x50
#define TrXFltF9 0x58
#define TrXFltF10 0x60
#define TrXFltF11 0x68
#define TrXFltF12 0x70
#define TrXFltF13 0x78
#define TrXFltF14 0x80
#define TrXFltF15 0x88
#define TrXFltF16 0x90
#define TrXFltF17 0x98
#define TrXFltF18 0xa0
#define TrXFltF19 0xa8
#define TrXFltF21 0xb0
#define TrXFltF23 0xb8
#define TrXFltF25 0xc0
#define TrXFltF27 0xc8
#define TrXFltF29 0xd0
#define TrXFltF31 0xd8

//
// 64-bit Volatile Integer State
//

#define TrXIntZero 0xe0
#define TrXIntAt 0xe8
#define TrXIntV0 0xf0
#define TrXIntV1 0xf8
#define TrXIntA0 0x100
#define TrXIntA1 0x108
#define TrXIntA2 0x110
#define TrXIntA3 0x118
#define TrXIntT0 0x120
#define TrXIntT1 0x128
#define TrXIntT2 0x130
#define TrXIntT3 0x138
#define TrXIntT4 0x140
#define TrXIntT5 0x148
#define TrXIntT6 0x150
#define TrXIntT7 0x158
#define TrXIntS0 0x160
#define TrXIntS1 0x168
#define TrXIntS2 0x170
#define TrXIntS3 0x178
#define TrXIntS4 0x180
#define TrXIntS5 0x188
#define TrXIntS6 0x190
#define TrXIntS7 0x198
#define TrXIntT8 0x1a0
#define TrXIntT9 0x1a8
#define TrXIntGp 0x1c0
#define TrXIntSp 0x1c8
#define TrXIntS8 0x1d0
#define TrXIntRa 0x1d8
#define TrXIntLo 0x1e0
#define TrXIntHi 0x1e8

#define TrFir 0x1f4
#define TrFsr 0x1f0
#define TrPsr 0x1f8
#define TrExceptionRecord 0x1fc
#define TrOldIrql 0x24c
#define TrPreviousMode 0x24d
#define TrSavedFlag 0x24e
#define TrOnInterruptStack 0x250
#define TrTrapFrame 0x250
#define TrapFrameLength 0x258
#define TrapFrameArguments 0x40

//
// Loader Parameter Block Offset Definitions
//

#define LpbLoadOrderListHead 0x0
#define LpbMemoryDescriptorListHead 0x8
#define LpbKernelStack 0x18
#define LpbPrcb 0x1c
#define LpbProcess 0x20
#define LpbThread 0x24
#define LpbInterruptStack 0x5c
#define LpbFirstLevelDcacheSize 0x60
#define LpbFirstLevelDcacheFillSize 0x64
#define LpbFirstLevelIcacheSize 0x68
#define LpbFirstLevelIcacheFillSize 0x6c
#define LpbGpBase 0x70
#define LpbPanicStack 0x74
#define LpbPcrPage 0x78
#define LpbPdrPage 0x7c
#define LpbSecondLevelDcacheSize 0x80
#define LpbSecondLevelDcacheFillSize 0x84
#define LpbSecondLevelIcacheSize 0x88
#define LpbSecondLevelIcacheFillSize 0x8c
#define LpbPcrPage2 0x90
#define LpbRegistryLength 0x28
#define LpbRegistryBase 0x2c

//
// Address Space Layout Definitions
//

#define KUSEG_BASE 0x0
#define KSEG0_BASE 0x80000000
#define KSEG1_BASE 0xa0000000
#define KSEG2_BASE 0xc0000000

//
// Page Table and Directory Entry Definitions
//

#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 0xc
#define PDI_SHIFT 0x16
#define PTI_SHIFT 0xc

//
// Software Interrupt Request Mask Definitions
//

#define APC_INTERRUPT 0x100
#define DISPATCH_INTERRUPT 0x200

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
#define KiPcr 0xfffff000
#define KiPcr2 0xffffe000
