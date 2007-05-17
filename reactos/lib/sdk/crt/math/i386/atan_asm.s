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
        fld     qword ptr [esp+4]
        fpatan
        ret
