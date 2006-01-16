/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    asm.h

Abstract:

    ASM Offsets for dealing with de-referencing structures in registers.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _ASM_H
#define _ASM_H

//
// Selector Names
//
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
#define KTHREAD_NPX_STATE                       0x4D
#define KTHREAD_STATE                           0x4C
#define KTHREAD_ALERTED                         0x5E
#define KTHREAD_APCSTATE_PROCESS                0x28 + 0x10
#define KTHREAD_PENDING_USER_APC                0x28 + 0x16
#define KTHREAD_PENDING_KERNEL_APC              0x28 + 0x15
#define KTHREAD_CONTEXT_SWITCHES                0x48
#define KTHREAD_WAIT_IRQL                       0x4E
#define KTHREAD_SERVICE_TABLE                   0x118
#define KTHREAD_PREVIOUS_MODE                   0xD7
#define KTHREAD_COMBINED_APC_DISABLE            0x70
#define KTHREAD_LARGE_STACK                     0x107
#define KTHREAD_TRAP_FRAME                      0x110
#define KTHREAD_CALLBACK_STACK                  0x114
#define KTHREAD_APC_STATE_INDEX                 0x11C
#define KTHREAD_STACK_BASE                      0x158

//
// KPROCESS Offsets
//
#define KPROCESS_DIRECTORY_TABLE_BASE           0x18
#define KPROCESS_LDT_DESCRIPTOR0                0x20
#define KPROCESS_LDT_DESCRIPTOR1                0x24
#define KPROCESS_IOPM_OFFSET                    0x30

//
// KPCR Offsets
//
#define KPCR_EXCEPTION_LIST                     0x0
#define KPCR_INITIAL_STACK                      0x4
#define KPCR_STACK_LIMIT                        0x8
#define KPCR_SET_MEMBER_COPY                    0x14
#define KPCR_TEB                                0x18
#define KPCR_SELF                               0x1C
#define KPCR_PRCB                               0x20
#define KPCR_IRQL                               0x24
#define KPCR_KD_VERSION_BLOCK                   0x34
#define KPCR_GDT                                0x3C
#define KPCR_TSS                                0x40
#define KPCR_SET_MEMBER                         0x48
#define KPCR_NUMBER                             0x51
#define KPCR_CURRENT_THREAD                     0x124
#define KPCR_PROCESSOR_NUMBER                   0x130
#define KPCR_PRCB_SET_MEMBER                    0x134
#define KPCR_NPX_THREAD                         0x2F4
#define KPCR_DR6                                0x428
#define KPCR_DR7                                0x42C
#define KPCR_SYSTEM_CALLS                       0x6B8

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
#define FN_CONTROL_WORD                         0x0
#define FN_STATUS_WORD                          0x4
#define FN_TAG_WORD                             0x8
#define FN_DATA_SELECTOR                        0x18
#define FN_CR0_NPX_STATE                        0x20C
#define SIZEOF_FX_SAVE_AREA                     528
#define NPX_FRAME_LENGTH                        0x210

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

//
// KUSER_SHARED_DATA Offsets
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
#define CONTEXT_FLOAT_SAVE_CONTROL_WORD         CONTEXT_FLOAT_SAVE + FN_CONTROL_WORD
#define CONTEXT_FLOAT_SAVE_STATUS_WORD          CONTEXT_FLOAT_SAVE + FN_STATUS_WORD
#define CONTEXT_FLOAT_SAVE_TAG_WORD             CONTEXT_FLOAT_SAVE + FN_TAG_WORD

//
// EXCEPTION_RECORD Offsets
//
#define EXCEPTION_RECORD_EXCEPTION_CODE         0x0
#define EXCEPTION_RECORD_EXCEPTION_FLAGS        0x4
#define EXCEPTION_RECORD_EXCEPTION_RECORD       0x8
#define EXCEPTION_RECORD_EXCEPTION_ADDRESS      0xC
#define EXCEPTION_RECORD_NUMBER_PARAMETERS      0x10
#define SIZEOF_EXCEPTION_RECORD                 0x14

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
#define EFLAGS_TF                               0x100
#define EFLAGS_INTERRUPT_MASK                   0x200
#define EFLAGS_NESTED_TASK                      0x4000
#define EFLAGS_V86_MASK                         0x20000
#define EFLAGS_ALIGN_CHECK                      0x40000
#define EFLAGS_VIF                              0x80000
#define EFLAGS_VIP                              0x100000
#define EFLAG_SIGN                              0x8000
#define EFLAG_ZERO                              0x4000
#ifndef EFLAG_SELECT
#define EFLAG_SELECT                            (EFLAG_SIGN + EFLAG_ZERO)
#endif

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
// Usermode callout frame definitions
//
#define CBSTACK_STACK                           0x0
#define CBSTACK_TRAP_FRAME                      0x4
#define CBSTACK_CALLBACK_STACK                  0x8
#define CBSTACK_RESULT                          0x20
#define CBSTACK_RESULT_LENGTH                   0x24

//
// Generic Definitions
//
#define MAXIMUM_IDTVECTOR                       0xFF
#endif // !_ASM_H

