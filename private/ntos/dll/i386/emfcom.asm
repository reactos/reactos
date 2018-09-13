	subttl  emfcom.asm - Comparison Instructions
	page
;*******************************************************************************
;emfcom.asm - Comparison Instructions
;
;        Microsoft Confidential
;
;        Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Purpose:
;       FCOM,FCOMP,FCOMPP,FUCOM,FUCOMP,FUCOMPP,FTST,FXAM instructions
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


;*******************************************************************************
;Dispatch table for compare
;
;One operand has been loaded into ecx:ebx:esi ("source"), the other is
;pointed to by edi ("dest").  
;
;Tag of source is shifted.  Tag values are as follows:
.erre   TAG_SNGL        eq      0       ;SINGLE: low 32 bits are zero
.erre   TAG_VALID       eq      1
.erre   TAG_ZERO        eq      2
.erre   TAG_SPCL        eq      3       ;NAN, Infinity, Denormal, Empty
;Any special case routines not found in this file are in emarith.asm
tFcomDisp       label   dword           ;Source (reg)   Dest (*[di] = ST)
        dd      ComDouble               ;single         single
        dd      ComDouble               ;single         double
	dd	ComDestZero		;single		zero
        dd      ComSpclDest             ;single         special
        dd      ComDouble               ;double         single
        dd      ComDouble               ;double         double
        dd      ComDestZero             ;double         zero
        dd      ComSpclDest             ;double         special
        dd      ComSrcZero              ;zero           single
        dd      ComSrcZero              ;zero           double
        dd      ComEqual                ;zero           zero
	dd	ComSpclDest		;zero		special
	dd	ComSpclSource		;special	single
	dd	ComSpclSource		;special	double
	dd	ComSpclSource		;special	zero
	dd	ComBothSpcl		;special	special


EM_ENTRY eFICOMP16
eFICOMP16:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset PopWhenDone
	push	offset ComOpLoaded
	jmp	Load16Int		;Returns to ComOpLoaded

EM_ENTRY eFICOM16
eFICOM16:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset ComOpLoaded
	jmp	Load16Int		;Returns to ComOpLoaded

EM_ENTRY eFICOMP32
eFICOMP32:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset PopWhenDone
	push	offset ComOpLoaded
	jmp	Load32Int		;Returns to ComOpLoaded

EM_ENTRY eFICOM32
eFICOM32:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset ComOpLoaded
	jmp	Load32Int		;Returns to ComOpLoaded

EM_ENTRY eFCOMP32
eFCOMP32:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset PopWhenDone
	push	offset ComOpLoaded
	jmp	Load32Real		;Returns to ComOpLoaded

EM_ENTRY eFCOM32
eFCOM32:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset ComOpLoaded
	jmp	Load32Real		;Returns to ComOpLoaded

EM_ENTRY eFCOMP64
eFCOMP64:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset PopWhenDone
	push	offset ComOpLoaded
	jmp	Load64Real		;Returns to ComOpLoaded

EM_ENTRY eFCOM64
eFCOM64:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset ComOpLoaded
	jmp	Load64Real		;Returns to ComOpLoaded

EM_ENTRY eFUCOMPP
eFUCOMPP:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset ComPop2
	jmp	eFUCOM0

EM_ENTRY eFUCOMP
eFUCOMP:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset PopWhenDone
	jmp	eFUCOM0

EM_ENTRY eFUCOM
eFUCOM:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
eFUCOM0:	
;esi = pointer to st(i) from instruction field
;edi = [CURstk]
	mov	ecx,EMSEG:[esi].ExpSgn
	mov	ebx,EMSEG:[esi].lManHi
	mov	esi,EMSEG:[esi].lManLo
	mov	dl,40H			;Flag FUCOM - Look for SNAN
	jmp	UComOpLoaded

EM_ENTRY eFCOMPP
eFCOMPP:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset ComPop2
	jmp	eFCOM0

EM_ENTRY eFCOMP
eFCOMP:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset PopWhenDone
	jmp	eFCOM0

EM_ENTRY eFCOM
eFCOM:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
eFCOM0:
;esi = pointer to st(i) from instruction field
;edi = [CURstk]
	mov	ecx,EMSEG:[esi].ExpSgn
	mov	ebx,EMSEG:[esi].lManHi
	mov	esi,EMSEG:[esi].lManLo

ComOpLoaded:
;	mov	EMSEG:[UpdateCCodes],1
	mov	dl,0			;flag FCOM - Look for any NAN
UComOpLoaded:
	mov     ebp,offset tFcomDisp
	mov	al,cl
	mov     ah,EMSEG:[edi].bTag
	test	ax,ZEROorSPCL * 100H + ZEROorSPCL
	jnz	TwoOpDispatch

;.erre	ComDouble eq $			;Fall into ComDouble

;*********
ComDouble:
;*********
;
;ebx:esi = op1 mantissa
;ecx = op1 sign in bit 15, exponent in high half
;edi = pointer to op2
	mov	eax,EMSEG:[edi].ExpSgn
	and	ax,bSign shl 8		;Keep sign only
	and	cx,bSign shl 8
	cmp	ah,ch			;Are signs the same?
	jnz	StBigger
	cmp	eax,ecx			;Are exponents the same?
	jl	StSmaller
	jg	StBigger
	cmp	EMSEG:[edi].lManHi,ebx	;Compare mantissas
	jnz	MantDif
	cmp	EMSEG:[edi].lManLo,esi	;Set flags for ST - src
	jz	ComEqual
MantDif:
	adc	al,al			;Copy CY flag to bit 0
	rol	ah,1			;Rotate sign to bit 0
	xor	al,ah			;Flip saved CY bit if negative
	mov	EMSEG:[SWcc],al		;Set condition code
	ret

StSmaller:
	not	ah
StBigger:
;ah = sign of ST
;ch = sign of other operand
;ST is bigger if it is positive (smaller if it is negative).
;Use the sign bit directly as the "less than" bit C0.
.erre	C0 eq 1
	shr	ah,7			;Bring sign down to bit 0, clear CY
	mov	EMSEG:[SWcc],ah		;Bit set if ST smaller (negative)
	ret

ComEqual:
	mov	EMSEG:[SWcc],CCequal
	ret



PopWhenDone:
.erre	bTAG_NOPOP eq -1
	inc	cl			;OK to pop?
	jz	ComPopX			;No - had unmasked Invalid Operation

	POPSTret

ComPop2:
.erre	bTAG_NOPOP eq -1
	inc	cl			;OK to pop?
	jz	ComPopX			;No - had unmasked Invalid Operation
	mov	esi,EMSEG:[CURstk]
	mov	EMSEG:[esi].bTag,bTAG_EMPTY
	add	esi,Reg87Len*2
	cmp	esi,ENDstk			;JWM
	je	PopOneOver
	ja	PopTwoOver
	mov	EMSEG:[esi-Reg87Len].bTag,bTAG_EMPTY
	mov	EMSEG:[CURstk],esi
ComPopX:
	ret

PopOneOver:
	mov	EMSEG:[CURstk],BEGstk		;JWM
ifdef NT386
	mov	EMSEG:[INITstk].bTAG,bTAG_EMPTY
else
	mov	EMSEG:[XINITstk].bTAG,bTAG_EMPTY
endif
	ret

PopTwoOver:
	mov	EMSEG:[CURstk],BEGstk+Reg87Len	;JWM
ifdef NT386
	mov	EMSEG:[BEGstk].bTAG,bTAG_EMPTY
else
	mov	EMSEG:[XBEGstk].bTAG,bTAG_EMPTY
endif
	ret

;*******************************************************************************
;Special cases for FCOM/FUCOM.
;These don't share with those in emarith.asm because NANs are treated
;differently.
ComDestZero:
;ST is zero, so Src is bigger if it is positive (smaller if it is negative).
;Use the sign bit directly as the "less than" bit C0.
	not	ch			;C0 is 1 if ST < Src
.erre	C0 eq 1
	shr	ch,7			;Bring sign down to bit 0
	mov	EMSEG:[SWcc],ch		;Bit set if Src smaller (negative)
	ret

ComSrcZero:
;ST is bigger if it is positive (smaller if it is negative).
;Use the sign bit directly as the "less than" bit C0.
	mov	al,EMSEG:[edi].bSgn
.erre	C0 eq 1
	shr	al,7			;Bring sign down to bit 0
	mov	EMSEG:[SWcc],al		;Bit set if ST smaller (negative)
	ret

ComSpclSource:
	cmp	cl,bTAG_NAN
	jz	ComSrcNAN
	cmp	cl,bTAG_INF
	jz	ComDestZero
	cmp	cl,bTAG_DEN
	jz	ComDenormal
;Must be empty
ComEmpty:
	mov	EMSEG:[CURerr],Invalid+StackFlag
	jmp	ComChkMask

ComSrcNAN:
	shl	edx,24			;Move dl to high byte
	test	ebx,edx			;See if we report error with this NAN
ComChkNAN:
	jnz	Incomp
ComInvalid:
	mov	EMSEG:[CURerr],Invalid	;Flag the error
ComChkMask:
	test	EMSEG:[CWmask],Invalid	;Is exception masked?
	jnz	Incomp
	mov	cl,bTAG_NOPOP		;Unmasked, don't pop stack
Incomp:
	mov	EMSEG:[SWcc],CCincomprable
	ret

ComSpclDest:
	mov	al,EMSEG:[edi].bTag
	cmp	al,bTAG_INF
	jz	ComSrcZero
	cmp	al,bTAG_Empty
	jz	ComEmpty
	cmp	al,bTAG_DEN
	jz	ComDenormal
;Must be NAN
ComDestNAN:
	test	EMSEG:[edi].bMan7,dl		;See if we report error with this NAN
	jmp	ComChkNAN

ComBothSpcl:
	mov	al,EMSEG:[edi].bTag
	cmp	cl,bTAG_EMPTY
	jz	ComEmpty
	cmp	al,bTAG_EMPTY
	jz	ComEmpty
	cmp	cl,bTAG_NAN
	jz	ComSrcNAN
	cmp	al,bTAG_NAN
	jz	ComDestNAN
	mov	ah,cl
	cmp	ax,(bTAG_INF shl 8) + bTag_INF	;Are both Infinity?
	jz	ComDouble		;If so, compare their signs
;Must have at least one denormal
ComDenormal:
	or	EMSEG:[CURerr],Denormal
        jmp     ComDouble

;*******************************************************************************

XAM_Unsupported	equ	0
XAM_NAN		equ	C0
XAM_Norm	equ	C2
XAM_Inf		equ	C2+C0
XAM_Zero	equ	C3
XAM_Empty	equ	C3+C0
XAM_Den		equ	C3+C2

tXamTag	label	byte
.erre	TAG_SNGL	eq	$-tXamTag
	db	XAM_Norm		;TAG_SNGL
.erre	TAG_VALID	eq	$-tXamTag
	db	XAM_Norm		;TAG_VALID
.erre	TAG_ZERO	eq	$-tXamTag
	db	XAM_Zero		;TAG_ZERO
.erre	TAG_EMPTY	eq	$-tXamTag
	db	XAM_Empty		;TAG_EMPTY
	db	0
	db	0
	db	0
.erre	TAG_INF 	eq	$-tXamTag
	db	XAM_Inf 		;TAG_INF
	db	0
	db	0
	db	0
.erre	TAG_NAN 	eq	$-tXamTag
	db	XAM_NAN 		;TAG_NAN
	db	0
	db	0
	db	0
.erre	TAG_DEN 	eq	$-tXamTag
	db	XAM_Den 		;TAG_DEN

EM_ENTRY eFXAM
eFXAM:
;edi = [CURstk]
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	mov	eax,EMSEG:[edi].ExpSgn	;Get sign and tag
	mov	bl,ah			;Save sign
	and	bl,bSign		;Keep only sign bit
	and	eax,0FH			;Save low 4 bits of tag
	mov	al,tXamTag[eax]		;Lookup cond. codes for this tag
.erre	C1 eq 2		;Bit 1
.erre	bSign eq 80H	;Bit 7
	shr	bl,7-1			;Move sign bit to CC C1
	or	al,bl
	mov	EMSEG:[SWcc],al
	ret

;*******************************************************************************

EM_ENTRY eFTST
eFTST:
;edi = [CURstk]
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	mov	eax,EMSEG:[edi].ExpSgn
	cmp	al,bTAG_ZERO
	jz	ComEqual
	ja	TestSpcl
;Either single or double, non-zero.  Just check sign.
TestSign:
	shr	ah,7			;Bring sign down to bit 0
	mov	EMSEG:[SWcc],ah		;Bit set if negative
	ret

TestSpcl:
	cmp	al,bTAG_INF
	jz	TestSign		;Normal test for Infinity
	cmp	al,bTAG_EMPTY
	jz	ComEmpty
	cmp	al,bTAG_NAN
	jz	ComInvalid
;Must be denormal
	mov	EMSEG:[CURerr],Denormal
	jmp	TestSign
