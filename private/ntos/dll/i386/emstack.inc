	subttl	emstack.asm - Emulator Stack Management Macros
	page
;***
;emstack.asm - Emulator Stack Management Area
;
;	 Microsoft Confidential
;	 Copyright (c) Microsoft Corporation 1991
;	 All Rights Reserved
;
;Purpose:
;	Handles emulator stack.
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


;POPSTret:  pops the stack and returns.  Uses esi.

POPSTret        macro	reg
        local   stackwrap
IFB	<reg>
        mov     esi,EMSEG:[CURstk]
_popreg	equ	esi
ELSE
_popreg	equ	reg
ENDIF
	mov     EMSEG:[_popreg].bTag,bTAG_EMPTY
        NextStackElem   _popreg,stackwrap
        mov     EMSEG:[CURstk],_popreg
        ret

Wrap&stackwrap:
	mov	EMSEG:[CURstk],BEGstk
	ret
	endm

;NextStackElem:  Given pST(0) = [CURstk] in reg, returns pST(1)
;Requires NextStackWrap macro with same arguments

NextStackElem	macro	reg,stackwrap
	cmp	reg,INITstk			;JWM
	jae	Wrap&stackwrap
	add	reg,Reg87Len
Cont&stackwrap:
	endm

NextStackWrap	macro	reg,stackwrap
Wrap&stackwrap:
	mov	reg,BEGstk			;JWM
	jmp	Cont&stackwrap
	endm


;PrevStackElem:  Given pST(0) = [CURstk] in reg, returns new pST(0) 
;after a push onto on the stack.
;Requires PrevStackWrap macro with same arguments

PrevStackElem	macro	reg,stackwrap
	cmp	reg,BEGstk			;JWM
	jbe	Wrap&stackwrap
	sub	reg,Reg87Len
Cont&stackwrap:
	endm

PrevStackWrap	macro	reg,stackwrap
Wrap&stackwrap:
	mov	reg,INITstk			;JWM
	jmp	Cont&stackwrap
	endm
