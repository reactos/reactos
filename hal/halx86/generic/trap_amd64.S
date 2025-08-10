/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         AMD64 trap handlers for HAL
 * PROGRAMMERS:     ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <ksamd64.inc>

#ifdef _M_AMD64

/* FUNCTIONS *****************************************************************/

.code64

/*
 * HalpClockInterrupt
 *
 * Handle the clock interrupt (IRQ0)
 */
PUBLIC HalpClockInterrupt
.PROC HalpClockInterrupt
    /* Create a minimal trap frame on the stack */
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .setframe rbp, 0
    .endprolog
    
    /* Save volatile registers we'll use */
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    
    /* Call HalpClockInterruptHandler with trap frame in RCX */
    mov rcx, rsp
    sub rsp, 0x20  /* Shadow space for call */
    call HalpClockInterruptHandler
    add rsp, 0x20
    
    /* Restore registers */
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax
    
    /* Return from interrupt */
    pop rbp
    iretq
.ENDP

#endif /* _M_AMD64 */

END