;;
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
;; Defines __Lcosarray and __Lsinarray arrays.
;; Used in sin.asm and cos.asm
;;

.const

ALIGN 16
PUBLIC __Lcosarray 
__Lcosarray DQ    03fa5555555555555h                          ; 0.0416667           c1
            DQ    0
            DQ    0bf56c16c16c16967h                          ; -0.00138889         c2
            DQ    0
            DQ    03EFA01A019F4EC91h                          ; 2.48016e-005        c3
            DQ    0
            DQ    0bE927E4FA17F667Bh                          ; -2.75573e-007       c4
            DQ    0
            DQ    03E21EEB690382EECh                          ; 2.08761e-009        c5
            DQ    0
            DQ    0bDA907DB47258AA7h                          ; -1.13826e-011       c6
            DQ    0

ALIGN 16
PUBLIC __Lsinarray 
__Lsinarray DQ    0bfc5555555555555h                          ; -0.166667           s1
            DQ    0
            DQ    03f81111111110bb3h                          ; 0.00833333          s2
            DQ    0
            DQ    0bf2a01a019e83e5ch                          ; -0.000198413        s3
            DQ    0
            DQ    03ec71de3796cde01h                          ; 2.75573e-006        s4
            DQ    0
            DQ    0be5ae600b42fdfa7h                          ; -2.50511e-008       s5
            DQ    0
            DQ    03de5e0b2f9a43bb8h                          ; 1.59181e-010        s6
            DQ    0

END
