BITS 32

EXTERN _NtDisplayString

_main:
        push dword _string
	call _NtDisplayString
l1:
        jmp l1

_string db 'Hello world from user mode!',0xa,0
