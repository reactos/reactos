	subttl	emfconst.asm - Loading of 387 on chip constants
        page
;*******************************************************************************
;emfconst.asm - Loading of 387 on chip constants
;
;        Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Purpose:
;       FLDZ, FLD1, FLDPI, FLDL2T, FLDL2E, FLDLG2, FLDLN2 instructions
;Inputs:
;	edi = [CURstk]
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


	PrevStackWrap	edi,Ld1		;Tied to PrevStackElem below

EM_ENTRY eFLD1
eFLD1:
;edi = [CURstk]
	PrevStackElem	edi,Ld1		;Point to receiving location
	cmp	EMSEG:[edi].bTag,bTAG_EMPTY	;Is it empty?
	jnz	FldErr			;in emload.asm
	mov	EMSEG:[CURstk],edi
	mov	EMSEG:[edi].lManLo,0
	mov	EMSEG:[edi].lManHi,1 shl 31
	mov	EMSEG:[edi].ExpSgn,bTAG_SNGL	;Exponent and sign are zero
	ret


	PrevStackWrap	edi,Ldz		;Tied to PrevStackElem below

EM_ENTRY eFLDZ
eFLDZ:
;edi = [CURstk]
	PrevStackElem	edi,Ldz		;Point to receiving location
	cmp	EMSEG:[edi].bTag,bTAG_EMPTY	;Is it empty?
	jnz	FldErr			;in emload.asm
	mov	EMSEG:[CURstk],edi
	mov	EMSEG:[edi].lManLo,0
	mov	EMSEG:[edi].lManHi,0
	mov	EMSEG:[edi].ExpSgn,bTAG_ZERO	;Exponent and sign are zero
	ret

;*******************************************************************************

;The 5 irrational constants need to be adjusted according to rounding mode.

DefConst	macro	cName,low,high,expon,round
c&cName&lo	equ	low
c&cName&hi	equ	high
c&cName&exp     equ     expon
c&cName&rnd     equ     round
	endm

DefConst	FLDL2T,0CD1B8AFEH,0D49A784BH,00001H,0

DefConst	FLDL2E,05C17F0BCH,0B8AA3B29H,00000H,1

DefConst	FLDLG2,0FBCFF799H,09A209A84H,0FFFEH,1

DefConst	FLDLN2,0D1CF79ACH,0B17217F7H,0FFFFH,1

DefConst	FLDPI,02168C235H,0C90FDAA2H,00001H,1


LoadConstant   macro   cName,nojmp
EM_ENTRY e&cName
e&cName:
	mov	ebx,c&cName&hi
	mov	edx,c&cName&lo
        mov     ecx,c&cName&exp shl 16 + c&cName&rnd
ifb	<nojmp>
	jmp	CommonConst
endif
	endm

LoadConstant	FLDL2T

LoadConstant	FLDL2E

LoadConstant	FLDLG2

LoadConstant	FLDLN2

LoadConstant	FLDPI,nojmp

CommonConst:
;ebx:edx = mantissa of constant, rounded to nearest
;high ecx = exponent 
;ch = sign
;cl = rounding flag: 1 indicates roundup occured for round nearest, else 0
;edi = [CURstk]
	test	EMSEG:[CWcntl],RoundControl	;Check rounding control bits
.erre	RCnear eq 0
	jnz	NotNearConst		;Adjust constant if not round nearest
StoreConst:
	mov	cl,bTAG_VALID
	mov	esi,edx
	jmp	FldCont			;In emload.asm

NotNearConst:
;It is known that the five constants positive irrational numbers.
;This means they are never exact, and chop and round down always
;produce the same answer.  It is also know that the values are such
;that rounding only alters bits in the last byte.
;
;A flag in cl indicates if the number has been rounded up for round
;nearest (1 = rounded up, 0 = rounded down).  In chop and round down 
;modes, this flag can be directly subtracted to reverse the rounding.  
;In round up mode, we want to add (1-flag) = -(flag-1).
.erre	RCchop eq 0CH			;Two bits set only for chop
	test	EMSEG:[CWcntl],RCdown	;DOWN bit set?
	jnz	DirectRoundConst	;If so, it's chop or down
;Round Up mode
	dec	cl			;-1 if round up needed, else 0
DirectRoundConst:
	sub	dl,cl			;Directed rounding
	jmp	StoreConst
