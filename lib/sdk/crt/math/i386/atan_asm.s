/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           
 * FILE:              
 * PROGRAMER:         Magnus Olsen (magnus@greatlord.com)
 *                    
 */
 
.globl _atan

.intel_syntax noprefix

/* FUNCTIONS ***************************************************************/

_atan:
        push    ebp
        mov     ebp,esp            
        fld     qword ptr [ebp+8]   
        fpatan                      
        pop     ebp
        ret
