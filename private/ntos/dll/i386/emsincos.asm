;      SCCSID = @(#)emsincos.asm	       13.5 90/03/27
	page	,132
	subttl	emsincos - fsin, fcos and fsincos
;***
;emulator.asm -  80387 emulator
;
;	 IBM/Microsoft Confidential
;
;	 Copyright (c) IBM Corporation 1987, 1989
;	 Copyright (c) Microsoft Corporation 1987, 1989
;
;	 All Rights Reserved
;
;Purpose:
;	Code for fsin, fcos and fsincos
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************

lab eFsincosStackOver
	or	SEL[CURerr], StackFlag or Invalid
	test	SEL[CWmask], Invalid
	JSZ	eFsincosStackOverRet

	mov	SEL[rsi.lMan0], 0			; st(0) = Ind
	mov	SEL[rsi.lMan1], 0c0000000h
	mov	SEL[rsi.wExp], 7fffh - IexpBias
	mov	SEL[rsi.bTag], bTAG_NAN
	mov	SEL[rsi.bFlags], bSign

	mov	SEL[rdi.lMan0], 0			; st(-1) = Ind
	mov	SEL[rdi.lMan1], 0c0000000h
	mov	SEL[rdi.wExp], 7fffh - IexpBias
	mov	SEL[rdi.bTag], bTAG_NAN
	mov	SEL[rdi.bFlags], bSign

	mov	SEL[CURstk], rdi		; push stack
lab eFsincosStackOverRet
	ret


lab eFSINCOS
	mov	esi, SEL[CURStk]    ; esi = st(0)
	mov	edi, esi
	PrevStackElem	 edi	    ; edi = st(-1)

	cmp	SEL[edi.bTag], bTAG_EMPTY
	JSNE	eFsincosStackOver

	cmp	SEL[esi.bTag], bTAG_NAN
	JSNE	eFsincosNotSNaN

	test	SEL[esi.bMan7], 40h
	JSNZ	eFsincosNotSNaN

	test	SEL[CWmask], Invalid
	JSNZ	eFsincosNotSNaN

	or	SEL[CURerr], Invalid
	ret

lab eFsincosNotSNaN
ifdef NT386
        push    eax
        mov     eax, dword ptr SEL[rsi]
        mov     dword ptr SEL[rdi], eax
        mov     eax, dword ptr SEL[rsi+4]
        mov     dword ptr SEL[rdi+4], eax
        mov     eax, dword ptr SEL[rsi+8]
        mov     dword ptr SEL[rdi+8], eax
        add     rsi, Reg87Len
        add     rdi, Reg87Len
        pop     eax
else
        push	ds		    ; Copy current stack into st(-1)
	pop	es
	movsd
	movsd
	movsd
endif

	call	eFSIN
	PUSHST
	call	eFCOS

	ret


lab eFcosSpecial
	mov	esp, ebp
	pop	ebp

	mov	SEL[RESULT], esi

	mov	al, SEL[esi.bTag]
	cmp	al, bTAG_ZERO
	JSNE	eFcosInf

lab eFcosRetOne
	mov	SEL[esi.lMan0], 0
	mov	SEL[esi.lMan1], 080000000h
	mov	SEL[esi.wExp], 3fffh - IexpBias
	mov	SEL[esi.bFlags], 0
	mov	SEL[esi.bTag], bTAG_VALID
	ret

lab eFcosInf
	cmp	al, bTAG_INF
	JE	RetIndInv

lab eFcosNaN
	jmp	OneArgOpNaNRet


cProc  eFCOS,<PLM,PUBLIC>,<>

	localT	temp
	localB	SignFlag

cBegin
	mov	esi, SEL[CURstk]

	cmp	SEL[esi.bTag], bTAG_VALID
	jne	eFcosSpecial

	or	SEL[CURerr], Precision

	and	SEL[esi].bFlags, not bSign ; st(0) = fabs( st(0) );

	call	SinCosReduce		; Set ah to condition code.

	add	SEL[esi].wExp, IExpBias

	push	SEL[esi].wExp
	push	SEL[esi].lMan1
	push	SEL[esi].lMan0
	lea	ecx, [temp]
	push	ecx

	mov	bl, ah			; if octant 2, 3, 4, or 5 then final
	and	bl, bOCT2 or bOCT4	; result must be negative
	mov	[SignFlag], bl

	test	ah, bOCT1 or bOCT2	; if octant is 1, 2, 5, 6 then must
	jpo	CosCallSin		; do sin()

	call	__FASTLDCOS
	jmp	short CosCopyRes

CosCallSin:
	call	__FASTLDSIN

CosCopyRes:
	mov	eax, dword ptr [temp]
	mov	SEL[esi].lMan0, eax
	mov	eax, dword ptr [temp+4]
	mov	SEL[esi].lMan1, eax

	mov	ax,  word ptr [temp+8]
	sub	ax, IExpBias
	mov	SEL[esi].wExp, ax

	cmp	[SignFlag], 0
	jpe	CosDone

	or	SEL[esi].bFlags, bSign	; Make result negative.
CosDone:

cEnd





lab eFsinSpecial
	mov	esp, ebp
	pop	ebp

	mov	al, SEL[esi.bTag]
	cmp	al, bTAG_ZERO
	JSNE	eFsinInf

lab eFsinZero
	ret

lab eFsinInf
	cmp	al, bTAG_INF
	JE	RetIndInv

lab eFsinNaN
	jmp	OneArgOpNaNRet


cProc  eFSIN,<PLM,PUBLIC>,<>

	localT	temp
	localB	SignFlag

cBegin
	mov	esi, SEL[CURstk]

	cmp	SEL[esi.bTag], bTAG_VALID
	jne	eFsinSpecial

	or	SEL[CURerr], Precision

	mov	al, SEL[esi].bFlags
	and	SEL[esi].bFlags, not bSign

	shl	al, 1		    ; shift sign into carry.
	sbb	cl, cl		    ; set cl to -1 if argument is negative.

	push	ecx
	call	SinCosReduce	    ; Set ah to condition code.
	pop	ecx

	cmp	SEL[esi].bTag, bTAG_ZERO
	je	SinDone

	add	SEL[esi].wExp, IExpBias

	push	SEL[esi].wExp
	push	SEL[esi].lMan1
	push	SEL[esi].lMan0
	lea	ebx, [temp]
	push	ebx

	mov	bl, ah			; if octant 4, 5, 6 or 7 then final
	and	bl, bOCT4		; result must be negative

	neg	cl			; set cl to odd parity if arg was < 0.0
	xor	bl, cl			; set bl to odd parity if result must be negative

	mov	[SignFlag], bl

	test	ah, bOCT1 or bOCT2	; if octant is 1, 2, 5, 6 then must
	jpo	SinCallCos		; do cos()

	call	__FASTLDSIN
	jmp	short SinCopyResult

SinCallCos:
	call	__FASTLDCOS

SinCopyResult:
	mov	eax, dword ptr [temp]
	mov	SEL[esi].lMan0, eax
	mov	eax, dword ptr [temp+4]
	mov	SEL[esi].lMan1, eax

	mov	ax, word ptr [temp+8]
	sub	ax, IExpBias
	mov	SEL[esi].wExp, ax

	cmp	[SignFlag], 0
	jpe	SinDone

	or	SEL[esi].bFlags, bSign	; Make result negative.
SinDone:

cEnd



lab SinCosReduce
	mov	SEL[TEMP1.bFlags], 0		; TEMP1 = pi/4
	mov	SEL[TEMP1.bTag], bTAG_VALID
	mov	SEL[TEMP1.wExp], 3ffeh-IExpBias
	mov	SEL[TEMP1.wMan3], 0c90fh
	mov	SEL[TEMP1.wMan2], 0daa2h
	mov	SEL[TEMP1.wMan1],	2168h
	mov	SEL[TEMP1.wMan0], 0c235h

ifdef NT386
        mov     edi, TEMP1
else
	mov	edi, edataOFFSET TEMP1
endif

	push	esi
	call	InternFPREM		    ; rsi = st(0), rdi = st(0)
	pop	esi

	mov	ah, SEL[SWcc]

	test	ah, bOCT1		; check for even octant
	jz	EvenOct 		;   yes

	add	SEL[esi.wExp], IExpBias	; convert to true long double

	push	ds
	push	esi
	push	cs
	push	ecodeOFFSET PIBY4
	push	ds
	push	esi
	push	-1
	call	__FASTLDADD		; st(0) = pi/4 - st(0)
	mov	ah, SEL[SWcc]

	sub	SEL[esi.wExp], IExpBias	; convert to squirly emulator long double

EvenOct:
	retn



labelW	PIBY4
    dw	    0c235h, 02168h, 0daa2h, 0c90fh, 3ffeh

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; BUGBUG tedm: NT masm can't handle floating-point constants  ;
;              because strtod and _strtold C-runtimes aren't  ;
;              there.  So the constants below must be pre-    ;
;              assembled and defined as a byte stream.        ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ifdef NOTDEF

staticT  FourByPI, +0.1273239544735162686151e+01

staticT  SinP0, +0.7853981633974483096141845e+00
staticT  SinP1, -0.8074551218828078152025820e-01
staticT  SinP2, +0.2490394570192716275251900e-02
staticT  SinP3, -0.3657620418214640005290000e-04
staticT  SinP4, +0.3133616889173253480000000e-06
staticT  SinP5, -0.1757247417617080600000000e-08
staticT  SinP6, +0.6948152035052200000000000e-11
staticT  SinP7, -0.2022531292930000000000000e-13

staticT  CosP0, +0.99999999999999999996415e+00
staticT  CosP1, -0.30842513753404245242414e+00
staticT  CosP2, +0.15854344243815410897540e-01
staticT  CosP3, -0.32599188692668755044000e-03
staticT  CosP4, +0.35908604458858195300000e-05
staticT  CosP5, -0.24611363826370050000000e-07
staticT  CosP6, +0.11500497024263000000000e-09
staticT  CosP7, -0.38577620372000000000000e-12

else

staticB  FourByPI, <02Ah,015h,044h,04Eh,06Eh,083h,0F9h,0A2h,0FFh,03Fh>

staticB  SinP0   , <035h,0C2h,068h,021h,0A2h,0DAh,00Fh,0C9h,0FEh,03Fh>
staticB  SinP1   , <0DAh,095h,0F2h,02Dh,031h,0E7h,05Dh,0A5h,0FBh,0BFh>
staticB  SinP2   , <0E9h,0C6h,056h,0ADh,03Bh,0E3h,035h,0A3h,0F6h,03Fh>
staticB  SinP3   , <0D5h,0E7h,05Dh,015h,073h,066h,069h,099h,0F0h,0BFh>
staticB  SinP4   , <0BCh,032h,069h,0E1h,042h,01Ah,03Ch,0A8h,0E9h,03Fh>
staticB  SinP5   , <021h,077h,004h,05Fh,0A1h,0A5h,083h,0F1h,0E1h,0BFh>
staticB  SinP6   , <0FCh,01Ah,0D1h,006h,0CCh,063h,077h,0F4h,0D9h,03Fh>
staticB  SinP7   , <04Ah,003h,086h,040h,07Ch,065h,02Ch,0B6h,0D1h,0BFh>

staticB  CosP0   , <0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FEh,03Fh>
staticB  CosP1   , <02Fh,0F2h,02Eh,0F2h,04Dh,0E6h,0E9h,09Dh,0FDh,0BFh>
staticB  CosP2   , <02Fh,04Eh,0D5h,0DAh,040h,0F8h,0E0h,081h,0F9h,03Fh>
staticB  CosP3   , <09Dh,0DEh,06Ah,0E4h,0F1h,0E3h,0E9h,0AAh,0F3h,0BFh>
staticB  CosP4   , <031h,01Eh,0F9h,081h,041h,083h,0FAh,0F0h,0ECh,03Fh>
staticB  CosP5   , <076h,0B1h,000h,0A4h,01Eh,0F6h,068h,0D3h,0E5h,0BFh>
staticB  CosP6   , <0D8h,005h,06Fh,08Ah,0EAh,00Ah,0E6h,0FCh,0DDh,03Fh>
staticB  CosP7   , <003h,0D5h,00Ah,0ACh,0CCh,035h,02Ch,0D9h,0D5h,0BFh>

endif

cProc __FASTLDSIN,<PLM,PUBLIC>,<isi,idi>

	parmT	x
	parmI	RetOff

	localT	x2
	localT	poly
	localI	count

cBegin

	lea	isi, [x]		    ; x = x * (4/PI)
	push	ss
	push	isi

	push	ss
	push	isi

	mov	iax, codeOFFSET FourByPI
	push	cs
	push	iax

	call	__FASTLDMULT


	lea	idi, [x2]		    ; x2 = x * x
	push	ss
	push	idi

	push	ss
	push	isi

	push	ss
	push	isi

	call	__FASTLDMULT

if 0
	push	ss
	pop	es
	lea	idi, [poly]
	mov	isi, codeOFFSET SinP7
	movsw
	movsw
	movsw
	movsw
	movsw
endif
	mov	eax, dword ptr [SinP7]	    ; poly = SinP7
	mov	dword ptr [poly], eax
	mov	eax, dword ptr [SinP7+4]
	mov	dword ptr [poly+4], eax
	mov	ax, word ptr [SinP7+8]
	mov	word ptr [poly+8], ax

	lea	isi, [poly]
	mov	idi, codeOFFSET SinP6

	mov	[count], 7

SinPolyLoop:
	push	ss
	push	isi		    ; poly = poly * x2

	push	ss
	push	isi

	lea	iax, [x2]
	push	ss
	push	iax

	call	__FASTLDMULT


	push	ss
	push	isi		    ; poly = poly + SinP[n]

	push	ss
	push	isi

	push	cs
	push	idi

	xor	iax, iax
	push	iax
	call	__FASTLDADD

	sub	idi, 10

	dec	[count]
	jnz	SinPolyLoop

	push	ss
	push	[RetOff]		; return x * poly

	lea	iax, [x]
	push	ss
	push	iax

	push	ss
	push	isi

	call	__FASTLDMULT

	mov	iax, [RetOff]
	mov	idx, ss
cEnd




cProc  __FASTLDCOS,<PLM,PUBLIC>,<isi,idi>

	parmT	x
	parmI	RetOff

	localT	x2
	localI	count

cBegin

	lea	isi, [x]		    ; x = x * (4/PI)
	push	ss
	push	isi

	push	ss
	push	isi

	mov	iax, codeOFFSET FourByPI
	push	cs
	push	iax

	call	__FASTLDMULT


	lea	idi, [x2]		    ; x2 = x * x
	push	ss
	push	idi

	push	ss
	push	isi

	push	ss
	push	isi

	call	__FASTLDMULT

if 0
	push	ss			    ; (return) = CosP7
	pop	es
	mov	idi, [RetOff]
	mov	isi, codeOFFSET CosP7
	movsw
	movsw
	movsw
	movsw
	movsw
endif
	mov	isi, [RetOff]
	mov	eax, dword ptr [CosP7]
	mov	dword ptr ss:[isi], eax
	mov	eax, dword ptr [CosP7+4]
	mov	dword ptr ss:[isi+4], eax
	mov	ax, word ptr [CosP7+8]
	mov	word ptr ss:[isi+8], ax

	mov	idi, codeOFFSET CosP6

	mov	[count], 7

CosPolyLoop:
	push	ss
	push	isi		    ; (return) = (return) * x2

	push	ss
	push	isi

	lea	iax, [x2]
	push	ss
	push	iax

	call	__FASTLDMULT


	push	ss
	push	isi		    ; (return) = (return) + SinP[n]

	push	ss
	push	isi

	push	cs
	push	idi

	xor	iax, iax
	push	iax

	call	__FASTLDADD


	sub	idi, 10

	dec	[count]
	jnz	CosPolyLoop

	mov	iax, isi
	mov	idx, ss
cEnd
