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
;; Defines __two_to_jby64_table table
;; Used by exp and expf
;;

.const

ALIGN 16
PUBLIC __two_to_jby64_table 
__two_to_jby64_table DQ 3ff0000000000000h
                     DQ 3ff02c9a3e778061h
                     DQ 3ff059b0d3158574h
                     DQ 3ff0874518759bc8h
                     DQ 3ff0b5586cf9890fh
                     DQ 3ff0e3ec32d3d1a2h
                     DQ 3ff11301d0125b51h
                     DQ 3ff1429aaea92de0h
                     DQ 3ff172b83c7d517bh
                     DQ 3ff1a35beb6fcb75h
                     DQ 3ff1d4873168b9aah
                     DQ 3ff2063b88628cd6h
                     DQ 3ff2387a6e756238h
                     DQ 3ff26b4565e27cddh
                     DQ 3ff29e9df51fdee1h
                     DQ 3ff2d285a6e4030bh
                     DQ 3ff306fe0a31b715h
                     DQ 3ff33c08b26416ffh
                     DQ 3ff371a7373aa9cbh
                     DQ 3ff3a7db34e59ff7h
                     DQ 3ff3dea64c123422h
                     DQ 3ff4160a21f72e2ah
                     DQ 3ff44e086061892dh
                     DQ 3ff486a2b5c13cd0h
                     DQ 3ff4bfdad5362a27h
                     DQ 3ff4f9b2769d2ca7h
                     DQ 3ff5342b569d4f82h
                     DQ 3ff56f4736b527dah
                     DQ 3ff5ab07dd485429h
                     DQ 3ff5e76f15ad2148h
                     DQ 3ff6247eb03a5585h
                     DQ 3ff6623882552225h
                     DQ 3ff6a09e667f3bcdh
                     DQ 3ff6dfb23c651a2fh
                     DQ 3ff71f75e8ec5f74h
                     DQ 3ff75feb564267c9h
                     DQ 3ff7a11473eb0187h
                     DQ 3ff7e2f336cf4e62h
                     DQ 3ff82589994cce13h
                     DQ 3ff868d99b4492edh
                     DQ 3ff8ace5422aa0dbh
                     DQ 3ff8f1ae99157736h
                     DQ 3ff93737b0cdc5e5h
                     DQ 3ff97d829fde4e50h
                     DQ 3ff9c49182a3f090h
                     DQ 3ffa0c667b5de565h
                     DQ 3ffa5503b23e255dh
                     DQ 3ffa9e6b5579fdbfh
                     DQ 3ffae89f995ad3adh
                     DQ 3ffb33a2b84f15fbh
                     DQ 3ffb7f76f2fb5e47h
                     DQ 3ffbcc1e904bc1d2h
                     DQ 3ffc199bdd85529ch
                     DQ 3ffc67f12e57d14bh
                     DQ 3ffcb720dcef9069h
                     DQ 3ffd072d4a07897ch
                     DQ 3ffd5818dcfba487h
                     DQ 3ffda9e603db3285h
                     DQ 3ffdfc97337b9b5fh
                     DQ 3ffe502ee78b3ff6h
                     DQ 3ffea4afa2a490dah
                     DQ 3ffefa1bee615a27h
                     DQ 3fff50765b6e4540h
                     DQ 3fffa7c1819e90d8h

END
