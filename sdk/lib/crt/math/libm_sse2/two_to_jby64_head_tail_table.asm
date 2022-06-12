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
;; Defines __two_to_jby64_head_table and __two_to_jby64_tail_table tables
;; Used in exp and pow
;;

.const

ALIGN 16
PUBLIC __two_to_jby64_head_table 
__two_to_jby64_head_table DQ 3ff0000000000000h
                          DQ 3ff02c9a30000000h
                          DQ 3ff059b0d0000000h
                          DQ 3ff0874510000000h
                          DQ 3ff0b55860000000h
                          DQ 3ff0e3ec30000000h
                          DQ 3ff11301d0000000h
                          DQ 3ff1429aa0000000h
                          DQ 3ff172b830000000h
                          DQ 3ff1a35be0000000h
                          DQ 3ff1d48730000000h
                          DQ 3ff2063b80000000h
                          DQ 3ff2387a60000000h
                          DQ 3ff26b4560000000h
                          DQ 3ff29e9df0000000h
                          DQ 3ff2d285a0000000h
                          DQ 3ff306fe00000000h
                          DQ 3ff33c08b0000000h
                          DQ 3ff371a730000000h
                          DQ 3ff3a7db30000000h
                          DQ 3ff3dea640000000h
                          DQ 3ff4160a20000000h
                          DQ 3ff44e0860000000h
                          DQ 3ff486a2b0000000h
                          DQ 3ff4bfdad0000000h
                          DQ 3ff4f9b270000000h
                          DQ 3ff5342b50000000h
                          DQ 3ff56f4730000000h
                          DQ 3ff5ab07d0000000h
                          DQ 3ff5e76f10000000h
                          DQ 3ff6247eb0000000h
                          DQ 3ff6623880000000h
                          DQ 3ff6a09e60000000h
                          DQ 3ff6dfb230000000h
                          DQ 3ff71f75e0000000h
                          DQ 3ff75feb50000000h
                          DQ 3ff7a11470000000h
                          DQ 3ff7e2f330000000h
                          DQ 3ff8258990000000h
                          DQ 3ff868d990000000h
                          DQ 3ff8ace540000000h
                          DQ 3ff8f1ae90000000h
                          DQ 3ff93737b0000000h
                          DQ 3ff97d8290000000h
                          DQ 3ff9c49180000000h
                          DQ 3ffa0c6670000000h
                          DQ 3ffa5503b0000000h
                          DQ 3ffa9e6b50000000h
                          DQ 3ffae89f90000000h
                          DQ 3ffb33a2b0000000h
                          DQ 3ffb7f76f0000000h
                          DQ 3ffbcc1e90000000h
                          DQ 3ffc199bd0000000h
                          DQ 3ffc67f120000000h
                          DQ 3ffcb720d0000000h
                          DQ 3ffd072d40000000h
                          DQ 3ffd5818d0000000h
                          DQ 3ffda9e600000000h
                          DQ 3ffdfc9730000000h
                          DQ 3ffe502ee0000000h
                          DQ 3ffea4afa0000000h
                          DQ 3ffefa1be0000000h
                          DQ 3fff507650000000h
                          DQ 3fffa7c180000000h

ALIGN 16
PUBLIC __two_to_jby64_tail_table
__two_to_jby64_tail_table DQ 0000000000000000h
                          DQ 3e6cef00c1dcdef9h
                          DQ 3e48ac2ba1d73e2ah
                          DQ 3e60eb37901186beh
                          DQ 3e69f3121ec53172h
                          DQ 3e469e8d10103a17h
                          DQ 3df25b50a4ebbf1ah
                          DQ 3e6d525bbf668203h
                          DQ 3e68faa2f5b9bef9h
                          DQ 3e66df96ea796d31h
                          DQ 3e368b9aa7805b80h
                          DQ 3e60c519ac771dd6h
                          DQ 3e6ceac470cd83f5h
                          DQ 3e5789f37495e99ch
                          DQ 3e547f7b84b09745h
                          DQ 3e5b900c2d002475h
                          DQ 3e64636e2a5bd1abh
                          DQ 3e4320b7fa64e430h
                          DQ 3e5ceaa72a9c5154h
                          DQ 3e53967fdba86f24h
                          DQ 3e682468446b6824h
                          DQ 3e3f72e29f84325bh
                          DQ 3e18624b40c4dbd0h
                          DQ 3e5704f3404f068eh
                          DQ 3e54d8a89c750e5eh
                          DQ 3e5a74b29ab4cf62h
                          DQ 3e5a753e077c2a0fh
                          DQ 3e5ad49f699bb2c0h
                          DQ 3e6a90a852b19260h
                          DQ 3e56b48521ba6f93h
                          DQ 3e0d2ac258f87d03h
                          DQ 3e42a91124893ecfh
                          DQ 3e59fcef32422cbeh
                          DQ 3e68ca345de441c5h
                          DQ 3e61d8bee7ba46e1h
                          DQ 3e59099f22fdba6ah
                          DQ 3e4f580c36bea881h
                          DQ 3e5b3d398841740ah
                          DQ 3e62999c25159f11h
                          DQ 3e668925d901c83bh
                          DQ 3e415506dadd3e2ah
                          DQ 3e622aee6c57304eh
                          DQ 3e29b8bc9e8a0387h
                          DQ 3e6fbc9c9f173d24h
                          DQ 3e451f8480e3e235h
                          DQ 3e66bbcac96535b5h
                          DQ 3e41f12ae45a1224h
                          DQ 3e55e7f6fd0fac90h
                          DQ 3e62b5a75abd0e69h
                          DQ 3e609e2bf5ed7fa1h
                          DQ 3e47daf237553d84h
                          DQ 3e12f074891ee83dh
                          DQ 3e6b0aa538444196h
                          DQ 3e6cafa29694426fh
                          DQ 3e69df20d22a0797h
                          DQ 3e640f12f71a1e45h
                          DQ 3e69f7490e4bb40bh
                          DQ 3e4ed9942b84600dh
                          DQ 3e4bdcdaf5cb4656h
                          DQ 3e5e2cffd89cf44ch
                          DQ 3e452486cc2c7b9dh
                          DQ 3e6cc2b44eee3fa4h
                          DQ 3e66dc8a80ce9f09h
                          DQ 3e39e90d82e90a7eh
END
