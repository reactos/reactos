;
;

%include 'internal/hal/segment.inc'

bits 32
section .text
extern __SystemServiceTable
global _interrupt_handler2e

_interrupt_handler2e:
         push ds
	 push es
	 push esi
	 push edi
	 
	 mov bx,KERNEL_DS
	 mov es,bx
	 
	 mov esi,edx
	 mov ecx,[es:__SystemServiceTable+eax*8]
	 sub esp,ecx
	 mov edi,esp
	 rep movsb
	 
	 mov ds,bx
	 	 
	 mov eax,[__SystemServiceTable+4+eax*8]
	 call eax
	 
	 pop edi
	 pop esi
	 pop es
	 pop ds
	 iret
