/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/ctxswitch.S
 * PURPOSE:         Thread Context Switching
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <roscfg.h>
#include <internal/i386/segment.h>
#include <internal/i386/ke.h>
#include <internal/i386/fpu.h>
#include <internal/ps.h>
#include <ntos/tss.h>
#include <internal/ntoskrnl.h>
.intel_syntax noprefix

#define Running 2 
#define SIZEOF_TRAP_FRAME 0x8c
#define APC_LEVEL 1

/* GLOBALS ****************************************************************/  

/* FUNCTIONS ****************************************************************/

/*++
 * KiThreadStartup
 *
 *     The KiThreadStartup routine is the beginning of any thread.
 *
 * Params:
 *     SystemRoutine - Pointer to the System Startup Routine. Either 
 *                     PspUserThreadStartup or PspSystemThreadStartup
 *
 *     StartRoutine - For Kernel Threads only, specifies the starting execution
 *                    point of the new thread.
 *
 *     StartContext - For Kernel Threads only, specifies a pointer to variable
 *                    context data to be sent to the StartRoutine above.
 *
 *     UserThread - Indicates whether or not this is a user thread. This tells
 *                  us if the thread has a context or not.
 *
 *     TrapFrame - Pointer to the KTHREAD to which the caller wishes to
 *           switch from.
 *
 * Returns:
 *     Should never return for a system thread. Returns through the System Call
 *     Exit Dispatcher for a user thread.
 *
 * Remarks:
 *     If a return from a system thread is detected, a bug check will occur.
 *
 *--*/
.globl _KiThreadStartup@156
_KiThreadStartup@156:

    /* 
     * Clear all the non-volatile registers, so the thread won't be tempted to
     * expect any static data (like some badly coded usermode/win9x apps do)
     */
    xor ebx, ebx
    xor esi, edi
    xor edi, edi
    xor ebp, ebp
    
    /* It's now safe to go to APC */
    mov ecx, APC_LEVEL
    call @KfLowerIrql@4
    
    /* 
     * Call the System Routine which is right on our stack now.
     * After we pop the pointer, the Start Routine/Context will be on the 
     * stack, as parameters to the System Routine
     */
    pop eax
    call eax
    
    /* The thread returned... was it a user-thread? */
    pop ecx
    or ecx, ecx
    jz BadThread
    
    /* Yes it was, set our trapframe for the System Call Exit Dispatcher */
    mov ebp, esp
    
    /* Exit back to user-mode */
    jmp _KiServiceExit2
    
BadThread:

    /* A system thread returned...this is very bad! */
    int 3

/*++
 * KiSwapContextInternal 
 *
 *     The KiSwapContextInternal routine switches context to another thread.
 *
 * Params:
 *     ESI - Pointer to the KTHREAD to which the caller wishes to
 *           switch to.
 *     EDI - Pointer to the KTHREAD to which the caller wishes to
 *           switch from.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     Absolutely all registers except ESP can be trampled here for maximum code flexibility.
 *
 *--*/
.globl @KiSwapContextInternal@0
@KiSwapContextInternal@0:
#ifdef KDBG
    //jmp SaveTrapFrameForKDB
SaveTrapFrameForKDB_Return:
#endif

    /* Get the PCR. It's faster to use ebx+offset then fs:offset */
    mov ebx, [fs:KPCR_SELF]
    
    /* Set the Thread to running */
    mov byte ptr [esi+KTHREAD_STATE], Running

    /* Save the Exception list */
    push [ebx+KPCR_EXCEPTION_LIST]
    
    /* Switching, disable interrupts now */
    cli

#ifdef CONFIG_SMP
    /* Save FPU state if the thread has used it. */
    mov dword ptr [ebx+KPCR_NPX_THREAD], 0
    test byte ptr [edi+KTHREAD_NPX_STATE], NPX_STATE_DIRTY
    jz 3f
    mov eax, [edi+KTHREAD_INITIAL_STACK]
    cmp dword ptr _FxsrSupport, 0
    je 1f
    fxsave [eax-SIZEOF_FX_SAVE_AREA]
    jmp 2f
1:
    fnsave [eax-SIZEOF_FX_SAVE_AREA]
2:
    mov byte ptr [edi+KTHREAD_NPX_STATE], NPX_STATE_VALID
3:
#endif /* CONFIG_SMP */
    
    /* Save the stack pointer in this processors TSS */
    mov ebp, [ebx+KPCR_TSS]
    push ss:[ebp+KTSS_ESP0]

    /* Switch stacks */
    mov [edi+KTHREAD_KERNEL_STACK], esp
    mov esp, [esi+KTHREAD_KERNEL_STACK]
   
    /* 
     * Sadly, ROS memory management is screwed up, so 
     * we must change the address space here.
     */
    mov eax, [esi+KTHREAD_APCSTATE_PROCESS]
    mov eax, [eax+KPROCESS_DIRECTORY_TABLE_BASE]
    mov cr3, eax
    
    /* Stack is OK, safe to enable interrupts now */
    sti 

    /* Check if address space switch is needed */
    mov eax, [edi+KTHREAD_APCSTATE_PROCESS]
    cmp eax, [esi+KTHREAD_APCSTATE_PROCESS]
    
    /* If they match, then use the fast-path and skip all this */
    jz SameProcess
    
    /* Get the new Process. */
    mov edi, [esi+KTHREAD_APCSTATE_PROCESS]
    
    /* Check if we need an LDT */
    xor eax, eax
    cmp [edi+KPROCESS_LDT_DESCRIPTOR0], eax
    jz NoLdt
    
    /* Write the LDT Selector */
    mov ebp, [ebx+KPCR_GDT]
    mov eax, [edi+KPROCESS_LDT_DESCRIPTOR0]
    mov [ebp+LDT_SELECTOR], eax
    mov eax, [edi+KPROCESS_LDT_DESCRIPTOR1]
    mov [ebp+LDT_SELECTOR+4], eax
    
    /* Save LDT Selector */
    mov eax, LDT_SELECTOR
        
NoLdt:
    
    /* Load LDT */
    lldt ax
  
    /* Get the IOPM */
    mov ecx, [edi+KPROCESS_IOPM_OFFSET]

    /* Set current IOPM offset in the TSS */
    mov [ebp+KTSS_IOMAPBASE], cx
    
SameProcess:
    
    /* Set the TEB */
    mov eax, [esi+KTHREAD_TEB]
    mov ecx, [ebx+KPCR_GDT]
    mov [ecx+0x3A], ax
    shr eax, 16
    mov [ecx+0x3C], al
    mov [ecx+0x3F], ah
    
    /* Increase context switches */
    inc dword ptr [esi+KTHREAD_CONTEXT_SWITCHES]
    
    /* Set TS in cr0 to catch FPU code and load the FPU state when needed */
#ifndef CONFIG_SMP
    cmp [ebx+KPCR_NPX_THREAD], esi
    je 4f
#endif /* !CONFIG_SMP */
    mov eax, cr0
    or eax, X86_CR0_TS
    mov cr0, eax
4:
    
    /* Restore the stack pointer in this processors TSS */
    pop ss:[ebp+KTSS_ESP0]
    
    /* Restore exception list */
    pop [ebx+KPCR_EXCEPTION_LIST]

    /* Return */
    call @KeReleaseDispatcherDatabaseLockFromDpcLevel@0
    ret 

/*++
 * KiSwapContext 
 *
 *     The KiSwapContext routine switches context to another thread.
 *
 * Params:
 *     TargetThread - Pointer to the KTHREAD to which the caller wishes to
 *                    switch to.
 *
 * Returns:
 *     The WaitStatus of the Target Thread. NOT YET SUPPORTED.
 *
 * Remarks:
 *     This is a wrapper around KiSwapContextInternal which will save all the
 *     non-volatile registers so that the Internal function can use all of
 *     them. It will also save the old current thread and set the new one.
 *
 *     The calling thread does not return after KiSwapContextInternal until 
 *     another thread switches to IT.
 *
 *--*/
.globl @KiSwapContext@4
@KiSwapContext@4:
    /* Note, we CANNOT touch ebp */

    /* Save 4 registers */
    sub esp, 4 * 4
    
    /* Save all the non-volatile ones */
    mov [esp+12], ebx
    mov [esp+8], esi
    mov [esp+4], edi
    mov [esp+0], ebp
    
    /* Get the Current Thread */
    mov edi, fs:[KPCR_CURRENT_THREAD]
    
    /* Get the New Thread */
    mov esi, ecx
    
    /* Save it as Current thread */
    mov fs:[KPCR_CURRENT_THREAD], esi
        
    /* Do the swap with the registers correctly setup */
    call @KiSwapContextInternal@0
    
    /* Return the registers */
    mov ebp, [esp+0]
    mov edi, [esp+4]
    mov esi, [esp+8]
    mov ebx, [esp+12]
    
    /* Clean stack */
    add esp, 4 * 4
    ret
    
#ifdef KDBG

SaveTrapFrameForKDB:
    /* Set up a trap frame */
    
    /* Fake Interrupt Stack */
    push esp                                     // 0x74
    pushf                                        // 0x70
    push cs                                      // 0x6C
    push [esp+12]    /* EIP */                   // 0x68
    mov [esp+16], ss                             // 0x78
    
    /* Trap Frame */
    push 0           /* Error Code */            // 0x64
    push ebp                                     // 0x60
    push ebx
    push esi
    push edi
    push fs
    push -1          /* Exception List */        // 0x4C
    push 0           /* Previous Mode */         // 0x48
    push eax
    push ecx
    push edx
    push ds
    push es
    push gs                                      // 0x30
    sub esp, 0x28    /* Debug Registers */       // 0x8
    push [esp+60]    /* Debug EIP */             // 0x4
    push ebp         /* Debug EBP */             // 0x0
    
    /* Set Stack */
    mov ebp, esp
    
    /* Push old Trap Frame */
    push [edi+KTHREAD_TRAP_FRAME]
    
    /* Save new one */
    mov [edi+KTHREAD_TRAP_FRAME], ebp
    
    /* Return EIP */
    push offset RestoreTrapFrameForKDB
        
    /* Restore EBP */
    mov ebp, [ebp+KTRAP_FRAME_EBP]
    
    /* Jump to normal code */
    jmp SaveTrapFrameForKDB_Return

RestoreTrapFrameForKDB:
  
    /* Restore the old trapframe */
    pop [edi+KTHREAD_TRAP_FRAME]
        
    /* Pop unused portions of the trap frame */
    add esp, 0x30

    /* Restore registers from Trap frame */
    pop gs
    pop es
    pop ds
    pop edx
    pop ecx
    pop eax
    add esp, 8
    pop fs
    pop edi
    pop esi
    pop ebx

    /* Remove SS:ESP from the stack */
    mov ebp, [esp+16]
    mov [esp+24], ebp
    mov ebp, [esp+12]
    mov [esp+20], ebp
    mov ebp, [esp+8]
    mov [esp+16], ebp
    
    /* Restore Fake INT Stack */
    pop ebp
    add esp, 12

    /* Return to the caller. */
    iret
#endif /* KDBG */


