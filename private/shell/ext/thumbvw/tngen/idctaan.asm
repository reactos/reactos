;***************************************************************************/
;*
;*                INTEL Corporation Proprietary Information  
;*
;*      
;*                  Copyright (c) 1996 Intel Corporation.
;*                         All rights reserved.
;*
;***************************************************************************/
;			AUTHOR:  Kumar Balasubramanian 
;***************************************************************************/

;; MMX version of the "integer fast mode" within IJG decompressor code.


.nolist
include iammx.inc                   ; IAMMX Emulator Macros
MMWORD	TEXTEQU	<DWORD>
.list

.586
.model flat

_DATA SEGMENT PARA PUBLIC USE32 'DATA'
x0005000200010001  	DQ 0005000200010001h
x0040000000000000   DQ 40000000000000h

x5a825a825a825a82	DW 16ah, 0h, 16ah, 0h  ; 23170---1.414
x539f539f539f539f 	DW 0fd63h, 0h, 0fd63h, 0h  ; 21407---2.613
x4546454645464546	DW 115h, 0h, 115h, 0h  ; 17734---1.082
x61f861f861f861f8	DW 1d9h, 0h, 1d9h, 0h  ; 25080---1.847

const_mask  DQ 3ff03ff03ff03ffh
const_zero  DQ 0
scratch1	DQ 0
scratch3	DQ 0
scratch5	DQ 0
scratch7	DQ 0
; for debug only
x0	DQ 0



preSC DW 16384, 16384, 16384, 16384,  16384, 16384, 16384, 16384
	  DW 16384, 16384, 16384, 16384,  16384, 16384, 16384, 16384
	  DW 16384, 16384, 16384, 16384,  16384, 16384, 16384, 16384
	  DW 16384, 16384, 16384, 16384,  16384, 16384, 16384, 16384
	  DW 16384, 16384, 16384, 16384,  16384, 16384, 16384, 16384
	  DW 16384, 16384, 16384, 16384,  16384, 16384, 16384, 16384
	  DW 16384, 16384, 16384, 16384,  16384, 16384, 16384, 16384
	  DW 16384, 16384, 16384, 16384,  16384, 16384, 16384, 16384



_DATA ENDS


_TEXT SEGMENT PARA PUBLIC USE32 'CODE'


PackMulW	MACRO
movq		mm0, mmword ptr scratch1
punpcklwd	mm0, mmword ptr const_zero
pmaddwd		mm0, mmword ptr scratch3
psrad		mm0, 8
movq		mm1, mmword ptr scratch1
punpckhwd	mm1, mmword ptr const_zero
pmaddwd		mm1, mmword ptr scratch3
psrad		mm1, 8
movq		mmword ptr scratch1, mm1
movq		mm1, mm0
punpcklwd	mm0, mmword ptr scratch1
punpckhwd	mm1, mmword ptr scratch1
punpcklwd	mm0, mm1
movq		mmword ptr scratch1, mm0

ENDM

COMMENT ^
void idct8x8aan (
    int16 *src_result);
^
public  _idct8x8aan
_idct8x8aan proc USES eax ebx ecx edx esi edi ebp



mov ebx, DWORD PTR [esp+32]   ; source coeff
mov	esi, DWORD PTR [esp+36]	  ; temp results
mov edi, DWORD PTR [esp+40]	  ; quant factors
;slot

; column 0: even part
; use V4, V12, V0, V8 to produce V22..V25
;slot

movq mm0, mmword ptr [ebx+8*12]	; V12
pmullw mm0, mmword ptr [edi+8*12]
;slot

movq mm1, mmword ptr [ebx+8*4]	; V4
pmullw mm1, mmword ptr [edi+8*4]
;slot

movq mm3, mmword ptr [ebx+8*0]	; V0
pmullw mm3, mmword ptr [edi+8*0]
;slot

movq mm2, mm1						; duplicate V4

movq mm5, mmword ptr [ebx+8*8]	; V8
pmullw mm5, mmword ptr [edi+8*8]
psubw mm1, mm0						; V16

movq	mmword ptr scratch1, mm1
movq mm1, mmword ptr x5a825a825a825a82	; 23170 ->V18
movq	mmword ptr scratch3, mm1
movq mmword ptr scratch5, mm0
PackMulW
movq	mm1, mmword ptr scratch1
movq mm0, mmword ptr scratch5

paddw mm2, mm0						; V17

movq mm0, mm2						; duplicate V17

movq mm4, mm3						; duplicate V0

paddw mm3, mm5						; V19
psubw mm4, mm5						; V20 ;mm5 free

movq mm6, mm3						; duplicate t74=t81

psubw mm1, mm0						; V21 ; mm0 free
paddw mm3, mm2						; V22

movq mm5, mm1						; duplicate V21
paddw mm1, mm4						; V23

movq mmword ptr [esi+8*4], mm3		; V22
psubw mm4, mm5						; V24; mm5 free

movq mmword ptr [esi+8*12], mm1		; V23
psubw mm6, mm2						; V25; mm2 free

movq mmword ptr [esi+8*0], mm4		; V24
;slot


movq mm7, mmword ptr [ebx+8*10]	; V10
pmullw mm7, mmword ptr [edi+8*10]
;slot

movq mm0, mmword ptr [ebx+8*6]	; V6
pmullw mm0, mmword ptr [edi+8*6]
;slot

movq mm3, mm7						; duplicate V10

movq mm5, mmword ptr [ebx+8*2]	; V2
pmullw mm5, mmword ptr [edi+8*2]
;slot

psubw mm7, mm0						; V26

movq mm4, mmword ptr [ebx+8*14]	; V14
pmullw mm4, mmword ptr [edi+8*14]
paddw mm3, mm0						; V29 ; free mm0

movq mm1, mm7						; duplicate V26

movq	mmword ptr scratch1, mm7
movq mm7, mmword ptr x539f539f539f539f	; 23170 ->V18
movq	mmword ptr scratch3, mm7
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm7, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7


movq mm0, mm5						; duplicate V2

paddw mm5, mm4						; V27
psubw mm0, mm4						; V28 ; free mm4

movq mm2, mm0						; duplicate V28


movq	mmword ptr scratch1, mm0
movq mm0, mmword ptr x4546454645464546	; 23170 ->V18
movq	mmword ptr scratch3, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm0, mmword ptr scratch1
movq mm1, mmword ptr scratch7


movq mm4, mm5						; duplicate t90=t93
paddw mm1, mm2						; V32 ; free mm2

movq	mmword ptr scratch1, mm1
movq mm1, mmword ptr x61f861f861f861f8	; 23170 ->V18
movq	mmword ptr scratch3, mm1
movq mmword ptr scratch5, mm0
PackMulW
movq	mm1, mmword ptr scratch1
movq mm0, mmword ptr scratch5

paddw mm5, mm3						; V31
psubw mm4, mm3						; V30 ; free mm3


movq	mmword ptr scratch1, mm4
movq mm4, mmword ptr x5a825a825a825a82	; 23170 ->V18
movq	mmword ptr scratch3, mm4
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm4, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7


psubw mm0, mm1						; V38
paddw mm1, mm7						; V37 ; free mm7
							  
movq mm3, mm6                       ; duplicate V25

;move from the next block
movq mm7, mmword ptr [esi+8*4]		; V22

psubw mm1, mm5						; V39 (mm5 still needed for next block)

;move from the next block
movq mm2, mmword ptr [esi+8*12]     ; V23

psubw mm4, mm1						; V40

paddw mm0, mm4						; V41; free mm0

; column 0: output butterfly

psubw mm6, mm0						; tm6
paddw mm3, mm0						; tm8; free mm1

movq mm0, mm1		; line added by Kumar

movq mm1, mm7						; duplicate V22
paddw mm7, mm5						; tm0

movq mmword ptr [esi+8*8], mm3     ; tm8; free mm3
psubw mm1, mm5						; tm14; free mm5

movq mmword ptr [esi+8*6], mm6		; tm6; free mm6
movq mm3, mm2						; duplicate t117=t125

movq mm6, mmword ptr [esi+8*0]		; V24
paddw mm2, mm0						; tm2

movq mmword ptr [esi+8*0], mm7      ; tm0; free mm7
psubw mm3, mm0						; tm12; free mm0

movq mmword ptr [esi+8*14], mm1		; tm14; free mm1

movq mmword ptr [esi+8*2], mm2		; tm2; free mm2
movq mm0, mm6						; duplicate t119=t123

movq mmword ptr [esi+8*12], mm3      ; tm12; free mm3
paddw mm6, mm4						; tm4

psubw mm0, mm4						; tm10; free mm4

movq mm1, mmword ptr [ebx+8*5]	; V5
pmullw mm1, mmword ptr [edi+8*5]

movq mmword ptr [esi+8*4], mm6		; tm4; free mm6

movq mmword ptr [esi+8*10], mm0     ; tm10; free mm0

; column 1: even part
; use V5, V13, V1, V9 to produce V56..V59

movq mm7, mmword ptr [ebx+8*13]	; V13
pmullw mm7, mmword ptr [edi+8*13]
movq mm2, mm1						; duplicate t128=t130

movq mm3, mmword ptr [ebx+8*1]	; V1
pmullw mm3, mmword ptr [edi+8*1]

psubw mm1, mm7						; V50

movq mm5, mmword ptr [ebx+8*9]	; V9
pmullw mm5, mmword ptr [edi+8*9]
paddw mm2, mm7						; V51


movq	mmword ptr scratch1, mm1
movq mm1, mmword ptr x5a825a825a825a82	; 23170 ->V18
movq	mmword ptr scratch3, mm1
movq mmword ptr scratch5, mm0
PackMulW
movq	mm1, mmword ptr scratch1
movq mm0, mmword ptr scratch5

movq mm6, mm2						; duplicate V51

movq mm4, mm3						; duplicate V1

paddw mm3, mm5						; V53

psubw mm4, mm5						; V54 ;mm5 free
movq mm7, mm3						; duplicate V53

psubw mm1, mm6						; V55 ; mm6 free
paddw mm3, mm2						; V56

movq mm5, mm4						; duplicate t140=t142
paddw mm4, mm1						; V57

movq mmword ptr [esi+8*5], mm3		; V56
psubw mm5, mm1						; V58; mm1 free

movq mmword ptr [esi+8*13], mm4		; V57
psubw mm7, mm2						; V59; mm2 free

movq mmword ptr [esi+8*9], mm5		; V58


movq mm0, mmword ptr [ebx+8*11]	; V11
pmullw mm0, mmword ptr [edi+8*11]

movq mm6, mmword ptr [ebx+8*7]	; V7
pmullw mm6, mmword ptr [edi+8*7]

movq mm3, mm0						; duplicate V11

movq mm4, mmword ptr [ebx+8*15]	; V15
pmullw mm4, mmword ptr [edi+8*15]

movq mm5, mmword ptr [ebx+8*3]	; V3
pmullw mm5, mmword ptr [edi+8*3]
paddw mm0, mm6						; V63

; note that V15 computation has a correction step:
; this is a 'magic' constant that rebiases the results to be closer to the expected result
; this magic constant can be refined to reduce the error even more
; by doing the correction step in a later stage when the number is actually multiplied by 16
psubw mm3, mm6						; V60 ; free mm6

movq mm1, mm3						; duplicate V60


movq	mmword ptr scratch1, mm1
movq mm1, mmword ptr x539f539f539f539f	; 23170 ->V18
movq	mmword ptr scratch3, mm1
movq mmword ptr scratch5, mm0
PackMulW
movq	mm1, mmword ptr scratch1
movq mm0, mmword ptr scratch5

movq mm6, mm5						; duplicate V3

paddw mm5, mm4						; V61
psubw mm6, mm4						; V62 ; free mm4

movq mm4, mm5						; duplicate V61

paddw mm5, mm0						; V65 -> result
psubw mm4, mm0						; V64 ; free mm0


movq	mmword ptr scratch1, mm4
movq mm4, mmword ptr x5a825a825a825a82	; 23170 ->V18
movq	mmword ptr scratch3, mm4
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm4, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7


paddw mm3, mm6						; V66
movq mm2, mm5					; duplicate V65


movq	mmword ptr scratch1, mm3
movq mm3, mmword ptr x61f861f861f861f8	; 23170 ->V18
movq	mmword ptr scratch3, mm3
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm3, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7

movq	mmword ptr scratch1, mm6
movq mm6, mmword ptr x4546454645464546	; 23170 ->V18
movq	mmword ptr scratch3, mm6
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm6, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7

movq mm0, mmword ptr [esi+8*5]		; V56

psubw mm6, mm3						; V72
paddw mm3, mm1						; V71 ; free mm1

psubw mm3, mm2						; V73 ; free mm2

psubw mm4, mm3						; V74

;moved from next block
movq mm1, mm0						; duplicate t177=t188

paddw mm6, mm4						; V75

;moved from next block
paddw mm0, mm5						; tm1

;location 
;  5 - V56
; 13 - V57
;  9 - V58
;  X - V59, mm7
;  X - V65, mm5
;  X - V73, mm6
;  X - V74, mm4
;  X - V75, mm3                              
; free mm0, mm1 & mm2                        
;move above

movq mm2, mmword ptr [esi+8*13]     ; V57
psubw mm1, mm5						; tm15; free mm5

movq mmword ptr [esi+8*1], mm0      ; tm1; free mm0

;save the store as used directly in the transpose
;movq mmword ptr [esi+8*15], mm1		; tm15; free mm1
movq mm5, mm7                       ; duplicate t182=t184

psubw mm7, mm6						; tm7

paddw mm5, mm6						; tm9; free mm3
;slot

movq mm6, mm3

movq mm0, mmword ptr [esi+8*9]		; V58
movq mm3, mm2						; duplicate V57

movq mmword ptr [esi+8*7], mm7		; tm7; free mm7
psubw mm3, mm6						; tm13

paddw mm2, mm6						; tm3 ; free mm6

movq mm6, mm0						; duplicate V58

movq mmword ptr [esi+8*3], mm2		; tm3; free mm2
paddw mm0, mm4						; tm5

psubw mm6, mm4						; tm11; free mm4

movq mmword ptr [esi+8*5], mm0		; tm5; free mm0


; transpose the bottom right quadrant(4X4) of the matrix
;  ---------       ---------
; | M1 | M2 |     | M1'| M3'|
;  ---------  -->  ---------
; | M3 | M4 |     | M2'| M4'|
;  ---------       ---------

movq		mm0, mm5			; copy w4---0,1,3,5,6
punpcklwd	mm5, mm6			;

punpckhwd	mm0, mm6			;---0,1,3,5,6 
movq	mm6, mmword ptr [esi+8*0]  ;get w0 of top left quadrant

movq		mm2, mm3			;---0,1,2,3,5,6
punpcklwd	mm3, mm1			;

movq	mm7, mmword ptr [esi+8*2]  ;get w1 of top left quadrant
punpckhwd	mm2, mm1			;---0,2,3,5,6,7

movq		mm4, mm5			;---0,2,3,4,5,6,7
punpckldq	mm5, mm3			; transposed w4

movq	mmword ptr [esi+8*9], mm5  ; store w4
punpckhdq	mm4, mm3			; transposed w5---0,2,4,6,7

movq		mm3, mm0			;---0,2,3,4,6,7
punpckldq	mm0, mm2			; transposed w6

movq	mmword ptr [esi+8*11], mm4  ; store w5
punpckhdq	mm3, mm2			; transposed w7---0,3,6,7

movq	mmword ptr [esi+8*13], mm0  ; store w6---3,5,6,7	
movq	mm5, mm6				; copy w0

movq	mmword ptr [esi+8*15], mm3  ; store w7---5,6,7
punpcklwd	mm6, mm7

; transpose the top left quadrant(4X4) of the matrix

punpckhwd	mm5, mm7			;---5,6,7
movq	mm7, mmword ptr [esi+8*4]  ; get w2 of TL quadrant

movq	mm4, mmword ptr [esi+8*6]  ; get w3 of TL quadrant
movq	mm3, mm7				; copy w2---3,4,5,6,7

movq		mm2, mm6
punpcklwd	mm7, mm4			;---2,3,4,5,6,7

punpckhwd	mm3, mm4			;---2,3,4,5,6,7
movq		mm4, mm5			;	

movq		mm1, mm5
punpckldq	mm6, mm7			;---1,2,3,4,5,6,7

movq	mmword ptr [esi+8*0], mm6	; store w0 of TL quadrant
punpckhdq	mm2, mm7			;---1,2,3,4,5,6,7

movq	mmword ptr [esi+8*2], mm2	; store w1 of TL quadrant
punpckldq	mm5, mm3			;---1,2,3,4,5,6,7

movq	mmword ptr [esi+8*4], mm5	; store w2 of TL quadrant
punpckhdq	mm1, mm3			;---1,2,3,4,5,6,7

movq	mmword ptr [esi+8*6], mm1	; store w3 of TL quadrant


; transpose the top right quadrant(4X4) of the matrix

movq	mm0, mmword ptr [esi+8*1]	;---0

movq	mm1, mmword ptr [esi+8*3]	;---0,1,2
movq	mm2, mm0

movq	mm3, mmword ptr [esi+8*5]
punpcklwd	mm0, mm1				;---0,1,2,3

punpckhwd	mm2, mm1
movq	mm1, mmword ptr [esi+8*7]	;---0,1,2,3

movq	mm4, mm3
punpcklwd	mm3, mm1				;---0,1,2,3,4

punpckhwd	mm4, mm1				;---0,1,2,3,4
movq	mm1, mm0

movq	mm5, mm2
punpckldq	mm0, mm3				;---0,1,2,3,4,5

punpckhdq	mm1, mm3				;---0,1,2,3,4,5
movq		mm3, mmword ptr [esi+8*8]

movq		mmword ptr [esi+8*8], mm0
punpckldq	mm2, mm4				;---1,2,3,4,5

punpckhdq	mm5, mm4				;---1,2,3,4,5
movq		mm4, mmword ptr [esi+8*10]

; transpose the bottom left quadrant(4X4) of the matrix
; Also store w1,w2,w3 of top right quadrant into
; w5,w6,w7 of bottom left quadrant. Storing w0 of TR in w4
; of BL is already done.

movq	mmword ptr [esi+8*10], mm1
movq	mm1, mm3					;---1,2,3,4,5

movq	mm0, mmword ptr [esi+8*12]
punpcklwd	mm3, mm4				;---0,1,2,3,4,5

punpckhwd	mm1, mm4				;---0,1,2,3,4,5
movq	mm4, mmword ptr [esi+8*14]

movq	mmword ptr [esi+8*12], mm2
movq	mm2, mm0

movq	mmword ptr [esi+8*14], mm5
punpcklwd	mm0, mm4				;---0,1,2,3,4

punpckhwd	mm2, mm4				;---0,1,2,3,4
movq	mm4, mm3

movq	mm5, mm1
punpckldq	mm3, mm0				;---0,1,2,3,4,5

movq	mmword ptr [esi+8*1], mm3
punpckhdq	mm4, mm0				;---1,2,4,5

movq	mmword ptr [esi+8*3], mm4
punpckldq	mm1, mm2				;---1,2,5

movq	mmword ptr [esi+8*5], mm1
punpckhdq	mm5, mm2				;---5

movq	mmword ptr [esi+8*7], mm5

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;   1D DCT of the rows    ;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;




mov	esi, DWORD PTR [esp+36]	  ; source
;slot

; column 0: even part
; use V4, V12, V0, V8 to produce V22..V25

movq mm0, mmword ptr [esi+8*12]	; V12

movq mm1, mmword ptr [esi+8*4]	; V4

movq mm3, mmword ptr [esi+8*0]	; V0

movq mm2, mm1						; duplicate V4

movq mm5, mmword ptr [esi+8*8]	; V8
psubw mm1, mm0						; V16

movq	mmword ptr scratch1, mm1
movq mm1, mmword ptr x5a825a825a825a82	; 23170 ->V18
movq	mmword ptr scratch3, mm1
movq mmword ptr scratch5, mm0
PackMulW
movq	mm1, mmword ptr scratch1
movq mm0, mmword ptr scratch5

paddw mm2, mm0						; V17

movq mm0, mm2						; duplicate V17

movq mm4, mm3						; duplicate V0

paddw mm3, mm5						; V19
psubw mm4, mm5						; V20 ;mm5 free

;moved from the block below
movq mm6, mm3						; duplicate t74=t81

psubw mm1, mm0						; V21 ; mm0 free
paddw mm3, mm2						; V22

movq mm5, mm1						; duplicate V21
paddw mm1, mm4						; V23

movq mmword ptr [esi+8*4], mm3		; V22
psubw mm4, mm5						; V24; mm5 free

movq mmword ptr [esi+8*12], mm1		; V23
psubw mm6, mm2						; V25; mm2 free

movq mmword ptr [esi+8*0], mm4		; V24

; keep mm6 alive all along the next block

; column 0: odd part
; use V2, V6, V10, V14 to produce V31, V39, V40, V41


movq mm7, mmword ptr [esi+8*10]	; V10

movq mm0, mmword ptr [esi+8*6]	; V6

movq mm3, mm7						; duplicate V10

movq mm5, mmword ptr [esi+8*2]	; V2

psubw mm7, mm0						; V26

movq mm4, mmword ptr [esi+8*14]	; V14
paddw mm3, mm0						; V29 ; free mm0

movq mm1, mm7						; duplicate V26

movq	mmword ptr scratch1, mm7
movq mm7, mmword ptr x539f539f539f539f	; 23170 ->V18
movq	mmword ptr scratch3, mm7
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm7, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7



movq mm0, mm5						; duplicate V2

paddw mm5, mm4						; V27
psubw mm0, mm4						; V28 ; free mm4

movq mm2, mm0						; duplicate V28


movq	mmword ptr scratch1, mm0
movq mm0, mmword ptr x4546454645464546	; 23170 ->V18
movq	mmword ptr scratch3, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm0, mmword ptr scratch1
movq mm1, mmword ptr scratch7


movq mm4, mm5						; duplicate t90=t93
paddw mm1, mm2						; V32 ; free mm2

movq	mmword ptr scratch1, mm1
movq mm1, mmword ptr x61f861f861f861f8	; 23170 ->V18
movq	mmword ptr scratch3, mm1
movq mmword ptr scratch5, mm0
PackMulW
movq	mm1, mmword ptr scratch1
movq mm0, mmword ptr scratch5


paddw mm5, mm3						; V31
psubw mm4, mm3						; V30 ; free mm3


movq	mmword ptr scratch1, mm4
movq mm4, mmword ptr x5a825a825a825a82	; 23170 ->V18
movq	mmword ptr scratch3, mm4
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm4, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7


psubw mm0, mm1						; V38
paddw mm1, mm7						; V37 ; free mm7
							  
;move from the next block
movq mm3, mm6                       ; duplicate V25

;move from the next block
movq mm7, mmword ptr [esi+8*4]		; V22

psubw mm1, mm5						; V39 (mm5 still needed for next block)

;move from the next block
movq mm2, mmword ptr [esi+8*12]     ; V23

psubw mm4, mm1						; V40

paddw mm0, mm4						; V41; free mm0
;move from the next block

; column 0: output butterfly
;move above
psubw mm6, mm0						; tm6
paddw mm3, mm0						; tm8; free mm1

movq mm0, mm1		; line added by Kumar

movq mm1, mm7						; duplicate V22
paddw mm7, mm5						; tm0

movq mmword ptr [esi+8*8], mm3     ; tm8; free mm3
psubw mm1, mm5						; tm14; free mm5

movq mmword ptr [esi+8*6], mm6		; tm6; free mm6
movq mm3, mm2						; duplicate t117=t125

movq mm6, mmword ptr [esi+8*0]		; V24
paddw mm2, mm0						; tm2

movq mmword ptr [esi+8*0], mm7      ; tm0; free mm7
psubw mm3, mm0						; tm12; free mm0

movq mmword ptr [esi+8*14], mm1		; tm14; free mm1

movq mmword ptr [esi+8*2], mm2		; tm2; free mm2
movq mm0, mm6						; duplicate t119=t123

movq mmword ptr [esi+8*12], mm3      ; tm12; free mm3
paddw mm6, mm4						; tm4

;moved from next block
psubw mm0, mm4						; tm10; free mm4

;moved from next block
movq mm1, mmword ptr [esi+8*5]	; V5

movq mmword ptr [esi+8*4], mm6		; tm4; free mm6

movq mmword ptr [esi+8*10], mm0     ; tm10; free mm0

; column 1: even part
; use V5, V13, V1, V9 to produce V56..V59
;moved to prev block

movq mm7, mmword ptr [esi+8*13]	; V13
movq mm2, mm1						; duplicate t128=t130

movq mm3, mmword ptr [esi+8*1]	; V1

psubw mm1, mm7						; V50

movq mm5, mmword ptr [esi+8*9]	; V9
paddw mm2, mm7						; V51


movq	mmword ptr scratch1, mm1
movq mm1, mmword ptr x5a825a825a825a82	; 23170 ->V18
movq	mmword ptr scratch3, mm1
movq mmword ptr scratch5, mm0
PackMulW
movq	mm1, mmword ptr scratch1
movq mm0, mmword ptr scratch5


movq mm6, mm2						; duplicate V51

movq mm4, mm3						; duplicate V1

paddw mm3, mm5						; V53

psubw mm4, mm5						; V54 ;mm5 free
movq mm7, mm3						; duplicate V53

;moved from next block

psubw mm1, mm6						; V55 ; mm6 free
paddw mm3, mm2						; V56

movq mm5, mm4						; duplicate t140=t142
paddw mm4, mm1						; V57

movq mmword ptr [esi+8*5], mm3		; V56
psubw mm5, mm1						; V58; mm1 free

movq mmword ptr [esi+8*13], mm4		; V57
psubw mm7, mm2						; V59; mm2 free

movq mmword ptr [esi+8*9], mm5		; V58

; keep mm7 alive all along the next block

movq mm0, mmword ptr [esi+8*11]	; V11


movq mm6, mmword ptr [esi+8*7]	; V7

movq mm3, mm0						; duplicate V11

movq mm4, mmword ptr [esi+8*15]	; V15


movq mm5, mmword ptr [esi+8*3]	; V3
paddw mm0, mm6						; V63

; note that V15 computation has a correction step:
; this is a 'magic' constant that rebiases the results to be closer to the expected result
; this magic constant can be refined to reduce the error even more
; by doing the correction step in a later stage when the number is actually multiplied by 16
psubw mm3, mm6						; V60 ; free mm6

movq mm1, mm3						; duplicate V60


movq	mmword ptr scratch1, mm1
movq mm1, mmword ptr x539f539f539f539f	; 23170 ->V18
movq	mmword ptr scratch3, mm1
movq mmword ptr scratch5, mm0
PackMulW
movq	mm1, mmword ptr scratch1
movq mm0, mmword ptr scratch5



movq mm6, mm5						; duplicate V3


paddw mm5, mm4						; V61
psubw mm6, mm4						; V62 ; free mm4

movq mm4, mm5						; duplicate V61

paddw mm5, mm0						; V65 -> result
psubw mm4, mm0						; V64 ; free mm0


movq	mmword ptr scratch1, mm4
movq mm4, mmword ptr x5a825a825a825a82	; 23170 ->V18
movq	mmword ptr scratch3, mm4
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm4, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7

paddw mm3, mm6						; V66
movq mm2, mm5					; duplicate V65


movq	mmword ptr scratch1, mm3
movq mm3, mmword ptr x61f861f861f861f8	; 23170 ->V18
movq	mmword ptr scratch3, mm3
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm3, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7


movq	mmword ptr scratch1, mm6
movq mm6, mmword ptr x4546454645464546	; 23170 ->V18
movq	mmword ptr scratch3, mm6
movq mmword ptr scratch5, mm0
movq mmword ptr scratch7, mm1
PackMulW
movq	mm6, mmword ptr scratch1
movq mm0, mmword ptr scratch5
movq mm1, mmword ptr scratch7

;moved from next block
movq mm0, mmword ptr [esi+8*5]		; V56


psubw mm6, mm3						; V72
paddw mm3, mm1						; V71 ; free mm1

psubw mm3, mm2						; V73 ; free mm2

psubw mm4, mm3						; V74

;moved from next block
movq mm1, mm0						; duplicate t177=t188

paddw mm6, mm4						; V75

;moved from next block
paddw mm0, mm5						; tm1

;location 
;  5 - V56
; 13 - V57
;  9 - V58
;  X - V59, mm7
;  X - V65, mm5
;  X - V73, mm6
;  X - V74, mm4
;  X - V75, mm3                              
; free mm0, mm1 & mm2                        
;move above

movq mm2, mmword ptr [esi+8*13]     ; V57
psubw mm1, mm5						; tm15; free mm5

movq mmword ptr [esi+8*1], mm0      ; tm1; free mm0

;save the store as used directly in the transpose
movq mm5, mm7                       ; duplicate t182=t184

psubw mm7, mm6						; tm7

paddw mm5, mm6						; tm9; free mm3

movq mm6, mm3

movq mm0, mmword ptr [esi+8*9]		; V58
movq mm3, mm2						; duplicate V57

movq mmword ptr [esi+8*7], mm7		; tm7; free mm7
psubw mm3, mm6						; tm13

paddw mm2, mm6						; tm3 ; free mm6

movq mm6, mm0						; duplicate V58

movq mmword ptr [esi+8*3], mm2		; tm3; free mm2
paddw mm0, mm4						; tm5

psubw mm6, mm4						; tm11; free mm4

movq mmword ptr [esi+8*5], mm0		; tm5; free mm0



; Final results to be stored after the transpose
; transpose the bottom right quadrant(4X4) of the matrix
;  ---------       ---------
; | M1 | M2 |     | M1'| M3'|
;  ---------  -->  ---------
; | M3 | M4 |     | M2'| M4'|
;  ---------       ---------
;
; get the pointer to array "range"
mov		edi, [esp+52]

; calculate the destination address
mov		ebp,  [esp+44]		; get output_buf[4]

mov		ebx, [ebp+20]
mov		ecx, [ebp+24]			
mov		edx, [ebp+28]		
mov		ebp, [ebp+16]

add		ebp,  [esp+48]			; add to output_col	
add		ebx,  [esp+48]			; add to output_col	
add		ecx,  [esp+48]			; add to output_col	
add		edx,  [esp+48]			; add to output_col	


movq		mm0, mm5			; copy w4---0,1,3,5,6
punpcklwd	mm5, mm6			;

punpckhwd	mm0, mm6			;---0,1,3,5,6 
movq	mm6, mmword ptr [esi+8*0]  ;get w0 of top left quadrant

movq		mm2, mm3			;---0,1,2,3,5,6
punpcklwd	mm3, mm1			;

movq	mm7, mmword ptr [esi+8*2]  ;get w1 of top left quadrant
punpckhwd	mm2, mm1			;---0,2,3,5,6,7

movq		mm4, mm5			;---0,2,3,4,5,6,7
punpckldq	mm5, mm3			; transposed w4

psrlw	mm5, 5
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+4], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+5], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+6], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+7], al


punpckhdq	mm4, mm3			; transposed w5---0,2,4,6,7

movq		mm3, mm0			;---0,2,3,4,6,7
punpckldq	mm0, mm2			; transposed w6

psrlw	mm4, 5
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+4], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+5], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+6], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+7], al

punpckhdq	mm3, mm2			; transposed w7---0,3,6,7

psrlw	mm0, 5

movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+4], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+5], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+6], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+7], al

movq	mm5, mm6				; copy w0

psrlw	mm3, 5
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+4], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+5], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+6], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+7], al

punpcklwd	mm6, mm7

; transpose the top left quadrant(4X4) of the matrix

; calculate the destination address
mov		ebp, [esp+44]		; get output_buf[0]

mov		ebx, [ebp+4]
mov		ecx, [ebp+8]			
mov		edx, [ebp+12]		
mov		ebp, [ebp+0]

add		ebp, [esp+48]			; add to output_col	
add		ebx, [esp+48]			; add to output_col	
add		ecx, [esp+48]			; add to output_col	
add		edx, [esp+48]			; add to output_col	

punpckhwd	mm5, mm7			;---5,6,7
movq	mm7, mmword ptr [esi+8*4]  ; get w2 of TL quadrant

movq	mm4, mmword ptr [esi+8*6]  ; get w3 of TL quadrant
movq	mm3, mm7				; copy w2---3,4,5,6,7

movq		mm2, mm6
punpcklwd	mm7, mm4			;---2,3,4,5,6,7

punpckhwd	mm3, mm4			;---2,3,4,5,6,7
movq		mm4, mm5			;	

movq		mm1, mm5
punpckldq	mm6, mm7			;---1,2,3,4,5,6,7

psrlw	mm6, 5
movd    eax, mm6
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp], al

psrlq	mm6, 16
movd    eax, mm6
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+1], al

psrlq	mm6, 16
movd    eax, mm6
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+2], al

psrlq	mm6, 16
movd    eax, mm6
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+3], al

punpckhdq	mm2, mm7			;---1,2,3,4,5,6,7

psrlw	mm2, 5
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+1], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+2], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+3], al

punpckldq	mm5, mm3			;---1,2,3,4,5,6,7

psrlw	mm5, 5
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+1], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+2], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+3], al

punpckhdq	mm1, mm3			;---1,2,3,4,5,6,7

psrlw	mm1, 5
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+1], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+2], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+3], al



; transpose the top right quadrant(4X4) of the matrix

; calculate the destination address for **bottom left quadrant
mov		ebp, [esp+44]		; get output_buf[4]

mov		ebx, [ebp+20]
mov		ecx, [ebp+24]			
mov		edx, [ebp+28]		
mov		ebp, [ebp+16]

add		ebp, [esp+48]			; add to output_col	
add		ebx, [esp+48]			; add to output_col	
add		ecx, [esp+48]			; add to output_col	
add		edx, [esp+48]			; add to output_col	


movq	mm0, mmword ptr [esi+8*1]	;---0

movq	mm1, mmword ptr [esi+8*3]	;---0,1,2
movq	mm2, mm0

movq	mm3, mmword ptr [esi+8*5]
punpcklwd	mm0, mm1				;---0,1,2,3

punpckhwd	mm2, mm1
movq	mm1, mmword ptr [esi+8*7]	;---0,1,2,3

movq	mm4, mm3
punpcklwd	mm3, mm1				;---0,1,2,3,4

punpckhwd	mm4, mm1				;---0,1,2,3,4
movq	mm1, mm0

movq	mm5, mm2
punpckldq	mm0, mm3				;---0,1,2,3,4,5

punpckhdq	mm1, mm3				;---0,1,2,3,4,5
movq		mm3, mmword ptr [esi+8*8]

psrlw	mm0, 5
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+1], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+2], al

psrlq	mm0, 16
movd    eax, mm0
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+3], al


punpckldq	mm2, mm4				;---1,2,3,4,5

punpckhdq	mm5, mm4				;---1,2,3,4,5
movq		mm4, mmword ptr [esi+8*10]

; transpose the bottom left quadrant(4X4) of the matrix
; Also store w1,w2,w3 of top right quadrant into
; w5,w6,w7 of bottom left quadrant. Storing w0 of TR in w4
; of BL is already done.

psrlw	mm1, 5
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+1], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+2], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+3], al

movq	mm1, mm3					;---1,2,3,4,5

movq	mm0, mmword ptr [esi+8*12]
punpcklwd	mm3, mm4				;---0,1,2,3,4,5

punpckhwd	mm1, mm4				;---0,1,2,3,4,5
movq	mm4, mmword ptr [esi+8*14]

psrlw	mm2, 5
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+1], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+2], al

psrlq	mm2, 16
movd    eax, mm2
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+3], al


movq	mm2, mm0

psrlw	mm5, 5
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx], al


psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+1], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+2], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+3], al


punpcklwd	mm0, mm4				;---0,1,2,3,4

punpckhwd	mm2, mm4				;---0,1,2,3,4
movq	mm4, mm3

movq	mm5, mm1
punpckldq	mm3, mm0				;---0,1,2,3,4,5

; calculate the destination address for **top right quadrant
mov		ebp, [esp+44]		; get output_buf[0]

mov		ebx, [ebp+4]
mov		ecx, [ebp+8]			
mov		edx, [ebp+12]		
mov		ebp, [ebp+0]

add		ebp, [esp+48]			; add to output_col	
add		ebx, [esp+48]			; add to output_col	
add		ecx, [esp+48]			; add to output_col	
add		edx, [esp+48]			; add to output_col	


psrlw	mm3, 5
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+4], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+5], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+6], al

psrlq	mm3, 16
movd    eax, mm3
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebp+7], al


punpckhdq	mm4, mm0				;---1,2,4,5

psrlw	mm4, 5
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+4], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+5], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+6], al

psrlq	mm4, 16
movd    eax, mm4
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ebx+7], al


punpckldq	mm1, mm2				;---1,2,5

psrlw	mm1, 5
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+4], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+5], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+6], al

psrlq	mm1, 16
movd    eax, mm1
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [ecx+7], al


punpckhdq	mm5, mm2				;---5

psrlw	mm5, 5
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+4], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+5], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+6], al

psrlq	mm5, 16
movd    eax, mm5
and		eax, 03ffh
mov		al, byte ptr [edi][eax]
mov		byte ptr [edx+7], al


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

emms


ret	

_idct8x8aan ENDP
_TEXT ENDS

END
