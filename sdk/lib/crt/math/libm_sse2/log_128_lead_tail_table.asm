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
;; Defines __log_128_lead and __log_128_tail tables
;; Used by log and pow
;;

.const

ALIGN 16
PUBLIC __log_128_lead  
__log_128_lead      DD 000000000h
                    DD 03bff0000h
                    DD 03c7e0000h
                    DD 03cbdc000h
                    DD 03cfc1000h
                    DD 03d1cf000h
                    DD 03d3ba000h
                    DD 03d5a1000h
                    DD 03d785000h
                    DD 03d8b2000h
                    DD 03d9a0000h
                    DD 03da8d000h
                    DD 03db78000h
                    DD 03dc61000h
                    DD 03dd49000h
                    DD 03de2f000h
                    DD 03df13000h
                    DD 03dff6000h
                    DD 03e06b000h
                    DD 03e0db000h
                    DD 03e14a000h
                    DD 03e1b8000h
                    DD 03e226000h
                    DD 03e293000h
                    DD 03e2ff000h
                    DD 03e36b000h
                    DD 03e3d5000h
                    DD 03e43f000h
                    DD 03e4a9000h
                    DD 03e511000h
                    DD 03e579000h
                    DD 03e5e1000h
                    DD 03e647000h
                    DD 03e6ae000h
                    DD 03e713000h
                    DD 03e778000h
                    DD 03e7dc000h
                    DD 03e820000h
                    DD 03e851000h
                    DD 03e882000h
                    DD 03e8b3000h
                    DD 03e8e4000h
                    DD 03e914000h
                    DD 03e944000h
                    DD 03e974000h
                    DD 03e9a3000h
                    DD 03e9d3000h
                    DD 03ea02000h
                    DD 03ea30000h
                    DD 03ea5f000h
                    DD 03ea8d000h
                    DD 03eabb000h
                    DD 03eae8000h
                    DD 03eb16000h
                    DD 03eb43000h
                    DD 03eb70000h
                    DD 03eb9c000h
                    DD 03ebc9000h
                    DD 03ebf5000h
                    DD 03ec21000h
                    DD 03ec4d000h
                    DD 03ec78000h
                    DD 03eca3000h
                    DD 03ecce000h
                    DD 03ecf9000h
                    DD 03ed24000h
                    DD 03ed4e000h
                    DD 03ed78000h
                    DD 03eda2000h
                    DD 03edcc000h
                    DD 03edf5000h
                    DD 03ee1e000h
                    DD 03ee47000h
                    DD 03ee70000h
                    DD 03ee99000h
                    DD 03eec1000h
                    DD 03eeea000h
                    DD 03ef12000h
                    DD 03ef3a000h
                    DD 03ef61000h
                    DD 03ef89000h
                    DD 03efb0000h
                    DD 03efd7000h
                    DD 03effe000h
                    DD 03f012000h
                    DD 03f025000h
                    DD 03f039000h
                    DD 03f04c000h
                    DD 03f05f000h
                    DD 03f072000h
                    DD 03f084000h
                    DD 03f097000h
                    DD 03f0aa000h
                    DD 03f0bc000h
                    DD 03f0cf000h
                    DD 03f0e1000h
                    DD 03f0f4000h
                    DD 03f106000h
                    DD 03f118000h
                    DD 03f12a000h
                    DD 03f13c000h
                    DD 03f14e000h
                    DD 03f160000h
                    DD 03f172000h
                    DD 03f183000h
                    DD 03f195000h
                    DD 03f1a7000h
                    DD 03f1b8000h
                    DD 03f1c9000h
                    DD 03f1db000h
                    DD 03f1ec000h
                    DD 03f1fd000h
                    DD 03f20e000h
                    DD 03f21f000h
                    DD 03f230000h
                    DD 03f241000h
                    DD 03f252000h
                    DD 03f263000h
                    DD 03f273000h
                    DD 03f284000h
                    DD 03f295000h
                    DD 03f2a5000h
                    DD 03f2b5000h
                    DD 03f2c6000h
                    DD 03f2d6000h
                    DD 03f2e6000h
                    DD 03f2f7000h
                    DD 03f307000h
                    DD 03f317000h

ALIGN 16
PUBLIC __log_128_tail
__log_128_tail      DD 000000000h
                    DD 03429ac41h
                    DD 035a8b0fch
                    DD 0368d83eah
                    DD 0361b0e78h
                    DD 03687b9feh
                    DD 03631ec65h
                    DD 036dd7119h
                    DD 035c30045h
                    DD 0379b7751h
                    DD 037ebcb0dh
                    DD 037839f83h
                    DD 037528ae5h
                    DD 037a2eb18h
                    DD 036da7495h
                    DD 036a91eb7h
                    DD 03783b715h
                    DD 0371131dbh
                    DD 0383f3e68h
                    DD 038156a97h
                    DD 038297c0fh
                    DD 0387e100fh
                    DD 03815b665h
                    DD 037e5e3a1h
                    DD 038183853h
                    DD 035fe719dh
                    DD 038448108h
                    DD 038503290h
                    DD 0373539e8h
                    DD 0385e0ff1h
                    DD 03864a740h
                    DD 03786742dh
                    DD 0387be3cdh
                    DD 03685ad3eh
                    DD 03803b715h
                    DD 037adcbdch
                    DD 0380c36afh
                    DD 0371652d3h
                    DD 038927139h
                    DD 038c5fcd7h
                    DD 038ae55d5h
                    DD 03818c169h
                    DD 038a0fde7h
                    DD 038ad09efh
                    DD 03862bae1h
                    DD 038eecd4ch
                    DD 03798aad2h
                    DD 037421a1ah
                    DD 038c5e10eh
                    DD 037bf2aeeh
                    DD 0382d872dh
                    DD 037ee2e8ah
                    DD 038dedfach
                    DD 03802f2b9h
                    DD 038481e9bh
                    DD 0380eaa2bh
                    DD 038ebfb5dh
                    DD 038255fddh
                    DD 038783b82h
                    DD 03851da1eh
                    DD 0374e1b05h
                    DD 0388f439bh
                    DD 038ca0e10h
                    DD 038cac08bh
                    DD 03891f65fh
                    DD 0378121cbh
                    DD 0386c9a9ah
                    DD 038949923h
                    DD 038777bcch
                    DD 037b12d26h
                    DD 038a6ced3h
                    DD 038ebd3e6h
                    DD 038fbe3cdh
                    DD 038d785c2h
                    DD 0387e7e00h
                    DD 038f392c5h
                    DD 037d40983h
                    DD 038081a7ch
                    DD 03784c3adh
                    DD 038cce923h
                    DD 0380f5fafh
                    DD 03891fd38h
                    DD 038ac47bch
                    DD 03897042bh
                    DD 0392952d2h
                    DD 0396fced4h
                    DD 037f97073h
                    DD 0385e9eaeh
                    DD 03865c84ah
                    DD 038130ba3h
                    DD 03979cf16h
                    DD 03938cac9h
                    DD 038c3d2f4h
                    DD 039755dech
                    DD 038e6b467h
                    DD 0395c0fb8h
                    DD 0383ebce0h
                    DD 038dcd192h
                    DD 039186bdfh
                    DD 0392de74ch
                    DD 0392f0944h
                    DD 0391bff61h
                    DD 038e9ed44h
                    DD 038686dc8h
                    DD 0396b99a7h
                    DD 039099c89h
                    DD 037a27673h
                    DD 0390bdaa3h
                    DD 0397069abh
                    DD 0388449ffh
                    DD 039013538h
                    DD 0392dc268h
                    DD 03947f423h
                    DD 0394ff17ch
                    DD 03945e10eh
                    DD 03929e8f5h
                    DD 038f85db0h
                    DD 038735f99h
                    DD 0396c08dbh
                    DD 03909e600h
                    DD 037b4996fh
                    DD 0391233cch
                    DD 0397cead9h
                    DD 038adb5cdh
                    DD 03920261ah
                    DD 03958ee36h
                    DD 035aa4905h
                    DD 037cbd11eh
                    DD 03805fdf4h
END
