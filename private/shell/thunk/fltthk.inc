; File: fltthk.inc
;
;   Support macros and routines for the flat thunk mode of the thunk
;   compiler.
;
;   Included in *.asm files generated using the 
;   "flatthunks = true" semantic.
;





; Macro: FAPILOG16. Prints out a log message at the start of each thunk.
; Debug only.
FAPILOG16	macro	dwOffset
ifdef DEBUG
	pushd	offset FT_ThunkLogNames + dwOffset
	call	FAPILOG16_Hlp
endif ;DEBUG
endm ;FAPILOG16




ifdef DEBUG

LogApiThkLSF	proto	near stdcall, psz:dword

FAPILOG16_Hlp:
	push	ebp
	mov	ebp,[esp+8]

	invoke	LogApiThkLSF, ebp

	pop	ebp
	ret	4

endif ;DEBUG


