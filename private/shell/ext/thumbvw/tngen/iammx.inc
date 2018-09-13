;/*
;*    INTEL CORPORATION PROPRIETARY INFORMATION 
;*
;*    This software is supplied under the terms of a license
;*    agreement or nondisclosure agreement with Intel Corporation 
;*    and may not be copied or disclosed except in accordance with 
;*    the terms of that agreement.
;*    Copyright (c) 1991,1992,1993  Intel Corporation. 
;*
;*/
;NOTE:NOTE:NOTE:
; IF YOU MODIFY ANY OF THE OPCODES HERE, MAKE SURE TO REFLECT THAT
; IN THE ValidMmxOpcode[] array in mm.c. 
;
;Please use MMWORD and not DWORD (QWORD does not work) for 64 bit data 
; in MMX instructions. After getting a real assembler you will just have to
; add the line " MMWORD	TEXTEQU	<QWORD>" to your code.
MMWORD	TEXTEQU	<DWORD>
opc_Rdpmc     = 033H
opc_Emms      = 077H
opc_Movd_ld   = 06EH
opc_Movd_st   = 07EH
opc_Movq_ld   = 06FH
opc_Movq_st   = 07FH
opc_Packssdw  = 06BH
opc_Packsswb  = 063H
opc_Packuswb  = 067H
opc_Paddb     = 0FCH
opc_Paddd     = 0FEH
opc_Paddsb    = 0ECH
opc_Paddsw    = 0EDH
opc_Paddusb   = 0DCH
opc_Paddusw   = 0DDH
opc_Paddw     = 0FDH
opc_Pand      = 0DBH
opc_Pandn     = 0DFH
opc_Pcmpeqb   = 074H
opc_Pcmpeqd   = 076H
opc_Pcmpeqw   = 075H
opc_Pcmpgtb   = 064H
opc_Pcmpgtd   = 066H
opc_Pcmpgtw   = 065H
opc_Pmaddwd   = 0F5H
opc_Pmulhw    = 0E5H
opc_Pmullw    = 0D5H
opc_Por       = 0EBH
opc_PSHimd    = 072H
opc_PSHimq    = 073H
opc_PSHimw    = 071H
opc_Pslld     = 0F2H
opc_Psllq     = 0F3H
opc_Psllw     = 0F1H
opc_Psrad     = 0E2H
opc_Psraw     = 0E1H
opc_Psrld     = 0D2H
opc_Psrlq     = 0D3H
opc_Psrlw     = 0D1H
opc_Psubb     = 0F8H
opc_Psubd     = 0FAH
opc_Psubsb    = 0E8H
opc_Psubsw    = 0E9H
opc_Psubusb   = 0D8H
opc_Psubusw   = 0D9H
opc_Psubw     = 0F9H
opc_Punpcklbw = 060H
opc_Punpckldq = 062H
opc_Punpcklwd = 061H
opc_Punpckhbw = 068H
opc_Punpckhdq = 06AH
opc_Punpckhwd = 069H
opc_Pxor      = 0EFH

.486P

; ALIAS R# to MM# registers

DefineMMxRegs Macro
IFDEF APP_16BIT
	MM0	TEXTEQU	<AX>
	MM1	TEXTEQU	<CX>
	MM2	TEXTEQU	<DX>
	MM3	TEXTEQU	<BX>
	MM4	TEXTEQU	<SP>
	MM5	TEXTEQU	<BP>
	MM6	TEXTEQU	<SI>
	MM7	TEXTEQU	<DI>

	mm0	TEXTEQU	<AX>
	mm1	TEXTEQU	<CX>
	mm2	TEXTEQU	<DX>
	mm3	TEXTEQU	<BX>
	mm4	TEXTEQU	<SP>
	mm5	TEXTEQU	<BP>
	mm6	TEXTEQU	<SI>
	mm7	TEXTEQU	<DI>

	Mm0	TEXTEQU	<AX>
	Mm1	TEXTEQU	<CX>
	Mm2	TEXTEQU	<DX>
	Mm3	TEXTEQU	<BX>
	Mm4	TEXTEQU	<SP>
	Mm5	TEXTEQU	<BP>
	Mm6	TEXTEQU	<SI>
	Mm7	TEXTEQU	<DI>

	mM0	TEXTEQU	<AX>
	mM1	TEXTEQU	<CX>
	mM2	TEXTEQU	<DX>
	mM3	TEXTEQU	<BX>
	mM4	TEXTEQU	<SP>
	mM5	TEXTEQU	<BP>
	mM6	TEXTEQU	<SI>
	mM7	TEXTEQU	<DI>

ELSE
	MM0	TEXTEQU	<EAX>
	MM1	TEXTEQU	<ECX>
	MM2	TEXTEQU	<EDX>
	MM3	TEXTEQU	<EBX>
	MM4	TEXTEQU	<ESP>
	MM5	TEXTEQU	<EBP>
	MM6	TEXTEQU	<ESI>
	MM7	TEXTEQU	<EDI>

	mm0	TEXTEQU	<EAX>
	mm1	TEXTEQU	<ECX>
	mm2	TEXTEQU	<EDX>
	mm3	TEXTEQU	<EBX>
	mm4	TEXTEQU	<ESP>
	mm5	TEXTEQU	<EBP>
	mm6	TEXTEQU	<ESI>
	mm7	TEXTEQU	<EDI>

	Mm0	TEXTEQU	<EAX>
	Mm1	TEXTEQU	<ECX>
	Mm2	TEXTEQU	<EDX>
	Mm3	TEXTEQU	<EBX>
	Mm4	TEXTEQU	<ESP>
	Mm5	TEXTEQU	<EBP>
	Mm6	TEXTEQU	<ESI>
	Mm7	TEXTEQU	<EDI>

	mM0	TEXTEQU	<EAX>
	mM1	TEXTEQU	<ECX>
	mM2	TEXTEQU	<EDX>
	mM3	TEXTEQU	<EBX>
	mM4	TEXTEQU	<ESP>
	mM5	TEXTEQU	<EBP>
	mM6	TEXTEQU	<ESI>
	mM7	TEXTEQU	<EDI>
ENDIF
EndM

; ALIAS R# to MM# registers
DefineMMxNUM Macro
	MM0	TEXTEQU	<0>
	MM1	TEXTEQU	<0>
	MM2	TEXTEQU	<0>
	MM3	TEXTEQU	<0>
	MM4	TEXTEQU	<0>
	MM5	TEXTEQU	<0>
	MM6	TEXTEQU	<0>
	MM7	TEXTEQU	<0>

	mm0	TEXTEQU	<0>
	mm1	TEXTEQU	<0>
	mm2	TEXTEQU	<0>
	mm3	TEXTEQU	<0>
	mm4	TEXTEQU	<0>
	mm5	TEXTEQU	<0>
	mm6	TEXTEQU	<0>
	mm7	TEXTEQU	<0>

	Mm0	TEXTEQU	<0>
	Mm1	TEXTEQU	<0>
	Mm2	TEXTEQU	<0>
	Mm3	TEXTEQU	<0>
	Mm4	TEXTEQU	<0>
	Mm5	TEXTEQU	<0>
	Mm6	TEXTEQU	<0>
	Mm7	TEXTEQU	<0>

	mM0	TEXTEQU	<0>
	mM1	TEXTEQU	<0>
	mM2	TEXTEQU	<0>
	mM3	TEXTEQU	<0>
	mM4	TEXTEQU	<0>
	mM5	TEXTEQU	<0>
	mM6	TEXTEQU	<0>
	mM7	TEXTEQU	<0>
EndM



UnDefineMMxRegs Macro
	MM0	TEXTEQU	<MM0>
	MM1	TEXTEQU	<MM1>
	MM2	TEXTEQU	<MM2>
	MM3	TEXTEQU	<MM3>
	MM4	TEXTEQU	<MM4>
	MM5	TEXTEQU	<MM5>
	MM6	TEXTEQU	<MM6>
	MM7	TEXTEQU	<MM7>

	mm0	TEXTEQU	<mm0>
	mm1	TEXTEQU	<mm1>
	mm2	TEXTEQU	<mm2>
	mm3	TEXTEQU	<mm3>
	mm4	TEXTEQU	<mm4>
	mm5	TEXTEQU	<mm5>
	mm6	TEXTEQU	<mm6>
	mm7	TEXTEQU	<mm7>

	Mm0	TEXTEQU	<Mm0>
	Mm1	TEXTEQU	<Mm1>
	Mm2	TEXTEQU	<Mm2>
	Mm3	TEXTEQU	<Mm3>
	Mm4	TEXTEQU	<Mm4>
	Mm5	TEXTEQU	<Mm5>
	Mm6	TEXTEQU	<Mm6>
	Mm7	TEXTEQU	<Mm7>

	mM0	TEXTEQU	<mM0>
	mM1	TEXTEQU	<mM1>
	mM2	TEXTEQU	<mM2>
	mM3	TEXTEQU	<mM3>
	mM4	TEXTEQU	<mM4>
	mM5	TEXTEQU	<mM5>
	mM6	TEXTEQU	<mM6>
	mM7	TEXTEQU	<mM7>
EndM


rdpmc     macro
	db	0fh, opc_Rdpmc
endm

emms     macro
	db	0fh, opc_Emms
endm



movd   macro   dst:req, src:req
        local   x, y

		DefineMMxNUM

IF (OPATTR(dst)) AND 00010000y ; register
		DefineMMxRegs
x:
        cmpxchg   dst, src
y:
        org     x+1
        byte    opc_Movd_st 
        org     y
		UnDefineMMxRegs
ELSE
		DefineMMxRegs
IF (OPATTR(dst)) AND 00010000y ; register
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Movd_ld 
        org     y
ELSE
x:
        cmpxchg   dst, src
y:
        org     x+1
        byte    opc_Movd_st 
        org     y
ENDIF
		UnDefineMMxRegs
ENDIF
        endm

movdt    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Movd_ld 
        org     y
		UnDefineMMxRegs
        endm

movdf   macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   dst, src
y:
        org     x+1
        byte    opc_Movd_st
        org     y
		UnDefineMMxRegs
        endm

movq   macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
IF (OPATTR(dst)) AND 00010000y ; register
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Movq_ld 
        org     y
ELSE
x:
        cmpxchg   dst, src
y:
        org     x+1
        byte    opc_Movq_st 
        org     y
ENDIF
		UnDefineMMxRegs
        endm


packssdw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Packssdw
        org     y
		UnDefineMMxRegs
        endm

packsswb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Packsswb
        org     y
		UnDefineMMxRegs
        endm

packuswb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Packuswb
        org     y
		UnDefineMMxRegs
        endm

paddd    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Paddd   
        org     y
		UnDefineMMxRegs
        endm

paddsb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Paddsb  
        org     y
		UnDefineMMxRegs
        endm

paddsw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Paddsw  
        org     y
		UnDefineMMxRegs
        endm

paddusb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Paddusb 
        org     y
		UnDefineMMxRegs
        endm

paddusw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Paddusw 
        org     y
		UnDefineMMxRegs
        endm

paddb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Paddb   
        org     y
		UnDefineMMxRegs
        endm

paddw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Paddw   
        org     y
		UnDefineMMxRegs
        endm

pand    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pand    
        org     y
		UnDefineMMxRegs
        endm

pandn    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pandn   
        org     y
		UnDefineMMxRegs
        endm

pcmpeqb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pcmpeqb 
        org     y
		UnDefineMMxRegs
        endm

pcmpeqd    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pcmpeqd 
        org     y
		UnDefineMMxRegs
        endm

pcmpeqw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pcmpeqw 
        org     y
		UnDefineMMxRegs
        endm

pcmpgtb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pcmpgtb 
        org     y
		UnDefineMMxRegs
        endm

pcmpgtd    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pcmpgtd 
        org     y
		UnDefineMMxRegs
        endm

pcmpgtw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pcmpgtw 
        org     y
		UnDefineMMxRegs
        endm

pmaddwd    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pmaddwd 
        org     y
		UnDefineMMxRegs
        endm

pmulhw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pmulhw  
        org     y
		UnDefineMMxRegs
        endm

pmullw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pmullw  
        org     y
		UnDefineMMxRegs
        endm

por    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Por     
        org     y
		UnDefineMMxRegs
        endm


pslld    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
IF (OPATTR(src)) AND 00000100y ; constant
x:
        btr   dst, src
y:
        org     x+1
        byte    opc_PSHimd  
        org     y
ELSE
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pslld   
        org     y
ENDIF
		UnDefineMMxRegs
        endm

psllw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
IF (OPATTR(src)) AND 00000100y ; constant
x:
        btr   dst, src
y:
        org     x+1
        byte    opc_PSHimw  
        org     y
ELSE
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psllw   
        org     y
ENDIF
		UnDefineMMxRegs
        endm

psrad    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
IF (OPATTR(src)) AND 00000100y ; constant
x:
        bt   dst, src
y:
        org     x+1
        byte    opc_PSHimd  
        org     y
ELSE
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psrad   
        org     y
ENDIF
		UnDefineMMxRegs
        endm

psraw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
IF (OPATTR(src)) AND 00000100y ; constant
x:
        bt   dst, src
y:
        org     x+1
        byte    opc_PSHimw  
        org     y
ELSE
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psraw   
        org     y
ENDIF
		UnDefineMMxRegs
        endm

psrld    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
IF (OPATTR(src)) AND 00000100y ; constant
x:
        cmpxchg dst,MM2
	byte	src
y:
        org     x+1
        byte    opc_PSHimd  
        org     y
ELSE
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psrld   
        org     y
ENDIF
		UnDefineMMxRegs
        endm

psrlq    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
IF (OPATTR(src)) AND 00000100y ; constant
x:
        cmpxchg dst,MM2
	byte	src
y:
        org     x+1
        byte    opc_PSHimq  
        org     y
ELSE
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psrlq   
        org     y
ENDIF
		UnDefineMMxRegs
        endm



psllq    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
IF (OPATTR(src)) AND 00000100y ; constant
x:
        btr   dst, src
y:
        org     x+1
        byte    opc_PSHimq  
        org     y
ELSE
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psllq   
        org     y
ENDIF
		UnDefineMMxRegs
        endm



psrlw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
IF (OPATTR(src)) AND 00000100y ; constant
x:
        cmpxchg dst,MM2
	byte	src
y:
        org     x+1
        byte    opc_PSHimw  
        org     y
ELSE
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psrlw   
        org     y
ENDIF
		UnDefineMMxRegs
        endm

psubsb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psubsb  
        org     y
		UnDefineMMxRegs
        endm

psubsw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psubsw  
        org     y
		UnDefineMMxRegs
        endm

psubusb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psubusb 
        org     y
		UnDefineMMxRegs
        endm

psubusw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psubusw 
        org     y
		UnDefineMMxRegs
        endm

psubb    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psubb   
        org     y
		UnDefineMMxRegs
        endm

psubw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psubw   
        org     y
		UnDefineMMxRegs
        endm

punpcklbw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Punpcklbw
        org     y
		UnDefineMMxRegs
        endm

punpckhdq    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Punpckhdq
        org     y
		UnDefineMMxRegs
        endm

punpcklwd    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Punpcklwd
        org     y
		UnDefineMMxRegs
        endm

punpckhbw    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Punpckhbw
        org     y
		UnDefineMMxRegs
        endm

punpckldq    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Punpckldq
        org     y
		UnDefineMMxRegs
        endm

punpckhwd    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Punpckhwd
        org     y
		UnDefineMMxRegs
        endm

pxor    macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Pxor    
        org     y
		UnDefineMMxRegs
        endm

psubd   macro   dst:req, src:req
        local   x, y
		DefineMMxRegs
x:
        cmpxchg   src, dst
y:
        org     x+1
        byte    opc_Psubd   
        org     y
		UnDefineMMxRegs
        endm
