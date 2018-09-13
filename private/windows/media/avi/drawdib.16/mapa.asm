        title   mapa.asm
        page    ,132

?PLM=1	    ; PASCAL Calling convention is DEFAULT
?WIN=0      ; Windows calling convention

        .xlist
        include cmacro32.inc
        include windows.inc
        .list

; -------------------------------------------------------
;               DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

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
;   STOSB32 - store a byte, every four times doing a STOSD
;
;--------------------------------------------------------------------------;

STOSB32_N = 0

STOSB32 macro
        ror     eax,8                       ; rotate pixel (al) into eax

        STOSB32_N = STOSB32_N + 1

        if (STOSB32_N mod 4) eq 0
;;          stos dword ptr es:[edi]
            mov     dword ptr es:[edi], eax
            add     edi,4
        endif

        endm

;--------------------------------------------------------------------------;
;
;   LODSB32 - get a byte, every four times doing a LODSD
;
;--------------------------------------------------------------------------;

LODSB32_N = 0

LODSB32 macro
        if (LODSB32_N mod 4) eq 0
;;          lods dword ptr ds:[esi]
            mov     eax,dword ptr ds:[esi]
            add     esi,4
        else
            ror     eax,8
        endif

        LODSB32_N = LODSB32_N + 1

        endm

;--------------------------------------------------------------------------;
;
;   MAP16
;
;--------------------------------------------------------------------------;

MAP16   macro   n

if n and 1
        shr     ebx,16                      ; get pel from last time
else
        mov     ebx, dword ptr ds:[esi]     ; grab two pels
        add     esi,4
endif
        ;
        ;       BX contains 5:5:5 RGB convert it to a 8:8:8 RGB
        ;
        mov     al,bl
        shl     al,3
        STOSB32

        shr     bx,2
        mov     al,bl
        and     al,0F8h
        STOSB32

        mov     al,bh
        shl     al,3
        STOSB32

        endm


;--------------------------------------------------------------------------;
;
;   Map16to24()
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

cProc   Map16to24,<FAR,PUBLIC,PASCAL>,<>
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
        parmD   lpDitherTable           ;not used

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
        and     DstXE, not 011b
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     eax,16                  ; source 16
        mov     ebx,24                  ; dest 24
        call    map_init                ; init all the frame variables
        jc      Map16to24Exit

        movzx   eax, DstXE               ; inner loop expanded by 4
        shr     eax, 2
        jz      Map16to24Exit
        mov     DstXE,ax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
align 4
Outer16to24Loop:
        movzx   ecx,DstXE

align 4
Inner16to24Loop:
        MAP16   0
        MAP16   1
        MAP16   2
        MAP16   3
        dec     ecx
        jnz     Inner16to24Loop

        add     edi, DstInc
        add     esi, SrcInc

        dec     DstYE
        jnz     Outer16to24Loop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Map16to24Exit:
        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   Map32to24()
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

cProc   Map32to24,<FAR,PUBLIC,PASCAL>,<>
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
        parmD   lpDitherTable           ;not used

        localD  SrcWidth                ;width of source in bytes
        localD  DstWidth                ;width of dest in bytes

        localD  SrcInc
        localD  DstInc
	localW	OriginalDstXE
cBegin
        push    esi
        push    edi
        push    ds

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   align everything on four pixel boundries, we realy should
;   not do this but should handle the general case instead,
;   but hey we are hackers.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
	mov	ax, DstXE
	mov	OriginalDstXE, ax
	mov	bx, ax
	add	ax, 011b
        and     ax, not 011b
	mov	DstXE, ax

	and	bx, 011b	
	jz	short @f
	dec	DstYE		; if the width isn't a multiple of 4, special-
				; case the last line.
@@:
	

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     eax,32                  ; source 32
        mov     ebx,24                  ; dest 24
        call    map_init                ; init all the frame variables
        jc      Map32to24Exit

        movzx   eax, DstXE               ; inner loop expanded by 4
        shr     eax, 2
        jz      short Map32to24Exit
        mov     DstXE,ax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
align 4
Outer32to24Loop:
        movzx   ecx,DstXE

align 4
Inner32to24Loop:
        mov     eax,[esi]           ; eax = XRGB
        mov     ebx,[esi+4]         ; ebx = xrgb
        shl     eax,8               ; eax = RGB0
        shrd    eax,ebx,8           ; eax = bRGB
        mov     es:[edi],eax        ; store pels
        shl     ebx,8               ; ebx = rgb0

        mov     eax,[esi+8]         ; eax = XRGB
        shrd    ebx,eax,16          ; ebx = GBrg
        mov     es:[edi+4],ebx      ; store pels
        shl     eax,8               ; eax = RGB0

        mov     ebx,[esi+12]        ; ebx = xrgb
        shrd    eax,ebx,24          ; eax = rgbR
        mov     es:[edi+8],eax      ; store pels

        add     esi,16
        add     edi,12
        dec     ecx
        jnz     Inner32to24Loop

        add     edi, DstInc
        add     esi, SrcInc

        dec     DstYE
        jnz     Outer32to24Loop

; done, but might have to do one more scan line
	mov	ax, OriginalDstXE
	and	ax, 011b
	jz	short Map32to24Exit

        movzx   ecx,OriginalDstXE

align 4
@@:	; one more scan line to do....
        mov     eax,[esi]           ; eax = XRGB
        shl     eax,8               ; eax = RGB0
	mov	es:[edi], ah
	shr	eax,16		    ; eax = 00RG
	mov	es:[edi+1], ax

        add     esi,4
        add     edi,3
        dec     ecx
        jnz     @b

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Map32to24Exit:
        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   map_init
;
;   init local frame vars for mapDIB
;
;   ENTRY:
;       AX      -   source bpp
;       BX      -   dest bpp
;       ss:bp   --> mapdib frame
;
;   EXIT:
;       DS:ESI  --> source DIB start x,y
;       ES:EDI  --> dest DIB start x,y
;
;--------------------------------------------------------------------------;

map_init_error:
        stc
        ret

map_init proc near

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   validate the DIBs
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        xor     edi,edi
	xor	esi,esi

        lds     si, biSrc
        les     di, biDst

        movzx   ecx, es:[di].biBitCount     ; dest must be right
        cmp     cx, bx
        jne     map_init_error

        mov     cx, [si].biBitCount         ; source must be 16
        cmp     cx, ax
        jne     map_init_error

map_init_bit_depth_ok:

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Set up the initial source pointer
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        mov     eax,[si].biWidth
        mul     ecx
        add     eax,31
        and     eax,not 31
        shr     eax,3
	mov	SrcWidth,eax
        mov     SrcInc,eax

        lds     si,lpSrc

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
        movzx   ecx, es:[di].biBitCount
        mov     eax,es:[di].biWidth
        mul     ecx
        add     eax,31
        and     eax,not 31
        shr     eax,3
	mov	DstWidth,eax
        mov     DstInc,eax

        les     di,lpDst

        movzx   edx,DstY
        mul     edx
	add	edi,eax

        movzx   eax,DstX
        mul     ecx
        shr     eax,3
        add     edi,eax

        movzx   eax, DstXE           ; DstInc = DstWidth - DstXE*bits/8
        mul     ecx
        shr     eax, 3
        sub     DstInc, eax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

map_init_exit:
        clc
        ret

map_init endp

;--------------------------------------------------------------------------;
;
;   HugeToFlat
;
;   map a bunch of bitmap bits in "huge" format to "flat" format
;
;   this code only works for bitmaps <= 128k
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

cProc   HugeToFlat,<FAR,PUBLIC,PASCAL>,<>
        parmD   lpBits                  ;--> bits
        parmD   cbBits                  ;count of bits.
        parmD   FillBytes               ;the fill bytes
cBegin
        push    esi
        push    edi
        push    ds

        mov     ax,word ptr lpBits[2]
        mov     ds,ax
        mov     es,ax
        mov     eax,FillBytes
        mov     ecx,cbBits
        add     ecx,eax
        mov     esi,00010000h
        sub     ecx,esi
        mov     edi,esi
        sub     edi,eax

        shr     ecx,2
        rep     movsd

        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   FlatToHuge
;
;   map a bunch of bitmap bits in "flat" format to "huge" format
;
;   this code only works for bitmaps <= 128k
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

cProc   FlatToHuge,<FAR,PUBLIC,PASCAL>,<>
        parmD   lpBits                  ;--> bits
        parmD   cbBits                  ;count of bits.
        parmD   FillBytes               ;the fill bytes
cBegin
        push    esi
        push    edi
        push    ds

        mov     ax,word ptr lpBits[2]
        mov     ds,ax
        mov     es,ax
        mov     eax,FillBytes
        mov     ecx,cbBits
        add     ecx,eax
        mov     edi,ecx
        sub     edi,4
        mov     esi,edi
        sub     esi,FillBytes
        sub     ecx,00010000h

        std
        shr     ecx,2
        rep     movsd
        cld

        pop     ds
        pop     edi
        pop     esi
cEnd

sEnd    CodeSeg

end
