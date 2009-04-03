/*
 * FILE:            hal/halx86/generic/bios.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         V8086 Real-Mode BIOS Thunking
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

//
// HAL BIOS Frame
//
#define HALP_BIOS_FRAME_SS          0x00
#define HALP_BIOS_FRAME_ESP         0x04
#define HALP_BIOS_FRAME_EFLAGS      0x08
#define HALP_BIOS_FRAME_CS          0x0C
#define HALP_BIOS_FRAME_EIP         0x10
#define HALP_BIOS_FRAME_TRAP_FRAME  0x14
#define HALP_BIOS_FRAME_CS_LIMIT    0x18
#define HALP_BIOS_FRAME_CS_BASE     0x1C
#define HALP_BIOS_FRAME_CS_FLAGS    0x20
#define HALP_BIOS_FRAME_SS_LIMIT    0x24
#define HALP_BIOS_FRAME_SS_BASE     0x28
#define HALP_BIOS_FRAME_SS_FLAGS    0x2C
#define HALP_BIOS_FRAME_PREFIX      0x30
#define HALP_BIOS_FRAME_LENGTH      0x34

/* GLOBALS *******************************************************************/

_HalpSavedEsp:
    .long 0

_UnhandledMsg:
    .asciz "\n\x7\x7!!! Unhandled or Unexpected Code at line: %lx!!!\n"

/* FUNCTIONS *****************************************************************/

.globl _HalpBiosCall@0
.func HalpBiosCall@0
_HalpBiosCall@0:

    /* Set up stack pointer */
    push ebp
    mov ebp, esp

    /* Build a trap frame */
    pushfd
    push edi
    push esi
    push ebx
    push ds
    push es
    push fs
    push gs
    push offset _HalpRealModeEnd

    /* Save the stack */
    mov _HalpSavedEsp, esp

    /* Turn off alignment faults */
    mov eax, cr0
    and eax, ~CR0_AM
    mov cr0, eax

    /* Setup a new stack */
    mov esi, fs:KPCR_TSS
    mov eax, esp
    sub eax, NPX_FRAME_LENGTH
    mov [esi+KTSS_ESP0], eax

    /* Save V86 registers */
    push 0
    push 0
    push 0
    push 0
    push 0x2000

    /* Get linear delta between stack and code */
    mov eax, offset _HalpRealModeEnd-4
    sub eax, offset _HalpRealModeStart

    /* Get offset of code */
    mov edx, offset _HalpRealModeStart
    and edx, 0xFFF

    /* Add offset to linear address and save the new V86 SP */
    add eax, edx
    push eax

    /* Start building interrupt frame. Setup V86 EFLAGS and IOPL 3 */
    pushfd
    or dword ptr [esp], EFLAGS_V86_MASK
    or dword ptr [esp], 0x3000

    /* Push the CS and IP */
    push 0x2000
    push edx

    /* Do the interrupt return (jump to V86 mode) */
    iretd

.globl _HalpRealModeStart
_HalpRealModeStart:

    /* Set mode 13 */
    mov ax, 0x12
    .byte 0
    .byte 0

    /* Do the interrupt */
    int 0x10

    /* BOP to exit V86 mode */
    .byte 0xC4
    .byte 0xC4

    /* The stack lives here */
.align 4
    .space 2048
.globl _HalpRealModeEnd
_HalpRealModeEnd:

    /* We're back, clean up the trap frame */
    pop gs
    pop fs
    pop es
    pop ds
    pop ebx
    pop esi
    pop edi
    popfd

    /* Return to caller */
    pop ebp
    ret 0
.endfunc

.globl _HalpOpcodeInvalid@0
.func HalpOpcodeInvalid@0
_HalpOpcodeInvalid@0:

    /* Unhandled */
    UNHANDLED_PATH

    /* Nothing to return */
    xor eax, eax
    ret 0
.endfunc

.globl _HalpPushInt@0
.func HalpPushInt@0
_HalpPushInt@0:

    /* Save EBX */
    push ebx

    /* Get SS offset and base */
    mov edx, [esi+HALP_BIOS_FRAME_ESP]
    mov ebx, [esi+HALP_BIOS_FRAME_SS_BASE]

    /* Convert to 16-bits */
    and edx, 0xFFFF
    sub dx, 2

    /* Get EFLAGS and write them into the linear address of SP */
    mov ax, word ptr [esi+HALP_BIOS_FRAME_EFLAGS]
    mov [ebx+edx], ax
    sub dx, 2

    /* Get CS segment and write it into SP */
    mov ax, word ptr [esi+HALP_BIOS_FRAME_CS]
    mov [ebx+edx], ax
    sub dx, 2

    /* Get IP and write it into SP */
    mov ax, word ptr [esi+HALP_BIOS_FRAME_EIP]
    mov [ebx+edx], ax

    /* Get new IP value (the interrupt ID is in ECX, so this is in the IVT) */
    mov eax, [ecx*4]
    push eax

    /* Now save the new IP */
    movzx eax, ax
    mov [esi+HALP_BIOS_FRAME_EIP], eax

    /* Save the new CS of this IP */
    pop eax
    shr eax, 16
    mov [esi+HALP_BIOS_FRAME_CS], eax

    /* Update the stack pointer after our manual interrupt frame construction */
    mov word ptr [esi+HALP_BIOS_FRAME_ESP], dx

    /* Get CS and convert it to linear format */
    mov eax, [esi+HALP_BIOS_FRAME_CS]
    shl eax, 4
    mov [esi+HALP_BIOS_FRAME_CS_BASE], eax
    mov dword ptr [esi+HALP_BIOS_FRAME_CS_LIMIT], 0xFFFF
    mov dword ptr [esi+HALP_BIOS_FRAME_CS_FLAGS], 0

    /* Return success and restore EBX */
    mov eax, 1
    pop ebx
    ret 0
.endfunc

.globl _HalpOpcodeINTnn@0
.func HalpOpcodeINTnn@0
_HalpOpcodeINTnn@0:

    /* Save non-volatiles and stack */
    push ebp
    push esi
    push ebx

    /* Get SS and convert it to linear format */
    mov eax, [esi+HALP_BIOS_FRAME_SS]
    shl eax, 4
    mov [esi+HALP_BIOS_FRAME_SS_BASE], eax
    mov dword ptr [esi+HALP_BIOS_FRAME_SS_LIMIT], 0xFFFF
    mov dword ptr [esi+HALP_BIOS_FRAME_SS_FLAGS], 0

    /* Increase IP and check if we're past the CS limit */
    inc dword ptr [esi+HALP_BIOS_FRAME_EIP]
    mov edi, [esi+HALP_BIOS_FRAME_EIP]
    cmp edi, [esi+HALP_BIOS_FRAME_CS_LIMIT]
    ja EipLimitReached

    /* Convert IP to linear address and read the interrupt number */
    add edi, [esi+HALP_BIOS_FRAME_CS_BASE]
    movzx ecx, byte ptr [edi]

    /* Increase EIP and do the interrupt, check for status */
    inc dword ptr [esi+HALP_BIOS_FRAME_EIP]
    call _HalpPushInt@0
    test eax, 0xFFFF
    jz Done

    /* Update the trap frame */
    mov ebp, [esi+HALP_BIOS_FRAME_TRAP_FRAME]
    mov eax, [esi+HALP_BIOS_FRAME_SS]
    mov [ebp+KTRAP_FRAME_SS], eax
    mov eax, [esi+HALP_BIOS_FRAME_ESP]
    mov [ebp+KTRAP_FRAME_ESP], eax
    mov eax, [esi+HALP_BIOS_FRAME_CS]
    mov [ebp+KTRAP_FRAME_CS], eax
    mov eax, [esi+HALP_BIOS_FRAME_EFLAGS]
    mov [ebp+KTRAP_FRAME_EFLAGS], eax

    /* Set success code */
    mov eax, 1
    
Done:
    /* Restore volatiles */
    pop ebx
    pop edi
    pop ebp
    ret 0
    
EipLimitReached:
    /* Set failure code */
    xor eax, eax
    jmp Done
.endfunc

.globl _HalpDispatchV86Opcode@0
.func HalpDispatchV86Opcode@0
_HalpDispatchV86Opcode@0:

    /* Make space for the HAL BIOS Frame on the stack */
    push ebp
    mov ebp, esp
    sub esp, HALP_BIOS_FRAME_LENGTH
    
    /* Save non-volatiles */
    push esi
    push edi

    /* Save pointer to the trap frame */
    mov esi, [ebp]
    mov [ebp-HALP_BIOS_FRAME_LENGTH+HALP_BIOS_FRAME_TRAP_FRAME], esi

    /* Save SS */
    movzx eax, word ptr [esi+KTRAP_FRAME_SS]
    mov [ebp-HALP_BIOS_FRAME_LENGTH+HALP_BIOS_FRAME_SS], eax

    /* Save ESP */
    mov eax, [esi+KTRAP_FRAME_ESP]
    mov [ebp-HALP_BIOS_FRAME_LENGTH+HALP_BIOS_FRAME_ESP], eax

    /* Save EFLAGS */
    mov eax, [esi+KTRAP_FRAME_EFLAGS]
    mov [ebp-HALP_BIOS_FRAME_LENGTH+HALP_BIOS_FRAME_EFLAGS], eax

    /* Save CS */
    movzx eax, word ptr [esi+KTRAP_FRAME_CS]
    mov [ebp-HALP_BIOS_FRAME_LENGTH+HALP_BIOS_FRAME_CS], eax

    /* Save EIP */
    mov eax, [esi+KTRAP_FRAME_EIP]
    mov [ebp-HALP_BIOS_FRAME_LENGTH+HALP_BIOS_FRAME_EIP], eax

    /* No prefix */
    xor eax, eax
    mov [ebp-HALP_BIOS_FRAME_LENGTH+HALP_BIOS_FRAME_PREFIX], eax

    /* Set pointer to HAL BIOS Frame */
    lea esi, [ebp-HALP_BIOS_FRAME_LENGTH]

    /* Convert CS to linear format */
    mov eax, [esi+HALP_BIOS_FRAME_CS]
    shl eax, 4
    mov [esi+HALP_BIOS_FRAME_CS_BASE], eax
    mov dword ptr [esi+HALP_BIOS_FRAME_CS_LIMIT], 0xFFFF
    mov dword ptr [esi+HALP_BIOS_FRAME_CS_FLAGS], 0

    /* Make sure IP is within the CS Limit */
    mov edi, [esi+HALP_BIOS_FRAME_EIP]
    cmp edi, [esi+HALP_BIOS_FRAME_CS_LIMIT]
    ja DispatchError

    /* Convert IP to linear address and read the opcode */
    add edi, [esi+HALP_BIOS_FRAME_CS_BASE]
    mov dl, [edi]

    /* We only deal with interrupts */
    cmp dl, 0xCD
    je DispatchInt

    /* Anything else is invalid */
    call _HalpOpcodeInvalid@0
    jmp DispatchError
    
DispatchInt:
    /* Handle dispatching the interrupt */
    call _HalpOpcodeINTnn@0
    test eax, 0xFFFF
    jz DispatchReturn

    /* Update the trap frame EIP */
    mov edi, [ebp-0x20]
    mov eax, [ebp-0x24]
    mov [edi+KTRAP_FRAME_EIP], eax
    
    /* Set success code */
    mov eax, 1
    
DispatchReturn:
    /* Restore registers and return */
    pop edi
    pop esi
    mov esp, ebp
    pop ebp
    ret 0
    
DispatchError:
    /* Set failure code and return */
    xor eax, eax
    jmp DispatchReturn
.endfunc

.func Ki16BitStackException
_Ki16BitStackException:

    /* Save stack */
    push ss
    push esp

    /* Go to kernel mode thread stack */
    mov eax, PCR[KPCR_CURRENT_THREAD]
    add esp, [eax+KTHREAD_INITIAL_STACK]

    /* Switch to good stack segment */
    UNHANDLED_PATH
.endfunc

.globl _HalpTrap0D@0
.func HalpTrap0D@0
TRAP_FIXUPS htd_a, htd_t, DoFixupV86, DoFixupAbios
_HalpTrap0D@0:

    /* Enter trap */
    TRAP_PROLOG htd_a, htd_t
    
    /* Check if this is a V86 trap */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz DoDispatch

    /* Unhandled */
    UNHANDLED_PATH

DoDispatch:
    /* Handle the opcode */
    call _HalpDispatchV86Opcode@0

    /* Exit the interrupt */
    jmp _Kei386EoiHelper@0
.endfunc

.globl _HalpTrap06@0
.func HalpTrap06@0
_HalpTrap06@0:

    /* Restore DS/ES segments */
    mov eax, KGDT_R3_DATA | RPL_MASK
    mov ds, ax
    mov es, ax

    /* Restore ESP and return */
    mov esp, _HalpSavedEsp
    ret 0
.endfunc
