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
;; Defines __log_F_inv_qword
;; Used in log10 and log
;;

.const

ALIGN 16
PUBLIC __log_F_inv_qword 
__log_F_inv_qword   DQ 4000000000000000h
                    DQ 3fffe01fe01fe020h
                    DQ 3fffc07f01fc07f0h
                    DQ 3fffa11caa01fa12h
                    DQ 3fff81f81f81f820h
                    DQ 3fff6310aca0dbb5h
                    DQ 3fff44659e4a4271h
                    DQ 3fff25f644230ab5h
                    DQ 3fff07c1f07c1f08h
                    DQ 3ffee9c7f8458e02h
                    DQ 3ffecc07b301ecc0h
                    DQ 3ffeae807aba01ebh
                    DQ 3ffe9131abf0b767h
                    DQ 3ffe741aa59750e4h
                    DQ 3ffe573ac901e574h
                    DQ 3ffe3a9179dc1a73h
                    DQ 3ffe1e1e1e1e1e1eh
                    DQ 3ffe01e01e01e01eh
                    DQ 3ffde5d6e3f8868ah
                    DQ 3ffdca01dca01dcah
                    DQ 3ffdae6076b981dbh
                    DQ 3ffd92f2231e7f8ah
                    DQ 3ffd77b654b82c34h
                    DQ 3ffd5cac807572b2h
                    DQ 3ffd41d41d41d41dh
                    DQ 3ffd272ca3fc5b1ah
                    DQ 3ffd0cb58f6ec074h
                    DQ 3ffcf26e5c44bfc6h
                    DQ 3ffcd85689039b0bh
                    DQ 3ffcbe6d9601cbe7h
                    DQ 3ffca4b3055ee191h
                    DQ 3ffc8b265afb8a42h
                    DQ 3ffc71c71c71c71ch
                    DQ 3ffc5894d10d4986h
                    DQ 3ffc3f8f01c3f8f0h
                    DQ 3ffc26b5392ea01ch
                    DQ 3ffc0e070381c0e0h
                    DQ 3ffbf583ee868d8bh
                    DQ 3ffbdd2b899406f7h
                    DQ 3ffbc4fd65883e7bh
                    DQ 3ffbacf914c1bad0h
                    DQ 3ffb951e2b18ff23h
                    DQ 3ffb7d6c3dda338bh
                    DQ 3ffb65e2e3beee05h
                    DQ 3ffb4e81b4e81b4fh
                    DQ 3ffb37484ad806ceh
                    DQ 3ffb2036406c80d9h
                    DQ 3ffb094b31d922a4h
                    DQ 3ffaf286bca1af28h
                    DQ 3ffadbe87f94905eh
                    DQ 3ffac5701ac5701bh
                    DQ 3ffaaf1d2f87ebfdh
                    DQ 3ffa98ef606a63beh
                    DQ 3ffa82e65130e159h
                    DQ 3ffa6d01a6d01a6dh
                    DQ 3ffa574107688a4ah
                    DQ 3ffa41a41a41a41ah
                    DQ 3ffa2c2a87c51ca0h
                    DQ 3ffa16d3f97a4b02h
                    DQ 3ffa01a01a01a01ah
                    DQ 3ff9ec8e951033d9h
                    DQ 3ff9d79f176b682dh
                    DQ 3ff9c2d14ee4a102h
                    DQ 3ff9ae24ea5510dah
                    DQ 3ff999999999999ah
                    DQ 3ff9852f0d8ec0ffh
                    DQ 3ff970e4f80cb872h
                    DQ 3ff95cbb0be377aeh
                    DQ 3ff948b0fcd6e9e0h
                    DQ 3ff934c67f9b2ce6h
                    DQ 3ff920fb49d0e229h
                    DQ 3ff90d4f120190d5h
                    DQ 3ff8f9c18f9c18fah
                    DQ 3ff8e6527af1373fh
                    DQ 3ff8d3018d3018d3h
                    DQ 3ff8bfce8062ff3ah
                    DQ 3ff8acb90f6bf3aah
                    DQ 3ff899c0f601899ch
                    DQ 3ff886e5f0abb04ah
                    DQ 3ff87427bcc092b9h
                    DQ 3ff8618618618618h
                    DQ 3ff84f00c2780614h
                    DQ 3ff83c977ab2beddh
                    DQ 3ff82a4a0182a4a0h
                    DQ 3ff8181818181818h
                    DQ 3ff8060180601806h
                    DQ 3ff7f405fd017f40h
                    DQ 3ff7e225515a4f1dh
                    DQ 3ff7d05f417d05f4h
                    DQ 3ff7beb3922e017ch
                    DQ 3ff7ad2208e0ecc3h
                    DQ 3ff79baa6bb6398bh
                    DQ 3ff78a4c8178a4c8h
                    DQ 3ff77908119ac60dh
                    DQ 3ff767dce434a9b1h
                    DQ 3ff756cac201756dh
                    DQ 3ff745d1745d1746h
                    DQ 3ff734f0c541fe8dh
                    DQ 3ff724287f46debch
                    DQ 3ff713786d9c7c09h
                    DQ 3ff702e05c0b8170h
                    DQ 3ff6f26016f26017h
                    DQ 3ff6e1f76b4337c7h
                    DQ 3ff6d1a62681c861h
                    DQ 3ff6c16c16c16c17h
                    DQ 3ff6b1490aa31a3dh
                    DQ 3ff6a13cd1537290h
                    DQ 3ff691473a88d0c0h
                    DQ 3ff6816816816817h
                    DQ 3ff6719f3601671ah
                    DQ 3ff661ec6a5122f9h
                    DQ 3ff6524f853b4aa3h
                    DQ 3ff642c8590b2164h
                    DQ 3ff63356b88ac0deh
                    DQ 3ff623fa77016240h
                    DQ 3ff614b36831ae94h
                    DQ 3ff6058160581606h
                    DQ 3ff5f66434292dfch
                    DQ 3ff5e75bb8d015e7h
                    DQ 3ff5d867c3ece2a5h
                    DQ 3ff5c9882b931057h
                    DQ 3ff5babcc647fa91h
                    DQ 3ff5ac056b015ac0h
                    DQ 3ff59d61f123ccaah
                    DQ 3ff58ed2308158edh
                    DQ 3ff5805601580560h
                    DQ 3ff571ed3c506b3ah
                    DQ 3ff56397ba7c52e2h
                    DQ 3ff5555555555555h
                    DQ 3ff54725e6bb82feh
                    DQ 3ff5390948f40febh
                    DQ 3ff52aff56a8054bh
                    DQ 3ff51d07eae2f815h
                    DQ 3ff50f22e111c4c5h
                    DQ 3ff5015015015015h
                    DQ 3ff4f38f62dd4c9bh
                    DQ 3ff4e5e0a72f0539h
                    DQ 3ff4d843bedc2c4ch
                    DQ 3ff4cab88725af6eh
                    DQ 3ff4bd3edda68fe1h
                    DQ 3ff4afd6a052bf5bh
                    DQ 3ff4a27fad76014ah
                    DQ 3ff49539e3b2d067h
                    DQ 3ff4880522014880h
                    DQ 3ff47ae147ae147bh
                    DQ 3ff46dce34596066h
                    DQ 3ff460cbc7f5cf9ah
                    DQ 3ff453d9e2c776cah
                    DQ 3ff446f86562d9fbh
                    DQ 3ff43a2730abee4dh
                    DQ 3ff42d6625d51f87h
                    DQ 3ff420b5265e5951h
                    DQ 3ff4141414141414h
                    DQ 3ff40782d10e6566h
                    DQ 3ff3fb013fb013fbh
                    DQ 3ff3ee8f42a5af07h
                    DQ 3ff3e22cbce4a902h
                    DQ 3ff3d5d991aa75c6h
                    DQ 3ff3c995a47babe7h
                    DQ 3ff3bd60d9232955h
                    DQ 3ff3b13b13b13b14h
                    DQ 3ff3a524387ac822h
                    DQ 3ff3991c2c187f63h
                    DQ 3ff38d22d366088eh
                    DQ 3ff3813813813814h
                    DQ 3ff3755bd1c945eeh
                    DQ 3ff3698df3de0748h
                    DQ 3ff35dce5f9f2af8h
                    DQ 3ff3521cfb2b78c1h
                    DQ 3ff34679ace01346h
                    DQ 3ff33ae45b57bcb2h
                    DQ 3ff32f5ced6a1dfah
                    DQ 3ff323e34a2b10bfh
                    DQ 3ff3187758e9ebb6h
                    DQ 3ff30d190130d190h
                    DQ 3ff301c82ac40260h
                    DQ 3ff2f684bda12f68h
                    DQ 3ff2eb4ea1fed14bh
                    DQ 3ff2e025c04b8097h
                    DQ 3ff2d50a012d50a0h
                    DQ 3ff2c9fb4d812ca0h
                    DQ 3ff2bef98e5a3711h
                    DQ 3ff2b404ad012b40h
                    DQ 3ff2a91c92f3c105h
                    DQ 3ff29e4129e4129eh
                    DQ 3ff293725bb804a5h
                    DQ 3ff288b01288b013h
                    DQ 3ff27dfa38a1ce4dh
                    DQ 3ff27350b8812735h
                    DQ 3ff268b37cd60127h
                    DQ 3ff25e22708092f1h
                    DQ 3ff2539d7e9177b2h
                    DQ 3ff2492492492492h
                    DQ 3ff23eb79717605bh
                    DQ 3ff23456789abcdfh
                    DQ 3ff22a0122a0122ah
                    DQ 3ff21fb78121fb78h
                    DQ 3ff21579804855e6h
                    DQ 3ff20b470c67c0d9h
                    DQ 3ff2012012012012h
                    DQ 3ff1f7047dc11f70h
                    DQ 3ff1ecf43c7fb84ch
                    DQ 3ff1e2ef3b3fb874h
                    DQ 3ff1d8f5672e4abdh
                    DQ 3ff1cf06ada2811dh
                    DQ 3ff1c522fc1ce059h
                    DQ 3ff1bb4a4046ed29h
                    DQ 3ff1b17c67f2bae3h
                    DQ 3ff1a7b9611a7b96h
                    DQ 3ff19e0119e0119eh
                    DQ 3ff19453808ca29ch
                    DQ 3ff18ab083902bdbh
                    DQ 3ff1811811811812h
                    DQ 3ff1778a191bd684h
                    DQ 3ff16e0689427379h
                    DQ 3ff1648d50fc3201h
                    DQ 3ff15b1e5f75270dh
                    DQ 3ff151b9a3fdd5c9h
                    DQ 3ff1485f0e0acd3bh
                    DQ 3ff13f0e8d344724h
                    DQ 3ff135c81135c811h
                    DQ 3ff12c8b89edc0ach
                    DQ 3ff12358e75d3033h
                    DQ 3ff11a3019a74826h
                    DQ 3ff1111111111111h
                    DQ 3ff107fbbe011080h
                    DQ 3ff0fef010fef011h
                    DQ 3ff0f5edfab325a2h
                    DQ 3ff0ecf56be69c90h
                    DQ 3ff0e40655826011h
                    DQ 3ff0db20a88f4696h
                    DQ 3ff0d24456359e3ah
                    DQ 3ff0c9714fbcda3bh
                    DQ 3ff0c0a7868b4171h
                    DQ 3ff0b7e6ec259dc8h
                    DQ 3ff0af2f722eecb5h
                    DQ 3ff0a6810a6810a7h
                    DQ 3ff09ddba6af8360h
                    DQ 3ff0953f39010954h
                    DQ 3ff08cabb37565e2h
                    DQ 3ff0842108421084h
                    DQ 3ff07b9f29b8eae2h
                    DQ 3ff073260a47f7c6h
                    DQ 3ff06ab59c7912fbh
                    DQ 3ff0624dd2f1a9fch
                    DQ 3ff059eea0727586h
                    DQ 3ff05197f7d73404h
                    DQ 3ff04949cc1664c5h
                    DQ 3ff0410410410410h
                    DQ 3ff038c6b78247fch
                    DQ 3ff03091b51f5e1ah
                    DQ 3ff02864fc7729e9h
                    DQ 3ff0204081020408h
                    DQ 3ff0182436517a37h
                    DQ 3ff0101010101010h
                    DQ 3ff0080402010080h
                    DQ 3ff0000000000000h
                    DQ 0000000000000000h


END
