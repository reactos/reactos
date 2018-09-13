;---------------------------------------------------------------------------
;
;  Module:   cpuspeed.asm
;
;  Purpose:
;     Computes the CPU speed using the 8254 and the NOP instruction.
;
;  Development Team:
;     Bryan A. Woodruff
;
;  History:   Date       Author      Comment
;              8/13/92   BryanW      Wrote it
;
;---------------------------------------------------------------------------
;
;  Written by Microsoft Product Support Service, Windows Developer Support.
;  Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
;
;---------------------------------------------------------------------------

NAME CPUSPEED

.286p

;-------------------------------------------------------------------------
.xlist                                  ; suspend listing
?PLM=1                                  ; support Pascal calling
?WIN=1                                  ; support Windows
memM=1                                  ; medium model
include CMacros.Inc
include Windows.Inc
.list
;-------------------------------------------------------------------------


;-------------------------------------------------------------------------
; local definitions
;-------------------------------------------------------------------------

Timer1_Ctl_Port equ     43h             
Timer1_Ctr2     equ     42h
System_Port_B   equ     61h

                                        ; SC1 SC0 RW1 RW0 M2 M1 M0 BCD
Ctr2_Access     equ     10110100b       ; 1   0   1   1   0  1  0  0

                                        ; Counter 2 (Speaker tone)
                                        ; Read/Write LSB first, then MSB
                                        ; Rate generator (count down)
                                        ; Binary counter

                                        ; SC1 SC0 RW1 RW0 D3 D2 D1 D0
Ctr2_Latch      equ     10000000b       ; 1   0   0   0   X  X  X  X

                                        ; Counter 2 (Speaker tone)
                                        ; Counter latch command


assumes DS, NOTHING

;-------------------------------------------------------------------------
; segment definition
;-------------------------------------------------------------------------

createSeg CPUSPEED, CPUSPEED, PARA, PUBLIC, CODE

sBegin  CPUSPEED
assumes CS, CPUSPEED

;------------------------------------------------------------------------
;  DWORD ComputeCPUSpeed, <PUBLIC, FAR>
;
;  Description:
;
;
;  Parameters:
;
;
;
;  History:   Date       Author      Comment
;              8/13/92   BryanW      Wrote it.
;
;------------------------------------------------------------------------

cProc   ComputeCPUSpeed, <PUBLIC, FAR>
LocalW  wTicks
cBegin
        mov     dx, System_Port_B                       ; turn off speaker
        in      al, dx                                  ; & timer 2...
        and     al, 0fch
        out     dx, al

        mov     dx, Timer1_Ctl_Port
        mov     al, Ctr2_Access
        out     dx, al

        mov     dx, Timer1_Ctr2
        xor     al, al
        out     dx, al                                  ; reset counter
        nop
        out     dx, al

        pushf
        cli

        mov     dx, System_Port_B                       ; enable input
        in      al, dx                                  ; to timer 1 / ctr 2
        mov     bl, al
        or      al, 1
        out     dx, al

;       db      250 dup (0f8h)                          ; clc
        dw      250 dup (0ad4h)                         ; aam

        mov     al, bl
        out     dx, al

        pop     ax
        cmp     ah, 2
        jz      SHORT  @F
        sti

@@:
        mov     dx, Timer1_Ctr2
        in      al, dx                                  ; al <- LSB
        mov     ah, al
        nop
        in      al, dx                                  ; al <- MSB
        xchg    al, ah                                  ; order AX
        neg     ax
        mov     dx, 250 * 15
cEnd

sEnd    CPUSPEED

end

;---------------------------------------------------------------------------
;  End of File: cpuspeed.asm
;---------------------------------------------------------------------------

