
#include <asm.inc>
#include <ksamd64.inc>
#include <arch/pc/x86common.h>
#include <arch/pc/pcbios.h>

EXTERN BootMain:PROC
EXTERN cmdline:DWORD

EXTERN DiskStopFloppyMotor:PROC

#ifdef _USE_ML
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
//    mov ss, ax

    //mov word ptr [HEX(b8000)], HEX(0e00) + '1'

    /* Setup long mode stack */
    mov rsp, qword ptr [stack64]

    /* Continue execution */
    jmp qword ptr [ContinueAddress]

ContinueAddress:
    .quad FrldrStartup

FrldrStartup:

    /* Set up CR0 for SSE */
    mov rax, cr0
    and eax, not CR0_EM // Clear coprocessor emulation CR0.EM CR0.EM (bit 2)
    or rax, CR0_MP // Set coprocessor monitoring CR0.MP (bit 1)
    mov cr0, rax

    /* Set up CR4 for SSE */
    mov rax, cr4
    or eax, CR4_FXSR // Enable fx save/restore CR4.OSFXSR (bit 9)
    or eax, CR4_XMMEXCPT // Enable XMMI exceptions CR4.OSXMMEXCPT (bit 10)
    mov cr4, rax

    /* Store BootDrive and BootPartition */
    mov al, byte ptr [BSS_BootDrive]
    mov byte ptr [FrldrBootDrive], al
    xor eax, eax
    mov al, byte ptr [BSS_BootPartition]
    mov dword ptr [FrldrBootPartition], eax

    /* Patch long jump with real mode entry point */
    mov eax, dword ptr [BSS_RealModeEntry]
    mov dword ptr [AddressOfRealModeEntryPoint], eax

    /* Clean out BSS */
    xor rax, rax
    mov rdi, offset __bss_start__
    mov rcx, offset __bss_end__ + 7
    sub rcx, rdi
    shr rcx, 3
    rep stosq

    /* Pass the command line to BootMain */
    mov rcx, offset cmdline

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
    // Since BSS_CallbackReturn contains a ULONG, store in its high word
    // the stack segment and in its low word the stack offset.
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
    // Since BSS_RealModeEntry contains a ULONG, store in its high word
    // the code segment and in its low word the code offset.
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
    mov qword ptr [stack64], rsp

    /* Set continue address and switch to real mode */
    lea rax, [CallRealMode_return]
    mov qword ptr [ContinueAddress], rax

SwitchToReal:
    /* Set sane segments */
    mov ax, LMODE_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    //mov ss, ax

    //mov word ptr [HEX(0b8008)], HEX(0e00) + '4'

    /* Save 64-bit stack pointer */
    mov qword ptr [stack64], rsp

    /* Step 1 - jump to compatibility segment */
    jmp fword ptr [jumpvector]

jumpvector:
    .long SwitchToRealCompSegment
    .word CMODE_CS

SwitchToRealCompSegment:
    /* Note: In fact the CPU is in 32 bit mode here. But it will interpret
       the generated instructions accordingly. rax will become eax */

    /* Step 2 - deactivate long mode, by disabling paging */
    mov rax, cr0
    and eax, HEX(7fffffff) //~0x80000000, upper bits cleared
    mov cr0, rax

//    mov word ptr [HEX(0b800a)], HEX(0e00) + '5'

    /* Step 3 - jump to 16-bit segment to set the limit correctly */
    .byte HEX(0EA) // 32bit long jmp
AddressOfRealModeEntryPoint:
    .long 0 // receives address of RealModeEntryPoint
    .word HEX(20)//RMODE_CS
    nop

CallRealMode_return:
    /* restore stack pointer */
    mov rsp, qword ptr [stack64]
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
