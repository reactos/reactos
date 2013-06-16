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
 *  _SEH3$_RegisterFrame(
 *       PSEH_REGISTRATION_FRAME RegistrationRecord<ecx>,
 *       PSEH_DATA_TABLE DataTable<eax>);
 */
.global __SEH3$_RegisterFrame
__SEH3$_RegisterFrame:

    /* Save the address of the static data table */
    mov [ecx + SEH3_REGISTRATION_FRAME_ScopeTable], eax

    /* Set the handler address */
    mov dword ptr [ecx + SEH3_REGISTRATION_FRAME_Handler], offset __SEH3$_except_handler

    /* Set this as the end of the internal chain */
    mov dword ptr [ecx + SEH3_REGISTRATION_FRAME_EndOfChain], ecx

    /* Register the frame in the TEB */
    mov eax, dword ptr fs:[0]
    mov [ecx + SEH3_REGISTRATION_FRAME_Next], eax
    mov dword ptr fs:[0], ecx

    /* Save the registers */
    mov [ecx + SEH3_REGISTRATION_FRAME_Esp], esp
    mov [ecx + SEH3_REGISTRATION_FRAME_Ebp], ebp

    ret


.global __SEH3$_RegisterTryLevel
__SEH3$_RegisterTryLevel:

    /* Save the address of the static data table */
    mov [ecx + SEH3_REGISTRATION_FRAME_ScopeTable], eax

    /* Set the handler address to NULL as identification */
    and dword ptr [ecx + SEH3_REGISTRATION_FRAME_Handler], 0

    /* Get the current registered frame */
    mov eax, dword ptr fs:[0]

    /* Get the current end of the chain and set this as Next */
    mov edx, [eax + SEH3_REGISTRATION_FRAME_EndOfChain]
    mov [ecx + SEH3_REGISTRATION_FRAME_Next], edx

    /* Set this as the end of the internal chain */
    mov dword ptr [eax + SEH3_REGISTRATION_FRAME_EndOfChain], ecx

    /* Save the registers */
    mov [ecx + SEH3_REGISTRATION_FRAME_Esp], esp
    mov [ecx + SEH3_REGISTRATION_FRAME_Ebp], ebp

    ret


