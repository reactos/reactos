/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/debug.S
 * PROGRAMER:         Alex Ionescu (alex@relsoft.net)
 * REVISION HISTORY:  27/07/2005 Created
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl  _DbgBreakPoint@0
.globl  _DbgBreakPointWithStatus@4
.globl  _DbgUserBreakPoint@0

/* FUNCTIONS ***************************************************************/

_DbgBreakPoint@0:
_DbgUserBreakPoint@0:
    int 3
    ret
 
_DbgBreakPointWithStatus@4:
    mov eax, [esp+4]
    int 3
    ret 4
