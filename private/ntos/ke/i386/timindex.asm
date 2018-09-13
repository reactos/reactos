        TITLE   "Compute Timer Table Index"
;++
;
; Copyright (c) 1993  Microsoft Corporation
;
; Module Name:
;
;    timindex.asm
;
; Abstract:
;
;    This module implements the code necessary to compute the timer table
;    index for a timer.
;
; Author:
;
;    David N. Cutler (davec) 19-May-1993
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

        extrn  _KiTimeIncrementReciprocal:dword
        extrn  _KiTimeIncrementShiftCount:BYTE

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page
        subttl  "Compute Timer Table Index"
;++
;
; ULONG
; KiComputeTimerTableIndex (
;    IN LARGE_INTEGER Interval,
;    IN LARGE_INTEGER CurrentTime,
;    IN PKTIMER Timer
;    )
;
; Routine Description:
;
;    This function computes the timer table index for the specified timer
;    object and stores the due time in the timer object.
;
;    N.B. The interval parameter is guaranteed to be negative since it is
;         expressed as relative time.
;
;    The formula for due time calculation is:
;
;    Due Time = Current Time - Interval
;
;    The formula for the index calculation is:
;
;    Index = (Due Time / Maximum time) & (Table Size - 1)
;
;    The time increment division is performed using reciprocal multiplication.
;
; Arguments:
;
;    Interval  - Supplies the relative time at which the timer is to
;        expire.
;
;    CurrentCount  - Supplies the current system tick count.
;
;    Timer - Supplies a pointer to a dispatch object of type timer.
;
; Return Value:
;
;    The time table index is returned as the function value and the due
;    time is stored in the timer object.
;
;--

LocalStack  equ  20

Interval    equ [esp+LocalStack+4]
CurrentTime equ [esp+LocalStack+12]
Timer       equ [esp+LocalStack+20]

cPublicProc _KiComputeTimerTableIndex ,5
        sub     esp, LocalStack
        mov     [esp+16], ebx
        mov     ebx,CurrentTime         ; get low current time
        mov     ecx,CurrentTime + 4     ; get high current time
        sub     ebx,Interval            ; subtract low parts
        sbb     ecx,Interval + 4        ; subtract high parts and borrow
        mov     eax,Timer               ; get address of timer object
        mov     [eax].TiDueTime.LiLowPart,ebx ; set low part of due time
        mov     [eax].TiDueTime.LiHighPart,ecx ; set high part of due time

;
; Compute low 32-bits of dividend times low 32-bits of divisor.
;

        mov     eax,ebx                 ; copy low 32-bits of dividend
        mul     [_KiTimeIncrementReciprocal] ; multiply by low 32-bits of divisor
        mov     [esp+12], edx           ; save high order 32-bits of product

;
; Compute low 32-bits of dividend times high 32-bits of divisor.
;

        mov     eax,ebx                 ; copy low 32-bits of dividend
        mul     [_KiTimeIncrementReciprocal+4] ;multiply by high 32-bits of divisor
        mov     [esp+8], eax            ; save full 64-bit product
        mov     [esp+4], edx            ;

;
; Compute high 32-bits of dividend times low 32-bits of divisor.
;

        mov     eax,ecx                 ; copy high 32-bits of dividend
        mul     [_KiTimeIncrementReciprocal] ; multiply by low 32-bits of divisor
        mov     [esp+0], edx            ; save high 32-bits of product

;
; Compute carry out of low 64-bits of 128-bit product.
;

        xor     ebx,ebx                 ; clear carry accumlator
        add     eax,[esp]+8             ; generate carry
        adc     ebx,0                   ; accumlate carry
        add     eax,[esp]+12             ; generate carry
        adc     ebx,0                   ; accumulate carry

;
; Compute high 32-bits of dividend times high 32-bits of divisor.
;

        mov     eax,ecx                 ; copy high 32-bits of dividend
        mul     [_KiTimeIncrementReciprocal+4] ; multiply by high 32-bits of divisor

;
; Compute high 64-bits of 128-bit product.
;

        add     eax,ebx                 ; add carry from low 64-bits
        adc     edx,0                   ; propagate carry
        add     eax,[esp]+0             ; add and generate carry
        adc     edx,0                   ; propagate carry
        add     eax,[esp]+4             ; add and generate carry
        adc     edx,0                   ; propagate carry

;
; Right shift the result by the specified shift count and mask off extra
; bits.
;

        mov     cl,[_KiTimeIncrementShiftCount] ; get shift count value
        shrd    eax,edx,cl              ; extract appropriate product bits

        mov     ebx, [esp+16]           ; restore register
        add     esp, LocalStack         ; trim stack
        and     eax,(TIMER_TABLE_SIZE-1); reduce to size of table

        stdRET    _KicomputeTimerTableIndex

stdENDP _KiComputeTimerTableIndex

_TEXT$00   ends
        end
