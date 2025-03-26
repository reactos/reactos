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
; An implementation of the tanf function using the fma3 instruction.
;
; Prototype:
;
;     float tanf(float x);
;
;   Computes tanf(x).
;   It will provide proper C99 return values,
;   but may not raise floating point status bits properly.
;   Based on the NAG C implementation.
;
.const
ALIGN 16
L_sign_mask    DQ 07FFFFFFFFFFFFFFFh
               DQ 07FFFFFFFFFFFFFFFh
L_twobypi      DQ 03FE45F306DC9C883h
               DQ 03FE45F306DC9C883h
L_int_three    DQ 00000000000000003h
               DQ 00000000000000003h
L_int_one      DQ 00000000000000001h
               DQ 00000000000000001h
L_signbit      DQ 08000000000000000h
               DQ 08000000000000000h

L_tanf         DQ 03FD8A8B0DA56CB17h    ; c0
               DQ 0BF919DBA6EFD6AADh    ; c1
               DQ 03FF27E84A3E73A2Eh    ; d0
               DQ 0BFE07266D7B3511Bh    ; d1
               DQ 03F92E29003C692D9h    ; d2

L_large_x_sse2 DQ 04160000000000000h    ; 8388608.
L_large_x_fma3 DQ 041E921FB40000000h    ; 3.373259264e9
L_point_333    DQ 03FD5555555555555h
L_mask_3e4     DQ 03e40000000000000h
L_mask_3f2     DQ 03f20000000000000h
L_point_five   DQ 03FE0000000000000h
L_piby2_1      DQ 03FF921FB54400000h
L_piby2_1tail  DQ 03DD0B4611A626331h
L_piby2_lead   DQ 03ff921fb54442d18h
L_n_one        DQ 0BFF0000000000000h
L_piby4        DQ 03fe921fb54442d18h
L_min_norm     DQ 00010000000000000h


L_inf_mask_32  DD 07F800000h
               DD 07F800000h

EXTRN __use_fma3_lib:DWORD
EXTRN __L_2_by_pi_bits:BYTE

fname           TEXTEQU <tanf>
fname_special   TEXTEQU <_tanf_special>

; define local variable storage offsets
; actually there aren't any, but we need to leave room for _tanf_special.
dummy_space     EQU 20h
stack_size      EQU 38h

include fm.inc

;Define name and any external functions being called
EXTERN           fname_special : PROC

.code
PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    .ENDPROLOG
    cmp          DWORD PTR __use_fma3_lib, 0
    jne          Ltanf_fma3

Ltanf_sse2:
    movd         eax,xmm0
    mov          r8d,L_inf_mask_32
    and          eax,r8d
    cmp          eax, r8d
    jz           Ltanf_sse2_naninf       

    cvtss2sd     xmm5,xmm0
    movd         r9,xmm5
    btr          r9,63                    ; r9 <-- |x|

    cmp          r9,L_piby4
    jg           Ltanf_sse2_range_reduce
    cmp          r9,L_mask_3f2 ; compare to 2^-13 = 0.0001220703125
    jge          Ltanf_sse2_compute_tanf_piby_4
    cmp          r9,L_mask_3e4 ; compare to 2^-27 = 7.4505805969238281e-009
    jge          Ltanf_sse2_compute_x_xxx_0_333       
    ; At this point tan(x) ~= x; if it's not exact, set the inexact flag.

    test         r9, r9
    je           Ltanf_sse2_exact_return
    movsd        xmm1, L_n_one
    addsd        xmm1, L_min_norm          ; set inexact
    
Ltanf_sse2_exact_return:
    StackDeallocate stack_size
    ret

ALIGN 16
Ltanf_sse2_compute_x_xxx_0_333:
    movapd       xmm2,xmm5
    mulsd        xmm2,xmm2                ; xmm2 <-- x^2
    movapd       xmm0,xmm2
    mulsd        xmm0,xmm5                ; xmm0 <-- x^3
    mulsd        xmm0,L_point_333
    addsd        xmm0,xmm5    ;  x + x*x*x*0.3333333333333333;
    jmp          Ltanf_sse2_return_s

ALIGN 16
Ltanf_sse2_compute_tanf_piby_4:
    movapd       xmm0,xmm5                ; xmm0 <-- x (as double)

    movapd       xmm1,xmm0
    mulsd        xmm1,xmm0                ; xmm1 <-- x*x

    movsd        xmm3,L_tanf+008h         ; xmm3 <-- c1
    mulsd        xmm3,xmm1                ; xmm3 <-- c1*x^2
    addsd        xmm3,L_tanf              ; xmm3 <-- c = c1*x^2 + c0

    movsd        xmm2,L_tanf+020h         ; xmm2 <-- d2
    mulsd        xmm2,xmm1                ; xmm2 <-- d2*x^2
    addsd        xmm2,L_tanf+018h         ; xmm2 <-- d2*x^2 + d1
    mulsd        xmm2,xmm1                ; xmm2 <-- (d2*x^2 + d1)*x^2
    addsd        xmm2,L_tanf+010h         ; xmm2 <-- d = (d2*x^2 + d1)*x^2 + d0
    divsd        xmm3,xmm2                ; xmm3 <-- c/d
    mulsd        xmm1,xmm0                ; xmm1 <-- x^3
    mulsd        xmm1,xmm3                ; xmm1 <-- x^3 * c/d
    addsd        xmm0,xmm1                ; xmm0 <-- x + x^3 * c/d
    jmp          Ltanf_sse2_return_s

Ltanf_sse2_range_reduce:
    movd         xmm0,r9
    cmp          r9,L_large_x_sse2
    jge          Ltanf_sse2_tanf_reduce_large
    
Ltanf_sse2_tanf_reduce_moderate:
    movapd       xmm1,xmm0
    andpd        xmm1,L_sign_mask
    movapd       xmm2,L_twobypi
    mulsd        xmm2,xmm1
    addsd        xmm2,L_point_five
    cvttpd2dq    xmm4,xmm2
    cvtdq2pd     xmm1,xmm4
    andpd        xmm4,L_int_three    ; xmm4 <-- region
    movapd       xmm2,xmm0

    movapd       xmm3,xmm1
    mulsd        xmm1,L_piby2_1
    subsd        xmm2,xmm1
    mulsd        xmm3,L_piby2_1tail  ; xmm3 rtail
    movapd       xmm0,xmm2
    subsd        xmm0,xmm3
    subsd        xmm2,xmm0
    movapd       xmm1,xmm2
    subsd        xmm1,xmm3
    jmp          Ltanf_sse2_exit_s

Ltanf_sse2_tanf_reduce_large:
    lea          r9,__L_2_by_pi_bits
    ;xexp = (x >> 52) 1023
    movd         r11,xmm0
    mov          rcx,r11
    shr          r11,52
    sub          r11,1023                 ; r11 <-- xexp = exponent of input x
    ;calculate the last byte from which to start multiplication
    ;last = 134 (xexp >> 3)
    mov          r10,r11
    shr          r10,3
    sub          r10,134                  ; r10 <-- -last
    neg          r10                      ; r10 <-- last
    ;load 64 bits of 2_by_pi
    mov          rax,[r9+r10]
    ;mantissa of x = ((x << 12) >> 12) | implied bit
    shl          rcx,12
    shr          rcx,12                   ; rcx <-- mantissa part of input x
    bts          rcx,52                   ; add the implied bit as well
    ;load next 128 bits of 2_by_pi
    add          r10,8                    ; increment to next 8 bytes of 2_by_pi
    movdqu       xmm0,[r9+r10]
    ;do three 64bit multiplications with mant of x
    mul          rcx
    mov          r8,rax                   ; r8 = last 64 bits of mul = res1[2]
    mov          r10,rdx                  ; r10 = carry
    vmovq        rax,xmm0
    mul          rcx
    ;resexp = xexp & 7
    and          r11,7                    ; r11 <-- resexp = last 3 bits of xexp
    psrldq       xmm0,8
    add          rax,r10                  ; add the previous carry
    adc          rdx,0
    mov          r9,rax                   ; r9 <-- next 64 bits of mul = res1[1]
    mov          r10,rdx                  ; r10 <-- carry
    movd         rax,xmm0
    mul          rcx
    add          r10,rax                 ;r10 = most sig 64 bits = res1[0]
    ;find the region
    ;last three bits ltb = most sig bits >> (54 resexp))
    ;  decimal point in last 18 bits == 8 lsb's in first 64 bits 
    ;  and 8 msb's in next 64 bits
    ;point_five = ltb & 01h;
    ;region = ((ltb >> 1) + point_five) & 3;
    mov          rcx,54
    mov          rax,r10
    sub          rcx,r11
    xor          rdx,rdx                  ;rdx = sign of x
    shr          rax,cl
    jnc          Ltanf_sse2_no_point_five_f
    ;;if there is carry.. then negate the result of multiplication
    not          r10
    not          r9
    not          r8
    mov          rdx,08000000000000000h
ALIGN 16
Ltanf_sse2_no_point_five_f:
    adc          rax,0
    and          rax,3
    movd         xmm4,eax                 ; xmm4 <-- region
    ;calculate the number of integer bits and zero them out
    mov          rcx,r11
    add          rcx,10                   ; rcx = no. of integer bits
    shl          r10,cl
    shr          r10,cl                   ; r10 contains only mant bits
    sub          rcx,64                   ; form the exponent
    mov          r11,rcx
    ;find the highest set bit
    bsr          rcx,r10
    jnz          Ltanf_sse2_form_mantissa_f
    mov          r10,r9
    mov          r9,r8
    mov          r8,0
    bsr          rcx,r10 ;rcx = hsb
    sub          r11,64
ALIGN 16
Ltanf_sse2_form_mantissa_f:
    add          r11,rcx                  ; for exp of x
    sub          rcx,52                   ; rcx = no. of bits to shift in r10
    cmp          rcx,0
    jl           Ltanf_sse2_hsb_below_52_f
    je           Ltanf_sse2_form_numbers_f
    ;hsb above 52
    mov          r8,r10
    shr          r10,cl                   ; r10 = mantissa of x with hsb at 52
    shr          r9,cl                    ; make space for bits from r10
    sub          rcx,64
    neg          rcx                      ; rcx = no of bits to shift r10
    shl          r8,cl
    or           r9,r8                    ; r9 = mantissa bits of xx
    jmp          Ltanf_sse2_form_numbers_f

ALIGN 16
Ltanf_sse2_hsb_below_52_f:
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
Ltanf_sse2_form_numbers_f:
    add          r11,1023
    btr          r10,52                   ; remove the implied bit
    mov          rcx,r11
    or           r10,rdx                  ; put the sign
    shl          rcx,52
    or           r10,rcx                  ; x is in r10
    movd         xmm0,r10                 ; xmm0 <-- x
    mulsd        xmm0,L_piby2_lead

Ltanf_sse2_exit_s:
    movd         eax,xmm4
    and          eax,1                    ; eax <-- region & 1
    movapd       xmm1,xmm0
    mulsd        xmm1,xmm0                ; xmm1 <-- x*x

    movsd        xmm3,L_tanf+008h         ; xmm3 <-- c1
    mulsd        xmm3,xmm1                ; xmm3 <-- c1*x^2
    addsd        xmm3,L_tanf              ; xmm3 <-- c = c1*x^2 + c0

    movsd        xmm2,L_tanf+020h         ; xmm2 <-- d2
    mulsd        xmm2,xmm1                ; xmm2 <-- d2*x^2
    addsd        xmm2,L_tanf+018h         ; xmm2 <-- d2*x^2 + d1
    mulsd        xmm2,xmm1                ; xmm2 <-- (d2*x^2 + d1)*x^2
    addsd        xmm2,L_tanf+010h         ; xmm2 <-- d = (d2*x^2 + d1)*x^2 + d0
    divsd        xmm3,xmm2                ; xmm3 <-- c/d
    mulsd        xmm1,xmm0                ; xmm1 <-- x^3
    mulsd        xmm1,xmm3                ; xmm1 <-- x^3 * c/d
    addsd        xmm0,xmm1                ; xmm0 <-- x + x^3 * c/d
    cmp          eax,01h
    jne          Ltanf_sse2_exit_tanpiby4
Ltanf_sse2_recip :
    movd         xmm3,L_n_one
    divsd        xmm3,xmm0
    movsd        xmm0,xmm3
Ltanf_sse2_exit_tanpiby4 :
    andpd        xmm5,L_signbit
    xorpd        xmm0,xmm5

Ltanf_sse2_return_s:
    cvtsd2ss     xmm0,xmm0         
Ltanf_sse2_return_c:
    StackDeallocate stack_size
    ret

Ltanf_sse2_naninf:
    call    fname_special
     StackDeallocate stack_size
    ret

ALIGN 16
Ltanf_fma3:
    vmovd        eax,xmm0
    mov          r8d,L_inf_mask_32
    and          eax,r8d
    cmp          eax, r8d
    jz           Ltanf_fma3_naninf       

    vcvtss2sd    xmm5,xmm0,xmm0
    vmovq        r9,xmm5
    btr          r9,63                    ; r9 <-- |x|

    cmp          r9,L_piby4
    jg           Ltanf_fma3_range_reduce
    cmp          r9,L_mask_3f2
    jge          Ltanf_fma3_compute_tanf_piby_4
    cmp          r9,L_mask_3e4
    jge          Ltanf_fma3_compute_x_xxx_0_333       
    jmp          Ltanf_fma3_return_c

Ltanf_fma3_compute_x_xxx_0_333:
    vmulsd       xmm2,xmm5,xmm5
    vmulsd       xmm0,xmm2,xmm5
    vfmadd132sd  xmm0,xmm5,L_point_333    ;  x + x*x*x*0.3333333333333333;
    jmp          Ltanf_fma3_return_s

Ltanf_fma3_compute_tanf_piby_4:
    vmovsd       xmm0,xmm5,xmm5
    vmulsd       xmm1,xmm0,xmm0
    vmovsd       xmm3,L_tanf+008h
    vfmadd213sd  xmm3,xmm1,L_tanf
    vmovsd       xmm2,L_tanf+020h
    vfmadd213sd  xmm2,xmm1,L_tanf+018h
    vfmadd213sd  xmm2,xmm1,L_tanf+010h
    vdivsd       xmm3,xmm3,xmm2
    vmulsd       xmm1,xmm1,xmm0
    vfmadd231sd  xmm0,xmm1,xmm3
    jmp          Ltanf_fma3_return_s

Ltanf_fma3_range_reduce:
    vmovq        xmm0,r9
    cmp          r9,L_large_x_fma3
    jge          Ltanf_fma3_tanf_reduce_large
    
Ltanf_fma3_tanf_reduce_moderate:
    vandpd       xmm1,xmm0,L_sign_mask
    vmovapd      xmm2,L_twobypi
    vfmadd213sd  xmm2,xmm1,L_point_five
    vcvttpd2dq   xmm2,xmm2
    vpmovsxdq    xmm1,xmm2
    vandpd       xmm4,xmm1,L_int_three    ; xmm4 <-- region
    vshufps      xmm1 ,xmm1,xmm1,8
    vcvtdq2pd    xmm1,xmm1
    vmovdqa      xmm2,xmm0
    vfnmadd231sd xmm2,xmm1,L_piby2_1      ; xmm2 rhead
    vmulsd       xmm3,xmm1,L_piby2_1tail  ; xmm3 rtail
    vsubsd       xmm0,xmm2,xmm3
    vsubsd       xmm2,xmm2,xmm0
    vsubsd       xmm1,xmm2,xmm3
    jmp          Ltanf_fma3_exit_s

Ltanf_fma3_tanf_reduce_large:
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
    sub          r10,134                  ; r10 <-- -last
    neg          r10                      ; r10 <-- last
    ;load 64 bits of 2_by_pi
    mov          rax,[r9+r10]
    ;mantissa of x = ((x << 12) >> 12) | implied bit
    shl          rcx,12
    shr          rcx,12                   ; rcx <-- mantissa part of input x
    bts          rcx,52                   ; add the implied bit as well
    ;load next 128 bits of 2_by_pi
    add          r10,8                    ; increment to next 8 bytes of 2_by_pi
    vmovdqu      xmm0,XMMWORD PTR[r9+r10]
    ;do three 64bit multiplications with mant of x
    mul          rcx
    mov          r8,rax                   ; r8 = last 64 bits of mul = res1[2]
    mov          r10,rdx                  ; r10 = carry
    vmovq        rax,xmm0
    mul          rcx
    ;resexp = xexp & 7
    and          r11,7                    ; r11 <-- resexp = last 3 bits of xexp
    vpsrldq      xmm0,xmm0,8
    add          rax,r10                  ; add the previous carry
    adc          rdx,0
    mov          r9,rax                   ; r9 <-- next 64 bits of mul = res1[1]
    mov          r10,rdx                  ; r10 <-- carry
    vmovq        rax,xmm0
    mul          rcx
    add          r10,rax                 ;r10 = most sig 64 bits = res1[0]
    ;find the region
    ;last three bits ltb = most sig bits >> (54 resexp))
    ;  decimal point in last 18 bits == 8 lsb's in first 64 bits 
    ;  and 8 msb's in next 64 bits
    ;point_five = ltb & 01h;
    ;region = ((ltb >> 1) + point_five) & 3;
    mov          rcx,54
    mov          rax,r10
    sub          rcx,r11
    xor          rdx,rdx                  ;rdx = sign of x
    shr          rax,cl
    jnc          Ltanf_fma3_no_point_five_f
    ;;if there is carry.. then negate the result of multiplication
    not          r10
    not          r9
    not          r8
    mov          rdx,08000000000000000h
ALIGN 16
Ltanf_fma3_no_point_five_f:
    adc          rax,0
    and          rax,3
    vmovd        xmm4,eax                 ; xmm4 <-- region
    ;calculate the number of integer bits and zero them out
    mov          rcx,r11
    add          rcx,10                   ; rcx = no. of integer bits
    shl          r10,cl
    shr          r10,cl                   ; r10 contains only mant bits
    sub          rcx,64                   ; form the exponent
    mov          r11,rcx
    ;find the highest set bit
    bsr          rcx,r10
    jnz          Ltanf_fma3_form_mantissa_f
    mov          r10,r9
    mov          r9,r8
    mov          r8,0
    bsr          rcx,r10 ;rcx = hsb
    sub          r11,64
ALIGN 16
Ltanf_fma3_form_mantissa_f:
    add          r11,rcx                  ; for exp of x
    sub          rcx,52                   ; rcx = no. of bits to shift in r10
    cmp          rcx,0
    jl           Ltanf_fma3_hsb_below_52_f
    je           Ltanf_fma3_form_numbers_f
    ;hsb above 52
    mov          r8,r10
    shr          r10,cl                   ; r10 = mantissa of x with hsb at 52
    shr          r9,cl                    ; make space for bits from r10
    sub          rcx,64
    neg          rcx                      ; rcx = no of bits to shift r10
    shl          r8,cl
    or           r9,r8                    ; r9 = mantissa bits of xx
    jmp          Ltanf_fma3_form_numbers_f

ALIGN 16
Ltanf_fma3_hsb_below_52_f:
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
Ltanf_fma3_form_numbers_f:
    add          r11,1023
    btr          r10,52                   ; remove the implied bit
    mov          rcx,r11
    or           r10,rdx                  ; put the sign
    shl          rcx,52
    or           r10,rcx                  ; x is in r10
    vmovq        xmm0,r10                 ; xmm0 <-- x
    vmulsd       xmm0,xmm0,L_piby2_lead

Ltanf_fma3_exit_s:
    vandpd       xmm2,xmm4,XMMWORD PTR L_int_one
    vmovd        eax,xmm2
    vmulsd       xmm1,xmm0,xmm0
    vmovsd       xmm3,L_tanf+008h
    vfmadd213sd  xmm3,xmm1,L_tanf
    vmovsd       xmm2,L_tanf+020h
    vfmadd213sd  xmm2,xmm1,L_tanf+018h
    vfmadd213sd  xmm2,xmm1,L_tanf+010h
    vdivsd       xmm3,xmm3,xmm2
    vmulsd       xmm1,xmm1,xmm0
    vfmadd231sd  xmm0,xmm1,xmm3
    cmp          eax,01h
    je           Ltanf_fma3_recip
    jmp          Ltanf_fma3_exit_tanpiby4

Ltanf_fma3_recip :
    vmovq        xmm3,L_n_one
    vdivsd       xmm0,xmm3,xmm0

Ltanf_fma3_exit_tanpiby4 :
    vandpd       xmm5,xmm5,L_signbit
    vxorpd       xmm0,xmm0,xmm5

Ltanf_fma3_return_s:
    vcvtsd2ss    xmm0,xmm0,xmm0         
Ltanf_fma3_return_c:
     StackDeallocate stack_size
    ret

Ltanf_fma3_naninf:
    call    fname_special
     StackDeallocate stack_size
    ret

fname endp
END 
