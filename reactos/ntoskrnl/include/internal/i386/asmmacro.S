/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnlinclude/i386/asmmacro.S
 * PURPOSE:         Assembly Macros for Spinlocks and common Trap Code (TODO)
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */
 
/* INCLUDES ******************************************************************/
#include <ndk/asm.h>
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
// SET_TF_DEBUG_HEADER
// This macro sets up the debug header in the trap frame.
// Assumptions:
// ebp = PKTRAP_FRAME
// edi/ebx = Have been saved and can be used
//
#define SET_TF_DEBUG_HEADER \
    /* Get the Debug Trap Frame EBP/EIP */ \
    mov ebx, [ebp+KTRAP_FRAME_EBP]; \
    mov edi, [ebp+KTRAP_FRAME_EIP]; \
\
    /* Write the debug data */ \
    mov [ebp+KTRAP_FRAME_DEBUGPOINTER], edx; \
    mov dword ptr [ebp+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00; \
    mov [ebp+KTRAP_FRAME_DEBUGEBP], ebx; \
    mov [ebp+KTRAP_FRAME_DEBUGEIP], edi;

//
// These macros control common execution paths for Traps and System Call Code
//
// TRAP_PROLOG
// This macro creates a standard trap entry prologue.
// It should be used for entry into any kernel trap (KiTrapXx), but not for
// system calls, which require special handling.
//
// Use as follows:
// _KiTrap00:
// /* Push fake error code */
// push 0
//
// /* Enter common prologue */
// TRAP_PROLOG(0)
//
// /* Handle trap */
// <Your Trap Code Here>
//
#define TRAP_PROLOG(Label) \
    /* Just to be safe, clear out the HIWORD, since it's reserved */ \
    mov word ptr [esp+2], 0; \
\
    /* Save the non-volatiles */ \
    push ebp; \
    push ebx; \
    push esi; \
    push edi; \
\
    /* Save FS and set it to PCR */ \
    push fs; \
    mov ebx, KGDT_R0_PCR; \
    mov fs, bx; \
\
    /* Save exception list and bogus previous mode */ \
    push fs:[KPCR_EXCEPTION_LIST]; \
    push -1; \
\
    /* Save volatiles and segment registers */ \
    push eax; \
    push ecx; \
    push edx; \
    push ds; \
    push es; \
    push gs; \
\
    /* Set the R3 data segment */ \
    mov ax, KGDT_R3_DATA + RPL_MASK; \
\
    /* Skip debug registers and debug stuff */ \
    sub esp, 0x30; \
\
    /* Load the segment registers */ \
    mov ds, ax; \
    mov es, ax; \
\
    /* Set up frame */ \
    mov ebp, esp; \
\
    /* Check if this was from V86 Mode */ \
    /* test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK; */ \
    /* jnz V86_Label; */ \
\
    /* Get current thread */ \
    mov ecx, [fs:KPCR_CURRENT_THREAD]; \
    cld; \
\
    /* Flush DR7 */ \
    and dword ptr [ebp+KTRAP_FRAME_DR7], 0; \
\
    /* Check if the thread was being debugged */ \
    /* test byte ptr [ecx+KTHREAD_DEBUG_ACTIVE], 0xFF; */ \
    /* jnz Dr_Label; */ \
\
    /* Set the Trap Frame Debug Header */ \
    SET_TF_DEBUG_HEADER



