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
 *  __attribute__((regparm(2)))
 *  __attribute__((returns_twice))
 *  _SEH3$_RegisterFrame[WithNonVolatiles](
 *       PSEH3$_REGISTRATION_FRAME RegistrationFrame<eax>,
 *       PSEH3$_SCOPE_TABLE ScopeTable<edx>);
 */
.global __SEH3$_RegisterFrameWithNonVolatiles
__SEH3$_RegisterFrameWithNonVolatiles:

    /* Save non-volatiles in the registration frame */
    mov [eax + SEH3_REGISTRATION_FRAME_Ebx], ebx
    mov [eax + SEH3_REGISTRATION_FRAME_Esi], esi
    mov [eax + SEH3_REGISTRATION_FRAME_Edi], edi

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
 *  __attribute__((regparm(2)))
 *  __attribute__((returns_twice))
 *  _SEH3$_RegisterTryLevel[WithNonVolatiles](
 *       PSEH3$_REGISTRATION_FRAME RegistrationFrame<edx>,
 *       PSEH3$_SCOPE_TABLE ScopeTable<eax>);
 */
.global __SEH3$_RegisterTryLevelWithNonVolatiles
__SEH3$_RegisterTryLevelWithNonVolatiles:

    /* Save non-volatiles in the registration frame */
    mov [eax + SEH3_REGISTRATION_FRAME_Ebx], ebx
    mov [eax + SEH3_REGISTRATION_FRAME_Esi], esi
    mov [eax + SEH3_REGISTRATION_FRAME_Edi], edi

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

