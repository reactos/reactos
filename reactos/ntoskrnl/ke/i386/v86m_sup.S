/*
 * FILE:            ntoskrnl/ke/i386/v86m_sup.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Virtual 8086 (V86) Mode Support
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * NOTE:            See asmmacro.S for the V86 trap code.
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* FUNCTIONS *****************************************************************/

.globl _Ki386SetupAndExitToV86Mode@4
.func Ki386SetupAndExitToV86Mode@4
_Ki386SetupAndExitToV86Mode@4:

    /* Save nonvolatiles */
    push ebp
    push ebx
    push esi
    push edi

    /* Give us a little stack */
    sub esp, 12
    mov ecx, esp

    /* Go past the KTRAP_FRAME and NPX Frame and set a new frame in EAX */
    sub esp, NPX_FRAME_LENGTH
    and esp, ~15
    sub esp, KTRAP_FRAME_LENGTH
    mov eax, esp

    /* Create a fake user-mode frame */
    mov dword ptr [eax+KTRAP_FRAME_CS], KGDT_R0_CODE + RPL_MASK
    mov dword ptr [eax+KTRAP_FRAME_ES], 0
    mov dword ptr [eax+KTRAP_FRAME_DS], 0
    mov dword ptr [eax+KTRAP_FRAME_FS], 0
    mov dword ptr [eax+KTRAP_FRAME_GS], 0
    mov dword ptr [eax+KTRAP_FRAME_ERROR_CODE], 0

    /* Get the current thread's initial stack */
    mov ebx, [fs:KPCR_SELF]
    mov edi, [ebx+KPCR_CURRENT_THREAD]
    mov edx, [edi+KTHREAD_INITIAL_STACK]
    sub edx, NPX_FRAME_LENGTH

    /* Save it on our stack, as well as the real TEB addresses */
    mov [ecx], edx
    mov edx, [edi+KTHREAD_TEB]
    mov [ecx+4], edx
    mov edx, [fs:KPCR_TEB]
    mov [ecx+8] , edx

    /* Set our ESP in ESI, and the return function in EIP */
    mov edi, offset _Ki386BiosCallReturnAddress
    mov [eax+KTRAP_FRAME_ESI], ecx
    mov [eax+KTRAP_FRAME_EIP], edi

    /* Push the flags and sanitize them */
    pushfd
    pop edi
    and edi, 0x60DD7
    or edi, EFLAGS_INTERRUPT_MASK

    /* Set SS and ESP, and fill out the rest of the frame */
    mov dword ptr [eax+KTRAP_FRAME_SS], KGDT_R3_DATA + RPL_MASK;
    mov dword ptr [eax+KTRAP_FRAME_ESP], 0x11FFE;
    mov dword ptr [eax+KTRAP_FRAME_EFLAGS], edi
    mov dword ptr [eax+KTRAP_FRAME_EXCEPTION_LIST], -1
    mov dword ptr [eax+KTRAP_FRAME_PREVIOUS_MODE], -1
    mov dword ptr [eax+KTRAP_FRAME_DR7], 0
    mov dword ptr [eax+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00

    /* Jump past the frame now */
    add eax, KTRAP_FRAME_LENGTH
    cli

    /* Save the current stack */
    push ecx

    /* Get the current thread's intial stack again */
    mov edi, [ebx+KPCR_CURRENT_THREAD]
    mov esi, [edi+KTHREAD_INITIAL_STACK]
    sub esi, NPX_FRAME_LENGTH

    /* Set the size of the copy, and the destination, and copy the NPX frame */
    mov ecx, NPX_FRAME_LENGTH / 4
    mov edi, eax
    rep movsd

    /* Restore stack */
    pop ecx

    /* Get the current thread and TSS */
    mov edi, [ebx+KPCR_CURRENT_THREAD]
    mov esi, [ebx+KPCR_TSS]

    /* Bias the V86 vrame */
    sub eax, KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS

    /* Set exception list and new ESP */
    mov dword ptr [ebx+KPCR_EXCEPTION_LIST], -1
    mov [esi+KTSS_ESP0], eax

    /* Now skip past the NPX frame and V86 fields and set this as the intial stack */
    add eax, NPX_FRAME_LENGTH + (KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS)
    mov [edi+KTHREAD_INITIAL_STACK], eax

    /* Setup our fake TEB pointer */
    mov eax, [ecx+0x20]
    mov [fs:KPCR_TEB], eax
    mov [edi+KTHREAD_TEB], eax

    /* Setup the descriptors for the fake TEB */
    mov ebx, [fs:KPCR_GDT]
    mov [ebx+0x3A], ax
    shr eax, 16
    mov [ebx+0x3C], al
    mov [ebx+0x3F], ah
    sti

    /*
     * Start VDM execution. This will save this fake 32-bit KTRAP_FRAME and
     * initialize a real 16-bit VDM context frame
     */
    push 0
    push 0 // VdmStartExecution
    call _NtVdmControl@8

    /* Exit to V86 mode */
    mov ebp, esp
    jmp _Kei386EoiHelper@0
.endfunc

.globl _Ki386BiosCallReturnAddress
.func Ki386BiosCallReturnAddress
_Ki386BiosCallReturnAddress:

    /* Get the PCR */
    mov eax, [fs:KPCR_SELF]

    /* Get NPX destination */
    mov edi, [ebp+KTRAP_FRAME_ESI]
    mov edi, [edi]

    /* Get initial stack */
    mov ecx, [eax+KPCR_CURRENT_THREAD]
    mov esi, [ecx+KTHREAD_INITIAL_STACK]
    sub esi, NPX_FRAME_LENGTH

    /* Set length and copy the NPX frame */
    mov ecx, NPX_FRAME_LENGTH / 4
    rep movsd

    /* Restore stack */
    mov esp, [ebp+KTRAP_FRAME_ESI]
    add esp, 4

    /* Set initial stack */
    mov ecx, [eax+KPCR_CURRENT_THREAD]
    mov [ecx+KTHREAD_INITIAL_STACK], edi

    /* Get TSS and set the ESP 0 */
    mov eax, [eax+KPCR_TSS]
    sub edi, NPX_FRAME_LENGTH + (KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS)
    mov [eax+KTSS_ESP0], edi

    /* Restore KTHREAD TEB in EDX */
    pop edx
    mov [ecx+KTHREAD_TEB], edx

    /* Restore PCR TEB in EDX */
    pop edx
    mov [fs:KPCR_TEB], edx

    /* Setup the descriptors for the real TEB */
    mov ebx, [fs:KPCR_GDT]
    mov [ebx+0x3A], dx
    shr edx, 16
    mov [ebx+0x3C], dl
    mov [ebx+0x3F], dh

    /* Enable interrupts and pop back non-volatiles */
    sti
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret 4
.endfunc
