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
; An implementation of the sin function.
;
; Prototype:
;
;     double sin(double x);
;
;   Computes sin(x).
;   It will provide proper C99 return values,
;   but may not raise floating point status bits properly.
;   Based on the NAG C implementation.
;
; If FMA3 hardware is available, an FMA3 implementation of sin will be used.


.const
ALIGN 16
L_real_piby2_1          DQ 03ff921fb54400000h         ; piby2_1
                        DQ 0
L_real_piby2_1tail      DQ 03dd0b4611a626331h         ; piby2_1tail
                        DQ 0
L_real_piby2_2          DQ 03dd0b4611a600000h         ; piby2_2
                        DQ 0
L_real_piby2_2tail      DQ 03ba3198a2e037073h         ; piby2_2tail
                        DQ 0                  
ALIGN 16

L_one           DQ 03FF0000000000000h, 03FF0000000000000h
L_signbit       DQ 08000000000000000h, 00000000000000000h
L_int_one       DQ 00000000000000001h, 00000000000000000h
L_int_two       DQ 00000000000000002h, 00000000000000000h
L_int_three     DQ 00000000000000003h, 00000000000000000h

L_2_by_pi       DQ 03fe45f306dc9c883h     ; 2/pi
L_one_half      DQ 03FE0000000000000h     ; .5
L_one_sixth     DQ 03FC5555555555555h     ; .1666...
L_two_to_neg_27 DQ 03e40000000000000h     ; 2^-27
L_two_to_neg_13 DQ 03f20000000000000h     ; 2^-13
L_piby4         DQ 03FE921FB54442D18h     ; pi/4
L_small_arg_cw  DQ 0411E848000000000h     ; 5.e5, appropriate for CW
L_small_arg_bdl DQ 0417312D000000000h     ; 2e7, works for BDL

L__inf_mask_64  DQ 07FF0000000000000h     ; +Inf

EXTRN __Lcosarray:QWORD
EXTRN __Lsinarray:QWORD
EXTRN __use_fma3_lib:DWORD

; define local variable storage offsets
p_temp          EQU 030h
p_temp1         EQU 040h
save_r10        EQU 050h
dummy_space     EQU 060h
stack_size      EQU 078h

include fm.inc

fname           TEXTEQU <sin>
fname_special   TEXTEQU <_sin_special>

;Define name and any external functions being called
EXTERN          __remainder_piby2_forAsm   : PROC
EXTERN          __remainder_piby2_fma3     : PROC
EXTERN          __remainder_piby2_fma3_bdl : PROC
EXTERN          fname_special      : PROC

.code

PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    .ENDPROLOG   
    cmp          DWORD PTR __use_fma3_lib, 0
    jne          Lsin_fma3

Lsin_sse2:
    movd    rdx, xmm0
    xorpd   xmm2, xmm2                                ; zeroed out for later use

    mov     r10,rdx
    mov     r8d, 1                     ; for determining region later on
    btr     r10,63                     ; r10 <-- |x|
    cmp     r10,L_piby4
    jb      Lsin_sse2_absx_lt_piby4

Lsin_sse2_absx_nlt_piby4:             ; common case
    mov     r11,rdx
    shr     r11,63
    movd    xmm0,r10                      ; xmm0 <-- |x|
    cmp     r10, QWORD PTR L_small_arg_cw
    jae     Lsin_reduce_precise           ; Note NaN/Inf will branch

; At this point we have |x| < L_small_arg_cw, which is currently 500000.
; Note that if |x| were too large, conversion of npi2 to integer would fail.
; We reduce  the argument to be in a range from -pi/4 to +pi/4
; by subtracting multiples of pi/2
    movapd  xmm2, xmm0
    mulsd   xmm2, L_2_by_pi
    movapd  xmm4, xmm0

;      xexp  = ax >> EXPSHIFTBITS_DP64;
    mov     r9, r10
    shr     r9, 52                        ; >>EXPSHIFTBITS_DP64

; How many pi/2 is |x| a multiple of?
;        npi2  = (int)(x * twobypi + 0.5);
    addsd   xmm2, L_one_half              ; npi2

    movsd   xmm3, L_real_piby2_1
    cvttpd2dq   xmm0, xmm2                ; convert npi2 to integer
    movsd   xmm1, L_real_piby2_1tail
    cvtdq2pd    xmm2, xmm0                ; npi2 back to double

;  Subtract the multiple from x to get an extra-precision remainder
;      rhead  = x - npi2 * piby2_1;
    mulsd        xmm3, xmm2
    subsd        xmm4, xmm3               ; rhead

;      rtail  = npi2 * piby2_1tail;
    mulsd   xmm1, xmm2                    ; rtail
    movd    eax, xmm0                     ; eax <-- npi2

;      GET_BITS_DP64(rhead-rtail, uy);                
; originally only rhead
    movapd  xmm0, xmm4
    subsd   xmm0, xmm1

    movsd   xmm3, L_real_piby2_2
    movd    rcx, xmm0                     ; rcx <-- rhead - rtail
    movsd   xmm5, L_real_piby2_2tail      ; piby2_2tail

;    xmm0=r, xmm1=rtail, xmm2=npi2, xmm3=temp for calc,
;    xmm4=rhead, xmm5= temp for calc
;      expdiff = xexp - ((uy & EXPBITS_DP64) >> EXPSHIFTBITS_DP64);
;   expdiff measures how close rhead - rtail is to |x|
;   (larger expdiff ==> more cancellation in |x| - (rhead-rtail) ==> closer)
    shl     rcx, 1                        ; strip any sign bit
    shr     rcx, 53                       ; >> EXPSHIFTBITS_DP64 +1
    sub     r9, rcx                       ; expdiff

;;      if (expdiff > 15)
    cmp     r9, 15
    jle     Lsin_sse2_cw_reduction_done

;    Here the remainder is pretty small compared with x, which
;    implies that x is a near multiple of pi/2
;    (x matches the multiple to at least 15 bits) 
;    So we do another stage of argument reduction.

;          t  = rhead;
    movapd  xmm1, xmm4

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
    movapd  xmm0, xmm4

;HARSHA
;xmm1=rtail
    movapd  xmm1, xmm5                    ; xmm1 <-- copy of rtail
    subsd   xmm0, xmm5


;    xmm0=r, xmm4=rhead, xmm1=rtail
Lsin_sse2_cw_reduction_done:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; if the input was close to a pi/2 multiple
; The original NAG code missed this trick.
; If the input is very close to n*pi/2 after reduction, so r < 2^-27,
; then the sin is either  ~ 1.0 or ~r, to within 53 bits.

; Note:  Unfortunately this introduces two jcc instructions close to each
; other and to other branches.  As r < 2^-13 should be rather uncommon, it
; almost certainly costs more than it saves. - WAT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;      region = npi2 & 3;

    subsd   xmm4, xmm0                                ; rhead-r
    subsd   xmm4, xmm1                                ; rr = (rhead-r) - rtail

Lsin_piby4:
; perform taylor series to calc sinx, sinx for |x| <= pi/4
;  x2 = r * r;

;xmm4 = a part of rr for the sin path, xmm4 is overwritten in the sin path
;instead use xmm3 because that was freed up in the sin path, xmm3 is overwritten in sin path
    movapd  xmm3, xmm0
    movapd  xmm2, xmm0
    mulsd   xmm2, xmm0                                ;x2

    bt      eax,0
    jc      Lsin_sse2_calc_cos

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; region 0 or 2 do a sin calculation
    movsd   xmm3, __Lsinarray+50h                     ; s6
    mulsd   xmm3, xmm2                                ; x2s6
    movsd   xmm5, __Lsinarray+20h                     ; s3
    movsd   QWORD PTR p_temp[rsp], xmm4               ; store xx
    movapd  xmm1, xmm2                                ; move for x4
    mulsd   xmm1, xmm2                                ; x4
    movsd   QWORD PTR p_temp1[rsp], xmm0              ; store x
    mulsd   xmm5, xmm2                                ; x2s3
    movapd  xmm4, xmm0                                ; move for x3
    addsd   xmm3, __Lsinarray+40h                     ; s5+x2s6
    mulsd   xmm1, xmm2                                ; x6
    mulsd   xmm3, xmm2                                ; x2(s5+x2s6)
    mulsd   xmm4, xmm2                                ; x3
    addsd   xmm5, __Lsinarray+10h                     ; s2+x2s3
    mulsd   xmm5, xmm2                                ; x2(s2+x2s3)
    addsd   xmm3, __Lsinarray+30h                     ; s4 + x2(s5+x2s6)
    mulsd   xmm2, L_one_half                          ; 0.5 *x2
    movsd   xmm0, QWORD PTR p_temp[rsp]               ; load xx
    mulsd   xmm3, xmm1                                ; x6(s4 + x2(s5+x2s6))
    addsd   xmm5, __Lsinarray                         ; s1+x2(s2+x2s3)
    mulsd   xmm2, xmm0                                ; 0.5 * x2 *xx
    addsd   xmm3, xmm5                                ; zs
    mulsd   xmm4, xmm3                                ; *x3
    subsd   xmm4, xmm2                                ; x3*zs - 0.5 * x2 *xx
    addsd   xmm0, xmm4                                ; +xx
    addsd   xmm0, QWORD PTR p_temp1[rsp]              ; +x
    
    jmp     Lsin_sse2_adjust_region

ALIGN 16
Lsin_sse2_calc_cos:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; region 1 or 3     - do a cos calculation
;  zc = (c2 + x2 * (c3 + x2 * (c4 + x2 * (c5 + x2 * c6))));
    mulsd   xmm4, xmm0                                ; x*xx
    movsd   xmm5, L_one_half
    movsd   xmm1, __Lcosarray+50h                     ; c6
    movsd   xmm0, __Lcosarray+20h                     ; c3
    mulsd   xmm5, xmm2                                ; r = 0.5 *x2
    movapd  xmm3, xmm2                                ; copy of x2
    movsd   QWORD PTR p_temp[rsp], xmm4               ; store x*xx
    mulsd   xmm1, xmm2                                ; c6*x2
    mulsd   xmm0, xmm2                                ; c3*x2
    subsd   xmm5, L_one                               ; -t=r-1.0, trash r
    mulsd   xmm3, xmm2                                ; x4
    addsd   xmm1, __Lcosarray+40h                     ; c5+x2c6
    addsd   xmm0, __Lcosarray+10h                     ; c2+x2C3
    addsd   xmm5, L_one                               ; 1 + (-t), trash t
    mulsd   xmm3, xmm2                                ; x6
    mulsd   xmm1, xmm2                                ; x2(c5+x2c6)
    mulsd   xmm0, xmm2                                ; x2(c2+x2C3)
    movapd  xmm4, xmm2                                ; copy of x2
    mulsd   xmm4, L_one_half  ; r recalculate
    addsd   xmm1, __Lcosarray+30h                     ; c4 + x2(c5+x2c6)
    addsd   xmm0, __Lcosarray                         ; c1+x2(c2+x2C3)
    mulsd   xmm2, xmm2                                ; x4 recalculate
    subsd   xmm5, xmm4                                ; (1 + (-t)) - r
    mulsd   xmm1, xmm3                                ; x6(c4 + x2(c5+x2c6))
    addsd   xmm0, xmm1                                ; zc
    subsd   xmm4, L_one                               ; t relaculate
    subsd   xmm5, QWORD PTR p_temp[rsp]               ; ((1 + (-t)) - r) - x*xx
    mulsd   xmm0, xmm2                                ; x4 * zc
    addsd   xmm0, xmm5                                ; x4 * zc + ((1 + (-t)) - r -x*xx)
    subsd   xmm0, xmm4                                ; result - (-t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Lsin_sse2_adjust_region:        
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
    jnz     Lsin_sse2_cleanup

;; if the original region 0, 1 and arg is negative, then we negate the result.
;; if the original region 2, 3 and arg is positive, then we negate the result.
    movapd  xmm2, xmm0
    xorpd   xmm0, xmm0
    subsd   xmm0, xmm2

ALIGN 16
Lsin_sse2_cleanup:
    StackDeallocate stack_size
    ret

ALIGN 16
Lsin_sse2_absx_lt_piby4:
;          sin = sin_piby4(x, 0.0);

;  x2 = r * r;
    movapd  xmm2, xmm0
    mulsd   xmm2, xmm0                                ; x2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; region 0 - do a sin calculation
;  zs = (s2 + x2 * (s3 + x2 * (s4 + x2 * (s5 + x2 * s6))));
    movsd   xmm3, __Lsinarray+50h                     ; s6
    mulsd   xmm3, xmm2                                ; x2s6
    movsd   xmm5, __Lsinarray+20h                     ; s3
    movapd  xmm1, xmm2                                ; move for x4
    mulsd   xmm1, xmm2                                ; x4
    mulsd   xmm5, xmm2                                ; x2s3
    movapd  xmm4, xmm0                                ; move for x3
    addsd   xmm3, __Lsinarray+40h                     ; s5+x2s6
    mulsd   xmm1, xmm2                                ; x6
    mulsd   xmm3, xmm2                                ; x2(s5+x2s6)
    mulsd   xmm4, xmm2                                ; x3
    addsd   xmm5, __Lsinarray+10h                     ; s2+x2s3
    mulsd   xmm5, xmm2                                ; x2(s2+x2s3)
    addsd   xmm3, __Lsinarray+30h                     ; s4 + x2(s5+x2s6)
    mulsd   xmm3, xmm1                                ; x6(s4 + x2(s5+x2s6))
    addsd   xmm5, __Lsinarray                         ; s1+x2(s2+x2s3)
    addsd   xmm3, xmm5                                ; zs
    mulsd   xmm4, xmm3                                ; *x3
    addsd   xmm0, xmm4                                ; +x
    jmp     Lsin_sse2_cleanup

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ALIGN 16
Lsin_reduce_precise:
;   Reduce x into range [-pi/4, pih/4]
    cmp     r10,L__inf_mask_64
    jae     Lsin_x_naninf
    mov     QWORD PTR p_temp[rsp], r11
    call    __remainder_piby2_forAsm
    mov     r11, QWORD PTR p_temp[rsp]

    ; At this point xmm0 has r, xmm1 has rr, rax has region

    movapd  xmm4, xmm1                ; xmm4 <-- rr
    jmp Lsin_piby4

; xmm0 = x, xmm4 = xx, eax= region


ALIGN 16
Lsin_x_naninf:
    call    fname_special
    StackDeallocate stack_size
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; From this point we assume that FMA3 and AVX hardware are present.

ALIGN 16
Lsin_fma3:
    vmovq        r9,xmm0
    mov          r10,r9                ; save x to get sign later
    btr          r9,63                 ; r9 <-- |x|
    cmp          r9,L_piby4
    jae          Lsin_fma3_absx_nlt_piby4  ; Note that NaN will branch
    cmp          r9,L_two_to_neg_13
    jae          Lsin_fma3_calc_sin_for_absx_lt_piby4
    cmp          r9,L_two_to_neg_27
    jae          Lsin_fma3_compute_x_xxx_0_1666
    StackDeallocate stack_size
    ret                                ; sin x ~= x for |x| < 2^-27

ALIGN 16
Lsin_fma3_compute_x_xxx_0_1666:        ; |x| in [2^-27,2^-13]
    vmulsd       xmm1,xmm0,xmm0        ; xmm1l <-- x*x
    vmulsd       xmm1,xmm1,xmm0        ; xmm1l <-- x*x*x
    vfnmadd231sd xmm0,xmm1,L_one_sixth ; xmm0l <-- x - x*x*x*(1/6)
    StackDeallocate stack_size
    ret

ALIGN 16
Lsin_fma3_calc_sin_for_absx_lt_piby4: ; |x| in [2^-13,pi/4]
    vmovsd       xmm5,__Lsinarray+050h
    vmulsd       xmm3,xmm0,xmm0        ; xmm3l <-- x^2
 
    vfmadd213sd  xmm5,xmm3,__Lsinarray+040h 
    vfmadd213sd  xmm5,xmm3,__Lsinarray+030h
    vfmadd213sd  xmm5,xmm3,__Lsinarray+020h
    vfmadd213sd  xmm5,xmm3,__Lsinarray+010h
    
    vmulsd       xmm4,xmm0,xmm3        ; xmm4l <-- x^3
    vfmadd213sd  xmm5,xmm3,__Lsinarray      
    vfmadd231sd  xmm0,xmm4,xmm5        ; xmm0l <-- x + x^3 p(x^2) 

    StackDeallocate stack_size
    ret

ALIGN 16
Lsin_fma3_absx_nlt_piby4:  ; !(|x| < pi/4)
    ; here r9 has |x|
    cmp          r9,L__inf_mask_64
    jae          Lsin_x_naninf
;Lrange_reduce:                         ;; unused label

    vmovq        xmm0,r9               ; xmm0 <-- |x|
    cmp          r9,L_small_arg_bdl
    jae          Lsin_fma3_do_general_arg_reduction

    ; Note that __remainder_piby2_fma3 conventions are
    ; on input
    ;   |x| is in xmm0
    ; on output
    ;   r is in xmm0
    ;   rr is in xmm1
    ;   region of |x| is in rax

    ; Boldo-Daumas-Li reduction for reasonably small |x|
    call         __remainder_piby2_fma3_bdl
Lsin_fma3_exit_s:
    bt           rax,0
    vmulsd       xmm3,xmm0,xmm0        ; xmm3 <-- x2 = x * x
    jc           Lsin_fma3_calc_cos
        
Lsin_fma3_calc_sin:                    ;; unused label
    ; region 0 or 2
    ; compute the sine of r+rr, where this sum is in [-pi/4,pi/4]
    vmovsd       xmm5,__Lsinarray+050h
    vfmadd213sd  xmm5,xmm3,__Lsinarray+040h 
    vfmadd213sd  xmm5,xmm3,__Lsinarray+030h
    vfmadd213sd  xmm5,xmm3,__Lsinarray+020h
    vfmadd213sd  xmm5,xmm3,__Lsinarray+010h  ; xmm5 <-- r
    
    vmulsd       xmm4,xmm0,xmm3        ; xmm4 <-- x3 = x*x*x
    vmulsd       xmm2,xmm4,xmm5        ; xmm2 <-- x*x*x * r
    vmulsd       xmm5,xmm1,L_one_half  ; xmm5 <-- .5*x*x
    vsubsd       xmm2,xmm5,xmm2        ; xmm2 <-- .5*x*x - x*x*x*r
    vmulsd       xmm2,xmm3,xmm2
    vsubsd       xmm2,xmm2,xmm1   
    vfnmadd231sd xmm2, xmm4,__Lsinarray 
    vsubsd       xmm0,xmm0,xmm2
    jmp          Lsin_fma3_exit_s_1
        
ALIGN 16
Lsin_fma3_calc_cos:
    ; region 1 or 3
    ; compute the cosine of r+rr, where this sum is in [-pi/4,pi/4]
    vmovapd      xmm2,L_one
    vmulsd       xmm5,xmm3,L_one_half  ; xmm5 <-- x*x*.5 == r
    vsubsd       xmm4,xmm2,xmm5        ; xmm4 <-- t = 1. - x*x*.5
    vsubsd       xmm2,xmm2,xmm4        ; 1-t
    vsubsd       xmm2,xmm2,xmm5        ; xmm2 <-- (1-t) - r
    vmovsd       xmm5,__Lcosarray+050h
    vfnmadd231sd xmm2,xmm0,xmm1        ; (1.0 - t) - r) - x * xx) xmm2
    vmulsd       xmm1,xmm3,xmm3        ; x2 * x2 xmm1
    vfmadd213sd  xmm5,xmm3,__Lcosarray+040h
    vfmadd213sd  xmm5,xmm3,__Lcosarray+030h
    vfmadd213sd  xmm5,xmm3,__Lcosarray+020h
    vfmadd213sd  xmm5,xmm3,__Lcosarray+010h
    vfmadd213sd  xmm5,xmm3,__Lcosarray
    vfmadd213sd  xmm5,xmm1,xmm2
    vaddsd       xmm0,xmm5,xmm4

Lsin_fma3_exit_s_1:
    xor          r8,r8                    ; prepare r8 for cmov
    and          r10,L_signbit            ; isolate original sign of x
    bt           eax,1
    cmovc        r8,L_signbit
    xor          r8,r10
    vmovq        xmm3,r8
    vxorpd       xmm0,xmm0,xmm3

    StackDeallocate stack_size
    ret

ALIGN 16
Lsin_fma3_do_general_arg_reduction:
    ; argument reduction for general x

    ; NOTE: the BDL argument reduction routine does not touch r10,
    ; but the general-purpose reduction does.
    mov          QWORD PTR [save_r10+rsp], r10
    call         __remainder_piby2_fma3
    mov          r10, QWORD PTR [save_r10+rsp]
    jmp          Lsin_fma3_exit_s

fname endp
END 

