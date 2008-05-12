
 /*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           
 * FILE:              
 * PROGRAMER:         Magnus Olsen (magnus@greatlord.com)
 *                    
 */
 
.globl _log10

.intel_syntax noprefix

/* FUNCTIONS ***************************************************************/

_log10:

    push    ebp
    mov     ebp,esp
    fld     qword ptr [ebp+8]       // Load real from stack
    fldlg2                          // Load log base 10 of 2
    fxch    st(1)                   // Exchange st, st(1)
    fyl2x                           // Compute the log base 10(x)
    pop     ebp
    ret

