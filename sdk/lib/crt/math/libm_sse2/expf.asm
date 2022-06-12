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
; expf.asm
;
; An implementation of the expf libm function.
;
; Prototype:
;
;     float expf(float x);
;

;
;   Algorithm:
;       Similar to one presnted in exp.asm
;
; If FMA3 hardware is available, an FMA3 implementation of expf will be used.


.const
ALIGN 16

__real_inf                      DD 7f800000h
                                DD 0
                                DQ 0

__real_ninf                     DD 0ff800000h
                                DD 0
                                DQ 0

__real_qnanbit                  DD 00400000h
                                DD 0
                                DQ 0

__real_zero                     DD 00000000h
                                DD 0
                                DQ 0

__real_p8192                    DQ 40c0000000000000h
                                DQ 0
__real_m9600                    DQ 0c0c2c00000000000h
                                DQ 0

__real_64_by_log2               DQ 40571547652b82feh ; 64/ln(2)
                                DQ 0
__real_log2_by_64               DQ 3f862e42fefa39efh ; log2_by_64
                                DQ 0

__real_1_by_6                   DQ 3fc5555555555555h ; 1/6
                                DQ 0
__real_1_by_2                   DQ 3fe0000000000000h ; 1/2
                                DQ 0

; these codes and the ones in the corresponding .c file have to match
__flag_x_nan            DD 00000001
__flag_y_zero           DD 00000002
__flag_y_inf            DD 00000003

EXTRN __two_to_jby64_table:QWORD
EXTRN __use_fma3_lib:DWORD

fname           TEXTEQU <expf>
fname_special   TEXTEQU <_expf_special>

; define local variable storage offsets

; make room for fname_special to save things
dummy_space     EQU    020h
stack_size      EQU    038h

include fm.inc

; external function
EXTERN fname_special:PROC

.code

ALIGN 16
PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    .ENDPROLOG

    ; Do this to avoid possible exceptions from a NaN argument.
    movd        edx, xmm0
    btr         edx,31
    cmp         edx, DWORD PTR __real_inf
    jge         Lexpf_x_is_inf_or_nan

    cmp          DWORD PTR __use_fma3_lib, 0
    jne          Lexpf_fma3

Lexpf_sse2:

    cvtss2sd    xmm0, xmm0

    ; x * (64/ln(2))
    movsd       xmm3, QWORD PTR __real_64_by_log2
    mulsd       xmm3, xmm0

    ; x <= 128*ln(2), ( x * (64/ln(2)) ) <= 64*128
    ; x > -150*ln(2), ( x * (64/ln(2)) ) > 64*(-150)
    comisd      xmm3, QWORD PTR __real_p8192
    jae         Lexpf_y_is_inf

    comisd      xmm3, QWORD PTR __real_m9600
    jb          Lexpf_y_is_zero

    ; n = int( x * (64/ln(2)) )
    cvtpd2dq    xmm4, xmm3
    lea         r10, __two_to_jby64_table
    cvtdq2pd    xmm1, xmm4

    ; r = x - n * ln(2)/64
    movsd       xmm2, QWORD PTR __real_log2_by_64
    mulsd       xmm2, xmm1
    movd        ecx, xmm4
    mov         rax, 3fh
    and         eax, ecx
    subsd       xmm0, xmm2
    movsd       xmm1, xmm0

    ; m = (n - j) / 64
    sub         ecx, eax
    sar         ecx, 6

    ; q
    movsd       xmm3, QWORD PTR __real_1_by_6
    mulsd       xmm3, xmm0
    mulsd       xmm0, xmm0
    addsd       xmm3, QWORD PTR __real_1_by_2
    mulsd       xmm0, xmm3
    addsd       xmm0, xmm1

    add         rcx, 1023
    shl         rcx, 52

    ; (f)*(1+q)
    movsd       xmm2, QWORD PTR [r10+rax*8]
    mulsd       xmm0, xmm2
    addsd       xmm0, xmm2

    movd        xmm1, rcx
    mulsd       xmm0, xmm1
    cvtsd2ss    xmm0, xmm0
 
Lexpf_final_check:
    StackDeallocate stack_size
    ret

ALIGN 16
Lexpf_y_is_zero:

    movss       xmm1, DWORD PTR __real_zero
    movd        xmm0, edx
    mov         r8d, DWORD PTR __flag_y_zero

    call        fname_special
    jmp         Lexpf_finish          

ALIGN 16
Lexpf_y_is_inf:

    movss       xmm1, DWORD PTR __real_inf
    movd        xmm0, edx
    mov         r8d, DWORD PTR __flag_y_inf

    call        fname_special
    jmp         Lexpf_finish      

ALIGN 16
Lexpf_x_is_inf_or_nan:

    cmp         edx, DWORD PTR __real_inf
    je          Lexpf_finish

    cmp         edx, DWORD PTR __real_ninf
    je          Lexpf_process_zero

    or          edx, DWORD PTR __real_qnanbit
    movd        xmm1, edx
    mov         r8d, DWORD PTR __flag_x_nan
    call        fname_special
    jmp         Lexpf_finish    

ALIGN 16
Lexpf_process_zero:
    movss       xmm0, DWORD PTR __real_zero
    jmp         Lexpf_final_check

ALIGN 16
Lexpf_finish:
    StackDeallocate stack_size
    ret


ALIGN 16
Lexpf_fma3:

    vcvtss2sd    xmm0, xmm0, xmm0

    ; x * (64/ln(2))
    vmulsd      xmm3, xmm0, QWORD PTR __real_64_by_log2

    ; x <= 128*ln(2), ( x * (64/ln(2)) ) <= 64*128
    ; x > -150*ln(2), ( x * (64/ln(2)) ) > 64*(-150)
    vcomisd     xmm3, QWORD PTR __real_p8192
    jae         Lexpf_fma3_y_is_inf

    vucomisd    xmm3, QWORD PTR __real_m9600
    jb          Lexpf_fma3_y_is_zero

    ; n = int( x * (64/ln(2)) )
    vcvtpd2dq   xmm4, xmm3
    lea         r10, __two_to_jby64_table
    vcvtdq2pd   xmm1, xmm4

    ; r = x - n * ln(2)/64
    vfnmadd231sd xmm0, xmm1, QWORD PTR __real_log2_by_64
    vmovd        ecx, xmm4
    mov          rax, 3fh
    and          eax, ecx
    vmovapd      xmm1, xmm0               ; xmm1 <-- copy of r

    ; m = (n - j) / 64
    sub          ecx, eax
    sar          ecx, 6

    ; q
    vmovsd       xmm3, QWORD PTR __real_1_by_6
    vmulsd       xmm0, xmm0, xmm0         ; xmm0 <-- r^2
    vfmadd213sd  xmm3, xmm1, QWORD PTR __real_1_by_2 ; xmm3 <-- r/6 + 1/2
    vfmadd213sd  xmm0, xmm3, xmm1         ; xmm0 <-- q = r^2*(r/6 + 1/2) + r

    add         rcx, 1023
    shl         rcx, 52

    ; (f)*(1+q)
    vmovsd       xmm2, QWORD PTR [r10+rax*8]
    vfmadd213sd  xmm0, xmm2, xmm2

    vmovq        xmm2,rcx
    vmulsd       xmm0, xmm0, xmm2
    vcvtsd2ss    xmm0, xmm0, xmm0
 
Lexpf_fma3_final_check:
    StackDeallocate stack_size
    ret

ALIGN 16
Lexpf_fma3_y_is_zero:

    vmovss       xmm1, DWORD PTR __real_zero
    vmovd        xmm0, edx
    mov          r8d, DWORD PTR __flag_y_zero

    call         fname_special
    jmp          Lexpf_fma3_finish          

ALIGN 16
Lexpf_fma3_y_is_inf:

    vmovss       xmm1, DWORD PTR __real_inf
    vmovd        xmm0, edx
    mov          r8d, DWORD PTR __flag_y_inf

    call         fname_special
    jmp          Lexpf_fma3_finish      

ALIGN 16
Lexpf_fma3_process_zero:
    vmovss       xmm0, DWORD PTR __real_zero
    jmp          Lexpf_fma3_final_check

ALIGN 16
Lexpf_fma3_finish:
    StackDeallocate stack_size
    ret

fname endp

END
