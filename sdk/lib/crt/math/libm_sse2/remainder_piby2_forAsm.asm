;
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
; An implementation of the remainder by pi/2 function
; This is a service routine for use by trig functions coded in asm
;
; On input,
;   xmm0 = x;
; On ouput
;   xmm0 = r
;   xmm1 = rr
;   xmm2 = region

.const
ALIGN 16
L__piby2_part3_piby2_lead DQ 03ff921fb54442d18h, 03c91a62633145c06h
L__piby2_part1            DQ 03ff921fb50000000h, 03ff921fb50000000h 
L__piby2_part2            DQ 03e5110b460000000h, 03e5110b460000000h
;; constants for CW reduction
L_piby2_1      DQ 03FF921FB54400000h, 03FF921FB54400000h
L_piby2_2      DQ 03DD0B4611A600000h, 03DD0B4611A600000h
L_piby2_3      DQ 03BA3198A2E000000h, 03BA3198A2E000000h
L_piby2_1tail  DQ 03DD0B4611A626331h, 03DD0B4611A626331h
L_piby2_2tail  DQ 03BA3198A2E037073h, 03BA3198A2E037073h
L_piby2_3tail  DQ 0397B839A252049C1h, 0397B839A252049C1h
L_twobypi      DQ 03FE45F306DC9C883h, 03FE45F306DC9C883h
L_point_five   DQ 03FE0000000000000h, 03FE0000000000000h
L_int_three    DQ 00000000000000003h, 00000000000000003h
L_inf_mask_64  DQ 07FF0000000000000h, 07FF0000000000000h
L_signbit      DQ 08000000000000000h, 08000000000000000h
L_int_1        DQ 00000000000000001h, 00000000000000001h
L_int_15       DQ 0000000000000000Fh
L_int_48       DQ 00000000000000030h
L_3pio4        DQ 04002D97C7F3321D2h
L_5pio4        DQ 0400F6A7A2955385Eh
L_7pio4        DQ 04015FDBBE9BBA775h
L_9pio4        DQ 0401c463abeccb2bbh
ALIGN 16
L__2_by_pi_bits DB 224, 241,  27, 193,  12,  88,  33, 116
                DB  53, 126, 196, 126, 237, 175, 169,  75
                DB  74,  41, 222, 231,  28, 244, 236, 197
                DB 151, 175,  31, 235, 158, 212, 181, 168
                DB 127, 121, 154, 253,  24,  61, 221,  38
                DB  44, 159,  60, 251, 217, 180, 125, 180
                DB  41, 104,  45,  70, 188, 188,  63,  96
                DB  22, 120, 255,  95, 226, 127, 236, 160
                DB 228, 247,  46, 126,  17, 114, 210, 231
                DB  76,  13, 230,  88,  71, 230,   4, 249
                DB 125, 209, 154, 192, 113, 166,  19,  18
                DB 237, 186, 212, 215,   8, 162, 251, 156
                DB 166, 196, 114, 172, 119, 248, 115,  72
                DB  70,  39, 168, 187,  36,  25, 128,  75
                DB  55,   9, 233, 184, 145, 220, 134,  21
                DB 239, 122, 175, 142,  69, 249,   7,  65
                DB  14, 241, 100,  86, 138, 109,   3, 119
                DB 211, 212,  71,  95, 157, 240, 167,  84
                DB  16,  57, 185,  13, 230, 139,   2,   0
                DB   0,   0,   0,   0,   0,   0


; local storage offsets
region          EQU 000h
stack_size      EQU 018h
sstack_size     EQU 000h   ; no stack for fsname

include fm.inc

fname TEXTEQU <__remainder_piby2_forAsm>
fsname TEXTEQU <__remainder_piby2_cw_forAsm>


.code

; xmm0l has |x|
PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    .ENDPROLOG

    ; This function is not using rdx, r8, and r9 as pointers;
    ; all returns are in registers

    ; get the unbiased exponent and the mantissa part of x
    lea       r9,L__2_by_pi_bits
 
 ;xexp = (x >> 52) - 1023
    movd      r11,xmm0
    mov       rcx,r11 
    shr       r11,52
    sub       r11,1023                 ; r11 <-- xexp = exponent of input x 

    ;calculate the last byte from which to start multiplication
    ;last = 134 - (xexp >> 3) 
    mov       r10,r11
    shr       r10,3
    sub       r10,134                  ; r10 <-- -last
    neg       r10                      ; r10 <-- last

    ; load 64 bits of 2_by_pi
    mov       rax,[r9 + r10]
 
    ; mantissa of x = ((x << 12) >> 12) | implied bit
    shl       rcx,12
    shr       rcx,12                   ; rcx <-- mantissa part of input x 
    bts       rcx,52                   ; add the implied bit as well 

    ; load next 128 bits of 2_by_pi 
    add       r10,8 ;increment to next 8 bytes of 2_by_pi
    movdqu    xmm0,[r9 + r10] 

    ; do three 64-bit multiplications with mant of x 
    mul rcx
    mov       r8,rax                   ; r8 <-- last 64 bits of mul = res1[2] 
    mov       r10,rdx                  ; r10 <-- carry
    movd      rax,xmm0
    mul       rcx
    ; resexp = xexp & 7 
    and       r11,7                    ; r11 <-- resexp = xexp & 7 = last 3 bits
    psrldq    xmm0,8 
    add       rax,r10                  ; add the previous carry
    adc       rdx,0
    mov       r9,rax                   ; r9 <-- next 64 bits of mul = res1[1]
    mov       r10,rdx                  ; r10 <-- carry
    movd      rax,xmm0
    mul       rcx
    add       r10,rax                  ; r10 <-- most sig. 64 bits = res1[0]
    ; find the region 
    ; last three bits ltb = most sig bits >> (54 - resexp));
    ;   decimal point in last 18 bits ==> 8 lsb's in first 64 bits
    ;   and 8 msb's in next 64 bits
    ; point_five = ltb & 01h;
    ; region = ((ltb >> 1) + point_five) & 3;  
    mov       rcx,54
    mov       rax,r10
    sub       rcx,r11
    xor       rdx,rdx                  ; rdx <-- sign of x 
    shr       rax,cl 
    jnc       L__no_point_five
    ; if there is carry then negate the result of multiplication
    not       r10
    not       r9
    not       r8
    mov       rdx,08000000000000000h

ALIGN  16 
L__no_point_five:
    adc       rax,0
    and       rax,3                    ; rax now has region
    mov       QWORD PTR [region+rsp],rax

    ; calculate the number of integer bits and zero them out
    mov       rcx,r11 
    add       rcx,10                   ; rcx = no. of integer bits
    shl       r10,cl
    shr       r10,cl                   ; r10 contains only mant bits
    sub       rcx,64                   ; form the exponent
    mov       r11,rcx
 
 ;find the highest set bit
    bsr       rcx,r10
    jnz       L__form_mantissa
    mov       r10,r9
    mov       r9,r8
    mov       r8,0
    bsr       rcx,r10                  ; rcx = hsb
    sub       r11,64
 
 
ALIGN  16 
L__form_mantissa:
    add       r11,rcx                  ; for exp of x
    sub       rcx,52                   ; rcx = no. of bits to shift in r10 
    cmp       rcx,0
    jl        L__hsb_below_52
    je        L__form_numbers
    ; hsb above 52
    mov       r8,r10                   ; previous contents of r8 not required
    shr       r10,cl                   ; r10 = mantissa of x with hsb at 52
    shr       r9,cl                    ; make space for bits from r10
    sub       rcx,64
    neg       rcx
    ; rcx <-- no of bits to shift r10 to move those bits to r9
    shl       r8,cl
    or        r9,r8                    ; r9 = mantissa bits of xx 
    jmp       L__form_numbers
 
ALIGN  16 
L__hsb_below_52:
    neg       rcx
    mov       rax,r9
    shl       r10,cl
    shl       r9,cl
    sub       rcx,64
    neg       rcx
    shr       rax,cl
    or        r10,rax
    shr       r8,cl
    or        r9,r8 
 
ALIGN  16
L__form_numbers:
    add       r11,1023
    btr       r10,52                   ; remove the implicit bit
    mov       rcx,r11
    or        r10,rdx                  ; put the sign 
    shl       rcx,52
    or        r10,rcx                  ; r10 <-- x
 
    movd      xmm0,r10                 ; xmm0 <-- x
    movdqa    xmm1,xmm0                ; xmm1 <-- x
    psrlq     xmm1,27
    psllq     xmm1,27                  ; xmm1 <-- hx
    movdqa    xmm2,xmm0                ; xmm2 <-- x 
    subsd     xmm2,xmm1                ; xmm2 <-- tx
    movlhps   xmm0,xmm0                ; xmm0 <-- x,x
    movlhps   xmm2,xmm1                ; xmm2 <-- hx,tx

    movdqa    xmm1,XMMWORD PTR L__piby2_part3_piby2_lead 
    movdqa    xmm3,XMMWORD PTR L__piby2_part1
    movdqa    xmm4,XMMWORD PTR L__piby2_part2

    ; form xx
    xor       rcx,rcx
    bsr       rcx,r9
    sub       rcx,64                   ; to shift the implicit bit as well
    neg       rcx
    shl       r9,cl
    shr       r9,12
    add       rcx,52
    sub       r11,rcx
    shl       r11,52
    or        r9,rdx
    or        r9,r11
    movd      xmm5,r9                  ; xmm5 <-- xx 
 
    mulpd     xmm0,xmm1 ; xmm0 <-- piby2_part3 * x,piby2_lead * x = c
    mulpd     xmm5,xmm1 ; xmm5 <-- piby2_lead * xx
    mulpd     xmm3,xmm2 ; xmm3 <-- piby2_part1 * hx,piby2_part1 * tx
    mulpd     xmm4,xmm2 ; xmm4 <-- piby2_part2 * hx,piby2_part2 * tx 
 
    ; cc = (piby2_part1 * hx - c) + (piby2_part1 * tx) +
    ;   (piby2_part2 * hx) + (piby2_part2 * tx) + 
    ;   (piby2_lead * xx + piby2_part3 * x)
    movhlps   xmm1,xmm3 ; xmm1 = piby2_part1 * hx
    movhlps   xmm2,xmm4 ; xmm2 = piby2_part2 * hx 
    subsd     xmm1,xmm0 ; xmm1 = (piby2_part1 * hx - c)
    addsd     xmm1,xmm3 ; xmm1 = (piby2_part1 * hx - c) + (piby2_part1 * tx)
    movhlps   xmm3,xmm0 ; xmm3 = piby2_part3 * x
    addsd     xmm1,xmm2
    ; xmm1 = (piby2_part1 * hx - c) + (piby2_part1 * tx) + (piby2_part2 * hx)
    addsd     xmm3,xmm5 ; xmm3 = (piby2_lead * xx + piby2_part3 * x)
    addsd     xmm1,xmm4
    ; xmm1 = (piby2_part1 * hx - c) + (piby2_part1 * tx) + 
    ;    (piby2_part2 * hx) + (piby2_part2 * tx)
    addsd     xmm1,xmm3                ; xmm1 = cc
 
    ; xmm0 <-- c, xmm1 <-- cc
    ; r = c + cc
    ; rr = (c - r) + cc

    movdqa    xmm2,xmm0                ; xmm2 <-- copy of c
    addsd     xmm0,xmm1                ; xmm0 <-- r = c + cc
    subsd     xmm2,xmm0                ; xmm2 <-- c - r
    addsd     xmm1,xmm2                ; xmm1 <-- rr = cc + (c - r)
    mov       rax, QWORD PTR[region+rsp] ; rax <-- region

    StackDeallocate stack_size
    ret 
 
fname        endp

; NOTE: If this is not going to be used, should probably remove it. - WAT
ALIGN 16
PUBLIC fsname
fsname PROC FRAME
    StackAllocate sstack_size
    .ENDPROLOG

; xmm0l has |x|
; r9 also has |x|
; ASSUMPTION: if we call this function, |x| > pi/4

    xor       r8d,r8d
    cmp       r9, QWORD PTR L_5pio4
    ja        Lax_gt_5pio4
    cmp       r9, QWORD PTR L_3pio4
    seta      r8b
    inc       r8d
    jmp       Lstage_npi2
Lax_gt_5pio4:
    cmp       r9, QWORD PTR L_9pio4
    ja        Lnpi2_full_computation
    cmp       r9, QWORD PTR L_7pio4
    seta      r8b
    add       r8d,3
Lstage_npi2:
    movd      xmm2, r8d
    cvtdq2pd  xmm4, xmm2
    jmp       Lnpi2_known

Lnpi2_full_computation:
;   movapd    xmm1, L_twobypi
;   movapd    xmm3, L_point_five
    movapd    xmm5,xmm0
;   mulsd     xmm5,xmm1
;   addsd     xmm5,xmm3                   ; xmm5 <-- |x|*2/pi + .5
    mulsd     xmm5, L_twobypi
    addsd     xmm5, L_point_five

    cvttpd2dq xmm5,xmm5                   ; xmm5 < npi2 = int part
    movapd    xmm2,xmm5
    andpd     xmm2,L_int_three
    cvtdq2pd  xmm4,xmm5

Lnpi2_known:
    movapd    xmm5,xmm4
    mulsd     xmm5,QWORD PTR L_piby2_1    ; xmm5 <-- npi2*piby2_1
    xorpd     xmm5,L_signbit              ; xmm5 <-- -npi2*piby2_1
    addpd     xmm5,xmm0                   ; xmm5 <-- rhead = x - npi2*piby2_1
    movapd    xmm3,xmm4
    mulsd     xmm3,QWORD PTR L_piby2_1tail ; xmm3 <-- rtail = npi2*piby2_1tail

    ; If x is nearly a multiple of pi/2, rhead will be small compared to |x|
    ; we check this by checking exponent difference.

    ; Note that both the unbiased exponents are positive, and that of rhead
    ; must be <= that of |x|
    movapd    xmm1,xmm5                   ; xmm1l <-- rhead
    subpd     xmm1,xmm3                   ; xmm1l <-- r = rhead - rtail
    andpd     xmm1,L_inf_mask_64
    psubq     xmm0,xmm1                   ; xmm0 <-- |x| - r
    psrlq     xmm0,52
    comisd    xmm0,L_int_15

;   movd      rax, xmm5                   ; really a movq
;   shr       rax, 52
;   shr       rdx, 52                     ; get exponent of |x| (no and needed)
;   sub       rdx, rax
;   cmp       rdx, 15
    jbe       Lcw_get_r_rr

    ; here expdiff > 15, so x is nearly a multiple of pi/2 and things are hard
    ; we use another piece of pi/2 in the reduction

    movapd    xmm1,xmm5
    movapd    xmm3,xmm4
    mulsd     xmm3,QWORD PTR L_piby2_2 ; xmm3 <--- rtail = npi2*piby2_2
    subsd     xmm5,xmm3 ; xmm5 <-- rhead = t - rtail

    ; now rtail = npi2*piby2_2tail - ((t-rhead) - rtail)
    subsd     xmm1,xmm5
    subsd     xmm1,xmm3
    movapd    xmm3,xmm4
    mulsd     xmm3,QWORD PTR L_piby2_2tail
    subsd     xmm3,xmm1 ; xmm3 <-- rtail

    comisd    xmm0,L_int_48
;   cmp       rdx, 48
    jbe       Lcw_get_r_rr

    ; here expdiff > 48, so x is REALLY close to a multiple of pi/2
    ; and we use yet another piece of pi/2 in the reduction

    movapd    xmm0,xmm5 ; xmm0 <-- t = rhead
    movapd    xmm3,xmm4
    mulsd     xmm3,QWORD PTR L_piby2_3 ; xmm3 <-- rtail = npi2 * piby2_3
    movapd    xmm5,xmm0
    subsd     xmm5,xmm3 ; xmm5 <-- rhead = t - rtail

    ; now rtail = npi2 * piby2_3tail - ((t - rhead) - rtail)
    movapd    xmm1,xmm0
    subsd     xmm1,xmm5
    subsd     xmm1,xmm3
    movapd    xmm3,xmm4
    mulsd     xmm3,QWORD PTR L_piby2_3tail
    subsd     xmm3,xmm1 ; xmm3 <-- rtail

Lcw_get_r_rr:
    ; We have a satisfactory rhead in xmm5 and rtail in xmm3
    ; We now produce r in xmm0 and rr in xmm1, where the actual reduced argument
    ; is the sum of r and rr, and rr is insignificant
    ; with respect to r under addition (i.e., r + rr == r).
    movapd    xmm0,xmm5 ; xmm0 <-- rhead
    subsd     xmm0,xmm3 ; xmm0 <-- r = rhead - rtail
    movapd    xmm1,xmm5 ; xmm1 <-- rhead
    subsd     xmm1,xmm0 ; xmm1 <-- (rhead - r)
    subsd     xmm1,xmm3 ; xmm1 <-- rr = (rhead - r) - rtail
    movd      rax,xmm2  ; rax <-- region
    StackDeallocate sstack_size
    ret
fsname        endp

END
