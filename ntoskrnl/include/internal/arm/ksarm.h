/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
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
.equ TrDbgArgMark,         0x00
.equ TrR0,                 0x04
.equ TrR1,                 0x08
.equ TrR2,                 0x0C
.equ TrR3,                 0x10
.equ TrR4,                 0x14
.equ TrR5,                 0x18
.equ TrR6,                 0x1C
.equ TrR7,                 0x20
.equ TrR8,                 0x24
.equ TrR9,                 0x28
.equ TrR10,                0x2C
.equ TrR11,                0x30
.equ TrR12,                0x34
.equ TrUserSp,             0x38
.equ TrUserLr,             0x3C
.equ TrSvcSp,              0x40
.equ TrSvcLr,              0x44
.equ TrPc,                 0x48
.equ TrSpsr,               0x4C
.equ TrapFrameLength,      (23 * 0x04)

/*
 * Exception Frame offsets
 */
.equ ExR4,                 0x00
.equ ExR5,                 0x04
.equ ExR6,                 0x08
.equ ExR7,                 0x0C
.equ ExR8,                 0x10
.equ ExR9,                 0x14
.equ ExR10,                0x18
.equ ExR11,                0x1C
.equ ExLr,                 0x20
.equ ExSpsr,               0x24
.equ ExceptionFrameLength, (10 * 0x04)

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

/*
 * CONTEXT Offsets
 */
.equ CONTEXT_FULL,         0x43
.equ CsContextFlags,       0x00
.equ CsR0,                 0x04
.equ CsR1,                 0x08
.equ CsR2,                 0x0C
.equ CsR3,                 0x10
.equ CsR4,                 0x14
.equ CsR5,                 0x18
.equ CsR6,                 0x1C
.equ CsR7,                 0x20
.equ CsR8,                 0x24
.equ CsR9,                 0x28
.equ CsR10,                0x2C
.equ CsR11,                0x30
.equ CsR12,                0x34
.equ CsSp,                 0x38
.equ CsLr,                 0x3C
.equ CsPc,                 0x40
.equ CsPsr,                0x44

/*
 * DebugService Control Types
 */
.equ BREAKPOINT_BREAK,          0
.equ BREAKPOINT_PRINT,          1
.equ BREAKPOINT_PROMPT,         2
.equ BREAKPOINT_LOAD_SYMBOLS,   3
.equ BREAKPOINT_UNLOAD_SYMBOLS, 4
.equ BREAKPOINT_COMMAND_STRING, 5

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
