        TITLE   "Large Integer Arithmetic"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    largeint.s
;
; Abstract:
;
;    This module implements routines for performing extended integer
;    arithmtic.
;
; Author:
;
;    David N. Cutler (davec) 24-Aug-1989
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

IFNDEF BLDR_KERNEL_RUNTIME
        EXTRNP  _RtlRaiseStatus, 1
ENDIF


_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "RtlLargeIntegerAdd"
;++
;
; LARGE_INTEGER
; RtlLargeIntegerAdd (
;    IN LARGE_INTEGER Addend1,
;    IN LARGE_INTEGER Addend2
;    )
;
; Routine Description:
;
;    This function adds a signed large integer to a signed large integer and
;    returns the signed large integer result.
;
; Arguments:
;
;    (TOS+4) = Addend1 - first addend value
;    (TOS+12) = Addend2 - second addend value
;
; Return Value:
;
;    The large integer result is stored in (edx:eax)
;
;--

cPublicProc _RtlLargeIntegerAdd ,4
cPublicFpo 4,0

        mov     eax,[esp]+4             ; (eax)=add1.low
        add     eax,[esp]+12            ; (eax)=sum.low
        mov     edx,[esp]+8             ; (edx)=add1.hi
        adc     edx,[esp]+16            ; (edx)=sum.hi
        stdRET    _RtlLargeIntegerAdd

stdENDP _RtlLargeIntegerAdd


        page
        subttl  "Enlarged Integer Multiply"
;++
;
; LARGE_INTEGER
; RtlEnlargedIntegerMultiply (
;    IN LONG Multiplicand,
;    IN LONG Multiplier
;    )
;
; Routine Description:
;
;    This function multiplies a signed integer by an signed integer and
;    returns a signed large integer result.
;
; Arguments:
;
;    (TOS+4) = Factor1
;    (TOS+8) = Factor2
;
; Return Value:
;
;    The large integer result is stored in (edx:eax)
;
;--

cPublicProc __RtlEnlargedIntegerMultiply ,2
cPublicFpo 2,0

        mov     eax,[esp]+4             ; (eax) = factor1
        imul    dword ptr [esp]+8       ; (edx:eax) = signed result
        stdRET    __RtlEnlargedIntegerMultiply

stdENDP __RtlEnlargedIntegerMultiply


        page
        subttl  "Enlarged Unsigned Integer Multiply"
;++
;
; LARGE_INTEGER
; RtlEnlargedUnsignedMultiply (
;    IN ULONG Multiplicand,
;    IN ULONG Multiplier
;    )
;
; Routine Description:
;
;    This function multiplies an un signed integer by an unsigned integer and
;    returns a signed large integer result.
;
; Arguments:
;
;    (TOS+4) = Factor1
;    (TOS+8) = Factor2
;
; Return Value:
;
;    The large integer result is stored in (edx:eax)
;
;--

cPublicProc __RtlEnlargedUnsignedMultiply ,2
cPublicFpo 2,0

        mov     eax,[esp]+4             ; (eax) = factor1
        mul     dword ptr [esp]+8       ; (edx:eax) = unsigned result
        stdRET    __RtlEnlargedUnsignedMultiply

stdENDP __RtlEnlargedUnsignedMultiply

        page
        subttl  "Enlarged Unsigned Integer Divide"

;++
;
; ULONG
; RtlEnlargedUnsignedDivide (
;    IN ULARGE_INTEGER Dividend,
;    IN ULONG Divisor,
;    IN PULONG Remainder
;    )
;
;
; Routine Description:
;
;    This function divides an unsigned large integer by an unsigned long
;    and returns the resultant quotient and optionally the remainder.
;
; Arguments:
;
;    Dividend - Supplies the dividend value.
;
;    Divisor - Supplies the divisor value.
;
;    Remainder - Supplies an optional pointer to a variable that
;        receives the remainder.
;
; Return Value:
;
;    The unsigned long integer quotient is returned as the function value.
;
;--

cPublicProc __RtlEnlargedUnsignedDivide,4
cPublicFpo 4,0

        mov     eax, [esp+4]            ; (eax) = Dividend.LowPart
        mov     edx, [esp+8]            ; (edx) = Dividend.HighPart
        mov     ecx, [esp+16]           ; (ecx) = pRemainder
        div     dword ptr [esp+12]      ; divide by Divisor

        or      ecx, ecx                ; return remainder?
        jnz     short @f

        stdRET    __RtlEnlargedUnsignedDivide    ; (eax) = Quotient

align 4
@@:     mov     [ecx], edx              ; save remainder
        stdRET    __RtlEnlargedUnsignedDivide    ; (eax) = Quotient

stdENDP __RtlEnlargedUnsignedDivide

        page
        subttl  "Extended Large Integer Divide"

;++
;
; LARGE_INTEGER
; RtlExtendedLargeIntegerDivide (
;     IN LARGE_INTEGER Dividend,
;     IN ULONG Divisor,
;     OUT PULONG Remainder OPTIONAL
;     )
;
; Routine Description:
;
;     This routine divides an unsigned 64 bit dividend by a 32 bit divisor
;     and returns a 64-bit quotient, and optionally the 32-bit remainder.
;
;
; Arguments:
;
;     Dividend - Supplies the 64 bit dividend for the divide operation.
;
;     Divisor - Supplies the 32 bit divisor for the divide operation.
;
;     Remainder - Supplies an optional pointer to a variable which receives
;         the remainder
;
; Return Value:
;
;     The 64-bit quotient is returned as the function value.
;
;--

cPublicProc _RtlExtendedLargeIntegerDivide, 4
cPublicFpo 4,3

        push    esi
        push    edi
        push    ebx

        mov     eax, [esp+16]       ; (eax) = Dividend.LowPart
        mov     edx, [esp+20]       ; (edx) = Dividend.HighPart

lid00:  mov     ebx, [esp+24]       ; (ebx) = Divisor
        or      ebx, ebx
        jz      short lid_zero      ; Attempted a divide by zero

        push    ebp

        mov     ecx, 64             ; Loop count
        xor     esi, esi            ; Clear partial remainder

; (edx:eax) = Dividend
; (ebx) = Divisor
; (ecx) = Loop count
; (esi) = partial remainder

align 4
lid10:  shl     eax, 1              ; (LowPart << 1)  | 0
        rcl     edx, 1              ; (HighPart << 1) | CF
        rcl     esi, 1              ; (Partial << 1)  | CF

        sbb     edi, edi            ; clone CF into edi (0 or -1)

        cmp     esi, ebx            ; check if partial remainder less then divisor
        cmc
        sbb     ebp, ebp            ; clone CF intp ebp
        or      edi, ebp            ; merge with remainder of high bit

        sub     eax, edi            ; merge quotient bit
        and     edi, ebx            ; Select divisor or 0
        sub     esi, edi

        dec     ecx                 ; dec interration count
        jnz     short lid10         ; go around again

        pop     ebp
        pop     ebx
        pop     edi

        mov     ecx, [esp+20]       ; (ecx) = Remainder
        or      ecx, ecx
        jnz     short lid20

        pop     esi
        stdRET  _RtlExtendedLargeIntegerDivide

align 4
lid20:
        mov     [ecx], esi          ; store remainder
        pop     esi
        stdRET  _RtlExtendedLargeIntegerDivide

lid_zero:
IFNDEF BLDR_KERNEL_RUNTIME
        stdCall _RtlRaiseStatus, <STATUS_INTEGER_DIVIDE_BY_ZERO>
ENDIF
        pop     ebx
        pop     edi
        pop     esi
        stdRET  _RtlExtendedLargeIntegerDivide

stdENDP     _RtlExtendedLargeIntegerDivide

        page
        subttl  "Extended Magic Divide"
;++
;
; LARGE_INTEGER
; RtlExtendedMagicDivide (
;    IN LARGE_INTEGER Dividend,
;    IN LARGE_INTEGER MagicDivisor,
;    IN CCHAR ShiftCount
;    )
;
; Routine Description:
;
;    This function divides a signed large integer by an unsigned large integer
;    and returns the signed large integer result. The division is performed
;    using reciprocal multiplication of a signed large integer value by an
;    unsigned large integer fraction which represents the most significant
;    64-bits of the reciprocal divisor rounded up in its least significant bit
;    and normalized with respect to bit 63. A shift count is also provided
;    which is used to truncate the fractional bits from the result value.
;
; Arguments:
;
;   (ebp+8) = Dividend
;   (ebp+16) = MagicDivisor value is a 64-bit multiplicative reciprocal
;   (ebp+24) = ShiftCount - Right shift adjustment value.
;
; Return Value:
;
;    The large integer result is stored  in (edx:eax)
;
;--

RemdDiv     equ [ebp+8]             ; Dividend
RemdRec     equ [ebp+16]            ; Reciprocal (magic divisor)
RemdShift   equ [ebp+24]
RemdTmp1    equ [ebp-4]
RemdTmp2    equ [ebp-8]
RemdTmp3    equ [ebp-12]

cPublicProc _RtlExtendedMagicDivide ,5

        push    ebp
        mov     ebp,esp
        sub     esp,12
        push    esi

        mov     esi, RemdDiv+4
        test    esi,80000000h
        jz      remd10                  ; no sign, no need to negate

        neg     dword ptr RemdDiv+4
        neg     dword ptr RemdDiv
        sbb     dword ptr RemdDiv+4,0   ; negate

remd10: mov     eax,RemdRec
        mul     dword ptr RemdDiv       ; (edx:eax) = Div.lo * Rec.lo
        mov     RemdTmp1,edx

        mov     eax,RemdRec
        mul     dword ptr RemdDiv+4     ; (edx:eax) = Div.hi * Rec.lo
        mov     RemdTmp2,eax
        mov     RemdTmp3,edx

        mov     eax,RemdRec+4
        mul     dword ptr RemdDiv       ; (edx:eax) = Div.lo * Rec.hi

;
;   Col 0 doesn't matter
;   Col 1 = Hi(Div.lo * Rec.lo) + Low(Div.Hi * Rec.lo) + Low(Div.lo * Rec.hi)
;         = RemdTmp1 + RemdTmp2 + eax
;         -> Only want carry from Col 1
;

        xor     ecx,ecx                 ; (ecx) = 0
        add     eax,RemdTmp1
        adc     ecx, 0                  ; save carry in ecx
        add     eax,RemdTmp2
        adc     ecx, 0                  ; Save Carry, all we want from Col2

        mov     RemdTmp1,edx

        mov     eax,RemdRec+4
        mul     dword ptr RemdDiv+4     ; (edx:eax) = Div.Hi * Rec.Hi

;
;   TOS = carry flag from Col 1
;
;   Col 2 = Col1 CF +
;           Hi(Div.Hi * Rec.Lo) + Hi(Div.Lo * Rec.Hi) + Low(Div.Hi * Rec.Hi)
;         = CF + RemdTmp3 + RemdTmp1 + eax
;
;   Col 3 = Col2 CF + Hi(Div.Hi * Rec.Hi)
;         = CF + edx
;

        add     eax,RemdTmp1
        adc     edx, 0                  ; add carry to edx
        add     eax,RemdTmp3            ; (eax) = col 2
        adc     edx, 0                  ; add carry to edx
        add     eax, ecx
        adc     edx, 0                  ; (edx) = col 3

;
;   (edx:eax) = the high 64 bits of the multiply, shift it right by
;               shift count to discard bits to right of virtual decimal pt.
;
;   RemdShift could be as large as 63 and still not 0 the result, 386
;   will only shift 31 bits at a time, so must do the sift multiple
;   times to get correct effect.
;

        mov     cl,RemdShift
remd20: cmp     cl,31
        jbe     remd30
        sub     cl,31
        shrd    eax,edx,31
        shr     edx,31
        jmp     remd20

remd30: shrd    eax,edx,cl
        shr     edx,cl

;
;   Negate the result if need be
;

        test    esi,80000000h
        jz      remd40                  ; no sign, go return without negate

        neg     edx
        neg     eax
        sbb     edx,0

;
;   Store the result
;

remd40:
        ; results in (edx:eax)

        pop     esi
        mov     esp,ebp
        pop     ebp
        stdRET    _RtlExtendedMagicDivide

stdENDP _RtlExtendedMagicDivide


        page
        subttl  "Extended Integer Multiply"
;++
;
; LARGE_INTEGER
; RtlExtendedIntegerMultiply (
;    IN LARGE_INTEGER Multiplicand,
;    IN ULONG Multiplier
;    )
;
; Routine Description:
;
;    This function multiplies a signed large integer by a signed integer and
;    returns the signed large integer result.
;
; Arguments:
;
;   (ebp+8,12)=multiplican (MCAN)
;   (ebp+16)=multiplier (MPER)
;
; Return Value:
;
;    The large integer result is stored in (edx:eax)
;
;--

ReimMCAN    equ <dword ptr [ebp+8]>
ReimMPER    equ <dword ptr [ebp+16]>

cPublicProc _RtlExtendedIntegerMultiply ,3

        push    ebp
        mov     ebp,esp
        push    esi

        mov     esi,ReimMPER
        xor     esi,ReimMCAN+4              ; (esi) = result sign

        test    ReimMCAN+4,80000000h
        jz      short reim10                ; MCAN pos, go look at MPER

        neg     dword ptr ReimMCAN+4
        neg     dword ptr ReimMCAN
        sbb     dword ptr ReimMCAN+4,0      ; negate multiplican

reim10: test    ReimMPER,80000000h
        jz      short reim20                ; MPER pos, go do multiply

        neg     dword ptr ReimMPER          ; negate multiplier

reim20: mov     eax,ReimMPER
        mul     dword ptr ReimMCAN          ; (edx:eax) = MPER * MCAN.low
        push    edx
        mov     ecx, eax
        mov     eax,ReimMPER
        mul     dword ptr ReimMCAN+4        ; (edx:eax) = MPER * MCAN.high
        add     eax,[esp]                   ; (eax) = hi part of MPER*MCAN.low
                                            ;   plus low part of MPER*MCAN.hi

        test    esi,80000000h
        jz      short reim30                ; result sign is OK, go return

        neg     eax
        neg     ecx
        sbb     eax,0                       ; negate result

reim30: add     esp,4                       ; clean eax off stack
        pop     esi                         ; restore nonvolatile reg
        mov     edx,eax                     ; (edx:ecx) = result
        mov     eax,ecx                     ; (edx:eax) = result

        pop     ebp
        stdRET    _RtlExtendedIntegerMultiply

stdENDP _RtlExtendedIntegerMultiply

        page
        subttl  "Large Integer Shift Left"
;++
;
; LARGE_INTEGER
; RtlLargeIntegerShiftLeft (
;     IN LARGE_INTEGER LargeInteger,
;     IN CCHAR ShiftCount
;     )
;
;
; Routine Description:
;
;     This routine does a left logical shift of a large integer by a
;     specified amount (ShiftCount) modulo 64.
;
; Arguments:
;
;     LargeInteger - Supplies the large integer to be shifted
;
;     ShiftCount - Supplies the left shift count
;
; Return Value:
;
;     LARGE_INTEGER - Receives the shift large integer result
;
;--
cPublicProc _RtlLargeIntegerShiftLeft,3
cPublicFpo 3,0

        mov     ecx, [esp+12]           ; (ecx) = ShiftCount
        and     ecx, 3fh                ; mod 64

        cmp     ecx, 32
        jnc     short sl10
;
;  Shift count is less then 32 bits.
;
        mov     eax, [esp+4]            ; (eax) = LargeInteger.LowPart
        mov     edx, [esp+8]            ; (edx) = LargeInteger.HighPart
        shld    edx, eax, cl
        shl     eax, cl

        stdRET  _RtlLargeIntegerShiftLeft

align 4
sl10:
;
;  Shift count is greater than or equal 32 bits - low half of result is zero,
;  high half is the low half shifted left by remaining count.
;
        mov     edx, [esp+4]            ; (edx) = LargeInteger.LowPart
        xor     eax, eax                ; store lowpart
        shl     edx, cl                 ; store highpart

        stdRET  _RtlLargeIntegerShiftLeft

stdENDP _RtlLargeIntegerShiftLeft

        page
        subttl  "Large Integer Shift Right"

;--
;
;LARGE_INTEGER
;RtlLargeIntegerShiftRight (
;    IN LARGE_INTEGER LargeInteger,
;    IN CCHAR ShiftCount
;    )
;
;Routine Description:
;
;    This routine does a right logical shift of a large integer by a
;    specified amount (ShiftCount) modulo 64.
;
;Arguments:
;
;    LargeInteger - Supplies the large integer to be shifted
;
;    ShiftCount - Supplies the right shift count
;
;Return Value:
;
;    LARGE_INTEGER - Receives the shift large integer result
;
;--*/
cPublicProc _RtlLargeIntegerShiftRight,3
cPublicFpo 3,0

        mov     ecx, [esp+12]           ; (ecx) = ShiftCount
        and     ecx, 3fh                ; mod 64

        cmp     ecx, 32
        jnc     short sr10

;
;  Shift count is less then 32 bits.
;
        mov     eax, [esp+4]            ; (eax) = LargeInteger.LowPart
        mov     edx, [esp+8]            ; (edx) = LargeInteger.HighPart
        shrd    eax, edx, cl
        shr     edx, cl

        stdRET  _RtlLargeIntegerShiftRight

align 4
sr10:
;
;  Shift count is greater than or equal 32 bits - high half of result is zero,
;  low half is the high half shifted right by remaining count.
;
        mov     eax, [esp+8]            ; (eax) = LargeInteger.HighPart
        xor     edx, edx                ; store highpart
        shr     eax, cl                 ; store lowpart

        stdRET  _RtlLargeIntegerShiftRight

stdENDP _RtlLargeIntegerShiftRight

;++
;
;LARGE_INTEGER
;RtlLargeIntegerArithmeticShift (
;    IN LARGE_INTEGER LargeInteger,
;    IN CCHAR ShiftCount
;    )
;
;Routine Description:
;
;    This routine does a right arithmetic shift of a large integer by a
;    specified amount (ShiftCount) modulo 64.
;
;Arguments:
;
;    LargeInteger - Supplies the large integer to be shifted
;
;    ShiftCount - Supplies the right shift count
;
;Return Value:
;
;    LARGE_INTEGER - Receives the shift large integer result
;
;--
cPublicProc _RtlLargeIntegerArithmeticShift,3
cPublicFpo 3,0

        mov     ecx, [esp+12]           ; (ecx) = ShiftCount
        and     ecx, 3fh                ; mod 64

        cmp     ecx, 32
        jc      short sar10

;
;  Shift count is greater than or equal 32 bits - high half of result is sign
;  bit, low half is the high half shifted right by remaining count.
;
        mov     eax, [esp+8]            ; (eax) = LargeInteger.HighPart
        sar     eax, cl                 ; store highpart
        bt      eax, 31                 ; sign bit set?
        sbb     edx, edx                ; duplicate sign bit into highpart

        stdRET  _RtlLargeIntegerArithmeticShift

align 4
sar10:
;
;  Shift count is less then 32 bits.
;
;
        mov     eax, [esp+4]            ; (eax) = LargeInteger.LowPart
        mov     edx, [esp+8]            ; (edx) = LargeInteger.HighPart
        shrd    eax, edx, cl
        sar     edx, cl

        stdRET  _RtlLargeIntegerArithmeticShift

stdENDP _RtlLargeIntegerArithmeticShift,3


        page
        subttl  "Large Integer Negate"
;++
;
; LARGE_INTEGER
; RtlLargeIntegerNegate (
;    IN LARGE_INTEGER Subtrahend
;    )
;
; Routine Description:
;
;    This function negates a signed large integer and returns the signed
;    large integer result.
;
; Arguments:
;
;   (TOS+4) = Subtrahend
;
; Return Value:
;
;    The large integer result is stored in (edx:eax)
;
;--

cPublicProc _RtlLargeIntegerNegate  ,2
cPublicFpo 2,0

        mov     eax,[esp]+4             ; (eax) = lo
        mov     edx,[esp]+8
        neg     edx                     ; (edx) = 2's comp of hi part
        neg     eax                     ; if ((eax) == 0) CF = 0
                                        ; else CF = 1
        sbb     edx,0                   ; (edx) = (edx) - CF
                                        ; (edx:eax) = result
        stdRET    _RtlLargeIntegerNegate

stdENDP _RtlLargeIntegerNegate


        page
        subttl  "Large Integer Subtract"
;++
;
; LARGE_INTEGER
; RtlLargeIntegerSubtract (
;    IN LARGE_INTEGER Minuend,
;    IN LARGE_INTEGER Subtrahend
;    )
;
; Routine Description:
;
;    This function subtracts a signed large integer from a signed large
;    integer and returns the signed large integer result.
;
; Arguments:
;
;    (TOS+4) = Minuend
;    (TOS+12) = Subtrahend
;
; Return Value:
;
;    The large integer result is stored in (edx:eax)
;
;--

cPublicProc _RtlLargeIntegerSubtract    ,4
cPublicFpo 4,0

        mov     eax,[esp]+4
        sub     eax,[esp]+12            ; (eax) = result.low
        mov     edx,[esp]+8
        sbb     edx,[esp]+16            ; (edx) = result.high
        stdRET    _RtlLargeIntegerSubtract

stdENDP _RtlLargeIntegerSubtract

        page
        subttl  "Convert Long to Large Integer"
;++
;
; LARGE_INTEGER
; RtlConvertLongToLargeInteger (
;     IN LONG SignedInteger
;     )
;
; Routine Description:
;
;     This function converts the input signed integer to a signed large
;     integer and returns the latter as the result.
;
; Arguments:
;
;   (TOS+4) = SignedInteger
;
; Return Value:
;
;    The large integer result is stored (edx:eax)
;
;--

cPublicProc _RtlConvertLongToLargeInteger   ,1
cPublicFpo 1,0

        mov     eax,[esp]+4             ; (eax) = SignedInteger
        cdq                             ; (edx:eax) = signed LargeInt
        stdRET    _RtlConvertLongToLargeInteger

stdENDP _RtlConvertLongToLargeInteger


        page
        subttl  "Convert Ulong to Large Integer"
;++
;
; LARGE_INTEGER
; RtlConvertUlongToLargeInteger (
;     IN LONG UnsignedInteger
;     )
;
; Routine Description:
;
;     This function converts the input unsigned integer to a signed large
;     integer and returns the latter as the result.
;
; Arguments:
;
;   (TOS+4) = UnsignedInteger
;
; Return Value:
;
;    The large integer result is stored in (edx:eax)
;
;--

cPublicProc _RtlConvertUlongToLargeInteger  ,1
cPublicFpo 1,0

        mov     eax,[esp]+4             ; store low
        xor     edx,edx                 ; store 0 in high
        stdRET    _RtlConvertUlongToLargeInteger

stdENDP _RtlConvertUlongToLargeInteger


_TEXT$00   ends
        end
