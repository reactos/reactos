        subttl  emfmisc.asm - FABS, FCHS, FFREE, FXCH
        page
;*******************************************************************************
;emfmisc.asm - FABS, FCHS, FFREE, FXCH
;
;        Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Purpose:
;       FABS, FCHS, FFREE, FXCH instructions
;Inputs:
;	edi = [CURstk]
;	esi = pointer to st(i) from instruction field
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


;******
EM_ENTRY eFABS
eFABS:
;******
	cmp	EMSEG:[edi].bTag,bTAG_EMPTY
	jz	StackError		;in emarith.asm
	mov	EMSEG:[edi].bSgn,0		;Turn sign bit off
	ret

;******
EM_ENTRY eFCHS
eFCHS:
;******
	cmp	EMSEG:[edi].bTag,bTAG_EMPTY
	jz	StackError		;in emarith.asm
	not	EMSEG:[edi].bSgn		;Flip the sign
	ret

;******
EM_ENTRY eFFREE
eFFREE:
;******
	mov	EMSEG:[esi].bTag,bTAG_EMPTY
	ret

;******
EM_ENTRY eFXCH
eFXCH:
;******
	cmp	EMSEG:[esi].bTag,bTAG_EMPTY
	jz	XchDestEmpty
XchgChkSrc:
	cmp	EMSEG:[edi].bTag,bTAG_EMPTY
	jz	XchSrcEmpty
DoSwap:
;Swap [esi] with [edi]
	mov	eax,EMSEG:[edi]
	xchg	eax,EMSEG:[esi]
	mov	EMSEG:[edi],eax
	mov	eax,EMSEG:[edi+4]
	xchg	eax,EMSEG:[esi+4]
	mov	EMSEG:[edi+4],eax
	mov	eax,EMSEG:[edi+8]
	xchg	eax,EMSEG:[esi+8]
	mov	EMSEG:[edi+8],eax
	ret

XchDestEmpty:
	call	ReturnIndefinite	;in emarith.asm - ZF set if unmasked
	jnz	XchgChkSrc		;Continue if masked
	ret

XchSrcEmpty:
	xchg	edi,esi			;pass pointer in esi
	call	ReturnIndefinite	;in emarith.asm - ZF set if unmasked
	xchg	edi,esi
	jnz	DoSwap			;Continue if masked
	ret
