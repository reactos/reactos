/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    amd64/asm.h

Abstract:

    ASM Offsets for dealing with de-referencing structures in registers.

Author:

    Timo Kreuzer (timo.kreuzer@reactos.org)   06-Sep-2008

--*/
#ifndef _ASM_AMD64_H
#define _ASM_AMD64_H

#ifndef HEX
 #ifdef _USE_ML
  #define HEX(x) x##h
 #else
  #define HEX(val) 0x0##val
 #endif
#endif

#define SIZEOF_FX_SAVE_AREA 528 // HACK

//
// CPU Modes
//
#define KernelMode                              HEX(0)
#define UserMode                                HEX(1)

//
// KTSS Offsets
//
#define KTSS64_RSP0                             HEX(04)
#define KTSS64_RSP1                             HEX(0c)
#define KTSS64_RSP2                             HEX(14)
#define KTSS64_IST                              HEX(1c)
#define KTSS64_IO_MAP_BASE                      HEX(66)

//
// KTHREAD Offsets
//
#define KTHREAD_DEBUG_ACTIVE                    HEX(03)
#define KTHREAD_INITIAL_STACK                   HEX(28)
#define KTHREAD_STACK_LIMIT                     HEX(30)
#define KTHREAD_WAIT_IRQL                       HEX(156)

//
// KPRCB Offsets
//
#define KPRCB_CurrentThread HEX(08)


//
// KPCR Offsets
//
#define KPCR_TSS_BASE                           HEX(08)
#define KPCR_SELF                               HEX(18)
#define KPCR_STALL_SCALE_FACTOR                 HEX(64)
#define KPCR_PRCB HEX(180

//
// KTRAP_FRAME Offsets
//
#define KTRAP_FRAME_P1Home HEX(00)
#define KTRAP_FRAME_P2Home HEX(08)
#define KTRAP_FRAME_P3Home HEX(10)
#define KTRAP_FRAME_P4Home HEX(18)
#define KTRAP_FRAME_P5 HEX(20)
#define KTRAP_FRAME_PreviousMode HEX(28)
#define KTRAP_FRAME_PreviousIrql HEX(29)
#define KTRAP_FRAME_FaultIndicator HEX(2A)
#define KTRAP_FRAME_ExceptionActive HEX(2B)
#define KTRAP_FRAME_MxCsr HEX(2C)
#define KTRAP_FRAME_Rax HEX(30)
#define KTRAP_FRAME_Rcx HEX(38)
#define KTRAP_FRAME_Rdx HEX(40)
#define KTRAP_FRAME_R8 HEX(48)
#define KTRAP_FRAME_R9 HEX(50)
#define KTRAP_FRAME_R10 HEX(58)
#define KTRAP_FRAME_R11 HEX(60)
#define KTRAP_FRAME_GsBase HEX(68)
#define KTRAP_FRAME_Xmm0 HEX(70)
#define KTRAP_FRAME_Xmm1 HEX(80)
#define KTRAP_FRAME_Xmm2 HEX(90)
#define KTRAP_FRAME_Xmm3 HEX(A0)
#define KTRAP_FRAME_Xmm4 HEX(B0)
#define KTRAP_FRAME_Xmm5 HEX(C0)
#define KTRAP_FRAME_FaultAddress HEX(D0)
#define KTRAP_FRAME_Dr0 HEX(D8)
#define KTRAP_FRAME_Dr1 HEX(E0)
#define KTRAP_FRAME_Dr2 HEX(E8)
#define KTRAP_FRAME_Dr3 HEX(F0)
#define KTRAP_FRAME_Dr6 HEX(F8)
#define KTRAP_FRAME_Dr7 HEX(100)
#define KTRAP_FRAME_DebugControl HEX(108)
#define KTRAP_FRAME_LastBranchToRip HEX(110)
#define KTRAP_FRAME_LastBranchFromRip HEX(118)
#define KTRAP_FRAME_LastExceptionToRip HEX(120)
#define KTRAP_FRAME_LastExceptionFromRip HEX(128)
#define KTRAP_FRAME_SegDs HEX(130)
#define KTRAP_FRAME_SegEs HEX(132)
#define KTRAP_FRAME_SegFs HEX(134)
#define KTRAP_FRAME_SegGs HEX(136)
#define KTRAP_FRAME_TrapFrame HEX(138)
#define KTRAP_FRAME_Rbx HEX(140)
#define KTRAP_FRAME_Rdi HEX(148)
#define KTRAP_FRAME_Rsi HEX(150)
#define KTRAP_FRAME_Rbp HEX(158)
#define KTRAP_FRAME_ErrorCode HEX(160)
#define KTRAP_FRAME_Rip HEX(168)
#define KTRAP_FRAME_SegCs HEX(170)
#define KTRAP_FRAME_Logging HEX(173)
#define KTRAP_FRAME_EFlags HEX(178)
#define KTRAP_FRAME_Rsp HEX(180)
#define KTRAP_FRAME_SegSs HEX(188)
#define KTRAP_FRAME_CodePatchCycle HEX(18c)
#define SIZE_KTRAP_FRAME HEX(190)
#define KTRAP_FRAME_ALIGN                       HEX(10)
#define KTRAP_FRAME_LENGTH                      HEX(190)

//
// CONTEXT Offsets
//
#define CONTEXT_P1Home 0
#define CONTEXT_P2Home HEX(08)
#define CONTEXT_P3Home HEX(10)
#define CONTEXT_P4Home HEX(18)
#define CONTEXT_P5Home HEX(20)
#define CONTEXT_P6Home HEX(28)
#define CONTEXT_ContextFlags HEX(30)
#define CONTEXT_MxCsr HEX(34)
#define CONTEXT_SegCs HEX(38)
#define CONTEXT_SegDs HEX(3a)
#define CONTEXT_SegEs HEX(3c)
#define CONTEXT_SegFs HEX(3e)
#define CONTEXT_SegGs HEX(40)
#define CONTEXT_SegSs HEX(42)
#define CONTEXT_EFlags HEX(44)
#define CONTEXT_Dr0 HEX(48)
#define CONTEXT_Dr1 HEX(50)
#define CONTEXT_Dr2 HEX(58)
#define CONTEXT_Dr3 HEX(60)
#define CONTEXT_Dr6 HEX(68)
#define CONTEXT_Dr7 HEX(70)
#define CONTEXT_Rax HEX(78)
#define CONTEXT_Rcx HEX(80)
#define CONTEXT_Rdx HEX(88)
#define CONTEXT_Rbx HEX(90)
#define CONTEXT_Rsp HEX(98)
#define CONTEXT_Rbp HEX(a0)
#define CONTEXT_Rsi HEX(a8)
#define CONTEXT_Rdi HEX(b0)
#define CONTEXT_R8  HEX(b8)
#define CONTEXT_R9  HEX(c0)
#define CONTEXT_R10 HEX(c8)
#define CONTEXT_R11 HEX(d0)
#define CONTEXT_R12 HEX(d8)
#define CONTEXT_R13 HEX(e0)
#define CONTEXT_R14 HEX(e8)
#define CONTEXT_R15 HEX(f0)
#define CONTEXT_Rip HEX(f8)
#define CONTEXT_Header HEX(100)
#define CONTEXT_Legacy HEX(120)
#define CONTEXT_Xmm0 HEX(1a0)
#define CONTEXT_Xmm1 HEX(1b0)
#define CONTEXT_Xmm2 HEX(1c0)
#define CONTEXT_Xmm3 HEX(1d0)
#define CONTEXT_Xmm4 HEX(1e0)
#define CONTEXT_Xmm5 HEX(1f0)
#define CONTEXT_Xmm6 HEX(200)
#define CONTEXT_Xmm7 HEX(210)
#define CONTEXT_Xmm8 HEX(220)
#define CONTEXT_Xmm9 HEX(230)
#define CONTEXT_Xmm10 HEX(240)
#define CONTEXT_Xmm11 HEX(250)
#define CONTEXT_Xmm12 HEX(260)
#define CONTEXT_Xmm13 HEX(270)
#define CONTEXT_Xmm14 HEX(280)
#define CONTEXT_Xmm15 HEX(290)
#define CONTEXT_VectorRegister HEX(300)
#define CONTEXT_VectorControl HEX(4a0)
#define CONTEXT_DebugControl HEX(4a8)
#define CONTEXT_LastBranchToRip HEX(4b0)
#define CONTEXT_LastBranchFromRip HEX(4b8)
#define CONTEXT_LastExceptionToRip HEX(4c0)
#define CONTEXT_LastExceptionFromRip HEX(4c8)

//
// KEXCEPTION_FRAME offsets
//
#define KEXCEPTION_FRAME_P1Home HEX(000)
#define KEXCEPTION_FRAME_P2Home HEX(008)
#define KEXCEPTION_FRAME_P3Home HEX(010)
#define KEXCEPTION_FRAME_P4Home HEX(018)
#define KEXCEPTION_FRAME_P5 HEX(020)
#define KEXCEPTION_FRAME_InitialStack HEX(028)
#define KEXCEPTION_FRAME_Xmm6 HEX(030)
#define KEXCEPTION_FRAME_Xmm7 HEX(040)
#define KEXCEPTION_FRAME_Xmm8 HEX(050)
#define KEXCEPTION_FRAME_Xmm9 HEX(060)
#define KEXCEPTION_FRAME_Xmm10 HEX(070)
#define KEXCEPTION_FRAME_Xmm11 HEX(080)
#define KEXCEPTION_FRAME_Xmm12 HEX(090)
#define KEXCEPTION_FRAME_Xmm13 HEX(0A0)
#define KEXCEPTION_FRAME_Xmm14 HEX(0B0)
#define KEXCEPTION_FRAME_Xmm15 HEX(0C0)
#define KEXCEPTION_FRAME_TrapFrame HEX(0D0)
#define KEXCEPTION_FRAME_CallbackStack HEX(0D8)
#define KEXCEPTION_FRAME_OutputBuffer HEX(0E0)
#define KEXCEPTION_FRAME_OutputLength HEX(0E8)
#define KEXCEPTION_FRAME_MxCsr HEX(0F0)
#define KEXCEPTION_FRAME_Rbp HEX(0F8)
#define KEXCEPTION_FRAME_Rbx HEX(100)
#define KEXCEPTION_FRAME_Rdi HEX(108)
#define KEXCEPTION_FRAME_Rsi HEX(110)
#define KEXCEPTION_FRAME_R12 HEX(118)
#define KEXCEPTION_FRAME_R13 HEX(120)
#define KEXCEPTION_FRAME_R14 HEX(128)
#define KEXCEPTION_FRAME_R15 HEX(130)
#define KEXCEPTION_FRAME_Return HEX(138)
#define SIZE_KEXCEPTION_FRAME HEX(140)


//
// EXCEPTION_RECORD Offsets
//
#define EXCEPTION_RECORD_ExceptionCode HEX(00)
#define EXCEPTION_RECORD_ExceptionFlags HEX(04)
#define EXCEPTION_RECORD_ExceptionRecord HEX(08)
#define EXCEPTION_RECORD_ExceptionAddress HEX(10)
#define EXCEPTION_RECORD_NumberParameters HEX(18)
#define EXCEPTION_RECORD_ExceptionInformation HEX(20)
#define SIZE_EXCEPTION_RECORD HEX(98)

//
// CR0
//
#define CR0_PE                                  HEX(1)
#define CR0_MP                                  HEX(2)
#define CR0_EM                                  HEX(4)
#define CR0_TS                                  HEX(8)
#define CR0_ET                                  HEX(10)
#define CR0_NE                                  HEX(20)
#define CR0_WP                                  HEX(10000)
#define CR0_AM                                  HEX(40000)
#define CR0_NW                                  HEX(20000000)
#define CR0_CD                                  HEX(40000000)
#define CR0_PG                                  HEX(80000000)

#ifdef _ASM_
//
// CR4
//
#define CR4_VME                                 HEX(1)
#define CR4_PVI                                 HEX(2)
#define CR4_TSD                                 HEX(4)
#define CR4_DE                                  HEX(8)
#define CR4_PSE                                 HEX(10)
#define CR4_PAE                                 HEX(20)
#define CR4_MCE                                 HEX(40)
#define CR4_PGE                                 HEX(80)
#define CR4_FXSR                                HEX(200)
#define CR4_XMMEXCPT                            HEX(400)
#endif

//
// Generic Definitions
//
#define PRIMARY_VECTOR_BASE                     HEX(30)
#define MAXIMUM_IDTVECTOR                       HEX(FF)

//
// Usermode callout frame definitions
//
#define CBSTACK_STACK                           HEX(0)
#define CBSTACK_TRAP_FRAME                      HEX(8)
#define CBSTACK_CALLBACK_STACK                  HEX(10)
#define CBSTACK_RBP                             HEX(18)
#define CBSTACK_RESULT                          HEX(20)
#define CBSTACK_RESULT_LENGTH                   HEX(28)


/* Following ones are ASM only! ***********************************************/

#ifdef __ASM__

//
// PCR Access
//
#define PCR                                     gs:

//
// EFLAGS
//
#define EFLAGS_TF                               HEX(100)
#define EFLAGS_INTERRUPT_MASK                   HEX(200)
#define EFLAGS_NESTED_TASK                      HEX(4000)
#define EFLAGS_V86_MASK                         HEX(20000)
#define EFLAGS_ALIGN_CHECK                      HEX(40000)
#define EFLAGS_VIF                              HEX(80000)
#define EFLAGS_VIP                              HEX(100000)
#define EFLAG_SIGN                              HEX(8000)
#define EFLAG_ZERO                              HEX(4000)
#define EFLAG_SELECT                            (EFLAG_SIGN + EFLAG_ZERO)
#define EFLAGS_USER_SANITIZE                    HEX(3F4DD7)

//
// Exception codes
//
#define EXCEPTION_DIVIDED_BY_ZERO     HEX(00000)
#define EXCEPTION_DEBUG               HEX(00001)
#define EXCEPTION_NMI                 HEX(00002)
#define EXCEPTION_INT3                HEX(00003)
#define EXCEPTION_BOUND_CHECK         HEX(00005)
#define EXCEPTION_INVALID_OPCODE      HEX(00006)
#define EXCEPTION_NPX_NOT_AVAILABLE   HEX(00007)
#define EXCEPTION_DOUBLE_FAULT        HEX(00008)
#define EXCEPTION_NPX_OVERRUN         HEX(00009)
#define EXCEPTION_INVALID_TSS         HEX(0000A)
#define EXCEPTION_SEGMENT_NOT_PRESENT HEX(0000B)
#define EXCEPTION_STACK_FAULT         HEX(0000C)
#define EXCEPTION_GP_FAULT            HEX(0000D)
#define EXCEPTION_RESERVED_TRAP       HEX(0000F)
#define EXCEPTION_NPX_ERROR           HEX(00010)
#define EXCEPTION_ALIGNMENT_CHECK     HEX(00011)

//
// NTSTATUS values
//
#define STATUS_ACCESS_VIOLATION                 HEX(C0000005)
#define STATUS_IN_PAGE_ERROR                    HEX(C0000006)
#define STATUS_GUARD_PAGE_VIOLATION             HEX(80000001)
#define STATUS_PRIVILEGED_INSTRUCTION           HEX(C0000096)
#define STATUS_STACK_OVERFLOW                   HEX(C00000FD)
#define KI_EXCEPTION_ACCESS_VIOLATION           HEX(10000004)
#define STATUS_INVALID_SYSTEM_SERVICE           HEX(C000001C)
#define STATUS_NO_CALLBACK_ACTIVE               HEX(C0000258)
#define STATUS_CALLBACK_POP_STACK               HEX(C0000423)
#define STATUS_ARRAY_BOUNDS_EXCEEDED            HEX(C000008C)
#define STATUS_ILLEGAL_INSTRUCTION              HEX(C000001D)
#define STATUS_INVALID_LOCK_SEQUENCE            HEX(C000001E)
#define STATUS_BREAKPOINT                       HEX(80000003)
#define STATUS_SINGLE_STEP                      HEX(80000004)
#define STATUS_INTEGER_DIVIDE_BY_ZERO           HEX(C0000094)
#define STATUS_INTEGER_OVERFLOW                 HEX(C0000095)
#define STATUS_FLOAT_DENORMAL_OPERAND           HEX(C000008D)
#define STATUS_FLOAT_DIVIDE_BY_ZERO             HEX(C000008E)
#define STATUS_FLOAT_INEXACT_RESULT             HEX(C000008F)
#define STATUS_FLOAT_INVALID_OPERATION          HEX(C0000090)
#define STATUS_FLOAT_OVERFLOW                   HEX(C0000091)
#define STATUS_FLOAT_STACK_CHECK                HEX(C0000092)
#define STATUS_FLOAT_UNDERFLOW                  HEX(C0000093)
#define STATUS_FLOAT_MULTIPLE_FAULTS            HEX(C00002B4)
#define STATUS_FLOAT_MULTIPLE_TRAPS             HEX(C00002B5)
#define STATUS_ASSERTION_FAILURE                HEX(C0000420)

//
// Bugcheck Codes
//
#define APC_INDEX_MISMATCH                      HEX(01)
#define IRQL_NOT_GREATER_OR_EQUAL               HEX(09)
#define IRQL_NOT_LESS_OR_EQUAL                  HEX(0A)
#define TRAP_CAUSE_UNKNOWN                      HEX(12)
#define KMODE_EXCEPTION_NOT_HANDLED             HEX(13)
#define IRQL_GT_ZERO_AT_SYSTEM_SERVICE          HEX(4A)
#define UNEXPECTED_KERNEL_MODE_TRAP             HEX(7F)
#define ATTEMPTED_SWITCH_FROM_DPC               HEX(B8)
#define HARDWARE_INTERRUPT_STORM                HEX(F2)

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
#define CLOCK_QUANTUM_DECREMENT                 HEX(3)

//
// Machine types
//
#define MACHINE_TYPE_ISA                        HEX(0000)
#define MACHINE_TYPE_EISA                       HEX(0001)
#define MACHINE_TYPE_MCA                        HEX(0002)

//
// Kernel Feature Bits
//
#define KF_RDTSC                                HEX(00000002)

//
// Kernel Stack Size
//
#define KERNEL_STACK_SIZE                       HEX(6000)

#endif // __ASM__

#endif // !_ASM_AMD64_H

