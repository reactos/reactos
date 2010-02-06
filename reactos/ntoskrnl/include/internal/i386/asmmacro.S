/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/i386/asmmacro.S
 * PURPOSE:         Assembly Macros for Spinlocks and common Trap Code
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */
 
/* INCLUDES ******************************************************************/

#include <ndk/asm.h>

// Arguments for idt
#define INT_32_DPL0                 0x8E00
#define INT_32_DPL3                 0xEE00

.intel_syntax noprefix

//
// These macros are inlined equivalents of KiAcquire/ReleaseSpinlock, that is,
// they will not be compiled into non-SMP builds. Usage is as follows:
//
// .BeginYourFunction
//      mov reg, lockaddr
//      ACQUIRE_SPINLOCK(reg, .spin)
//      <thread-safe code here>
//      RELEASE_SPINLOCK(reg)
//      <misc code here>
//  retn
//  #IFDEF CONFIG_SMP
//  .spin
//      <any necessary steps to be able to jump back safely>
/       SPIN_ON_LOCK(reg, .BeginYourFunction)
//  #ENDIF
//
#ifdef CONFIG_SMP
#define LOCK lock
#define ACQUIRE_SPINLOCK(x, y) \
    lock bts dword ptr [x], 0; \
    jb y
#define RELEASE_SPINLOCK(x) mov byte ptr [x], 0
#define SPIN_ON_LOCK(x, y) \
1: \
    test dword ptr [x], 1; \
    jz y; \
    pause; \
    jmp 1b
#else
#define LOCK
#define ACQUIRE_SPINLOCK(x, y)
#define RELEASE_SPINLOCK(x) 
#endif

//
// @name IDT
//
// This macro creates an IDT entry for the given handler
//
// @param Handler
//        Pointer to the IDT handler
//
// @param Bits
//        Descriptor Bits to associate
//
// @remark None.
//
.macro idt Handler, Bits
    .long \Handler
    .short \Bits
    .short KGDT_R0_CODE
.endm

//
// @name GENERATE_IDT_STUB
//
// This macro creates an IDT entry for an unexpected interrupt handler.
//
// @param None.
//
// @remark None.
//
.macro GENERATE_IDT_STUB Number
idt _KiUnexpectedInterrupt&Number, INT_32_DPL0
.endm

//
// @name GENERATE_IDT_STUBS
//
// This macro creates unexpected interrupt IDT entries.
//
// @param None.
//
// @remark None.
//
.altmacro
.macro GENERATE_IDT_STUBS
.set i, 0
.rept 208
    GENERATE_IDT_STUB %i
    .set i, i + 1
.endr
.endm

//
// @name GENERATE_INT_HANDLER
//
// This macro creates an unexpected interrupt handler.
//
// @param None.
//
// @remark None.
//
.macro GENERATE_INT_HANDLER Number
.func KiUnexpectedInterrupt&Number
_KiUnexpectedInterrupt&Number:
    mov eax, PRIMARY_VECTOR_BASE + Number
    jmp _KiEndUnexpectedRange@0
.endfunc
.endm

//
// @name GENERATE_INT_HANDLERS
//
// This macro creates the unexpected interrupt handlers.
//
// @param None.
//
// @remark None.
//
.altmacro
.macro GENERATE_INT_HANDLERS
.set i, 0
.rept 208
    GENERATE_INT_HANDLER %i
    .set i, i + 1
.endr
.endm
