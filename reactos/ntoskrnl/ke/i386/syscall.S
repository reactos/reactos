/* $Id$
 *
 * FILE:            ntoskrnl/ke/i386/syscall.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Call Handler
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  ??-??-??: Original Version - David Welch(?)
 *                  13-01-05: Complete rewrite, added support for SYSENTER, direct kmode syscalls
 *                            and re-wrote most of handler code. - Alex Ionescu
 */

#include <roscfg.h>
#include <internal/i386/ke.h>
#include <internal/asm.h>
#include <ndk/i386/segment.h>

#define UserMode  (1)

.globl KeReturnFromSystemCallWithHook
.globl _KiServiceExit
.globl _KiServiceExit2
.globl _KiFastCallEntry
.globl _KiSystemService
.globl _KiDebugService

_KiFastCallEntry:

    /* Set FS to PCR */
    movl $PCR_SELECTOR, %ecx
    movw %cx, %fs

    /* Set the current stack to Kernel Stack */
    movl %fs:KPCR_TSS, %ecx
    movl %ss:KTSS_ESP0(%ecx), %ecx
    movl %ecx, %esp

    /* Set up a fake INT Stack. */
    pushl $USER_DS
    pushl %edx                  /* Ring 3 SS:ESP */
    pushfl
    orl $X86_EFLAGS_IF, (%esp)  /* Re-enable IRQs in EFLAGS, to fake INT */
    pushl $USER_CS
    pushl $KUSER_SHARED_SYSCALL_RET

    /* User Parameter List */
    add $8, %edx

_KiSystemService:

    /*
     * Construct a trap frame on the stack.
     * The following are already on the stack.
     */
    // SS                                                                  + 0x0
    // ESP                                                                 + 0x4
    // EFLAGS                                                              + 0x8
    // CS                                                                  + 0xC
    // EIP                                                                 + 0x10
    pushl $0                                                            // + 0x14
    pushl %ebp                                                          // + 0x18
    pushl %ebx                                                          // + 0x1C
    pushl %esi                                                          // + 0x20
    pushl %edi                                                          // + 0x24
    pushl %fs                                                           // + 0x28

    /* Load PCR Selector into fs */
    movw $PCR_SELECTOR, %bx
    movw %bx, %fs

    /* Save the previous exception list */
    pushl %fs:KPCR_EXCEPTION_LIST                                      // + 0x2C

    /* Set the exception handler chain terminator */
    movl $0xffffffff, %fs:KPCR_EXCEPTION_LIST

    /* Get a pointer to the current thread */
    movl %fs:KPCR_CURRENT_THREAD, %esi

    /* Save the old previous mode */
    pushl %ss:KTHREAD_PREVIOUS_MODE(%esi)                              // + 0x30

    /* Set the new previous mode based on the saved CS selector */
    movl 0x24(%esp), %ebx
    andl $1, %ebx
    movb %bl, %ss:KTHREAD_PREVIOUS_MODE(%esi)

    /* Save other registers */
    pushl %eax                                                         // + 0x34
    pushl %ecx                                                         // + 0x38
    pushl %edx                                                         // + 0x3C
    pushl %ds                                                          // + 0x40
    pushl %es                                                          // + 0x44
    pushl %gs                                                          // + 0x48
    sub $0x28, %esp                                                    // + 0x70

#ifdef DBG
    /* Trick gdb 6 into backtracing over the system call */
    mov 0x6c(%esp), %ebx
    pushl 4(%ebx)    /* DebugEIP */                                    // + 0x74
#else
    pushl 0x60(%esp) /* DebugEIP */                                    // + 0x74
#endif
    pushl %ebp       /* DebugEBP */                                    // + 0x78

    /* Load the segment registers */
    sti
    movw $KERNEL_DS, %bx
    movw %bx, %ds
    movw %bx, %es

    /* Save the old trap frame pointer where EDX would be saved */
    movl KTHREAD_TRAP_FRAME(%esi), %ebx
    movl %ebx, KTRAP_FRAME_EDX(%esp)

    /* Allocate new Kernel stack frame */
    movl %esp,%ebp

    /* Save a pointer to the trap frame in the TCB */
    movl %ebp, KTHREAD_TRAP_FRAME(%esi)

CheckValidCall:

#ifdef DBG
    /*
     * GDB thinks the function starts here and
     * wants a standard prolog, so let's give it
     */
    pushl %ebp
    movl %esp,%ebp
    popl %ebp
#endif

    /*
     * Find out which table offset to use. Converts 0x1124 into 0x10.
     * The offset is related to the Table Index as such: Offset = TableIndex x 10
     */
    movl %eax, %edi
    shrl $8, %edi
    andl $0x10, %edi
    movl %edi, %ecx

    /* Now add the thread's base system table to the offset */
    addl KTHREAD_SERVICE_TABLE(%esi), %edi

    /* Get the true syscall ID and check it */
    movl %eax, %ebx
    andl $0x0FFF, %eax
    cmpl 8(%edi), %eax

    /* Invalid ID, try to load Win32K Table */
    jnb KiBBTUnexpectedRange

    /* Users's current stack frame pointer is source */
    movl %edx, %esi

    /* Allocate room for argument list from kernel stack */
    movl 12(%edi), %ecx
    movb (%ecx, %eax), %cl
    movzx %cl, %ecx

    /* Allocate space on our stack */
    subl %ecx, %esp

    /* Get pointer to function */
    movl (%edi), %edi
    movl (%edi, %eax, 4), %eax

    /* Copy the arguments from the user stack to our stack */
    shr $2, %ecx
    movl %esp, %edi
    cld
    rep movsd

    /* Do the System Call */
    call *%eax
    movl %eax, KTRAP_FRAME_EAX(%ebp)

    /* Deallocate the kernel stack frame  */
    movl %ebp, %esp

KeReturnFromSystemCall:

    /* Get the Current Thread */
    movl %fs:KPCR_CURRENT_THREAD, %esi

    /* Restore the old trap frame pointer */
    movl KTRAP_FRAME_EDX(%esp), %ebx
    movl %ebx, KTHREAD_TRAP_FRAME(%esi)

_KiServiceExit:

    /* Get the Current Thread */
    cli
    movl %fs:KPCR_CURRENT_THREAD, %esi

    /* Deliver APCs only if we were called from user mode */
    testb $1, KTRAP_FRAME_CS(%esp)
    je KiRosTrapReturn

    /* And only if any are actually pending */
    cmpb $0, KTHREAD_PENDING_USER_APC(%esi)
    je KiRosTrapReturn

    /* Save pointer to Trap Frame */
    movl %esp, %ebx

    /* Raise IRQL to APC_LEVEL */
    movl $1, %ecx
    call @KfRaiseIrql@4

    /* Save old IRQL */
    pushl %eax

    /* Deliver APCs */
    sti
    pushl %ebx
    pushl $0
    pushl $UserMode
    call _KiDeliverApc@12
    cli

    /* Return to old IRQL */
    popl %ecx
    call @KfLowerIrql@4

KiRosTrapReturn:

    /* Skip debug information and unsaved registers */
    addl $0x30, %esp                                                   // + 0x48
    popl %gs                                                           // + 0x44
    popl %es                                                           // + 0x40
    popl %ds                                                           // + 0x3C
    popl %edx                                                          // + 0x38
    popl %ecx                                                          // + 0x34
    popl %eax                                                          // + 0x30

    /* Restore the old previous mode */
    popl %ebx                                                          // + 0x2C
    movb %bl, %ss:KTHREAD_PREVIOUS_MODE(%esi)

    /* Restore the old exception handler list */
    popl %fs:KPCR_EXCEPTION_LIST                                       // + 0x28

    /* Restore final registers from trap frame */
    popl %fs                                                           // + 0x24
    popl %edi                                                          // + 0x20
    popl %esi                                                          // + 0x1C
    popl %ebx                                                          // + 0x18
    popl %ebp                                                          // + 0x14
    add $4, %esp                                                       // + 0x10

    /* Check if previous CS is from user-mode */
    testl $1, 4(%esp)

    /* It is, so use Fast Exit */
    jnz FastRet

    /*
     * Restore what the stub pushed, and return back to it.
     * Note that we were CALLed, so the first thing on our stack is the ret EIP!
     */
    pop %edx                                                           // + 0x0C
    pop %ecx                                                           // + 0x08
    popf                                                               // + 0x04
    jmp *%edx

IntRet:

    iret

FastRet:

    /* Is SYSEXIT Supported/Wanted? */
    cmpl $0, %ss:_KiFastSystemCallDisable
    jnz IntRet

    /* Restore FS to TIB */
    mov $TEB_SELECTOR, %ecx
    mov %ecx, %fs

    /* We will be cleaning up the stack ourselves */
    popl %edx                         /* New Ring 3 EIP */
    add $0x4, %esp                    /* Skip Ring 3 DS */
    andl $~X86_EFLAGS_IF, (%esp)      /* Remove IRQ hack from EFLAGS */
    popfl                             /* Restore old EFLAGS */
    popl %ecx                         /* Old Ring 3 SS:ESP */

    /*
     * At this point:
     *     ECX points to the old User Stack.
     *     EDX points to the instruction to execute in usermode after the sysenter
     */
    sti
    sysexit

KiBBTUnexpectedRange:

    /* If this isn't a Win32K call, fail */
    cmp $0x10, %ecx
    jne InvalidCall

    /* Set up Win32K Table */
    pushl %edx
    pushl %ebx
    call _KiServiceCheck
    popl %eax
    popl %edx

    /* Try the Call again */
    jmp CheckValidCall

InvalidCall:

    /* Invalid System Call */
    movl $0xC000001C, %eax /* STATUS_INVALID_SYSTEM_SERVICE */
    movl %eax, KTRAP_FRAME_EAX(%ebp)
    jmp _KiServiceExit

_KiServiceExit2:

    /* Get the Current Thread */
    cli
    movl %fs:KPCR_CURRENT_THREAD, %esi

    /* Deliver APCs only if we were called from user mode */
    testb $1, KTRAP_FRAME_CS(%esp)
    je KiRosTrapReturn

    /* And only if any are actually pending */
    cmpb $0, KTHREAD_PENDING_USER_APC(%esi)
    je KiRosTrapReturn

    /* Save pointer to Trap Frame */
    movl %esp, %ebx

    /* Raise IRQL to APC_LEVEL */
    movl $1, %ecx
    call @KfRaiseIrql@4

    /* Save old IRQL */
    pushl %eax

    /* Deliver APCs */
    sti
    pushl %ebx
    pushl $0
    pushl $UserMode
    call _KiDeliverApc@12
    cli

    /* Return to old IRQL */
    popl %ecx
    call @KfLowerIrql@4

    /* Skip useless Debug Data */
    addl $0x18, %esp                                                   // + 0x74

    /* Restore Debug Registers */
    popl %eax                                                          // + 0x5C
    movl %eax, %dr0
    popl %eax                                                          // + 0x58
    movl %eax, %dr1
    popl %eax                                                          // + 0x54
    movl %eax, %dr2
    popl %eax                                                          // + 0x50
    movl %eax, %dr3
    popl %eax                                                          // + 0x4C
    movl %eax, %dr6
    popl %eax                                                          // + 0x48
    movl %eax, %dr7

    /* Restore Registers */
    popl %gs                                                           // + 0x44
    popl %es                                                           // + 0x40
    popl %ds                                                           // + 0x3C
    popl %edx                                                          // + 0x38
    popl %ecx                                                          // + 0x34
    popl %eax                                                          // + 0x30

    /* Restore the old previous mode */
    popl %ebx                                                          // + 0x2C
    movb %bl, %ss:KTHREAD_PREVIOUS_MODE(%esi)

    /* Restore the old exception handler list */
    popl %fs:KPCR_EXCEPTION_LIST                                       // + 0x28

    /* Restore final registers from trap frame */
    popl %fs                                                           // + 0x24
    popl %edi                                                          // + 0x20
    popl %esi                                                          // + 0x1C
    popl %ebx                                                          // + 0x18
    popl %ebp                                                          // + 0x14
    add $4, %esp                                                       // + 0x10

    /* Return to user-mode */
    iret

.intel_syntax noprefix
_KiDebugService:

    /* Create the Trap Frame */
    push 0
    push ebp
    push ebx
    push esi
    push edi
    push fs

    /* Switch to correct FS */
    mov bx, PCR_SELECTOR
    mov fs, bx

    /* Save Exception List */
    push fs:[KPCR_EXCEPTION_LIST]

    /* Use Old Previous Mode */
    mov ebx, fs:[KPCR_CURRENT_THREAD]
    push [ebx+KTHREAD_PREVIOUS_MODE]

    /* Continue building the Trap Frame */
    push eax
    push ecx
    push edx
    push ds
    push es
    push gs

    /* Switch Segments to Kernel */
    mov bx, KERNEL_DS
    mov ds, bx
    mov es, bx

    /* Save Debug Registers */
    mov ebx, eax
    mov eax, dr7
    push eax
    mov eax, dr6
    push eax
    mov eax, dr3
    push eax
    mov eax, dr2
    push eax
    mov eax, dr1
    push eax
    mov eax, dr0
    push eax
    mov eax, ebx

    /* Skip useless debug data */
    sub esp, 0x18

    /* Call debug service dispatcher */
    push edx
    push ecx
    push eax
    call _KdpServiceDispatcher@12

    /* Exit through common routine */
    jmp _KiServiceExit2
