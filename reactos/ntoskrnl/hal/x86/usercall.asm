;
;

%include "internal/hal/segment.inc"

bits 32
section .text
extern _SystemServiceTable

_kernel_mode_call_handler:

          ;
	  ; Save some registers
	  ; 
          push ds
	  push es
	  push esi
	  push edi
	  
	  ;
	  ; Transfer the parameters from user mode 
	  ;
	  push USER_DS
	  pop es
	  
	  mov edx,esi
	  mov esp,edi
	  mov ecx,_SystemServiceTable[eax*4]
	  sub esp,ecx
	  cld
	  rep movsb
	  
	  ;
	  ; Call the actual service routine
	  ;
	  mov eax,_SystemServiceTable[eax*4+4]
	  jmp eax
	  
	  ;
	  ; Restore registers and return
	  ;
	  pop edi
	  pop esi
	  pop es
	  pop ds
	  ret
