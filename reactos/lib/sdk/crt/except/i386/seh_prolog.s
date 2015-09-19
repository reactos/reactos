/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS CRT
 * FILE:            lib/sdk/crt/except/i386/seh_prolog.s
 * PURPOSE:         SEH Support for MSVC
 * PROGRAMMERS:     Timo Kreuzer
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

EXTERN __except_handler3:PROC

/* The very first thing a function compiled with MSVC containing SEH
 * will do is call __SEH_prolog like this:
 *
 *  push <Number of stackbytes>
 *  push <Address of exception handler>
 *  call __SEH_prolog
 *
 * When entering the function the stack layout is like this:
 *
 *  esp + 08: OLDFRAME.StackBytes
 *  esp + 04: OLDFRAME.SEHTable
 *  esp + 00: OLDFRAME.ReturnAddress
 *
 * __SEH_prolog will now setup the stack to the following layout:
 *
 *  esp + N + 24: SEH_FRAME.OriginalEbp       OLDFRAME.StackBytes
 *  esp + N + 20: SEH_FRAME.Disable           OLDFRAME.SEHTable
 *  esp + N + 1C: SEH_FRAME.SEHTable          OLDFRAME.ReturnAddress
 *  esp + N + 18: SEH_FRAME.Handler
 *  esp + N + 14: SEH_FRAME.PreviousRecord
 *  esp + N + 10: SEH_FRAME.unused
 *  esp + N + 0c: SEH_FRAME.NewEsp
 *
 *           N bytes local variables
 *  ...
 *  esp +     08: SAFE_AREA.Ebx
 *  esp +     04: SAFE_AREA.Esi
 *  esp +     00: SAFE_AREA.Edi
 *
 * all this is documented here (with some minor errors):
 * http://reactos-blog.blogspot.com/2009/08/inside-mind-of-reactos-developer.html
 */

OLDFRAME_ReturnAddress   =  0 /* 0x00 */
OLDFRAME_SEHTable        =  4 /* 0x04 */
OLDFRAME_StackBytes      =  8 /* 0x08 */
OLDFRAME_Size            = 12 /* 0x0c */

SEH_FRAME_NewEsp         =  0 /* 0x00 */
SEH_FRAME_unused         =  4 /* 0x04 */
SEH_FRAME_PreviousRecord =  8 /* 0x08 */
SEH_FRAME_Handler        = 12 /* 0x0c */
SEH_FRAME_SEHTable       = 16 /* 0x10 */
SEH_FRAME_Disable        = 20 /* 0x14 */
SEH_FRAME_OriginalEbp    = 24 /* 0x18 */
SEH_FRAME_Size           = 28 /* 0x1c */

SAFE_AREA_Edi            =  0 /* 0x00 */
SAFE_AREA_Esi            =  4 /* 0x04 */
SAFE_AREA_Ebx            =  8 /* 0x08 */
SAFE_AREA_Size           = 12 /* 0x0c */


.code

PUBLIC __SEH_prolog
__SEH_prolog:

    /* Get the number of stack bytes to reserve */
    mov eax, [esp + OLDFRAME_StackBytes]

    /* Push address of __except_handler3 on the stack */
    push offset __except_handler3

    /* Push the old exception record on the stack */
    push dword ptr fs:0

    /* Adjust stack allocation, add size of the stack frame minus 2 pushes */
    add eax, SEH_FRAME_Size + SAFE_AREA_Size - OLDFRAME_Size - 8

    /* Save old ebp, overwriting OLDFRAME.StackBytes */
    mov [esp + 8 + OLDFRAME_StackBytes], ebp

    /* Load new ebp, pointing to OLDFRAME.StackBytes */
    lea ebp, [esp + 8 + OLDFRAME_StackBytes]

    /* Allocate stack space */
    sub esp, eax

    /* Push the return address on the stack */
    push dword ptr [ebp - OLDFRAME_StackBytes + OLDFRAME_ReturnAddress]

    /* Get address of the SEH table */
    mov eax, [ebp + OLDFRAME_SEHTable]

    /* Save new esp */
    mov [ebp - SEH_FRAME_OriginalEbp + SEH_FRAME_NewEsp], esp

    /* Safe SEH table, overwriting OLDFRAME.ReturnAddress */
    mov [ebp - SEH_FRAME_OriginalEbp + SEH_FRAME_SEHTable], eax

    /* Safe the disable value, overwriting OLDFRAME.SEHTable */
    mov dword ptr [ebp - SEH_FRAME_OriginalEbp + SEH_FRAME_Disable], -1

    /* Load the address of the new registration record */
    lea eax, [ebp - SEH_FRAME_OriginalEbp + SEH_FRAME_PreviousRecord]

    /* Save registers */
    mov [esp + SAFE_AREA_Edi], edi
    mov [esp + SAFE_AREA_Esi], esi
    mov [esp + SAFE_AREA_Ebx], ebx

    /* Enqueue the new record */
    mov fs:[0], eax

    /* Return to the caller */
    ret


PUBLIC __SEH_epilog
__SEH_epilog:

    /* Restore the previous exception registration record */
    mov ecx, [ebp - SEH_FRAME_OriginalEbp + SEH_FRAME_PreviousRecord]
    mov fs:[0], ecx

    /* Restore saved registers */
    mov edi, [esp + 4 + SAFE_AREA_Edi]
    mov esi, [esp + 4 + SAFE_AREA_Esi]
    mov ebx, [esp + 4 + SAFE_AREA_Ebx]

    /* Get the return address */
    mov ecx, [esp]

    /* Clean up stack */
    mov esp, ebp

    /* Get previous ebp */
    mov ebp, [esp]

    /* Save return address */
    mov [esp], ecx

    /* Return to the caller */
    ret


END
