%include 'internal/i386/segment.inc'

BITS 32
segment .text
extern _KeApcProlog2

DECLARE_GLOBAL_SYMBOL KeApcProlog
        pushad
	push eax
	call _KeApcProlog2
	pop eax
	popad
	pop eax
        iretd

