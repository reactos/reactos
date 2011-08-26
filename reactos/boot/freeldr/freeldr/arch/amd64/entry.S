

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
    mov byte ptr [FrldrBootDrive], dl
    xor eax, eax
    mov al, dh
    mov dword ptr [FrldrBootPartition], eax

    /* Patch long jump with real mode entry point */
    mov eax, dword ptr [BSS_RealModeEntry]
    mov dword ptr [AddressOfRealModeEntryPoint], eax

    /* GO! */
    xor rcx, rcx
    call BootMain

	/* We should never get here */
stop:
	jmp	stop
	nop
	nop


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

    mov word ptr [HEX(0b8008)], HEX(0e00) + '4'

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
    .double STACK64ADDR

PUBLIC FrldrBootDrive
FrldrBootDrive:
    .byte 0

PUBLIC FrldrBootPartition
FrldrBootPartition:
    .long 0

PUBLIC PageDirectoryEnd
PageDirectoryEnd:
PUBLIC PageDirectoryStart
PageDirectoryStart:
PUBLIC PnpBiosGetDeviceNode
PnpBiosGetDeviceNode:
PUBLIC PnpBiosGetDeviceNodeCount
PnpBiosGetDeviceNodeCount:
PUBLIC PnpBiosSupported
PnpBiosSupported:

/* int Int386(int ivec<ecx>, REGS* in<rdx>, REGS* out<r8>); */
PUBLIC Int386
Int386:

    /* Save home registers */
    mov r11, rsp
    mov qword ptr [r11 + 8], rcx
    mov qword ptr [r11 + 16], rdx
    mov qword ptr [r11 + 24], r8

	/* Save non-volatile registers */
	push rbx
	push rbp
	push rsi
	push rdi

    /* Alloc stack space for home registers */
	sub rsp, 40
    //.ENDPROLOG

    mov word ptr [HEX(0b8006)], HEX(0e00) + '3'

	/* Copy the int vector to shared memory */
    mov dword ptr [BSS_IntVector], ecx

    /* Copy input registers */
    mov rsi, rdx
    mov rdi, BSS_RegisterSet
    mov rcx, REGS_SIZE / 4
    rep movsd

    /* Set the function ID and call real mode */
    mov bx, FNID_Int386
    call CallRealMode

    /* Copy output registers */
    mov rsi, BSS_RegisterSet
    mov rdi, [r11 + 16]
    mov rcx, REGS_SIZE / 4
    rep movsd

    /* cleanup and return */
    add rsp, 40
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret


PUBLIC PxeCallApi
PxeCallApi:
PUBLIC __lgdt
__lgdt:
PUBLIC __ltr
__ltr:

nop


END
