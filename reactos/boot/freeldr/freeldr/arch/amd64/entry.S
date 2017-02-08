
#include <asm.inc>
#include <arch/pc/x86common.h>

EXTERN BootMain:PROC

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
    .double offset FrldrStartup

FrldrStartup:

    /* Store BootDrive and BootPartition */
    mov al, byte ptr [BSS_BootDrive]
    mov byte ptr [FrldrBootDrive], al
    xor eax, eax
    mov al, byte ptr [BSS_BootPartition]
    mov dword ptr [FrldrBootPartition], eax

    /* Patch long jump with real mode entry point */
    mov eax, dword ptr [BSS_RealModeEntry]
    mov dword ptr [AddressOfRealModeEntryPoint], eax

    /* GO! */
    xor rcx, rcx
    call BootMain

    /* We should never get here */
stop:
    jmp stop
    nop
    nop


PUBLIC Reboot
Reboot:
    /* Set the function ID */
    mov bx, FNID_Reboot

    /* Switch to real mode (We don't return) */
    jmp SwitchToReal


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
    .long offset SwitchToRealCompSegment
    .word CMODE_CS

SwitchToRealCompSegment:
    /* Note: In fact the CPU is in 32 bit mode here. But it will interprete
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
    .double STACKADDR

PUBLIC FrldrBootDrive
FrldrBootDrive:
    .byte 0

PUBLIC FrldrBootPartition
FrldrBootPartition:
    .long 0

PUBLIC PxeCallApi
PxeCallApi:
    xor eax, eax
    ret

//void __lgdt(void *Source);
PUBLIC __lgdt
__lgdt:
    lgdt fword ptr [rcx]
    ret

//void __ltr(unsigned short Source);
PUBLIC __ltr
__ltr:
    ltr cx
    ret


END
