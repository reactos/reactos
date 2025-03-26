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
; logf.asm
;
; An implementation of the logf libm function.
;
; Prototype:
;
;     float logf(float x);
;

;
;   Algorithm:
;       Similar to one presnted in log.asm
;
.const


ALIGN 16

L_real_one              DQ 0000000003f800000h   ; 1.0
                        DQ 0000000000000000h
L_real_two              DQ 00000000040000000h   ; 1.0
                        DQ 00000000000000000h
L_real_ninf             DQ 000000000ff800000h   ; -inf
                        DQ 0000000000000000h
L_real_inf              DQ 0000000007f800000h   ; +inf
                        DQ 0000000000000000h
L_real_nan              DQ 0000000007fc00000h   ; NaN
                        DQ 0000000000000000h
L_real_neg_qnan         DQ 000000000ffc00000h
                        DQ 0000000000000000h
L_real_notsign          DQ 0000000007ffFFFFFh   ; ^sign bit
                        DQ 0000000000000000h
L_real_mant             DQ 0007FFFFF007FFFFFh   ; mantissa bits
                        DQ 0007FFFFF007FFFFFh
L_mask_127              DQ 00000007f0000007fh   ; 
                        DQ 00000007f0000007fh
L_mask_253              DQ 000000000000000fdh
                        DQ 00000000000000000h
L_mask_mant_all7        DQ 00000000007f0000h
                        DQ 00000000007f0000h
L_mask_mant8            DQ 0000000000008000h
                        DQ 0000000000000000h
L_real_ca1              DQ 0000000003DAAAAABh   ; 8.33333333333317923934e-02
                        DQ 00000000000000000h
L_real_ca2              DQ 0000000003C4CCCCDh   ; 1.25000000037717509602e-02
                        DQ 00000000000000000h
L_real_log2_lead        DQ 03F3170003F317000h   ; 0.693115234375
                        DQ 00000000000000000h
L_real_log2_tail        DQ 0000000003805FDF4h   ; 0.000031946183
                        DQ 00000000000000000h
L_real_half             DQ 0000000003f000000h   ; 1/2
                        DQ 00000000000000000h
L_real_1_over_3         DQ 0000000003eaaaaabh
                        DQ 00000000000000000h

L_real_1_over_2         DD 03f000000h
L_real_neg127           DD 0c2fe0000h
L_real_qnanbit          DD 000400000h   ; quiet nan bit
L_real_threshold        DD 03d800000h

; these codes and the ones in the corresponding .c file have to match
L_flag_x_zero           DD 00000001
L_flag_x_neg            DD 00000002
L_flag_x_nan            DD 00000003

EXTRN __log_128_lead:DWORD
EXTRN __log_128_tail:DWORD
EXTRN __log_F_inv_dword:DWORD
EXTRN __use_fma3_lib:DWORD

fname           TEXTEQU <logf>
fname_special   TEXTEQU <_logf_special>

; define local variable storage offsets

dummy_space     EQU     020h
stack_size      EQU     038h

include fm.inc

; external function
EXTERN fname_special:PROC

.code

PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    .ENDPROLOG
    cmp         DWORD PTR __use_fma3_lib, 0
    jne         Llogf_fma3

    ; Some of the placement of instructions below iwll be odd.
    ; We are attempting to have no more than one branch per 32-byte block.
Llogf_sse2:
    ; Zero the high bits of rax because it will be used as an index later.
    xor         rax, rax
    movdqa      xmm3, xmm0
    movaps      xmm4, xmm0

    ; This computation of the expoonent of x will produce nonsenes if x <= 0.,
    ; but those cases are eliminated below, so it does no harm.
    psrld       xmm3, 23                         ; xmm3 <-- biased exp if x > 0.

    ; Is x Inf or NaN?
    movd        eax, xmm0                        ; eax <-- x
    mov         ecx, eax
    btr         ecx, 31                          ; ecx <-- |x|
    cmp         ecx, DWORD PTR L_real_inf
    jae         Llogf_sse2_x_is_inf_or_nan

    ; Finish computing exponent.
    psubd       xmm3, XMMWORD PTR L_mask_127     ; xmm3 <-- xexp (unbiased)
    movdqa      xmm2, xmm0
    cvtdq2ps    xmm5, xmm3                       ; (float)xexp, unless x <= 0.

    ; Is x negative or zero?
    xorps       xmm1, xmm1
    comiss      xmm0, xmm1
    jbe         Llogf_sse2_x_is_zero_or_neg

    pand        xmm2, XMMWORD PTR L_real_mant    ; xmm2 <-- x mantissa for later
    subss       xmm4, DWORD PTR L_real_one       ; xmm4 <-- x - 1. for later

    comiss      xmm5, DWORD PTR L_real_neg127    ; x!=0, xexp==0 ==> subnormal
    je          Llogf_sse2_subnormal_adjust

Llogf_sse2_continue_common:
    ; At this point we need |x| (possibly adjusted) in eax
    ; and m = xexpx (possibly adjusted) in xmm5
    ; We also need the value of x - 1. computed above.

    ; compute the index into the log tables
    mov         r9d, eax
    and         eax, DWORD PTR L_mask_mant_all7  ; eax <-- 7 bits of x mantissa
    and         r9d, DWORD PTR L_mask_mant8      ; r9d <-- 8th bit
    shl         r9d, 1
    add         eax, r9d                         ; use 8th bit to round up
    movd        xmm1, eax

    ; Is x near 1.0 ?
    ; Note that if x is subnormal it is perforce not near one.
    andps       xmm4, XMMWORD PTR L_real_notsign ; xmm4 <-- |x-1|
    comiss      xmm4, DWORD PTR L_real_threshold ; is |x-1| < 1/16?
    jb          Llogf_sse2_near_one              ; if so, handle elsewhere

    ; F, Y
    ; F is a number in [.5,1) scaled from the rounded mantissa bits computed
    ; above by oring in the exponent of .5.
    ; Y is all of the mantissa bits of X scaled to [.5,1.) similarly
    shr         eax, 16                          ; shift eax to use as index
    por         xmm2, XMMWORD PTR L_real_half    ; xmm2 <-- Y
    por         xmm1, XMMWORD PTR L_real_half    ; xmm2 <-- F
    lea         r9, QWORD PTR __log_F_inv_dword


    ; f = F - Y, r = f * inv
    subss       xmm1, xmm2                       ; xmm1 <-- f
    mulss       xmm1, DWORD PTR [r9+rax*4]       ; xmm1 <-- r = f*inv (tabled)

    movaps      xmm2, xmm1
    movaps      xmm0, xmm1

    ; poly
    mulss       xmm2, DWORD PTR L_real_1_over_3  ; xmm2 <-- r/3
    mulss       xmm0, xmm1                       ; xmm0 <-- r^2
    addss       xmm2, DWORD PTR L_real_1_over_2
    movaps      xmm3, XMMWORD PTR L_real_log2_tail

    lea         r9, QWORD PTR __log_128_tail
    lea         r10, QWORD PTR __log_128_lead

    mulss       xmm2, xmm0                       ; xmm2 <-- r^2 * (r/3 + 1/2)
    mulss       xmm3, xmm5                       ; xmm3 <-- (m=xexp)*log2_tail
    addss       xmm1, xmm2                       ; xmm1 <-- poly

    ; m*log(2) + log(G) - poly, where G is just 2*F
    ; log(G) is precomputed to extra precision.
    ; small pieces and large pieces are separated until the final add,
    ; to preserve accuracy
    movaps      xmm0, XMMWORD PTR L_real_log2_lead
    subss       xmm3, xmm1                       ; xmm3 <-- m*log2_tail - poly
    mulss       xmm0, xmm5                       ; xmm0 <-- m*log1_lead
    addss       xmm3, DWORD PTR [r9+rax*4]       ; xmm3 += log(G) tail
    addss       xmm0, DWORD PTR [r10+rax*4]      ; xmm0 += log(G) lead

    addss       xmm0, xmm3                       ; xmm0 <-- m*log(2)+log(G)-poly

    StackDeallocate stack_size
    ret

ALIGN 16
Llogf_sse2_near_one:
    ; Computation of the log for x near one requires special techniques.
    movaps      xmm2, DWORD PTR L_real_two
    subss       xmm0, DWORD PTR L_real_one       ; xmm0 <-- r = x - 1.0
    addss       xmm2, xmm0
    movaps      xmm1, xmm0
    divss       xmm1, xmm2                       ; xmm1 <-- u = r/(2.0+r)
    movaps      xmm4, xmm0
    mulss       xmm4, xmm1                       ; xmm4 <-- correction = r*u
    addss       xmm1, xmm1                       ; xmm1 <-- u = 2.*u
    movaps      xmm2, xmm1
    mulss       xmm2, xmm2                       ; xmm2 <-- u^2

    ; r2 = (u^3 * (ca_1 + u^2 * ca_2) - correction)
    movaps      xmm3, xmm1
    mulss       xmm3, xmm2                       ; xmm3 <-- u^3
    mulss       xmm2, DWORD PTR L_real_ca2       ; xmm2 <-- ca2*u^2
    addss       xmm2, DWORD PTR L_real_ca1       ; xmm2 <-- ca2*u^2 + ca1
    mulss       xmm2, xmm3                       ; xmm2 <-- u^3*(ca1+u^2*ca2)
    subss       xmm2, xmm4                       ; xmm2 <-- r2

    ; return r + r2
    addss       xmm0, xmm2
    StackDeallocate stack_size
    ret

ALIGN 16
Llogf_sse2_subnormal_adjust:
    ; This code adjusts eax and xmm5.
    ; It must preserve xmm4.
    por         xmm2, XMMWORD PTR L_real_one
    subss       xmm2, DWORD PTR L_real_one
    movdqa      xmm5, xmm2
    pand        xmm2, XMMWORD PTR L_real_mant
    movd        eax, xmm2
    psrld       xmm5, 23
    psubd       xmm5, XMMWORD PTR L_mask_253
    cvtdq2ps    xmm5, xmm5
    jmp         Llogf_sse2_continue_common

; Until we get to the FMA3 code, the rest of this is special case handling.
Llogf_sse2_x_is_zero_or_neg:
    jne         Llogf_sse2_x_is_neg

    movaps      xmm1, XMMWORD PTR L_real_ninf
    mov         r8d, DWORD PTR L_flag_x_zero
    call        fname_special
    jmp         Llogf_sse2_finish

Llogf_sse2_x_is_neg:

    movaps      xmm1, XMMWORD PTR L_real_neg_qnan
    mov         r8d, DWORD PTR L_flag_x_neg
    call        fname_special
    jmp         Llogf_sse2_finish

Llogf_sse2_x_is_inf_or_nan:

    cmp         eax, DWORD PTR L_real_inf
    je          Llogf_sse2_finish

    cmp         eax, DWORD PTR L_real_ninf
    je          Llogf_sse2_x_is_neg

    or          eax, DWORD PTR L_real_qnanbit
    movd        xmm1, eax
    mov         r8d, DWORD PTR L_flag_x_nan
    call        fname_special
    jmp         Llogf_sse2_finish    

Llogf_sse2_finish:
    StackDeallocate stack_size
    ret

ALIGN 16
Llogf_fma3:
    ; compute exponent part
    vmovaps      xmm4,XMMWORD PTR L_real_inf ; preload for inf/nan test
    xor          rax,rax
    vpsrld       xmm3,xmm0,23             ; xmm3 <-- (ux>>23)
    vmovd        eax,xmm0  ;eax = x
    vpsubd       xmm3,xmm3,DWORD PTR L_mask_127 ; xmm3 <-- (ux>>23) - 127
    vcvtdq2ps    xmm5,xmm3                ; xmm5 <-- float((ux>>23)-127) = xexp

    ;  NaN or inf
    vpand        xmm1,xmm0,xmm4           ; xmm1 <-- (ux & 07f800000h)
    vcomiss      xmm1,xmm4
    je           Llogf_fma3_x_is_inf_or_nan

    ; check for negative numbers or zero
    vpxor        xmm1,xmm1,xmm1
    vcomiss      xmm0,xmm1
    jbe          Llogf_fma3_x_is_zero_or_neg

    vpand        xmm2,xmm0,DWORD PTR L_real_mant  ; xmm2 <-- ux & 0007FFFFFh
    vsubss       xmm4,xmm0,DWORD PTR L_real_one   ; xmm4 <-- x - 1.0

    vcomiss      xmm5,DWORD PTR L_real_neg127
    je           Llogf_fma3_subnormal_adjust

Llogf_fma3_continue_common:

    ; compute the index into the log tables
    vpand        xmm1,xmm0,DWORD PTR L_mask_mant_all7 ; xmm1 = ux & 0007f0000h 
    vpand        xmm3,xmm0,DWORD PTR L_mask_mant8     ; xmm3 = ux & 000008000h
    vpslld       xmm3,xmm3,1              ; xmm3  = (ux & 000008000h) << 1
    vpaddd       xmm1,xmm3,xmm1
    ; eax = (ux & 0007f0000h) + ((ux & 000008000h) << 1)
    ; eax <-- x/127., rounded to nearest
    vmovd        eax,xmm1

    ; near one codepath
    vandps       xmm4,xmm4,DWORD PTR L_real_notsign   ; xmm4 <-- fabs (x - 1.0)
    vcomiss      xmm4,DWORD PTR L_real_threshold
    jb           Llogf_fma3_near_one

    ; F,Y
    shr          eax,16
    vpor         xmm2,xmm2,DWORD PTR L_real_half ; xmm2 <-- Y
    vpor         xmm1,xmm1,DWORD PTR L_real_half ; xmm1 <-- F
    lea          r9,QWORD PTR __log_F_inv_dword

    ; f = F - Y
    vsubss       xmm1,xmm1,xmm2           ; f = F - Y
    ; r = f * log_F_inv_dword[index]
    vmulss       xmm1,xmm1,DWORD PTR [r9 + rax * 4]

    ; poly
    vmovaps      xmm2,XMMWORD PTR L_real_1_over_3
    vfmadd213ss  xmm2,xmm1,DWORD PTR L_real_1_over_2 ; 1/3*r + 1/2
    vmulss       xmm0,xmm1,xmm1           ; r*r
    vmovaps      xmm3,DWORD PTR L_real_log2_tail;

    lea          r9,DWORD PTR __log_128_tail
    lea          r10,DWORD PTR __log_128_lead

    vfmadd231ss  xmm1,xmm2,xmm0           ; poly = r + 1/2*r*r + 1/3*r*r*r 
    vfmsub213ss  xmm3,xmm5,xmm1           ; (xexp * log2_tail) - poly

    ; m*log(2) + log(G) - poly
    vmovaps      xmm0,DWORD PTR L_real_log2_lead
    vfmadd213ss  xmm0,xmm5,[r10 + rax * 4]
    ; z2 = (xexp * log2_tail) - poly + log_128_tail[index]
    vaddss       xmm3,xmm3,DWORD PTR [r9 + rax * 4]
    vaddss       xmm0,xmm0,xmm3           ; return z1 + z2
    
    StackDeallocate stack_size
    ret

ALIGN  16
Llogf_fma3_near_one:
    ; r = x - 1.0;
    vmovaps      xmm2,DWORD PTR L_real_two
    vsubss       xmm0,xmm0,DWORD PTR L_real_one  ; xmm0 = r = = x - 1.0

    ; u = r / (2.0 + r)
    vaddss       xmm2,xmm2,xmm0           ; (r+2.0)
    vdivss       xmm1,xmm0,xmm2           ; u = r / (2.0 + r)

    ; correction = r * u
    vmulss       xmm4,xmm0,xmm1           ; correction = u*r

    ; u = u + u;
    vaddss       xmm1,xmm1,xmm1           ; u = u+u 
    vmulss       xmm2,xmm1,xmm1           ; v = u^2

    ; r2 = (u * v * (ca_1 + v * ca_2) - correction)
    vmulss       xmm3,xmm1,xmm2           ; u^3
    vmovaps      xmm5,DWORD PTR L_real_ca2
    vfmadd213ss  xmm2,xmm5,DWORD PTR L_real_ca1
    vfmsub213ss  xmm2,xmm3,xmm4       ; r2 = (ca1 + ca2 * v) * u^3 - correction

    ; r + r2
    vaddss       xmm0,xmm0,xmm2
    StackDeallocate stack_size
    ret


ALIGN  16
Llogf_fma3_subnormal_adjust:
    vmovaps      xmm3,DWORD PTR L_real_one
    vpor         xmm2,xmm2,xmm3  ; xmm2 = temp = ((ux &0007FFFFFh) | 03f800000h)
    vsubss       xmm2,xmm2,xmm3  ; xmm2 = temp -1.0
    vpsrld       xmm5,xmm2,23                 ; xmm5 = (utemp >> 23)
    vpand        xmm2,xmm2,DWORD PTR L_real_mant ; xmm2 = (utemp & 0007FFFFFh)
    vmovaps      xmm0,xmm2
    vpsubd       xmm5,xmm5,DWORD PTR L_mask_253  ; xmm5 = (utemp >> 23) - 253
    vcvtdq2ps    xmm5,xmm5               ; xmm5 = (float) ((utemp >> 23) - 253)
    jmp          Llogf_fma3_continue_common

Llogf_fma3_x_is_zero_or_neg:
    jne          Llogf_fma3_x_is_neg

    vmovaps      xmm1,DWORD PTR L_real_ninf
    mov          r8d,DWORD PTR L_flag_x_zero
    call         fname_special
    
    StackDeallocate stack_size
    ret
 

Llogf_fma3_x_is_neg:

    vmovaps      xmm1,DWORD PTR L_real_neg_qnan
    mov          r8d,DWORD PTR L_flag_x_neg
    call         fname_special
    
    StackDeallocate stack_size
    ret
     
Llogf_fma3_x_is_inf_or_nan:

    cmp          eax,DWORD PTR L_real_inf
    je           Llogf_fma3_finish

    cmp          eax,DWORD PTR L_real_ninf
    je           Llogf_fma3_x_is_neg

    or           eax,DWORD PTR L_real_qnanbit
    vmovd        xmm1,eax
    mov          r8d,DWORD PTR L_flag_x_nan
    call         fname_special
    
    StackDeallocate stack_size
    ret

Llogf_fma3_finish:
    
    StackDeallocate stack_size
    ret


fname endp
END
