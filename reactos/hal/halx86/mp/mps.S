/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/mps.S
 * PURPOSE:         Intel MultiProcessor specification support
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *                  Created 12/04/2001
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>

/* FUNCTIONS *****************************************************************/

#define BEFORE \
  cld; \
  pusha; \
  pushl %ds; \
  pushl %es; \
  pushl %fs; \
  pushl %gs; \
  movl  $(KGDT_R0_DATA), %eax; \
  movl  %eax, %ds; \
  movl  %eax, %es; \
  movl  %eax, %gs; \
  movl  $(KGDT_R0_PCR), %eax; \
  movl  %eax, %fs;
  
#define AFTER \
  popl  %gs; \
  popl  %fs; \
  popl  %es; \
  popl  %ds; \
  popa;

.global _MpsIpiInterrupt
_MpsIpiInterrupt:
  /* Save registers */	
  BEFORE

  /* Call the C handler */
  call  _MpsIpiHandler

	/* Return to the caller */
  AFTER
  iret
  
  
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
	cld
	pusha	
	movl	$0xef,%ebx		
  	pushl 	%ds
  	pushl 	%es
  	pushl 	%fs
  	pushl	%gs
  	movl	$0xceafbeef,%eax
  	pushl 	%eax
  	movl	$(KGDT_R0_DATA),%eax
  	movl	%eax,%ds
  	movl	%eax,%es
  	movl	%eax,%gs
  	movl	$(KGDT_R0_PCR),%eax
  	movl	%eax,%fs
  	pushl 	%esp
  	pushl 	%ebx
  	call	_MpsTimerHandler
    	popl	%eax
  	popl	%eax
  	popl	%eax
  	popl	%gs
  	popl	%fs
  	popl	%es
  	popl	%ds
  	popa
  	iret
  
  
  
/* EOF */
