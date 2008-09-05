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
// KPCR Offsets
//
#define KPCR_TSS_BASE                           0x08
#define KPCR_SELF                               0x18
#define KPCR_STALL_SCALE_FACTOR                 0x64

//
// Trap Frame Offsets
//
#define KTRAP_FRAME_ALIGN                       0x10
#define KTRAP_FRAME_LENGTH                      0x190

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
// Generic Definitions
//
#define PRIMARY_VECTOR_BASE                     0x30
#define MAXIMUM_IDTVECTOR                       0xFF


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

