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
; pow.asm
;
; An implementation of the pow libm function.
;
; Prototype:
;
;     double pow(double x, double y);
;

;
;   Algorithm:
;       x^y = e^(y*ln(x))
;
;       Look in exp, log for the respective algorithms
;

.const

ALIGN 16

; these codes and the ones in the corresponding .c file have to match
__flag_x_one_y_snan             DD 00000001
__flag_x_zero_z_inf             DD 00000002
__flag_x_nan                    DD 00000003
__flag_y_nan                    DD 00000004
__flag_x_nan_y_nan              DD 00000005
__flag_x_neg_y_notint           DD 00000006
__flag_z_zero                   DD 00000007
__flag_z_denormal               DD 00000008
__flag_z_inf                    DD 00000009

ALIGN 16
   
__ay_max_bound              DQ 43e0000000000000h
__ay_min_bound              DQ 3c00000000000000h
__sign_mask                 DQ 8000000000000000h
__sign_and_exp_mask         DQ 0fff0000000000000h
__exp_mask                  DQ 7ff0000000000000h
__neg_inf                   DQ 0fff0000000000000h
__pos_inf                   DQ 7ff0000000000000h
__pos_one                   DQ 3ff0000000000000h
__pos_zero                  DQ 0000000000000000h
__exp_mant_mask             DQ 7fffffffffffffffh
__mant_mask                 DQ 000fffffffffffffh
__ind_pattern               DQ 0fff8000000000000h


__neg_qnan                  DQ 0fff8000000000000h
__qnan                      DQ 7ff8000000000000h
__qnan_set                  DQ 0008000000000000h

__neg_one                   DQ 0bff0000000000000h
__neg_zero                  DQ 8000000000000000h

__exp_shift                 DQ 0000000000000034h ; 52
__exp_bias                  DQ 00000000000003ffh ; 1023
__exp_bias_m1               DQ 00000000000003feh ; 1022

__yexp_53                   DQ 0000000000000035h ; 53
__mant_full                 DQ 000fffffffffffffh
__1_before_mant             DQ 0010000000000000h

__mask_mant_all8            DQ 000ff00000000000h
__mask_mant9                DQ 0000080000000000h



ALIGN 16
__real_fffffffff8000000     DQ 0fffffffff8000000h
                            DQ 0fffffffff8000000h

__mask_8000000000000000     DQ 8000000000000000h
                            DQ 8000000000000000h

__real_4090040000000000     DQ 4090040000000000h
                            DQ 4090040000000000h

__real_C090C80000000000     DQ 0C090C80000000000h
                            DQ 0C090C80000000000h

;---------------------
; log data
;---------------------

ALIGN 16

__real_ninf     DQ 0fff0000000000000h   ; -inf
                DQ 0000000000000000h
__real_inf      DQ 7ff0000000000000h    ; +inf
                DQ 0000000000000000h
__real_nan      DQ 7ff8000000000000h    ; NaN
                DQ 0000000000000000h
__real_mant     DQ 000FFFFFFFFFFFFFh    ; mantissa bits
                DQ 0000000000000000h
__mask_1023     DQ 00000000000003ffh
                DQ 0000000000000000h
__mask_001      DQ 0000000000000001h
                DQ 0000000000000000h

__real_log2_lead    DQ 3fe62e42e0000000h ; log2_lead  6.93147122859954833984e-01
                    DQ 0000000000000000h
__real_log2_tail    DQ 3e6efa39ef35793ch ; log2_tail  5.76999904754328540596e-08
                    DQ 0000000000000000h

__real_two          DQ 4000000000000000h ; 2
                    DQ 0000000000000000h

__real_one          DQ 3ff0000000000000h ; 1
                    DQ 0000000000000000h

__real_half         DQ 3fe0000000000000h ; 1/2
                    DQ 0000000000000000h

__mask_100          DQ 0000000000000100h
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
__real_1_over_7     DQ 3fc2492492492494h
                    DQ 0000000000000000h

__mask_1023_f       DQ 0c08ff80000000000h
                    DQ 0000000000000000h

__mask_2045         DQ 00000000000007fdh
                    DQ 0000000000000000h

__real_threshold    DQ 3fc0000000000000h ; 0.125
                    DQ 3fc0000000000000h

__real_notsign      DQ 7ffFFFFFFFFFFFFFh ; ^sign bit
                    DQ 0000000000000000h


EXTRN __log_256_lead:QWORD
EXTRN __log_256_tail:QWORD
EXTRN __use_fma3_lib:DWORD

; This table differs from the tables in log_256_lead_tail_table.asm:
; the heads have fewer significant bits (hence the tails also differ).
ALIGN 16
__log_F_inv_head    DQ 4000000000000000h
                    DQ 3fffe00000000000h
                    DQ 3fffc00000000000h
                    DQ 3fffa00000000000h
                    DQ 3fff800000000000h
                    DQ 3fff600000000000h
                    DQ 3fff400000000000h
                    DQ 3fff200000000000h
                    DQ 3fff000000000000h
                    DQ 3ffee00000000000h
                    DQ 3ffec00000000000h
                    DQ 3ffea00000000000h
                    DQ 3ffe900000000000h
                    DQ 3ffe700000000000h
                    DQ 3ffe500000000000h
                    DQ 3ffe300000000000h
                    DQ 3ffe100000000000h
                    DQ 3ffe000000000000h
                    DQ 3ffde00000000000h
                    DQ 3ffdc00000000000h
                    DQ 3ffda00000000000h
                    DQ 3ffd900000000000h
                    DQ 3ffd700000000000h
                    DQ 3ffd500000000000h
                    DQ 3ffd400000000000h
                    DQ 3ffd200000000000h
                    DQ 3ffd000000000000h
                    DQ 3ffcf00000000000h
                    DQ 3ffcd00000000000h
                    DQ 3ffcb00000000000h
                    DQ 3ffca00000000000h
                    DQ 3ffc800000000000h
                    DQ 3ffc700000000000h
                    DQ 3ffc500000000000h
                    DQ 3ffc300000000000h
                    DQ 3ffc200000000000h
                    DQ 3ffc000000000000h
                    DQ 3ffbf00000000000h
                    DQ 3ffbd00000000000h
                    DQ 3ffbc00000000000h
                    DQ 3ffba00000000000h
                    DQ 3ffb900000000000h
                    DQ 3ffb700000000000h
                    DQ 3ffb600000000000h
                    DQ 3ffb400000000000h
                    DQ 3ffb300000000000h
                    DQ 3ffb200000000000h
                    DQ 3ffb000000000000h
                    DQ 3ffaf00000000000h
                    DQ 3ffad00000000000h
                    DQ 3ffac00000000000h
                    DQ 3ffaa00000000000h
                    DQ 3ffa900000000000h
                    DQ 3ffa800000000000h
                    DQ 3ffa600000000000h
                    DQ 3ffa500000000000h
                    DQ 3ffa400000000000h
                    DQ 3ffa200000000000h
                    DQ 3ffa100000000000h
                    DQ 3ffa000000000000h
                    DQ 3ff9e00000000000h
                    DQ 3ff9d00000000000h
                    DQ 3ff9c00000000000h
                    DQ 3ff9a00000000000h
                    DQ 3ff9900000000000h
                    DQ 3ff9800000000000h
                    DQ 3ff9700000000000h
                    DQ 3ff9500000000000h
                    DQ 3ff9400000000000h
                    DQ 3ff9300000000000h
                    DQ 3ff9200000000000h
                    DQ 3ff9000000000000h
                    DQ 3ff8f00000000000h
                    DQ 3ff8e00000000000h
                    DQ 3ff8d00000000000h
                    DQ 3ff8b00000000000h
                    DQ 3ff8a00000000000h
                    DQ 3ff8900000000000h
                    DQ 3ff8800000000000h
                    DQ 3ff8700000000000h
                    DQ 3ff8600000000000h
                    DQ 3ff8400000000000h
                    DQ 3ff8300000000000h
                    DQ 3ff8200000000000h
                    DQ 3ff8100000000000h
                    DQ 3ff8000000000000h
                    DQ 3ff7f00000000000h
                    DQ 3ff7e00000000000h
                    DQ 3ff7d00000000000h
                    DQ 3ff7b00000000000h
                    DQ 3ff7a00000000000h
                    DQ 3ff7900000000000h
                    DQ 3ff7800000000000h
                    DQ 3ff7700000000000h
                    DQ 3ff7600000000000h
                    DQ 3ff7500000000000h
                    DQ 3ff7400000000000h
                    DQ 3ff7300000000000h
                    DQ 3ff7200000000000h
                    DQ 3ff7100000000000h
                    DQ 3ff7000000000000h
                    DQ 3ff6f00000000000h
                    DQ 3ff6e00000000000h
                    DQ 3ff6d00000000000h
                    DQ 3ff6c00000000000h
                    DQ 3ff6b00000000000h
                    DQ 3ff6a00000000000h
                    DQ 3ff6900000000000h
                    DQ 3ff6800000000000h
                    DQ 3ff6700000000000h
                    DQ 3ff6600000000000h
                    DQ 3ff6500000000000h
                    DQ 3ff6400000000000h
                    DQ 3ff6300000000000h
                    DQ 3ff6200000000000h
                    DQ 3ff6100000000000h
                    DQ 3ff6000000000000h
                    DQ 3ff5f00000000000h
                    DQ 3ff5e00000000000h
                    DQ 3ff5d00000000000h
                    DQ 3ff5c00000000000h
                    DQ 3ff5b00000000000h
                    DQ 3ff5a00000000000h
                    DQ 3ff5900000000000h
                    DQ 3ff5800000000000h
                    DQ 3ff5800000000000h
                    DQ 3ff5700000000000h
                    DQ 3ff5600000000000h
                    DQ 3ff5500000000000h
                    DQ 3ff5400000000000h
                    DQ 3ff5300000000000h
                    DQ 3ff5200000000000h
                    DQ 3ff5100000000000h
                    DQ 3ff5000000000000h
                    DQ 3ff5000000000000h
                    DQ 3ff4f00000000000h
                    DQ 3ff4e00000000000h
                    DQ 3ff4d00000000000h
                    DQ 3ff4c00000000000h
                    DQ 3ff4b00000000000h
                    DQ 3ff4a00000000000h
                    DQ 3ff4a00000000000h
                    DQ 3ff4900000000000h
                    DQ 3ff4800000000000h
                    DQ 3ff4700000000000h
                    DQ 3ff4600000000000h
                    DQ 3ff4600000000000h
                    DQ 3ff4500000000000h
                    DQ 3ff4400000000000h
                    DQ 3ff4300000000000h
                    DQ 3ff4200000000000h
                    DQ 3ff4200000000000h
                    DQ 3ff4100000000000h
                    DQ 3ff4000000000000h
                    DQ 3ff3f00000000000h
                    DQ 3ff3e00000000000h
                    DQ 3ff3e00000000000h
                    DQ 3ff3d00000000000h
                    DQ 3ff3c00000000000h
                    DQ 3ff3b00000000000h
                    DQ 3ff3b00000000000h
                    DQ 3ff3a00000000000h
                    DQ 3ff3900000000000h
                    DQ 3ff3800000000000h
                    DQ 3ff3800000000000h
                    DQ 3ff3700000000000h
                    DQ 3ff3600000000000h
                    DQ 3ff3500000000000h
                    DQ 3ff3500000000000h
                    DQ 3ff3400000000000h
                    DQ 3ff3300000000000h
                    DQ 3ff3200000000000h
                    DQ 3ff3200000000000h
                    DQ 3ff3100000000000h
                    DQ 3ff3000000000000h
                    DQ 3ff3000000000000h
                    DQ 3ff2f00000000000h
                    DQ 3ff2e00000000000h
                    DQ 3ff2e00000000000h
                    DQ 3ff2d00000000000h
                    DQ 3ff2c00000000000h
                    DQ 3ff2b00000000000h
                    DQ 3ff2b00000000000h
                    DQ 3ff2a00000000000h
                    DQ 3ff2900000000000h
                    DQ 3ff2900000000000h
                    DQ 3ff2800000000000h
                    DQ 3ff2700000000000h
                    DQ 3ff2700000000000h
                    DQ 3ff2600000000000h
                    DQ 3ff2500000000000h
                    DQ 3ff2500000000000h
                    DQ 3ff2400000000000h
                    DQ 3ff2300000000000h
                    DQ 3ff2300000000000h
                    DQ 3ff2200000000000h
                    DQ 3ff2100000000000h
                    DQ 3ff2100000000000h
                    DQ 3ff2000000000000h
                    DQ 3ff2000000000000h
                    DQ 3ff1f00000000000h
                    DQ 3ff1e00000000000h
                    DQ 3ff1e00000000000h
                    DQ 3ff1d00000000000h
                    DQ 3ff1c00000000000h
                    DQ 3ff1c00000000000h
                    DQ 3ff1b00000000000h
                    DQ 3ff1b00000000000h
                    DQ 3ff1a00000000000h
                    DQ 3ff1900000000000h
                    DQ 3ff1900000000000h
                    DQ 3ff1800000000000h
                    DQ 3ff1800000000000h
                    DQ 3ff1700000000000h
                    DQ 3ff1600000000000h
                    DQ 3ff1600000000000h
                    DQ 3ff1500000000000h
                    DQ 3ff1500000000000h
                    DQ 3ff1400000000000h
                    DQ 3ff1300000000000h
                    DQ 3ff1300000000000h
                    DQ 3ff1200000000000h
                    DQ 3ff1200000000000h
                    DQ 3ff1100000000000h
                    DQ 3ff1100000000000h
                    DQ 3ff1000000000000h
                    DQ 3ff0f00000000000h
                    DQ 3ff0f00000000000h
                    DQ 3ff0e00000000000h
                    DQ 3ff0e00000000000h
                    DQ 3ff0d00000000000h
                    DQ 3ff0d00000000000h
                    DQ 3ff0c00000000000h
                    DQ 3ff0c00000000000h
                    DQ 3ff0b00000000000h
                    DQ 3ff0a00000000000h
                    DQ 3ff0a00000000000h
                    DQ 3ff0900000000000h
                    DQ 3ff0900000000000h
                    DQ 3ff0800000000000h
                    DQ 3ff0800000000000h
                    DQ 3ff0700000000000h
                    DQ 3ff0700000000000h
                    DQ 3ff0600000000000h
                    DQ 3ff0600000000000h
                    DQ 3ff0500000000000h
                    DQ 3ff0500000000000h
                    DQ 3ff0400000000000h
                    DQ 3ff0400000000000h
                    DQ 3ff0300000000000h
                    DQ 3ff0300000000000h
                    DQ 3ff0200000000000h
                    DQ 3ff0200000000000h
                    DQ 3ff0100000000000h
                    DQ 3ff0100000000000h
                    DQ 3ff0000000000000h
                    DQ 3ff0000000000000h

ALIGN 16
__log_F_inv_tail    DQ 0000000000000000h
                    DQ 3effe01fe01fe020h
                    DQ 3f1fc07f01fc07f0h
                    DQ 3f31caa01fa11caah
                    DQ 3f3f81f81f81f820h
                    DQ 3f48856506ddaba6h
                    DQ 3f5196792909c560h
                    DQ 3f57d9108c2ad433h
                    DQ 3f5f07c1f07c1f08h
                    DQ 3f638ff08b1c03ddh
                    DQ 3f680f6603d980f6h
                    DQ 3f6d00f57403d5d0h
                    DQ 3f331abf0b7672a0h
                    DQ 3f506a965d43919bh
                    DQ 3f5ceb240795ceb2h
                    DQ 3f6522f3b834e67fh
                    DQ 3f6c3c3c3c3c3c3ch
                    DQ 3f3e01e01e01e01eh
                    DQ 3f575b8fe21a291ch
                    DQ 3f6403b9403b9404h
                    DQ 3f6cc0ed7303b5cch
                    DQ 3f479118f3fc4da2h
                    DQ 3f5ed952e0b0ce46h
                    DQ 3f695900eae56404h
                    DQ 3f3d41d41d41d41dh
                    DQ 3f5cb28ff16c69aeh
                    DQ 3f696b1edd80e866h
                    DQ 3f4372e225fe30d9h
                    DQ 3f60ad12073615a2h
                    DQ 3f6cdb2c0397cdb3h
                    DQ 3f52cc157b864407h
                    DQ 3f664cb5f7148404h
                    DQ 3f3c71c71c71c71ch
                    DQ 3f6129a21a930b84h
                    DQ 3f6f1e0387f1e038h
                    DQ 3f5ad4e4ba80709bh
                    DQ 3f6c0e070381c0e0h
                    DQ 3f560fba1a362bb0h
                    DQ 3f6a5713280dee96h
                    DQ 3f53f59620f9ece9h
                    DQ 3f69f22983759f23h
                    DQ 3f5478ac63fc8d5ch
                    DQ 3f6ad87bb4671656h
                    DQ 3f578b8efbb8148ch
                    DQ 3f6d0369d0369d03h
                    DQ 3f5d212b601b3748h
                    DQ 3f0b2036406c80d9h
                    DQ 3f629663b24547d1h
                    DQ 3f4435e50d79435eh
                    DQ 3f67d0ff2920bc03h
                    DQ 3f55c06b15c06b16h
                    DQ 3f6e3a5f0fd7f954h
                    DQ 3f61dec0d4c77b03h
                    DQ 3f473289870ac52eh
                    DQ 3f6a034da034da03h
                    DQ 3f5d041da2292856h
                    DQ 3f3a41a41a41a41ah
                    DQ 3f68550f8a39409dh
                    DQ 3f5b4fe5e92c0686h
                    DQ 3f3a01a01a01a01ah
                    DQ 3f691d2a2067b23ah
                    DQ 3f5e7c5dada0b4e5h
                    DQ 3f468a7725080ce1h
                    DQ 3f6c49d4aa21b490h
                    DQ 3f63333333333333h
                    DQ 3f54bc363b03fccfh
                    DQ 3f2c9f01970e4f81h
                    DQ 3f697617c6ef5b25h
                    DQ 3f6161f9add3c0cah
                    DQ 3f5319fe6cb39806h
                    DQ 3f2f693a1c451ab3h
                    DQ 3f6a9e240321a9e2h
                    DQ 3f63831f3831f383h
                    DQ 3f5949ebc4dcfc1ch
                    DQ 3f480c6980c6980ch
                    DQ 3f6f9d00c5fe7403h
                    DQ 3f69721ed7e75347h
                    DQ 3f6381ec0313381fh
                    DQ 3f5b97c2aec12653h
                    DQ 3f509ef3024ae3bah
                    DQ 3f38618618618618h
                    DQ 3f6e0184f00c2780h
                    DQ 3f692ef5657dba52h
                    DQ 3f64940305494030h
                    DQ 3f60303030303030h
                    DQ 3f58060180601806h
                    DQ 3f5017f405fd017fh
                    DQ 3f412a8ad278e8ddh
                    DQ 3f17d05f417d05f4h
                    DQ 3f6d67245c02f7d6h
                    DQ 3f6a4411c1d986a9h
                    DQ 3f6754d76c7316dfh
                    DQ 3f649902f149902fh
                    DQ 3f621023358c1a68h
                    DQ 3f5f7390d2a6c406h
                    DQ 3f5b2b0805d5b2b1h
                    DQ 3f5745d1745d1746h
                    DQ 3f53c31507fa32c4h
                    DQ 3f50a1fd1b7af017h
                    DQ 3f4bc36ce3e0453ah
                    DQ 3f4702e05c0b8170h
                    DQ 3f4300b79300b793h
                    DQ 3f3f76b4337c6cb1h
                    DQ 3f3a62681c860fb0h
                    DQ 3f36c16c16c16c17h
                    DQ 3f3490aa31a3cfc7h
                    DQ 3f33cd153729043eh
                    DQ 3f3473a88d0bfd2eh
                    DQ 3f36816816816817h
                    DQ 3f39f36016719f36h
                    DQ 3f3ec6a5122f9016h
                    DQ 3f427c29da5519cfh
                    DQ 3f4642c8590b2164h
                    DQ 3f4ab5c45606f00bh
                    DQ 3f4fd3b80b11fd3ch
                    DQ 3f52cda0c6ba4eaah
                    DQ 3f56058160581606h
                    DQ 3f5990d0a4b7ef87h
                    DQ 3f5d6ee340579d6fh
                    DQ 3f60cf87d9c54a69h
                    DQ 3f6310572620ae4ch
                    DQ 3f65798c8ff522a2h
                    DQ 3f680ad602b580adh
                    DQ 3f6ac3e24799546fh
                    DQ 3f6da46102b1da46h
                    DQ 3f15805601580560h
                    DQ 3f3ed3c506b39a23h
                    DQ 3f4cbdd3e2970f60h
                    DQ 3f55555555555555h
                    DQ 3f5c979aee0bf805h
                    DQ 3f621291e81fd58eh
                    DQ 3f65fead500a9580h
                    DQ 3f6a0fd5c5f02a3ah
                    DQ 3f6e45c223898adch
                    DQ 3f35015015015015h
                    DQ 3f4c7b16ea64d422h
                    DQ 3f57829cbc14e5e1h
                    DQ 3f60877db8589720h
                    DQ 3f65710e4b5edceah
                    DQ 3f6a7dbb4d1fc1c8h
                    DQ 3f6fad40a57eb503h
                    DQ 3f43fd6bb00a5140h
                    DQ 3f54e78ecb419ba9h
                    DQ 3f600a44029100a4h
                    DQ 3f65c28f5c28f5c3h
                    DQ 3f6b9c68b2c0cc4ah
                    DQ 3f2978feb9f34381h
                    DQ 3f4ecf163bb6500ah
                    DQ 3f5be1958b67ebb9h
                    DQ 3f644e6157dc9a3bh
                    DQ 3f6acc4baa3f0ddfh
                    DQ 3f26a4cbcb2a247bh
                    DQ 3f50505050505050h
                    DQ 3f5e0b4439959819h
                    DQ 3f66027f6027f602h
                    DQ 3f6d1e854b5e0db4h
                    DQ 3f4165e7254813e2h
                    DQ 3f576646a9d716efh
                    DQ 3f632b48f757ce88h
                    DQ 3f6ac1b24652a906h
                    DQ 3f33b13b13b13b14h
                    DQ 3f5490e1eb208984h
                    DQ 3f62385830fec66eh
                    DQ 3f6a45a6cc111b7eh
                    DQ 3f33813813813814h
                    DQ 3f556f472517b708h
                    DQ 3f631be7bc0e8f2ah
                    DQ 3f6b9cbf3e55f044h
                    DQ 3f40e7d95bc609a9h
                    DQ 3f59e6b3804d19e7h
                    DQ 3f65c8b6af7963c2h
                    DQ 3f6eb9dad43bf402h
                    DQ 3f4f1a515885fb37h
                    DQ 3f60eeb1d3d76c02h
                    DQ 3f6a320261a32026h
                    DQ 3f3c82ac40260390h
                    DQ 3f5a12f684bda12fh
                    DQ 3f669d43fda2962ch
                    DQ 3f02e025c04b8097h
                    DQ 3f542804b542804bh
                    DQ 3f63f69b02593f6ah
                    DQ 3f6df31cb46e21fah
                    DQ 3f5012b404ad012bh
                    DQ 3f623925e7820a7fh
                    DQ 3f6c8253c8253c82h
                    DQ 3f4b92ddc02526e5h
                    DQ 3f61602511602511h
                    DQ 3f6bf471439c9adfh
                    DQ 3f4a85c40939a85ch
                    DQ 3f6166f9ac024d16h
                    DQ 3f6c44e10125e227h
                    DQ 3f4cebf48bbd90e5h
                    DQ 3f62492492492492h
                    DQ 3f6d6f2e2ec0b673h
                    DQ 3f5159e26af37c05h
                    DQ 3f64024540245402h
                    DQ 3f6f6f0243f6f024h
                    DQ 3f55e60121579805h
                    DQ 3f668e18cf81b10fh
                    DQ 3f32012012012012h
                    DQ 3f5c11f7047dc11fh
                    DQ 3f69e878ff70985eh
                    DQ 3f4779d9fdc3a219h
                    DQ 3f61eace5c957907h
                    DQ 3f6e0d5b450239e1h
                    DQ 3f548bf073816367h
                    DQ 3f6694808dda5202h
                    DQ 3f37c67f2bae2b21h
                    DQ 3f5ee58469ee5847h
                    DQ 3f6c0233c0233c02h
                    DQ 3f514e02328a7012h
                    DQ 3f6561072057b573h
                    DQ 3f31811811811812h
                    DQ 3f5e28646f5a1060h
                    DQ 3f6c0d1284e6f1d7h
                    DQ 3f523543f0c80459h
                    DQ 3f663cbeea4e1a09h
                    DQ 3f3b9a3fdd5c8cb8h
                    DQ 3f60be1c159a76d2h
                    DQ 3f6e1d1a688e4838h
                    DQ 3f572044d72044d7h
                    DQ 3f691713db81577bh
                    DQ 3f4ac73ae9819b50h
                    DQ 3f6460334e904cf6h
                    DQ 3f31111111111111h
                    DQ 3f5feef80441fef0h
                    DQ 3f6de021fde021feh
                    DQ 3f57b7eacc9686a0h
                    DQ 3f69ead7cd391fbch
                    DQ 3f50195609804390h
                    DQ 3f6641511e8d2b32h
                    DQ 3f4222b1acf1ce96h
                    DQ 3f62e29f79b47582h
                    DQ 3f24f0d1682e11cdh
                    DQ 3f5f9bb096771e4dh
                    DQ 3f6e5ee45dd96ae2h
                    DQ 3f5a0429a0429a04h
                    DQ 3f6bb74d5f06c021h
                    DQ 3f54fce404254fceh
                    DQ 3f695766eacbc402h
                    DQ 3f50842108421084h
                    DQ 3f673e5371d5c338h
                    DQ 3f4930523fbe3368h
                    DQ 3f656b38f225f6c4h
                    DQ 3f426e978d4fdf3bh
                    DQ 3f63dd40e4eb0cc6h
                    DQ 3f397f7d73404146h
                    DQ 3f6293982cc98af1h
                    DQ 3f30410410410410h
                    DQ 3f618d6f048ff7e4h
                    DQ 3f2236a3ebc349deh
                    DQ 3f60c9f8ee53d18ch
                    DQ 3f10204081020408h
                    DQ 3f60486ca2f46ea6h
                    DQ 3ef0101010101010h
                    DQ 3f60080402010080h
                    DQ 0000000000000000h

;---------------------
; exp data
;---------------------

ALIGN 16

__denormal_threshold            DD 0fffffc02h ; -1022
                                DD 0
                                DQ 0

__enable_almost_inf             DQ 7fe0000000000000h
                                DQ 0

__real_zero                     DQ 0000000000000000h
                                DQ 0

__real_smallest_denormal        DQ 0000000000000001h
                                DQ 0
__denormal_tiny_threshold       DQ 0c0874046dfefd9d0h
                                DQ 0

__real_p65536                   DQ 40f0000000000000h    ; 65536
                                DQ 0
__real_m68800                   DQ 0c0f0cc0000000000h   ; -68800
                                DQ 0
__real_64_by_log2               DQ 40571547652b82feh    ; 64/ln(2)
                                DQ 0
__real_log2_by_64_head          DQ 3f862e42f0000000h    ; log2_by_64_head
                                DQ 0
__real_log2_by_64_tail          DQ 0bdfdf473de6af278h   ; -log2_by_64_tail
                                DQ 0
__real_1_by_720                 DQ 3f56c16c16c16c17h    ; 1/720
                                DQ 0
__real_1_by_120                 DQ 3f81111111111111h    ; 1/120
                                DQ 0
__real_1_by_24                  DQ 3fa5555555555555h    ; 1/24
                                DQ 0
__real_1_by_6                   DQ 3fc5555555555555h    ; 1/6
                                DQ 0
__real_1_by_2                   DQ 3fe0000000000000h    ; 1/2
                                DQ 0


EXTRN __two_to_jby64_head_table:QWORD
EXTRN __two_to_jby64_tail_table:QWORD
EXTRN __use_fma3_lib:DWORD

fname           TEXTEQU <pow>
fname_special   TEXTEQU <_pow_special>

; define local variable storage offsets

save_x          EQU     10h
save_y          EQU     20h
p_temp_exp      EQU     30h
negate_result   EQU     40h
save_ax         EQU     50h
y_head          EQU     60h
p_temp_log      EQU     70h
save_xmm6       EQU     080h
save_xmm7       EQU     090h
dummy_space     EQU     0a0h

stack_size      EQU     0c8h

include fm.inc

; external function
EXTERN fname_special:PROC

.code
ALIGN 16
PUBLIC fname
fname PROC FRAME
    StackAllocate stack_size
    SaveXmm xmm6, save_xmm6
    SaveXmm xmm7, save_xmm7
    .ENDPROLOG   
    cmp          DWORD PTR __use_fma3_lib, 0
    jne          Lpow_fma3

ALIGN 16
Lpow_sse2:
    movsd       QWORD PTR [save_x+rsp], xmm0
    movsd       QWORD PTR [save_y+rsp], xmm1

    mov         rdx, QWORD PTR [save_x+rsp]
    mov         r8, QWORD PTR [save_y+rsp]

    mov         r10, QWORD PTR __exp_mant_mask
    and         r10, r8
    jz          Lpow_sse2_y_is_zero

    cmp         r8, QWORD PTR __pos_one
    je          Lpow_sse2_y_is_one

    mov         r9, QWORD PTR __sign_mask
    and         r9, rdx
    mov         rax, QWORD PTR __pos_zero
    mov         QWORD PTR [negate_result+rsp], rax    
    cmp         r9, QWORD PTR __sign_mask
    je          Lpow_sse2_x_is_neg

    cmp         rdx, QWORD PTR __pos_one
    je          Lpow_sse2_x_is_pos_one

    cmp         rdx, QWORD PTR __pos_zero
    je          Lpow_sse2_x_is_zero

    mov         r9, QWORD PTR __exp_mask
    and         r9, rdx
    cmp         r9, QWORD PTR __exp_mask
    je          Lpow_sse2_x_is_inf_or_nan
   
    mov         r10, QWORD PTR __exp_mask
    and         r10, r8
    cmp         r10, QWORD PTR __ay_max_bound
    jg          Lpow_sse2_ay_is_very_large

    mov         r10, QWORD PTR __exp_mask
    and         r10, r8
    cmp         r10, QWORD PTR __ay_min_bound
    jl          Lpow_sse2_ay_is_very_small

    ; -----------------------------
    ; compute log(x) here
    ; -----------------------------
Lpow_sse2_log_x:

    ; compute exponent part
    xor         r8, r8
    movdqa      xmm3, xmm0
    psrlq       xmm3, 52
    movd        r8, xmm0
    psubq       xmm3, XMMWORD PTR __mask_1023
    movdqa      xmm2, xmm0
    cvtdq2pd    xmm6, xmm3 ; xexp
    pand        xmm2, XMMWORD PTR __real_mant

    comisd      xmm6, QWORD PTR __mask_1023_f
    je          Lpow_sse2_denormal_adjust

Lpow_sse2_continue_common:

    ; compute index into the log tables
    movsd       xmm7, xmm0
    mov         r9, r8
    and         r8, QWORD PTR __mask_mant_all8
    and         r9, QWORD PTR __mask_mant9
    subsd       xmm7, __real_one
    shl         r9, 1
    add         r8, r9
    mov         QWORD PTR [p_temp_log+rsp], r8
    andpd       xmm7, __real_notsign

    ; F, Y, switch to near-one codepath
    movsd       xmm1, QWORD PTR [p_temp_log+rsp]
    shr         r8, 44
    por         xmm2, XMMWORD PTR __real_half
    por         xmm1, XMMWORD PTR __real_half
    lea         r9, QWORD PTR __log_F_inv_head
    lea         rdx, QWORD PTR __log_F_inv_tail
    comisd      xmm7, __real_threshold
    jb          Lpow_sse2_near_one

    ; f = F - Y, r = f * inv
    subsd       xmm1, xmm2
    movsd       xmm4, xmm1
    mulsd       xmm1, QWORD PTR [r9+r8*8]
    movsd       xmm5, xmm1
    mulsd       xmm4, QWORD PTR [rdx+r8*8]
    movsd       xmm7, xmm4
    addsd       xmm1, xmm4

    movsd       xmm2, xmm1
    movsd       xmm0, xmm1
    lea         r9, __log_256_lead

    ; poly
    movsd       xmm3, QWORD PTR __real_1_over_6
    movsd       xmm1, QWORD PTR __real_1_over_3
    mulsd       xmm3, xmm2                         
    mulsd       xmm1, xmm2                         
    mulsd       xmm0, xmm2                         
    subsd       xmm5, xmm2
    movsd       xmm4, xmm0
    addsd       xmm3, QWORD PTR __real_1_over_5
    addsd       xmm1, QWORD PTR __real_1_over_2
    mulsd       xmm4, xmm0                         
    mulsd       xmm3, xmm2                         
    mulsd       xmm1, xmm0                         
    addsd       xmm3, QWORD PTR __real_1_over_4
    addsd       xmm7, xmm5
    mulsd       xmm3, xmm4                         
    addsd       xmm1, xmm3                         
    addsd       xmm1, xmm7

    movsd       xmm5, QWORD PTR __real_log2_tail
    lea         rdx, __log_256_tail
    mulsd       xmm5, xmm6
    movsd       xmm0, QWORD PTR [r9+r8*8]
    subsd       xmm5, xmm1

    movsd       xmm3, QWORD PTR [rdx+r8*8]
    addsd       xmm3, xmm5
    movsd       xmm1, xmm3
    subsd       xmm3, xmm2

    movsd       xmm7, QWORD PTR __real_log2_lead
    mulsd       xmm7, xmm6
    addsd       xmm0, xmm7

    ; result of ln(x) is computed from head and tail parts, resH and resT
    ; res = ln(x) = resH + resT
    ; resH and resT are in full precision 

    ; resT is computed from head and tail parts, resT_h and resT_t
    ; resT = resT_h + resT_t

    ; now
    ; xmm3 - resT
    ; xmm0 - resH
    ; xmm1 - (resT_t)
    ; xmm2 - (-resT_h)

Lpow_sse2_log_x_continue:

    movsd       xmm7, xmm0
    addsd       xmm0, xmm3
    movsd       xmm5, xmm0
    andpd       xmm0, XMMWORD PTR __real_fffffffff8000000
   
    ; xmm0 - H
    ; xmm7 - resH
    ; xmm5 - res

    mov         rax, QWORD PTR [save_y+rsp]
    and         rax, QWORD PTR __real_fffffffff8000000

    addsd       xmm2, xmm3
    subsd       xmm7, xmm5
    subsd       xmm1, xmm2
    addsd       xmm7, xmm3
    subsd       xmm5, xmm0

    mov         QWORD PTR [y_head+rsp], rax
    movsd       xmm4, QWORD PTR [save_y+rsp]
   
    addsd       xmm7, xmm1 
    addsd       xmm7, xmm5

    ; res = H + T
    ; H has leading 26 bits of precision
    ; T has full precision

    ; xmm0 - H
    ; xmm7 - T

    movsd       xmm2, QWORD PTR [y_head+rsp] 
    subsd       xmm4, xmm2

    ; y is split into head and tail
    ; for y * ln(x) computation

    ; xmm4 - Yt
    ; xmm2 - Yh
    ; xmm0 - H
    ; xmm7 - T

    movsd   xmm3, xmm4
    movsd   xmm5, xmm7
    movsd   xmm6, xmm0
    mulsd   xmm3, xmm7 ; YtRt
    mulsd   xmm4, xmm0 ; YtRh
    mulsd   xmm5, xmm2 ; YhRt
    mulsd   xmm6, xmm2 ; YhRh 

    movsd   xmm1, xmm6
    addsd   xmm3, xmm4
    addsd   xmm3, xmm5

    addsd   xmm1, xmm3
    movsd   xmm0, xmm1

    subsd   xmm6, xmm1
    addsd   xmm6, xmm3 

    ; y * ln(x) = v + vt
    ; v and vt are in full precision 
 
    ; xmm0 - v
    ; xmm6 - vt

    ; -----------------------------
    ; compute exp( y * ln(x) ) here
    ; -----------------------------

    ; v * (64/ln(2))
    movsd       xmm7, QWORD PTR __real_64_by_log2
    movsd       QWORD PTR [p_temp_exp+rsp], xmm0
    mulsd       xmm7, xmm0
    mov         rdx, QWORD PTR [p_temp_exp+rsp]

    ; v < 1024*ln(2), ( v * (64/ln(2)) ) < 64*1024
    ; v >= -1075*ln(2), ( v * (64/ln(2)) ) >= 64*(-1075)
    comisd      xmm7, QWORD PTR __real_p65536
    ja          Lpow_sse2_process_result_inf

    comisd      xmm7, QWORD PTR __real_m68800
    jb          Lpow_sse2_process_result_zero

    ; n = int( v * (64/ln(2)) )
    cvtpd2dq    xmm4, xmm7
    lea         r10, __two_to_jby64_head_table
    lea         r11, __two_to_jby64_tail_table
    cvtdq2pd    xmm1, xmm4

    ; r1 = x - n * ln(2)/64 head
    movsd       xmm2, QWORD PTR __real_log2_by_64_head
    mulsd       xmm2, xmm1
    movd        ecx, xmm4
    mov         rax, 3fh
    and         eax, ecx
    subsd       xmm0, xmm2

    ; r2 = - n * ln(2)/64 tail
    mulsd       xmm1, QWORD PTR __real_log2_by_64_tail
    movsd       xmm2, xmm0

    ; m = (n - j) / 64
    sub         ecx, eax
    sar         ecx, 6

    ; r1+r2
    addsd       xmm2, xmm1
    addsd       xmm2, xmm6 ; add vt here
    movsd       xmm1, xmm2

    ; q
    movsd       xmm0, QWORD PTR __real_1_by_2
    movsd       xmm3, QWORD PTR __real_1_by_24
    movsd       xmm4, QWORD PTR __real_1_by_720
    mulsd       xmm1, xmm2
    mulsd       xmm0, xmm2
    mulsd       xmm3, xmm2
    mulsd       xmm4, xmm2

    movsd       xmm5, xmm1
    mulsd       xmm1, xmm2
    addsd       xmm0, QWORD PTR __real_one
    addsd       xmm3, QWORD PTR __real_1_by_6
    mulsd       xmm5, xmm1
    addsd       xmm4, QWORD PTR __real_1_by_120
    mulsd       xmm0, xmm2
    mulsd       xmm3, xmm1
 
    mulsd       xmm4, xmm5

    ; deal with denormal results
    xor         r9d, r9d

    addsd       xmm3, xmm4
    addsd       xmm0, xmm3

    cmp         ecx, DWORD PTR __denormal_threshold
    cmovle      r9d, ecx
    add         rcx, 1023
    shl         rcx, 52

    ; f1, f2
    movsd       xmm5, QWORD PTR [r11+rax*8]
    movsd       xmm1, QWORD PTR [r10+rax*8]
    mulsd       xmm5, xmm0
    mulsd       xmm1, xmm0


    ; (f1+f2)*(1+q)
    addsd       xmm5, QWORD PTR [r11+rax*8]
    addsd       xmm1, xmm5
    addsd       xmm1, QWORD PTR [r10+rax*8]
    movsd       xmm0, xmm1

    cmp         rcx, QWORD PTR __real_inf
    je          Lpow_sse2_process_almost_inf

    mov         QWORD PTR [p_temp_exp+rsp], rcx
    test        r9d, r9d
    jnz         Lpow_sse2_process_denormal
    mulsd       xmm0, QWORD PTR [p_temp_exp+rsp]
    orpd        xmm0, XMMWORD PTR [negate_result+rsp]

Lpow_sse2_final_check:
    RestoreXmm   xmm7, save_xmm7
    RestoreXmm   xmm6, save_xmm6
    StackDeallocate stack_size
    ret

ALIGN 16
Lpow_sse2_process_almost_inf:
    comisd      xmm0, QWORD PTR __real_one
    jae         Lpow_sse2_process_result_inf

    orpd        xmm0, XMMWORD PTR __enable_almost_inf
    orpd        xmm0, XMMWORD PTR [negate_result+rsp]
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_process_denormal:
    mov         ecx, r9d
    xor         r11d, r11d
    comisd      xmm0, QWORD PTR __real_one
    cmovae      r11d, ecx
    cmp         r11d, DWORD PTR __denormal_threshold
    jne         Lpow_sse2_process_true_denormal  

    mulsd       xmm0, QWORD PTR [p_temp_exp+rsp]
    orpd        xmm0, XMMWORD PTR [negate_result+rsp]
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_process_true_denormal:
    xor         r8, r8
    mov         r9, 1
    cmp         rdx, QWORD PTR __denormal_tiny_threshold
    jg          Lpow_sse2_process_denormal_tiny
    add         ecx, 1074
    cmovs       rcx, r8
    shl         r9, cl
    mov         rcx, r9

    mov         QWORD PTR [p_temp_exp+rsp], rcx
    mulsd       xmm0, QWORD PTR [p_temp_exp+rsp]
    orpd        xmm0, XMMWORD PTR [negate_result+rsp]
    jmp         Lpow_sse2_z_denormal        

ALIGN 16
Lpow_sse2_process_denormal_tiny:
    movsd       xmm0, QWORD PTR __real_smallest_denormal
    orpd        xmm0, XMMWORD PTR [negate_result+rsp]
    jmp         Lpow_sse2_z_denormal

ALIGN 16
Lpow_sse2_process_result_zero:
    mov         r11, QWORD PTR __real_zero
    or          r11, QWORD PTR [negate_result+rsp]
    jmp         Lpow_sse2_z_is_zero_or_inf
 
ALIGN 16
Lpow_sse2_process_result_inf:
    mov         r11, QWORD PTR __real_inf
    or          r11, QWORD PTR [negate_result+rsp]
    jmp         Lpow_sse2_z_is_zero_or_inf

ALIGN 16
Lpow_sse2_denormal_adjust:
    por         xmm2, XMMWORD PTR __real_one
    subsd       xmm2, QWORD PTR __real_one
    movsd       xmm5, xmm2
    pand        xmm2, XMMWORD PTR __real_mant
    movd        r8, xmm2
    psrlq       xmm5, 52
    psubd       xmm5, XMMWORD PTR __mask_2045
    cvtdq2pd    xmm6, xmm5
    jmp         Lpow_sse2_continue_common

ALIGN 16
Lpow_sse2_x_is_neg:

    mov         r10, QWORD PTR __exp_mask
    and         r10, r8
    cmp         r10, QWORD PTR __ay_max_bound
    jg          Lpow_sse2_ay_is_very_large

    ; determine if y is an integer
    mov         r10, QWORD PTR __exp_mant_mask
    and         r10, r8
    mov         r11, r10
    mov         rcx, QWORD PTR __exp_shift
    shr         r10, cl
    sub         r10, QWORD PTR __exp_bias
    js          Lpow_sse2_x_is_neg_y_is_not_int
   
    mov         rax, QWORD PTR __exp_mant_mask
    and         rax, rdx
    mov         QWORD PTR [save_ax+rsp], rax

    mov         rcx, r10
    cmp         r10, QWORD PTR __yexp_53
    jg          Lpow_sse2_continue_after_y_int_check

    mov         r9, QWORD PTR __mant_full
    shr         r9, cl
    and         r9, r11
    jnz         Lpow_sse2_x_is_neg_y_is_not_int

    mov         r9, QWORD PTR __1_before_mant
    shr         r9, cl
    and         r9, r11
    jz          Lpow_sse2_continue_after_y_int_check

    mov         rax, QWORD PTR __sign_mask
    mov         QWORD PTR [negate_result+rsp], rax    

Lpow_sse2_continue_after_y_int_check:

    cmp         rdx, QWORD PTR __neg_zero
    je          Lpow_sse2_x_is_zero

    cmp         rdx, QWORD PTR __neg_one
    je          Lpow_sse2_x_is_neg_one

    mov         r9, QWORD PTR __exp_mask
    and         r9, rdx
    cmp         r9, QWORD PTR __exp_mask
    je          Lpow_sse2_x_is_inf_or_nan
   
    movsd       xmm0, QWORD PTR [save_ax+rsp]
    jmp         Lpow_sse2_log_x


ALIGN 16
Lpow_sse2_near_one:

    ; f = F - Y, r = f * inv
    movsd       xmm0, xmm1
    subsd       xmm1, xmm2
    movsd       xmm4, xmm1

    movsd       xmm3, QWORD PTR [r9+r8*8]
    addsd       xmm3, QWORD PTR [rdx+r8*8]
    mulsd       xmm4, xmm3
    andpd       xmm4, XMMWORD PTR __real_fffffffff8000000
    movsd       xmm5, xmm4 ; r1
    mulsd       xmm4, xmm0
    subsd       xmm1, xmm4
    mulsd       xmm1, xmm3
    movsd       xmm7, xmm1 ; r2
    addsd       xmm1, xmm5

    movsd       xmm2, xmm1
    movsd       xmm0, xmm1

    lea         r9, __log_256_lead

    ; poly
    movsd       xmm3, QWORD PTR __real_1_over_7
    movsd       xmm1, QWORD PTR __real_1_over_4
    mulsd       xmm3, xmm2
    mulsd       xmm1, xmm2
    mulsd       xmm0, xmm2
    movsd       xmm4, xmm0
    addsd       xmm3, QWORD PTR __real_1_over_6
    addsd       xmm1, QWORD PTR __real_1_over_3
    mulsd       xmm4, xmm0
    mulsd       xmm3, xmm2
    mulsd       xmm1, xmm2
    addsd       xmm3, QWORD PTR __real_1_over_5
    mulsd       xmm3, xmm2
    mulsd       xmm1, xmm0
    mulsd       xmm3, xmm4

    movsd       xmm2, xmm5
    movsd       xmm0, xmm7
    mulsd       xmm0, xmm0
    mulsd       xmm0, QWORD PTR __real_1_over_2
    mulsd       xmm5, xmm7
    addsd       xmm5, xmm0
    addsd       xmm5, xmm7

    movsd       xmm0, xmm2
    movsd       xmm7, xmm2
    mulsd       xmm0, xmm0
    mulsd       xmm0, QWORD PTR __real_1_over_2
    movsd       xmm4, xmm0
    addsd       xmm2, xmm0 ; r1 + r1^2/2
    subsd       xmm7, xmm2
    addsd       xmm7, xmm4

    addsd       xmm3, xmm7
    movsd       xmm4, QWORD PTR __real_log2_tail
    addsd       xmm1, xmm3
    mulsd       xmm4, xmm6
    lea         rdx, __log_256_tail
    addsd       xmm1, xmm5
    addsd       xmm4, QWORD PTR [rdx+r8*8]
    subsd       xmm4, xmm1

    movsd       xmm3, xmm4
    movsd       xmm1, xmm4
    subsd       xmm3, xmm2

    movsd       xmm0, QWORD PTR [r9+r8*8]
    movsd       xmm7, QWORD PTR __real_log2_lead
    mulsd       xmm7, xmm6
    addsd       xmm0, xmm7

    jmp         Lpow_sse2_log_x_continue


ALIGN 16
Lpow_sse2_x_is_pos_one:
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_y_is_zero:
    movsd       xmm0, QWORD PTR __real_one
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_y_is_one:
    xor         rax, rax
    mov         r11, rdx
    mov         r9, QWORD PTR __exp_mask
    ;or          r11, QWORD PTR __qnan_set
    and         r9, rdx
    cmp         r9, QWORD PTR __exp_mask
    cmove       rax, rdx
    mov         r9, QWORD PTR __mant_mask
    and         r9, rax
    jnz         Lpow_sse2_x_is_nan

    movd        xmm0, rdx 
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_x_is_neg_one:
    mov         rdx, QWORD PTR __pos_one
    or          rdx, QWORD PTR [negate_result+rsp]
    xor         rax, rax
    mov         r11, r8
    mov         r10, QWORD PTR __exp_mask
    ;or          r11, QWORD PTR __qnan_set
    and         r10, r8
    cmp         r10, QWORD PTR __exp_mask
    cmove       rax, r8
    mov         r10, QWORD PTR __mant_mask
    and         r10, rax
    jnz         Lpow_sse2_y_is_nan

    movd        xmm0, rdx
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_x_is_neg_y_is_not_int:
    mov         r9, QWORD PTR __exp_mask
    and         r9, rdx
    cmp         r9, QWORD PTR __exp_mask
    je          Lpow_sse2_x_is_inf_or_nan

    cmp         rdx, QWORD PTR __neg_zero
    je          Lpow_sse2_x_is_zero

    movsd       xmm0, QWORD PTR [save_x+rsp]
    movsd       xmm1, QWORD PTR [save_y+rsp]
    movsd       xmm2, QWORD PTR __neg_qnan
    mov         r9d, DWORD PTR __flag_x_neg_y_notint

    call        fname_special
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_ay_is_very_large:
    mov         r9, QWORD PTR __exp_mask
    and         r9, rdx
    cmp         r9, QWORD PTR __exp_mask
    je          Lpow_sse2_x_is_inf_or_nan

    mov         r9, QWORD PTR __exp_mant_mask
    and         r9, rdx
    jz          Lpow_sse2_x_is_zero 

    cmp         rdx, QWORD PTR __neg_one
    je          Lpow_sse2_x_is_neg_one

    mov         r9, rdx
    and         r9, QWORD PTR __exp_mant_mask
    cmp         r9, QWORD PTR __pos_one
    jl          Lpow_sse2_ax_lt1_y_is_large_or_inf_or_nan
  
    jmp         Lpow_sse2_ax_gt1_y_is_large_or_inf_or_nan

ALIGN 16
Lpow_sse2_x_is_zero:
    mov         r10, QWORD PTR __exp_mask
    xor         rax, rax
    and         r10, r8
    cmp         r10, QWORD PTR __exp_mask
    je          Lpow_sse2_x_is_zero_y_is_inf_or_nan

    mov         r10, QWORD PTR __sign_mask
    and         r10, r8
    cmovnz      rax, QWORD PTR __pos_inf
    jnz         Lpow_sse2_x_is_zero_z_is_inf

    movd        xmm0, rax
    orpd        xmm0, XMMWORD PTR [negate_result+rsp]
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_x_is_zero_z_is_inf:

    movsd       xmm0, QWORD PTR [save_x+rsp]
    movsd       xmm1, QWORD PTR [save_y+rsp]
    movd        xmm2, rax
    orpd        xmm2, XMMWORD PTR [negate_result+rsp]
    mov         r9d, DWORD PTR __flag_x_zero_z_inf

    call        fname_special
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_x_is_zero_y_is_inf_or_nan:
    mov         r11, r8
    cmp         r8, QWORD PTR __neg_inf
    cmove       rax, QWORD PTR __pos_inf
    je          Lpow_sse2_x_is_zero_z_is_inf

    ;or          r11, QWORD PTR __qnan_set
    mov         r10, QWORD PTR __mant_mask
    and         r10, r8
    jnz         Lpow_sse2_y_is_nan

    movd        xmm0, rax
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_x_is_inf_or_nan:
    xor         r11, r11
    mov         r10, QWORD PTR __sign_mask
    and         r10, r8
    cmovz       r11, QWORD PTR __pos_inf
    mov         rax, rdx
    mov         r9, QWORD PTR __mant_mask
    ;or          rax, QWORD PTR __qnan_set
    and         r9, rdx
    cmovnz      r11, rax
    jnz         Lpow_sse2_x_is_nan

    xor         rax, rax
    mov         r9, r8
    mov         r10, QWORD PTR __exp_mask
    ;or          r9, QWORD PTR __qnan_set
    and         r10, r8
    cmp         r10, QWORD PTR __exp_mask
    cmove       rax, r8
    mov         r10, QWORD PTR __mant_mask
    and         r10, rax
    cmovnz      r11, r9
    jnz         Lpow_sse2_y_is_nan

    movd        xmm0, r11
    orpd        xmm0, XMMWORD PTR [negate_result+rsp]
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_ay_is_very_small:
    movsd       xmm0, QWORD PTR __pos_one
    addsd       xmm0, xmm1
    jmp         Lpow_sse2_final_check


ALIGN 16
Lpow_sse2_ax_lt1_y_is_large_or_inf_or_nan:
    xor         r11, r11
    mov         r10, QWORD PTR __sign_mask
    and         r10, r8
    cmovnz      r11, QWORD PTR __pos_inf
    jmp         Lpow_sse2_adjust_for_nan

ALIGN 16
Lpow_sse2_ax_gt1_y_is_large_or_inf_or_nan:
    xor         r11, r11
    mov         r10, QWORD PTR __sign_mask
    and         r10, r8
    cmovz       r11, QWORD PTR __pos_inf

ALIGN 16
Lpow_sse2_adjust_for_nan:

    xor         rax, rax
    mov         r9, r8
    mov         r10, QWORD PTR __exp_mask
    ;or          r9, QWORD PTR __qnan_set
    and         r10, r8
    cmp         r10, QWORD PTR __exp_mask
    cmove       rax, r8
    mov         r10, QWORD PTR __mant_mask
    and         r10, rax
    cmovnz      r11, r9
    jnz         Lpow_sse2_y_is_nan

    test        rax, rax
    jnz         Lpow_sse2_y_is_inf

ALIGN 16
Lpow_sse2_z_is_zero_or_inf:

    mov         r9d, DWORD PTR __flag_z_zero
    test        r11, QWORD PTR __exp_mant_mask
    cmovnz      r9d, DWORD PTR __flag_z_inf
    
    movsd       xmm0, QWORD PTR [save_x+rsp]
    movsd       xmm1, QWORD PTR [save_y+rsp]
    movd        xmm2, r11

    call        fname_special
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_y_is_inf:

    movd        xmm0, r11
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_x_is_nan:

    xor         rax, rax
    mov         r10, QWORD PTR __exp_mask
    and         r10, r8
    cmp         r10, QWORD PTR __exp_mask
    cmove       rax, r8
    mov         r10, QWORD PTR __mant_mask
    and         r10, rax
    jnz         Lpow_sse2_x_is_nan_y_is_nan

    movsd       xmm0, QWORD PTR [save_x+rsp]
    movsd       xmm1, QWORD PTR [save_y+rsp]
    movd        xmm2, r11
    mov         r9d, DWORD PTR __flag_x_nan

    call        fname_special
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_y_is_nan:

    movsd       xmm0, QWORD PTR [save_x+rsp]
    movsd       xmm1, QWORD PTR [save_y+rsp]
    movd        xmm2, r11
    mov         r9d, DWORD PTR __flag_y_nan

    call        fname_special
    jmp         Lpow_sse2_final_check

ALIGN 16
Lpow_sse2_x_is_nan_y_is_nan:

    mov         r9, r8
    
    cmp         r11, QWORD PTR __ind_pattern
    cmove       r11, r9
    je          Lpow_sse2_continue_xy_nan

    cmp         r9, QWORD PTR __ind_pattern
    cmove       r9, r11

    mov         r10, r9
    and         r10, QWORD PTR __sign_mask
    cmovnz      r9, r11

    mov         r10, r11
    and         r10, QWORD PTR __sign_mask
    cmovnz      r11, r9
    
Lpow_sse2_continue_xy_nan:    
    ;or          r11, QWORD PTR __qnan_set
    movsd       xmm0, QWORD PTR [save_x+rsp]
    movsd       xmm1, QWORD PTR [save_y+rsp]
    movd        xmm2, r11
    mov         r9d, DWORD PTR __flag_x_nan_y_nan

    call        fname_special
    jmp         Lpow_sse2_final_check  
    
ALIGN 16
Lpow_sse2_z_denormal:
    
    movsd       xmm2, xmm0
    movsd       xmm0, QWORD PTR [save_x+rsp]
    movsd       xmm1, QWORD PTR [save_y+rsp]
    mov         r9d, DWORD PTR __flag_z_denormal

    call        fname_special
    jmp         Lpow_sse2_final_check  

Lpow_fma3:
    vmovsd       QWORD PTR [save_x+rsp], xmm0
    vmovsd       QWORD PTR [save_y+rsp], xmm1

    mov          rdx, QWORD PTR [save_x+rsp]
    mov          r8, QWORD PTR [save_y+rsp]

    mov          r10, QWORD PTR __exp_mant_mask
    and          r10, r8
    jz           Lpow_fma3_y_is_zero

    cmp          r8, QWORD PTR __pos_one
    je           Lpow_fma3_y_is_one

    mov          r9, QWORD PTR __sign_mask
    and          r9, rdx
    cmp          r9, QWORD PTR __sign_mask
    mov          rax, QWORD PTR __pos_zero
    mov          QWORD PTR [negate_result+rsp], rax
    je           Lpow_fma3_x_is_neg

    cmp          rdx, QWORD PTR __pos_one
    je           Lpow_fma3_x_is_pos_one

    cmp          rdx, QWORD PTR __pos_zero
    je           Lpow_fma3_x_is_zero

    mov          r9, QWORD PTR __exp_mask
    and          r9, rdx
    cmp          r9, QWORD PTR __exp_mask
    je           Lpow_fma3_x_is_inf_or_nan

    mov          r10, QWORD PTR __exp_mask
    and          r10, r8
    cmp          r10, QWORD PTR __ay_max_bound
    jg           Lpow_fma3_ay_is_very_large

    mov          r10, QWORD PTR __exp_mask
    and          r10, r8
    cmp          r10, QWORD PTR __ay_min_bound
    jl           Lpow_fma3_ay_is_very_small

    ; -----------------------------
    ; compute log(x) here
    ; -----------------------------
Lpow_fma3_log_x:

    ; compute exponent part
    vpsrlq       xmm3, xmm0, 52
    vmovq        r8, xmm0
    vpsubq       xmm3, xmm3, XMMWORD PTR __mask_1023
    vcvtdq2pd    xmm6, xmm3 ; xexp
    vpand        xmm2, xmm0, XMMWORD PTR __real_mant

    vcomisd      xmm6, QWORD PTR __mask_1023_f
    je           Lpow_fma3_denormal_adjust

Lpow_fma3_continue_common:

    ; compute index into the log tables
    mov          r9, r8
    and          r8, QWORD PTR __mask_mant_all8
    and          r9, QWORD PTR __mask_mant9
    vsubsd       xmm7, xmm0, __real_one
    shl          r9, 1
    add          r8, r9
    vmovq        xmm1, r8
    vandpd       xmm7, xmm7, __real_notsign

    ; F, Y, switch to near-one codepath
    shr          r8, 44
    vpor         xmm2, xmm2, XMMWORD PTR __real_half
    vpor         xmm1, xmm1, XMMWORD PTR __real_half
    vcomisd      xmm7, __real_threshold
    lea          r9, QWORD PTR __log_F_inv_head
    lea          rdx, QWORD PTR __log_F_inv_tail
    jb           Lpow_fma3_near_one

    ; f = F - Y, r = f * inv
    vsubsd       xmm4, xmm1, xmm2          ; xmm4 <-- f = F - Y
    vmulsd       xmm1, xmm4, QWORD PTR [r9+r8*8] ; xmm1 <-- rhead = f*inv_head
    vmovapd      xmm5, xmm1                ; xmm5 <-- copy of rhead
    vmulsd       xmm4, xmm4, QWORD PTR [rdx+r8*8] ; xmm4 <-- rtail = f*inv_tail
    vmovapd      xmm7, xmm4                ; xmm7 <-- copy of rtail
    vaddsd       xmm1, xmm1, xmm4          ; xmm1 <-- r = rhead + rtail

    vmovapd      xmm2, xmm1                ; xmm2 <-- copy of r
    vmovapd      xmm0, xmm1                ; xmm1 <-- copy of r
    lea          r9, __log_256_lead

    ; poly
;    movsd       xmm3, QWORD PTR __real_1_over_6
;    movsd       xmm1, QWORD PTR __real_1_over_3
;    mulsd       xmm3, xmm2               ; r*1/6
;    mulsd       xmm1, xmm2               ; r*1/3
;    mulsd       xmm0, xmm2               ; r^2
;    subsd       xmm5, xmm2               ; xmm5 <-- rhead - r
;    movsd       xmm4, xmm0               ; xmm4 <-- copy of r^2
;    addsd       xmm3, QWORD PTR __real_1_over_5 ; xmm3 <-- r*1/6 + 1/5
;    addsd       xmm1, QWORD PTR __real_1_over_2 ; xmm1 <-- r*1/3 + 1/2
;    mulsd       xmm4, xmm0               ; xmm4 <-- r^4
;    mulsd       xmm3, xmm2               ; xmm3 <-- (r*1/6 + 1/5)*r
;    mulsd       xmm1, xmm0               ; xmm1 <-- (r*1/3 + 1/2)*r^2
;    addsd       xmm3, QWORD PTR __real_1_over_4 ; xmm3 <-- (r*1/6+1/5)*r + 1/4
;    addsd       xmm7, xmm5               ; xmm7 <-- rtail + (rhead - r)
;    mulsd       xmm3, xmm4               ; xmm3 <-- (r*1/6 + 1/5)*r^5 + r^4*1/4
;    addsd       xmm1, xmm3               ; xmm1 <-- poly down to r^2
;    addsd       xmm1, xmm7               ; xmm1 <-- poly + correction


    vsubsd       xmm3, xmm5, xmm2
    vmovsd       xmm1, QWORD PTR __real_1_over_6
    vmulsd       xmm0,xmm0,xmm0
    vaddsd       xmm3, xmm3, xmm7
    vfmadd213sd  xmm1, xmm2, QWORD PTR __real_1_over_5
    vfmadd213sd  xmm1, xmm2, QWORD PTR __real_1_over_4
    vfmadd213sd  xmm1, xmm2, QWORD PTR __real_1_over_3
    vfmadd213sd  xmm1, xmm2, QWORD PTR __real_1_over_2
    vfmadd213sd  xmm1, xmm0, xmm3

    vmovsd       xmm5, QWORD PTR __real_log2_tail
    lea          rdx, __log_256_tail
    vfmsub213sd  xmm5, xmm6, xmm1
    vmovsd       xmm0, QWORD PTR [r9+r8*8]

    vaddsd       xmm3, xmm5, QWORD PTR [rdx+r8*8]
    vmovapd      xmm1, xmm3
    vsubsd       xmm3, xmm3, xmm2

    vfmadd231sd  xmm0, xmm6, QWORD PTR __real_log2_lead

    ; result of ln(x) is computed from head and tail parts, resH and resT
    ; res = ln(x) = resH + resT
    ; resH and resT are in full precision

    ; resT is computed from head and tail parts, resT_h and resT_t
    ; resT = resT_h + resT_t

    ; now
    ; xmm3 - resT
    ; xmm0 - resH
    ; xmm1 - (resT_t)
    ; xmm2 - (-resT_h)

Lpow_fma3_log_x_continue:

    vmovapd      xmm7, xmm0
    vaddsd       xmm0, xmm0, xmm3
    vmovapd      xmm5, xmm0
    vandpd       xmm0, xmm0, XMMWORD PTR __real_fffffffff8000000

    ; xmm0 - H
    ; xmm7 - resH
    ; xmm5 - res

    mov          rax, QWORD PTR [save_y+rsp]
    and          rax, QWORD PTR __real_fffffffff8000000

    vaddsd       xmm2, xmm2, xmm3
    vsubsd       xmm7, xmm7, xmm5
    vsubsd       xmm1, xmm1, xmm2
    vaddsd       xmm7, xmm7, xmm3
    vsubsd       xmm5, xmm5, xmm0

    mov          QWORD PTR [y_head+rsp], rax
    vmovsd       xmm4, QWORD PTR [save_y+rsp]

    vaddsd       xmm7, xmm7, xmm1
    vaddsd       xmm7, xmm7, xmm5

    ; res = H + T
    ; H has leading 26 bits of precision
    ; T has full precision

    ; xmm0 - H
    ; xmm7 - T

    vmovsd       xmm2, QWORD PTR [y_head+rsp]
    vsubsd       xmm4, xmm4, xmm2

    ; y is split into head and tail
    ; for y * ln(x) computation

    ; xmm4 - Yt
    ; xmm2 - Yh
    ; xmm0 - H
    ; xmm7 - T

    vmulsd       xmm3, xmm4, xmm7 ; YtRt
    vmulsd       xmm4, xmm4, xmm0 ; YtRh
    vmulsd       xmm5, xmm7, xmm2 ; YhRt
    vmulsd       xmm6, xmm0, xmm2 ; YhRh

    vmovapd      xmm1, xmm6
    vaddsd       xmm3, xmm3, xmm4
    vaddsd       xmm3, xmm3, xmm5

    vaddsd       xmm1, xmm1, xmm3
    vmovapd      xmm0, xmm1

    vsubsd       xmm6, xmm6, xmm1
    vaddsd       xmm6, xmm6, xmm3

    ; y * ln(x) = v + vt
    ; v and vt are in full precision

    ; xmm0 - v
    ; xmm6 - vt

    ; -----------------------------
    ; compute exp( y * ln(x) ) here
    ; -----------------------------

    ; v * (64/ln(2))
    vmovsd       QWORD PTR [p_temp_exp+rsp], xmm0
    vmulsd       xmm7, xmm0, QWORD PTR __real_64_by_log2
    mov          rdx, QWORD PTR [p_temp_exp+rsp]

    ; v < 1024*ln(2), ( v * (64/ln(2)) ) < 64*1024
    ; v >= -1075*ln(2), ( v * (64/ln(2)) ) >= 64*(-1075)
    vcomisd      xmm7, QWORD PTR __real_p65536
    ja           Lpow_fma3_process_result_inf

    vcomisd      xmm7, QWORD PTR __real_m68800
    jb           Lpow_fma3_process_result_zero

    ; n = int( v * (64/ln(2)) )
    vcvtpd2dq    xmm4, xmm7
    lea          r10, __two_to_jby64_head_table
    lea          r11, __two_to_jby64_tail_table
    vcvtdq2pd    xmm1, xmm4

    ; r1 = x - n * ln(2)/64 head
    vfnmadd231sd xmm0, xmm1, QWORD PTR __real_log2_by_64_head
    vmovd        ecx, xmm4
    mov          rax, 3fh
    and          eax, ecx

    ; r2 = - n * ln(2)/64 tail
    vmulsd       xmm1, xmm1, QWORD PTR __real_log2_by_64_tail
    vmovapd      xmm2, xmm0

    ; m = (n - j) / 64
    sub          ecx, eax
    sar          ecx, 6

    ; r1+r2
    vaddsd       xmm2, xmm2, xmm1
    vaddsd       xmm2, xmm2, xmm6 ; add vt here
    vmovapd      xmm1, xmm2

    ; q
    vmovsd       xmm0, QWORD PTR __real_1_by_720
    xor         r9d, r9d
    vfmadd213sd  xmm0, xmm2,  QWORD PTR __real_1_by_120
    cmp         ecx, DWORD PTR __denormal_threshold
    vfmadd213sd  xmm0, xmm2,  QWORD PTR __real_1_by_24
    cmovle      r9d, ecx
    vfmadd213sd  xmm0, xmm2,  QWORD PTR __real_1_by_6
    add         rcx, 1023
    vfmadd213sd  xmm0, xmm2,  QWORD PTR __real_1_by_2
    shl         rcx, 52
    vfmadd213sd  xmm0, xmm2,  QWORD PTR __real_one
    vmulsd       xmm0, xmm0, xmm2         ; xmm0 <-- q
;    movsd       xmm0, QWORD PTR __real_1_by_2
;    movsd       xmm3, QWORD PTR __real_1_by_24
;    movsd       xmm4, QWORD PTR __real_1_by_720
;    mulsd       xmm1, xmm2                ; xmm1 <-- r^2
;    mulsd       xmm0, xmm2                ; xmm0 <-- r/2
;    mulsd       xmm3, xmm2                ; xmm3 <-- r/24
;    mulsd       xmm4, xmm2                ; xmm4 <-- r/720

;    movsd       xmm5, xmm1                ; xmm5 <-- copy of r^2
;    mulsd       xmm1, xmm2                ; xmm1 <-- r^3
;    addsd       xmm0, QWORD PTR __real_one ; xmm0 <-- r/2 + 1
;    addsd       xmm3, QWORD PTR __real_1_by_6 ; xmm3 <-- r/24 + 1/6
;    mulsd       xmm5, xmm1                ; xmm5 <-- r^5
;    addsd       xmm4, QWORD PTR __real_1_by_120 ; xmm4 <-- r/720 + 1/120
;    mulsd       xmm0, xmm2                ; xmm0 <-- (r/2 + 1)*r
;    mulsd       xmm3, xmm1                ; xmm3 <-- (r/24 + 1/6)*r^3

;    mulsd       xmm4, xmm5                ; xmm4 <-- (r/720 + 1/120)*r^5

;   ; deal with denormal results
;   xor         r9d, r9d
;   cmp         ecx, DWORD PTR __denormal_threshold

;    addsd       xmm3, xmm4  ; xmm3 <-- (r/720 + 1/120)*r^5 + (r/24 + 1/6)*r^3
;    addsd       xmm0, xmm3  ; xmm0 <-- poly

;   cmovle      r9d, ecx
;   add         rcx, 1023
;   shl         rcx, 52

    ; f1, f2
    vmulsd       xmm5, xmm0, QWORD PTR [r11+rax*8]
    vmulsd       xmm1, xmm0, QWORD PTR [r10+rax*8]

    cmp          rcx, QWORD PTR __real_inf

    ; (f1+f2)*(1+q)
    vaddsd       xmm5, xmm5, QWORD PTR [r11+rax*8]
    vaddsd       xmm1, xmm1, xmm5
    vaddsd       xmm1, xmm1, QWORD PTR [r10+rax*8]
    vmovapd      xmm0, xmm1

    je           Lpow_fma3_process_almost_inf

    test         r9d, r9d
    mov          QWORD PTR [p_temp_exp+rsp], rcx
    jnz          Lpow_fma3_process_denormal
    vmulsd       xmm0, xmm0, QWORD PTR [p_temp_exp+rsp]
    vorpd        xmm0, xmm0, XMMWORD PTR [negate_result+rsp]

Lpow_fma3_final_check:
    AVXRestoreXmm  xmm7, save_xmm7
    AVXRestoreXmm  xmm6, save_xmm6
    StackDeallocate stack_size
    ret

ALIGN 16
Lpow_fma3_process_almost_inf:
    vcomisd      xmm0, QWORD PTR __real_one
    jae          Lpow_fma3_process_result_inf

    vorpd        xmm0, xmm0, XMMWORD PTR __enable_almost_inf
    vorpd        xmm0, xmm0, XMMWORD PTR [negate_result+rsp]
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_process_denormal:
    mov          ecx, r9d
    xor          r11d, r11d
    vcomisd      xmm0, QWORD PTR __real_one
    cmovae       r11d, ecx
    cmp          r11d, DWORD PTR __denormal_threshold
    jne          Lpow_fma3_process_true_denormal

    vmulsd       xmm0, xmm0, QWORD PTR [p_temp_exp+rsp]
    vorpd        xmm0, xmm0, XMMWORD PTR [negate_result+rsp]
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_process_true_denormal:
    xor          r8, r8
    cmp          rdx, QWORD PTR __denormal_tiny_threshold
    mov          r9, 1
    jg           Lpow_fma3_process_denormal_tiny
    add          ecx, 1074
    cmovs        rcx, r8
    shl          r9, cl
    mov          rcx, r9

    mov          QWORD PTR [p_temp_exp+rsp], rcx
    vmulsd       xmm0, xmm0, QWORD PTR [p_temp_exp+rsp]
    vorpd        xmm0, xmm0, XMMWORD PTR [negate_result+rsp]
    jmp          Lpow_fma3_z_denormal

ALIGN 16
Lpow_fma3_process_denormal_tiny:
    vmovsd       xmm0, QWORD PTR __real_smallest_denormal
    vorpd        xmm0, xmm0, XMMWORD PTR [negate_result+rsp]
    jmp          Lpow_fma3_z_denormal

ALIGN 16
Lpow_fma3_process_result_zero:
    mov          r11, QWORD PTR __real_zero
    or           r11, QWORD PTR [negate_result+rsp]
    jmp          Lpow_fma3_z_is_zero_or_inf

ALIGN 16
Lpow_fma3_process_result_inf:
    mov          r11, QWORD PTR __real_inf
    or           r11, QWORD PTR [negate_result+rsp]
    jmp          Lpow_fma3_z_is_zero_or_inf

ALIGN 16
Lpow_fma3_denormal_adjust:
    vpor         xmm2, xmm2, XMMWORD PTR __real_one
    vsubsd       xmm2, xmm2, QWORD PTR __real_one
    vmovapd      xmm5, xmm2
    vpand        xmm2, xmm2, XMMWORD PTR __real_mant
    vmovq        r8, xmm2
    vpsrlq       xmm5, xmm5, 52
    vpsubd       xmm5, xmm5, XMMWORD PTR __mask_2045
    vcvtdq2pd    xmm6, xmm5
    jmp          Lpow_fma3_continue_common

ALIGN 16
Lpow_fma3_x_is_neg:

    mov          r10, QWORD PTR __exp_mask
    and          r10, r8
    cmp          r10, QWORD PTR __ay_max_bound
    jg           Lpow_fma3_ay_is_very_large

    ; determine if y is an integer
    mov          r10, QWORD PTR __exp_mant_mask
    and          r10, r8
    mov          r11, r10
    mov          rcx, QWORD PTR __exp_shift
    shr          r10, cl
    sub          r10, QWORD PTR __exp_bias
    js           Lpow_fma3_x_is_neg_y_is_not_int

    mov          rax, QWORD PTR __exp_mant_mask
    and          rax, rdx
    mov          QWORD PTR [save_ax+rsp], rax

    cmp          r10, QWORD PTR __yexp_53
    mov          rcx, r10
    jg           Lpow_fma3_continue_after_y_int_check

    mov          r9, QWORD PTR __mant_full
    shr          r9, cl
    and          r9, r11
    jnz          Lpow_fma3_x_is_neg_y_is_not_int

    mov          r9, QWORD PTR __1_before_mant
    shr          r9, cl
    and          r9, r11
    jz           Lpow_fma3_continue_after_y_int_check

    mov          rax, QWORD PTR __sign_mask
    mov          QWORD PTR [negate_result+rsp], rax

Lpow_fma3_continue_after_y_int_check:

    cmp          rdx, QWORD PTR __neg_zero
    je           Lpow_fma3_x_is_zero

    cmp          rdx, QWORD PTR __neg_one
    je           Lpow_fma3_x_is_neg_one

    mov          r9, QWORD PTR __exp_mask
    and          r9, rdx
    cmp          r9, QWORD PTR __exp_mask
    je           Lpow_fma3_x_is_inf_or_nan

    vmovsd       xmm0, QWORD PTR [save_ax+rsp]
    jmp          Lpow_fma3_log_x


ALIGN 16
Lpow_fma3_near_one:

    ; f = F - Y, r = f * inv
    vmovapd      xmm0, xmm1
    vsubsd       xmm1, xmm1, xmm2         ; xmm1 <-- f
    vmovapd      xmm4, xmm1               ; xmm4 <-- copy of f

    vmovsd       xmm3, QWORD PTR [r9+r8*8]
    vaddsd       xmm3, xmm3, QWORD PTR [rdx+r8*8]
    vmulsd       xmm4, xmm4, xmm3         ; xmm4 <-- r = f*inv
    vandpd       xmm4, xmm4, XMMWORD PTR __real_fffffffff8000000 ; r1
    vmovapd      xmm5, xmm4               ; xmm5 <-- copy of r1
;   mulsd        xmm4, xmm0               ; xmm4 <-- F*r1
;   subsd        xmm1, xmm4               ; xmm1 <-- f - F*r1
    vfnmadd231sd xmm1, xmm4, xmm0         ; xmm1 <-- f - F*r1
    vmulsd       xmm1, xmm1, xmm3         ; xmm1 <-- r2 = (f - F*r1)*inv
    vmovapd      xmm7, xmm1               ; xmm7 <-- copy of r2
    vaddsd       xmm1, xmm1, xmm5         ; xmm1 <-- r = r1 + r2

    vmovapd      xmm2, xmm1               ; xmm2 <-- copy of r
    vmovapd      xmm0, xmm1               ; xmm0 <-- copy of r

    lea          r9, __log_256_lead

    ; poly
    ; NOTE: Given the complicated corrections here,
    ; I'm afraid to mess with it too much - WAT
    vmovsd       xmm3, QWORD PTR __real_1_over_7
    vmovsd       xmm1, QWORD PTR __real_1_over_4
    vmulsd       xmm0, xmm0, xmm2         ; xmm0 <-- r^2
    vmovapd      xmm4, xmm0               ; xmm4 <-- copy of r^2
    vfmadd213sd  xmm3, xmm2, QWORD PTR __real_1_over_6 ; xmm3 <-- r/7 + 1/6
    vfmadd213sd  xmm1, xmm2, QWORD PTR __real_1_over_3 ; xmm1 <-- r/4 + 1/3
    vmulsd       xmm4, xmm4, xmm0         ; xmm4 <-- r^4
    vmulsd       xmm1, xmm1, xmm2         ; xmm1 <-- (r/4 + 1/3)*r
    vfmadd213sd  xmm3, xmm2, QWORD PTR __real_1_over_5 ; xmm3 <-- ((r/7 + 1/6)*r) + 1/5
    vmulsd       xmm3, xmm3, xmm2         ; xmm3 <-- (((r/7 + 1/6)*r) + 1/5)*r
    vmulsd       xmm1, xmm1, xmm0         ; xmm1 <-- ((r/4 + 1/3)*r)*r^2
    vmulsd       xmm3, xmm3, xmm4         ; xmm3 <-- ((((r/7 + 1/6)*r) + 1/5)*r)*r^4

    vmovapd      xmm2, xmm5               ; xmm2 <-- copy of r1
    vmovapd      xmm0, xmm7               ; xmm0 <-- copy of r2
    vmulsd       xmm0, xmm0, xmm0         ; xmm0 <-- r2^2
    vmulsd       xmm0, xmm0, QWORD PTR __real_1_over_2 ; xmm0 <-- r2^2/2
;   mulsd        xmm5, xmm7               ; xmm5 <-- r1*r2
;   addsd        xmm5, xmm0               ; xmm5 <-- r1*r2 + r2^2^2
    vfmadd213sd  xmm5, xmm7, xmm0         ; xmm5 <-- r1*r2 + r2^2^2
    vaddsd       xmm5, xmm5, xmm7         ; xmm5 <-- r1*r2 + r2^2/2 + r2

    vmovapd      xmm0, xmm2               ; xmm0 <-- copy of r1
    vmovapd      xmm7, xmm2               ; xmm7 <-- copy of r1
    vmulsd       xmm0, xmm0, xmm0         ; xmm0 <-- r1^2
    vmulsd       xmm0, xmm0, QWORD PTR __real_1_over_2 ; xmm0 <-- r1^2/2
    vmovapd      xmm4, xmm0               ; xmm4 <-- copy of r1^2/2
    vaddsd       xmm2, xmm2, xmm0         ; xmm2 <--  r1 + r1^2/2
    vsubsd       xmm7, xmm7, xmm2         ; xmm7 <-- r1 - (r1 + r1^2/2)
    vaddsd       xmm7, xmm7, xmm4         ; xmm7 <-- r1 - (r1 + r1^2/2) + r1^2/2
    ; xmm3 <-- ((((r/7 + 1/6)*r) + 1/5)*r)*r^4 + r1 - (r1 + r1^2/2) + r1^2/2
    vaddsd       xmm3, xmm3, xmm7
    vmovsd       xmm4, QWORD PTR __real_log2_tail
    ; xmm1 <-- (((((r/7 + 1/6)*r) + 1/5)*r)*r^4) +
    ;   (r1 - (r1 + r1^2/2) + r1^2/2) + ((r/4 + 1/3)*r)*r^2)
    vaddsd       xmm1, xmm1, xmm3
    lea          rdx, __log_256_tail
    ; xmm1 <-- ((((((r/7 + 1/6)*r) + 1/5)*r)*r^4) +
    ;   (r1 - (r1 + r1^2/2) + r1^2/2) + ((r/4 + 1/3)*r)*r^2))
    ;   +(r1*r2 + r2^2/2 + r2)
    vaddsd       xmm1, xmm1, xmm5
    ; xmm4 <-- vt * log2_tail  + log256_tail
    vfmadd213sd  xmm4, xmm6, QWORD PTR [rdx+r8*8]
    ; xmm4 <-- vt * log2_tail  + log2_tail - corrected poly
    vsubsd       xmm4, xmm4, xmm1

    vmovapd      xmm1, xmm4
    vsubsd       xmm3, xmm4, xmm2 ; xmm3 <-- xmm4 - more correction???

    vmovsd       xmm0, QWORD PTR [r9+r8*8] ; xmm0 <-- log256_lead
    ; xmm0 <-- log256_lead + vt*log2_lead
    vfmadd231sd  xmm0, xmm6, QWORD PTR __real_log2_lead

    ; at this point, xmm0, xmm1, xmm2, and xmm3 should matter
    jmp          Lpow_fma3_log_x_continue


ALIGN 16
Lpow_fma3_x_is_pos_one:
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_y_is_zero:
    vmovsd       xmm0, QWORD PTR __real_one
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_y_is_one:
    xor          rax, rax
    mov          r11, rdx
    mov          r9, QWORD PTR __exp_mask
    ;or          r11, QWORD PTR __qnan_set
    and          r9, rdx
    cmp          r9, QWORD PTR __exp_mask
    cmove        rax, rdx
    mov          r9, QWORD PTR __mant_mask
    and          r9, rax
    jnz          Lpow_fma3_x_is_nan

    vmovq        xmm0, rdx
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_x_is_neg_one:
    mov          rdx, QWORD PTR __pos_one
    or           rdx, QWORD PTR [negate_result+rsp]
    xor          rax, rax
    mov          r11, r8
    mov          r10, QWORD PTR __exp_mask
    ;or          r11, QWORD PTR __qnan_set
    and          r10, r8
    cmp          r10, QWORD PTR __exp_mask
    cmove        rax, r8
    mov          r10, QWORD PTR __mant_mask
    and          r10, rax
    jnz          Lpow_fma3_y_is_nan

    vmovq        xmm0, rdx
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_x_is_neg_y_is_not_int:
    mov          r9, QWORD PTR __exp_mask
    and          r9, rdx
    cmp          r9, QWORD PTR __exp_mask
    je           Lpow_fma3_x_is_inf_or_nan

    cmp          rdx, QWORD PTR __neg_zero
    je           Lpow_fma3_x_is_zero

    vmovsd       xmm0, QWORD PTR [save_x+rsp]
    vmovsd       xmm1, QWORD PTR [save_y+rsp]
    vmovsd       xmm2, QWORD PTR __neg_qnan
    mov          r9d, DWORD PTR __flag_x_neg_y_notint

    call         fname_special
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_ay_is_very_large:
    mov          r9, QWORD PTR __exp_mask
    and          r9, rdx
    cmp          r9, QWORD PTR __exp_mask
    je           Lpow_fma3_x_is_inf_or_nan

    mov          r9, QWORD PTR __exp_mant_mask
    and          r9, rdx
    jz           Lpow_fma3_x_is_zero

    cmp          rdx, QWORD PTR __neg_one
    je           Lpow_fma3_x_is_neg_one

    mov          r9, rdx
    and          r9, QWORD PTR __exp_mant_mask
    cmp          r9, QWORD PTR __pos_one
    jl           Lpow_fma3_ax_lt1_y_is_large_or_inf_or_nan

    jmp          Lpow_fma3_ax_gt1_y_is_large_or_inf_or_nan

ALIGN 16
Lpow_fma3_x_is_zero:
    mov          r10, QWORD PTR __exp_mask
    xor          rax, rax
    and          r10, r8
    cmp          r10, QWORD PTR __exp_mask
    je           Lpow_fma3_x_is_zero_y_is_inf_or_nan

    mov          r10, QWORD PTR __sign_mask
    and          r10, r8
    cmovnz       rax, QWORD PTR __pos_inf
    jnz          Lpow_fma3_x_is_zero_z_is_inf

    vmovq        xmm0, rax
    vorpd        xmm0, xmm0, XMMWORD PTR [negate_result+rsp]
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_x_is_zero_z_is_inf:

    vmovsd       xmm0, QWORD PTR [save_x+rsp]
    vmovsd       xmm1, QWORD PTR [save_y+rsp]
    vmovq        xmm2, rax
    vorpd        xmm2, xmm2, XMMWORD PTR [negate_result+rsp]
    mov          r9d, DWORD PTR __flag_x_zero_z_inf

    call         fname_special
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_x_is_zero_y_is_inf_or_nan:
    mov          r11, r8
    cmp          r8, QWORD PTR __neg_inf
;   The next two lines do not correspond to IEEE754-2008.
;   +-0 ^ -Inf should be +Inf with no exception
;   +-0 ^ +Inf should be +0 with no exception
;   cmove        rax, QWORD PTR __pos_inf
;   je           Lpow_fma3_x_is_zero_z_is_inf
;  begin replacement
    je           Lpow_fma3_x_is_zero_y_is_neg_inf
    cmp          r8, QWORD PTR __neg_inf
    je           Lpow_fma3_x_is_zero_y_is_pos_inf
;  end replacement

    ;or          r11, QWORD PTR __qnan_set
    mov          r10, QWORD PTR __mant_mask
    and          r10, r8
    jnz          Lpow_fma3_y_is_nan

    vmovq        xmm0, rax
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_x_is_zero_y_is_neg_inf:
    ; quietly return +Inf
    vmovsd       xmm0, __pos_inf
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_x_is_zero_y_is_pos_inf:
    ; quietly return +0.
    vxorpd       xmm0, xmm0, xmm0
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_x_is_inf_or_nan:
    xor          r11, r11
    mov          r10, QWORD PTR __sign_mask
    and          r10, r8
    cmovz        r11, QWORD PTR __pos_inf
    mov          rax, rdx
    mov          r9, QWORD PTR __mant_mask
    ;or          rax, QWORD PTR __qnan_set
    and          r9, rdx
    cmovnz       r11, rax
    jnz          Lpow_fma3_x_is_nan

    xor          rax, rax
    mov          r9, r8
    mov          r10, QWORD PTR __exp_mask
    ;or          r9, QWORD PTR __qnan_set
    and          r10, r8
    cmp          r10, QWORD PTR __exp_mask
    cmove        rax, r8
    mov          r10, QWORD PTR __mant_mask
    and          r10, rax
    cmovnz       r11, r9
    jnz          Lpow_fma3_y_is_nan

    vmovq        xmm0, r11
    vorpd        xmm0, xmm0, XMMWORD PTR [negate_result+rsp]
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_ay_is_very_small:
    vaddsd       xmm0, xmm1, QWORD PTR __pos_one
    jmp          Lpow_fma3_final_check


ALIGN 16
Lpow_fma3_ax_lt1_y_is_large_or_inf_or_nan:
    xor          r11, r11
    mov          r10, QWORD PTR __sign_mask
    and          r10, r8
    cmovnz       r11, QWORD PTR __pos_inf
    jmp          Lpow_fma3_adjust_for_nan

ALIGN 16
Lpow_fma3_ax_gt1_y_is_large_or_inf_or_nan:
    xor          r11, r11
    mov          r10, QWORD PTR __sign_mask
    and          r10, r8
    cmovz        r11, QWORD PTR __pos_inf

ALIGN 16
Lpow_fma3_adjust_for_nan:

    xor          rax, rax
    mov          r9, r8
    mov          r10, QWORD PTR __exp_mask
    ;or          r9, QWORD PTR __qnan_set
    and          r10, r8
    cmp          r10, QWORD PTR __exp_mask
    cmove        rax, r8
    mov          r10, QWORD PTR __mant_mask
    and          r10, rax
    cmovnz       r11, r9
    jnz          Lpow_fma3_y_is_nan

    test         rax, rax
    jnz          Lpow_fma3_y_is_inf

ALIGN 16
Lpow_fma3_z_is_zero_or_inf:

    mov          r9d, DWORD PTR __flag_z_zero
    test         r11, QWORD PTR __exp_mant_mask
    cmovnz       r9d, DWORD PTR __flag_z_inf

    vmovsd       xmm0, QWORD PTR [save_x+rsp]
    vmovsd       xmm1, QWORD PTR [save_y+rsp]
    vmovq        xmm2, r11

    call         fname_special
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_y_is_inf:

    vmovq        xmm0, r11
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_x_is_nan:

    xor          rax, rax
    mov          r10, QWORD PTR __exp_mask
    and          r10, r8
    cmp          r10, QWORD PTR __exp_mask
    cmove        rax, r8
    mov          r10, QWORD PTR __mant_mask
    and          r10, rax
    jnz          Lpow_fma3_x_is_nan_y_is_nan

    vmovsd       xmm0, QWORD PTR [save_x+rsp]
    vmovsd       xmm1, QWORD PTR [save_y+rsp]
    vmovq        xmm2, r11
    mov          r9d, DWORD PTR __flag_x_nan

    call         fname_special
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_y_is_nan:

    vmovsd       xmm0, QWORD PTR [save_x+rsp]
    vmovsd       xmm1, QWORD PTR [save_y+rsp]
    vmovq        xmm2, r11
    mov          r9d, DWORD PTR __flag_y_nan

    call         fname_special
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_x_is_nan_y_is_nan:

    mov          r9, r8

    cmp          r11, QWORD PTR __ind_pattern
    cmove        r11, r9
    je           Lpow_fma3_continue_xy_nan

    cmp          r9, QWORD PTR __ind_pattern
    cmove        r9, r11

    mov          r10, r9
    and          r10, QWORD PTR __sign_mask
    cmovnz       r9, r11

    mov          r10, r11
    and          r10, QWORD PTR __sign_mask
    cmovnz       r11, r9

Lpow_fma3_continue_xy_nan:
    ;or          r11, QWORD PTR __qnan_set
    vmovsd       xmm0, QWORD PTR [save_x+rsp]
    vmovsd       xmm1, QWORD PTR [save_y+rsp]
    vmovq        xmm2, r11
    mov          r9d, DWORD PTR __flag_x_nan_y_nan

    call         fname_special
    jmp          Lpow_fma3_final_check

ALIGN 16
Lpow_fma3_z_denormal:
    vmovapd      xmm2, xmm0
    vmovsd       xmm0, QWORD PTR [save_x+rsp]
    vmovsd       xmm1, QWORD PTR [save_y+rsp]
    mov          r9d, DWORD PTR __flag_z_denormal

    call         fname_special
    jmp          Lpow_fma3_final_check

fname endp
END
