page    ,132
;----------------------------Module-Header------------------------------;
; Module Name: DITH666.ASM
;
; code to dither 16,24,32 bit DIBs to a 8bit DIB with a fixed palette
;
; NOTES:
;       this is a ASM version of the code found in dith666.c
;
;-----------------------------------------------------------------------;
?PLM=1
?WIN=0
	.xlist
        include cmacro32.inc
        include windows.inc
        .list

;-----------------------------------------------------------------------;
;
;   Halftone tables...
;
;	extern BYTE aHalftone8[3][4][4][256];
;       extern BYTE aHalftone5[3][4][4][256];
;       extern BYTE aHalftoneTranslate[256];
;
;   for 24 bit or 32 bit (256 levels):
;
;	pal8 = aHalftone8[0][x%4][y%4][r] +
;	       aHalftone8[1][x%4][y%4][g] +
;              aHalftone8[2][x%4][y%4][b];
;
;       pal8 = aHalftoneTranslate[pal8]
;
;   for 16 bit (32 levels):
;
;       pal8 = aHalftone5[0][x%4][y%4][rgb & 0xFF] +
;              aHalftone5[1][x%4][y%4][rgb&0xFF |] +
;              aHalftone5[2][x%4][y%4][rgb>>8];
;
;       pal8 = aHalftoneTranslate[pal8]
;
;   for fast 16 bit: (2x2 dither...)
;       pal8 = aHalftone16[y%1][x%1][rgb16]
;
;-----------------------------------------------------------------------;

sBegin  Data
sEnd    Data

        externB _aHalftone8                  ; in dith666.h
        externB _aTranslate666               ; in dith666.h

        aHalftone8R  equ <_aHalftone8>
        aHalftone8G  equ <_aHalftone8 + 4096>
        aHalftone8B  equ <_aHalftone8 + 8192>
        aTranslate   equ <_aTranslate666>

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

FARPOINTER      struc
off     dw      ?
sel     dw      ?
FARPOINTER      ends

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        .386
        assumes cs,CodeSeg
        assumes ds,nothing
        assumes es,nothing

;--------------------------------------------------------------------------;
;
;   DITH16 row
;
;       dither 4 16 bit pels
;
;   Entry:
;       fs:esi  - 16bit pel to dither
;       es:edi  - 8bit dest
;       ds      - dither tables
;
;   Uses:
;       eax, ebx, edx, ebp, esi, edi, flags
;
;   Saves:
;       ecx, edi, es,ds,fs,gs,ss
;
;   access the dither tables like so:
;
;       0 1 0 1         0 = (0,0) (what dither matrix to look at)
;       2 3 2 3         1 = (0,1)
;       0 1 0 1         2 = (1,0)
;       2 3 2 3         3 = (1,1)
;
;   the table is layed out like so
;
;           yrrrrrgggggbbbbbx
;
;--------------------------------------------------------------------------;

DITH16  macro row

        mov     edx,fs:[esi+4]      ; edx = ?rrrrrgggggbbbbb?rrrrrgggggbbbbb
        or      dh,80h              ; edx = ?rrrrrgggggbbbbb1rrrrrgggggbbbbb
        add     edx,edx             ; edx = rrrrrgggggbbbbb1rrrrrgggggbbbbb0
        mov     bx,dx               ; ebx = 0000000000000000rrrrrgggggbbbbb0
        shr     edx,16              ; edx = 0000000000000000rrrrrgggggbbbbb1
        mov     al,[ebx + ((row and 1) * 65536)]
        mov     ah,[edx + ((row and 1) * 65536)]

        shl     eax,16

        mov     edx,fs:[esi]        ; edx = ?rrrrrgggggbbbbb?rrrrrgggggbbbbb
        or      dh,80h              ; edx = ?rrrrrgggggbbbbb1rrrrrgggggbbbbb
        shl     edx,1               ; edx = rrrrrgggggbbbbb1rrrrrgggggbbbbb0
        mov     bx,dx               ; ebx = 0000000000000000rrrrrgggggbbbbb0
        shr     edx,16              ; edx = 0000000000000000rrrrrgggggbbbbb1
        mov     al,[ebx + ((row and 1) * 65536)]
        mov     ah,[edx + ((row and 1) * 65536)]

        mov     es:[edi],eax

        add     esi,8
        add     edi,4
endm

;--------------------------------------------------------------------------;
;
;   Dither16()
;
;   Entry:
;       Stack based parameters as described below.
;
;   Returns:
;       none
;
;   Registers Preserved:
;       DS,ES,ESI,EDI,EBP
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

Dither16JumpTable label dword
        dd      Dither16Scan0
        dd      Dither16Scan3
        dd      Dither16Scan2
        dd      Dither16Scan1

cProc	Dither16,<FAR,PUBLIC,PASCAL>,<>
        parmD   biDst                   ;--> BITMAPINFO of dest
        parmD   lpDst                   ;--> to destination bits
        parmW   DstX                    ;Destination origin - x coordinate
        parmW   DstY                    ;Destination origin - y coordinate
        parmW   DstXE                   ;x extent of the BLT
        parmW   DstYE                   ;y extent of the BLT
        parmD   biSrc                   ;--> BITMAPINFO of source
        parmD   lpSrc                   ;--> to source bits
        parmW   SrcX                    ;Source origin - x coordinate
        parmW   SrcY                    ;Source origin - y coordinate
        parmD   lpDitherTable           ;not used (for 8->4 bit dither)

        localD  SrcWidth                ;width of source in bytes
        localD  DstWidth                ;width of dest in bytes

        localD  SrcInc
        localD  DstInc
cBegin
        push    esi
        push    edi
        push    ds

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   We only handle (DstXE % 4) == 0 or 3.  If it's == 1 or 2, then we
;   round down, because otherwise we'd have to deal with half of a
;   dither cell on the end. 
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
	inc	DstXE			; Make the == 3 mod 4 case work
        and     DstXE, not 011b
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     eax,16
        mov     ebx,8
        call    dither_init             ; init all the frame variables
        jc      Dither16Exit

        mov     ds,word ptr lpDitherTable[2]     ; DS --> dither table
        assumes ds,nothing

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   ecx, DstYE               ; divide by 4
        mov     ebx, ecx

        add     ecx, 3                   ; be sure to round up
        shr     ecx, 2
        jz      Dither16Exit
        mov     DstYE, cx

        movzx   ecx, DstXE               ; divide by 4
        shr     ecx,2
        jz      Dither16Exit
        mov     DstXE,cx

        movzx   ecx, DstXE

        and     ebx, 011b               ; Get height mod 4
        xor     edx,edx                 ; set up for dither macros
        jmp     Dither16JumpTable[ebx*4]

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        align   4
Dither16OuterLoop:

        movzx   ecx, DstXE
        align   4
Dither16Scan0:
        DITH16  0
        dec     ecx
        jnz     Dither16Scan0

        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16Scan1:
        DITH16  1
        dec     ecx
        jnz     Dither16Scan1

        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16Scan2:
        DITH16  2
        dec     ecx
        jnz     Dither16Scan2

	add	esi, SrcInc
	add	edi, DstInc

        movzx   ecx, DstXE
	align	4
Dither16Scan3:
        DITH16  3
        dec     ecx
        jnz     Dither16Scan3

	add	esi, SrcInc
	add	edi, DstInc

	dec	DstYE
        jnz     Dither16OuterLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Dither16Exit:
        xor     ax,ax
        mov     fs,ax       ; to make KRNL286.EXE and DOSX happy
        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   GET24 - get a byte, every four times doing a LODSD
;
;--------------------------------------------------------------------------;

GET24_N = 0

GET24 macro dst
	if (GET24_N mod 4) eq 0
	    mov     edx,dword ptr fs:[esi]
	    mov     dst,dl
	    add     esi,4
        elseif (GET24_N mod 4) eq 1
	    mov     dst,dh
        elseif (GET24_N mod 4) eq 2
            shr     edx,16
	    mov     dst,dl
        elseif (GET24_N mod 4) eq 3
	    mov     dst,dh
        endif

	GET24_N = GET24_N + 1

        endm

;--------------------------------------------------------------------------;
;
;   Dither24() - dither 24 to 8
;
;   Entry:
;       Stack based parameters as described below.
;
;   Returns:
;       none
;
;   Registers Preserved:
;       DS,ES,ESI,EDI,EBP
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

DITH24	macro row, col, dst

	GET24	bl	    ; get BLUE
        mov     dst,ds:[aHalftone8B + ebx + row*256 + col*256*4]

	GET24	bl	    ; get GREEN
        add     dst,ds:[aHalftone8G + ebx + row*256 + col*256*4]

	GET24	bl	    ; get RED
        add     dst,ds:[aHalftone8R + ebx + row*256 + col*256*4]

        mov     bl,dst
        mov     dst,ds:[aTranslate + ebx]
	endm

Dither24JumpTable label dword
	dd	Dither24Scan0
	dd	Dither24Scan3
	dd	Dither24Scan2
	dd	Dither24Scan1

cProc	Dither24,<FAR,PUBLIC,PASCAL>,<>
        parmD   biDst                   ;--> BITMAPINFO of dest
        parmD   lpDst                   ;--> to destination bits
        parmW   DstX                    ;Destination origin - x coordinate
        parmW   DstY                    ;Destination origin - y coordinate
        parmW   DstXE                   ;x extent of the BLT
        parmW   DstYE                   ;y extent of the BLT
        parmD   biSrc                   ;--> BITMAPINFO of source
        parmD   lpSrc                   ;--> to source bits
        parmW   SrcX                    ;Source origin - x coordinate
        parmW   SrcY                    ;Source origin - y coordinate
        parmD   lpDitherTable           ;196k dither table

        localD  SrcWidth                ;width of source in bytes
        localD  DstWidth                ;width of dest in bytes

        localD  SrcInc
        localD  DstInc
cBegin
        push    esi
        push    edi
        push    ds

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   align everything on four pixel boundries, we realy should
;   not do this but should handle the general case instead,
;   but hey we are hackers.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;       inc     DstXE                   ; Make the == 3 mod 4 case work
        and     DstXE, not 011b
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     eax,24
        mov     ebx,8
        call    dither_init             ; init all the frame variables
        jc      Dither24Exit

        mov     ax, seg aHalftone8R
        mov     ds, ax
        assumes ds,nothing

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   ecx, DstYE               ; divide by 4
        mov     ebx, ecx

        add     ecx, 3                   ; be sure to round up
        shr     ecx, 2
	jz	Dither24Exit
        mov     DstYE, cx

        movzx   ecx, DstXE               ; divide by 4
        shr     ecx,2
	jz	Dither24Exit
        mov     DstXE,cx

        and     ebx, 011b                ; Get height mod 4
	jmp	Dither24JumpTable[ebx*4]

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        align   4
Dither24OuterLoop:

        movzx   ecx, DstXE
        align   4
Dither24Scan0:
	DITH24	0,0,al
	DITH24	0,1,ah
	shl	eax,16
	DITH24	0,2,al
	DITH24	0,3,ah
        rol     eax,16
	mov	es:[edi], eax
	add	edi,4
        dec     ecx
	jnz	Dither24Scan0
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither24Scan1:
	DITH24	1,0,al
	DITH24	1,1,ah
	shl	eax,16
	DITH24	1,2,al
	DITH24	1,3,ah
        rol     eax,16
	mov	es:[edi], eax
	add	edi,4
        dec     ecx
	jnz	Dither24Scan1
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither24Scan2:
	DITH24	2,0,al
	DITH24	2,1,ah
	shl	eax,16
	DITH24	2,2,al
	DITH24	2,3,ah
        rol     eax,16
	mov	es:[edi], eax
	add	edi,4
        dec     ecx
	jnz	Dither24Scan2
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither24Scan3:
	DITH24	3,0,al
	DITH24	3,1,ah
	shl	eax,16
	DITH24	3,2,al
	DITH24	3,3,ah
        rol     eax,16
	mov	es:[edi], eax
	add	edi,4
        dec     ecx
	jnz	Dither24Scan3
        add     esi, SrcInc
        add     edi, DstInc

        dec     DstYE
	jnz	Dither24OuterLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Dither24Exit:
        xor     ax,ax
        mov     fs,ax       ; to make KRNL286.EXE and DOSX happy
        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   GET32 - get a byte from a 32 bit DIB
;
;--------------------------------------------------------------------------;

GET32_N = 0

GET32 macro dst
	if (GET32_N mod 3) eq 0
	    mov     edx,dword ptr fs:[esi]
	    mov     dst,dl
	    add     esi,4
        elseif (GET32_N mod 3) eq 1
	    mov     dst,dh
        elseif (GET32_N mod 3) eq 2
            shr     edx,16
	    mov     dst,dl
        endif

	GET32_N = GET32_N + 1

        endm

;--------------------------------------------------------------------------;
;
;   Dither32() - dither 32 to 8
;
;   Entry:
;       Stack based parameters as described below.
;
;   Returns:
;       none
;
;   Registers Preserved:
;       DS,ES,ESI,EDI,EBP
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
	assumes es,nothing

DITH32	macro row, col, dst

	GET32	bl	    ; get BLUE
        mov     dst,ds:[aHalftone8B + ebx + row*256 + col*256*4]

	GET32	bl	    ; get GREEN
        add     dst,ds:[aHalftone8G + ebx + row*256 + col*256*4]

	GET32	bl	    ; get RED
        add     dst,ds:[aHalftone8R + ebx + row*256 + col*256*4]

        mov     bl,dst
        mov     dst,aTranslate[ebx]
	endm

Dither32JumpTable label dword
	dd	Dither32Scan0
	dd	Dither32Scan3
	dd	Dither32Scan2
	dd	Dither32Scan1

cProc	Dither32,<FAR,PUBLIC,PASCAL>,<>
        parmD   biDst                   ;--> BITMAPINFO of dest
        parmD   lpDst                   ;--> to destination bits
        parmW   DstX                    ;Destination origin - x coordinate
        parmW   DstY                    ;Destination origin - y coordinate
        parmW   DstXE                   ;x extent of the BLT
        parmW   DstYE                   ;y extent of the BLT
        parmD   biSrc                   ;--> BITMAPINFO of source
        parmD   lpSrc                   ;--> to source bits
        parmW   SrcX                    ;Source origin - x coordinate
        parmW   SrcY                    ;Source origin - y coordinate
        parmD   lpDitherTable           ;196k dither table

        localD  SrcWidth                ;width of source in bytes
        localD  DstWidth                ;width of dest in bytes

        localD  SrcInc
        localD  DstInc
cBegin
        push    esi
        push    edi
        push    ds

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   align everything on four pixel boundries, we realy should
;   not do this but should handle the general case instead,
;   but hey we are hackers.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;       inc     DstXE                   ; Make the == 3 mod 4 case work
        and     DstXE, not 011b
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     eax,32
        mov     ebx,8
        call    dither_init             ; init all the frame variables
        jc      Dither32Exit

        mov     ax, seg aHalftone8R
        mov     ds, ax
        assumes ds,nothing

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   ecx, DstYE               ; divide by 4
        mov     ebx, ecx

        add     ecx, 3                   ; be sure to round up
        shr     ecx, 2
	jz	Dither32Exit
        mov     DstYE, cx

        movzx   ecx, DstXE               ; divide by 4
        shr     ecx,2
	jz	Dither32Exit
        mov     DstXE,cx

        and     ebx, 011b                ; Get height mod 4
	jmp	Dither32JumpTable[ebx*4]

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        align   4
Dither32OuterLoop:

        movzx   ecx, DstXE
        align   4
Dither32Scan0:
	DITH32	0,0,al
	DITH32	0,1,ah
	shl	eax,16
	DITH32	0,2,al
        DITH32  0,3,ah
        rol     eax,16
	mov	es:[edi], eax
	add	edi,4
        dec     ecx
	jnz	Dither32Scan0
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither32Scan1:
	DITH32	1,0,al
	DITH32	1,1,ah
	shl	eax,16
	DITH32	1,2,al
	DITH32	1,3,ah
        rol     eax,16
	mov	es:[edi], eax
	add	edi,4
        dec     ecx
	jnz	Dither32Scan1
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither32Scan2:
	DITH32	2,0,al
	DITH32	2,1,ah
	shl	eax,16
	DITH32	2,2,al
	DITH32	2,3,ah
        rol     eax,16
	mov	es:[edi], eax
	add	edi,4
        dec     ecx
	jnz	Dither32Scan2
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither32Scan3:
	DITH32	3,0,al
	DITH32	3,1,ah
	shl	eax,16
	DITH32	3,2,al
	DITH32	3,3,ah
        rol     eax,16
	mov	es:[edi], eax
	add	edi,4
        dec     ecx
	jnz	Dither32Scan3
        add     esi, SrcInc
        add     edi, DstInc

        dec     DstYE
	jnz	Dither32OuterLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Dither32Exit:
        xor     ax,ax
        mov     fs,ax       ; to make KRNL286.EXE and DOSX happy
        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   dither_init
;
;   init local frame vars for DitherDIB
;
;   ENTRY:
;       AX      -   source bpp
;       BX      -   dest bpp
;	ss:ebp	--> ditherdib frame
;
;   EXIT:
;       FS:ESI  --> source DIB start x,y
;       ES:EDI  --> dest DIB start x,y
;       DS:     --> dither tables (in DGROUP)
;
;--------------------------------------------------------------------------;

dither_init_error:
        stc
        ret

dither_init proc near

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   validate the DIBs
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   eax,ax
        movzx   ebx,bx
        movzx   ecx,cx
        movzx   edx,dx
        movzx   edi,di
        movzx   esi,si

        lfs     si, biSrc
        les     di, biDst

	mov	cx, es:[di].biBitCount	  ; dest must be right
        cmp     cx, bx
	jne	short dither_init_error

	mov	cx, fs:[si].biBitCount	 ; source must be right
        cmp     cx, ax
	jne	short dither_init_error

dither_init_bit_depth_ok:

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Set up the initial source pointer
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        mov     eax,fs:[si].biWidth
        mul     ecx
        add     eax,31
        and     eax,not 31
        shr     eax,3
	mov	SrcWidth,eax
        mov     SrcInc,eax

        lfs     si,lpSrc

	movzx	edx,SrcY
        mul     edx
        add     esi,eax

        movzx   eax,SrcX
        mul     ecx
        shr     eax,3
        add     esi,eax

        movzx   eax, DstXE           ; SrcInc = SrcWidth - DstXE*bits/8
        mul     ecx
        shr     eax, 3
        sub     SrcInc, eax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Set up the initial dest pointer
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        mov     cx, es:[di].biBitCount
        mov     eax,es:[di].biWidth
        mul     ecx
        add     eax,31
        and     eax,not 31
        shr     eax,3
	mov	DstWidth,eax
        mov     DstInc,eax

        cmp     es:[edi].biHeight,0            ; init a upside down DIB
        jge     short @f
        movsx   ebx,DstY
        add     ebx,es:[edi].biHeight
        not     ebx
        mov     DstY,bx
        neg     DstWidth
        neg     DstInc
@@:
        les     di,lpDst

        movsx   edx,DstY
        mul     edx
	add	edi,eax

        movsx   eax,DstX
        mul     ecx
        shr     eax,3
        add     edi,eax

        movzx   eax, DstXE           ; DstInc = DstWidth - DstXE*bits/8
        mul     ecx
        shr     eax, 3
        sub     DstInc, eax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

dither_init_exit:
        clc
        ret

dither_init endp

sEnd    CodeSeg

end

;--------------------------------------------------------------------------;
;
;   GET16  - read a RGB555 from the input
;
;   INPUT:
;       fs:[esi]        rgb555 DIB
;
;   OUTPUT:
;       edx             rgb555
;       esi+=2
;--------------------------------------------------------------------------;

GET16   macro col
if col and 1
        shr     edx,6                       ; get pel from last time
else
        mov     edx, dword ptr fs:[esi]     ; grab two pels
        add     esi,4
endif
        endm

;--------------------------------------------------------------------------;
;
;   DITH16 row, col
;
;       grab a 16 bit pel and dither it.
;
;   Entry:
;       fs:esi  - 16bit pel to dither
;       ds      - data segment
;
;   Returns:
;       al      - dithered pel (rotated into eax)
;
;   Uses:
;       eax, ebx, edx, ebp, esi, edi, flags
;
;   Saves:
;       ecx, edi, es,ds,fs,gs,ss
;
;--------------------------------------------------------------------------;

DITH16  macro row, col, dst

        GET16 col

        mov     bl,dl
        mov     dst,aHalftone5B[ebx + row*256 + col*256*4]

        mov     bl,dh
        add     dst,aHalftone5R[ebx + row*256 + col*256*4]

        and     bl,11b
        or      bl,dl

        add     dst,aHalftone5G[ebx + row*256 + col*256*4]

        endm

;--------------------------------------------------------------------------;
;
;   Dither16()
;
;   Entry:
;       Stack based parameters as described below.
;
;   Returns:
;       none
;
;   Registers Preserved:
;       DS,ES,ESI,EDI,EBP
;
;--------------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing        

Dither16JumpTable label dword
        dd      Dither16Scan0
        dd      Dither16Scan3
        dd      Dither16Scan2
        dd      Dither16Scan1

cProc	Dither16,<FAR,PUBLIC,PASCAL>,<>
        parmD   biDst                   ;--> BITMAPINFO of dest
        parmD   lpDst                   ;--> to destination bits
        parmW   DstX                    ;Destination origin - x coordinate
        parmW   DstY                    ;Destination origin - y coordinate
        parmW   DstXE                   ;x extent of the BLT
        parmW   DstYE                   ;y extent of the BLT
        parmD   biSrc                   ;--> BITMAPINFO of source
        parmD   lpSrc                   ;--> to source bits
        parmW   SrcX                    ;Source origin - x coordinate
        parmW   SrcY                    ;Source origin - y coordinate
        parmD   lpDitherTable           ;not used (for 8->4 bit dither)

        localD  SrcWidth                ;width of source in bytes
        localD  DstWidth                ;width of dest in bytes

        localD  SrcInc
        localD  DstInc
cBegin
        push    esi
        push    edi

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   We only handle (DstXE % 4) == 0 or 3.  If it's == 1 or 2, then we
;   round down, because otherwise we'd have to deal with half of a
;   dither cell on the end. 
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
	inc	DstXE			; Make the == 3 mod 4 case work
        and     DstXE, not 011b
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     eax,16
        mov     ebx,8
        call    dither_init             ; init all the frame variables
        jc      Dither16Exit

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   ecx, DstYE               ; divide by 4
        mov     ebx, ecx

        add     ecx, 3                   ; be sure to round up
        shr     ecx, 2
        jz      Dither16Exit
        mov     DstYE, cx

        movzx   ecx, DstXE               ; divide by 4
        shr     ecx,2
        jz      Dither16Exit
        mov     DstXE,cx

        and     ebx, 011b                ; Get height mod 4
        jmp     Dither16JumpTable[ebx*4]

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        align   4
Dither16OuterLoop:

        movzx   ecx, DstXE
        align   4
Dither16Scan0:
        DITH16  0,0,al
        DITH16  0,1,ah
        shl     eax,16
        DITH16  0,2,al
        DITH16  0,3,ah
        rol     eax,16
	mov	dword ptr es:[edi], eax
	add	edi,4
        dec     ecx
        jnz     Dither16Scan0
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16Scan1:
        DITH16  1,0,al
        DITH16  1,1,ah
        shl     eax,16
        DITH16  1,2,al
        DITH16  1,3,ah
        rol     eax,16
	mov	dword ptr es:[edi], eax
	add	edi,4
        dec     ecx
        jnz     Dither16Scan1
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16Scan2:
        DITH16  2,0,al
        DITH16  2,1,ah
        shl     eax,16
        DITH16  2,2,al
        DITH16  2,3,ah
        rol     eax,16
	mov	dword ptr es:[edi], eax
	add	edi,4
	dec	ecx
	jnz	Dither16Scan2
	add	esi, SrcInc
	add	edi, DstInc

	movzx	ecx, DstXE
	align	4
Dither16Scan3:
        DITH16  3,0,al
        DITH16  3,1,ah
        shl     eax,16
        DITH16  3,2,al
        DITH16  3,3,ah
        rol     eax,16
	mov	dword ptr es:[edi], eax
	add	edi,4
	dec	ecx
	jnz	Dither16Scan3
	add	esi, SrcInc
	add	edi, DstInc

	dec	DstYE
        jnz     Dither16OuterLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Dither16Exit:
        xor     ax,ax
        mov     fs,ax       ; to make KRNL286.EXE and DOSX happy
        pop     edi
        pop     esi
cEnd
