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
;; Defines __log_F_inv_dword
;; Used in log10f and logf
;;

.const

ALIGN 16
PUBLIC __log_F_inv_dword
__log_F_inv_dword   DD 40000000h
                    DD 3ffe03f8h
                    DD 3ffc0fc1h
                    DD 3ffa232dh
                    DD 3ff83e10h
                    DD 3ff6603eh
                    DD 3ff4898dh
                    DD 3ff2b9d6h
                    DD 3ff0f0f1h
                    DD 3fef2eb7h
                    DD 3fed7304h
                    DD 3febbdb3h
                    DD 3fea0ea1h
                    DD 3fe865ach
                    DD 3fe6c2b4h
                    DD 3fe52598h
                    DD 3fe38e39h
                    DD 3fe1fc78h
                    DD 3fe07038h
                    DD 3fdee95ch
                    DD 3fdd67c9h
                    DD 3fdbeb62h
                    DD 3fda740eh
                    DD 3fd901b2h
                    DD 3fd79436h
                    DD 3fd62b81h
                    DD 3fd4c77bh
                    DD 3fd3680dh
                    DD 3fd20d21h
                    DD 3fd0b6a0h
                    DD 3fcf6475h
                    DD 3fce168ah
                    DD 3fcccccdh
                    DD 3fcb8728h
                    DD 3fca4588h
                    DD 3fc907dah
                    DD 3fc7ce0ch
                    DD 3fc6980ch
                    DD 3fc565c8h
                    DD 3fc43730h
                    DD 3fc30c31h
                    DD 3fc1e4bch
                    DD 3fc0c0c1h
                    DD 3fbfa030h
                    DD 3fbe82fah
                    DD 3fbd6910h
                    DD 3fbc5264h
                    DD 3fbb3ee7h
                    DD 3fba2e8ch
                    DD 3fb92144h
                    DD 3fb81703h
                    DD 3fb70fbbh
                    DD 3fb60b61h
                    DD 3fb509e7h
                    DD 3fb40b41h
                    DD 3fb30f63h
                    DD 3fb21643h
                    DD 3fb11fd4h
                    DD 3fb02c0bh
                    DD 3faf3adeh
                    DD 3fae4c41h
                    DD 3fad602bh
                    DD 3fac7692h
                    DD 3fab8f6ah
                    DD 3faaaaabh
                    DD 3fa9c84ah
                    DD 3fa8e83fh
                    DD 3fa80a81h
                    DD 3fa72f05h
                    DD 3fa655c4h
                    DD 3fa57eb5h
                    DD 3fa4a9cfh
                    DD 3fa3d70ah
                    DD 3fa3065eh
                    DD 3fa237c3h
                    DD 3fa16b31h
                    DD 3fa0a0a1h
                    DD 3f9fd80ah
                    DD 3f9f1166h
                    DD 3f9e4cadh
                    DD 3f9d89d9h
                    DD 3f9cc8e1h
                    DD 3f9c09c1h
                    DD 3f9b4c70h
                    DD 3f9a90e8h
                    DD 3f99d723h
                    DD 3f991f1ah
                    DD 3f9868c8h
                    DD 3f97b426h
                    DD 3f97012eh
                    DD 3f964fdah
                    DD 3f95a025h
                    DD 3f94f209h
                    DD 3f944581h
                    DD 3f939a86h
                    DD 3f92f114h
                    DD 3f924925h
                    DD 3f91a2b4h
                    DD 3f90fdbch
                    DD 3f905a38h
                    DD 3f8fb824h
                    DD 3f8f177ah
                    DD 3f8e7835h
                    DD 3f8dda52h
                    DD 3f8d3dcbh
                    DD 3f8ca29ch
                    DD 3f8c08c1h
                    DD 3f8b7034h
                    DD 3f8ad8f3h
                    DD 3f8a42f8h
                    DD 3f89ae41h
                    DD 3f891ac7h
                    DD 3f888889h
                    DD 3f87f781h
                    DD 3f8767abh
                    DD 3f86d905h
                    DD 3f864b8ah
                    DD 3f85bf37h
                    DD 3f853408h
                    DD 3f84a9fah
                    DD 3f842108h
                    DD 3f839930h
                    DD 3f83126fh
                    DD 3f828cc0h
                    DD 3f820821h
                    DD 3f81848eh
                    DD 3f810204h
                    DD 3f808081h
                    DD 3f800000h

END
