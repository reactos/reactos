        page    ,132
;---------------------------Module-Header-------------------------------;
; Module Name: MATH.ASM
;
; Contains FIXED point math routines.
;
; Created:  Sun 30-Aug-1987 19:28:30
; Author: Charles Whitmer [chuckwh]
;
; Copyright (c) 1987  Microsoft Corporation
;-----------------------------------------------------------------------;

?WIN	= 0
?PLM	= 1
?NODATA = 0

        .286

        .xlist
        include cmacros.inc
        include windows.inc
        .list

        externA __WinFlags

UQUAD   struc
uq0     dw      ?
uq1     dw      ?
uq2     dw      ?
uq3     dw      ?
UQUAD	ends

;       The following two equates are just used as shorthand
;       for the "word ptr" and "byte ptr" overrides.

wptr    equ     word ptr
bptr    equ     byte ptr

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

EAXtoDXAX   macro
        shld    edx,eax,16      ; move HIWORD(eax) to dx
        endm

DXAXtoEAX   macro
        ror     eax,16          ; xchg HIWORD(eax) and LOWORD(eax)
        shrd    eax,edx,16      ; move LOWORD(edx) to HIWORD(eax)
        endm

neg32       macro hi, lo
        neg lo
        adc hi,0                ; carry set unless lo zero
        neg hi
        endm

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        assumes cs,CodeSeg
	assumes ds,nothing
        assumes es,nothing

;---------------------------Public-Routine------------------------------;
; long muldiv32(long, long, long)
;
; multiples two 32 bit values and then divides the result by a third
; 32 bit value with full 64 bit presision
;
; lResult = (lNumber * lNumerator) / lDenominator with correct rounding
;
; Entry:
;       lNumber = number to multiply by nNumerator
;       lNumerator = number to multiply by nNumber
;       lDenominator = number to divide the multiplication result by.
;   
; Returns:
;       DX:AX = result of multiplication and division.
;
; Error Returns:
;       none
; Registers Preserved:
;       DS,ES,SI,DI
; History:
;   Fri 05-Oct-1990 -by- Rob Williams [Robwi] 
;   Behavior consistent with MulDiv16 routine (signed, no int 0 on overflow)
;   Stole muldiv16 psuedocode
;
;   Wed 14-June-1990 -by-  Todd Laney [ToddLa]
;   converted it to 386/286 code. (by checking __WinFlags)
;
;   Tue 08-May-1990 -by-  Rob Williams [Robwi]
;   Wrote it.
;
;----------------------------Pseudo-Code--------------------------------;
; long FAR PASCAL muldiv32(long, long, long)
; long l;
; long Numer;
; long Denom;
; {
;
;   Sign = sign of Denom;   // Sign will keep track of final sign //
;
;
;   if (Denom < 0)
;   {
;	negate Denom;	// make sure Denom is positive //
;   }
;
;   if (l < 0)
;   {
;	negate l;	// make sure l is positive //
;   }
;
;   make Sign reflect any sign change;
;
;
;   if (Numer < 0)
;   {
;	negate Numer;	// make sure Numer is positive //
;   }
;
;   make Sign reflect any sign change;
;
;   Numer *= l;
;   Numer += (Denom/2); // adjust for rounding //
;
;   if (overflow)   // check for overflow, and handle divide by zero //
;   {
;	jump to md5;
;   }
;
;   result = Numer/Denom;
;
;   if (overflow)   // check again to see if overflow occured //
;   {
;	jump to md5;
;   }
;
;   if (Sign is negative)   // put sign on the result //
;   {
;	negate result;
;   }
;
;md6:
;   return(result);
;
;md5:
;   DX = 7FFF;	    // indicate overflow by //
;   AX = 0xFFFF     // return largest integer //
;   if (Sign is negative)
;   {
;	DX = 0x8000;	// with correct sign //
;	AX = 0x0000;	
;   }
;
;   jump to md6;
; }
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   muldiv32,<PUBLIC,FAR,NODATA,NONWIN>,<>
;       ParmD  lNumber
;       ParmD  lNumerator
;       ParmD  lDenominator
cBegin	<nogen>
	mov	ax,__WinFlags
        test    ax,WF_CPU286+WF_CPU086+WF_CPU186
        jz      md32_1
        jmp     short NEAR PTR muldiv32_286
md32_1:
        errn$   muldiv32_386
cEnd    <nogen>

cProc   muldiv32_386,<PUBLIC,FAR,NODATA,NONWIN>,<>
        ParmD   lNumber
        ParmD   lNumerator
        ParmD   lDenominator
cBegin
        .386


        mov     ebx,lDenominator    ; get the demoninator
        mov     ecx,ebx	            ; ECX holds the final sign in hiword
        or      ebx,ebx             ; ensure the denominator is positive
        jns     short md386_1
        neg     ebx

md386_1:
        mov     eax,lNumber         ; get the long we are multiplying
        xor     ecx,eax	            ; make ECX reflect any sign change
        or      eax,eax             ; ensure the long is positive
        jns     short md386_2
        neg     eax

md386_2:
        mov     edx,lNumerator      ; get the numerator
        xor     ecx,edx	            ; make ECX reflect any sign change
        or      edx,edx             ; ensure the numerator is positive
        jns     short md386_3
        neg     edx

md386_3:
        mul     edx                 ; multiply
        mov     cx,bx               ; get half of the demoninator to adjust for rounding
        sar     ebx,1               
        add     eax,ebx             ; adjust for possible rounding error
        adc     edx,0               ; this is really a long addition
        sal     ebx,1               ; restore the demoninator
        or      bx,cx               ; fix bottom bit
        cmp     edx,ebx             ; check for overflow
        jae     short md386_5           ; (ae handles /0 case)
        div     ebx                 ; divide
        or      eax,eax             ; If sign is set, then overflow occured
        js      short md386_5           ; Overflow.
        or      ecx,ecx             ; put the sign on the result
        jns     short md386_6
        neg     eax

md386_6:
        EAXtoDXAX                   ; convert eax to dx:ax for 16 bit programs

        .286
cEnd 
        .386

md386_5:
        mov     eax,7FFFFFFFh       ; return the largest integer
        or      ecx,ecx             ; with the correct sign
        jns     md386_6
        not     eax
        jmp     md386_6

        .286

cProc   muldiv32_286,<PUBLIC,FAR,NODATA,NONWIN>,<di,si>
        ParmD  lNumber             
        ParmD  lNumerator
        ParmD  lDenominator
        LocalW wSign

cBegin

        mov     dx,lDenominator.hi      ; get the demoninator
        mov     si,dx	                ; SI holds the final sign 
        or      dx,dx                   ; ensure the denominator is positive
        jns     md286_1
        neg32   dx, lDenominator.lo
        mov     lDenominator.hi, dx


md286_1:
        mov     ax,lNumber.lo           ; get the long we are multiplying
        mov     dx,lNumber.hi           
        xor     si,dx	                ; make ECX reflect any sign change
        or      dx,dx                   ; ensure the long is positive
        jns     md286_2
        neg32   dx, ax

md286_2:
        mov     bx,lNumerator.lo        ; get the numerator
        mov     cx,lNumerator.hi        ; get the numerator
        xor     si,cx	                ; make ECX reflect any sign change
        or      cx,cx                   ; ensure the numerator is positive
        jns     md286_3
        neg32   cx, bx

md286_3:
        mov     wSign, si               ; save sign
        call    dmul                    ; multiply (result in dx:cx:bx:ax)
        mov     si, lDenominator.hi
        mov     di, lDenominator.lo
        sar     si, 1                   ; get half of the demoninator 
        rcr     di, 1                   ; to adjust for rounding
        add     ax, di                  ; adjust for possible rounding error
        adc     bx, si
        adc     cx, 0
        adc     dx, 0                   ; this is really a long addition

        sal     di, 1                   ; restore the demoninator
        rcl     si, 1

        or      di, lDenominator.lo     ; fix bottom bit

        cmp     dx, si                  ; begin overflow check (unsigned for div 0 check)
        ja      md286_5                 ; overflow
        jb      md286_7                 ; no overflow
        cmp     cx, di       
        jae     md286_5                 ; overflow

md286_7:
        call    qdiv                    ; DX:AX is quotient     
        or      dx,dx                   ; If sign is set, then overflow occured
        js      md286_5                 ; Overflow.
        mov     cx, wSign
        or      cx,cx                   ; put the sign on the result
        jns     md286_6
        neg32   dx,ax

md286_6:

cEnd

md286_5:
        mov     cx, wSign
        mov     ax, 0FFFFh              ; return the largest integer
        mov     dx, 7FFFh
        or      cx, cx                  ; with the correct sign
        jns     md286_6
        not     dx
        not     ax
        jmp     md286_6

cProc   muldivru32,<PUBLIC,FAR,NODATA,NONWIN>,<>
;       ParmD  lNumber
;       ParmD  lNumerator
;       ParmD  lDenominator
cBegin	<nogen>
	mov	ax,__WinFlags
        test    ax,WF_CPU286+WF_CPU086+WF_CPU186
        jz      mdru32_1
        jmp     short NEAR PTR muldivru32_286
mdru32_1:
        errn$   muldivru32_386
cEnd    <nogen>

cProc   muldivru32_386,<PUBLIC,FAR,NODATA,NONWIN>,<>
        ParmD   lNumber
        ParmD   lNumerator
        ParmD   lDenominator
cBegin
        .386


        mov     ebx,lDenominator    ; get the demoninator
        mov     ecx,ebx	            ; ECX holds the final sign in hiword
        or      ebx,ebx             ; ensure the denominator is positive
        jns     short mdru386_1
        neg     ebx

mdru386_1:
        mov     eax,lNumber         ; get the long we are multiplying
        xor     ecx,eax	            ; make ECX reflect any sign change
        or      eax,eax             ; ensure the long is positive
        jns     short mdru386_2
        neg     eax

mdru386_2:
        mov     edx,lNumerator      ; get the numerator
        xor     ecx,edx	            ; make ECX reflect any sign change
        or      edx,edx             ; ensure the numerator is positive
        jns     short mdru386_3
        neg     edx

mdru386_3:
        mul     edx                 ; multiply
        mov     cx,bx               ; get demoninator - 1 to adjust for rounding
        sub     ebx,1               
        add     eax,ebx             ; adjust for possible rounding error
        adc     edx,0               ; this is really a long addition
        add     ebx,1               ; restore the demoninator
        cmp     edx,ebx             ; check for overflow
        jae     short mdru386_5           ; (ae handles /0 case)
        div     ebx                 ; divide
        or      eax,eax             ; If sign is set, then overflow occured
        js      short mdru386_5           ; Overflow.
        or      ecx,ecx             ; put the sign on the result
        jns     short mdru386_6
        neg     eax

mdru386_6:
        EAXtoDXAX                   ; convert eax to dx:ax for 16 bit programs

        .286
cEnd 
        .386

mdru386_5:
        mov     eax,7FFFFFFFh       ; return the largest integer
        or      ecx,ecx             ; with the correct sign
        jns     mdru386_6
        not     eax
        jmp     mdru386_6

        .286

cProc   muldivru32_286,<PUBLIC,FAR,NODATA,NONWIN>,<di,si>
        ParmD  lNumber             
        ParmD  lNumerator
        ParmD  lDenominator
        LocalW wSign

cBegin

        mov     dx,lDenominator.hi      ; get the demoninator
        mov     si,dx	                ; SI holds the final sign 
        or      dx,dx                   ; ensure the denominator is positive
        jns     mdru286_1
        neg32   dx, lDenominator.lo
        mov     lDenominator.hi, dx


mdru286_1:
        mov     ax,lNumber.lo           ; get the long we are multiplying
        mov     dx,lNumber.hi           
        xor     si,dx	                ; make ECX reflect any sign change
        or      dx,dx                   ; ensure the long is positive
        jns     mdru286_2
        neg32   dx, ax

mdru286_2:
        mov     bx,lNumerator.lo        ; get the numerator
        mov     cx,lNumerator.hi        ; get the numerator
        xor     si,cx	                ; make ECX reflect any sign change
        or      cx,cx                   ; ensure the numerator is positive
        jns     mdru286_3
        neg32   cx, bx

mdru286_3:
        mov     wSign, si               ; save sign
        call    dmul                    ; multiply (result in dx:cx:bx:ax)
        mov     si, lDenominator.hi
        mov     di, lDenominator.lo
        sub     di, 1                   ; get demoninator - 1 
        sbb     si, 0                   ; to adjust for rounding
        add     ax, di                  ; adjust for possible rounding error
        adc     bx, si
        adc     cx, 0
        adc     dx, 0                   ; this is really a long addition

        add	di, 1                   ; restore the demoninator
        adc     si, 0

        cmp     dx, si                  ; begin overflow check (unsigned for div 0 check)
        ja      mdru286_5                 ; overflow
        jb      mdru286_7                 ; no overflow
        cmp     cx, di       
        jae     mdru286_5                 ; overflow

mdru286_7:
        call    qdiv                    ; DX:AX is quotient     
        or      dx,dx                   ; If sign is set, then overflow occured
        js      mdru286_5                 ; Overflow.
        mov     cx, wSign
        or      cx,cx                   ; put the sign on the result
        jns     mdru286_6
        neg32   dx,ax

mdru286_6:

cEnd

mdru286_5:
        mov     cx, wSign
        mov     ax, 0FFFFh              ; return the largest integer
        mov     dx, 7FFFh
        or      cx, cx                  ; with the correct sign
        jns     mdru286_6
        not     dx
        not     ax
        jmp     mdru286_6

cProc   muldivrd32,<PUBLIC,FAR,NODATA,NONWIN>,<>
;       ParmD  lNumber
;       ParmD  lNumerator
;       ParmD  lDenominator
cBegin	<nogen>
	mov	ax,__WinFlags
        test    ax,WF_CPU286+WF_CPU086+WF_CPU186
        jz      mdrd32_1
        jmp     short NEAR PTR muldivrd32_286
mdrd32_1:
        errn$   muldivrd32_386
cEnd    <nogen>

cProc   muldivrd32_386,<PUBLIC,FAR,NODATA,NONWIN>,<>
        ParmD   lNumber
        ParmD   lNumerator
        ParmD   lDenominator
cBegin
        .386


        mov     ebx,lDenominator    ; get the demoninator
        mov     ecx,ebx	            ; ECX holds the final sign in hiword
        or      ebx,ebx             ; ensure the denominator is positive
        jns     short mdrd386_1
        neg     ebx

mdrd386_1:
        mov     eax,lNumber         ; get the long we are multiplying
        xor     ecx,eax	            ; make ECX reflect any sign change
        or      eax,eax             ; ensure the long is positive
        jns     short mdrd386_2
        neg     eax

mdrd386_2:
        mov     edx,lNumerator      ; get the numerator
        xor     ecx,edx	            ; make ECX reflect any sign change
        or      edx,edx             ; ensure the numerator is positive
        jns     short mdrd386_3
        neg     edx

mdrd386_3:
        mul     edx                 ; multiply

        div     ebx                 ; divide
        or      eax,eax             ; If sign is set, then overflow occured
        js      short mdrd386_5         ; Overflow.
        or      ecx,ecx             ; put the sign on the result
        jns     short mdrd386_6
        neg     eax

mdrd386_6:
        EAXtoDXAX                   ; convert eax to dx:ax for 16 bit programs

        .286
cEnd 
        .386

mdrd386_5:
        mov     eax,7FFFFFFFh       ; return the largest integer
        or      ecx,ecx             ; with the correct sign
        jns     mdrd386_6
        not     eax
        jmp     mdrd386_6

        .286

cProc   muldivrd32_286,<PUBLIC,FAR,NODATA,NONWIN>,<di,si>
        ParmD  lNumber             
        ParmD  lNumerator
        ParmD  lDenominator
        LocalW wSign

cBegin

        mov     dx,lDenominator.hi      ; get the demoninator
        mov     si,dx	                ; SI holds the final sign 
        or      dx,dx                   ; ensure the denominator is positive
        jns     mdrd286_1
        neg32   dx, lDenominator.lo
        mov     lDenominator.hi, dx


mdrd286_1:
        mov     ax,lNumber.lo           ; get the long we are multiplying
        mov     dx,lNumber.hi           
        xor     si,dx	                ; make ECX reflect any sign change
        or      dx,dx                   ; ensure the long is positive
        jns     mdrd286_2
        neg32   dx, ax

mdrd286_2:
        mov     bx,lNumerator.lo        ; get the numerator
        mov     cx,lNumerator.hi        ; get the numerator
        xor     si,cx	                ; make ECX reflect any sign change
        or      cx,cx                   ; ensure the numerator is positive
        jns     mdrd286_3
        neg32   cx, bx

mdrd286_3:
        mov     wSign, si               ; save sign
        call    dmul                    ; multiply (result in dx:cx:bx:ax)
        mov     si, lDenominator.hi
        mov     di, lDenominator.lo

        cmp     dx, si                  ; begin overflow check (unsigned for div 0 check)
        ja      mdrd286_5                 ; overflow
        jb      mdrd286_7                 ; no overflow
        cmp     cx, di       
        jae     mdrd286_5                 ; overflow

mdrd286_7:
        call    qdiv                    ; DX:AX is quotient     
        or      dx,dx                   ; If sign is set, then overflow occured
        js      mdrd286_5                 ; Overflow.
        mov     cx, wSign
        or      cx,cx                   ; put the sign on the result
        jns     mdrd286_6
        neg32   dx,ax

mdrd286_6:

cEnd

mdrd286_5:
        mov     cx, wSign
        mov     ax, 0FFFFh              ; return the largest integer
        mov     dx, 7FFFh
        or      cx, cx                  ; with the correct sign
        jns     mdrd286_6
        not     dx
        not     ax
        jmp     mdrd286_6


;---------------------------Public-Routine------------------------------;
; idmul
;
; This is an extended precision multiply routine, intended to emulate
; 80386 imul instruction.
;
; Entry:
;       DX:AX = LONG
;       CX:BX = LONG
; Returns:
;       DX:CX:BX:AX = QUAD product
; Registers Destroyed:
;       none
; History:
;  Tue 26-Jan-1988 23:47:02  -by-  Charles Whitmer [chuckwh]
; Wrote it.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   idmul,<PUBLIC,NEAR>,<si,di>
        localQ  qTemp
cBegin

; put one argument in safe registers

        mov     si,dx
        mov     di,ax

; do the low order unsigned product

        mul     bx
        mov     qTemp.uq0,ax
        mov     qTemp.uq1,dx

; do the high order signed product

        mov     ax,si
        imul    cx
        mov     qTemp.uq2,ax
        mov     qTemp.uq3,dx

; do a mixed product

        mov     ax,si
        cwd
        and     dx,bx
        sub     qTemp.uq2,dx            ; adjust for sign bit
        sbb     qTemp.uq3,0
        mul     bx
        add     qTemp.uq1,ax
        adc     qTemp.uq2,dx
        adc     qTemp.uq3,0

; do the other mixed product

        mov     ax,cx
        cwd
	and	dx,di
        sub     qTemp.uq2,dx
        sbb     qTemp.uq3,0
        mul     di

; pick up the answer

        mov     bx,ax
        mov     cx,dx
        xor     dx,dx

        mov     ax,qTemp.uq0
        add     bx,qTemp.uq1
        adc     cx,qTemp.uq2
        adc     dx,qTemp.uq3
cEnd

;---------------------------Public-Routine------------------------------;
; dmul
;
; This is an extended precision multiply routine, intended to emulate
; 80386 mul instruction.
;
; Entry:
;       DX:AX = LONG
;       CX:BX = LONG
; Returns:
;       DX:CX:BX:AX = QUAD product
; Registers Destroyed:
;       none
; History:
;  Tue 02-Feb-1988 10:50:44  -by-  Charles Whitmer [chuckwh]
; Copied from idmul and modified.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   dmul,<PUBLIC,NEAR>,<si,di>
        localQ  qTemp
cBegin

; put one argument in safe registers

        mov     si,dx
        mov     di,ax

; do the low order product

        mul     bx
        mov     qTemp.uq0,ax
        mov     qTemp.uq1,dx

; do the high order product

        mov     ax,si
        mul     cx
        mov     qTemp.uq2,ax
        mov     qTemp.uq3,dx

; do a mixed product

        mov     ax,si
        mul     bx
        add     qTemp.uq1,ax
        adc     qTemp.uq2,dx
        adc     qTemp.uq3,0

; do the other mixed product

        mov     ax,cx
        mul     di

; pick up the answer

        mov     bx,ax
        mov     cx,dx
        xor     dx,dx
        mov     ax,qTemp.uq0
        add     bx,qTemp.uq1
        adc     cx,qTemp.uq2
        adc     dx,qTemp.uq3
cEnd

;---------------------------Public-Routine------------------------------;
; iqdiv
;
; This is an extended precision divide routine which is intended to
; emulate the 80386 64 bit/32 bit IDIV instruction.  We don't have the
; 32 bit registers to work with, but we pack the arguments and results
; into what registers we do have.  We will divide two signed numbers
; and return the quotient and remainder.  We will do INT 0 for overflow,
; just like the 80386 microcode.  This should ease conversion later.
;
; This routine just keeps track of the signs and calls qdiv to do the
; real work.
;
; Entry:
;       DX:CX:BX:AX = QUAD Numerator
;       SI:DI       = LONG Denominator
; Returns:
;       DX:AX = quotient
;       CX:BX = remainder
; Registers Destroyed:
;       DI,SI
; History:
;  Tue 26-Jan-1988 02:49:19  -by-  Charles Whitmer [chuckwh]
; Wrote it.
;-----------------------------------------------------------------------;

WIMP    equ     1

IQDIV_RESULT_SIGN       equ     1
IQDIV_REM_SIGN          equ     2

        assumes ds,nothing
        assumes es,nothing

cProc   iqdiv,<PUBLIC,NEAR>
        localB  flags
cBegin
        mov     flags,0

; take the absolute value of the denominator

        or      si,si
        jns     denominator_is_cool
        xor     flags,IQDIV_RESULT_SIGN
        neg     di
        adc     si,0
        neg     si
denominator_is_cool:

; take the absolute value of the denominator

        or      dx,dx
        jns     numerator_is_cool
        xor     flags,IQDIV_RESULT_SIGN + IQDIV_REM_SIGN
        not     ax
        not     bx
        not     cx
        not     dx
        add     ax,1
        adc     bx,0
        adc     cx,0
        adc     dx,0
numerator_is_cool:

; do the unsigned division

        call    qdiv
ifdef WIMP
        jo      iqdiv_exit
endif

; check for overflow

        or      dx,dx
        jns     have_a_bit_to_spare
ifdef WIMP
        mov     ax,8000h
        dec     ah
        jmp     short iqdiv_exit
else
        int     0                       ; You're toast, Jack!
endif
have_a_bit_to_spare:

; negate the result, if required

        test    flags,IQDIV_RESULT_SIGN
        jz      result_is_done
        neg     ax
        adc     dx,0
        neg     dx
result_is_done:

; negate the remainder, if required

        test    flags,IQDIV_REM_SIGN
        jz      remainder_is_done
        neg     bx
        adc     cx,0
        neg     cx
remainder_is_done:
iqdiv_exit:
cEnd

;---------------------------Public-Routine------------------------------;
; qdiv
;
; This is an extended precision divide routine which is intended to
; emulate the 80386 64 bit/32 bit DIV instruction.  We don't have the
; 32 bit registers to work with, but we pack the arguments and results
; into what registers we do have.  We will divide two unsigned numbers
; and return the quotient and remainder.  We will do INT 0 for overflow,
; just like the 80386 microcode.  This should ease conversion later.
;
; Entry:
;       DX:CX:BX:AX = UQUAD Numerator
;       SI:DI       = ULONG Denominator
; Returns:
;       DX:AX = quotient
;       CX:BX = remainder
; Registers Destroyed:
;       none
; History:
;  Tue 26-Jan-1988 00:02:09  -by-  Charles Whitmer [chuckwh]
; Wrote it.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   qdiv,<PUBLIC,NEAR>,<si,di>
        localQ  uqNumerator
        localD  ulDenominator
        localD  ulQuotient
        localW  cShift
cBegin

; stuff the quad word into local memory

        mov     uqNumerator.uq0,ax
        mov     uqNumerator.uq1,bx
        mov     uqNumerator.uq2,cx
        mov     uqNumerator.uq3,dx


; check for overflow

qdiv_restart:
        cmp     si,dx
        ja      qdiv_no_overflow
        jb      qdiv_overflow
        cmp     di,cx
        ja      qdiv_no_overflow
qdiv_overflow:
ifdef WIMP
        mov     ax,8000h
        dec     ah
        jmp     qdiv_exit
else
        int     0                       ; You're toast, Jack!
        jmp     qdiv_restart
endif
qdiv_no_overflow:

; check for a zero Numerator

        or      ax,bx
        or      ax,cx
        or      ax,dx
        jz      qdiv_exit_relay         ; quotient = remainder = 0

; handle the special case when the denominator lives in the low word

        or      si,si
        jnz     not_that_special

; calculate (DX=0):CX:BX:uqNumerator.uq0 / (SI=0):DI

        cmp     di,1                    ; separate out the trivial case
        jz      div_by_one
        xchg    dx,cx                   ; CX = remainder.hi = 0
        mov     ax,bx
        div     di
        mov     bx,ax                   ; BX = quotient.hi
        mov     ax,uqNumerator.uq0
        div     di                      ; AX = quotient.lo
        xchg    bx,dx                   ; DX = quotient.hi, BX = remainder.lo
ifdef WIMP
        or      ax,ax           ; clear OF
endif
qdiv_exit_relay:
        jmp     qdiv_exit

; calculate (DX=0):(CX=0):BX:uqNumerator.uq0 / (SI=0):(DI=1)

div_by_one:
        xchg    dx,bx                   ; DX = quotient.hi, BX = remainder.lo = 0
        mov     ax,uqNumerator.uq0      ; AX = quotient.lo
        jmp     qdiv_exit
not_that_special:

; handle the special case when the denominator lives in the high word

        or      di,di
        jnz     not_this_special_either

; calculate DX:CX:BX:uqNumerator.uq0 / SI:(DI=0)

        cmp     si,1                    ; separate out the trivial case
        jz      div_by_10000h
        mov     ax,cx
        div     si
        mov     cx,ax                   ; CX = quotient.hi
        mov     ax,bx
        div     si                      ; AX = quotient.lo
        xchg    cx,dx                   ; DX = quotient.hi, CX = remainder.hi
        mov     bx,uqNumerator.uq0      ; BX = remainder.lo
ifdef WIMP
        or      ax,ax           ; clear OF
endif
        jmp     qdiv_exit

; calculate (DX=0):CX:BX:uqNumerator.uq0 / (SI=1):(DI=0)

div_by_10000h:
        xchg    cx,dx                   ; DX = quotient.hi, CX = remainder.hi = 0
        mov     ax,bx                   ; AX = quotient.lo
        mov     bx,uqNumerator.uq0      ; BX = remainder.lo
        jmp     qdiv_exit
not_this_special_either:

; normalize the denominator

        mov     dx,si
        mov     ax,di
        call    ulNormalize             ; DX:AX = normalized denominator
        mov     cShift,cx               ; CX < 16
        mov     ulDenominator.lo,ax
        mov     ulDenominator.hi,dx


; shift the Numerator by the same amount

        jcxz    numerator_is_shifted
        mov     si,-1
        shl     si,cl
        not     si                      ; SI = mask
        mov     bx,uqNumerator.uq3
        shl     bx,cl
        mov     ax,uqNumerator.uq2
        rol     ax,cl
        mov     di,si
        and     di,ax
        or      bx,di
        mov     uqNumerator.uq3,bx
        xor     ax,di
        mov     bx,uqNumerator.uq1
        rol     bx,cl
        mov     di,si
        and     di,bx
        or      ax,di
        mov     uqNumerator.uq2,ax
        xor     bx,di
        mov     ax,uqNumerator.uq0
        rol     ax,cl
        mov     di,si
        and     di,ax
        or      bx,di
        mov     uqNumerator.uq1,bx
        xor     ax,di
        mov     uqNumerator.uq0,ax
numerator_is_shifted:

; set up registers for division

        mov     dx,uqNumerator.uq3
        mov     ax,uqNumerator.uq2
        mov     di,uqNumerator.uq1
        mov     cx,ulDenominator.hi
        mov     bx,ulDenominator.lo

; check for case when Denominator has only 16 bits

        or      bx,bx
        jnz     must_do_long_division
        div     cx
        mov     si,ax
        mov     ax,uqNumerator.uq1
        div     cx
        xchg    si,dx                   ; DX:AX = quotient
        mov     di,uqNumerator.uq0      ; SI:DI = remainder (shifted)
        jmp     short unshift_remainder
must_do_long_division:

; do the long division, part IZ@NL@%

        cmp     dx,cx                   ; we only know that DX:AX < CX:BX!
        jb      first_division_is_safe
        mov     ulQuotient.hi,0         ; i.e. 10000h, our guess is too big
        mov     si,ax
        sub     si,bx                   ; ... remainder is negative
        jmp     short first_adjuster
first_division_is_safe:
        div     cx
        mov     ulQuotient.hi,ax
        mov     si,dx
        mul     bx                      ; fix remainder for low order term
        sub     di,ax
        sbb     si,dx
        jnc     first_adjuster_done     ; The remainder is UNSIGNED!  We have
first_adjuster:                         ; to use the carry flag to keep track
        dec     ulQuotient.hi           ; of the sign.  The adjuster loop
        add     di,bx                   ; watches for a change to the carry
        adc     si,cx                   ; flag which would indicate a sign
        jnc     first_adjuster          ; change IF we had more bits to keep
first_adjuster_done:                    ; a sign in.

; do the long division, part II

        mov     dx,si
        mov     ax,di
        mov     di,uqNumerator.uq0
        cmp     dx,cx                   ; we only know that DX:AX < CX:BX!
        jb      second_division_is_safe
        mov     ulQuotient.lo,0         ; i.e. 10000h, our guess is too big
        mov     si,ax
        sub     si,bx                   ; ... remainder is negative
        jmp     short second_adjuster
second_division_is_safe:
        div     cx
        mov     ulQuotient.lo,ax
        mov     si,dx
        mul     bx                      ; fix remainder for low order term
        sub     di,ax
        sbb     si,dx
        jnc     second_adjuster_done
second_adjuster:
        dec     ulQuotient.lo
        add     di,bx
        adc     si,cx
        jnc     second_adjuster
second_adjuster_done:
        mov     ax,ulQuotient.lo
        mov     dx,ulQuotient.hi

; unshift the remainder in SI:DI

unshift_remainder:
        mov     cx,cShift
        jcxz    remainder_unshifted
        mov     bx,-1
        shr     bx,cl
        not     bx
        shr     di,cl
        ror     si,cl
        and     bx,si
        or      di,bx
        xor     si,bx
remainder_unshifted:
        mov     cx,si
        mov     bx,di
ifdef WIMP
        or      ax,ax           ; clear OF
endif
qdiv_exit:
cEnd

;---------------------------Public-Routine------------------------------;
; ulNormalize
;
; Normalizes a ULONG so that the highest order bit is 1.  Returns the
; number of shifts done.  Also returns ZF=1 if the ULONG was zero.
;
; Entry:
;       DX:AX = ULONG
; Returns:
;       DX:AX = normalized ULONG
;       CX    = shift count
;       ZF    = 1 if the ULONG is zero, 0 otherwise
; Registers Destroyed:
;       none
; History:
;  Mon 25-Jan-1988 22:07:03  -by-  Charles Whitmer [chuckwh]
; Wrote it.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   ulNormalize,<PUBLIC,NEAR>
cBegin

; shift by words

        xor     cx,cx
        or      dx,dx
        js      ulNormalize_exit
        jnz     top_word_ok
        xchg    ax,dx
        or      dx,dx
        jz      ulNormalize_exit        ; the zero exit
        mov     cl,16
        js      ulNormalize_exit
top_word_ok:

; shift by bytes

        or      dh,dh
        jnz     top_byte_ok
        xchg    dh,dl
        xchg    dl,ah
        xchg    ah,al
        add     cl,8
        or      dh,dh
        js      ulNormalize_exit
top_byte_ok:

; do the rest by bits

        inc     cx
        add     ax,ax
        adc     dx,dx
        js      ulNormalize_exit
        inc     cx
        add     ax,ax
        adc     dx,dx
        js      ulNormalize_exit
        inc     cx
        add     ax,ax
        adc     dx,dx
        js      ulNormalize_exit
        inc     cx
        add     ax,ax
        adc     dx,dx
        js      ulNormalize_exit
        inc     cx
        add     ax,ax
        adc     dx,dx
        js      ulNormalize_exit
        inc     cx
        add     ax,ax
        adc     dx,dx
        js      ulNormalize_exit
        inc     cx
        add     ax,ax
        adc     dx,dx
ulNormalize_exit:
cEnd

sEnd   CodeSeg

       end
