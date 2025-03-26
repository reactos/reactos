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
; An implementation of the tan function.
;
; Prototype:
;
;     double tan(double x);
;
;   Computes tan(x).
;   It will provide proper C99 return values,
;   but may not raise floating point status bits properly.
;   Based on the NAG C implementation.
;
; If FMA3 hardware is present, it will be used for the calculation.
;

.const
ALIGN 16
L_signbit      DQ 08000000000000000h
               DQ 08000000000000000h ; duplicate for pd

L_sign_mask    DQ 07FFFFFFFFFFFFFFFh
               DQ 07FFFFFFFFFFFFFFFh ; duplicate for pd

L_int_one      DQ 00000000000000001h
               DQ 00000000000000001h ; duplicate for pd

L_twobypi      DQ 03FE45F306DC9C883h
               DQ 03FE45F306DC9C883h ; duplicate for pd

L_point_333    DQ 03FD5555555555555h; 1/3
               DQ 03FD5555555555555h ; duplicate for pd

L_tan_p0       DQ 03FD7D50F6638564Ah ; 0.372379159759792203640806338901e0
               DQ 03FD7D50F6638564Ah ; duplicate for pd

L_tan_p2       DQ 0BF977C24C7569ABBh ; -0.229345080057565662883358588111e-1
               DQ 0BF977C24C7569ABBh ; duplicate for pd

L_tan_p4       DQ 03F2D5DAF289C385Ah ; 0.224044448537022097264602535574e-3
               DQ 03F2D5DAF289C385Ah ; duplicate for pd

L_tan_q0       DQ 03FF1DFCB8CAA40B8h ; 0.111713747927937668539901657944e1
               DQ 03FF1DFCB8CAA40B8h ; duplicate for pd

L_tan_q2       DQ 0BFE08046499EB90Fh ; -0.515658515729031149329237816945e0
               DQ 0BFE08046499EB90Fh ; duplicate for pd

L_tan_q4       DQ 03F9AB0F4F80A0ACFh ; 0.260656620398645407524064091208e-1
               DQ 03F9AB0F4F80A0ACFh ; duplicate for pd

L_tan_q6       DQ 0BF2E7517EF6D98F8h ; -0.232371494088563558304549252913e-3
               DQ 0BF2E7517EF6D98F8h ; duplicate for pd

L_half_mask    DQ 0ffffffff00000000h
               DQ 0ffffffff00000000h ; duplicate for pd

L_piby4_lead   DQ 03FE921FB54442D18h ; pi/4, high part
               DQ 03FE921FB54442D18h ; duplicate for pd

L_piby4_tail   DQ 03C81A62633145C06h ; pi/4, low parft
               DQ 03C81A62633145C06h ; duplicate for pd

; Different parts of argument reduction need different versions of pi/2

L_piby2_1      DQ 03FF921FB54400000h ; pi/2, high 33 bits
L_piby2_1tail  DQ 03DD0B4611A626331h ; pi/2, second 53 bits, overlaps...
L_piby2_2      DQ 03DD0B4611A600000h ; pi/2, second 33 bits
L_piby2_2tail  DQ 03BA3198A2E037073h ; pi/2, third 53 bits, overlaps...
L_piby2_3      DQ 03BA3198A2E000000h ; pi/2, third 33 bits
L_piby2_3tail  DQ 0397B839A252049C1h ; pi/2, fourth 53 bits

; end of pi/2 versions

L_two_to_neg_27     DQ 03e40000000000000h ; 2^-27
L_two_to_neg_13     DQ 03f20000000000000h ; 2^-13

L_inf_mask_64  DQ 07FF0000000000000h
L_point_five   DQ 03FE0000000000000h
L_point_68     DQ 03FE5C28F5C28F5C3h ; .68
L_n_point_68   DQ 0BFE5C28F5C28F5C3h ; -.68

L_zero         DQ -0000000000000000h ; 0.0
L_one          DQ 03FF0000000000000h ; 1.0
L_n_one        DQ 0BFF0000000000000h ; -1.0
L_two          DQ 04000000000000000h ; 2.0

L_moderate_arg_cw  DQ 0411E848000000000h ;  5.e5
L_moderate_arg_bdl DQ 0417312D000000000h ; 2e7, works for BDL

fname         TEXTEQU <tan>
fname_special TEXTEQU <_tan_special>

; local storage offsets
save_xmm6       EQU 020h
save_xmm7       EQU 030h
store_input     EQU 040h
save_r10        EQU 050h
dummy_space     EQU 060h
stack_size      EQU 088h

include fm.inc

EXTERN           __use_fma3_lib:DWORD
EXTERN           fname_special      : PROC
EXTERN           __remainder_piby2_fma3 : PROC
EXTERN           __remainder_piby2_fma3_bdl : PROC
EXTERN           __remainder_piby2_forAsm : PROC
EXTERN           _set_statfp   : PROC

.code
ALIGN 16
PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    SaveXmm      xmm6, save_xmm6
    SaveXmm      xmm7, save_xmm7
    .ENDPROLOG
    cmp          DWORD PTR __use_fma3_lib, 0
    jne          Ltan_fma3

Ltan_sse2:
    movd      rdx, xmm0                          ; really movq
    movaps    xmm6, xmm0
    mov       rcx, rdx
    btr       rcx, 63                            ; rcx <-- |x|

    cmp       rcx, L_piby4_lead
    ja        Ltan_abs_x_nle_pio4               ; branch if > pi/4 or NaN


    cmp       rcx, L_two_to_neg_13
    jae       Ltan_abs_x_ge_two_to_neg_13

    cmp       rcx, L_two_to_neg_27
    jae       Labs_x_ge_two_to_neg_27

    ; At this point tan(x) ~= x; if it's not exact, set the inexact flag

    test      rcx, rcx
    je        Ltan_return

    mov       ecx, 20h                    ; ecx <-- AMD_F_INEXACT
    call      _set_statfp
    movaps    xmm0, xmm6                  ; may be redundant, but xmm0 <-- x

    RestoreXmm   xmm7, save_xmm7
    RestoreXmm   xmm6, save_xmm6
    StackDeallocate stack_size
    ret       0

Labs_x_ge_two_to_neg_27:

    mulsd     xmm0, xmm0
    mulsd     xmm0, xmm6
    mulsd     xmm0, QWORD PTR L_point_333

    addsd     xmm0, xmm6

    RestoreXmm   xmm7, save_xmm7
    RestoreXmm   xmm6, save_xmm6
    StackDeallocate stack_size
    ret       0

Ltan_abs_x_ge_two_to_neg_13:
    xorps     xmm1, xmm1                  ; xmm1 <-- xx = 0
    xor       r8d, r8d                    ; r8 <-- recip flag = 0
    call      _tan_piby4

Ltan_return:
    RestoreXmm   xmm7, save_xmm7
    RestoreXmm   xmm6, save_xmm6
    StackDeallocate stack_size
    ret       0

Ltan_abs_x_nle_pio4:

    cmp       rcx, L_inf_mask_64          ; |x| uint >= +inf as uint ?
    jnae       Ltan_x_is_finite

    call      fname_special
    RestoreXmm   xmm7, save_xmm7
    RestoreXmm   xmm6, save_xmm6
    StackDeallocate stack_size
    ret

ALIGN 16
Ltan_x_is_finite:
    xor       r8d, r8d
    xor       r10, r10
    cmp       rcx, rdx
    setne     r10b                         ; r10 <-- x was negative flag
    andpd     xmm6, L_sign_mask

    movsd   xmm0, QWORD PTR L_moderate_arg_cw        ; currently 5e5
    comisd  xmm0, xmm6
    jbe     Ltan_x_is_very_large

Ltan_x_is_moderate:                                  ; unused label

    ; For these arguments we do a Cody-Waite reduction, subtracting the
    ; appropriate multiple of pi/2, using extra precision where x is close
    ; to an exact multiple of pi/2
    ; We special-case region setting for |x| <= 9pi/4
    ; It seems strange that this speeds things up, but it does

    mov       rdx, rcx

    mov       rax, 4616025215990052958           ; 400f6a7a2955385eH (5pi/4)
    shr       rdx, 52                            ; rdx <-- xexp
    cmp       rcx, rax
    ja        Labs_x_gt_5pio4

    mov       rax, 4612488097114038738           ; 4002d97c7f3321d2H (3pi/4)
    cmp       rcx, rax
    seta      r8b
    inc       r8d                                ; r8d <-- region (1 or 2)
    jmp       Lhave_region

Labs_x_gt_5pio4:
    mov       rax, 4619644535898419899           ; 401c463abeccb2bbH (9pi/4)
    cmp       rcx, rax
    ja        Lneed_region_computation
    mov       rax, 4617875976460412789           ; 4015fdbbe9bba775H (7pi/4)
    cmp       rcx, rax
    seta      r8b
    add       r8d, 3                             ; r8d <-- region (3 or 4)
    jmp       Lhave_region

ALIGN 16
Lneed_region_computation:
    movaps    xmm0, xmm6
    mulsd     xmm0, QWORD PTR L_twobypi
    addsd     xmm0, QWORD PTR L_point_five
    cvttsd2si r8d, xmm0                          ; r8d <-- region

Lhave_region:
    movd      xmm3, r8d
    cvtdq2pd  xmm3, xmm3

    movaps    xmm2, xmm3
    movaps    xmm0, xmm3
    mulsd     xmm0, QWORD PTR L_piby2_1
    mulsd     xmm2, QWORD PTR L_piby2_1tail ; xmm2 < rtail = npi2 * piby2_1tail
    subsd     xmm6, xmm0                    ; xmm6 <-- rhead = x - npi2*piby2_1

    ; If x is not too close to multiple of pi/2,
    ; we're essentially done with reduction
    ; If the exponent of rhead is not close to that of x,
    ; then most of x has been subtracted away in computing rhead;
    ; i.e., x is close to a multiple of pi/2.

    movd      rax, xmm6
    shr       rax, 52
    and       eax, 2047
    sub       rdx, rax                      ; rdx <-- exp diff of x vs rhead

    cmp       rdx, 15
    jbe       Ltan_have_rhead_rtail

    ; Oops, x is almost a multiple of pi/2.  Compute more bits of reduced x

    ;   t = rhead;
    ;   rtail = npi2 * piby2_2;
    ;   rhead  = t - rtail;
    ;   rtail  = npi2 * piby2_2tail - ((t - rhead) - rtail);

    movaps    xmm1, xmm6
    movaps    xmm0, xmm3

    movaps    xmm2, xmm3
    mulsd     xmm0, QWORD PTR L_piby2_2
    mulsd     xmm2, QWORD PTR L_piby2_2tail
    subsd     xmm6, xmm0
    subsd     xmm1, xmm6
    subsd     xmm1, xmm0
    subsd     xmm2, xmm1

    cmp       rdx, 48
    jbe       Ltan_have_rhead_rtail         ; We've done enough

    ; Wow, x is REALLY close to a multiple of pi/2.  Compute more bits.

    ;   t = rhead;
    ;   rtail = npi2 * piby2_3;
    ;   rhead  = t - rtail;
    ;   rtail  = npi2 * piby2_3tail - ((t - rhead) - rtail);

    movaps    xmm1, xmm6
    movaps    xmm0, xmm3
    movaps    xmm2, xmm3
    mulsd     xmm0, QWORD PTR L_piby2_3
    mulsd     xmm2, QWORD PTR L_piby2_3tail
    subsd     xmm6, xmm0                    ; xmm6 <-- rhead = t - rtail
    subsd     xmm1, xmm6                    ; xmm1 <-- t - rhead
    subsd     xmm1, xmm0                    ; xmm1 <-- ((t - rhead) - rtail)
    subsd     xmm2, xmm1                    ; xmm2 <-- final rtail

Ltan_have_rhead_rtail:

    ; At this point xmm6 has a suitable rhead, xmm2 a suitable rtail
    movaps  xmm0, xmm6                      ; xmm0 <-- copy of rhead

    ;   r = rhead - rtail
    ;   rr = (rhead - r) - rtail;
    ;   region = npi2 & 3;

    and       r8d, 3                        ; r8d <-- region
    subsd     xmm0, xmm2                    ; xmm0 <-- r = rhead - rtail
    subsd     xmm6, xmm0                    ; xmm6 <-- rhead - r
    subsd     xmm6, xmm2                    ; xmm6 <-- rr = (rhead - r) - rtail

Ltan_do_tan_computation:
    and       r8d, 1                        ; r8d <-- region & 1
    movaps    xmm1, xmm6
    call      _tan_piby4
    test      r10d, r10d
    je        Ltan_pos_return
    xorpd     xmm0, QWORD PTR L_signbit
Ltan_pos_return:
    RestoreXmm   xmm7, save_xmm7
    RestoreXmm   xmm6, save_xmm6
    StackDeallocate stack_size
    ret       0

ALIGN 16
Ltan_x_is_very_large:
    ; Reduce x into range [-pi/4,pi/4] (general case)
    movaps    xmm0, xmm6
    mov       QWORD PTR [rsp+save_r10], r10
    call      __remainder_piby2_forAsm      ; this call clobbers r10
    mov       r10, QWORD PTR [rsp+save_r10]
    movapd    xmm6,xmm1                     ; xmm6 <-- rr
    mov       r8d,eax                       ; r8d <-- region
    jmp       Ltan_do_tan_computation

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; From here on, it is assumed that the hardware supports FMA3 (and AVX).

ALIGN 16
Ltan_fma3:
    vmovq        r9,xmm0
    mov          rdx,r9               ; rdx <-- x
    btr          r9,63                ; r9 <-- |x|
    cmp          r9,L_piby4_lead
    jae          Ltan_fma3_absx_gt_pio4 ; Note that NaN will branch

Ltan_fma3_absx_le_pio4:
    ; no argument reduction is needed, so recip is 0, xx is 0.
    ; Note that this routine is not special-casing very small |x|
    vmovsd       xmm5,L_piby4_lead
    vmovsd       xmm6,L_piby4_tail
    vxorpd       xmm1,xmm1,xmm1        ; xx <-- 0.
    vxorpd       xmm7,xmm7,xmm7        ; transform <-- 0
    comisd       xmm0,L_point_68
    jbe          Ltan_fma3_small_x_le_point_68
Ltan_fma3_x_small_gt_point_68:
    vmovsd       xmm7,L_one            ; xmm7 <-- transform = 1.0
    vsubsd       xmm0,xmm5,xmm0        ; x = piby4_lead - x
    vaddsd       xmm0,xmm0,xmm6        ; xmm0 <-- x = x + xl = x + piby4_tail
    jmp          Ltan_fma3_compute_Remez_for_small_x
ALIGN 16
Ltan_fma3_small_x_le_point_68:
    comisd       xmm0,L_n_point_68
    jae          Ltan_fma3_compute_Remez_for_small_x
Ltan_fma3_small_x_lt_neg_point_68:
    vmovsd       xmm7,L_n_one          ; xmm7 <-- transform = -1.0
    vaddsd       xmm0,xmm5,xmm0        ; x = piby4_lead + x
    vaddsd       xmm0,xmm0,xmm6        ; xmm0 <-- x = x + xl = x + piby4_tail
Ltan_fma3_compute_Remez_for_small_x:
    ; At this point xmm0 holds x, possibly transformed

    ; now do core Remez rational approximation for x in [0,0.68]
    vmovsd       xmm4,L_tan_q6
    vmovsd       xmm3,L_tan_p4
    vmulsd       xmm2,xmm0,xmm0        ; xx is 0, so xmm2 <-- r = x*x
    vfmadd213sd  xmm4,xmm2,L_tan_q4
    vfmadd213sd  xmm3,xmm2,L_tan_p2
    vfmadd213sd  xmm4,xmm2,L_tan_q2
    vfmadd213sd  xmm3,xmm2,L_tan_p0    ; xmm3 <-- p2 (polynomial)
    vfmadd213sd  xmm4,xmm2,L_tan_q0    ; xmm4 <-- q3 (polynomial)
    vdivsd       xmm3,xmm3,xmm4        ; xmm3 <-- r3 = p2/q3
    vmulsd       xmm3,xmm3,xmm2        ; xmm3 <-- r * r3
    vfmadd132sd  xmm0,xmm0,xmm3        ; xx = 0, so xmm0 <-- t = x + x*(r*r3)
    comisd       xmm7,L_zero           ; did we transform x?
    ; if x was transformed, we need to transform t to get answer;
    ; if not, the answer is just t.
    je           Ltan_fma3_ext_piby4_zero
        
    ; x was transformed, so answer is +- (1. - 2.*t/(1.+t))
    ; (remember recip is 0 here)
    vmovsd       xmm3,L_one
    vaddsd       xmm4,xmm0,L_one       ; xmm4 <-- 1. + t
    vdivsd       xmm6,xmm0,xmm4        ; xmm6 <-- t / (1.+t)
    vfnmadd231sd xmm3,xmm6,L_two       ; xmm3 <-- 1. - 2.*t/(1.+t)
    vmulsd       xmm0,xmm3,xmm7        ; multiply by +- 1.
    
Ltan_fma3_ext_piby4_zero:
    ; restore volatile registers
    AVXRestoreXmm xmm7, save_xmm7
    AVXRestoreXmm xmm6, save_xmm6
    StackDeallocate stack_size
    ret          0

ALIGN 16
Ltan_fma3_absx_gt_pio4: ;;; come here if |x| > pi/4
    cmp          r9, L_inf_mask_64
    jae          Ltan_fma3_naninf

;Ltan_fma3_range_reduce:
    vmovapd      [store_input + rsp],xmm0 ; save copy of x
    vmovq        xmm0,r9               ; xmm0l <-- |x|
    cmp          r9,L_moderate_arg_bdl
    jge          Ltan_fma3_remainder_piby2  ; go elsewhere if |x| > 500000.

    ; Note that __remainder_piby2_fma3 and __remainder_piby2_fma3_bdl
    ; have calling conventions that differ from the C routine
    ; on input
    ;   |x| is in xmm0
    ; on output
    ;   z is in xmm0
    ;   zz is in xmm1
    ;   where z + zz = arg reduced |x| and zz is small compared to z
    ;   region of |x| is in rax

 Ltan_fma3_remainder_piby2_small:
    ; Boldo-Daumas-Li reduction for reasonably small |x|
    call         __remainder_piby2_fma3_bdl


Ltan_fma3_full_computation:
    ; we have done argument reduction; recip and xx may be nonzero
    ; x is in xmm0, xx is in xmm1
    ; recip is region & 1, and region is in rax.

    vmovsd       xmm5,L_piby4_lead
    vmovsd       xmm6,L_piby4_tail

    vxorpd       xmm7,xmm7,xmm7        ; transform <-- 0
    vcomisd      xmm0,L_point_68
    jbe          Ltan_fma3_full_x_le_point_68
Ltan_fma3_full_x_gt_point_68:
    vmovsd       xmm7,L_one            ; xmm7 <-- transform = 1.0
    vsubsd       xmm0,xmm5,xmm0        ; xmm0 <-- x = piby4_lead - x
    vsubsd       xmm2,xmm6,xmm1        ; xmm2 <-- xl = pibi4_tail - xx
    vaddsd       xmm0,xmm0,xmm2        ; xmm0 <-- x = x + xl
    vxorps       xmm1,xmm1,xmm1        ; xmm1 <-- xx  = 0
    jmp          Ltan_fma3_compute_Remez
ALIGN 16
Ltan_fma3_full_x_le_point_68:
    vcomisd      xmm0,L_n_point_68
    jae          Ltan_fma3_compute_Remez
Ltan_fma3_full_x_lt_neg_point_68:
    vmovsd       xmm7,L_n_one          ; xmm7 <-- transform = -1.0
    vaddsd       xmm0,xmm5,xmm0        ; x = piby4_lead + x
    vaddsd       xmm2,xmm6,xmm1        ; xmm2 <-- xl = piby4_tail + xx
    vaddsd       xmm0,xmm0,xmm2        ; xmm0 <-- x = x + xl
    vxorps       xmm1,xmm1,xmm1        ; xmm1 <-- xx  = 0

Ltan_fma3_compute_Remez:
    vmulsd       xmm2,xmm0,xmm0           ; xmm2 <-- x*x
    vmulsd       xmm5,xmm1,xmm0           ; xmm5 <-- x*xx
    vfmadd132sd  xmm5,xmm2,L_two          ; xmm5 <-- r = x*x + 2.*x*xx
    vmovsd       xmm2,L_tan_p4
    vfmadd213sd  xmm2,xmm5,L_tan_p2       ; xmm2 <-- p4*r+p2
    vfmadd213sd  xmm2,xmm5,L_tan_p0       ; xmm2 <-- p = (p4*r+p2)*r+p0
    vmovsd       xmm4,L_tan_q6
    vfmadd213sd  xmm4,xmm5,L_tan_q4       ; xmm4 <-- q6*r+q4
    vfmadd213sd  xmm4,xmm5,L_tan_q2       ; xmm4 <-- (q6*r+q4)*r+q2
    vfmadd213sd  xmm4,xmm5,L_tan_q0       ; xmm4 <-- q = ((q6*r+q4)*r+q2)*r+q0
    vdivsd       xmm2,xmm2,xmm4           ; xmm2 <-- p/q
    vmulsd       xmm2,xmm2,xmm5           ; xmm2 <-- r*p/q
    vfmadd213sd  xmm2,xmm0,xmm1           ; xmm2 <-- t2 = xx + x*r*(p/q)
    vaddsd       xmm1,xmm0,xmm2           ; xmm1 <-- t = (t1=x) + t2

    ; If |x| > .68 we transformed, and t is an approximation of
    ; tan(pi/4 +- (x+xx))
    ; otherwise, t is just tan(x+xx)
    vxorpd       xmm6,xmm6,xmm6
    vcomisd      xmm7,xmm6                ; did we transform? (|x| > .68) ?
    jz           Ltan_fma3_if_recip_set   ; if not, go check recip

Ltan_fma3_if_transfor_set:
    ; Because we transformed x+xx, we have to transform t before returning
    ; let transform be 1 for x > .68, -1 for x < -.68, then we return
    ; transform * (recip ? (2.*t/(t-1.) - 1.) : (1. - 2.*t/(1.+t)))
    vaddsd       xmm6,xmm1,xmm1           ; xmm6 <-- 2.*t
    vmovsd       xmm4,L_one
    vaddsd       xmm2,xmm1,xmm4           ; xmm2 <-- t+1    
    vsubsd       xmm5,xmm1,xmm4           ; xmm5 <-- t-1
    bt           rax,0
    jc           Ltan_fma3_transform_and_recip_set
    ; here recip is not set
    vaddsd       xmm2,xmm1,xmm4           ; xmm2 <-- t+1    
    vdivsd       xmm2,xmm1,xmm2           ; xmm2 <-- t/(t+1)
    vfnmadd132sd xmm2,xmm4,L_two          ; xmm2 <-- 1 - 2*t/(t+1)
    vmulsd       xmm1,xmm2,xmm7           ; xmm1 <-- transform*(1 - 2*t/(t+1))
    jmp          Ltan_fma3_exit_piby4
ALIGN 16
Ltan_fma3_transform_and_recip_set:
    ; here recip is set
    vsubsd       xmm2,xmm1,xmm4           ; xmm2 <-- t-1    
    vdivsd       xmm2,xmm1,xmm2           ; xmm2 <-- t/(t-1)
    vfmsub132sd  xmm2,xmm4,L_two          ; xmm2 <-- 2*t/(t-1) - 1
    vmulsd       xmm1,xmm2,xmm7           ; xmm1 <-- transform*(2*t/(t-1) - 1)
    jmp          Ltan_fma3_exit_piby4

ALIGN 16
Ltan_fma3_if_recip_set:
    ; Here we did not transform x and xx, but if we are in an odd quadrant
    ; we will need to return -1./(t1+t2), computed accurately
    ; (t=t1 is in xmm1, t2 is in xmm2)
    bt           rax,0
    jnc          Ltan_fma3_exit_piby4

    vandpd       xmm7,xmm1,L_half_mask    ; xmm7 <-- z1 = high bits of t
    vsubsd       xmm4,xmm7,xmm0           ; xmm4 <-- z1 - t1
    vsubsd       xmm4,xmm2,xmm4           ; xmm4 <-- z2 = t2 - (z1-t1)
    vmovsd       xmm2,L_n_one
    vdivsd       xmm2,xmm2,xmm1           ; xmm2 <-- trec = -1./t
    vandpd       xmm5,xmm2,L_half_mask    ; xmm5 <-- trec_top=high bits of trec
    vfmadd213sd  xmm7,xmm5,L_one ; xmm7 <-- trec_top*z1 + 1.
    vfmadd231sd  xmm7 ,xmm4,xmm5 ; xmm7 <-- z2*trec_top + (trec_top*z1 + 1.)
    vfmadd213sd  xmm7,xmm2,xmm5  ; xmm7 <-- u = trec_top + trec*(z2*trec_top + (trec_top*z1+1.))
    vmovapd      xmm1,xmm7       ; xmm1 <-- u

Ltan_fma3_exit_piby4:
    vmovapd      xmm0,xmm1        ; xmm0 <-- t, u, or v, as needed

    vmovapd      xmm1,[store_input + rsp]
    vandpd       xmm1,xmm1,L_signbit
    vxorpd       xmm0,xmm0,xmm1           ; tan(-x) = -tan(x)

    ; restore volatile registers
    AVXRestoreXmm   xmm7, save_xmm7
    AVXRestoreXmm   xmm6, save_xmm6
    StackDeallocate stack_size
    ret

ALIGN 16
Ltan_fma3_remainder_piby2:
    ; argument reduction for general x

    call         __remainder_piby2_fma3
    jmp          Ltan_fma3_full_computation


Ltan_fma3_naninf: ; here argument is +-Inf or NaN.  Special case.
    call         fname_special
    AVXRestoreXmm   xmm7, save_xmm7
    AVXRestoreXmm   xmm6, save_xmm6
    StackDeallocate stack_size
    ret

fname endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.const
tan_piby4_save_xmm6  EQU 030h
tan_piby4_stack_size EQU 048h
.code
ALIGN 16
_tan_piby4 PROC PRIVATE FRAME
    StackAllocate tan_piby4_stack_size
    SaveXmm      xmm6, tan_piby4_save_xmm6
    .ENDPROLOG

    ; Compute tangent for x+xx in [-pi/4,pi/4].
    ; xmm0 has x
    ; xmm1 has xx
    ; r8d has recip.  If recip is true, return -1/tan(x+xx) else tan(x+xx)

    xor       eax, eax

    comisd    xmm0, QWORD PTR L_point_68
    movaps    xmm3, xmm1
    movaps    xmm6, xmm0
    jbe       Ltan_piby4_x_le_point_68

    ; Here x > .68, so we transform x using the identity
    ;   tan(pi/4-x) = (1-tan(x))/(1+tan(x))

    movsd     xmm2, QWORD PTR L_piby4_lead
    mov       eax, 1                        ; eax <-- transform = 1
    subsd     xmm2, xmm0                    ; xmm2 <-- x = piby4_lead - x
    movsd     xmm0, QWORD PTR L_piby4_tail
    subsd     xmm0, xmm1                    ; xmm0 <-- xl = piby4_tail - xx
    movaps    xmm6, xmm2
    addsd     xmm6, xmm0                    ; xmm6 <-- x = x + xl
    xorps     xmm3,xmm3                     ; xmm3 <-- xx = 0.
    jmp       Ltan_piby4_do_remez

Ltan_piby4_x_le_point_68:
; 43   :   else if (x < -0.68)

    movsd     xmm0, QWORD PTR L_n_point_68
    comisd    xmm0, xmm6
    jbe       Ltan_piby4_do_remez           ; jump if x >= -.68

    ; Here x < -.68, so we transform x using the identity
    ; tan(x-pi/4) = (tan(x)-1)/(tan(x)+1)

    addsd     xmm6, QWORD PTR L_piby4_lead  ; xmm6 <-- x = piby4_lead + x
    addsd     xmm3, QWORD PTR L_piby4_tail  ; xmm3 <-- xl = piby4_tail + xx
    or        eax, -1                       ; eax <-- transform = -1
    addsd     xmm6, xmm3                    ; xmm6 <-- x = x + xl
    xorps     xmm3, xmm3                    ; xmm3 <-- xx = 0

Ltan_piby4_do_remez:

    ; Core Remez [2,3] approximation to tan(x+xx) on the interval [0,0.68].
    movaps    xmm0, xmm6
    movaps    xmm2, xmm6;
; An implementation of the tan function.
;
; Prototype:
;
;     double tan(double x);
;
;   Computes tan(x).
;   It will provide proper C99 return values,
;   but may not raise floating point status bits properly.
;   Based on the NAG C implementation.
;
;

    mulsd     xmm0, xmm6                    ; xmm0 <-- x*x
    addsd     xmm2, xmm2                    ; xmm2 <-- 2*x
    mulsd     xmm2, xmm3                    ; xmm2 <-- 2*x*xx
    addsd     xmm2, xmm0                    ; xmm2 <-- r = x*x + 2*x*xx

    ; Magic Remez approximation
    movaps    xmm0, xmm2
    movaps    xmm5, xmm2
    movaps    xmm1, xmm2
    mulsd     xmm5, QWORD PTR L_tan_p4
    mulsd     xmm1, QWORD PTR L_tan_q6
    mulsd     xmm0, xmm6
    addsd     xmm5, QWORD PTR L_tan_p2
    mulsd     xmm5, xmm2
    addsd     xmm5, QWORD PTR L_tan_p0
    mulsd     xmm5, xmm0
    movsd     xmm0, QWORD PTR L_tan_q4
    addsd     xmm0, xmm1
    mulsd     xmm0, xmm2
    addsd     xmm0, QWORD PTR L_tan_q2
    mulsd     xmm0, xmm2
    addsd     xmm0, QWORD PTR L_tan_q0
    divsd     xmm5, xmm0
    addsd     xmm5, xmm3                    ; xmm5 <-- t2

    test      eax, eax
    je        Ltan_piby4_transform_false

    addsd     xmm5, xmm6                    ; xmm5 <-- t = t1 + t2 = x + t2

    test      r8d, r8d
    je        Ltan_piby4_transform_true_recip_false

    ; Here transform and recip are both true.
    ;   return transform*(2*t/(t-1) - 1.0);

    movaps    xmm0, xmm5
    subsd     xmm5, QWORD PTR L_one
    movd      xmm1, eax
    addsd     xmm0, xmm0
    divsd     xmm0, xmm5
    cvtdq2pd  xmm1, xmm1
    subsd     xmm0, QWORD PTR L_one
    mulsd     xmm0, xmm1
    RestoreXmm      xmm6, tan_piby4_save_xmm6
    StackDeallocate tan_piby4_stack_size
    ret       0

Ltan_piby4_transform_true_recip_false:
    ; Here return transform*(1.0 - 2*t/(1+t));
    movsd     xmm0, QWORD PTR L_one
    movaps    xmm1, xmm5
    addsd     xmm5, xmm0
    addsd     xmm1, xmm1
    divsd     xmm1, xmm5
    subsd     xmm0, xmm1
    movd      xmm1, eax
    cvtdq2pd  xmm1, xmm1
    mulsd     xmm0, xmm1
    RestoreXmm      xmm6, tan_piby4_save_xmm6
    StackDeallocate tan_piby4_stack_size
    ret       0

Ltan_piby4_transform_false:
    test      r8d, r8d
    je        Ltan_piby4_atransform_false_recip_false

    ; Here transform is false but recip is true
    ; We return an accurate computation of -1.0/(t1 + t2).

    movsd     xmm4, QWORD PTR L_n_one
    movaps    xmm0, xmm5
    mov       rcx, -4294967296              ; ffffffff00000000H
    addsd     xmm0, xmm6
    movd      rax, xmm0                     ; really movq
    divsd     xmm4, xmm0
    and       rax, rcx
    movd      xmm3, rax                     ; really movq
    movaps    xmm1, xmm3
    subsd     xmm1, xmm6

    movd      rax, xmm4                     ; really movq
    subsd     xmm5, xmm1

    and       rax, rcx
    movd      xmm2, rax                     ; really movq

    ;   return trec_top + trec * ((1.0 + trec_top * z1) + trec_top * z2);

    movaps    xmm0, xmm2
    mulsd     xmm5, xmm2
    mulsd     xmm0, xmm3
    addsd     xmm0, QWORD PTR L_one
    addsd     xmm0, xmm5
    mulsd     xmm0, xmm4
    addsd     xmm0, xmm2

    RestoreXmm      xmm6, tan_piby4_save_xmm6
    StackDeallocate tan_piby4_stack_size
    ret       0

Ltan_piby4_atransform_false_recip_false:
    ; Here both transform and recip are false; we just return t1 + t2
    addsd     xmm5, xmm6
    movaps    xmm0, xmm5
    RestoreXmm      xmm6, tan_piby4_save_xmm6
    StackDeallocate tan_piby4_stack_size
    ret       0

_tan_piby4 endp
END 
