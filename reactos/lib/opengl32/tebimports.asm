section .text

%macro TEB_ENTRY 4
global _%1@%4
_%1@%4:
	mov eax, [fs:18h] ; obtain a pointer to the TEB
	jmp [eax+%3+7C4h]
%endmacro
%include "teblist.mac"

%macro SLOW_ENTRY 3
global _%1@%3
_%1@%3:
	mov eax, [fs:18h] ; obtain a pointer to the TEB
	mov eax, [eax+0BE8h] ; get glTable pointer
	jmp [eax+4*%3]
%endmacro
%include "slowlist.mac"
