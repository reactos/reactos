section .text

%macro TEB_ENTRY 4
;TEBOFF_%1 equ ($%3+7C4h)
;export %1
global _%1@%4
_%1@%4:
	mov eax, [fs:18h] ; obtain a pointer to the TEB
	jmp [eax+%3+7C4h]
%endmacro
%include "teblist.mac"
%undef TEB_ENTRY
