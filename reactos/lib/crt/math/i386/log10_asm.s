
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
        fld     qword ptr [ebp+8]   
        fldlg2  
        fyl2x                    
        pop     ebp
        ret

