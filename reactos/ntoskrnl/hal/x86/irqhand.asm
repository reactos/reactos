;
; This module has code to handle IRQs 
;

%include "internal/i386/segment.inc"

bits 32
section .text
extern _KiInterruptDispatch

%macro IRQ_HANDLER_FIRST 1
global _irq_handler_%1
_irq_handler_%1:

        ;
	; Save registers
	;
        pusha
        push    ds
	
	;
	; Load DS
	;
        mov     ax,KERNEL_DS
        mov     ds,ax
	
	;
	; Mask the corresponding vector at the PIC
	; NOTE: Since lower priority interrupts are masked until an EOI
	; is sent to the PIC this preserves the IRQL semantics. 
	;
        in      al,0x21
        or      al,1<<%1	
        out     0x21,al
	
	;
	; Call the irq dispatcher (passing the IRQ number as an argument)
	;
        push    dword %1
        call    _KiInterruptDispatch
	
	;
	; Restore stack, registers and return to interrupted routine
	;
        pop     eax
        pop     ds
        popa
        iret
%endmacro

%macro IRQ_HANDLER_SECOND 1
global _irq_handler_%1
_irq_handler_%1:
        ;
	; Save registers
	;
        pusha
        push ds
	
	;
	; Load DS
	;
        mov ax,KERNEL_DS
        mov ds,ax
	
	;
	; Mask the related vector at the PIC
	;
        in al,0xa1
        or al,1<<(%1-8)
        out 0xa1,al
	
	;
	; Call the irq dispatcher
	;
        push dword %1
        call _KiInterruptDispatch
	
	;
	; Restore stack, registers and return to the interrupted routine
	;
        pop  eax
        pop  ds
        popa
        iret
%endmacro


IRQ_HANDLER_FIRST 0
IRQ_HANDLER_FIRST 1
IRQ_HANDLER_FIRST 2
IRQ_HANDLER_FIRST 3
IRQ_HANDLER_FIRST 4
IRQ_HANDLER_FIRST 5
IRQ_HANDLER_FIRST 6
IRQ_HANDLER_FIRST 7
IRQ_HANDLER_SECOND 8
IRQ_HANDLER_SECOND 9
IRQ_HANDLER_SECOND 10
IRQ_HANDLER_SECOND 11
IRQ_HANDLER_SECOND 12
IRQ_HANDLER_SECOND 13
IRQ_HANDLER_SECOND 14
IRQ_HANDLER_SECOND 15


