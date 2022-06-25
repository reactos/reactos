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
; log10.asm
;
; An implementation of the log10 libm function.
;
; Prototype:
;
;     double log10(double x);
;

;
;   Algorithm:
;       Similar to one presnted in log.asm
;

.const

ALIGN 16

__real_ninf     DQ 0fff0000000000000h   ; -inf
                DQ 0000000000000000h
__real_inf      DQ 7ff0000000000000h    ; +inf
                DQ 0000000000000000h
__real_neg_qnan DQ 0fff8000000000000h   ; neg qNaN
                DQ 0000000000000000h
__real_qnanbit  DQ 0008000000000000h
                DQ 0000000000000000h
__int_1023      DQ 00000000000003ffh
                DQ 0000000000000000h
__mask_001      DQ 0000000000000001h
                DQ 0000000000000000h

__mask_mant     DQ 000FFFFFFFFFFFFFh    ; mask for mantissa bits
                DQ 0000000000000000h

__mask_mant_top8 DQ 000ff00000000000h   ; mask for top 8 mantissa bits
                DQ 0000000000000000h

__mask_mant9    DQ 0000080000000000h    ; mask for 9th mantissa bit
                DQ 0000000000000000h

__real_log10_e      DQ 3fdbcb7b1526e50eh
                    DQ 0000000000000000h

__real_log10_e_lead DQ 3fdbcb7800000000h ; log10e_lead 4.34293746948242187500e-01
                    DQ 0000000000000000h
__real_log10_e_tail DQ 3ea8a93728719535h ; log10e_tail 7.3495500964015109100644e-7
                    DQ 0000000000000000h

__real_log10_2_lead DQ 3fd3441350000000h
                    DQ 0000000000000000h
__real_log10_2_tail DQ 3e03ef3fde623e25h
                    DQ 0000000000000000h

__real_two          DQ 4000000000000000h ; 2
                    DQ 0000000000000000h

__real_one          DQ 3ff0000000000000h ; 1
                    DQ 0000000000000000h

__real_half         DQ 3fe0000000000000h ; 1/2
                    DQ 0000000000000000h

__mask_100          DQ 0000000000000100h
                    DQ 0000000000000000h
__real_1_over_512   DQ 3f60000000000000h
                    DQ 0000000000000000h

__real_1_over_2     DQ 3fe0000000000000h
                    DQ 0000000000000000h
__real_1_over_3     DQ 3fd5555555555555h
                    DQ 0000000000000000h
__real_1_over_4     DQ 3fd0000000000000h
                    DQ 0000000000000000h
__real_1_over_5     DQ 3fc999999999999ah
                    DQ 0000000000000000h
__real_1_over_6     DQ 3fc5555555555555h
                    DQ 0000000000000000h

__real_neg_1023     DQ 0c08ff80000000000h
                    DQ 0000000000000000h

__mask_2045         DQ 00000000000007fdh
                    DQ 0000000000000000h

__real_threshold    DQ 3fb0000000000000h ; .0625
                    DQ 0000000000000000h

__real_near_one_lt  DQ 3fee000000000000h ; .9375
                    DQ 0000000000000000h

__real_near_one_gt  DQ 3ff1000000000000h ; 1.0625
                    DQ 0000000000000000h

__real_min_norm     DQ 0010000000000000h
                    DQ 0000000000000000h

__real_notsign      DQ 7ffFFFFFFFFFFFFFh ; ^sign bit
                    DQ 0000000000000000h

__real_ca1          DQ 3fb55555555554e6h ; 8.33333333333317923934e-02
                    DQ 0000000000000000h
__real_ca2          DQ 3f89999999bac6d4h ; 1.25000000037717509602e-02
                    DQ 0000000000000000h
__real_ca3          DQ 3f62492307f1519fh ; 2.23213998791944806202e-03
                    DQ 0000000000000000h
__real_ca4          DQ 3f3c8034c85dfff0h ; 4.34887777707614552256e-04
                    DQ 0000000000000000h

__mask_lower        DQ 0ffffffff00000000h
                    DQ 0000000000000000h

; these codes and the ones in the corresponding .c file have to match
__flag_x_zero           DD 00000001
__flag_x_neg            DD 00000002
__flag_x_nan            DD 00000003


EXTRN __log10_256_lead:QWORD
EXTRN __log10_256_tail:QWORD
EXTRN __log_F_inv_qword:QWORD
EXTRN __use_fma3_lib:DWORD


; local variable storage offsets
save_xmm6       EQU     20h
dummy_space     EQU     30h
stack_size      EQU     058h

include fm.inc

fname           TEXTEQU <log10>
fname_special   TEXTEQU <_log10_special>

EXTERN fname_special:PROC

.code
ALIGN 16
PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    SaveXmm xmm6, save_xmm6
    .ENDPROLOG

    cmp          DWORD PTR __use_fma3_lib, 0
    jne          Llog10_fma3

Llog10_sse2:

    ; compute exponent part
    movapd      xmm3, xmm0
    movapd      xmm4, xmm0
    psrlq       xmm3, 52
    movd        rax, xmm0
    psubq       xmm3, XMMWORD PTR __int_1023 ; xmm3 <-- unbiased exponent

    ;  NaN or inf
    movapd      xmm5, xmm0
    andpd       xmm5, XMMWORD PTR __real_inf
    comisd      xmm5, QWORD PTR __real_inf
    je          Llog10_sse2_x_is_inf_or_nan

    movapd      xmm2, xmm0
    cvtdq2pd    xmm6, xmm3                   ; xmm6 <-- unbiased exp as double


    pand        xmm2, XMMWORD PTR __mask_mant
    subsd       xmm4, QWORD PTR __real_one

    comisd      xmm6, QWORD PTR __real_neg_1023
    je          Llog10_sse2_denormal_adjust

Llog10_sse2_continue_common:    

    andpd       xmm4, XMMWORD PTR __real_notsign
    ; compute index into the log tables
    mov         r9, rax
    and         rax, QWORD PTR __mask_mant_top8
    and         r9, QWORD PTR __mask_mant9
    shl         r9, 1
    add         rax, r9
    movd        xmm1, rax

    ; near one codepath
    comisd      xmm4, QWORD PTR __real_threshold
    jb          Llog10_sse2_near_one

    ; F, Y
    shr         rax, 44
    por         xmm2, XMMWORD PTR __real_half
    por         xmm1, XMMWORD PTR __real_half
    lea         r9, QWORD PTR __log_F_inv_qword

    ; check for negative numbers or zero
    xorpd       xmm5, xmm5
    comisd      xmm0, xmm5
    jbe         Llog10_sse2_x_is_zero_or_neg

    ; f = F - Y, r = f * inv
    subsd       xmm1, xmm2
    mulsd       xmm1, QWORD PTR [r9+rax*8]

    movapd      xmm2, xmm1
    movapd      xmm0, xmm1
    lea         r9, QWORD PTR __log10_256_lead

    ; poly
    movsd       xmm3, QWORD PTR __real_1_over_6
    movsd       xmm1, QWORD PTR __real_1_over_3
    mulsd       xmm3, xmm2                         
    mulsd       xmm1, xmm2                         
    mulsd       xmm0, xmm2                         
    movapd      xmm4, xmm0
    addsd       xmm3, QWORD PTR __real_1_over_5
    addsd       xmm1, QWORD PTR __real_1_over_2
    mulsd       xmm4, xmm0                         
    mulsd       xmm3, xmm2                         
    mulsd       xmm1, xmm0                         
    addsd       xmm3, QWORD PTR __real_1_over_4
    addsd       xmm1, xmm2                         
    mulsd       xmm3, xmm4                         
    addsd       xmm1, xmm3                         

    movsd       xmm5, QWORD PTR __real_log10_2_tail
    mulsd       xmm1, QWORD PTR __real_log10_e

    ; m*log(10) + log10(G) - poly
    mulsd       xmm5, xmm6
    subsd       xmm5, xmm1

    movsd       xmm0, QWORD PTR [r9+rax*8]
    lea         rdx, QWORD PTR __log10_256_tail
    movsd       xmm2, QWORD PTR [rdx+rax*8]

    movsd       xmm4, QWORD PTR __real_log10_2_lead
    mulsd       xmm4, xmm6
    addsd       xmm0, xmm4
    addsd       xmm2, xmm5

    addsd       xmm0, xmm2

    RestoreXmm  xmm6, save_xmm6
    StackDeallocate stack_size
    ret

ALIGN 16
Llog10_sse2_near_one:

    ; r = x - 1.0
    movsd       xmm2, QWORD PTR __real_two
    subsd       xmm0, QWORD PTR __real_one ; r

    addsd       xmm2, xmm0
    movapd      xmm1, xmm0
    divsd       xmm1, xmm2 ; r/(2+r) = u/2

    movsd       xmm4, QWORD PTR __real_ca2
    movsd       xmm5, QWORD PTR __real_ca4

    movapd       xmm6, xmm0
    mulsd       xmm6, xmm1 ; correction

    addsd       xmm1, xmm1 ; u
    movapd      xmm2, xmm1

    mulsd       xmm2, xmm1 ; u^2

    mulsd       xmm4, xmm2
    mulsd       xmm5, xmm2

    addsd       xmm4, QWORD PTR __real_ca1
    addsd       xmm5, QWORD PTR __real_ca3

    mulsd       xmm2, xmm1 ; u^3
    mulsd       xmm4, xmm2

    mulsd       xmm2, xmm2
    mulsd       xmm2, xmm1 ; u^7
    mulsd       xmm5, xmm2

    movsd       xmm2, QWORD PTR __real_log10_e_tail
    addsd       xmm4, xmm5
    subsd       xmm4, xmm6
    movsd       xmm6, QWORD PTR __real_log10_e_lead

    movapd      xmm3, xmm0
    pand        xmm3, XMMWORD PTR __mask_lower
    subsd       xmm0, xmm3
    addsd       xmm4, xmm0

    movapd      xmm0, xmm3
    movapd      xmm1, xmm4

    mulsd       xmm4, xmm2
    mulsd       xmm0, xmm2
    mulsd       xmm1, xmm6
    mulsd       xmm3, xmm6

    addsd       xmm0, xmm4
    addsd       xmm0, xmm1
    addsd       xmm0, xmm3

    RestoreXmm  xmm6, save_xmm6
    StackDeallocate stack_size
    ret

Llog10_sse2_denormal_adjust:
    por         xmm2, XMMWORD PTR __real_one
    subsd       xmm2, QWORD PTR __real_one
    movsd       xmm5, xmm2
    pand        xmm2, XMMWORD PTR __mask_mant
    movd        rax, xmm2
    psrlq       xmm5, 52
    psubd       xmm5, XMMWORD PTR __mask_2045
    cvtdq2pd    xmm6, xmm5
    jmp         Llog10_sse2_continue_common

ALIGN 16
Llog10_sse2_x_is_zero_or_neg:
    jne         Llog10_sse2_x_is_neg

    movsd       xmm1, QWORD PTR __real_ninf
    mov         r8d, DWORD PTR __flag_x_zero
    call        fname_special
    jmp         Llog10_sse2_finish

ALIGN 16
Llog10_sse2_x_is_neg:

    movsd       xmm1, QWORD PTR __real_neg_qnan
    mov         r8d, DWORD PTR __flag_x_neg
    call        fname_special
    jmp         Llog10_sse2_finish

ALIGN 16
Llog10_sse2_x_is_inf_or_nan:

    cmp         rax, QWORD PTR __real_inf
    je          Llog10_sse2_finish

    cmp         rax, QWORD PTR __real_ninf
    je          Llog10_sse2_x_is_neg

    or          rax, QWORD PTR __real_qnanbit
    movd        xmm1, rax
    mov         r8d, DWORD PTR __flag_x_nan
    call        fname_special
    jmp         Llog10_sse2_finish    

ALIGN 16
Llog10_sse2_finish:
    RestoreXmm  xmm6, save_xmm6
    StackDeallocate stack_size
    ret

ALIGN 16
Llog10_fma3:
    ; compute exponent part
    xor          rax,rax
    vpsrlq       xmm3,xmm0,52
    vmovq        rax,xmm0
    vpsubq       xmm3,xmm3,XMMWORD PTR __int_1023
    vcvtdq2pd    xmm6,xmm3                ; xmm6 <-- (double)xexp

    ;  NaN or Inf?
    vpand        xmm5,xmm0,__real_inf
    vcomisd      xmm5,QWORD PTR __real_inf
    je           Llog10_fma3_x_is_inf_or_nan

    ; negative number or zero?
    vpxor        xmm5,xmm5,xmm5
    vcomisd      xmm0,xmm5
    jbe          Llog10_fma3_x_is_zero_or_neg

    vpand        xmm2,xmm0,__mask_mant
    vsubsd       xmm4,xmm0,QWORD PTR __real_one

    ; Subnormal?
    vcomisd      xmm6,QWORD PTR __real_neg_1023
    je           Llog10_fma3_denormal_adjust

Llog10_fma3_continue_common:
    ; compute index into the log tables
    vpand        xmm1,xmm0,XMMWORD PTR __mask_mant_top8
    vpand        xmm3,xmm0,XMMWORD PTR __mask_mant9
    vpsllq       xmm3,xmm3,1
    vpaddq       xmm1,xmm3,xmm1
    vmovq        rax,xmm1

    ; near one codepath
    vpand        xmm4,xmm4,XMMWORD PTR __real_notsign
    vcomisd      xmm4,QWORD PTR __real_threshold
    jb           Llog10_fma3_near_one

    ; F,Y
    shr          rax,44
    vpor         xmm2,xmm2,XMMWORD PTR __real_half
    vpor         xmm1,xmm1,XMMWORD PTR __real_half
    lea          r9,DWORD PTR __log_F_inv_qword

    ; f = F - Y,r = f * inv
    vsubsd       xmm1,xmm1,xmm2
    vmulsd       xmm1,xmm1,QWORD PTR [r9 + rax * 8]

    lea          r9,DWORD PTR __log10_256_lead

    ; poly
    vmulsd       xmm0,xmm1,xmm1 ; r*r
    vmovsd       xmm3,QWORD PTR __real_1_over_6
    vmovsd       xmm5,QWORD PTR __real_1_over_3
    vfmadd213sd  xmm3,xmm1,QWORD PTR __real_1_over_5  ; r*1/6 + 1/5
    vfmadd213sd  xmm5,xmm1,QWORD PTR __real_half      ; 1/2+r*1/3
    movsd        xmm4,xmm0                             ; r*r
    vfmadd213sd  xmm3 ,xmm1,QWORD PTR __real_1_over_4 ; 1/4+(1/5*r+r*r*1/6)

    vmulsd       xmm4,xmm0,xmm0                        ; r*r*r*r
    vfmadd231sd  xmm1,xmm5,xmm0                        ; r*r*(1/2+r*1/3) + r
    vfmadd231sd  xmm1,xmm3,xmm4

    vmulsd       xmm1,xmm1,QWORD PTR __real_log10_e
    ; m*log(2) + log(G) - poly*log10_e
    vmovsd       xmm5,QWORD PTR __real_log10_2_tail
    vfmsub213sd  xmm5,xmm6,xmm1

    movsd        xmm0,QWORD PTR [r9 + rax * 8]
    lea          rdx,DWORD PTR __log10_256_tail
    movsd        xmm2,QWORD PTR [rdx + rax * 8]
    vaddsd       xmm2,xmm2,xmm5

    vfmadd231sd  xmm0,xmm6,QWORD PTR __real_log10_2_lead

    vaddsd       xmm0,xmm0,xmm2
    AVXRestoreXmm  xmm6, save_xmm6
    StackDeallocate stack_size
    ret


ALIGN  16
Llog10_fma3_near_one:
    ; r = x - 1.0
    vmovsd       xmm2,QWORD PTR __real_two
    vsubsd       xmm0,xmm0,QWORD PTR __real_one ; r

    vaddsd       xmm2,xmm2,xmm0
    vdivsd       xmm1,xmm0,xmm2 ; r/(2+r) = u/2

    vmovsd       xmm4,QWORD PTR __real_ca2
    vmovsd       xmm5,QWORD PTR __real_ca4

    vmulsd       xmm6,xmm0,xmm1 ; correction
    vaddsd       xmm1,xmm1,xmm1 ; u

    vmulsd       xmm2,xmm1,xmm1 ; u^2
    vfmadd213sd  xmm4,xmm2,QWORD PTR __real_ca1
    vfmadd213sd  xmm5,xmm2,QWORD PTR __real_ca3

    vmulsd       xmm2,xmm2,xmm1 ; u^3
    vmulsd       xmm4,xmm4,xmm2

    vmulsd       xmm2,xmm2,xmm2
    vmulsd       xmm2,xmm2,xmm1 ; u^7

    vmulsd       xmm5,xmm5,xmm2
    vaddsd       xmm4,xmm4,xmm5
    vsubsd       xmm4,xmm4,xmm6
    vpand        xmm3,xmm0,XMMWORD PTR __mask_lower
    vsubsd       xmm0,xmm0,xmm3
    vaddsd       xmm4,xmm4,xmm0

    vmulsd       xmm1,xmm4,QWORD PTR __real_log10_e_lead
    vmulsd       xmm4,xmm4,QWORD PTR __real_log10_e_tail
    vmulsd       xmm0,xmm3,QWORD PTR __real_log10_e_tail
    vmulsd       xmm3,xmm3,QWORD PTR __real_log10_e_lead

    vaddsd       xmm0,xmm0,xmm4
    vaddsd       xmm0,xmm0,xmm1
    vaddsd       xmm0,xmm0,xmm3

    AVXRestoreXmm  xmm6, save_xmm6
    StackDeallocate stack_size
    ret


Llog10_fma3_denormal_adjust:
    vpor         xmm2,xmm2,XMMWORD PTR __real_one
    vsubsd       xmm2,xmm2,QWORD PTR __real_one
    vpsrlq       xmm5,xmm2,52
    vpand        xmm2,xmm2,XMMWORD PTR __mask_mant
    vmovapd      xmm0,xmm2
    vpsubd       xmm5,xmm5,XMMWORD PTR __mask_2045
    vcvtdq2pd    xmm6,xmm5
    jmp          Llog10_fma3_continue_common

ALIGN  16
Llog10_fma3_x_is_zero_or_neg:
    jne          Llog10_fma3_x_is_neg
    vmovsd       xmm1,QWORD PTR __real_ninf
    mov          r8d,DWORD PTR __flag_x_zero
    call         fname_special

    AVXRestoreXmm  xmm6, save_xmm6
    StackDeallocate stack_size
    ret


ALIGN  16
Llog10_fma3_x_is_neg:

    vmovsd       xmm1,QWORD PTR __real_neg_qnan
    mov          r8d,DWORD PTR __flag_x_neg
    call         fname_special

    AVXRestoreXmm  xmm6, save_xmm6
    StackDeallocate stack_size
    ret


ALIGN  16
Llog10_fma3_x_is_inf_or_nan:

    cmp          rax,QWORD PTR __real_inf
    je           Llog10_fma3_finish

    cmp          rax,QWORD PTR __real_ninf
    je           Llog10_fma3_x_is_neg

    or           rax,QWORD PTR __real_qnanbit
    movd         xmm1,rax
    mov          r8d,DWORD PTR __flag_x_nan
    call         fname_special
    jmp          Llog10_fma3_finish

ALIGN  16
Llog10_fma3_finish:

    AVXRestoreXmm  xmm6, save_xmm6
    StackDeallocate stack_size
    ret
fname            endp

END

