	subttl  emftran.asm - Transcendental instructions
	page
;*******************************************************************************
;	 Copyright (c) Microsoft Corporation 1991
;	 All Rights Reserved
;
;emftran.asm - Transcendental instructions
;	by Tim Paterson
;
;Purpose:
;	F2XM1, FPATAN, FYL2X, FYL2XP1 instructions
;Inputs:
;	edi = [CURstk]
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


;********************* Polynomial Coefficients *********************

;These polynomial coefficients were all taken from "Computer Approximations"
;by J.F. Hart (reprinted 1978 w/corrections).  All calculations and 
;conversions to hexadecimal were done with a character-string calculator
;written in Visual Basic with precision set to 30 digits.  Once the constants
;were typed into this file, all transfers were done with cut-and-paste
;operations to and from the calculator to help eliminate any typographical
;errors.


tAtanPoly	label	word

;These constants are from Hart #5056: atan(x) = x * P(x^2) / Q(x^2),
;accurate to 20.78 digits over interval [0, tan(pi/12)].

	dd	4			;P() is degree four

;  Hart constant
;
;+.16241 70218 72227 96595 08	      E0
;Hex value:    0.A650A5D5050DE43A2C25A8C00 HFFFE
	dq	0A650A5D5050DE43AH
	dw	bTAG_VALID,0FFFEH-1

;+.65293 76545 29069 63960 675	      E1
;Hex value:    0.D0F0A714A9604993AC4AC49A0 H3
	dq	0D0F0A714A9604994H
	dw	bTAG_VALID,03H-1

;+.39072 57269 45281 71734 92684      E2
;Hex value:    0.9C4A507F16530AC3CDDEFA3DE H6
	dq	09C4A507F16530AC4H
	dw	bTAG_VALID,06H-1

;+.72468 55912 17450 17145 90416 9    E2
;Hex value:    0.90EFE6FB30465042CF089D1310 H7
	dq	090EFE6FB30465043H
	dw	bTAG_VALID,07H-1

;+.41066 29181 34876 24224 77349 62   E2
;Hex value:    0.A443E2004BB000B84A5154D44 H6
	dq	0A443E2004BB000B8H
	dw	bTAG_VALID,06H-1

	dd	4			;Q() is degree four

;  Hart constant
;
;+.15023 99905 56978 85827 4928	      E2
;Hex value:    0.F0624CD575B782643AFB912D0 H4
	dq	0F0624CD575B78264H
	dw	bTAG_VALID,04H-1

;+.59578 42201 83554 49303 22456      E2
;Hex value:    0.EE504DDC907DEAEB7D7473B82 H6
	dq	0EE504DDC907DEAEBH
	dw	bTAG_VALID,06H-1

;+.86157 32305 95742 25062 42472      E2
;Hex value:    0.AC508CA5E78E504AB2032E864 H7
	dq	0AC508CA5E78E504BH
	dw	bTAG_VALID,07H-1

;+.41066 29181 34876 24224 84140 84   E2
;Hex value:    0.A443E2004BB000B84F542813C H6
	dq	0A443E2004BB000B8H
	dw	bTAG_VALID,06H-1


;tan(pi/12) = tan(15 deg.) = 2 - sqrt(3) 
;= 0.26794 91924 31122 70647 25536 58494 12763	;From Hart appendix
;Hex value:    0.8930A2F4F66AB189B517A51F2 HFFFF
Tan15Hi		equ	08930A2F4H
Tan15Lo		equ	0F66AB18AH
Tan15exp	equ	0FFFFH-1

;1/tan(pi/6) = sqrt(3) = 1.73205 08075 68877 29352 74463 41505 87236	;From Hart appendix
;Hex value:    0.DDB3D742C265539D92BA16B8 H1
Sqrt3Hi		equ	0DDB3D742H
Sqrt3Lo		equ	0C265539EH
Sqrt3exp	equ	01H-1

;pi = +3.14159265358979323846264338328
;Hex value:    0.C90FDAA22168C234C4C6628B8 H2
PiHi		equ	0C90FDAA2H
PiLo		equ	02168C235H
PiExp		equ	02H-1

;3*pi = +9.42477796076937971538793014984
;Hex value:    0.96CBE3F9990E91A79394C9E890 H4
XThreePiHi	equ	096CBE3F9H
XThreePiMid	equ	0990E91A7H
XThreePiLo	equ	090000000H
ThreePiExp	equ	04H-1


;This is a table of multiples of pi/6.  It is used to adjust the
;final result angle after atan().  Derived from Hart appendix
;pi/180 = 0.01745 32925 19943 29576 92369 07684 88612
;
;When the reduced argument for atan() is very small, these correction
;constants simply become the result.  These constants have all been
;rounded to nearest, but the user may have selected a different rounding
;mode.  The tag byte is not needed for these constants, so its space
;is used to indicate if it was rounded.  To determine if a constant 
;was rounded, 7FH is subtracted from this flag; CY set means it was
;rounded up.

RoundedUp	equ	040H
RoundedDown	equ	0C0H

tAtanPiFrac	label	dword
;pi/2 = +1.57079632679489661923132169163
;Hex value:    0.C90FDAA22168C234C4C6628B0 H1
	dq	0C90FDAA22168C235H
	dw	RoundedUp,01H-1

;2*pi/3 = +2.09439510239319549230842892218
;Hex value:    0.860A91C16B9B2C232DD997078 H2
	dq	0860A91C16B9B2C23H
	dw	RoundedDown,02H-1

;none
	dd	0,0,0

;pi/6 = +0.523598775598298873077107230544E0
;Hex value:    0.860A91C16B9B2C232DD99707A H0
	dq	0860A91C16B9B2C23H
	dw	RoundedDown,00H-1

;pi/2 = +1.57079632679489661923132169163
;Hex value:    0.C90FDAA22168C234C4C6628B0 H1
	dq	0C90FDAA22168C235H
	dw	RoundedUp,01H-1

;pi/3 = +1.04719755119659774615421446109
;Hex value:    0.860A91C16B9B2C232DD997078 H1
	dq	0860A91C16B9B2C23H
	dw	RoundedDown,01H-1

;pi = +3.14159265358979323846264338328
;Hex value:    0.C90FDAA22168C234C4C6628B8 H2
	dq	0C90FDAA22168C235H
	dw	RoundedUp,02H-1

;5*pi/6 = +2.61799387799149436538553615272
;Hex value:    0.A78D3631C681F72BF94FFCC96 H2
	dq	0A78D3631C681F72CH
	dw	RoundedUp,02H-1

;*********************

tExpPoly	label	word

;These constants are from Hart #1324: 2^x - 1 = 
; 2 * x * P(x^2) / ( Q(x^2) - x * P(x^2) )
;accurate to 21.54 digits over interval [0, 0.5].

	dd	2			;P() is degree two

;  Hart constant
;
;+.60613 30790 74800 42574 84896 07	E2
;Hex value:    0.F27406FCF405189818F68BB78 H6
	dq	0F27406FCF4051898H
	dw	bTAG_VALID,06H-1

;+.30285 61978 21164 59206 24269 927	E5
;Hex value:    0.EC9B3D5414E1AD0852E432A18 HF
	dq	0EC9B3D5414E1AD08H
	dw	bTAG_VALID,0FH-1

;+.20802 83036 50596 27128 55955 242	E7
;Hex value:    0.FDF0D84AC3A35FAF89A690CC4 H15
	dq	0FDF0D84AC3A35FB0H
	dw	bTAG_VALID,015H-1

	dd	3			;Q() is degree three.  First 
					;coefficient is 1.0 and is not listed.
;  Hart constant
;
;+.17492 20769 51057 14558 99141 717	E4
;Hex value:    0.DAA7108B387B776F212ECFBEC HB
	dq	0DAA7108B387B776FH
	dw	bTAG_VALID,0BH-1

;+.32770 95471 93281 18053 40200 719	E6
;Hex value:    0.A003B1829B7BE85CC81BD5309 H13
	dq	0A003B1829B7BE85DH
	dw	bTAG_VALID,013H-1

;+.60024 28040 82517 36653 36946 908	E7
;Hex value:    0.B72DF814E709837E066855BDD H17
	dq	0B72DF814E709837EH
	dw	bTAG_VALID,017H-1


;sqrt(2) = 1.41421 35623 73095 04880 16887 24209 69808	;From Hart appendix
;Hex value:    0.B504F333F9DE6484597D89B30 H1
Sqrt2Hi		equ	0B504F333H
Sqrt2Lo		equ	0F9DE6484H
Sqrt2Exp	equ	01H-1

;sqrt(2) - 1 = +0.4142135623730950488016887242E0
;Hex value:    0.D413CCCFE779921165F626CC4 HFFFF
Sqrt2m1Hi	equ	0D413CCCFH
Sqrt2m1Lo	equ	0E7799211H
XSqrt2m1Lo	equ	060000000H
Sqrt2m1Exp	equ	0FFFFH-1

;2 - sqrt(2) = +0.5857864376269049511983112758E0
;Hex value:    0.95F619980C4336F74D04EC9A0 H0
TwoMinusSqrt2Hi	equ	095F61998H
TwoMinusSqrt2Lo	equ	00C4336F7H
TwoMinusSqrt2Exp equ	00H-1

;*********************

tLogPoly	label	dword

;These constants are derived from Hart #2355: log2(x) = z * P(z^2) / Q(z^2),
; z = (x+1) / (x-1) accurate to 19.74 digits over interval 
;[1/sqrt(2), sqrt(2)].  The original Hart coefficients were for log10(); 
;the P() coefficients have been scaled by log2(10) to compute log2().
;
;log2(10) = 3.32192 80948 87362 34787 03194 29489 39017	;From Hart appendix

	dd	3			;P() is degree three

;  Original Hart constant	 	Scaled value
;
;+.18287 59212 09199 9337	 E0	+0.607500660543248917834110566373E0
;Hex value:    0.9B8529CD54E72022A12BAEC53 H0
	dq	09B8529CD54E72023H
	dw	bTAG_VALID,00H-1

;-.41855 96001 31266 20633	 E1	-13.9042489506087332809657007634
;Hex value:    0.DE77CDBF64E8C53F0DCD458D0 H4
	dq	0DE77CDBF64E8C53FH
	dw	bSign shl 8 + bTAG_VALID,04H-1

;+.13444 58152 27503 62236	 E2	+44.6619330844279438866067340334
;Hex value:    0.B2A5D1C95708A0C9FE50F6F97 H6
	dq	0B2A5D1C95708A0CAH
	dw	bTAG_VALID,06H-1

;-.10429 11213 72526 69497 44122 E2	-34.6447606134704282123622236943
;Hex value:    0.8A943C20526AE439A98B30F6A H6
	dq	08A943C20526AE43AH
	dw	bSign shl 8 + bTAG_VALID,06H-1


	dd	3			;Q() is degree three.  First 
					;coefficient is 1.0 and is not listed.
;  Hart constant
;
;-.89111 09060 90270 85654	 E1
;Hex value:    0.8E93E7183AA998D74F45CDFF0 H4
	dq	08E93E7183AA998D7H
	dw	bSign shl 8 + bTAG_VALID,04H-1

;+.19480 96618 79809 36524 155	 E2
;Hex value:    0.9BD904CCFEE118D4BEF319716 H5
	dq	09BD904CCFEE118D5H
	dw	bTAG_VALID,05H-1

;-.12006 95907 02006 34243 4218	 E2
;Hex value:    0.C01C811D2EC1B5806304B1858 H4
	dq	0C01C811D2EC1B580H
	dw	bSign shl 8 + bTAG_VALID,04H-1

;Log2(e) = 1.44269 50408 88963 40735 99246 81001 89213	;From Hart appendix
;Hex value:    0.B8AA3B295C17F0BBBE87FED04 H1
Log2OfEHi	equ	0B8AA3B29H
Log2OfELo	equ	05C17F0BCH
Log2OfEexp	equ	01H-1


;********************* Generic polynomial evaluation *********************
;
;EvalPoly, EvalPolyAdd, EvalPolySetup, Eval2Poly
;
;Inputs:
;	ebx:esi,ecx = floating point number, internal format
;	edi = pointer to polynomial degree and coefficients
;Outputs:
;	result in ebx:esi,ecx
;	edi incremented to start of last coefficient in list
;
;EvalPoly is the basic polynomial evaluator, using Horner's rule.  The
;polynomial pointer in edi points to a list: the first dword in the list
;is the degree of the polynomial (n); it is followed by the n+1 
;coefficients in internal (12-byte) format.  The argment for EvalPoly
;must be stored in the static FloatTemp in addition to being in
;registers.
;
;EvalPolyAdd is an alternate entry point into the middle of EvalPoly.
;It is used when the first coefficient is 1.0, so it skips the first
;multiplication.  It requires that the degree of the polynomial be
;already loaded into ebp.
;
;EvalPolySetup store a copy of the argument in the static ArgTemp,
;and stores the square of the argument in the static FloatTemp.  
;Then it falls into EvalPoly to evaluate the polynomial on the square.
;
;Eval2Poly evaluate two polynomials on its argument.  The first 
;polynomial is  x * P(x^2), and its result is left at [[CURstk]].
;The second polynomial is Q(x^2), and its result is left in registers.
;The most significant coefficient of Q() is 1.
;
;Polynomial evaluation uses a slight variation on the standard add
;and multiply routines.  PolyAddDouble and PolyMulDouble both check
;to see if the argument in registers (the current accumulation) is 
;zero.  The argument pointed to by edi is a coefficient and is never
;zero.
;
;In addition, the [RoundMode] and [ZeroVector] vectors are "trapped",
;i.e., redirected to special handlers for polynomial evaluation.
;[RoundMode] ordinarily points to the routine that handles the
;the current rounding mode and precision control; however, during
;polynomial evaluation, we always want full precision and round
;nearest.  The normal rounding routines also store their result
;at [[Result]], but we want the result left in registers.
;[ZeroVector] exists solely so polynomial evaluation can trap
;when AddDouble results of zero.  The normal response is to store
;a zero at [[Result]], but we need the zero left in registers.
;PolyRound and PolyZero handle these traps.


EvalPolySetup:
;Save x in ArgTemp
	mov	EMSEG:[ArgTemp].ExpSgn,ecx
	mov	EMSEG:[ArgTemp].lManHi,ebx
	mov	EMSEG:[ArgTemp].lManLo,esi
	mov	EMSEG:[RoundMode],offset PolyRound
	mov	EMSEG:[ZeroVector],offset PolyZero
	push	edi			;Save pointer to  polynomials
;op1 mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7
	mov	edx,ebx
	mov	edi,esi
	mov	eax,ecx
;op2 mantissa in edx:edi, exponent in high eax, sign in ah bit 7
	call	MulDoubleReg		;Compute x^2
;Save x^2 in FloatTemp
	mov	EMSEG:[FloatTemp].ExpSgn,ecx
	mov	EMSEG:[FloatTemp].lManHi,ebx
	mov	EMSEG:[FloatTemp].lManLo,esi
	pop	edi
EvalPoly:
;ebx:esi,ecx = arg to evaluate, also in FloatTemp
;edi = pointer to degree and list of coefficients.
	push	edi
	mov	eax,cs:[edi+4].ExpSgn
	mov	edx,cs:[edi+4].lManHi
	mov	edi,cs:[edi+4].lManLo
	call	MulDoubleReg		;Multiply arg by first coef.
	pop	edi
	mov	ebp,cs:[edi]		;Get polynomial degree
	add	edi,4+Reg87Len		;Point to second coefficient
	jmp	EvalPolyAdd

PolyLoop:
	push	ebp			;Save loop count
ifdef NT386
        mov	edi,YFloatTemp
else
	mov	edi,offset edata:FloatTemp
endif
        call	PolyMulDouble
	pop	ebp
	pop	edi
	add	di,Reg87Len
EvalPolyAdd:
	push	edi
	mov	eax,cs:[edi].ExpSgn
	mov	edx,cs:[edi].lManHi
	mov	edi,cs:[edi].lManLo
	cmp	cl,bTAG_ZERO		;Adding to zero?
	jz	AddToZero
	call	AddDoubleReg		;ebp preserved
ContPolyLoop:
	dec	ebp
	jnz	PolyLoop
	pop	edi
	ret

AddToZero:
;Number in registers is zero, so just return value from memory.
	mov	ecx,eax
	mov	ebx,edx
	mov	esi,edi
	jmp	ContPolyLoop


Eval2Poly:
	call	EvalPolySetup
	push	edi
ifdef NT386
        mov	edi,YArgTemp
else
	mov	edi,offset edata:ArgTemp
endif
	call	PolyMulDouble		;Multiply first result by argument
	pop	edi
;Save result of first polynomial at [[CURstk]]
	mov	edx,EMSEG:[CURstk]
	mov	EMSEG:[edx].ExpSgn,ecx
	mov	EMSEG:[edx].lManHi,ebx
	mov	EMSEG:[edx].lManLo,esi
;Load x^2 back into registers
	mov	ecx,EMSEG:[FloatTemp].ExpSgn
	mov	ebx,EMSEG:[FloatTemp].lManHi
	mov	esi,EMSEG:[FloatTemp].lManLo
;Start second polynomial evaluation
	add	edi,4+Reg87Len		;Point to coefficient
	mov	ebp,cs:[edi-4]		;Get polynomial degree
	jmp	EvalPolyAdd


PolyRound:
;This routine handles all rounding during polynomial evaluation.
;It performs 64-but round nearest, with result left in registers.
;
;Inputs:
;	mantissa in ebx:esi:eax, exponent in high ecx, sign in ch bit 7
;Outputs:
;	same, plus tag in cl.
;
;To perform "round even" when the round bit is set and the sticky bits
;are zero, we treat the LSB as if it were a sticky bit.  Thus if the LSB
;is set, that will always force a round up (to even) if the round bit is
;set.  If the LSB is zero, then the sticky bits remain zero and we always
;round down.  This rounding rule is implemented by adding RoundBit-1
;(7F..FFH), setting CY if round up.  
;
;This routine needs to be reversible in case we're at the last step
;in the polynomial and final rounding uses a different rounding mode.
;We do this by copying the LSB of esi into al.  While the rounding is 
;reversible, you can't tell if the answer was exact.

	mov	edx,esi
	and	dl,1			;Look at LSB
	or	al,dl			;Set LSB as sticky bit
	add	eax,(1 shl 31)-1	;Sum LSB & sticky bits--CY if round up
	adc	esi,0
	adc	ebx,0
	jc	PolyBumpExponent	;Overflowed, increment exponent
	or      esi,esi			;Any bits in low half?
.erre   bTAG_VALID eq 1
.erre   bTAG_SNGL eq 0
	setnz   cl			;if low half==0 then cl=0 else cl=1
	ret

PolyBumpExponent:
	add	ecx,1 shl 16		;Mantissa overflowed, bump exponent
	or	ebx,1 shl 31		;Set MSB
	mov     cl,bTAG_SNGL
PolyZero:
;Enter here when result is zero
	ret

;*******************************************************************************

;FPATAN instruction

;Actual instruction entry point is in emarith.asm

tFpatanDisp	label	dword		;Source (ST(0))	Dest (*[di] = ST(1))
	dd	AtanDouble		;single		single
	dd	AtanDouble		;single		double
	dd	AtanZeroDest		;single		zero
	dd	AtanSpclDest		;single		special
	dd	AtanDouble		;double		single
	dd	AtanDouble		;double		double
	dd	AtanZeroDest		;double		zero
	dd	AtanSpclDest		;double		special
	dd	AtanZeroSource		;zero		single
	dd	AtanZeroSource		;zero		double
	dd	AtanZeroDest		;zero		zero
	dd	AtanSpclDest		;zero		special
	dd	AtanSpclSource		;special	single
	dd	AtanSpclSource		;special	double
	dd	AtanSpclSource		;special	zero
	dd	TwoOpBothSpcl		;special	special
	dd	AtanTwoInf		;Two infinites

;Compute atan( st(1)/st(0) ).  Neither st(0) or st(1) are zero or
;infinity at this point.
;
;Argument reduction starts by dividing the smaller by the larger,
;ensuring that the result x is <= 1.  The absolute value of the quotient
;is used and the quadrant is fixed up later.  If x = st(0)/st(1), then 
;the final atan result is subtracted from pi/2 (and normalized for the
;correct range of -pi to +pi).  
;
;The range of x is further reduced using the formulas:
;	t = (x - k) / (1 + kx)
;	atan(x) = atan(k) + atan(t)
;
;Given that x <= 1, if we choose k = tan(pi/6) = 1/sqrt(3), then we
;are assured that t <= tan(pi/12) = 2 - sqrt(3), and
;for x >= tan(pi/12) = 2 - sqrt(3), t >= -tan(pi/12).
;Thus we can always reduce the argument to abs(t) <= tan(pi/12).
;
;Since k = 1/sqrt(3), it is convenient to multiply the numerator
;and denominator of t by 1/k, which gives
;t = (x/k - 1) / (1/k + x) = ( x*sqrt(3) - 1 ) / ( sqrt(3) + x ).
;This is the form found in Cody and Waite and in previous versions
;of the emulator.  It requires one each add, subtract, multiply, and
;divide.
;
;Hart has derived a simpler version of this formula:
;t = 1/k - (1/k^2 + 1) / (1/k + x) = sqrt(3) - 4 / ( sqrt(3) + x ).
;Note that this computation requires one each add, subtract, and
;divide, but no multiply.

;st(0) mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7
;[edi] points to st(1), where result is returned

AtanDouble:
	mov	EMSEG:[Result],edi
	mov	EMSEG:[RoundMode],offset PolyRound
	mov	EMSEG:[ZeroVector],offset PolyZero
	mov	ah,EMSEG:[edi].bSgn	;Sign of result
	mov	al,ch			;Affects quadrant of result
	and	al,bSign		;Zero other bits, used as flags
	push	eax			;Save flag
;First figure out which is larger
	push	offset AtanQuo		;Return address for DivDouble
	shld	edx,ecx,16		;Get exponent to ax
	cmp	dx,EMSEG:[edi].wExp	;Compare exponents
	jl	DivrDoubleSetFlag	;ST(0) is smaller, make it dividend
	jg	DivDouble		;   ...is bigger, make it divisor
;Exponents are equal, compare mantissas
	cmp	ebx,EMSEG:[edi].lManHi
	jb	DivrDoubleSetFlag	;ST(0) is smaller, make it dividend
	ja	DivDouble		;   ...is bigger, make it divisor
	cmp	esi,EMSEG:[edi].lManLo
	jbe	DivrDoubleSetFlag	;ST(0) is smaller, make it dividend
	jmp	DivDouble

TinyAtan:
;Come here if the angle was reduced to zero, or the divide resulted in
;unmasked underflow so that the quotient exponent was biased.
;Note that an angle of zero means reduction was performed, and the
;result will be corrected to a non-zero value.
	mov	dl,[esp]		;Get flag byte
	or	dl,dl			;No correction needed?
	jz	AtanSetSign		;Just return result of divide
	and	EMSEG:[CURerr],not Underflow
;Angle in registers is too small to affect correction amount.  Just
;load up correction angle instead of adding it in.
	add	dl,40H			;Change flags for correction lookup
	shr	dl,5-2			;Now in bits 2,3,4
	and	edx,7 shl 2
	mov	ebx,[edx+2*edx+tAtanPiFrac].lManHi
	mov	esi,[edx+2*edx+tAtanPiFrac].lManLo
	mov	ecx,[edx+2*edx+tAtanPiFrac].ExpSgn
	shrd	eax,ecx,8		;Copy rounding flag to high eax
	jmp	AtanSetSign

AtanQuo:
;Return here after divide.  Underflow flag is set only for "big underflow",
;meaning the (15-bit) exponent couldn't even be kept in 16 bits.  This can
;only happen dividing a denormal by one of the largest numbers.
;
;Rounded mantissa in ebx:esi:eax, exp/sign in high ecx
	test	EMSEG:[CURerr],Underflow;Did we underflow?
	jnz	TinyAtan
;Now compare quotient in ebx:esi,ecx with tan(pi/12) = 2 - sqrt(3)
	xor	cx,cx			;Use absolute value
	cmp	ecx,Tan15exp shl 16
	jg	AtnNeedReduce
	jl	AtnReduced
	cmp	ebx,Tan15Hi
	ja	AtnNeedReduce
	jb	AtnReduced
	cmp	esi,Tan15Lo
	jbe	AtnReduced
AtnNeedReduce:
	or	byte ptr [esp],20H	;Note reduction in flags on stack
;Compute t = sqrt(3) - 4 / ( sqrt(3) + x ).
	mov	eax,Sqrt3exp shl 16
	mov	edx,Sqrt3Hi
	mov	edi,Sqrt3Lo
	call	AddDoubleReg		;x + sqrt(3)
	mov	edi,esi
	mov	esi,ebx			;Mantissa in esi:edi
	mov	ebx,ecx			;ExpSgn to ebx
	mov	ecx,(2+TexpBias) shl 16
	mov	edx,1 shl 31
	xor	eax,eax			;edx:edi,eax = 4.0
;dividend mantissa in edx:eax, exponent in high ecx, sign in ch bit 7
;divisor mantissa in esi:edi, exponent in high ebx, sign in bh bit 7
	call	DivDoubleReg		;4 / ( x + sqrt(3) )
	not	ch			;Flip sign
	mov	eax,Sqrt3exp shl 16
	mov	edx,Sqrt3Hi
	mov	edi,Sqrt3Lo
	call	AddDoubleReg		;sqrt(3) - 4 / ( x + sqrt(3) )
;Result in ebx:esi,ecx could be very small (or zero) if arg was near tan(pi/6).
	cmp	cl,bTAG_ZERO
	jz	TinyAtan
AtnReduced:
;If angle is small, skip the polynomial. atan(x) = x when x - x^3/3 = x
;[or 1 - x^2/3 = 1], which happens when x < 2^-32.  This prevents underflow
;in computing x^2.
TinyAtanArg	equ	-32
	cmp	ecx,TinyAtanArg shl 16
	jl	AtanCorrection
	mov	edi,offset tAtanPoly
	call	Eval2Poly
	mov	edi,EMSEG:[CURstk]	;Point to first result
	call	DivDouble		;x * P(x^2) / Q(x^2)
AtanCorrection:
;Rounded mantissa in ebx:esi:eax, exp/sign in high ecx
;
;Correct sign and add fraction of pi to account for various angle reductions:
;
;    flag bit	   indicates		correction
;----------------------------------------------------
;	5	arg > tan(pi/12)	add pi/6
;	6	st(1) > st(0)		sub from pi/2
;	7	st(0) < 0		sub from pi
;
;This results in the following correction for the result R:
;
;bit  7 6 5	correction
;---------------------------
;     0 0 0	none
;     0 0 1	pi/6 + R
;     0 1 0	pi/2 - R
;     0 1 1	pi/3 - R
;     1 0 0	pi - R
;     1 0 1	5*pi/6 - R
;     1 1 0	pi/2 + R
;     1 1 1	2*pi/3 + R

	mov	dl,[esp]		;Get flag byte
	or	dl,dl			;No correction needed?
	jz	AtanSetSign
	add	dl,40H			;Set bit 7 for all -R cases

;This changes the meaning of the flag bits to the following:
;
;bit  7 6 5	correction
;---------------------------
;     0 0 0	pi/2 + R
;     0 0 1	2*pi/3 + R
;     0 1 0	none
;     0 1 1	pi/6 + R
;     1 0 0	pi/2 - R
;     1 0 1	pi/3 - R
;     1 1 0	pi - R
;     1 1 1	5*pi/6 - R

	xor	ch,dl			;Flip sign bit in cases 4 - 7
	shr	dl,5-2			;Now in bits 2,3,4
	and	edx,7 shl 2
	mov	eax,[edx+2*edx+tAtanPiFrac].ExpSgn
	mov	edi,[edx+2*edx+tAtanPiFrac].lManLo
	mov	edx,[edx+2*edx+tAtanPiFrac].lManHi
	call	AddDoubleReg		;Add in correction angle
AtanSetSign:
	pop	edx			;Get flags again
	mov	ch,dh			;Set sign to original ST(1)
;Rounded mantissa in ebx:esi:eax, exp/sign in ecx
	jmp     TransUnround


;***
AtanSpclDest:
	mov	al,EMSEG:[edi].bTag	;Pick up tag
;	cmp     cl,bTAG_INF		;Is argument infinity?
	cmp     al,bTAG_INF		;Is argument infinity?
	jnz	SpclDest		;In emarith.asm
AtanZeroSource:
;Dividend is infinity or divisor is zero.  Return pi/2 with 
;same sign as dividend.
	mov	ecx,(PiExp-1) shl 16 + bTAG_VALID	;Exponent for pi/2
PiMant:
;For storing multiples of pi.  Exponent/tag is in ecx.
	mov	ch,EMSEG:[edi].bSgn	;Get dividend's sign
	mov	ebx,XPiHi
	mov	esi,XPiMid
	mov	eax,XPiLo
;A jump through [TransRound] is only valid if the number is known not to
;underflow.  Unmasked underflow requires [RoundMode] be set.
	jmp	EMSEG:[TransRound]

;***
AtanSpclSource:
	cmp	cl,bTAG_INF		;Scaling by infinity?
	jnz	SpclSource		;in emarith.asm
AtanZeroDest:
;Divisor is infinity or dividend is zero.  Return zero for +divisor, 
;pi for -divisor.  Result sign is same is dividend.
	or	ch,ch			;Check divisor's sign
	mov	ecx,PiExp shl 16 + bTAG_VALID	;Exponent for pi
	js	PiMant			;Store pi
;Result is zero
	mov	EMSEG:[edi].lManHi,0
	mov	EMSEG:[edi].lManLo,0
	mov	EMSEG:[edi].wExp,0
	mov	EMSEG:[edi].bTAG,bTAG_ZERO
	ret

;***
AtanTwoInf:
;Return pi/4 for +infinity divisor, 3*pi/4 for -infinity divisor.
;Result sign is same is dividend infinity.
	or	ch,ch			;Check divisor's sign
	mov	ecx,(PiExp-2) shl 16 + bTAG_VALID	;Exponent for pi/4
	jns	PiMant			;Store pi/4
	mov	ecx,(ThreePiExp-2) shl 16 + bTAG_VALID	;Exponent for 3*pi/4
	mov	ch,EMSEG:[edi].bSgn	;Get dividend's sign
	mov	ebx,XThreePiHi
	mov	esi,XThreePiMid
	mov	eax,XThreePiLo
;A jump through [TransRound] is only valid if the number is known not to
;underflow.  Unmasked underflow requires [RoundMode] be set.
	jmp	EMSEG:[TransRound]

;*******************************************************************************

ExpSpcl:
;Tagged special
	cmp	cl,bTAG_DEN
	jz	ExpDenorm
	cmp	cl,bTAG_INF
        mov     al, cl
	jnz	SpclDestNotDen		;Check for Empty or NAN
;Have infinity, check its sign.  
;Return -1 for -infinity, no change if +infinity
	or	ch,ch			;Check sign
	jns	ExpRet			;Just return the +inifinity
	mov	EMSEG:[edi].lManLo,0
	mov	EMSEG:[edi].lManHi,1 shl 31
	mov	EMSEG:[edi].ExpSgn,bSign shl 8 + bTAG_SNGL	;-1.0 (exponent is zero)
	ret

ExpDenorm:
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal	;Is denormal exception masked?
	jnz	ExpCont			;Yes, continue
ExpRet:
	ret

EM_ENTRY eF2XM1
eF2XM1:
;edi = [CURstk]
	mov	ecx,EMSEG:[edi].ExpSgn
	cmp	cl,bTAG_ZERO
	jz	ExpRet			;Return same zero
	ja	ExpSpcl
ExpCont:

;The input range specified for the function is (-1, +1).  The polynomial 
;used for this function is valid only over the range [0, +0.5], so range
;reduction is needed.  Range reduction is based on the identity:
;
;  2^(a+b) = 2^a * 2^b
;
;1.0 or 0.5 can be added/subtracted from the argument to bring it into
;range.  We calculate 2^x - 1 with a polynomial, and then adjust the
;result according to the amount added or subtracted, as shown in the table:
;
;Arg range	Adj	Polynomial result	Required result, 2^x - 1
;
; (-1, -0.5]	+1	P = 2^(x+1) - 1		(P - 1)/2
;
; (-0.5, 0)	+0.5	P = 2^(x+0.5) - 1	P * sqrt(2)/2 + (sqrt(2)/2 - 1)
;
; (0, 0.5)	0	P = 2^x - 1		P
;
; [0.5, 1)	-0.5	P = 2^(x-0.5) - 1	P * sqrt(2) + (sqrt(2)-1)
;
;Since the valid input range does not include +1.0 or -1.0, and zero is
;handled separately, the precision exception will always be set.

	mov	EMSEG:[Result],edi
	mov	EMSEG:[RoundMode],offset PolyRound
	mov	EMSEG:[ZeroVector],offset PolyZero
	push	offset TransUnround		;Always exit through here
	mov	ebx,EMSEG:[edi].lManHi
	mov	esi,EMSEG:[edi].lManLo
;Check for small argument, so that x^2 does not underflow.  Note that 
;e^x = 1+x for small x, where small x means  x + x^2/2 = x  [or 1 + x/2 = 1], 
;which happens when x < 2^-64, so 2^x - 1 = x * ln(2) for small x.
TinyExpArg	equ	-64
	cmp	ecx,TinyExpArg shl 16
	jl	TinyExp
	cmp	ecx,-1 shl 16 + bSign shl 8	;See if positive, < 0.5
	jl	ExpReduced
;Argument was not in range (0, 0.5), so we need some kind of reduction
	or	ecx,ecx			;Exp >= 0 means arg >= 1.0 --> too big
;CONSIDER: this returns through TransUnround which restores the rounding
;vectors, but it also randomly rounds the result becase eax is not set.
	jge	ExpRet			;Give up if arg out of range
;We're going to need to add/subtract 1.0 or 0.5, so load up the constant
	mov	edx,1 shl 31
	xor	edi,edi
	mov	eax,-1 shl 16 + bSign shl 8	;edx:edi,eax = -0.5
	mov	ebp,offset ExpReducedMinusHalf
	or	ch,ch			;If it's positive, must be [0.5, 1)
	jns	ExpReduction
	xor	ah,ah			;edx:edi,eax = +0.5
	mov	ebp,offset ExpReducedPlusHalf
	cmp	ecx,eax			;See if abs(arg) >= 0.5
	jl	ExpReduction		;No, adjust by .5
	xor	eax,eax			;edx:edi,eax = 1.0
	mov	ebp,offset ExpReducedPlusOne
ExpReduction:
	call	AddDoubleReg		;Argument now in range [0, 0.5]
	cmp	cl,bTAG_ZERO		;Did reduction result in zero?
	jz	ExpHalf			;If so, must have been exactly 0.5
	push	ebp			;Address of reduction cleanup
ExpReduced:
	mov	edi,offset tExpPoly
	call	Eval2Poly
;2^x - 1 is approximated with 2 * x*P(x^2) / ( Q(x^2) - x*P(x^2) )
;Q(x^2) is in registers, P(x^2) is at [[CURstk]]
	mov	edi,EMSEG:[CURstk]
	mov	dx,bSign shl 8		;Subtract memory operand
;Note that Q() and P() have no roots over the input range
;(they will never be zero).
	call	AddDouble		;Q(x^2) - x*P(x^2)
	sub	ecx,1 shl 16		;Divide by two
	mov	edi,EMSEG:[CURstk]
	jmp	DivDouble		;2 * x*P(x^2) / ( Q(x^2) - x*P(x^2) )
;Returns to correct argument reduction correction routine or TransUnround

TinyExp:
;Exponent is very small (and was not reduced)
	mov	edx,cFLDLN2hi
	mov	edi,cFLDLN2lo
	mov	eax,cFLDLN2exp shl 16
;This could underflow (but not big time)
	jmp	MulDoubleReg		;Returns to TransUnround

ExpHalf:
;Argument of exactly 0.5 was reduced to zero.  Just return result.
	mov	ebx,Sqrt2m1Hi
	mov	esi,Sqrt2m1Lo
	mov	eax,XSqrt2m1Lo + 1 shl 31 - 1
	mov	ecx,Sqrt2m1Exp shl 16
	ret				;Exit through TransUnround

ExpReducedPlusOne:
;Correct result is (P - 1)/2
	sub	ecx,1 shl 16		;Divide by two
	mov	edx,1 shl 31
	xor	edi,edi
	mov	eax,-1 shl 16 + bSign shl 8	;edx:edi,eax = -0.5
	jmp	AddDoubleReg

ExpReducedPlusHalf:
;Correct result is P * sqrt(2)/2 - (1 - sqrt(2)/2)
	mov	edx,Sqrt2Hi
	mov	edi,Sqrt2Lo
	mov	eax,Sqrt2exp-1 shl 16	;sqrt(2)/2
	call	MulDoubleReg
	mov	edx,TwoMinusSqrt2Hi
	mov	edi,TwoMinusSqrt2Lo
	mov	eax,(TwoMinusSqrt2Exp-1) shl 16 + bSign shl 8	;(2-sqrt(2))/2
	jmp	AddDoubleReg

ExpReducedMinusHalf:
;Correct result is P * sqrt(2) + (sqrt(2)-1)
	mov	edx,Sqrt2Hi
	mov	edi,Sqrt2Lo
	mov	eax,Sqrt2exp shl 16
	call	MulDoubleReg
	mov	edx,Sqrt2m1Hi
	mov	edi,Sqrt2m1Lo
	mov	eax,Sqrt2m1Exp shl 16
	jmp	AddDoubleReg

;*******************************************************************************

;Dispatch table for log(x+1)
;
;One operand has been loaded into ecx:ebx:esi ("source"), the other is
;pointed to by edi ("dest").  
;
;Tag of source is shifted.  Tag values are as follows:

.erre	TAG_SNGL	eq	0	;SINGLE: low 32 bits are zero
.erre	TAG_VALID	eq	1
.erre	TAG_ZERO	eq	2
.erre	TAG_SPCL	eq	3	;NAN, Infinity, Denormal, Empty

;Any special case routines not found in this file are in emarith.asm

tFyl2xp1Disp	label	dword		;Source (ST(0))	Dest (*[di] = ST(1))
	dd	LogP1Double		;single		single
	dd	LogP1Double		;single		double
	dd	LogP1ZeroDest		;single		zero
	dd	LogP1SpclDest		;single		special
	dd	LogP1Double		;double		single
	dd	LogP1Double		;double		double
	dd	LogP1ZeroDest		;double		zero
	dd	LogP1SpclDest		;double		special
	dd	XorSourceSign		;zero		single
	dd	XorSourceSign		;zero		double
	dd	XorDestSign		;zero		zero
	dd	LogP1SpclDest		;zero		special
	dd	LogSpclSource		;special	single
	dd	LogSpclSource		;special	double
	dd	LogSpclSource		;special	zero
	dd	TwoOpBothSpcl		;special	special
	dd	LogTwoInf		;Two infinites


LogP1Double:
;st(0) mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7
;[edi] points to st(1), where result is returned
;
;This instruction is defined only for x+1 in the range [1/sqrt(2), sqrt(2)]
;The approximation used (valid over exactly this range) is
; log2(x) = z * P(z^2) / Q(z^2), z = (x-1) / (x+1), which is
; log2(x+1) = r * P(r^2) / Q(r^2), r = x / (x+2)
;
;We're not too picky about this range check because the function is simply
;"undefined" if out of range--EXCEPT, we're supposed to check for -1 and
;signal Invalid if less, -infinity if equal.
	or	ecx,ecx			;abs(x) >= 1.0?
	jge	LogP1OutOfRange		;Valid range is approx [-0.3, +0.4]
	mov	EMSEG:[Result],edi
	mov	EMSEG:[RoundMode],offset PolyRound
	mov	EMSEG:[ZeroVector],offset PolyZero
	mov	eax,1 shl 16		;Exponent of 1 for adding 2.0
	push	offset TotalLog		;Return address for BasicLog
;	jmp	BasicLog		;Fall into BasicLog
;.erre	BasicLog eq $

;BasicLog is used by eFYL2X and eFYL2XP1.
;eax has exponent and sign to add 1.0 or 2.0 to argument
;ebx:esi,ecx has argument, non-zero, tag not set
;ST has argument to take log2 of, minus 1.  (This is the actual argument
;of eFYL2XP1, or argument minus 1 of eFYL2X.)

BasicLog:
	mov	edx,1 shl 31
	xor	edi,edi			;edx:edi,eax = +1.0 or +2.0
	call	AddDoubleReg
	mov	edi,EMSEG:[CURstk]	;Point to x-1
	call	DivDouble		;Compute (x-1) / (x+1)
;Result in registers is z = (x-1)/(x+1).  For tiny z, ln(x) = 2*z, so
; log2(x) = 2 * log2(e) * z.  Tiny z is such that z + z^3/3 = z.
	cmp	ecx,-32 shl 16		;Smallest exponent to bother with
	jl	LogSkipPoly
	mov	edi,offset tLogPoly
	call	Eval2Poly
	mov	edi,EMSEG:[CURstk]	;Point to first result, r * P(r^2)
	jmp	DivDouble		;Compute r * P(r^2) / Q(r^2)

LogSkipPoly:
;Multiply r by 2 * log2(e)
	mov	edx,Log2OfEHi
	mov	edi,Log2OfELo
	mov	eax,(Log2OfEexp+1) shl 16
	jmp	MulDoubleReg

LogP1OutOfRange:
;Input range isn't valid, so we can return anything we want--EXCEPT, for
;numbers < -1 we must signal Invalid Operation, and Divide By Zero for
;-1.  Otherwise, we return an effective log of one by just leaving the
;second operand as the return value.
;
;Exponent in ecx >= 0  ( abs(x) >= 1 )
	or	ch,ch			;Is it positive?
	jns	LogP1Ret		;If so, skip it
	and	ecx,0FFFFH shl 16	;Look at exponent only: 0 for -1.0
	sub	ebx,1 shl 31		;Kill MSB
	or	ebx,esi
	or	ebx,ecx
	jnz	ReturnIndefinite	;Must be < -1.0
	jmp	DivideByMinusZero

LogP1Ret:
	ret
	
;***
LogP1ZeroDest:
	or	ch,ch			;Is it negative?
	jns	LogP1Ret		;If not, just leave it zero
	or	ecx,ecx			;abs(x) >= 1.0?
	jl	XorDestSign		;Flip sign of zero
;Argument is <= -1
	jmp	ReturnIndefinite	;Have 0 * log( <=0 )

;***
LogP1SpclDest:
	mov	al,EMSEG:[edi].bTag		;Pick up tag
	cmp	al,bTAG_INF		;Is argument infinity?
	jnz	SpclDest		;In emarith.asm
;Multiplying log(x+1) * infinity.
;If x > 0, return original infinity.
;If -1 <= x < 0, return infinity with sign flipped.
;If x < -1 or x == 0, invalid operation.
	cmp	cl,bTAG_ZERO
	jz	ReturnIndefinite
	or	ch,ch			;Is it positive?
	jns	LogP1Ret
	test	ecx,0FFFFH shl 16	;Is exponent zero?
	jl	XorDestSign
	jg	ReturnIndefinite
	sub	ebx,1 shl 31		;Kill MSB
	or	ebx,esi
	jnz	ReturnIndefinite	;Must be < -1.0
	jmp	XorDestSign

;***
LogSpclSource:
	cmp	cl,bTAG_INF		;Is argument infinity?
	jnz	SpclSource		;in emarith.asm
	or	ch,ch			;Is it negative infinity?
	js	ReturnIndefinite
	jmp	MulByInf

;***
LogTwoInf:
	or	ch,ch			;Is it negative infinity?
	js	ReturnIndefinite
	jmp	XorDestSign

;*******************************************************************************

;Dispatch table for log(x)
;
;One operand has been loaded into ecx:ebx:esi ("source"), the other is
;pointed to by edi ("dest").  
;
;Tag of source is shifted.  Tag values are as follows:

.erre	TAG_SNGL	eq	0	;SINGLE: low 32 bits are zero
.erre	TAG_VALID	eq	1
.erre	TAG_ZERO	eq	2
.erre	TAG_SPCL	eq	3	;NAN, Infinity, Denormal, Empty

;Any special case routines not found in this file are in emarith.asm

tFyl2xDisp	label	dword		;Source (ST(0))	Dest (*[di] = ST(1))
	dd	LogDouble		;single		single
	dd	LogDouble		;single		double
	dd	LogZeroDest		;single		zero
	dd	LogSpclDest		;single		special
	dd	LogDouble		;double		single
	dd	LogDouble		;double		double
	dd	LogZeroDest		;double		zero
	dd	LogSpclDest		;double		special
	dd	DivideByMinusZero	;zero		single
	dd	DivideByMinusZero	;zero		double
	dd	ReturnIndefinite	;zero		zero
	dd	LogSpclDest		;zero		special
	dd	LogSpclSource		;special	single
	dd	LogSpclSource		;special	double
	dd	LogSpclSource		;special	zero
	dd	TwoOpBothSpcl		;special	special
	dd	LogTwoInf		;Two infinites


LogDouble:
;st(0) mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7
;[edi] points to st(1), where result is returned
;
;Must reduce the argument to the range [1/sqrt(2), sqrt(2)]
	or	ch,ch			;Is it positive?
	js	ReturnIndefinite	;Can't take log of negative number
	mov	EMSEG:[Result],edi
	mov	EMSEG:[RoundMode],offset PolyRound
	mov	EMSEG:[ZeroVector],offset PolyZero
	shld	eax,ecx,16		;Save exponent in ax as int part of log2
	xor	ecx,ecx			;Zero exponent: 1 <= x < 2
	cmp	ebx,Sqrt2Hi		;x > sqrt(2)?
	jb	LogReduced
	ja	LogReduceOne
	cmp	esi,Sqrt2Lo
	jb	LogReduced
LogReduceOne:
	sub	ecx,1 shl 16		;1/sqrt(2) < x < 1
	inc	eax
LogReduced:
	push	eax			;Save integer part of log2
	mov	ebp,ecx 		;Save reduced exponent (tag is wrong!)
	mov	edx,1 shl 31
	mov	eax,bSign shl 8		;Exponent of 0, negaitve
	xor	edi,edi			;edx:edi,eax = -1.0
	call	AddDoubleReg
	cmp	cl,bTAG_ZERO		;Was it exact power of two?
	jz	LogDone			;Skip log if power of two
;Save (x - 1), reload x with reduced exponent
	mov	edi,EMSEG:[CURstk]	;Point to original x again
	xchg	EMSEG:[edi].lManHi,ebx
	xchg	EMSEG:[edi].lManLo,esi
	mov	EMSEG:[edi].ExpSgn,ecx
	mov	ecx,ebp			;Get reduced exponent
	xor	eax,eax			;Exponent of 0, positive
	call	BasicLog
LogDone:
	pop	eax			;Get integer part back
	cwde
	or	eax,eax			;Is it zero?
	jz	TotalLog
;Next 3 instructions take abs() of integer
	cdq				;Extend sign through edx
	xor	eax,edx			;Complement...
	sub	eax,edx			;  and increment if negative
	bsr	dx,ax			;Look for MSB to normalize integer
;Bit number in dx ranges from 0 to 15
	mov	cl,dl
	not	cl			;Convert to shift count
	shl	eax,cl			;Normalize
.erre	TexpBias eq 0
	rol	edx,16			;Move exponent high, sign low
	or	ebx,ebx			;Was log zero?
	jz	ExactPower
	xchg	edx,eax			;Exp/sign to eax, mantissa to edx
	xor	edi,edi			;Extend with zero
	call	AddDoubleReg
TotalLog:
;Registers could be zero if input was exactly 1.0
	cmp	cl,bTAG_ZERO
	jz	ZeroLog
TotalLogNotZero:
	mov	edi,EMSEG:[Result]	;Point to second arg
	push	offset TransUnround
	jmp	MulDouble

ExactPower:
;Arg was a power of two, so log is exact (but not zero).
	mov     ebx,eax			;Mantissa to ebx
	mov     ecx,edx			;Exponent to ecx
	xor     esi,esi			;Extend with zero
;Exponent of arg [= log2(arg)] is now normalized in ebx:esi,ecx
;
;The result log is exact, so we don't want TransUnround, which is designed 
;to ensure the result is never exact.  Instead we set the [RoundMode]
;vector to [TransRound] before the final multiply.
	mov	eax,EMSEG:[TransRound]
	mov	EMSEG:[RoundMode],eax
	mov	edi,EMSEG:[Result]	;Point to second arg
	push	offset RestoreRound	;Return addr. for MulDouble in emtrig.asm
	jmp	MulDouble

ZeroLog:
	mov	eax,EMSEG:[SavedRoundMode]
	mov	EMSEG:[RoundMode],eax
	mov	EMSEG:[ZeroVector],offset SaveResult
	jmp	SaveResult

;***
LogZeroDest:
	or	ch,ch			;Is it negative?
	js	ReturnIndefinite	;Can't take log of negative numbers
;See if log is + or - so we can get correct sign of zero
	or	ecx,ecx			;Is exponent >= 0?
	jge	LogRet			;If so, keep present zero sign
FlipDestSign:
	not	EMSEG:[edi].bSgn
	ret

;***
LogSpclDest:
	mov	al,EMSEG:[edi].bTag		;Pick up tag
	cmp	al,bTAG_INF		;Is argument infinity?
	jnz	SpclDest		;In emarith.asm
;Multiplying log(x) * infinity.
;If x > 1, return original infinity.
;If 0 <= x < 1, return infinity with sign flipped.
;If x < 0 or x == 1, invalid operation.
	cmp	cl,bTAG_ZERO
	jz	FlipDestSign
	or	ch,ch			;Is it positive?
	js	ReturnIndefinite
	test	ecx,0FFFFH shl 16	;Is exponent zero?
	jg	LogRet			;x > 1, just return infinity
	jl	FlipDestSign
	sub	ebx,1 shl 31		;Kill MSB
	or	ebx,esi
	jz	ReturnIndefinite	;x == 1.0
LogRet:
	ret
