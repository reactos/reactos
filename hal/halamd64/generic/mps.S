/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/amd64/generic/mps.S
 * PURPOSE:         Intel MultiProcessor specification support
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>

/* FUNCTIONS *****************************************************************/

#define BEFORE \
  cld; \
  pushq %rax; \
  pushq %rbx; \
  pushq %rcx; \
  pushq %rdx; \
  pushq %rsi; \
  pushq %rdi; \
  pushq %fs; \
  pushq %gs; \
  
#define AFTER \
  popq  %gs; \
  popq  %fs; \
  popq  %rdi; \
  popq  %rsi; \
  popq  %rdx; \
  popq  %rcx; \
  popq  %rbx; \
  popq  %rax; \

#ifdef CONFIG_SMP
.global _MpsIpiInterrupt
_MpsIpiInterrupt:
    /* Save registers */	
    BEFORE

    /* Call the C handler */
    call  _MpsIpiHandler

    /* Return to the caller */
    AFTER
    iret
#endif
  
.globl _MpsErrorInterrupt
_MpsErrorInterrupt:
    /* Save registers */	
    BEFORE

    /* Call the C handler */
    call  _MpsErrorHandler

    /* Return to the caller */
    AFTER
    iret


.globl _MpsSpuriousInterrupt
_MpsSpuriousInterrupt:
    /* Save registers */	
    BEFORE

    /* Call the C handler */
    call  _MpsSpuriousHandler

    /* Return to the caller */
    AFTER
    iret

.global _MpsTimerInterrupt
_MpsTimerInterrupt:
    /* Save registers */	
    BEFORE

    movl	$0xef,%ebx		
  	movl	$0xceafbeef,%eax
  	pushq 	%rax
  	pushq 	%rsp
  	pushq 	%rbx
  	call	_MpsTimerHandler
   	popq	%rax
  	popq	%rax
  	popq	%rax

    /* Return to the caller */
  	AFTER
  	iret
  
  
/* EOF */
