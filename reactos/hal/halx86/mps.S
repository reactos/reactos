/* $Id: mps.S,v 1.1 2001/08/21 20:18:27 chorns Exp $
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

#include <internal/i386/segment.h>

/* FUNCTIONS *****************************************************************/

#define BEFORE \
  pusha; \
  pushl %ds; \
  pushl %es; \
  pushl %fs; \
  pushl %gs; \
  movl  $(KERNEL_DS), %eax; \
  movl  %eax, %ds; \
  movl  %eax, %es; \
  movl  %eax, %gs; \
  movl  $(PCR_SELECTOR), %eax; \
  movl  %eax, %fs;
  
#define AFTER \
  popl  %gs; \
  popl  %fs; \
  popl  %es; \
  popl  %ds; \
  popa;

.globl _MpsTimerInterrupt
_MpsTimerInterrupt:
  /* Save registers */	
  BEFORE

  /* Call the C handler */
  call  _MpsTimerHandler

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

/* EOF */
