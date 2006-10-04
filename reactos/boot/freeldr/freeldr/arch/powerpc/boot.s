	.extern PpcInit
_start:
	b	PpcInit

	.globl _bss
	.section ".bss"
_bss:
	.long	0
