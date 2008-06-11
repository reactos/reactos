/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/arm/ksarm.h
 * PURPOSE:         Definitions and offsets for ARM Assembly and C code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#ifdef _ASM_

/*
 * CPSR Values
 */
.equ CPSR_THUMB_ENABLE,    0x20   
.equ CPSR_FIQ_DISABLE,     0x40
.equ CPSR_IRQ_DISABLE,     0x80
.equ CPSR_USER_MODE,       0x10
.equ CPSR_FIQ_MODE,        0x11
.equ CPSR_IRQ_MODE,        0x12
.equ CPSR_SVC_MODE,        0x13
.equ CPSR_ABORT_MODE,      0x17
.equ CPSR_UND_MODE,        0x1B
.equ CPSR_MODES,           0x1F


/*
 * C1 Register Values
 */
.equ C1_MMU_CONTROL,       0x01
.equ C1_ALIGNMENT_CONTROL, 0x02
.equ C1_DCACHE_CONTROL,    0x04
.equ C1_ICACHE_CONTROL,    0x1000
.equ C1_VECTOR_CONTROL,    0x2000

/*
 * Loader Parameter Block Offsets
 */
.equ LpbKernelStack,       0x18
.equ LpbPanicStack,        0x74
.equ LpbInterruptStack,    0x5C

/*
 * Trap Frame offsets
 */
.equ TrR0,                 0x00
.equ TrapFrameLength,      0x144

/*
 * PCR
 */
.equ KiPcr,                0xFFFFF000

/*
 * PCR Offsets
 */
.equ PcCurrentIrql,        0x14C

/*
 * KTHREAD Offsets
 */
.equ ThKernelStack,        0x20

#else

/*
 * CPSR Values
 */
#define CPSR_THUMB_ENABLE    0x20   
#define CPSR_FIQ_DISABLE     0x40
#define CPSR_IRQ_DISABLE     0x80
#define CPSR_USER_MODE       0x10
#define CPSR_FIQ_MODE        0x11
#define CPSR_IRQ_MODE        0x12
#define CPSR_SVC_MODE        0x13
#define CPSR_ABORT_MODE      0x17
#define CPSR_UND_MODE        0x1B
#define CPSR_MODES           0x1F

#endif
