/*++ NDK Version: 0095

Copyright (c) Timo Kreuzer.  All rights reserved.

Header Name:

    amd64/asm.h

Abstract:

    ASM Offsets for dealing with de-referencing structures in registers.

Author:

    Timo Kreuzer (timo.kreuzer@reactos.org)   06-Sep-2008

--*/
#ifndef _ASM_AMD64_H
#define _ASM_AMD64_H


#define SIZEOF_FX_SAVE_AREA 528 // HACK

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
// KTSS Offsets
//
#define KTSS64_RSP0                             0x04
#define KTSS64_RSP1                             0x0c
#define KTSS64_RSP2                             0x14
#define KTSS64_IST                              0x1c   
#define KTSS64_IO_MAP_BASE                      0x66

//
// KTHREAD Offsets
//
#define KTHREAD_DEBUG_ACTIVE                    0x03
#define KTHREAD_INITIAL_STACK                   0x28
#define KTHREAD_STACK_LIMIT                     0x30
#define KTHREAD_WAIT_IRQL                       0x156

//
// KPRCB Offsets
//
#define KPRCB_CurrentThread 0x08


//
// KPCR Offsets
//
#define KPCR_TSS_BASE                           0x08
#define KPCR_SELF                               0x18
#define KPCR_STALL_SCALE_FACTOR                 0x64
#define KPCR_PRCB 0x180

//
// KTRAP_FRAME Offsets
//
#define KTRAP_FRAME_P1Home 0x00
#define KTRAP_FRAME_P2Home 0x08
#define KTRAP_FRAME_P3Home 0x10
#define KTRAP_FRAME_P4Home 0x18
#define KTRAP_FRAME_P5 0x20
#define KTRAP_FRAME_PreviousMode 0x28
#define KTRAP_FRAME_PreviousIrql 0x29
#define KTRAP_FRAME_FaultIndicator 0x2A
#define KTRAP_FRAME_ExceptionActive 0x2B
#define KTRAP_FRAME_MxCsr 0x2C
#define KTRAP_FRAME_Rax 0x30
#define KTRAP_FRAME_Rcx 0x38
#define KTRAP_FRAME_Rdx 0x40
#define KTRAP_FRAME_R8 0x48
#define KTRAP_FRAME_R9 0x50
#define KTRAP_FRAME_R10 0x58
#define KTRAP_FRAME_R11 0x60
#define KTRAP_FRAME_Spare0 0x68
#define KTRAP_FRAME_Xmm0 0x70
#define KTRAP_FRAME_Xmm1 0x80
#define KTRAP_FRAME_Xmm2 0x90
#define KTRAP_FRAME_Xmm3 0xA0
#define KTRAP_FRAME_Xmm4 0xB0
#define KTRAP_FRAME_Xmm5 0xC0
#define KTRAP_FRAME_FaultAddress 0xD0
#define KTRAP_FRAME_Dr0 0xD8
#define KTRAP_FRAME_Dr1 0xE0
#define KTRAP_FRAME_Dr2 0xE8
#define KTRAP_FRAME_Dr3 0xF0
#define KTRAP_FRAME_Dr6 0xF8
#define KTRAP_FRAME_Dr7 0x100
#define KTRAP_FRAME_DebugControl 0x108
#define KTRAP_FRAME_LastBranchToRip 0x110
#define KTRAP_FRAME_LastBranchFromRip 0x118
#define KTRAP_FRAME_LastExceptionToRip 0x120
#define KTRAP_FRAME_LastExceptionFromRip 0x128
#define KTRAP_FRAME_SegDs 0x130
#define KTRAP_FRAME_SegEs 0x132
#define KTRAP_FRAME_SegFs 0x134
#define KTRAP_FRAME_SegGs 0x136
#define KTRAP_FRAME_TrapFrame 0x138
#define KTRAP_FRAME_Rbx 0x140
#define KTRAP_FRAME_Rdi 0x148
#define KTRAP_FRAME_Rsi 0x150
#define KTRAP_FRAME_Rbp 0x158
#define KTRAP_FRAME_ErrorCode 0x160
#define KTRAP_FRAME_Rip 0x168
#define KTRAP_FRAME_SegCs 0x170
#define KTRAP_FRAME_EFlags 0x178
#define KTRAP_FRAME_Rsp 0x180
#define KTRAP_FRAME_SegSs 0x188
#define SIZE_KTRAP_FRAME 0x190
#define KTRAP_FRAME_ALIGN                       0x10
#define KTRAP_FRAME_LENGTH                      0x190

//
// CONTEXT Offsets
//
#define CONTEXT_P1Home 0
#define CONTEXT_P2Home 0x08
#define CONTEXT_P3Home 0x10
#define CONTEXT_P4Home 0x18
#define CONTEXT_P5Home 0x20
#define CONTEXT_P6Home 0x28
#define CONTEXT_ContextFlags 0x30
#define CONTEXT_MxCsr 0x34
#define CONTEXT_SegCs 0x38
#define CONTEXT_SegDs 0x3a
#define CONTEXT_SegEs 0x3c
#define CONTEXT_SegFs 0x3e
#define CONTEXT_SegGs 0x40
#define CONTEXT_SegSs 0x42
#define CONTEXT_EFlags 0x44
#define CONTEXT_Dr0 0x48
#define CONTEXT_Dr1 0x50
#define CONTEXT_Dr2 0x58
#define CONTEXT_Dr3 0x60
#define CONTEXT_Dr6 0x68
#define CONTEXT_Dr7 0x70
#define CONTEXT_Rax 0x78
#define CONTEXT_Rcx 0x80
#define CONTEXT_Rdx 0x88
#define CONTEXT_Rbx 0x90
#define CONTEXT_Rsp 0x98
#define CONTEXT_Rbp 0xa0
#define CONTEXT_Rsi 0xa8
#define CONTEXT_Rdi 0xb0
#define CONTEXT_R8  0xb8
#define CONTEXT_R9  0xc0
#define CONTEXT_R10 0xc8
#define CONTEXT_R11 0xd0
#define CONTEXT_R12 0xd8
#define CONTEXT_R13 0xe0
#define CONTEXT_R14 0xe8
#define CONTEXT_R15 0xf0
#define CONTEXT_Rip 0xf8
#define CONTEXT_Header 0x100
#define CONTEXT_Legacy 0x120
#define CONTEXT_Xmm0 0x1a0
#define CONTEXT_Xmm1 0x1b0
#define CONTEXT_Xmm2 0x1c0
#define CONTEXT_Xmm3 0x1d0
#define CONTEXT_Xmm4 0x1e0
#define CONTEXT_Xmm5 0x1f0
#define CONTEXT_Xmm6 0x200
#define CONTEXT_Xmm7 0x210
#define CONTEXT_Xmm8 0x220
#define CONTEXT_Xmm9 0x230
#define CONTEXT_Xmm10 0x240
#define CONTEXT_Xmm11 0x250
#define CONTEXT_Xmm12 0x260
#define CONTEXT_Xmm13 0x270
#define CONTEXT_Xmm14 0x280
#define CONTEXT_Xmm15 0x290
#define CONTEXT_VectorRegister 0x300
#define CONTEXT_VectorControl 0x4a0
#define CONTEXT_DebugControl 0x4a8
#define CONTEXT_LastBranchToRip 0x4b0
#define CONTEXT_LastBranchFromRip 0x4b8
#define CONTEXT_LastExceptionToRip 0x4c0
#define CONTEXT_LastExceptionFromRip 0x4c8

//
// EXCEPTION_RECORD Offsets
//
#define EXCEPTION_RECORD_ExceptionCode 0x00
#define EXCEPTION_RECORD_ExceptionFlags 0x04
#define EXCEPTION_RECORD_ExceptionRecord 0x08
#define EXCEPTION_RECORD_ExceptionAddress 0x10
#define EXCEPTION_RECORD_NumberParameters 0x18
#define EXCEPTION_RECORD_ExceptionInformation 0x20
#define SIZE_EXCEPTION_RECORD 0x98

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

//
// Generic Definitions
//
#define PRIMARY_VECTOR_BASE                     0x30
#define MAXIMUM_IDTVECTOR                       0xFF

//
// Usermode callout frame definitions
//
#define CBSTACK_STACK                           0x0
#define CBSTACK_TRAP_FRAME                      0x8
#define CBSTACK_CALLBACK_STACK                  0x10
#define CBSTACK_RBP                             0x18
#define CBSTACK_RESULT                          0x20
#define CBSTACK_RESULT_LENGTH                   0x28


/* Following ones are ASM only! ***********************************************/

#ifdef __ASM__

//
// PCR Access
//
#define PCR                                     gs:

//
// EFLAGS
//
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
#define EFLAGS_USER_SANITIZE                    0x3F4DD7

//
// NTSTATUS and Bugcheck Codes
//
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
#define PASSIVE_LEVEL                              0
#define LOW_LEVEL                                  0
#define APC_LEVEL                                  1
#define DISPATCH_LEVEL                             2
#define CLOCK_LEVEL                               13
#define IPI_LEVEL                                 14
#define POWER_LEVEL                               14
#define PROFILE_LEVEL                             15
#define HIGH_LEVEL                                15

//
// Quantum Decrements
//
#define CLOCK_QUANTUM_DECREMENT                 0x3

//
// Machine types
//
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
#define KERNEL_STACK_SIZE                       0x6000

#endif // __ASM__

#endif // !_ASM_AMD64_H

