/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    asm.h

Abstract:

    ASM Offsets for dealing with de-referencing structures in registers.
    C-compatible version of the file ks386.inc present in the newest WDK.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _ASM_H
#define _ASM_H

#define NEW_SCHEDULER

//
// PCR Access
//
#ifdef __ASM__
#ifdef CONFIG_SMP
#define PCR                                     fs:
#else
#define PCR                                     ds:[0xFFDFF000]
#endif
#endif

//
// CPU Modes
//
#define KernelMode                              0x0
#define UserMode                                0x1

//
// CPU Types
//
#define CPU_INTEL                               0x1
#define CPU_AMD                                 0x2

//
// Selector Names
//
#ifdef __ASM__
#define RPL_MASK                                0x0003
#define MODE_MASK                               0x0001
#define KGDT_R0_CODE                            (0x8)
#define KGDT_R0_DATA                            (0x10)
#define KGDT_R3_CODE                            (0x18)
#define KGDT_R3_DATA                            (0x20)
#define KGDT_TSS                                (0x28)
#define KGDT_R0_PCR                             (0x30)
#define KGDT_R3_TEB                             (0x38)
#define KGDT_LDT                                (0x48)
#define KGDT_DF_TSS                             (0x50)
#define KGDT_NMI_TSS                            (0x58)
#endif

//
// KV86M_REGISTERS Offsets
//
#define KV86M_REGISTERS_EBP                     0x0
#define KV86M_REGISTERS_EDI                     0x4
#define KV86M_REGISTERS_ESI                     0x8
#define KV86M_REGISTERS_EDX                     0xC
#define KV86M_REGISTERS_ECX                     0x10
#define KV86M_REGISTERS_EBX                     0x14
#define KV86M_REGISTERS_EAX                     0x18
#define KV86M_REGISTERS_DS                      0x1C
#define KV86M_REGISTERS_ES                      0x20
#define KV86M_REGISTERS_FS                      0x24
#define KV86M_REGISTERS_GS                      0x28
#define KV86M_REGISTERS_EIP                     0x2C
#define KV86M_REGISTERS_CS                      0x30
#define KV86M_REGISTERS_EFLAGS                  0x34
#define KV86M_REGISTERS_ESP                     0x38
#define KV86M_REGISTERS_SS                      0x3C
#define TF_SAVED_EXCEPTION_STACK                0x8C
#define TF_REGS                                 0x90
#define TF_ORIG_EBP                             0x94

//
// TSS Offsets
//
#define KTSS_ESP0                               0x4
#define KTSS_CR3                                0x1C
#define KTSS_EFLAGS                             0x24
#define KTSS_IOMAPBASE                          0x66
#define KTSS_IO_MAPS                            0x68

//
// KTHREAD Offsets
//
#define KTHREAD_DEBUG_ACTIVE                    0x03
#define KTHREAD_INITIAL_STACK                   0x18
#define KTHREAD_STACK_LIMIT                     0x1C
#define KTHREAD_TEB                             0x74
#define KTHREAD_KERNEL_STACK                    0x20
#define KTHREAD_ALERTED                         0x5E
#define KTHREAD_APCSTATE_PROCESS                0x28 + 0x10
#define KTHREAD_PENDING_USER_APC                0x28 + 0x16
#define KTHREAD_PENDING_KERNEL_APC              0x28 + 0x15
#define KTHREAD_CONTEXT_SWITCHES                0x48
#define KTHREAD_STATE_                          0x4C
#define KTHREAD_NPX_STATE                       0x4D
#define KTHREAD_WAIT_IRQL                       0x4E
#define KTHREAD_NEXT_PROCESSOR                  0x40
#define KTHREAD_WAIT_REASON                     0x5A
#define KTHREAD_PRIORITY                        0x5B
#define KTHREAD_SWAP_BUSY                       0x5D
#define KTHREAD_SERVICE_TABLE                   0x118
#define KTHREAD_PREVIOUS_MODE                   0xD7
#define KTHREAD_COMBINED_APC_DISABLE            0x70
#define KTHREAD_SPECIAL_APC_DISABLE             0x72
#define KTHREAD_LARGE_STACK                     0x107
#define KTHREAD_TRAP_FRAME                      0x110
#define KTHREAD_CALLBACK_STACK                  0x114
#define KTHREAD_APC_STATE_INDEX                 0x11C
#define KTHREAD_STACK_BASE                      0x158
#define KTHREAD_QUANTUM                         0x15D
#define KTHREAD_KERNEL_TIME                     0x160
#define KTHREAD_USER_TIME                       0x18C

//
// KPROCESS Offsets
//
#define KPROCESS_DIRECTORY_TABLE_BASE           0x18
#define KPROCESS_LDT_DESCRIPTOR0                0x20
#define KPROCESS_LDT_DESCRIPTOR1                0x24
#define KPROCESS_INT21_DESCRIPTOR0              0x28
#define KPROCESS_INT21_DESCRIPTOR1              0x2C
#define KPROCESS_IOPM_OFFSET                    0x30
#define KPROCESS_ACTIVE_PROCESSORS              0x34
#define EPROCESS_VDM_OBJECTS                    0x144

//
// KTIMER_TABLE Offsets
//
#ifdef __ASM__
#define KTIMER_TABLE_ENTRY                      0x00
#define KTIMER_TABLE_TIME                       0x08
#define TIMER_ENTRY_SIZE                        0x10
#define TIMER_TABLE_SIZE                        0x200
#endif

//
// KPRCB Offsets
//
#define KPRCB_DR0                               0x2F8
#define KPRCB_DR1                               0x2FC
#define KPRCB_DR2                               0x300
#define KPRCB_DR3                               0x304
#define KPRCB_DR6                               0x308
#define KPRCB_DR7                               0x30C
#define KPRCB_TIMER_HAND                        0x964
#define KPRCB_TIMER_REQUEST                     0x968

//
// KPCR Offsets
//
#define KPCR_EXCEPTION_LIST                     0x0
#define KPCR_INITIAL_STACK                      0x4
#define KPCR_STACK_LIMIT                        0x8
#define KPCR_PERF_GLOBAL_GROUP_MASK             0x8
#define KPCR_CONTEXT_SWITCHES                   0x10
#define KPCR_SET_MEMBER_COPY                    0x14
#define KPCR_TEB                                0x18
#define KPCR_SELF                               0x1C
#define KPCR_PRCB                               0x20
#define KPCR_IRQL                               0x24
#define KPCR_IRR                                0x28
#define KPCR_IRR_ACTIVE                         0x2C
#define KPCR_IDR                                0x30
#define KPCR_KD_VERSION_BLOCK                   0x34
#define KPCR_IDT                                0x38
#define KPCR_GDT                                0x3C
#define KPCR_TSS                                0x40
#define KPCR_STALL_SCALE_FACTOR                 0x4C
#define KPCR_SET_MEMBER                         0x48
#define KPCR_NUMBER                             0x51
#define KPCR_VDM_ALERT                          0x54
#define KPCR_PRCB_DATA                          0x120
#define KPCR_CURRENT_THREAD                     0x124
#define KPCR_PRCB_NEXT_THREAD                   0x128
#define KPCR_PRCB_IDLE_THREAD                   0x12C
#define KPCR_PROCESSOR_NUMBER                   0x130
#define KPCR_PRCB_SET_MEMBER                    0x134
#define KPCR_PRCB_CPU_TYPE                      0x138
#define KPCR_NPX_THREAD                         0x640
#define KPCR_DR6                                0x428
#define KPCR_DR7                                0x42C
#define KPCR_PRCB_INTERRUPT_COUNT               0x644
#define KPCR_PRCB_KERNEL_TIME                   0x648
#define KPCR_PRCB_USER_TIME                     0x64C
#define KPCR_PRCB_DPC_TIME                      0x650
#define KPCR_PRCB_DEBUG_DPC_TIME                0x654
#define KPCR_PRCB_INTERRUPT_TIME                0x658
#define KPCR_PRCB_ADJUST_DPC_THRESHOLD          0x65C
#define KPCR_PRCB_SKIP_TICK                     0x664
#define KPCR_SYSTEM_CALLS                       0x6B8
#define KPCR_PRCB_DPC_QUEUE_DEPTH               0xA4C
#define KPCR_PRCB_DPC_COUNT                     0xA50
#define KPCR_PRCB_DPC_STACK                     0xA68
#define KPCR_PRCB_MAXIMUM_DPC_QUEUE_DEPTH       0xA6C
#define KPCR_PRCB_DPC_REQUEST_RATE              0xA70
#define KPCR_PRCB_DPC_INTERRUPT_REQUESTED       0xA78
#define KPCR_PRCB_DPC_ROUTINE_ACTIVE            0xA7A
#define KPCR_PRCB_DPC_LAST_COUNT                0xA80
#define KPCR_PRCB_TIMER_REQUEST                 0xA88
#define KPCR_PRCB_QUANTUM_END                   0xAA1
#define KPCR_PRCB_DEFERRED_READY_LIST_HEAD      0xC10
#define KPCR_PRCB_POWER_STATE_IDLE_FUNCTION     0xEC0

//
// KINTERRUPT Offsets
//
#define KINTERRUPT_SERVICE_ROUTINE              0x0C
#define KINTERRUPT_SERVICE_CONTEXT              0x10
#define KINTERRUPT_TICK_COUNT                   0x18
#define KINTERRUPT_ACTUAL_LOCK                  0x1C
#define KINTERRUPT_IRQL                         0x20
#define KINTERRUPT_VECTOR                       0x24
#define KINTERRUPT_SYNCHRONIZE_IRQL             0x29
#define KINTERRUPT_DISPATCH_COUNT               0x38

//
// KGDTENTRY Offsets
//
#define KGDT_BASE_LOW                           0x2
#define KGDT_BASE_MID                           0x4
#define KGDT_BASE_HI                            0x7
#define KGDT_LIMIT_HI                           0x6
#define KGDT_LIMIT_LOW                          0x0

//
// FPU Save Area Offsets
//
#define FP_CONTROL_WORD                         0x0
#define FP_STATUS_WORD                          0x4
#define FP_TAG_WORD                             0x8
#define FP_ERROR_OFFSET                         0xC
#define FP_ERROR_SELECTOR                       0x10
#define FP_DATA_OFFSET                          0x14
#define FP_DATA_SELECTOR                        0x18
#define FN_CR0_NPX_STATE                        0x20C
#define SIZEOF_FX_SAVE_AREA                     528
#define NPX_FRAME_LENGTH                        0x210

//
// FX Save Area Offsets
//
#define FX_CONTROL_WORD                         0x0
#define FX_STATUS_WORD                          0x2
#define FX_TAG_WORD                             0x4
#define FX_ERROR_OPCODE                         0x6
#define FX_ERROR_OFFSET                         0x8
#define FX_ERROR_SELECTOR                       0xC
#define FX_DATA_OFFSET                          0x10
#define FX_DATA_SELECTOR                        0x14
#define FX_MXCSR                                0x18

//
// NPX States
//
#define NPX_STATE_NOT_LOADED                    0xA
#define NPX_STATE_LOADED                        0x0

//
// Trap Frame Offsets
//
#define KTRAP_FRAME_DEBUGEBP                    0x0
#define KTRAP_FRAME_DEBUGEIP                    0x4
#define KTRAP_FRAME_DEBUGARGMARK                0x8
#define KTRAP_FRAME_DEBUGPOINTER                0xC
#define KTRAP_FRAME_TEMPCS                      0x10
#define KTRAP_FRAME_TEMPESP                     0x14
#define KTRAP_FRAME_DR0                         0x18
#define KTRAP_FRAME_DR1                         0x1C
#define KTRAP_FRAME_DR2                         0x20
#define KTRAP_FRAME_DR3                         0x24
#define KTRAP_FRAME_DR6                         0x28
#define KTRAP_FRAME_DR7                         0x2C
#define KTRAP_FRAME_GS                          0x30
#define KTRAP_FRAME_RESERVED1                   0x32
#define KTRAP_FRAME_ES                          0x34
#define KTRAP_FRAME_RESERVED2                   0x36
#define KTRAP_FRAME_DS                          0x38
#define KTRAP_FRAME_RESERVED3                   0x3A
#define KTRAP_FRAME_EDX                         0x3C
#define KTRAP_FRAME_ECX                         0x40
#define KTRAP_FRAME_EAX                         0x44
#define KTRAP_FRAME_PREVIOUS_MODE               0x48
#define KTRAP_FRAME_EXCEPTION_LIST              0x4C
#define KTRAP_FRAME_FS                          0x50
#define KTRAP_FRAME_RESERVED4                   0x52
#define KTRAP_FRAME_EDI                         0x54
#define KTRAP_FRAME_ESI                         0x58
#define KTRAP_FRAME_EBX                         0x5C
#define KTRAP_FRAME_EBP                         0x60
#define KTRAP_FRAME_ERROR_CODE                  0x64
#define KTRAP_FRAME_EIP                         0x68
#define KTRAP_FRAME_CS                          0x6C
#define KTRAP_FRAME_EFLAGS                      0x70
#define KTRAP_FRAME_ESP                         0x74
#define KTRAP_FRAME_SS                          0x78
#define KTRAP_FRAME_RESERVED5                   0x7A
#define KTRAP_FRAME_V86_ES                      0x7C
#define KTRAP_FRAME_RESERVED6                   0x7E
#define KTRAP_FRAME_V86_DS                      0x80
#define KTRAP_FRAME_RESERVED7                   0x82
#define KTRAP_FRAME_V86_FS                      0x84
#define KTRAP_FRAME_RESERVED8                   0x86
#define KTRAP_FRAME_V86_GS                      0x88
#define KTRAP_FRAME_RESERVED9                   0x8A
#define KTRAP_FRAME_SIZE                        0x8C
#define KTRAP_FRAME_LENGTH                      0x8C
#define KTRAP_FRAME_ALIGN                       0x04
#define FRAME_EDITED                            0xFFF8

//
// KUSER_SHARED_DATA Offsets
//
#ifdef __ASM__
#define USER_SHARED_DATA                        0xFFDF0000
#endif
#define USER_SHARED_DATA_INTERRUPT_TIME         0x8
#define USER_SHARED_DATA_SYSTEM_TIME            0x14
#define USER_SHARED_DATA_TICK_COUNT             0x320

//
// KUSER_SHARED_DATA Offsets (this stuff is trash)
//
#define KERNEL_USER_SHARED_DATA                 0x7FFE0000
#define KUSER_SHARED_PROCESSOR_FEATURES         KERNEL_USER_SHARED_DATA + 0x274
#define KUSER_SHARED_SYSCALL                    KERNEL_USER_SHARED_DATA + 0x300
#define KUSER_SHARED_SYSCALL_RET                KERNEL_USER_SHARED_DATA + 0x304
#define PROCESSOR_FEATURE_FXSR                  KUSER_SHARED_PROCESSOR_FEATURES + 0x4

//
// CONTEXT Offsets
//
#define CONTEXT_FLAGS                           0x0
#define CONTEXT_DR6                             0x14
#define CONTEXT_FLOAT_SAVE                      0x1C
#define CONTEXT_SEGGS                           0x8C
#define CONTEXT_SEGFS                           0x90
#define CONTEXT_SEGES                           0x94
#define CONTEXT_SEGDS                           0x98
#define CONTEXT_EDI                             0x9C
#define CONTEXT_ESI                             0xA0
#define CONTEXT_EBX                             0xA4
#define CONTEXT_EDX                             0xA8
#define CONTEXT_ECX                             0xAC
#define CONTEXT_EAX                             0xB0
#define CONTEXT_EBP                             0xB4
#define CONTEXT_EIP                             0xB8
#define CONTEXT_SEGCS                           0xBC
#define CONTEXT_EFLAGS                          0xC0
#define CONTEXT_ESP                             0xC4
#define CONTEXT_SEGSS                           0xC8
#define CONTEXT_FLOAT_SAVE_CONTROL_WORD         CONTEXT_FLOAT_SAVE + FP_CONTROL_WORD
#define CONTEXT_FLOAT_SAVE_STATUS_WORD          CONTEXT_FLOAT_SAVE + FP_STATUS_WORD
#define CONTEXT_FLOAT_SAVE_TAG_WORD             CONTEXT_FLOAT_SAVE + FP_TAG_WORD
#define CONTEXT_ALIGNED_SIZE                    0x2CC

//
// EXCEPTION_RECORD Offsets
//
#define EXCEPTION_RECORD_EXCEPTION_CODE         0x0
#define EXCEPTION_RECORD_EXCEPTION_FLAGS        0x4
#define EXCEPTION_RECORD_EXCEPTION_RECORD       0x8
#define EXCEPTION_RECORD_EXCEPTION_ADDRESS      0xC
#define EXCEPTION_RECORD_NUMBER_PARAMETERS      0x10
#define SIZEOF_EXCEPTION_RECORD                 0x14
#define EXCEPTION_RECORD_LENGTH                 0x50

//
// Exception types
//
#ifdef __ASM__
#define EXCEPTION_NONCONTINUABLE                0x0001
#define EXCEPTION_UNWINDING                     0x0002
#define EXCEPTION_EXIT_UNWIND                   0x0004
#define EXCEPTION_STACK_INVALID                 0x0008
#define EXCEPTION_NESTED_CALL                   0x00010
#define EXCEPTION_TARGET_UNWIND                 0x00020
#define EXCEPTION_COLLIDED_UNWIND               0x00040
#define EXCEPTION_UNWIND                        0x00066
#define EXCEPTION_EXECUTE_HANDLER               0x00001
#define EXCEPTION_CONTINUE_SEARCH               0x00000
#define EXCEPTION_CONTINUE_EXECUTION            0xFFFFFFFF
#define EXCEPTION_CHAIN_END                     0xFFFFFFFF
#endif

//
// TEB Offsets
//
#define TEB_EXCEPTION_LIST                      0x0
#define TEB_STACK_BASE                          0x4
#define TEB_STACK_LIMIT                         0x8
#define TEB_FIBER_DATA                          0x10
#define TEB_PEB                                 0x30
#define TEB_EXCEPTION_CODE                      0x1A4
#define TEB_ACTIVATION_CONTEXT_STACK_POINTER    0x1A8
#define TEB_DEALLOCATION_STACK                  0xE0C
#define TEB_GDI_BATCH_COUNT                     0xF70
#define TEB_GUARANTEED_STACK_BYTES              0xF78
#define TEB_FLS_DATA                            0xFB4

//
// PEB Offsets
//
#define PEB_KERNEL_CALLBACK_TABLE               0x2C

//
// FIBER Offsets
//
#define FIBER_PARAMETER                         0x0
#define FIBER_EXCEPTION_LIST                    0x4
#define FIBER_STACK_BASE                        0x8
#define FIBER_STACK_LIMIT                       0xC
#define FIBER_DEALLOCATION_STACK                0x10
#define FIBER_CONTEXT                           0x14
#define FIBER_GUARANTEED_STACK_BYTES            0x2E0
#define FIBER_FLS_DATA                          0x2E4
#define FIBER_ACTIVATION_CONTEXT_STACK          0x2E8
#define FIBER_CONTEXT_FLAGS                     FIBER_CONTEXT + CONTEXT_FLAGS
#define FIBER_CONTEXT_EAX                       FIBER_CONTEXT + CONTEXT_EAX
#define FIBER_CONTEXT_EBX                       FIBER_CONTEXT + CONTEXT_EBX
#define FIBER_CONTEXT_ECX                       FIBER_CONTEXT + CONTEXT_ECX
#define FIBER_CONTEXT_EDX                       FIBER_CONTEXT + CONTEXT_EDX
#define FIBER_CONTEXT_ESI                       FIBER_CONTEXT + CONTEXT_ESI
#define FIBER_CONTEXT_EDI                       FIBER_CONTEXT + CONTEXT_EDI
#define FIBER_CONTEXT_EBP                       FIBER_CONTEXT + CONTEXT_EBP
#define FIBER_CONTEXT_ESP                       FIBER_CONTEXT + CONTEXT_ESP
#define FIBER_CONTEXT_DR6                       FIBER_CONTEXT + CONTEXT_DR6
#define FIBER_CONTEXT_FLOAT_SAVE_STATUS_WORD    FIBER_CONTEXT + CONTEXT_FLOAT_SAVE_STATUS_WORD
#define FIBER_CONTEXT_FLOAT_SAVE_CONTROL_WORD   FIBER_CONTEXT + CONTEXT_FLOAT_SAVE_CONTROL_WORD
#define FIBER_CONTEXT_FLOAT_SAVE_TAG_WORD       FIBER_CONTEXT + CONTEXT_FLOAT_SAVE_TAG_WORD

//
// EFLAGS
//
#ifdef __ASM__
#define EFLAGS_TF                               0x100
#define EFLAGS_INTERRUPT_MASK                   0x200
#define EFLAGS_NESTED_TASK                      0x4000
#define EFLAGS_V86_MASK                         0x20000
#define EFLAGS_ALIGN_CHECK                      0x40000
#define EFLAGS_VIF                              0x80000
#define EFLAGS_VIP                              0x100000
#define EFLAG_SIGN                              0x8000
#define EFLAG_ZERO                              0x4000
#define EFLAG_SELECT                            (EFLAG_SIGN + EFLAG_ZERO)
#endif
#define EFLAGS_USER_SANITIZE                    0x3F4DD7

//
// CR0
//
#define CR0_PE                                  0x1
#define CR0_MP                                  0x2
#define CR0_EM                                  0x4
#define CR0_TS                                  0x8
#define CR0_ET                                  0x10
#define CR0_NE                                  0x20
#define CR0_WP                                  0x10000
#define CR0_AM                                  0x40000
#define CR0_NW                                  0x20000000
#define CR0_CD                                  0x40000000
#define CR0_PG                                  0x80000000

//
// CR4
//
#ifdef __ASM__
#define CR4_VME                                 0x1
#define CR4_PVI                                 0x2
#define CR4_TSD                                 0x4
#define CR4_DE                                  0x8
#define CR4_PSE                                 0x10
#define CR4_PAE                                 0x20
#define CR4_MCE                                 0x40
#define CR4_PGE                                 0x80
#define CR4_FXSR                                0x200
#define CR4_XMMEXCPT                            0x400
#endif

//
// DR6 and 7 Masks
//
#define DR6_LEGAL                               0xE00F
#define DR7_LEGAL                               0xFFFF0155
#define DR7_ACTIVE                              0x55
#define DR7_OVERRIDE_V                          0x04
#define DR7_RESERVED_MASK                       0xDC00
#define DR7_OVERRIDE_MASK                       0xF0000

//
// Usermode callout frame definitions
//
#define CBSTACK_STACK                           0x0
#define CBSTACK_TRAP_FRAME                      0x4
#define CBSTACK_CALLBACK_STACK                  0x8
#define CBSTACK_EBP                             0x18
#define CBSTACK_RESULT                          0x20
#define CBSTACK_RESULT_LENGTH                   0x24

//
// NTSTATUS and Bugcheck Codes
//
#ifdef __ASM__
#define STATUS_ACCESS_VIOLATION                 0xC0000005
#define STATUS_IN_PAGE_ERROR                    0xC0000006
#define STATUS_GUARD_PAGE_VIOLATION             0x80000001
#define STATUS_PRIVILEGED_INSTRUCTION           0xC0000096
#define STATUS_STACK_OVERFLOW                   0xC00000FD
#define KI_EXCEPTION_ACCESS_VIOLATION           0x10000004
#define STATUS_INVALID_SYSTEM_SERVICE           0xC000001C
#define STATUS_NO_CALLBACK_ACTIVE               0xC0000258
#define STATUS_CALLBACK_POP_STACK               0xC0000423
#define STATUS_ARRAY_BOUNDS_EXCEEDED            0xC000008C
#define STATUS_ILLEGAL_INSTRUCTION              0xC000001D
#define STATUS_INVALID_LOCK_SEQUENCE            0xC000001E
#define STATUS_BREAKPOINT                       0x80000003
#define STATUS_SINGLE_STEP                      0x80000004
#define STATUS_INTEGER_DIVIDE_BY_ZERO           0xC0000094
#define STATUS_INTEGER_OVERFLOW                 0xC0000095
#define STATUS_FLOAT_DENORMAL_OPERAND           0xC000008D
#define STATUS_FLOAT_DIVIDE_BY_ZERO             0xC000008E
#define STATUS_FLOAT_INEXACT_RESULT             0xC000008F
#define STATUS_FLOAT_INVALID_OPERATION          0xC0000090
#define STATUS_FLOAT_OVERFLOW                   0xC0000091
#define STATUS_FLOAT_STACK_CHECK                0xC0000092
#define STATUS_FLOAT_UNDERFLOW                  0xC0000093
#define STATUS_FLOAT_MULTIPLE_FAULTS            0xC00002B4
#define STATUS_FLOAT_MULTIPLE_TRAPS             0xC00002B5
#define APC_INDEX_MISMATCH                      0x01
#define IRQL_NOT_GREATER_OR_EQUAL               0x09
#define IRQL_NOT_LESS_OR_EQUAL                  0x0A
#define TRAP_CAUSE_UNKNOWN                      0x12
#define KMODE_EXCEPTION_NOT_HANDLED             0x13
#define IRQL_GT_ZERO_AT_SYSTEM_SERVICE          0x4A
#define UNEXPECTED_KERNEL_MODE_TRAP             0x7F
#define ATTEMPTED_SWITCH_FROM_DPC               0xB8
#define HARDWARE_INTERRUPT_STORM                0xF2

//
// IRQL Levels
//
#define PASSIVE_LEVEL                           0x0
#define APC_LEVEL                               0x1
#define DISPATCH_LEVEL                          0x2
#define CLOCK2_LEVEL                            0x1C
#define HIGH_LEVEL                              0x1F

//
// Quantum Decrements
//
#define CLOCK_QUANTUM_DECREMENT                 0x3
#endif

//
// System Call Table definitions
//
#define NUMBER_SERVICE_TABLES                   0x0002
#define SERVICE_NUMBER_MASK                     0x0FFF
#define SERVICE_TABLE_SHIFT                     0x0008
#define SERVICE_TABLE_MASK                      0x0010
#define SERVICE_TABLE_TEST                      0x0010
#define SERVICE_DESCRIPTOR_BASE                 0x0000
#define SERVICE_DESCRIPTOR_COUNT                0x0004
#define SERVICE_DESCRIPTOR_LIMIT                0x0008
#define SERVICE_DESCRIPTOR_NUMBER               0x000C
#define SERVICE_DESCRIPTOR_LENGTH               0x0010

//
// VDM State Pointer
//
#define FIXED_NTVDMSTATE_LINEAR_PC_AT           0x714

//
// Machine types
//
#ifdef __ASM__
#define MACHINE_TYPE_ISA                        0x0000
#define MACHINE_TYPE_EISA                       0x0001
#define MACHINE_TYPE_MCA                        0x0002

//
// Kernel Feature Bits
//
#define KF_RDTSC                                0x00000002

//
// Kernel Stack Size
//
#define KERNEL_STACK_SIZE                       0x3000
#endif

//
// Generic Definitions
//
#define PRIMARY_VECTOR_BASE                     0x30 // FIXME: HACK
#define MAXIMUM_IDTVECTOR                       0xFF
#endif // !_ASM_H





