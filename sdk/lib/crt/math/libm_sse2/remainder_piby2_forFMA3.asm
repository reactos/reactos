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
; An implementation of the remainder by pi/2 function using fma3
; This is a service routine for use by trig functions coded in asm that use fma3
;
; On input,
;   xmm0 = x;
; On ouput
;   xmm0 = r
;   xmm1 = rr
;   rax = region

.const
ALIGN 16
L_piby2_lead   DQ 03ff921fb54442d18h, 03ff921fb54442d18h
L_fff800       DQ 0fffffffff8000000h, 0fffffffff8000000h 
L_piby2_part1  DQ 03ff921fb50000000h, 03ff921fb50000000h 
L_piby2_part2  DQ 03e5110b460000000h, 03e5110b460000000h
L_piby2_part3  DQ 03c91a62633145c06h, 03c91a62633145c06h
L_piby2_1      DQ 03FF921FB54400000h, 03FF921FB54400000h
L_piby2_2      DQ 03DD0B4611A600000h, 03DD0B4611A600000h
L_piby2_3      DQ 03BA3198A2E000000h, 03BA3198A2E000000h
L_piby2_1tail  DQ 03DD0B4611A626331h, 03DD0B4611A626331h
L_piby2_2tail  DQ 03BA3198A2E037073h, 03BA3198A2E037073h
L_piby2_3tail  DQ 0397B839A252049C1h, 0397B839A252049C1h
L_sign_mask    DQ 07FFFFFFFFFFFFFFFh, 07FFFFFFFFFFFFFFFh
L_twobypi      DQ 03FE45F306DC9C883h, 03FE45F306DC9C883h
L_point_five   DQ 03FE0000000000000h, 03FE0000000000000h
L_int_three    DQ 00000000000000003h, 00000000000000003h
L_inf_mask_64  DQ 07FF0000000000000h, 07FF0000000000000h
L_signbit      DQ 08000000000000000h, 08000000000000000h
;; constants for BDL reduction
L_r            DQ 03FE45F306DC9C883h, 03FE45F306DC9C883h  ; 2/pi
L_xc1          DQ 03FF921FB54442D18H, 03FF921FB54442D18h  ; pi/2 (L_piby2_lead)
L_xc2          DQ 03C91A62633145C00H, 03C91A62633145C00h  ; pi/2 part 2
L_xc3          DQ 0397B839A252049C0H, 0397B839A252049C0h  ; pi/2 part 3
; sigma is 3*2^(p-n-2) where n is 0 and p is 53.
L_sigma        DQ 04338000000000000h, 04338000000000000h  ; 6755399441055744.

EXTRN __L_2_by_pi_bits:BYTE

region      EQU        020h
stack_size  EQU        038h

include fm.inc

fname TEXTEQU  <__remainder_piby2_fma3>
fbname TEXTEQU <__remainder_piby2_fma3_bdl>

.code

PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    .ENDPROLOG

    ; This function is not using rdx, r8, and r9 as pointers;
    ; all returns are in registers

    ; get the unbiased exponent and the mantissa part of x
    lea          r9,__L_2_by_pi_bits
 
    ; xexp = (x >> 52) - 1023
    vmovq        r11,xmm0
    mov          rcx,r11 
    shr          r11,52
    sub          r11,1023                 ; r11 <-- xexp = exponent of input x 

    ; calculate the last byte from which to start multiplication
    ; last = 134 - (xexp >> 3) 
    mov          r10,r11
    shr          r10,3
    sub          r10,134                  ; r10 <-- -last
    neg          r10                      ; r10 <-- last

    ; load 64 bits of 2_by_pi
    mov          rax,[r9 + r10]
 
    ; mantissa of x = ((x << 12) >> 12) | implied bit
    shl          rcx,12
    shr          rcx,12                   ; rcx <-- mantissa part of input x 
    bts          rcx,52                   ; add the implied bit as well 

    ; load next 128 bits of 2_by_pi 
    add          r10,8                    ; increment to next 8 bytes of 2_by_pi
    vmovdqu      xmm0,XMMWORD PTR[r9 + r10] 

    ; do three 64-bit multiplications with mant of x 
    mul          rcx
    mov          r8,rax                   ; r8 <-- last 64 bits of mul = res1[2]
    mov          r10,rdx                  ; r10 <-- carry
    vmovq        rax,xmm0
    mul          rcx
    ; resexp = xexp & 7 
    and          r11,7                    ; r11 <-- resexp = last 3 bits of xexp
    vpsrldq      xmm0,xmm0,8 
    add          rax,r10                  ; add the previous carry
    adc          rdx,0
    mov          r9,rax                   ; r9 <-- next 64 bits of mul = res1[1]
    mov          r10,rdx                  ; r10 <-- carry
    vmovq        rax,xmm0
    mul          rcx
    add          r10,rax                  ; r10 <-- most sig. 64 bits = res1[0]
 
    ; find the region 
    ; last three bits ltb = most sig bits >> (54 - resexp));
    ;   decimal point in last 18 bits ==> 8 lsb's in first 64 bits
    ;   and 8 msb's in next 64 bits
    ; point_five = ltb & 01h;
    ; region = ((ltb >> 1) + point_five) & 3; 
    mov          rcx,54
    mov          rax,r10
    sub          rcx,r11
    xor          rdx,rdx                  ; rdx <-- sign of x 
    shr          rax,cl 
    jnc          L__no_point_five
    ; if there is carry then negate the result of multiplication
    not          r10
    not          r9
    not          r8
    mov          rdx,08000000000000000h

ALIGN  16 
L__no_point_five:
    adc          rax,0
    and          rax,3                    ; rax now has region
    mov          QWORD PTR [region+rsp], rax

    ; calculate the number of integer bits and zero them out
    mov          rcx,r11 
    add          rcx,10                   ; rcx = no. of integer bits
    shl          r10,cl
    shr          r10,cl                   ; r10 contains only mant bits
    sub          rcx,64                   ; form the exponent
    mov          r11,rcx
 
    ; find the highest set bit
    bsr          rcx,r10
    jnz          L__form_mantissa
    mov          r10,r9
    mov          r9,r8
    mov          r8,0
    bsr          rcx,r10                  ; rcx = hsb
    sub          r11,64
 
ALIGN  16 
L__form_mantissa:
    add          r11,rcx                  ; for exp of x
    sub          rcx,52                   ; rcx = no. of bits to shift in r10 
    cmp          rcx,0
    jl           L__hsb_below_52
    je           L__form_numbers
    ; hsb above 52
    mov          r8,r10                   ; previous r8 not required
    shr          r10,cl                   ; r10 = mantissa of x with hsb at 52
    shr          r9,cl                    ; make space for bits from r10
    sub          rcx,64
    neg          rcx
    ; rcx <-- no of bits to shift r10 to move those bits to r9
    shl          r8,cl
    or           r9,r8                    ; r9 = mantissa bits of xx 
    jmp          L__form_numbers
 
ALIGN  16 
L__hsb_below_52:
    ; rcx has shift count (< 0)
    neg          rcx
    mov          rax,r9
    shl          r10,cl
    shl          r9,cl
    sub          rcx,64
    neg          rcx
    shr          rax,cl
    or           r10,rax
    shr          r8,cl
    or           r9,r8
 
ALIGN  16
;   Here r11 has unbiased exponent
;   r10 has mantissa, with implicit bit possibly set
;   rdx has the sign bit
L__form_numbers:
    add          r11,1023                 ; r11 <-- biased exponent
    btr          r10,52                   ; remove the implicit bit
    mov          rcx,r11                  ; rcx <-- copy of biased exponent
    or           r10,rdx                  ; put the sign 
    shl          rcx,52                   ; shift biased exponent into place
    or           r10,rcx                  ; r10 <-- x
    vmovq        xmm2,r10                 ; xmm1l <-- x
 
    ; form xx
;   xor          rcx,rcx ; Why is this necessary???
    bsr          rcx,r9                   ; scan for high bit of xx mantissa
    sub          rcx,64                   ; to shift the implied bit as well
    neg          rcx
    shl          r9,cl
    shr          r9,12
    add          rcx,52
    sub          r11,rcx
    shl          r11,52
    or           r9,rdx
    or           r9,r11
    vmovq        xmm1,r9                  ; xmm1 <-- xx
    vandpd       xmm4,xmm2,L_fff800       ; xmm4 <-- hx
    vsubsd       xmm0,xmm2,xmm4           ; xmm0 <-- tx
    vmulsd       xmm5,xmm2,L_piby2_lead   ; xmm5 <-- c
    vmulsd       xmm3,xmm4,L_piby2_part1
    vsubsd       xmm3,xmm3,xmm5
    vfmadd231sd  xmm3,xmm0,L_piby2_part1
    vfmadd231sd  xmm3,xmm4,L_piby2_part2
    vfmadd231sd  xmm3,xmm0,L_piby2_part2
    vmulsd       xmm4,xmm1,L_piby2_lead
    vfmadd231sd  xmm4,xmm2,L_piby2_part3
    vaddsd       xmm3,xmm3,xmm4           ; xmm3 <-- cc
    vaddsd       xmm0,xmm5,xmm3           ; xmm0 <--r
    vsubsd       xmm1,xmm5,xmm0
    vaddsd       xmm1,xmm1,xmm3           ; xmm1 <-- rr
    mov          rax, QWORD PTR [region+rsp]

    StackDeallocate stack_size
    ret
fname endp

ALIGN 16
PUBLIC fbname
fbname PROC FRAME
    .ENDPROLOG
    ; Boldo, Daumas, annd Li, "Formally Verified Argument
    ; Reduction With a Fused Multiply-Add,"
    ; IEEE Trans. Comp., vol. 58, #8, Aug. 2009
    ; coefficients are from table 1, mutatis mutandis
    ; algorithm is their formula 3.1 (for getting z from sigma) and
    ; algorithm 5.1 (and extended version) for actual reduction
    vmovapd      xmm1,xmm0
    vmovapd      xmm4,L_xc2               ; xmm4 <-- xc2
    vmovapd      xmm2,L_sigma
    vfmadd132sd  xmm1,xmm2,L_r            ; z = arg*r + sigma
    vsubsd       xmm1,xmm1,xmm2           ; xmm1 <-- z -= sigma
    vcvttpd2dq   xmm5,xmm1
    vmovq        rax, xmm5
    vmovapd      xmm2,xmm1
    vfnmadd132sd xmm2,xmm0,L_xc1          ; xmm2 <-- u = arg - z*xc1
    vmulsd       xmm3,xmm1,xmm4           ; xmm3 <-- p1 = z*xc2
    vmovapd      xmm0,xmm1                ; xmm0 <-- copy of z
    vfmsub213sd  xmm0,xmm4,xmm3           ; xmm0 <-- p2 = z*xc2 - p1
    vsubsd       xmm5,xmm2,xmm3           ; xmm5 <-- t1 = u - p1
    ; We really don't want to spill in this code, so we're commandeering xmm4
    vsubsd       xmm4,xmm2,xmm5           ; xmm4 <-- temp = u - t1
    vsubsd       xmm4,xmm4,xmm3           ; xmm4 <-- t2 = temp - p1
    ; used to use xmm4 here for L_xc2
    vfnmadd231sd xmm2,xmm1,L_xc2          ; xmm2 <-- v1 = -xc2*z + u
    vsubsd       xmm5,xmm5,xmm2           ; xmm5 <-- v2 = t1 - v1
    vaddsd       xmm5,xmm5,xmm4           ; xmm5 <-- v2 += t2
    vsubsd       xmm5,xmm5,xmm0           ; xmm5 <-- v2 -= p2
    vmovapd      xmm0,xmm2                ; xmm0 <-- arghead = v1
    vfnmadd132sd xmm1,xmm5,L_xc3          ; xmm1 <-- argtail = -xc3*z + v2
    and          rax, 3                   ; rax <-- region
    ret
fbname        endp
END
