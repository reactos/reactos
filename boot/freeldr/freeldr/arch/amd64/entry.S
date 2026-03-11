
#include <asm.inc>
#include <ksamd64.inc>
#include <arch/pc/x86common.h>
#include <arch/pc/pcbios.h>

EXTERN BootMain:PROC
EXTERN cmdline:DWORD

EXTERN DiskStopFloppyMotor:PROC

#ifndef __clang__
EXTERN __bss_start__:FWORD
EXTERN __bss_end__:FWORD
#endif

.code64

PUBLIC RealEntryPoint
RealEntryPoint:
    /* Setup segment selectors */
    mov ax, LMODE_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    /* Setup long mode stack */
    mov rsp, qword ptr [rip + stack64]

    /* Continue execution */
    jmp qword ptr [rip + ContinueAddress]

ContinueAddress:
    .quad FrldrStartup

FrldrStartup:

    /* Set up CR0 for SSE */
    mov rax, cr0
    and eax, not CR0_EM
    or rax, CR0_MP
    mov cr0, rax

    /* Set up CR4 for SSE */
    mov rax, cr4
    or eax, CR4_FXSR
    or eax, CR4_XMMEXCPT
    mov cr4, rax

    /* Store BootDrive and BootPartition */
    mov al, byte ptr [BSS_BootDrive]
    mov byte ptr [rip + FrldrBootDrive], al
    xor eax, eax
    mov al, byte ptr [BSS_BootPartition]
    mov dword ptr [rip + FrldrBootPartition], eax

    /* Patch long jump with real mode entry point */
    mov eax, dword ptr [BSS_RealModeEntry]
    mov dword ptr [rip + AddressOfRealModeEntryPoint], eax

#ifndef __clang__
    /* Clean out BSS (GNU ld provides these symbols via linker script;
     * for Clang/LLD builds, BSS is zeroed from C in BootMain) */
    xor rax, rax
    lea rdi, [rip + __bss_start__]
    lea rcx, [rip + __bss_end__ + 7]
    sub rcx, rdi
    shr rcx, 3
    rep stosq
#endif

    /* Pass the command line to BootMain */
    lea rcx, [rip + cmdline]

    /* GO! */
    call BootMain

    /* We should never get here */
stop:
    jmp short stop
    nop
    nop


PUBLIC Reboot
Reboot:
    /* Stop the floppy drive motor */
    call DiskStopFloppyMotor

    /* Set the function ID and switch to real mode (we don't return) */
    mov bx, FNID_Reboot
    jmp SwitchToReal


/*
 * VOID __cdecl Relocator16Boot(
 *     IN REGS*  In<rcx>,
 *     IN USHORT StackSegment<dx>,
 *     IN USHORT StackPointer<r8w>,
 *     IN USHORT CodeSegment<r9w>,
 *     IN USHORT CodePointer<rsp+40>);
 *
 * RETURNS: Nothing.
 *
 * NOTE: The implementation of this function is similar to that of Int386(),
 * with the proviso that no attempt is done to save the original values of
 * the registers since we will not need them anyway, as we do not return back
 * to the caller but instead place the machine in a permanent new CPU state.
 */
PUBLIC Relocator16Boot
Relocator16Boot:

    /* Save home registers */
    mov qword ptr [rsp +  8], rcx
    mov  word ptr [rsp + 16], dx
    mov  word ptr [rsp + 24], r8w
    mov  word ptr [rsp + 32], r9w

#if 0
    /* Save non-volatile registers */
    push rbx
    push rsi
    push rdi
#endif

    /* Copy input registers */
    mov rsi, qword ptr [rsp + 8]
    mov rdi, BSS_RegisterSet
    mov rcx, REGS_SIZE / 4
    rep movsd

    /* Set the stack segment/offset */
    mov ax, word ptr [rsp + 16]
    shl eax, 16
    mov ax, word ptr [rsp + 24]
    mov dword ptr [BSS_CallbackReturn], eax

    /*
     * Set the code segment/offset (Copy entry point)
     * NOTE: We permanently *ERASE* the contents of ds:[BSS_RealModeEntry]
     * but it is not a problem since we are going to place the machine in
     * a permanent new CPU state.
     */
    mov ax, word ptr [rsp + 32]
    shl eax, 16
    mov ax, word ptr [rsp + 40]
    mov dword ptr [BSS_RealModeEntry], eax

    /* Set the function ID and switch to real mode (we don't return) */
    mov bx, FNID_Relocator16Boot
    jmp SwitchToReal


/*
 * U16 PxeCallApi(U16 Segment, U16 Offset, U16 Service, VOID *Parameter);
 *
 * RETURNS:
 */
PUBLIC PxeCallApi
PxeCallApi:
    xor eax, eax
    ret


/* Internal function for realmode calls
 * bx must be set to the ID of the realmode function to call. */
PUBLIC CallRealMode
CallRealMode:
    /* Save current stack pointer */
    mov qword ptr [rip + stack64], rsp

    /* Set continue address and switch to real mode */
    lea rax, [rip + CallRealMode_return]
    mov qword ptr [rip + ContinueAddress], rax

SwitchToReal:
    /* Set sane segments */
    mov ax, LMODE_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    /* Save 64-bit stack pointer */
    mov qword ptr [rip + stack64], rsp

    /* Step 1 - jump to compatibility segment */
    jmp fword ptr [rip + jumpvector]

jumpvector:
    .long SwitchToRealCompSegment
    .word CMODE_CS

SwitchToRealCompSegment:
    /* Note: In fact the CPU is in 32 bit mode here. But it will interpret
       the generated instructions accordingly. rax will become eax */

    /* Step 2 - deactivate long mode, by disabling paging */
    mov rax, cr0
    and eax, HEX(7fffffff)
    mov cr0, rax

    /* Step 3 - jump to 16-bit segment to set the limit correctly */
    .byte HEX(0EA)
AddressOfRealModeEntryPoint:
    .long 0
    .word L_RMODE_CS
    nop

CallRealMode_return:
    /* restore stack pointer */
    mov rsp, qword ptr [rip + stack64]
    ret

/////////////////////////////////////////

    /* 64-bit stack pointer */
stack64:
    .quad STACKADDR

PUBLIC FrldrBootDrive
FrldrBootDrive:
    .byte 0

PUBLIC FrldrBootPartition
FrldrBootPartition:
    .long 0

END
