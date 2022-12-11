;
; MIT License
; -----------
; 
; Copyright (c) 2002-2019 Advanced Micro Devices, Inc.
; 
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this Software and associated documentaon files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
; 
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
;
; $Workfile: fmod.asm $
; $Revision: 4 $
;     $Date: 9/15/04 16:43 $
;
;
; This is an optimized version of fmod.
;
; Define _CRTBLD_C9X to make it compliant with C90 and on.
;
; If building the OS CRTL (_NTSUBSET_ defined), abort.

;		.ERRDEF _NTSUBSET_, "x87 code cannot be used in kernel mode"

DOMAIN		EQU	1			; _DOMAIN
EDOM		EQU	33			; EDOM
FPCODEFMOD	EQU	22			; _FpCodeFmod
INVALID		EQU	8			; AMD_F_INVALID

FPIND		EQU	0fff8000000000000h	; indefinite
FPSNAN		EQU	07ff7ffffffffffffh	; SNAN
FPQNAN		EQU	07fffffffffffffffh	; QNAN

X87SW		RECORD	X87SW_B: 1,
			X87SW_C3: 1,
			X87SW_TOP: 3,
			X87SW_C: 3,
			X87SW_ES: 1,
			X87SW_SF: 1,
			X87SW_PE: 1,
			X87SW_E: 5

X87XAM		EQU	MASK X87SW_C3 OR MASK X87SW_C AND NOT (1 SHL (X87SW_C + 1))
X87XAM_INF	EQU	5 SHL X87SW_C
X87XAM_NAN	EQU	1 SHL X87SW_C
X87XAM_BAD	EQU	MASK X87SW_E AND NOT 2

		EXTRN	_handle_error: PROC	; float _handle_error (char *fname, int opcode, unsigned long long value, int type, int flags, int error, double arg1, double arg2, int nargs)

		.const

@fmodz		DB	"fmod", 0

		.CODE

; double fmod [double, double] ----------------------------------

PUBLIC fmod
fmod		PROC	FRAME

		sub	rsp, 40 + 32

		.ALLOCSTACK 40 + 32
		.ENDPROLOG

		movsd	QWORD PTR 24 [rsp + 32], xmm1	; Y
		movsd	QWORD PTR 16 [rsp + 32], xmm0	; X

		DB	0ddh, 44h, 24h, 38h	; fld	QWORD PTR 24 [rsp + 32]
		DB	0ddh, 44h, 24h, 30h	; fld	QWORD PTR 16 [rsp + 32]

		DB	0d9h, 0e5h		; fxam (X)
		DB	09bh, 0ddh, 07ch, 024h, 010h ; fstsw 16 [rsp]

		movzx	ecx, WORD PTR 16 [rsp]
		and	ecx, X87XAM

		fnclex			; clear exception flags
						; in preparation for fprem

@again:
		DB	0d9h, 0f8h		; fprem

		DB	09bh, 0dfh, 0e0h 	; fstsw	ax
		test	ax, 4 SHL X87SW_C
		jnz	@again	; do it again in case of partial result

		DB	0ddh, 01ch, 024h	; fstp	QWORD PTR [rsp]
		movlpd	xmm0, QWORD PTR [rsp]		; result

		DB	0d9h, 0e5h		; fxam (Y)
		DB	09bh, 0ddh, 07ch, 024h, 008h ; fstsw 8 [rsp]

		movzx	edx, WORD PTR 8 [rsp]
		and	edx, X87XAM

		DB	0ddh, 0d8h		; fstp	st(0)

		cmp	edx, X87XAM_NAN		; fmod (x, NAN) = QNAN
		je	@error

		cmp	ecx, X87XAM_NAN		; fmod (NAN, y) = QNAN
		je	@error

		and	eax, X87XAM_BAD
		jnz	@raise			; handle error

		IFNDEF	_CRTBLD_C9X		; Not C90
		cmp	edx, X87XAM_INF		; fmod (x, infinity) = ???
		je	@raise
		ELSE				; C90
						; fmod (x, infinity) = x (as x87 already does)
		ENDIF

@exit:
		add	rsp, 40 + 32
		ret

		ALIGN	16

@raise:
		mov	eax, INVALID		; raise exception
		mov	r8, FPIND
		jmp	@fail

@error:
		xor	eax, eax		; no exception
		movd	r8, xmm0
		jmp	@fail

@fail:
		lea	rcx, [@fmodz]		; fname
		mov	edx, FPCODEFMOD		; opcode
;		mov	r8, INDEF		; value
		mov	r9d, DOMAIN		; type
		mov	DWORD PTR  0 [rsp + 32], eax ; flags
		mov	DWORD PTR  8 [rsp + 32], EDOM ; error
		mov	DWORD PTR 32 [rsp + 32], 2 ; nargs
		call	_handle_error		; (char *fname, int opcode, unsigned long long value, int type, int flags, int error, double arg1, double arg2, int nargs)

		DB	09bh, 0dbh, 0e2h	; fclex
		jmp	@exit

fmod		ENDP

; ---------------------------------------------------------------

		END
