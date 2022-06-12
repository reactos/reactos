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
;
; An implementation of the sinf function.
;
; Prototype   
;
;     float sinf(float x);
;
;   Computes sinf(x).
;   It will provide proper C99 return values,
;   but may not raise floating point status bits properly.
;   Based on the NAG C implementation.
;

.const
ALIGN 16
L_signbit        DQ 08000000000000000h
                 DQ 08000000000000000h
L_sign_mask      DQ 07FFFFFFFFFFFFFFFh
                 DQ 07FFFFFFFFFFFFFFFh
L_one            DQ 03FF0000000000000h
                 DQ 03FF0000000000000h
L_int_three      DQ 00000000000000003h
                 DQ 00000000000000003h
L_one_half       DQ 03FE0000000000000h
                 DQ 03FE0000000000000h
L_twobypi        DQ 03FE45F306DC9C883h
                 DQ 03FE45F306DC9C883h
L_piby2_1        DQ 03FF921FB54400000h
                 DQ 03FF921FB54400000h
L_one_sixth      DQ 03FC5555555555555h
                 DQ 03FC5555555555555h
L_piby2_1tail    DQ 03DD0B4611A626331h
                 DQ 03DD0B4611A626331h
L_piby2_2        DQ 03dd0b4611a600000h
                 DQ 03dd0b4611a600000h
L_piby2_2tail    DQ 03ba3198a2e037073h
                 DQ 03ba3198a2e037073h
L_inf_mask_32    DD 07F800000h
                 DD 07F800000h
                 DQ 07F8000007F800000h
L_int_two        DQ 00000000000000002h
                 DQ 00000000000000002h
L_piby2_lead     DQ 03ff921fb54442d18h
                 DQ 03ff921fb54442d18h
L_piby4          DQ 03fe921fb54442d18h
                 DQ 03fe921fb54442d18h
L_mask_3f2       DQ 03f20000000000000h
                 DQ 03f20000000000000h
L_mask_3f8       DQ 03f80000000000000h
                 DQ 03f80000000000000h

; Do these really need to be different?
L_large_x_fma3   DQ 04170008AC0000000h     ; 16779436
L_large_x_sse2   DQ 0416E848000000000h     ; 16000000

EXTRN __Lcosfarray:QWORD
EXTRN __Lsinfarray:QWORD
EXTRN __use_fma3_lib:DWORD
EXTRN __L_2_by_pi_bits:BYTE

; define local variable storage offsets
p_temp              EQU 010h     ; temporary for get/put bits operation
p_temp1             EQU 018h     ; temporary for get/put bits operation
region              EQU 020h     ; pointer to region for remainder_piby2
r                   EQU 028h     ; pointer to r for remainder_piby2
dummy_space         EQU 040h

stack_size          EQU 058h

include fm.inc

fname           TEXTEQU <sinf>
fname_special   TEXTEQU <_sinf_special>

;Define name and any external functions being called
EXTRN           __remainder_piby2d2f_forC : PROC    ; NEAR
EXTERN          fname_special        : PROC

.code
ALIGN 16
PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    .ENDPROLOG   
    cmp          DWORD PTR __use_fma3_lib, 0
    jne          Lsinf_fma3

Lsinf_sse2:

    xorpd   xmm2, xmm2                                ; zeroed out for later use

;;  if NaN or inf
    movd    edx, xmm0
    mov     eax, 07f800000h
    mov     r10d, eax
    and     r10d, edx
    cmp     r10d, eax 
    jz      Lsinf_sse2_naninf

; GET_BITS_DP64(x, ux);
; get the input value to an integer register.
    cvtss2sd     xmm0, xmm0               ; convert input to double.
    movd         rdx, xmm0                ; rdx is ux

;  ax = (ux & ~SIGNBIT_DP64);
    mov     r10, rdx
    btr     r10, 63                                   ; r10 is ax
    mov     r8d, 1                                    ; for determining region later on

;;  if (ax <= 0x3fe921fb54442d18)  abs(x) <= pi/4 
    mov     rax, 03fe921fb54442d18h
    cmp     r10, rax
    jg      Lsinf_absx_gt_piby4

;;      if (ax < 0x3f80000000000000)  abs(x) < 2.0^(-7) 
    mov     rax, 3f80000000000000h
    cmp     r10, rax
    jge     Lsinf_sse2_small

;;          if (ax < 0x3f20000000000000) abs(x) < 2.0^(-13)
    mov     rax, 3f20000000000000h
    cmp     r10, rax
    jge     Lsinf_sse2_smaller

;                  sinf = x;
    jmp     Lsinf_sse2_cleanup 

ALIGN 16
Lsinf_sse2_smaller:   
;   sinf = x - x^3 * 0.1666666666666666666;
    movsd   xmm2, xmm0
    movsd   xmm4, QWORD PTR L_one_sixth   ; 0.1666666666666666666
    mulsd   xmm2, xmm2                    ; x^2
    mulsd   xmm2, xmm0                    ; x^3
    mulsd   xmm2, xmm4                    ; x^3 * 0.1666666666666666666
    subsd   xmm0, xmm2                    ; x - x^3 * 0.1666666666666666666
    jmp     Lsinf_sse2_cleanup  

ALIGN 16
Lsinf_sse2_small:   
    movsd   xmm2, xmm0                    ; x2 = r * r;
    mulsd   xmm2, xmm0                    ; x2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; region 0 or 2     - do a sinf calculation
;  zs = x + x3((s1 + x2 * s2) + x4(s3 + x2 * s4));
    movsd   xmm1, QWORD PTR __Lsinfarray+18h          ; s4
    mulsd   xmm1, xmm2                                ; s4x2
    movsd   xmm4, xmm2                                ; move for x4
    movsd   xmm5, QWORD PTR __Lsinfarray+8h           ; s2
    mulsd   xmm4, xmm2                                ; x4
    movsd   xmm3, xmm0                                ; move for x3
    mulsd   xmm5, xmm2                                ; s2x2
    mulsd   xmm3, xmm2                                ; x3        
    addsd   xmm1, QWORD PTR __Lsinfarray+10h          ; s3+s4x2
    mulsd   xmm1, xmm4                                ; s3x4+s4x6
    addsd   xmm5, QWORD PTR __Lsinfarray              ; s1+s2x2
    addsd   xmm1, xmm5                                ; s1+s2x2+s3x4+s4x6
    mulsd   xmm1, xmm3                                ; x3(s1+s2x2+s3x4+s4x6)
    addsd   xmm0, xmm1                                ; x + x3(s1+s2x2+s3x4+s4x6)
    jmp     Lsinf_sse2_cleanup

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ALIGN 16
Lsinf_absx_gt_piby4:   
;  xneg = (ax != ux);
    cmp     rdx, r10
    mov     r11d, 0
;;  if (xneg) x = -x;
    jz      Lsinf_sse2_reduce_moderate
    
    mov     r11d, 1
    subsd   xmm2, xmm0
    movsd   xmm0, xmm2

Lsinf_sse2_reduce_moderate:
;;  if (x < 5.0e6)
    cmp     r10, QWORD PTR L_large_x_sse2
    jae     Lsinf_sse2_reduce_large

; reduce  the argument to be in a range from -pi/4 to +pi/4
; by subtracting multiples of pi/2
    movsd   xmm2, xmm0
    movsd   xmm3, QWORD PTR L_twobypi
    movsd   xmm4, xmm0
    movsd   xmm5, QWORD PTR L_one_half              ; .5
    mulsd   xmm2, xmm3

;/* How many pi/2 is x a multiple of? */
;      xexp  = ax >> EXPSHIFTBITS_DP64;
    mov     r9, r10
    shr     r9, 52                                    ; >>EXPSHIFTBITS_DP64

;        npi2  = (int)(x * twobypi + 0.5);
    addsd   xmm2, xmm5                        ; npi2

    movsd   xmm3, QWORD PTR L_piby2_1
    cvttpd2dq  xmm0, xmm2                             ; convert to integer
    movsd   xmm1, QWORD PTR L_piby2_1tail
    cvtdq2pd   xmm2, xmm0                             ; and back to double.

;      /* Subtract the multiple from x to get an extra-precision remainder */
;      rhead  = x - npi2 * piby2_1;
    mulsd   xmm3, xmm2
    subsd   xmm4, xmm3                                ; rhead

;      rtail  = npi2 * piby2_1tail;
    mulsd   xmm1, xmm2
    movd    eax, xmm0

;      GET_BITS_DP64(rhead-rtail, uy);                   
; originally only rhead
    movsd   xmm0, xmm4
    subsd   xmm0, xmm1

    movsd   xmm3, QWORD PTR L_piby2_2
    movd    rcx, xmm0
    movsd   xmm5, QWORD PTR L_piby2_2tail

;    xmm0=r, xmm4=rhead, xmm1=rtail, xmm2=npi2, xmm3=temp for calc, xmm5= temp for calc
;      expdiff = xexp - ((uy & EXPBITS_DP64) >> EXPSHIFTBITS_DP64);
    shl     rcx, 1                                    ; strip any sign bit
    shr     rcx, 53                                   ; >> EXPSHIFTBITS_DP64 +1
    sub     r9, rcx                                   ; expdiff

;;      if (expdiff > 15)
    cmp     r9, 15
    jle     Lsinf_sse2_expdiff_le_15

;       The remainder is pretty small compared with x, which
;       implies that x is a near multiple of pi/2
;       (x matches the multiple to at least 15 bits)
;          t  = rhead;
    movsd   xmm1, xmm4

;          rtail  = npi2 * piby2_2;
    mulsd   xmm3, xmm2

;          rhead  = t - rtail;
    mulsd   xmm5, xmm2                                ; npi2 * piby2_2tail
    subsd   xmm4, xmm3                                ; rhead

;          rtail  = npi2 * piby2_2tail - ((t - rhead) - rtail);
    subsd   xmm1, xmm4                                ; t - rhead
    subsd   xmm1, xmm3                                ; -rtail
    subsd   xmm5, xmm1                                ; rtail

;      r = rhead - rtail;
    movsd   xmm0, xmm4

;HARSHA
;xmm1=rtail
    movsd   xmm1, xmm5
    subsd   xmm0, xmm5

;    xmm0=r, xmm4=rhead, xmm1=rtail
 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Lsinf_sse2_expdiff_le_15:  
    cmp     rcx, 03f2h                                ; is r < 2^-13 ?
    jge     Lsinf_sse2_calc_sincosf_piby4             ; use taylor series if not
    cmp     rcx, 03deh                                ; if r really small.
    jle     Lsinf_sse2_r_very_small                   ; then sinf(r) ~ r or 1

    movsd   xmm2, xmm0
    mulsd   xmm2, xmm0                                ; xmm2 <-- r^2

;;      if region is 0 or 2 do a sinf calc.
    and     r8d, eax
    jnz     Lsinf_sse2_small_calc_sin

; region 0 or 2 do a sinf calculation
; use simply polynomial
;              x - x*x*x*0.166666666666666666;
    movsd   xmm3, QWORD PTR L_one_sixth
    mulsd   xmm3, xmm0                                ; * x
    mulsd   xmm3, xmm2                                ; * x^2
    subsd   xmm0, xmm3                                ; xs
    jmp     Lsinf_sse2_adjust_region

ALIGN 16
Lsinf_sse2_small_calc_sin:   
; region 1 or 3 do a cosf calculation
; use simply polynomial
;              1.0 - x*x*0.5;
    movsd   xmm0, QWORD PTR L_one  ; 1.0
    mulsd   xmm2, QWORD PTR L_one_half              ; 0.5 *x^2
    subsd   xmm0, xmm2                                ; xc
    jmp     Lsinf_sse2_adjust_region

ALIGN 16
Lsinf_sse2_r_very_small:
;;      if region is 0 or 2    do a sinf calc. (sinf ~ x)
    and     r8d, eax
    jz      Lsinf_sse2_adjust_region

    movsd   xmm0, QWORD PTR L_one  ; cosf(r) is a 1
    jmp     Lsinf_sse2_adjust_region

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ALIGN 16
Lsinf_sse2_reduce_large:   
;      Reduce x into range [-pi/4, pi/4]
;      __remainder_piby2d2f_forC(x, &r, &region);

    mov          QWORD PTR p_temp[rsp], r11
    lea          rdx, QWORD PTR r[rsp]
    lea          r8,  QWORD PTR region[rsp]
    movd         rcx, xmm0
    call         __remainder_piby2d2f_forC
    mov          r11, QWORD PTR p_temp[rsp]
    mov          r8d, 1                   ; for determining region later on
    movsd        xmm1, QWORD PTR r[rsp]   ; x
    mov          eax,  DWORD PTR region[rsp] ; region

; xmm0 = x, xmm4 = xx, r8d = 1, eax= region
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; perform taylor series to calc sinfx, cosfx
Lsinf_sse2_calc_sincosf_piby4:
;  x2 = r * r;
    movsd   xmm2, xmm0
    mulsd   xmm2, xmm0                                ; x2

;;      if region is 1 or 3, do a cosf calc.
    and     r8d, eax
    jnz     Lsinf_sse2_do_cosf_calc

; region is 0 or 2: do a sinf calc.
;  zs = x + x3((s1 + x2 * s2) + x4(s3 + x2 * s4));
Lsinf_sse2_do_sinf_calc:
    movsd   xmm1, QWORD PTR __Lsinfarray+18h          ; s4
    mulsd   xmm1, xmm2                                ; s4x2
    movsd   xmm4, xmm2                                ; move for x4    
    mulsd   xmm4, xmm2                                ; x4
    movsd   xmm5, QWORD PTR __Lsinfarray+8h           ; s2
    mulsd   xmm5, xmm2                                ; s2x2
    movsd   xmm3, xmm0                                ; move for x3
    mulsd   xmm3, xmm2                                ; x3        
    addsd   xmm1, QWORD PTR __Lsinfarray+10h          ; s3+s4x2
    mulsd   xmm1, xmm4                                ; s3x4+s4x6     
    addsd   xmm5, QWORD PTR __Lsinfarray              ; s1+s2x2
    addsd   xmm1, xmm5                                ; s1+s2x2+s3x4+s4x6
    mulsd   xmm1, xmm3                                ; x3(s1+s2x2+s3x4+s4x6)
    addsd   xmm0, xmm1                                ; x + x3(s1+s2x2+s3x4+s4x6)
    jmp     Lsinf_sse2_adjust_region

ALIGN 16
Lsinf_sse2_do_cosf_calc:   

; region 1 or 3     - do a cosf calculation
;  zc = 1-0.5*x2+ c1*x4 +c2*x6 +c3*x8;
;     zc = 1-0.5*x2+ c1*x4 +c2*x6 +c3*x8 + c4*x10 for a higher precision
    movsd   xmm1, QWORD PTR __Lcosfarray+20h          ; c4
    movsd   xmm4, xmm2                                ; move for x4
    mulsd   xmm1, xmm2                                ; c4x2
    movsd   xmm3, QWORD PTR __Lcosfarray+10h          ; c2
    mulsd   xmm4, xmm2                                ; x4
    movsd   xmm0, QWORD PTR __Lcosfarray              ; c0
    mulsd   xmm3, xmm2                                ; c2x2
    mulsd   xmm0, xmm2                                ; c0x2 (=-0.5x2)
    addsd   xmm1, QWORD PTR __Lcosfarray+18h          ; c3+c4x2
    mulsd   xmm1, xmm4                                ; c3x4 + c4x6
    addsd   xmm3, QWORD PTR __Lcosfarray+8h           ; c1+c2x2
    addsd   xmm1, xmm3                                ; c1 + c2x2 + c3x4 + c4x6
    mulsd   xmm1, xmm4                                ; c1x4 + c2x6 + c3x8 + c4x10
    addsd   xmm0, QWORD PTR L_one                     ; 1 - 0.5x2
    addsd   xmm0, xmm1                                ; 1 - 0.5x2 + c1x4 + c2x6 + c3x8 + c4x10

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


Lsinf_sse2_adjust_region:           
; positive or negative
;      switch (region)
    shr     eax, 1
    mov     ecx, eax
    and     eax, r11d

    not     ecx
    not     r11d
    and     ecx, r11d

    or      eax, ecx
    and     eax, 1
    jnz     Lsinf_sse2_cleanup

;; if the original region 0, 1 and arg is negative, then we negate the result.
;; if the original region 2, 3 and arg is positive, then we negate the result.
    movsd   xmm2, xmm0
    xorpd   xmm0, xmm0
    subsd   xmm0, xmm2


Lsinf_sse2_cleanup:   
    cvtsd2ss xmm0, xmm0
    StackDeallocate stack_size
    ret

ALIGN 16
Lsinf_sse2_naninf:   
    call    fname_special
    StackDeallocate stack_size
    ret

ALIGN 16
Lsinf_fma3:
    vmovd        eax,xmm0
    mov          r8d,L_inf_mask_32
    and          eax,r8d
    cmp          eax, r8d
    jz           Lsinf_fma3_naninf

    vcvtss2sd    xmm5,xmm0,xmm0
    vmovq        r9,xmm5
    btr          r9,63                    ; r9 <-- |x|
    cmp          r9,L_piby4
    jg           Lsinf_fma3_range_reduce

    cmp          r9,L_mask_3f8
    jge          Lsinf_fma3_compute_sinf_piby_4

    cmp          r9,L_mask_3f2
    jge          Lsinf_fma3_compute_x_xxx_0_1666

    ; Here |x| < 2^-13; just return sin x ~ x
    StackDeallocate stack_size
    ret

ALIGN 16
Lsinf_fma3_compute_x_xxx_0_1666:
    ; Here |x| < 2^-7; return sin x ~ x + 1/6 x^3
    vmulsd       xmm1,xmm5,xmm5
    vmulsd       xmm0,xmm1,xmm5           ; xmm1 <-- x^3
    vfnmadd132sd xmm0,xmm5,L_one_sixth    ; x - x*x*x*0.166666666666666666
    jmp          Lsinf_fma3_return_sinf_s

ALIGN 16
Lsinf_fma3_compute_sinf_piby_4:
    vmovapd      xmm0,xmm5
    vmovsd       xmm1,__Lsinfarray+010h
    vmulsd       xmm3,xmm0,xmm0           ; xmm3 <-- x^2
    vfmadd231sd  xmm1,xmm3,__Lsinfarray+018h
    vfmadd213sd  xmm1,xmm3,__Lsinfarray+08h
    vfmadd213sd  xmm1,xmm3,__Lsinfarray
    vmulsd       xmm3,xmm0,xmm3           ; xmm3 <-- x^3
    vfmadd231sd  xmm0,xmm1,xmm3
    jmp          Lsinf_fma3_return_sinf_s

ALIGN 16
Lsinf_fma3_range_reduce:
    vmovq        xmm0,r9                  ; xmm0 <-- |x|
    cmp          r9,L_large_x_fma3
    jge          Lsinf_fma3_reduce_large

Lsinf_fma3_sinf_reduce_moderate:
    vandpd       xmm1,xmm0,L_sign_mask    ; xmm1 <-- |x|  mov should suffice WAT
    vmovapd      xmm2,L_twobypi
    vfmadd213sd  xmm2,xmm1,L_one_half
    vcvttpd2dq   xmm2,xmm2
    vpmovsxdq    xmm1,xmm2
    vandpd       xmm4,xmm1,L_int_three    ; xmm4 <-- region
    vshufps      xmm1 ,xmm1,xmm1,8
    vcvtdq2pd    xmm1,xmm1
    vmovdqa      xmm2,xmm0
    vfnmadd231sd xmm2,xmm1,L_piby2_1      ; xmm2 <-- rhead
    vmulsd       xmm3,xmm1,L_piby2_1tail  ; xmm3 <-- rtail
    vsubsd       xmm0,xmm2,xmm3           ; xmm0 <-- r_1
    vsubsd       xmm2,xmm2,xmm0
    vsubsd       xmm1,xmm2,xmm3           ; xmm4 <-- rr_1
    jmp          Lsinf_fma3_exit_s

ALIGN 16
Lsinf_fma3_reduce_large:
    lea          r9,__L_2_by_pi_bits
    ;xexp = (x >> 52) 1023
    vmovq        r11,xmm0
    mov          rcx,r11
    shr          r11,52
    sub          r11,1023                 ; r11 <-- xexp = exponent of input x
    ;calculate the last byte from which to start multiplication
    ;last = 134 (xexp >> 3)
    mov          r10,r11
    shr          r10,3
    sub          r10,134 ;r10 = last
    neg          r10 ;r10 = last
    ;load 64 bits of 2_by_pi
    mov          rax,[r9+r10]
    ;mantissa of x = ((x << 12) >> 12) | implied bit
    shl          rcx,12
    shr          rcx,12 ;rcx = mantissa part of input x
    bts          rcx,52 ;add the implied bit as well
    ;load next 128 bits of 2_by_pi
    add          r10,8 ;increment to next 8 bytes of 2_by_pi
    vmovdqu      xmm0,XMMWORD PTR[r9+r10]
    ;do three 64bit multiplications with mant of x
    mul          rcx
    mov          r8,rax                   ; r8 <-- last 64 bits of mul = res1[2]
    mov          r10,rdx                  ; r10 <-- carry
    vmovq        rax,xmm0
    mul          rcx
    ;resexp = xexp & 7
    and          r11,7                    ; r11 <-- resexp = last 3 bits
    psrldq       xmm0,8
    add          rax,r10                  ; add the previous carry
    adc          rdx,0
    mov          r9,rax                   ; r9 <-- next 64 bits of mul = res1[1]
    mov          r10,rdx                  ; r10 <-- carry
    vmovq        rax,xmm0
    mul          rcx
    add          r10,rax                  ; r10 = most sig 64 bits = res1[0]
    ;find the region
    ;last three bits ltb = most sig bits >> (54 resexp))
    ; decimal point in last 18 bits == 8 lsb's in first 64 bits 
    ; and 8 msb's in next 64 bits
    ;point_five = ltb & 01h;
    ;region = ((ltb >> 1) + point_five) & 3;
    mov          rcx,54
    mov          rax,r10
    sub          rcx,r11
    xor          rdx,rdx ;rdx = sign of x(i.e first part of x * 2bypi)
    shr          rax,cl
    jnc          Lsinf_fma3_no_point_five_f
    ;;if there is carry.. then negate the result of multiplication
    not          r10
    not          r9
    not          r8
    mov          rdx,08000000000000000h

Lsinf_fma3_no_point_five_f:
    adc          rax,0
    and          rax,3
    vmovd        xmm4,eax ;store region to xmm4
    ;calculate the number of integer bits and zero them out
    mov          rcx,r11
    add          rcx,10                   ; rcx <-- no. of integer bits
    shl          r10,cl
    shr          r10,cl                   ; r10 contains only mant bits
    sub          rcx,64                   ; form the exponent
    mov          r11,rcx
    ;find the highest set bit
    bsr          rcx,r10
    jnz          Lsinf_fma3_form_mantissa_f
    mov          r10,r9
    mov          r9,r8
    mov          r8,0
    bsr          rcx,r10                  ; rcx <-- hsb
    sub          r11,64

Lsinf_fma3_form_mantissa_f:
    add          r11,rcx ;for exp of x
    sub          rcx,52 ;rcx = no. of bits to shift in r10
    cmp          rcx,0
    jl           Lsinf_fma3_hsb_below_52_f
    je           Lsinf_fma3_form_numbers_f
    ;hsb above 52
    mov         r8,r10                    ; previous contents of r8 not required
    shr         r10,cl                    ; r10 = mantissa of x with hsb at 52
    shr         r9,cl                     ; make space for bits from r10
    sub         rcx,64
    neg         rcx
    shl         r8,cl
    or          r9,r8                     ; r9 = mantissa bits of xx
    jmp         Lsinf_fma3_form_numbers_f

ALIGN 16
Lsinf_fma3_hsb_below_52_f:
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

ALIGN 16
Lsinf_fma3_form_numbers_f:
    add          r11,1023
    btr          r10,52                   ; remove the implied bit
    mov          rcx,r11
    or           r10,rdx                  ; put the sign
    shl          rcx,52
    or           r10,rcx                  ; r10 <-- x
    vmovq        xmm0,r10                 ; xmm0 <-- x
    vmulsd       xmm0,xmm0,L_piby2_lead
Lsinf_fma3_exit_s:
    vmovq        rax,xmm4
    and          rax,01h
    cmp          rax,01h
    jz           Lsinf_fma3_cos_piby4_compute

Lsinf_fma3_sin_piby4_compute:
;;    vmovapd      xmm1,__Lsinfarray+010h
    vmovsd       xmm1,__Lsinfarray+010h
    vmulsd       xmm3,xmm0,xmm0
    vfmadd231sd  xmm1,xmm3,__Lsinfarray+018h
    vfmadd213sd  xmm1,xmm3,__Lsinfarray+008h
    vfmadd213sd  xmm1,xmm3,__Lsinfarray
    vmulsd       xmm3,xmm0,xmm3           ; xmm3 <-- x^3
    vfmadd231sd  xmm0,xmm1,xmm3
    jmp          Lsinf_fma3_exit_s_1

ALIGN 16
Lsinf_fma3_cos_piby4_compute:
    vmovapd      xmm2,L_one
    vmulsd       xmm3,xmm0,xmm0
    vfmadd231sd  xmm2,xmm3,__Lcosfarray   ; xmm2 <-- 1 + c0 x^2
    ; would simple Horner's be slower?
    vmovsd       xmm1,__Lcosfarray+018h       ; xmm1 <-- c3
    vfmadd231sd  xmm1,xmm3,__Lcosfarray+020h  ; xmm1 <--  c4 x^2+ c3
    vfmadd213sd  xmm1,xmm3,__Lcosfarray+010h  ; xmm1 <--  (c4 x^2+ c3)x^2 + c2
    vfmadd213sd  xmm1,xmm3,__Lcosfarray+008h  ; xmm1 <--  ((c4 x^2+ c3)x^2 + c2)x^2 + c1
    vmulsd       xmm3,xmm3,xmm3               ; xmm3 <-- x^4
    vmovdqa      xmm0,xmm2
    vfmadd231sd  xmm0,xmm1,xmm3
Lsinf_fma3_exit_s_1:
    ; assuming FMA3 ==> AVX ==> SSE4.1
    vpcmpeqq     xmm2,xmm4,XMMWORD PTR L_int_two
    vpcmpeqq     xmm3,xmm4,XMMWORD PTR L_int_three
    vorpd        xmm3,xmm2,xmm3
    vandnpd      xmm3,xmm3,L_signbit
    vxorpd       xmm0,xmm0,xmm3

    vandnpd      xmm1,xmm5,L_signbit
    vxorpd       xmm0,xmm1,xmm0
Lsinf_fma3_return_sinf_s:
    vcvtsd2ss    xmm0,xmm0,xmm0
    StackDeallocate stack_size
    ret
    
Lsinf_fma3_naninf:
    call         fname_special
    StackDeallocate stack_size
    ret

fname endp
END
