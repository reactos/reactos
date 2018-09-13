        page    ,132
;---------------------------Module-Header-------------------------------;
; Module Name: MULDIV32
;
; Copyright (c) 1987  Microsoft Corporation
;-----------------------------------------------------------------------;

?WIN	= 0
?PLM	= 1
?NODATA = 0

        .xlist
        include cmacros.inc
        .list

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        .386
        assumes cs,CodeSeg
	assumes ds,nothing
        assumes es,nothing

;---------------------------Public-Routine------------------------------;
; muldiv32
;
; multiples two 32 bit values and then divides the result by a third
; 32 bit value with full 64 bit presision
;
; lResult = (lNumber * lNumerator) / lDenominator
;
; Entry:
;       lNumber = number to multiply by nNumerator
;       lNumerator = number to multiply by nNumber
;       lDenominator = number to divide the multiplication result by.
;   
; Returns:
;       EAX and DX:AX = result of multiplication and division.
; Error Returns:
;       none
; Registers Preserved:
;       DS,ES,ESI,EDI
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   muldiv32,<PUBLIC,FAR,PASCAL>,<>
;       ParmD  ulNumber
;       ParmD  ulNumerator
;       ParmD  ulDenominator
cBegin
        pop     edx     ; return addr
        pop     ebx     ; ulDenominator
        pop     ecx     ; ulNumerator
        pop     eax     ; ulNumber
        push    edx     ; put back return addr

        imul    ecx     ; edx:eax = (lNumber * lNumerator)
        idiv    ebx     ; eax     = (lNumber * lNumerator) / lDenominator

        shld    edx,eax,16      ; move HIWORD(eax) to dx
cEnd

sEnd   CodeSeg

       end
