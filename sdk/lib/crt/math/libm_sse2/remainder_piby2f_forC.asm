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
; This is a service routine for use by trig functions coded in C
;

fname TEXTEQU <__remainder_piby2d2f_forC>

save_rdi        EQU      20h
save_rsi        EQU      30h
stack_size      EQU     088h
include fm.inc

.code
PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    SaveReg rdi,save_rdi
    SaveReg rsi,save_rsi 
    .ENDPROLOG
 
 mov rdi, rcx
 mov rsi, rdx
 mov rdx, r8

 ;get the unbiased exponent and the mantissa part of x
 movd    xmm0,rdi
 lea    r9,L__2_by_pi_bits
 
 ;xexp = (x >> 52) - 1023
 movd   r11,xmm0
 mov    rcx,r11 
 shr    r11,52
 sub    r11,1023 ;r11 = xexp = exponent of input x 

 ;calculate the last byte from which to start multiplication
 ;last = 134 - (xexp >> 3) 
 mov    r10,r11
 shr    r10,3
 sub    r10,134 ;r10 = -last
 neg    r10 ;r10 = last

 ;load 64 bits of 2_by_pi
 mov    rax,[r9 + r10]
 mov    rdi,rdx ; save address of region since mul modifies rdx
 
 ;mantissa of x = ((x << 12) >> 12) | implied bit
 shl    rcx,12
 shr    rcx,12 ;rcx = mantissa part of input x 
 bts    rcx,52 ;add the implied bit as well 

 ;load next 128 bits of 2_by_pi 
 add    r10,8 ;increment to next 8 bytes of 2_by_pi
 movdqu xmm0,[r9 + r10] 

 ;do three 64-bit multiplications with mant of x 
 mul rcx
 mov    r8,rax ;r8 = last 64 bits of multiplication = res1[2] 
 mov    r10,rdx ;r10 = carry
 movd   rax,xmm0
 mul rcx
 ;resexp = xexp & 7 
 and    r11,7 ;r11 = resexp = xexp & 7 = last 3 bits
 psrldq xmm0,8 
 add    rax,r10 ; add the previous carry
 adc    rdx,0
 mov    r9,rax ;r9 = next 64 bits of multiplication = res1[1]
 mov    r10,rdx ;r10 = carry
 movd   rax,xmm0
 mul    rcx
 add    r10,rax ;r10 = most significant 64 bits = res1[0]
 
 ;find the region 
 ;last three bits ltb = most sig bits >> (54 - resexp));  decimal point in last 18 bits == 8 lsb's in first 64 bits and 8 msb's in next 64 bits
 ;point_five = ltb & 01h;
 ;region = ((ltb >> 1) + point_five) & 3; 
 mov    rcx,54
 mov    rax,r10
 sub    rcx,r11
 xor    rdx,rdx ;rdx = sign of x(i.e first part of x * 2bypi) 
 shr    rax,cl 
 jnc    L__no_point_five
 ;;if there is carry.. then negate the result of multiplication
 not r10
 not r9
 not r8
 mov    rdx,08000000000000000h

ALIGN  16 
L__no_point_five:
 adc    rax,0
 and    rax,3
 mov    DWORD PTR[rdi],eax ;store region to memory

 ;calculate the number of integer bits and zero them out
 mov    rcx,r11 
 add    rcx,10 ;rcx = no. of integer bits
 shl    r10,cl
 shr    r10,cl ;r10 contains only mant bits
 sub    rcx,64 ;form the exponent
 mov    r11,rcx
 
 ;find the highest set bit
 bsr    rcx,r10
 jnz L__form_mantissa
 mov    r10,r9
 mov    r9,r8
 bsr    rcx,r10 ;rcx = hsb
 sub    r11,64
 
 
ALIGN  16 
L__form_mantissa:
 add    r11,rcx ;for exp of x
 sub    rcx,52 ;rcx = no. of bits to shift in r10 
 cmp    rcx,0
 jl L__hsb_below_52
 je L__form_numbers
 ;hsb above 52
 mov    r8,r10 ;previous contents of r8 not required
 shr    r10,cl ;r10 = mantissa of x with hsb at 52
 jmp L__form_numbers
 
ALIGN  16 
L__hsb_below_52:
 neg rcx
 mov    rax,r9
 shl    r10,cl
 shl    r9,cl
 sub    rcx,64
 neg rcx
 shr    rax,cl
 or    r10,rax
 
ALIGN  16
L__form_numbers:
 add    r11,1023
 btr    r10,52 ;remove the implied bit
 mov    rcx,r11
 or    r10,rdx ;put the sign 
 shl    rcx,52
 or    r10,rcx ;x is in r10
 
 movd    xmm0,r10 ;xmm0 = x
 mulsd    xmm0,L__piby2
 movsd    QWORD PTR[rsi],xmm0
 RestoreReg rsi,save_rsi
 RestoreReg rdi,save_rdi
 StackDeallocate stack_size 
 ret 
 
fname        endp

.const
ALIGN 16
L__piby2 DQ 03ff921fb54442d18h

ALIGN 16
L__2_by_pi_bits DB 224
  DB 241
  DB 27
  DB 193
  DB 12
  DB 88
  DB 33
  DB 116
  DB 53
  DB 126
  DB 196
  DB 126
  DB 237
  DB 175
  DB 169
  DB 75
  DB 74
  DB 41
  DB 222
  DB 231
  DB 28
  DB 244
  DB 236
  DB 197
  DB 151
  DB 175
  DB 31
  DB 235
  DB 158
  DB 212
  DB 181
  DB 168
  DB 127
  DB 121
  DB 154
  DB 253
  DB 24
  DB 61
  DB 221
  DB 38
  DB 44
  DB 159
  DB 60
  DB 251
  DB 217
  DB 180
  DB 125
  DB 180
  DB 41
  DB 104
  DB 45
  DB 70
  DB 188
  DB 188
  DB 63
  DB 96
  DB 22
  DB 120
  DB 255
  DB 95
  DB 226
  DB 127
  DB 236
  DB 160
  DB 228
  DB 247
  DB 46
  DB 126
  DB 17
  DB 114
  DB 210
  DB 231
  DB 76
  DB 13
  DB 230
  DB 88
  DB 71
  DB 230
  DB 4
  DB 249
  DB 125
  DB 209
  DB 154
  DB 192
  DB 113
  DB 166
  DB 19
  DB 18
  DB 237
  DB 186
  DB 212
  DB 215
  DB 8
  DB 162
  DB 251
  DB 156
  DB 166
  DB 196
  DB 114
  DB 172
  DB 119
  DB 248
  DB 115
  DB 72
  DB 70
  DB 39
  DB 168
  DB 187
  DB 36
  DB 25
  DB 128
  DB 75
  DB 55
  DB 9
  DB 233
  DB 184
  DB 145
  DB 220
  DB 134
  DB 21
  DB 239
  DB 122
  DB 175
  DB 142
  DB 69
  DB 249
  DB 7
  DB 65
  DB 14
  DB 241
  DB 100
  DB 86
  DB 138
  DB 109
  DB 3
  DB 119
  DB 211
  DB 212
  DB 71
  DB 95
  DB 157
  DB 240
  DB 167
  DB 84
  DB 16
  DB 57
  DB 185
  DB 13
  DB 230
  DB 139
  DB 2
  DB 0
  DB 0
  DB 0
  DB 0
  DB 0
  DB 0
  DB 0

END

