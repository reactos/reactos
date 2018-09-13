        title   "Critical Section Support"
;++
;
;  Copyright (c) 1991  Microsoft Corporation
;
;  Module Name:
;
;     critsect.asm
;
;  Abstract:
;
;     This module implements functions to support user mode interlocked operations.
;
;  Author:
;
;     Bryan M. Willman (bryanwi) 2-Oct-91
;
;  Environment:
;
;     Any mode.
;
;  Revision History:
;
;
;   WARNING!!!!!!!!!! Some of this code is duplicated in
;   ntos\dll\i386\critsect.asm
;
;   Some day we should put it in a .inc file that both include.
;
;--

.486p
        .xlist
include ks386.inc
include callconv.inc
        .list

_DATA   SEGMENT DWORD PUBLIC 'DATA'
    public _BasepLockPrefixTable
_BasepLockPrefixTable    label dword
        dd offset FLAT:Lock1
        dd offset FLAT:Lock2
        dd offset FLAT:Lock3
        dd offset FLAT:Lock4
        dd offset FLAT:Lock5
        dd 0
_DATA   ENDS


_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


Addend      equ     [esp + 4]
FlagMask    equ     0c0000000H
FlagShift   equ     24
FlagSelect  equ     30

        page , 132
        subttl  "InterlockedIncrement"

;++
;
; LONG
; InterlockedIncrement(
;    IN PLONG Addend
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of one to the addend variable.
;
;    No checking is done for overflow.
;
; Arguments:
;
;    Addend - Supplies a pointer to a variable whose value is to be
;       incremented by one.
;
; Return Value:
;
;   (eax) - the incremented value.
;
;--

cPublicProc _InterlockedIncrement,1
cPublicFpo 1,0
        mov     ecx,Addend              ; get pointer to addend variable
        mov     eax,1                   ; set increment value
Lock1:
   lock xadd    [ecx],eax               ; interlocked increment
        inc     eax                     ; adjust return value
        stdRET _InterlockedIncrement    ;

stdENDP _InterlockedIncrement

        page , 132
        subttl  "InterlockedDecrment"
;++
;
; LONG
; InterlockedDecrement(
;    IN PLONG Addend
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of -1 to the addend variable.
;
;    No checking is done for overflow
;
; Arguments:
;
;    Addend - Supplies a pointer to a variable whose value is to be
;       decremented by one.
;
; Return Value:
;
;   (eax) - The decremented value.
;
;--

cPublicProc _InterlockedDecrement,1
cPublicFpo 1,0

        mov     ecx,Addend              ; get pointer to addend variable
        mov     eax,-1                  ; set decrement value
Lock2:
   lock xadd    [ecx],eax               ; interlocked decrement
        dec     eax                     ; adjust return value
        stdRET _InterlockedDecrement    ;

stdENDP _InterlockedDecrement

        page , 132
        subttl  "Interlocked Exchange"
;++
;
; LONG
; InterlockedExchange(
;    IN OUT LPLONG Target,
;    IN LONG Value
;    )
;
; Routine Description:
;
;    This function atomically exchanges the Target and Value, returning
;    the prior contents of Target
;
; Arguments:
;
;    Target - Address of LONG to exchange
;    Value  - New value of LONG
;
; Return Value:
;
;    (eax) - The prior value of target.
;--

cPublicProc _InterlockedExchange, 2
cPublicFpo 2,0

        mov     ecx, [esp+4]                ; (ecx) = Target
        mov     edx, [esp+8]                ; (edx) = Value
        mov     eax, [ecx]                  ; get comperand value
Ixchg:
Lock5:
   lock cmpxchg [ecx], edx                  ; compare and swap
        jnz     Ixchg                       ; if nz, exchange failed
        stdRET  _InterlockedExchange

stdENDP _InterlockedExchange

        page , 132
        subttl  "Interlocked Compare Exchange"
;++
;
;   PVOID
;   InterlockedCompareExchange (
;       IN OUT PVOID *Destination,
;       IN PVOID Exchange,
;       IN PVOID Comperand
;       )
;
;   Routine Description:
;
;    This function performs an interlocked compare of the destination
;    value with the comperand value. If the destination value is equal
;    to the comperand value, then the exchange value is stored in the
;    destination. Otherwise, no operation is performed.
;
; Arguments:
;
;    Destination - Supplies a pointer to destination value.
;
;    Exchange - Supplies the exchange value.
;
;    Comperand - Supplies the comperand value.
;
; Return Value:
;
;    (eax) - The initial destination value.
;
;--

cPublicProc _InterlockedCompareExchange, 3
cPublicFpo 3,0

        mov     ecx, [esp + 4]          ; get destination address
        mov     edx, [esp + 8]          ; get exchange value
        mov     eax, [esp + 12]         ; get comperand value
Lock3:
   lock cmpxchg [ecx], edx              ; compare and exchange

        stdRET  _InterlockedCompareExchange

stdENDP _InterlockedCompareExchange

        page , 132
        subttl  "Interlocked Exchange Add"
;++
;
;   LONG
;   InterlockedExchangeAdd (
;       IN OUT PLONG Addend,
;       IN LONG Increment
;       )
;
;   Routine Description:
;
;    This function performs an interlocked add of an increment value to an
;    addend variable of type unsinged long. The initial value of the addend
;    variable is returned as the function value.
;
;       It is NOT possible to mix ExInterlockedDecrementLong and
;       ExInterlockedIncrementong with ExInterlockedAddUlong.
;
;
; Arguments:
;
;    Addend - Supplies a pointer to a variable whose value is to be
;       adjusted by the increment value.
;
;    Increment - Supplies the increment value to be added to the
;       addend variable.
;
; Return Value:
;
;    (eax) - The initial value of the addend.
;
;--

cPublicProc _InterlockedExchangeAdd, 2
cPublicFpo 2,0

        mov     ecx, [esp + 4]          ; get addend address
        mov     eax, [esp + 8]          ; get increment value
Lock4:
   lock xadd    [ecx], eax              ; exchange add

        stdRET  _InterlockedExchangeAdd

stdENDP _InterlockedExchangeAdd

        page , 132
        subttl  "Multiply and Divide"
;++
;
; LONG
; MulDiv(
;    IN LONG nNumber,
;    IN LONG nNumerator,
;    IN LONG nDenominator
;    )
;
; Routine Description:
;
;    This function multiples two 32-bit numbers forming a 64-bit product.
;    The 64-bit product is rounded and then divided by a 32-bit divisor
;    yielding a 32-bit result.
;
; Arguments:
;
;    nNumber - Supllies the multiplier.
;
;    nNumerator - Supplies the multiplicand.
;
;    nDenominator - Supplies the divisor.
;
; Return Value:
;
;    If the divisor is zero or an overflow occurs, then a value of -1 is
;    returned as the function value. Otherwise, the rounded quotient is
;    returned as the funtion value.
;
;--

nNumber      equ [esp + 4]
nNumerator   equ [esp + 8]
nDenominator equ DWORD PTR [esp + 12]

cPublicProc _MulDiv, 3
cPublicFpo 3,0
        mov     eax, nNumber            ; get multiplier absolute value
        or      eax, eax                ;
        js      short MD32_First        ; if s, multiplier is negative

;
; The multiplier is positive.
;

        mov     edx, nNumerator         ; get multiplicand absolute value
        or      edx, edx                ;
        js      MD32_Second             ; if s, multiplicand is negative

;
; The multiplicand is positive.
;

        mul     edx                     ; compute 64-bit product
        mov     ecx, nDenominator       ; get denominator absolute value
        or      ecx, ecx                ;
        js      MD32_Third              ; if s, divisor is negative

;
; The divisor is positive.
;

        sar     ecx, 1                  ; compute rounding value
        add     eax, ecx                ; round the 64-bit produce by the
        adc     edx, 0                  ; divisor / 2
        cmp     edx, nDenominator       ; check for overflow
        jae     short MD32_error        ; if ae, overflow or divide by 0
        div     nDenominator            ; compute quotient

;
; The result is postive.
;

        or      eax, eax                ; check for overflow
        js      short MD32_error        ; if s, overlfow has occured

        stdRET  _MulDiv

MD32_error:
        xor     eax, eax                ; set return value to - 1
        dec     eax                     ;

        stdRET  _MulDiv

;
; The multiplier is negative.
;

MD32_First:                             ;
        neg     eax                     ; negate multiplier
        mov     edx, nNumerator         ; get multiplicand absolute value
        or      edx, edx                ;
        js      short MD32_First10      ; if s, multiplicand is negative

;
; The multiplicand is positive.
;

        mul     edx                     ; compute 64-bit product
        mov     ecx, nDenominator       ; get denominator absolute value
        or      ecx, ecx                ;
        js      short MD32_First20      ; if s, divisor is negative

;
; The divisor is positive.
;

        sar     ecx, 1                  ; compute rounding value
        add     eax, ecx                ; round the 64-bit produce by the
        adc     edx, 0                  ; divisor / 2
        cmp     edx, nDenominator       ; check for overflow
        jae     short MD32_error10      ; if ae, overflow or divide by 0
        div     nDenominator            ; compute quotient

;
; The result is negative.
;

        neg     eax                     ; negate result
        jg      short MD32_error10      ; if g, overlfow has occured

        stdRET  _MulDiv

;
; The multiplier is negative and the multiplicand is negative.
;

MD32_First10:                           ;
        neg     edx                     ; negate multiplicand
        mul     edx                     ; compute 64-bit product
        mov     ecx, nDenominator       ; get denominator absolute value
        or      ecx, ecx                ;
        js      short MD32_First30      ; if s, divisor is negative

;
; The divisor is positive.
;

        sar     ecx, 1                  ; compute rounding value
        add     eax, ecx                ; round the 64-bit produce by the
        adc     edx, 0                  ; divisor / 2
        cmp     edx, nDenominator       ; check for overflow
        jae     short MD32_error10      ; if ae, overflow or divide by 0
        div     nDenominator            ; compute quotient

;
; The result is positive.
;

        or      eax, eax                ; check for overflow
        js      short MD32_error10      ; if s, overlfow has occured

        stdRET  _MulDiv

MD32_error10:                           ;
        xor     eax, eax                ; set return value to - 1
        dec     eax                     ;

        stdRET  _MulDiv


;
; The multiplier is negative, the multiplicand is positive, and the
; divisor is negative.
;

MD32_First20:                           ;
        neg     ecx                     ; negate divisor
        push    ecx                     ; save absolute value of divisor
        sar     ecx, 1                  ; compute rounding value
        add     eax, ecx                ; round the 64-bit produce by the
        adc     edx, 0                  ; divisor / 2
        pop     ecx                     ; restore divisor
        cmp     edx, ecx                ; check for overflow
        jae     short MD32_error10      ; if ae, overflow or divide by 0
        div     ecx                     ; compute quotient

;
; The result is postive.
;

        or      eax, eax                ; check for overflow
        js      short MD32_error10      ; if s, overlfow has occured

        stdRET  _MulDiv

;
; The multiplier is negative, the multiplier is negative, and the divisor
; is negative.
;

MD32_First30:                           ;
        neg     ecx                     ; negate divisor
        push    ecx                     ; save absolute value of divisor
        sar     ecx, 1                  ; compute rounding value
        add     eax, ecx                ; round the 64-bit produce by the
        adc     edx, 0                  ; divisor / 2
        pop     ecx                     ; restore divisor
        cmp     edx, ecx                ; check for overflow
        jae     short MD32_error10      ; if ae, overflow or divide by 0
        div     ecx                     ; compute quotient

;
; The result is negative.
;

        neg     eax                     ; negate result
        jg      short MD32_error10      ; if g, overlfow has occured

        stdRET  _MulDiv

;
; The multiplier is positive and the multiplicand is negative.
;

MD32_Second:                            ;
        neg     edx                     ; negate multiplicand
        mul     edx                     ; compute 64-bit product
        mov     ecx, nDenominator       ; get denominator absolute value
        or      ecx, ecx                ;
        js      short MD32_Second10     ; if s, divisor is negative

;
; The divisor is positive.
;

        sar     ecx, 1                  ; compute rounding value
        add     eax, ecx                ; round the 64-bit produce by the
        adc     edx, 0                  ; divisor / 2
        cmp     edx, nDenominator       ; check for overflow
        jae     short MD32_error20      ; if ae, overflow or divide by 0
        div     nDenominator            ; compute quotient

;
; The result is negative.
;

        neg     eax                     ; check for overflow
        jg      short MD32_error20      ; if g, overlfow has occured

        stdRET  _MulDiv

MD32_error20:                           ;
        xor     eax, eax                ; set return value to - 1
        dec     eax                     ;

        stdRET  _MulDiv

;
; The multiplier is positive, the multiplicand is negative, and the divisor
; is negative.
;

MD32_Second10:                          ;
        neg     ecx                     ; negate divisor
        push    ecx                     ; save absolute value of divisor
        sar     ecx, 1                  ; compute rounding value
        add     eax, ecx                ; round the 64-bit produce by the
        adc     edx, 0                  ; divisor / 2
        pop     ecx                     ; restore divisor
        cmp     edx, ecx                ; check for overflow
        jae     short MD32_error20      ; if ae, overflow or divide by 0
        div     ecx                     ; compute quotient

;
; The result is positive.
;

        or      eax, eax                ; check for overflow
        js      short MD32_error10      ; if s, overlfow has occured

        stdRET  _MulDiv

;
; The multiplier is positive, the multiplicand is positive, the divisor
; is negative.
;

MD32_Third:                             ;
        neg     ecx                     ; negate divisor
        push    ecx                     ; save absolute value of divisor
        sar     ecx, 1                  ; compute rounding value
        add     eax, ecx                ; round the 64-bit produce by the
        adc     edx, 0                  ; divisor / 2
        pop     ecx                     ; restore divisor
        cmp     edx, ecx                ; check for overflow
        jae     short MD32_error20      ; if ae, overflow or divide by 0
        div     ecx                     ; compute quotient

;
; The result is negative.
;

        neg     eax                     ; negate result
        jg      short MD32_error20      ; if g, overflow has occured

        stdRET  _MulDiv

stdENDP _MulDiv

_TEXT   ends
        end
