/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: trap.s,v 1.6 2000/12/23 02:37:40 dwelch Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/trap.s
 * PURPOSE:         Exception handlers
 * PROGRAMMER:      David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/
	
#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <ddk/defines.h>

/* FUNCTIONS *****************************************************************/

/*
 * Epilog for exception handlers
 */
_exception_handler_epilog:
	cmpl	$1, %eax       /* Check for v86 recovery */
	jne     _exception_handler_ret
	jmp	_KiV86Complete
_exception_handler_ret:		
        addl    $4, %esp       /* Ignore pointer to trap frame */
	popa                   /* Restore general purpose registers */
	addl	$4, %esp       /* Ignore trap code */
	popl	%ds            /* Restore segment registers */
	popl	%es
	popl	%fs
	popl	%gs
	addl	$4, %esp       /* Ignore error code */
	iret                   /* Return from interrupt */
			
.globl _exception_handler0
_exception_handler0:
	pushl	$0
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$0                        
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs      
	pushl   %esp
        call	_exception_handler
	jmp	_exception_handler_epilog

.globl _exception_handler1
_exception_handler1:
	pushl	$0
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$1
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
	jmp	_exception_handler_epilog


.globl _exception_handler2
_exception_handler2:
	pushl	$0
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$2
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler
	jmp	_exception_handler_epilog

.globl _exception_handler3
_exception_handler3:
	pushl	$0
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$3
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl   %esp
	call _exception_handler	

.globl _exception_handler4
_exception_handler4:
        pushl	$0
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$4
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
	jmp	 _exception_handler_epilog

.globl _exception_handler5
_exception_handler5:
	pushl	$0
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$5
        pusha                          
        movw	$KERNEL_DS,%ax      
	movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
        jmp	_exception_handler_epilog

.globl _exception_handler6
_exception_handler6:
	pushl	$0
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$6
        pusha                          
        movw	$KERNEL_DS,%ax        
	movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
        jmp	_exception_handler_epilog

.globl _exception_handler7
_exception_handler7:
        pushl	$0
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$7
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
	jmp	_exception_handler_epilog

.globl _exception_handler8
_exception_handler8:
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$8
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
	jmp	_exception_handler_epilog

.globl _exception_handler9
_exception_handler9:
        pushl	$0
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$9
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
	movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
        jmp	_exception_handler_epilog

.globl _exception_handler10
_exception_handler10:
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$10
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
	jmp	_exception_handler_epilog

.globl _exception_handler11
_exception_handler11:
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$11
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
	movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
        jmp	_exception_handler_epilog

.globl _exception_handler12
_exception_handler12:
	pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$12
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
	movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
	call	_exception_handler        
	jmp	_exception_handler_epilog

.globl _exception_handler13
_exception_handler13:
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$13
        pusha                          
	movw	 $KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
        jmp	_exception_handler_epilog

.globl _exception_handler14
_exception_handler14:
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$14
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
        jmp	_exception_handler_epilog

.globl _exception_handler15
_exception_handler15:
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$15
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
        jmp	_exception_handler_epilog

.globl _exception_handler16
_exception_handler16:
        pushl	%gs 
        pushl	%fs 
        pushl	%es 
        pushl	%ds    
        pushl	$16
        pusha                          
        movw	$KERNEL_DS,%ax        
        movw	%ax,%ds      
        movw	%ax,%es      
        movw	%ax,%fs      
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler        
        jmp	_exception_handler_epilog
	 
.globl _exception_handler_unknown
_exception_handler_unknown:
        pushl	$0
        pushl	%gs
	pushl	%fs
        pushl	%es
        pushl	%ds
        pushl	$0xff
        pusha                         
        movw	$KERNEL_DS,%ax
        movw	%ax,%ds
        movw	%ax,%es
        movw	%ax,%fs
        movw	%ax,%gs
	pushl	%esp
        call	_exception_handler
	jmp	_exception_handler_epilog


/* EOF */
