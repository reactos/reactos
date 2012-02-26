/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/kernel32/thread/i386/fiber.S
 * PURPOSE:     Fiber context switch code for the x86 architecture
 * PROGRAMMERS: Alex Ionescu (alex@relsoft.net)
 *              KJK::Hyperion <noog@libero.it>
 */

#include <asm.inc>
#include <ks386.inc>

EXTERN _BaseThreadStartup@8:PROC

.code

PUBLIC _BaseFiberStartup@0
.PROC _BaseFiberStartup@0
    /* Frame pointer is zeroed */
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    /* Get the fiber data */
    mov eax, fs:[TEB_FIBER_DATA]

    push dword ptr [eax+FIBER_CONTEXT_EBX] /* Parameter */
    push dword ptr [eax+FIBER_CONTEXT_EAX] /* Start Address */
    call _BaseThreadStartup@8
.ENDP


PUBLIC _SwitchToFiber@4
_SwitchToFiber@4:
    /* Get the TEB */
    mov edx, fs:[TEB_SELF]

    /* Get the Fiber */
    mov eax, [edx+TEB_FIBER_DATA]

    /* Save the non-volatile registers */
    mov [eax+FIBER_CONTEXT_EBX], ebx
    mov [eax+FIBER_CONTEXT_ESI], esi
    mov [eax+FIBER_CONTEXT_EDI], edi
    mov [eax+FIBER_CONTEXT_EBP], ebp

    /* Check if we're to save FPU State */
    cmp dword ptr [eax+FIBER_CONTEXT_FLAGS], CONTEXT_FULL OR CONTEXT_FLOATING_POINT
    jnz NoFpuStateSave

    /* Save the FPU State (Status and Control)*/
    fstsw [eax+FIBER_CONTEXT_FLOAT_SAVE_STATUS_WORD]
    fnstcw [eax+FIBER_CONTEXT_FLOAT_SAVE_CONTROL_WORD]

    /* Check if the CPU supports SIMD MXCSR State Save */
    cmp byte ptr ds:[PF_XMMI_INSTRUCTIONS_AVAILABLE], 1
    jnz NoFpuStateSave
    stmxcsr [eax+FIBER_CONTEXT_DR6]

NoFpuStateSave:

    /* Save stack since we're not touching it anymore */
    mov [eax+FIBER_CONTEXT_ESP], esp

    /* Transfer some data from the TEB */
    mov ecx, [edx+TEB_FLS_DATA]
    mov [eax+FIBER_FLS_DATA], ecx
    mov ecx, [edx+TEB_ACTIVATION_CONTEXT_STACK_POINTER]
    mov [eax+FIBER_ACTIVATION_CONTEXT_STACK], ecx

    /* Transfer some data related to the Stack */
    mov ecx, [edx+TEB_EXCEPTION_LIST]
    mov [eax+FIBER_EXCEPTION_LIST], ecx
    mov ecx, [edx+TEB_STACK_LIMIT]
    mov [eax+FIBER_STACK_LIMIT], ecx
    mov ecx, [edx+TEB_GUARANTEED_STACK_BYTES]
    mov [eax+FIBER_GUARANTEED_STACK_BYTES], ecx

    /* Switch to the new fiber */
    mov ecx, [esp+4]
    mov [edx+TEB_FIBER_DATA], ecx

    /* Switch Fiber Data */
    mov esi, [ecx+FIBER_EXCEPTION_LIST]
    mov [edx+TEB_EXCEPTION_LIST], esi
    mov esi, [ecx+FIBER_STACK_BASE]
    mov [edx+TEB_STACK_BASE], esi
    mov esi, [ecx+FIBER_STACK_LIMIT]
    mov [edx+TEB_STACK_LIMIT], esi
    mov esi, [ecx+FIBER_DEALLOCATION_STACK]
    mov [edx+TEB_DEALLOCATION_STACK], esi
    mov esi, [ecx+FIBER_GUARANTEED_STACK_BYTES]
    mov [edx+TEB_GUARANTEED_STACK_BYTES], esi
    mov esi, [ecx+FIBER_ACTIVATION_CONTEXT_STACK]
    mov [edx+TEB_ACTIVATION_CONTEXT_STACK_POINTER], esi

    /* Restore FPU State */
    cmp dword ptr [eax+FIBER_CONTEXT_FLAGS], CONTEXT_FULL OR CONTEXT_FLOATING_POINT
    jnz NoFpuStateRestore

    /* Check if the Status Word Changed */
    mov esi, [eax+FIBER_CONTEXT_FLOAT_SAVE_STATUS_WORD]
    cmp si, word ptr [ecx+FIBER_CONTEXT_FLOAT_SAVE_STATUS_WORD]
    jnz StatusWordChanged

    /* Check if the Control Word Changed */
    mov esi, [eax+FIBER_CONTEXT_FLOAT_SAVE_CONTROL_WORD]
    cmp si, word ptr [ecx+FIBER_CONTEXT_FLOAT_SAVE_CONTROL_WORD]
    jz ControlWordEqual

StatusWordChanged:

    /* Load the new one */
    mov word ptr [ecx+FIBER_CONTEXT_FLOAT_SAVE_TAG_WORD], HEX(0FFFF)
    fldenv [ecx+FIBER_CONTEXT_FLOAT_SAVE_CONTROL_WORD]

ControlWordEqual:

    /* Load the new one */
    cmp byte ptr ds:[PF_XMMI_INSTRUCTIONS_AVAILABLE], 1
    jnz NoFpuStateRestore
    ldmxcsr [ecx+FIBER_CONTEXT_DR6]

NoFpuStateRestore:

    /* Restore non-volatile registers */
    mov esi, [ecx+FIBER_CONTEXT_ESI]
    mov edi, [ecx+FIBER_CONTEXT_EDI]
    mov ebx, [ecx+FIBER_CONTEXT_EBX]
    mov ebp, [ecx+FIBER_CONTEXT_EBP]
    mov esp, [ecx+FIBER_CONTEXT_ESP]

    /* Restore FLS Data */
    mov eax, [ecx+FIBER_FLS_DATA]
    mov [edx+TEB_FLS_DATA], eax

    /* Jump to new fiber */
    mov esp, [ecx+FIBER_CONTEXT_ESP]
    ret 4


END
/* EOF */
