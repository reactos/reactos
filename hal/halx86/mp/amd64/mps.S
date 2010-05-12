/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/amd64/generic/mps.S
 * PURPOSE:         Intel MultiProcessor specification support
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES ******************************************************************/

#include <reactos/asm.h>
#include <ndk/amd64/asm.h>

/* FUNCTIONS *****************************************************************/

#define BEFORE \
  cld; \
  push rax; \
  push rbx; \
  push rcx; \
  push rdx; \
  push rsi; \
  push rdi; \
  push fs; \
  push gs; \
  
#define AFTER \
  pop  gs; \
  pop  fs; \
  pop  rdi; \
  pop  rsi; \
  pop  rdx; \
  pop  rcx; \
  pop  rbx; \
  pop  rax; \

#ifdef CONFIG_SMP
PUBLIC MpsIpiInterrupt
MpsIpiInterrupt:
    /* Save registers */	
    BEFORE

    /* Call the C handler */
    call  MpsIpiHandler

    /* Return to the caller */
    AFTER
    iret
#endif
  
PUBLIC MpsErrorInterrupt
MpsErrorInterrupt:
    /* Save registers */	
    BEFORE

    /* Call the C handler */
    call  MpsErrorHandler

    /* Return to the caller */
    AFTER
    iret


PUBLIC MpsSpuriousInterrupt
MpsSpuriousInterrupt:
    /* Save registers */	
    BEFORE

    /* Call the C handler */
    call  MpsSpuriousHandler

    /* Return to the caller */
    AFTER
    iret

PUBLIC MpsTimerInterrupt
MpsTimerInterrupt:
    /* Save registers */	
    BEFORE

    mov	ebx, 0xef
  	mov eax, 0xceafbeef
  	push rax
  	push rsp
  	push rbx
  	call MpsTimerHandler
   	pop rax
  	pop rax
  	pop rax

    /* Return to the caller */
  	AFTER
  	iret
  
  
/* EOF */
