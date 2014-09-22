/*
 * PROJECT:         ReactOS system libraries
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 * PURPOSE:         Support library for PSEH3
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "pseh3_asmdef.h"

.intel_syntax noprefix

.text

.extern __SEH3$_except_handler

/*
 *  void
 *  __attribute__((regparm(3)))
 *  __attribute__((returns_twice))
 *  _SEH3$_RegisterFrame[WithNonVolatiles](
 *       PSEH3$_REGISTRATION_FRAME RegistrationFrame<eax>,
 *       PSEH3$_SCOPE_TABLE ScopeTable<edx>,
 *       PVOID AllocaFrame<ecx>);
 */
.global __SEH3$_RegisterFrameWithNonVolatiles
__SEH3$_RegisterFrameWithNonVolatiles:

    /* Save non-volatiles in the registration frame */
    mov [eax + SEH3_REGISTRATION_FRAME_Ebx], ebx
    mov [eax + SEH3_REGISTRATION_FRAME_Esi], esi
    mov [eax + SEH3_REGISTRATION_FRAME_Edi], edi

    /* Safe the return address */
    mov ebx, [esp]
    mov [eax + SEH3_REGISTRATION_FRAME_ReturnAddress], ebx
    mov ebx, [eax + SEH3_REGISTRATION_FRAME_Ebx]

.global __SEH3$_RegisterFrameWithStackLayout
__SEH3$_RegisterFrameWithStackLayout:

    /* Save the pointer to the alloca frame */
    mov [eax + SEH3_REGISTRATION_FRAME_AllocaFrame], ecx

.global __SEH3$_RegisterFrame
__SEH3$_RegisterFrame:

    /* Save the address of the static data table */
    mov [eax + SEH3_REGISTRATION_FRAME_ScopeTable], edx

    /* Set the handler address */
    mov dword ptr [eax + SEH3_REGISTRATION_FRAME_Handler], offset __SEH3$_except_handler

    /* Set this as the end of the internal chain */
    mov dword ptr [eax + SEH3_REGISTRATION_FRAME_EndOfChain], eax

    /* Register the frame in the TEB */
    mov edx, dword ptr fs:[0]
    mov [eax + SEH3_REGISTRATION_FRAME_Next], edx
    mov dword ptr fs:[0], eax

    /* Save the stack registers */
    mov [eax + SEH3_REGISTRATION_FRAME_Esp], esp
    mov [eax + SEH3_REGISTRATION_FRAME_Ebp], ebp

    /* Set eax to 0 to indicate 1st return */
    xor eax, eax
    ret


/*
 *  void
 *  __attribute__((regparm(3)))
 *  __attribute__((returns_twice))
 *  _SEH3$_RegisterTryLevel[WithNonVolatiles](
 *       PSEH3$_REGISTRATION_FRAME RegistrationFrame<eax>,
 *       PSEH3$_SCOPE_TABLE ScopeTable<edx>,
 *       PVOID AllocaFrame<ecx>);
 */
.global __SEH3$_RegisterTryLevelWithNonVolatiles
__SEH3$_RegisterTryLevelWithNonVolatiles:

    /* Save non-volatiles in the registration frame */
    mov [eax + SEH3_REGISTRATION_FRAME_Ebx], ebx
    mov [eax + SEH3_REGISTRATION_FRAME_Esi], esi
    mov [eax + SEH3_REGISTRATION_FRAME_Edi], edi

    /* Safe the return address */
    mov ebx, [esp]
    mov [eax + SEH3_REGISTRATION_FRAME_ReturnAddress], ebx
    mov ebx, [eax + SEH3_REGISTRATION_FRAME_Ebx]

.global __SEH3$_RegisterTryLevelWithStackLayout
__SEH3$_RegisterTryLevelWithStackLayout:

    /* Save the pointer to the alloca frame */
    mov [eax + SEH3_REGISTRATION_FRAME_AllocaFrame], ecx

.global __SEH3$_RegisterTryLevel
__SEH3$_RegisterTryLevel:

    /* Save the address of the static data table */
    mov [eax + SEH3_REGISTRATION_FRAME_ScopeTable], edx

    /* Set the handler address to NULL as identification */
    and dword ptr [eax + SEH3_REGISTRATION_FRAME_Handler], 0

    /* Get the current registered frame */
    mov edx, dword ptr fs:[0]

    /* Get the current end of the chain and set this as Next */
    mov ecx, [edx + SEH3_REGISTRATION_FRAME_EndOfChain]
    mov [eax + SEH3_REGISTRATION_FRAME_Next], ecx

    /* Set this as the end of the internal chain */
    mov dword ptr [edx + SEH3_REGISTRATION_FRAME_EndOfChain], eax

    /* Save the stack registers */
    mov [eax + SEH3_REGISTRATION_FRAME_Esp], esp
    mov [eax + SEH3_REGISTRATION_FRAME_Ebp], ebp

    /* Set eax to 0 to indicate 1st return */
    xor eax, eax
    ret


.global __SEH3$_InvokeEmbeddedFilterFromRegistration
__SEH3$_InvokeEmbeddedFilterFromRegistration:

    /* Safe the current non-volatiles */
    push ebp
    push ebx
    push esi
    push edi

    /* Save the registration frame pointer */
    push eax

    /* Load the non-volatiles from the registration invocation */
    mov ebx, [eax + SEH3_REGISTRATION_FRAME_Ebx]
    mov esi, [eax + SEH3_REGISTRATION_FRAME_Esi]
    mov edi, [eax + SEH3_REGISTRATION_FRAME_Edi]
    mov ebp, [eax + SEH3_REGISTRATION_FRAME_Ebp]

    /* Calculate the size of the temp stack frame region */
    mov ecx, [eax + SEH3_REGISTRATION_FRAME_AllocaFrame]
    sub ecx, [eax + SEH3_REGISTRATION_FRAME_Esp]

    /* Put the return address on the stack */
    push offset __SEH3$_InvokeEmbeddedFilterReturnClang

    /* Save the current stack pointer in the AllocaFrame member */
    mov [eax + SEH3_REGISTRATION_FRAME_AllocaFrame], esp

    /* Allocate enough temp stack space on the stack */
    sub esp, ecx

    /* Get the return address that was saved when registering the frame */
    mov edx, [eax + SEH3_REGISTRATION_FRAME_ReturnAddress]

    /* Jump into the filter or finally function */
    xor eax, eax
    inc eax
    jmp edx

    /* We return to this label with a cleaned up stack */
__SEH3$_InvokeEmbeddedFilterReturnClang:

    /* Restore the registration frame pointer */
    pop ecx

    /* Save the non-volatiles back in the registration frame */
    mov [ecx + SEH3_REGISTRATION_FRAME_Ebx], ebx
    mov [ecx + SEH3_REGISTRATION_FRAME_Esi], esi
    mov [ecx + SEH3_REGISTRATION_FRAME_Edi], edi
    mov [ecx + SEH3_REGISTRATION_FRAME_Ebp], ebp

    /* Restore the current non-volatiles */
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret


.global __SEH3$_InvokeEmbeddedFilter
__SEH3$_InvokeEmbeddedFilter:

    /* Safe the current non-volatiles */
    push ebp
    push ebx
    push esi
    push edi

    /* Load ebp from the registration invocation */
    mov ebp, [eax + SEH3_REGISTRATION_FRAME_Ebp]

    /* Calculate the size of the temp stack frame region */
    mov ecx, [eax + SEH3_REGISTRATION_FRAME_AllocaFrame]
    sub ecx, [eax + SEH3_REGISTRATION_FRAME_Esp]

    /* Put the return address on the stack */
    push offset __SEH3$_InvokeEmbeddedFilterReturn

    /* Save the current stack pointer in the AllocaFrame member */
    mov [eax + SEH3_REGISTRATION_FRAME_AllocaFrame], esp

    /* Allocate enough temp stack space on the stack */
    sub esp, ecx

    /* Get the scope table */
    mov edx, [eax + SEH3_REGISTRATION_FRAME_ScopeTable]

    /* Jump into the filter or finally function */
    jmp [edx + SEH3_SCOPE_TABLE_Filter]

    /* We return to this label with a cleaned up stack */
__SEH3$_InvokeEmbeddedFilterReturn:

    /* Restore the current non-volatiles */
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret

/*
 * void
 * __fastcall
 * _SEH3$_CallRtlUnwind(
 *     PSEH3$_REGISTRATION_FRAME RegistrationFrame<ecx>)
 */
.global @_SEH3$_CallRtlUnwind@4
@_SEH3$_CallRtlUnwind@4:

    push ebp
    mov ebp, esp

    push edi
    push esi
    push ebx

    push 0   /* ReturnValue */
    push 0   /* ExceptionRecord */
    push 0   /* TargetIp */
    push ecx /* TargetFrame */
    call _RtlUnwind@16

    pop ebx
    pop esi
    pop edi
    pop ebp
    ret

