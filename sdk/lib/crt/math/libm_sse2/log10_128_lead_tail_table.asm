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
;;
;; Defines __log_128_lead and __log_128_tail tables
;; Used by log and pow
;;

.const

ALIGN 16
PUBLIC __log10_128_lead  
__log10_128_lead:
                    DD 00000000h
                    DD 3b5d4000h
                    DD 3bdc8000h
                    DD 3c24c000h
                    DD 3c5ac000h
                    DD 3c884000h
                    DD 3ca2c000h
                    DD 3cbd4000h
                    DD 3cd78000h
                    DD 3cf1c000h
                    DD 3d05c000h
                    DD 3d128000h
                    DD 3d1f4000h
                    DD 3d2c0000h
                    DD 3d388000h
                    DD 3d450000h
                    DD 3d518000h
                    DD 3d5dc000h
                    DD 3d6a0000h
                    DD 3d760000h
                    DD 3d810000h
                    DD 3d870000h
                    DD 3d8d0000h
                    DD 3d92c000h
                    DD 3d98c000h
                    DD 3d9e8000h
                    DD 3da44000h
                    DD 3daa0000h
                    DD 3dafc000h
                    DD 3db58000h
                    DD 3dbb4000h
                    DD 3dc0c000h
                    DD 3dc64000h
                    DD 3dcc0000h
                    DD 3dd18000h
                    DD 3dd6c000h
                    DD 3ddc4000h
                    DD 3de1c000h
                    DD 3de70000h
                    DD 3dec8000h
                    DD 3df1c000h
                    DD 3df70000h
                    DD 3dfc4000h
                    DD 3e00c000h
                    DD 3e034000h
                    DD 3e05c000h
                    DD 3e088000h
                    DD 3e0b0000h
                    DD 3e0d8000h
                    DD 3e100000h
                    DD 3e128000h
                    DD 3e150000h
                    DD 3e178000h
                    DD 3e1a0000h
                    DD 3e1c8000h
                    DD 3e1ec000h
                    DD 3e214000h
                    DD 3e23c000h
                    DD 3e260000h
                    DD 3e288000h
                    DD 3e2ac000h
                    DD 3e2d4000h
                    DD 3e2f8000h
                    DD 3e31c000h
                    DD 3e344000h
                    DD 3e368000h
                    DD 3e38c000h
                    DD 3e3b0000h
                    DD 3e3d4000h
                    DD 3e3fc000h
                    DD 3e420000h
                    DD 3e440000h
                    DD 3e464000h
                    DD 3e488000h
                    DD 3e4ac000h
                    DD 3e4d0000h
                    DD 3e4f4000h
                    DD 3e514000h
                    DD 3e538000h
                    DD 3e55c000h
                    DD 3e57c000h
                    DD 3e5a0000h
                    DD 3e5c0000h
                    DD 3e5e4000h
                    DD 3e604000h
                    DD 3e624000h
                    DD 3e648000h
                    DD 3e668000h
                    DD 3e688000h
                    DD 3e6ac000h
                    DD 3e6cc000h
                    DD 3e6ec000h
                    DD 3e70c000h
                    DD 3e72c000h
                    DD 3e74c000h
                    DD 3e76c000h
                    DD 3e78c000h
                    DD 3e7ac000h
                    DD 3e7cc000h
                    DD 3e7ec000h
                    DD 3e804000h
                    DD 3e814000h
                    DD 3e824000h
                    DD 3e834000h
                    DD 3e840000h
                    DD 3e850000h
                    DD 3e860000h
                    DD 3e870000h
                    DD 3e880000h
                    DD 3e88c000h
                    DD 3e89c000h
                    DD 3e8ac000h
                    DD 3e8bc000h
                    DD 3e8c8000h
                    DD 3e8d8000h
                    DD 3e8e8000h
                    DD 3e8f4000h
                    DD 3e904000h
                    DD 3e914000h
                    DD 3e920000h
                    DD 3e930000h
                    DD 3e93c000h
                    DD 3e94c000h
                    DD 3e958000h
                    DD 3e968000h
                    DD 3e978000h
                    DD 3e984000h
                    DD 3e994000h
                    DD 3e9a0000h

ALIGN 16
PUBLIC __log10_128_tail
__log10_128_tail:
                    DD 00000000h
                    DD 367a8e44h
                    DD 368ed49fh
                    DD 36c21451h
                    DD 375211d6h
                    DD 3720ea11h
                    DD 37e9eb59h
                    DD 37b87be7h
                    DD 37bf2560h
                    DD 33d597a0h
                    DD 37806a05h
                    DD 3820581fh
                    DD 38223334h
                    DD 378e3bach
                    DD 3810684fh
                    DD 37feb7aeh
                    DD 36a9d609h
                    DD 37a68163h
                    DD 376a8b27h
                    DD 384c8fd6h
                    DD 3885183eh
                    DD 3874a760h
                    DD 380d1154h
                    DD 38ea42bdh
                    DD 384c1571h
                    DD 38ba66b8h
                    DD 38e7da3bh
                    DD 38eee632h
                    DD 38d00911h
                    DD 388bbedeh
                    DD 378a0512h
                    DD 3894c7a0h
                    DD 38e30710h
                    DD 36db2829h
                    DD 3729d609h
                    DD 38fa0e82h
                    DD 38bc9a75h
                    DD 383a9297h
                    DD 38dc83c8h
                    DD 37eac335h
                    DD 38706ac3h
                    DD 389574c2h
                    DD 3892d068h
                    DD 38615032h
                    DD 3917acf4h
                    DD 3967a126h
                    DD 38217840h
                    DD 38b420abh
                    DD 38f9c7b2h
                    DD 391103bdh
                    DD 39169a6bh
                    DD 390dd194h
                    DD 38eda471h
                    DD 38a38950h
                    DD 37f6844ah
                    DD 395e1cdbh
                    DD 390fcffch
                    DD 38503e9dh
                    DD 394b00fdh
                    DD 38a9910ah
                    DD 39518a31h
                    DD 3882d2c2h
                    DD 392488e4h
                    DD 397b0affh
                    DD 388a22d8h
                    DD 3902bd5eh
                    DD 39342f85h
                    DD 39598811h
                    DD 3972e6b1h
                    DD 34d53654h
                    DD 360ca25eh
                    DD 39785cc0h
                    DD 39630710h
                    DD 39424ed7h
                    DD 39165101h
                    DD 38be5421h
                    DD 37e7b0c0h
                    DD 394fd0c3h
                    DD 38efaaaah
                    DD 37a8f566h
                    DD 3927c744h
                    DD 383fa4d5h
                    DD 392d9e39h
                    DD 3803feaeh
                    DD 390a268ch
                    DD 39692b80h
                    DD 38789b4fh
                    DD 3909307dh
                    DD 394a601ch
                    DD 35e67edch
                    DD 383e386dh
                    DD 38a7743dh
                    DD 38dccec3h
                    DD 38ff57e0h
                    DD 39079d8bh
                    DD 390651a6h
                    DD 38f7bad9h
                    DD 38d0ab82h
                    DD 38979e7dh
                    DD 381978eeh
                    DD 397816c8h
                    DD 39410cb2h
                    DD 39015384h
                    DD 3863fa28h
                    DD 39f41065h
                    DD 39c7668ah
                    DD 39968afah
                    DD 39430db9h
                    DD 38a18cf3h
                    DD 39eb2907h
                    DD 39a9e10ch
                    DD 39492800h
                    DD 385a53d1h
                    DD 39ce0cf7h
                    DD 3979c7b2h
                    DD 389f5d99h
                    DD 39ceefcbh
                    DD 39646a39h
                    DD 380d7a9bh
                    DD 39ad6650h
                    DD 390ac3b8h
                    DD 39d9a9a8h
                    DD 39548a99h
                    DD 39f73c4bh
                    DD 3980960eh
                    DD 374b3d5ah
                    DD 39888f1eh
                    DD 37679a07h
                    DD 39826a13h
END
