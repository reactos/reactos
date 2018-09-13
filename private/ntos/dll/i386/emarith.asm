	subttl  emarith.asm - Arithmetic Operations
	page
;*******************************************************************************
;emarith.asm - Arithmetic Operations
;
;        Microsoft Confidential
;
;        Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Purpose:
;       Arithmetic Operations
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************

	NextStackWrap   esi,TwoOp       ;Tied to NextStackElem below

EM_ENTRY eFPREM
eFPREM:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset PremCont		;Return address if normal
PremPointTopTwo:
	push	offset PremSpclDone	;Return address if special
	mov	ebp,offset tFpremDisp
PointTopTwo:
	mov	esi,edi
	NextStackElem   esi,TwoOp
TwoOpSiDi:
	mov	ecx,EMSEG:[esi].ExpSgn
	mov	ebx,EMSEG:[esi].lManHi
	mov	esi,EMSEG:[esi].lManLo
TwoOpSetResult:
	mov	EMSEG:[Result],edi		;Save result pointer
TwoOpResultSet:
	mov     ah,EMSEG:[edi].bTag
TwoOpDispAh:
	mov	al,cl
TwoOpDispatch:
	and     eax,TAG_MASK + 100H*TAG_MASK	;Look at internal tags only
	shl     al,TAG_SHIFT
	or      al,ah
	xor	ah,ah			;Zero ah
;UNDONE:  masm bug!  ebp + scaled index requires a displacement.
;UNDONE:  No displacement is needed here, so masm should generate a
;UNDONE:  zero.  It doesn't!  dec eax so we can add 4*1 back.
	dec	eax			;UNDONE
	jmp     dword ptr cs:[ebp+4*eax+4];UNDONE Go to appropriate routine.

EM_ENTRY eFPREM1
eFPREM1:
    and		[esp].[OldLongStatus+4],NOT(ConditionCode SHL 16)	;clear C0,C1,C2,C3
	push	offset Prem1Cont	;Return address if normal
	jmp	PremPointTopTwo

EM_ENTRY eFSCALE
eFSCALE:
	mov	ebp,offset tFscaleDisp
	jmp	PointTopTwo

EM_ENTRY eFPATAN
eFPATAN:
	mov	ebp,offset tFpatanDisp
TopTwoPop:
	push	offset PopWhenDone
	mov	esi,edi
	add	edi,Reg87Len		;edi = ST(1)
        cmp     edi,ENDstk
	jb	TwoOpSiDi
        mov     edi,BEGstk
	jmp	TwoOpSiDi

EM_ENTRY eFYL2X
eFYL2X:
	mov	ebp,offset tFyl2xDisp
	jmp	TopTwoPop

EM_ENTRY eFYL2XP1
eFYL2XP1:
	mov	ebp,offset tFyl2xp1Disp
	jmp	TopTwoPop

;*******************************************************************************

page
;-----------------------------------------------------------;
;                                                           ;
;       Special Case Routines for Arithmetic Functions      ;
;                                                           ;
;-----------------------------------------------------------;

;There are four kinds of "specials", encoded in the tag:
;
;	Empty
; 	Infinity
;	NAN (which can be QNAN or SNAN)
;	Denormal
;
;Empty always results in an Invalid Operation exception with Stack Flag set
;and C1 (O/U#) bit clear, and returns Indefinite (a specific QNAN).
;
;Operations on NAN return the same NAN except it is always modified to a 
;QNAN.  If both  operands are NAN, the one with the larger mantissa is
;returned.  An SNAN causes an Invalid Operation exception except for
;internal FP stack operations, FCHS, and FABS.  A QNAN does not cause
;and exception.  
;
;Operations on Infinity return a result depending on the operation.
;
;UNDONE: Old code plays with sign of NAN when two NANs with equal
;mantissas are used.  Why?

;"***" means entry point from dispatch tables

;***
DivSpclSource:
	cmp	cl,bTAG_INF
	jnz	SpclSource
;Division by infinity always returns zero
	xor	ch,EMSEG:[edi].bSgn
	jmp	SignedZero		;in emfmul.asm

;***
MulSpclSource:
	cmp	cl,bTAG_INF
	jnz	SpclSource
MulByInf:
	cmp	EMSEG:[edi].bTag,bTAG_ZERO	;Infinity * zero?
	jz	ReturnIndefinite
XorSourceSign:
	xor	ch,EMSEG:[edi].bSgn
	jmp	SaveResultEdi

;***
AddSpclSource:
	cmp	cl,bTAG_INF
	jnz	SpclSource
	xor	ch,dl			;Flip sign of infinity if subtracting
	jmp	SaveResultEdi

DenormalSource:
	mov	cl,bTAG_VALID		;Change denormal to DOUBLE
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal	;Is denormal exception masked?
	jnz	TwoOpResultSet
AbortOp:
	mov	cl,bTAG_NOPOP		;Unmasked, don't pop stack
	ret

DenormalDisp:
;Repeat dispatch, but for normal ops
	jmp     dword ptr cs:[ebp+4*(TAG_VALID + TAG_VALID shl TAG_SHIFT)]

;***
DivrSpclSource:
	cmp	cl,bTAG_INF
	jz	XorSourceSign		;Return infinity
SpclSource:
	cmp	cl,bTAG_DEN
	jz	DenormalSource
	cmp	cl,bTAG_EMPTY
	jz	StackError
;Must be a NAN
SourceNAN:
	test	ebx,1 shl 30		;Check for SNAN
	jnz	SaveResultEdi		;If QNAN, just use it as result
SourceSNAN:
	or	EMSEG:[CURerr],Invalid	;Flag the error
	or	ebx,1 shl 30		;Make it into a QNAN
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jnz	SaveResultEdi		;If so, update with masked response
	mov	cl,bTAG_NOPOP		;Unmasked, don't pop stack
	ret


;***
DivrSpclDest:
	mov	eax,EMSEG:[edi].ExpSgn	;Pick up tag
	cmp	al,bTAG_INF
	jnz	SpclDest
;Division by infinity always returns zero
	xor	ch,ah
	jmp	SignedZero		;in emfmul.asm

;***
MulSpclDest:
	mov	al,EMSEG:[edi].bTag	;Pick up tag
	cmp	al,bTAG_INF
	jnz	SpclDest
	cmp	cl,bTAG_ZERO		;Infinity * zero?
	jz	ReturnIndefinite
XorDestSign:
	xor	EMSEG:[edi].bSgn,ch	;Xor signs
	ret

;***
AddSpclDest:
	mov	al,EMSEG:[edi].bTag	;Pick up tag
	cmp	al,bTAG_INF
	jnz	SpclDest
	xor	EMSEG:[edi].bSgn,dh	;Flip sign of infinity if subtracting
	ret

DenormalDest:
	mov	ah,bTAG_VALID		;Change denormal to DOUBLE
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal	;Is denormal exception masked?
	jnz	TwoOpDispAh
	mov	cl,bTAG_NOPOP		;Unmasked, don't pop stack
	ret

;***
DivSpclDest:
	mov	al,EMSEG:[edi].bTag	;Pick up tag
	cmp	al,bTAG_INF
	jz	XorDestSign		;Return infinity
SpclDest:
	cmp	al,bTAG_DEN
	jz	DenormalDest
SpclDestNotDen:
	cmp	al,bTAG_EMPTY
	jz	StackError
;Must be a NAN
DestNAN:
	test	EMSEG:[edi].bMan7,40H	;Check for SNAN
	jnz	ReturnDest		;If QNAN, just use it as result
DestSNAN:
	or	EMSEG:[CURerr],Invalid	;Flag the error
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jz	AbortOp			;No - preserve value
	or	EMSEG:[edi].bMan7,40H	;Make it into a QNAN
	ret

StackError:
	mov	EMSEG:[CURerr],Invalid+StackFlag
ReturnIndefinite:
	or 	EMSEG:[CURerr],Invalid
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jz	AbortOp			;No - preserve value
	mov	EMSEG:[edi].lManLo,0
	mov	EMSEG:[edi].lManHi,0C0000000H
	mov	EMSEG:[edi].ExpSgn,TexpMax shl 16 + bSign shl 8 + bTAG_NAN
ReturnDest:
	ret


AddTwoInf:
;Adding two infinites.
;If signs are the same, return that infinity.  Otherwise, Invalid Operation.
	xor	ch,dl			;Possibly subtracting source
	xor	ah,dh			;Possibly subtracting dest
	xor	ch,ah			;Compare signs
	js	ReturnIndefinite
	mov	EMSEG:[edi].bSgn,ah	;Correct the sign if subtracting
	ret

;***
TwoOpBothSpcl:
;ebp = dispatch table address
	mov	al,EMSEG:[edi].bTag
	mov	ah,cl
	cmp	ax,(bTAG_NAN shl 8) + bTag_NAN	;Are both NAN?
	jz	TwoNANs
	cmp	cl,bTAG_EMPTY
	jz	StackError
	cmp	al,bTAG_EMPTY
	jz	StackError
	cmp	cl,bTAG_NAN
	jz	SourceNAN
	cmp	al,bTAG_NAN
	jz	DestNAN
	cmp	ax,(bTAG_INF shl 8) + bTag_INF	;Are both infinity?
	jz	TwoInfs
;At least one of the operands is a denormal
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal	;Is denormal exception masked?
	jz	AbortOp			;If not, don't do operation
;Denormal exception is masked, treat denormals as VALID
;Dispatch through operation table in ebp again
	cmp	ax,(bTAG_DEN shl 8) + bTag_DEN	;Are both denormal?
	jz	DenormalDisp
;Have an infinity and a denormal
	cmp	al,bTAG_INF
	jz	DestInf
;Source is denormal, Dest is infinity
	jmp	dword ptr [ebp+4*(TAG_SPCL + TAG_VALID shl TAG_SHIFT)]

DestInf:
;Source is infinity, Dest is denormal
	jmp	dword ptr [ebp+4*(TAG_VALID + TAG_SPCL shl TAG_SHIFT)]

TwoNANs:
;Two NANs. Use largest mantissa
	cmp	ebx,EMSEG:[edi].lManHi
	ja	BiggerNAN
	jb	DestBigger
;Now we know they're both the same type, SNAN or QNAN
	cmp	esi,EMSEG:[edi].lManLo
	ja	SourceNAN
;UNDONE: Old code did funny business with signs when mantissas were equal
	jmp	DestNAN

BiggerNAN:
	test	EMSEG:[edi].bMan7,40H		;Is smaller one SNAN?
	jz	SourceSNAN
	jmp	SourceNAN

DestBigger:
	test	ebx,40H			;Is smaller one SNAN?
	jz	DestSNAN
	jmp	DestNAN

TwoInfs:
        mov     ah,EMSEG:[edi].bSgn
	jmp	dword ptr [ebp+4*16]	;Go do code for two infinites


;***
DivideByMinusZero:
	mov	ch,bSign
;***
DivideByZero:
	or	EMSEG:[CURerr],ZeroDivide
	test	EMSEG:[CWmask],ZeroDivide	;Is exception masked?
	jz	AbortOp			;No - preserve value
;Set up a signed infinity
	xor	ch,EMSEG:[edi].bSgn		;Get result sign
	and	ecx,1 shl 15		;Keep only sign bit
	or	ecx,(4000H+TexpBias) shl 16 + bTAG_INF	;ExpSgn of infinity
	mov	ebx,1 shl 31
	xor	esi,esi
	jmp	SaveResultEdi
