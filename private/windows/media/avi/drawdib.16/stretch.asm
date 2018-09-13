page    ,132
;----------------------------Module-Header------------------------------;
; Module Name: STRETCH32.ASM
;
; StretchBLT for DIBs
;
; NOTES:
;       - Does not handle mirroring in x or y.
;       - Does not handle pixel translation
;       - will not work "in place"
;
;       - This is the 32 bit version (32 bit code seg that is...)
;
;  AUTHOR: ToddLa (Todd Laney) Microsoft
;
;-----------------------------------------------------------------------;
?PLM=1
?WIN=0
	.xlist
        include cmacro32.inc
;       include cmacros.inc
        include windows.inc
	.list

sBegin  Data
sEnd    Data

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
;   DDA type constants returned from stretch_init_dda
;
;--------------------------------------------------------------------------;

STRETCH_1to1    equ     (0*4)       ;   dst = src
STRETCH_1to2    equ     (1*4)       ;   dst = src * 2
STRETCH_1to4    equ     (2*4)       ;   dst = src * 4
STRETCH_1toN    equ     (3*4)       ;   dst > src
STRETCH_2to1    equ     (4*4)       ;   dst = src / 2
STRETCH_4to1    equ     (5*4)       ;   dst = src / 4
STRETCH_Nto1    equ     (6*4)       ;   dst < src
STRETCH_VOID    equ     (7*4)       ;   not used.

;--------------------------------------------------------------------------;
;
; x_stretch_functions
;
;     functions for stretching a single scanline, this table is indexed
;     by the x dda type (see above) and the bit depth
;
;--------------------------------------------------------------------------;

x_stretch_functions label dword             ;function table for x_stretch

x_stretch_8_functions label dword           ;function table for x_stretch
        dd      x_stretch_1to1              ; STRETCH_1to1
        dd      x_stretch_8_1to2            ; STRETCH_1to2
        dd      x_stretch_8_1to4            ; STRETCH_1to4
        dd      x_stretch_8_1toN            ; STRETCH_1toN
        dd      x_stretch_8_Nto1            ; STRETCH_2to1
        dd      x_stretch_8_Nto1            ; STRETCH_4to1
        dd      x_stretch_8_Nto1            ; STRETCH_Nto1
        dd      0

x_stretch_16_functions label dword          ;function table for x_stretch
        dd      x_stretch_1to1              ; STRETCH_1to1
        dd      x_stretch_16_1to2           ; STRETCH_1to2
        dd      x_stretch_16_1toN           ; STRETCH_1to4
        dd      x_stretch_16_1toN           ; STRETCH_1toN
        dd      x_stretch_16_Nto1           ; STRETCH_2to1
        dd      x_stretch_16_Nto1           ; STRETCH_4to1
        dd      x_stretch_16_Nto1           ; STRETCH_Nto1
        dd      0

x_stretch_24_functions label dword          ;function table for x_stretch
        dd      x_stretch_1to1              ; STRETCH_1to1
        dd      x_stretch_24_1toN           ; STRETCH_1to2
        dd      x_stretch_24_1toN           ; STRETCH_1to4
        dd      x_stretch_24_1toN           ; STRETCH_1toN
        dd      x_stretch_24_Nto1           ; STRETCH_2to1
        dd      x_stretch_24_Nto1           ; STRETCH_4to1
        dd      x_stretch_24_Nto1           ; STRETCH_Nto1
        dd      0

x_stretch_32_functions label dword          ;function table for x_stretch
        dd      x_stretch_1to1              ; STRETCH_1to1
        dd      x_stretch_32_1to2           ; STRETCH_1to2
        dd      x_stretch_32_1toN           ; STRETCH_1to4
        dd      x_stretch_32_1toN           ; STRETCH_1toN
        dd      x_stretch_32_Nto1           ; STRETCH_2to1
        dd      x_stretch_32_Nto1           ; STRETCH_4to1
        dd      x_stretch_32_Nto1           ; STRETCH_Nto1
        dd      0

;--------------------------------------------------------------------------;
;
; y_stretch_functions
;
;     functions for stretching in the y direction, indexed by the y dda type
;
;--------------------------------------------------------------------------;

y_stretch_functions label dword             ;function table for y_stretch
        dd      y_stretch_1toN              ; STRETCH_1to1
        dd      y_stretch_1toN              ; STRETCH_1to2
        dd      y_stretch_1toN              ; STRETCH_1to4
        dd      y_stretch_1toN              ; STRETCH_1toN
        dd      y_stretch_Nto1              ; STRETCH_2to1
        dd      y_stretch_Nto1              ; STRETCH_4to1
        dd      y_stretch_Nto1              ; STRETCH_Nto1

;--------------------------------------------------------------------------;
;
; stretch_functions
;
;   special case stretching routines, used if (y dda type) = (x dda type)
;   (and entry exists in this table)
;
;   these functions stretch the entire image.
;
;--------------------------------------------------------------------------;

stretch_functions label dword

stretch_8_functions label dword
        dd      0                           ; STRETCH_1to1
        dd      stretch_8_1to2              ; STRETCH_1to2
        dd      0                           ; STRETCH_1to4
        dd      0                           ; STRETCH_1toN
        dd      0                           ; STRETCH_2to1
        dd      0                           ; STRETCH_4to1
        dd      0                           ; STRETCH_Nto1
        dd      0

stretch_16_functions label dword
        dd      0                           ; STRETCH_1to1
        dd      stretch_16_1to2             ; STRETCH_1to2
        dd      0                           ; STRETCH_1to4
        dd      0                           ; STRETCH_1toN
        dd      0                           ; STRETCH_2to1
        dd      0                           ; STRETCH_4to1
        dd      0                           ; STRETCH_Nto1
        dd      0

stretch_24_functions label dword
        dd      0                           ; STRETCH_1to1
        dd      0                           ; STRETCH_1to2
        dd      0                           ; STRETCH_1to4
        dd      0                           ; STRETCH_1toN
        dd      0                           ; STRETCH_2to1
        dd      0                           ; STRETCH_4to1
        dd      0                           ; STRETCH_Nto1
        dd      0

stretch_32_functions label dword
        dd      0                           ; STRETCH_1to1
        dd      0                           ; STRETCH_1to2
        dd      0                           ; STRETCH_1to4
        dd      0                           ; STRETCH_1toN
        dd      0                           ; STRETCH_2to1
        dd      0                           ; STRETCH_4to1
        dd      0                           ; STRETCH_Nto1
        dd      0

;--------------------------------------------------------------------------;
;
;   StretchDIB()
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

cProc   StretchDIB,<FAR,PUBLIC,PASCAL>,<>
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
        parmW   SrcXE                   ;x extent of the BLT
        parmW   SrcYE                   ;y extent of the BLT

        localD  x_stretch               ;X stretch function
        localD  y_stretch               ;Y stretch function

        localD  x_stretch_dda           ;X stretch DDA
        localD  x_stretch_dda_fract     ;X stretch DDA fract

        localD  y_stretch_dda           ;Y stretch DDA
        localD  y_stretch_dda_fract     ;Y stretch DDA fract

        localD  ScanCount               ;number of scans left to do
        localD  Yerr                    ;Y dda error

        localD  SrcWidth                ;width of source dib in bytes
        localD  DstWidth                ;width of dest in bytes

        localD  SrcBytes                ;width of source blt in bytes
        localD  DstBytes                ;width of dest blt in bytes

        localD  SrcInc
        localD  DstInc

        localD  lDstXE                  ;x extent of the BLT
        localD  lDstYE                  ;y extent of the BLT
        localD  lSrcXE                  ;x extent of the BLT
        localD  lSrcYE                  ;y extent of the BLT
cBegin
        cld

        push    esi
        push    edi
        push    ds

	call	stretch_init	; init all the frame variables
        jc      short StretchDIBExit

        call    y_stretch       ; do it!

StretchDIBExit:
        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   stretch_init_dda
;
;       initialize the parameters of a DDA from ax to dx.
;
;   Entry:
;       eax         - source coord
;       edx         - dest coord
;   Returns:
;       eax         - STRETCH_1to1
;                     STRETCH_1to2
;                     STRETCH_1to4
;                     STRETCH_1toN
;                     STRETCH_2to1
;                     STRETCH_4to1
;                     STRETCH_Nto1
;
;       edx         - src / dst
;       ebx         - src / dst fraction
;
;--------------------------------------------------------------------------;

stretch_init_dda proc near

        cmp     eax,edx
        je      short stretch_init_dda_1to1
        ja      short stretch_init_dda_Nto1
        errn$   stretch_init_dda_1toN

stretch_init_dda_1toN:
        mov     ebx,eax
        add     ebx,ebx
        cmp     ebx,edx
        je      short stretch_init_dda_1to2
        add     ebx,ebx
        cmp     ebx,edx
        je      short stretch_init_dda_1to4

        mov     ebx,edx             ; ebx = dest
        mov     edx,eax             ; edx = src
        xor     eax,eax             ; edx:eax = src<<32
        div     ebx                 ; eax = (src<<32)/dst

        mov     ebx,eax
        xor     edx,edx
        mov     eax,STRETCH_1toN
        ret

stretch_init_dda_Nto1:
        mov     ebx,edx
        add     ebx,ebx
        cmp     eax,ebx
        je      short stretch_init_dda_2to1
        add     ebx,ebx
        cmp     eax,ebx
        je      short stretch_init_dda_4to1

        mov     ebx,edx             ; ebx = dst
        xor     edx,edx             ; edx:eax = src
        mov     eax,eax
        div     ebx                 ; eax = src/dst edx = rem
        push    eax
        xor     eax,eax             ; edx:eax = rem<<32
        div     ebx                 ; eax = rem<<32/dst
        mov     ebx,eax
        pop     edx
        mov     eax,STRETCH_Nto1
        ret

stretch_init_dda_1to1:
        mov     edx, 1
        xor     ebx, ebx
        mov     eax, STRETCH_1to1
        ret

stretch_init_dda_1to2:
        xor     edx, edx
        mov     ebx, 80000000h
        mov     eax, STRETCH_1to2
        ret

stretch_init_dda_1to4:
        xor     edx, edx
        mov     ebx, 40000000h
        mov     eax, STRETCH_1to4
        ret

stretch_init_dda_2to1:
        mov     edx, 2
        xor     ebx, ebx
        mov     eax, STRETCH_2to1
        ret

stretch_init_dda_4to1:
        mov     edx, 4
        xor     ebx, ebx
        mov     eax, STRETCH_4to1
        ret

stretch_init_dda endp

;--------------------------------------------------------------------------;
;
;   stretch_init
;
;   init local frame vars for StretchDIB
;
;   ENTRY:
;       ss:bp   --> stretchdib frame
;
;--------------------------------------------------------------------------;

stretch_init_error:
        stc
        ret

stretch_init proc near

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   expand word params to dwords
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   eax,DstXE
        movzx   ebx,DstYE
        movzx   ecx,SrcXE
        movzx   edx,SrcYE

        mov     lDstXE,eax
        mov     lDstYE,ebx
        mov     lSrcXE,ecx
        mov     lSrcYE,edx

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; Make sure they didn't give us an extent of zero anywhere
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        or      eax,eax
        jz      short stretch_init_error

        or      ebx,ebx
        jz      short stretch_init_error

        or      ecx,ecx
        jz      short stretch_init_error

        or      edx,edx
        jz      short stretch_init_error

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   make sure the bit depth of the source and dest match and are valid
;   we only handle 8,16,24 bit depths.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        xor     edi,edi                     ; be sure the high words are clear.
	xor	esi,esi
        xor     ecx,ecx

        lds     si, biSrc
        les     di, biDst

        mov     cx, [esi].biBitCount        ; get the bit depth
        cmp     cx, es:[edi].biBitCount     ; make sure they are the same.
        jne     short stretch_init_error

        cmp     ecx,8
        je      short stretch_init_bit_depth_ok

        cmp     ecx,16
        je      short stretch_init_bit_depth_ok

        cmp     ecx,24
        je      short stretch_init_bit_depth_ok

        cmp     ecx,32
        jne     short stretch_init_error
        errn$   stretch_init_bit_depth_ok

stretch_init_bit_depth_ok:

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Set up the initial source pointer
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        mov     eax,[esi].biWidth
        mul     ecx
        add     eax,31
        and     eax,not 31
        shr     eax,3
        mov     SrcWidth,eax
        mov     SrcInc,eax

        lds     si,lpSrc

        movzx   edx,SrcY
        mul     edx
	add	esi,eax

        movzx   eax,SrcX
        mul     ecx
        shr     eax,3
        add     esi,eax

        mov     eax,lSrcXE
        mul     ecx
        shr     eax,3
        mov     SrcBytes,eax
        sub     SrcInc,eax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Set up the initial dest pointer
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        mov     eax,es:[edi].biWidth
        mul     ecx
        add     eax,31
        and     eax,not 31
        shr     eax,3
        mov     DstWidth,eax
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

        mov     eax,lDstXE
        mul     ecx
        shr     eax,3
        mov     DstBytes,eax
        sub     DstInc,eax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  conver the bit depth (in cx) to a table index
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        sub     ecx,8                   ; cx = 0,8,16,24
        shl     ecx,2                   ; cx = (0,1,2,3) * 32

        errnz   <stretch_16_functions - stretch_8_functions - 32>
        errnz   <stretch_24_functions - stretch_16_functions - 32>
        errnz   <stretch_32_functions - stretch_24_functions - 32>

        errnz   <x_stretch_16_functions - x_stretch_8_functions - 32>
        errnz   <x_stretch_24_functions - x_stretch_16_functions - 32>
        errnz   <x_stretch_32_functions - x_stretch_24_functions - 32>

        errnz   <STRETCH_1to2 - STRETCH_1to1 - 4>

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Setup y_stretch function pointer
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        mov     eax,lSrcYE
        mov     edx,lDstYE
        call    stretch_init_dda
        mov     y_stretch_dda,edx
        mov     y_stretch_dda_fract,ebx

        mov     ebx,eax
        mov     edx,y_stretch_functions[ebx]
        mov     y_stretch,edx

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Setup x_stretch function pointer
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        push    eax                     ; Save Y stretch type
        mov     eax,lSrcXE
        mov     edx,lDstXE

        call    stretch_init_dda
        mov     x_stretch_dda,edx
        mov     x_stretch_dda_fract,ebx

        mov     ebx,eax                 ; get x stretch
        or      ebx,ecx                 ; or in bit depth
        mov     edx,x_stretch_functions[ebx]
        mov     x_stretch,edx

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  check for a special case stretch routine
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        pop     edx                     ; get y stretch back
        cmp     eax,edx                 ; is x stretch == to y stretch?
        jne     short stretch_init_exit

        or      ebx,ecx
        mov     edx,stretch_functions[ebx]
        or      edx,edx
        jz      short stretch_init_exit

        mov     y_stretch,edx           ; we have special case routine.
        errn$   stretch_init_exit

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

stretch_init_exit:
        clc
        ret

stretch_init endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;--------------------------------------------------------------------------;
;
;   y_stretch_1toN
;
;   do the entire stretch, y stretching  (DstYE > SrcYE)
;
;   Entry:
;       ds:esi  --> (SrcX, SrcY) in source
;       es:edi  --> (DstX, DstY) in destination
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       none. (stretch is done.)
;--------------------------------------------------------------------------;
y_stretch_1toN proc near
        mov     ecx,lDstYE
        mov     ScanCount,ecx

        mov     edx,lDstYE          ; dda error
        dec     edx
        mov     Yerr,edx

y_stretch_1toN_loop:
        push    esi
        push    edi
        call    x_stretch
        pop     edi
        pop     esi

        add     edi, DstWidth       ; next dest scan.

        mov     eax,lSrcYE
        sub     Yerr,eax
        jnc     short y_stretch_1toN_next

        mov     eax,lDstYE
        add     Yerr,eax

        add     esi, SrcWidth       ; next source scan.

y_stretch_1toN_next:
        dec     ScanCount
        jnz     short y_stretch_1toN_loop

        ret

y_stretch_1toN endp

;--------------------------------------------------------------------------;
;
;   y_stretch_Nto1
;
;   do the entire stretch, y shrinking   (DstYE < SrcYE)
;
;   Entry:
;       ds:esi  --> (SrcX, SrcY) in source
;       es:edi  --> (DstX, DstY) in destination
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       none. (stretch is done.)
;--------------------------------------------------------------------------;
y_stretch_Nto1 proc near
        mov     ecx,lDstYE
        mov     ScanCount,ecx

        mov     edx,lSrcYE          ; dda error
        dec     edx
        mov     Yerr,edx

y_stretch_Nto1_loop:
        push    esi
        push    edi
        call    x_stretch
        pop     edi
        pop     esi

        add     edi, DstWidth       ; next dest scan.

        mov     eax, lDstYE
@@:     add     esi, SrcWidth       ; next source scan.
        sub     Yerr, eax
        jnc     short @b

        mov     eax,lSrcYE
        add     Yerr,eax

y_stretch_Nto1_next:
        dec     ScanCount
        jnz     short y_stretch_Nto1_loop

        ret

y_stretch_Nto1 endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;--------------------------------------------------------------------------;
;
;   x_stretch_1to1
;
;   handle a stretch of a scanline  (DstXE == SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_1to1 proc near

        mov     ecx,DstBytes        ; number of bytes to copy
        mov     ebx,ecx

        shr     ecx,2               ; get count in DWORDs
        rep     movs dword ptr es:[edi], dword ptr ds:[esi]
        mov     ecx,ebx
        and     ecx,3
	rep	movs byte ptr es:[edi], byte ptr ds:[esi]
        ret

x_stretch_1to1 endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;8 BIT;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;--------------------------------------------------------------------------;
;
;   x_stretch_8_1toN
;
;   handle a stretch of a scanline  (DstXE > SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_8_1toN proc near

        mov     ebx,x_stretch_dda_fract
        xor     edx,edx
        mov     ecx,lDstXE          ; # bytes to store
        shr     ecx,2               ; unroll by 4
        jz      short x_stretch_8_1toN_next
align 4
x_stretch_8_1toN_loop:
rept 2
        mov     al,byte ptr ds:[esi]
        add     edx,ebx
        adc     esi,0
        mov     ah,byte ptr ds:[esi]
        add     edx,ebx
        adc     esi,0
        ror     eax,16
endm
        mov     dword ptr es:[edi],eax
        add     edi,4
        dec     ecx
        jnz     short x_stretch_8_1toN_loop

x_stretch_8_1toN_next:
        mov     ecx,lDstXE
        and     ecx,3
        jnz     short x_stretch_8_1toN_odd
        ret

x_stretch_8_1toN_odd:
        mov     al,byte ptr ds:[esi]
        mov     byte ptr es:[edi],al
        add     edx,ebx
        adc     esi,0
        inc     edi
        dec     ecx
        jnz     short x_stretch_8_1toN_odd
        ret

x_stretch_8_1toN endp

;--------------------------------------------------------------------------;
;
;   x_stretch_8_1to2
;
;   handle a stretch of a scanline  (DstXE > SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_8_1to2 proc near
        mov     ecx,lSrcXE          ; loop cout
        mov     ebx,ecx
        shr     ecx,1
        jz      short x_stretch_8_1to2_next
align 4
x_stretch_8_1to2_loop:
        mov     ax, word ptr ds:[esi]   ; get 2 pixels
        add     esi,2

        mov     edx,eax
        mov     al,ah
        shl     eax,16
        mov     al,dl
        mov     ah,dl
        mov     dword ptr es:[edi], eax ; store 4
        add     edi,4
        dec     ecx
        jnz     short x_stretch_8_1to2_loop

x_stretch_8_1to2_next:
        test    ebx,1
        jnz     short x_stretch_8_1to2_done
        ret

x_stretch_8_1to2_done:
        mov     al,byte ptr ds:[esi]
        mov     ah,al
        mov     word ptr es:[edi],ax
        inc     esi
        add     edi,2
        ret

x_stretch_8_1to2 endp

;--------------------------------------------------------------------------;
;
;   x_stretch_8_1to4
;
;   handle a stretch of a scanline  (DstXE > SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;
;--------------------------------------------------------------------------;
align 4
x_stretch_8_1to4 proc near
        mov     ecx,lSrcXE           ; loop cout
        mov     ebx,ecx
        shr     ecx,1
        jz      short x_stretch_8_1to4_next
align 4
x_stretch_8_1to4_loop:
;       lods    word ptr ds:[esi]   ; get 2 pixels
        mov     ax,word ptr ds:[esi]   ; get 2 pixels
        add     esi,2

        mov     edx,eax
        mov     ah,al
        shl     eax,16
        mov     al,dl
        mov     ah,dl
;       stos    dword ptr es:[edi]  ; store 4
        mov     dword ptr es:[edi],eax
        add     edi,4

        mov     ax,dx
        mov     al,ah
        shl     eax,16
        mov     ax,dx
        mov     al,ah
;       stos    dword ptr es:[edi]  ; store 4
        mov     dword ptr es:[edi],eax  ; store 4
        add     edi,4

        dec     ecx
        jnz     short x_stretch_8_1to4_loop

x_stretch_8_1to4_next:
        test    ebx,1
        jnz     short x_stretch_8_1to4_done
        ret

x_stretch_8_1to4_done:
        mov     al,byte ptr ds:[esi]
        mov     ah,al
        mov     dx,ax
        shl     eax,16
        mov     ax,dx
        mov     dword ptr es:[edi],eax
        inc     esi
        add     edi,4
        ret

x_stretch_8_1to4 endp

;--------------------------------------------------------------------------;
;
;   x_stretch_8_Nto1
;
;   handle a shrink of a scanline  (DstXE < SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ecx     -   destination pixels to write
;       edx     -   source pixels to copy
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_8_Nto1 proc near
        mov     ebx,x_stretch_dda_fract
        xor     edx,edx

        mov     ecx,lDstXE                      ; # bytes to store
        shr     ecx,2
        jz      short x_stretch_8_Nto1_cleanup

        push    ebp
        mov     ebp,x_stretch_dda
align 4
x_stretch_8_Nto1_loop:
rept    4
        mov     al,byte ptr ds:[esi]
        ror     eax,8
        add     edx,ebx
        adc     esi,ebp
endm
        mov     dword ptr es:[edi],eax
        add     edi,4
        dec     ecx
        jnz     short x_stretch_8_Nto1_loop
        pop     ebp

x_stretch_8_Nto1_cleanup:
        mov     ecx,lDstXE
        and     ecx,011b
        jnz     short x_stretch_8_Nto1_loop2
        ret

x_stretch_8_Nto1_loop2:
        mov     al,byte ptr ds:[esi]
        mov     byte ptr es:[edi],al
        inc     edi
        add     edx,ebx
        adc     esi,x_stretch_dda
        dec     ecx
        jnz     short x_stretch_8_Nto1_loop2
        ret

x_stretch_8_Nto1 endp

;--------------------------------------------------------------------------;
;
;   stretch_8_1to2
;
;   handle a x2 stretch of a entire image
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
stretch_8_1to2 proc near
        mov     eax, DstWidth
        add     DstInc, eax

        mov     ecx, lSrcXE
        mov     ebx, DstWidth
align 4
stretch_8_1to2_outer_loop:
        mov     ecx,lSrcXE          ; loop cout (/4)
        shr     ecx,2
        jz      short stretch_8_1to2_next
align 4
stretch_8_1to2_loop:
        mov     edx, dword ptr ds:[esi]   ; get 4 pixels
        mov     al,dh
        mov     ah,dh
        shl     eax,16
        mov     al,dl
        mov     ah,dl
        mov     dword ptr es:[edi], eax     ; store 4
        mov     dword ptr es:[edi+ebx], eax ; store 4

        shr     edx,16

        mov     al,dh
        mov     ah,dh
        shl     eax,16
        mov     al,dl
        mov     ah,dl
        mov     dword ptr es:[edi+4], eax     ; store 4
        mov     dword ptr es:[edi+4+ebx], eax ; store 4

        add     edi,8
        add     esi,4

        dec     ecx
        jnz     short stretch_8_1to2_loop

stretch_8_1to2_next:
        mov     ecx,lSrcXE
        and     ecx,3
        jnz     short stretch_8_1to2_odd

stretch_8_1to2_even:
        add     edi, DstInc
        add     esi, SrcInc

        dec     lSrcYE
        jnz     short stretch_8_1to2_outer_loop
        ret

stretch_8_1to2_odd:
        mov     al,byte ptr ds:[esi]
        mov     ah,al
        mov     word ptr es:[edi],ax
        mov     word ptr es:[edi+ebx],ax
        inc     esi
        add     edi,2
        dec     ecx
        jnz     short stretch_8_1to2_odd
        jz      short stretch_8_1to2_even

stretch_8_1to2 endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;16 BIT;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;--------------------------------------------------------------------------;
;
;   x_stretch_16_1toN
;
;   handle a stretch of a scanline  (DstXE > SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_16_1toN proc near
        xor     edx,edx

        mov     ecx,lDstXE
        shr     ecx,1
        jz      short x_stretch_16_1toN_cleanup

align 4
x_stretch_16_1toN_loop:
rept 2
        mov     ax,word ptr ds:[esi]
        ror     eax,16

        add     edx, x_stretch_dda_fract
        sbb     ebx, ebx                ; ebx = CF ? -1 : 0
        and     ebx, 2
        add     esi, ebx
endm
        mov     dword ptr es:[edi],eax
        add     edi,4

        dec     ecx
        jnz     short x_stretch_16_1toN_loop

x_stretch_16_1toN_cleanup:
        test    byte ptr DstXE, 1
        jnz     short x_stretch_16_1toN_odd
        ret

x_stretch_16_1toN_odd:
        mov     ax,word ptr ds:[esi]
        mov     word ptr es:[edi],ax
        add     esi,2
        add     edi,2
        ret

x_stretch_16_1toN endp

;--------------------------------------------------------------------------;
;
;   x_stretch_16_1to2
;
;   handle a stretch of a scanline  (DstXE > SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_16_1to2 proc near
        mov     ecx,lSrcXE          ; loop cout
        mov     ebx,ecx
        shr     ecx,1
        jz      short x_stretch_16_1to2_done
align 4
x_stretch_16_1to2_loop:
	mov	eax, dword ptr ds:[esi] ; get 2 pixels
	add	esi,4

	mov	edx,eax
        shl     eax,16
        mov     ax,dx
	mov	dword ptr es:[edi], eax ; store 2
	add	edi,4

	mov	eax,edx
	shr	edx,16
        mov     ax,dx
	mov	dword ptr es:[edi], eax ; store 2
	add	edi,4

        dec     ecx
        jnz     short x_stretch_16_1to2_loop

x_stretch_16_1to2_done:
        test    ebx,1
        jnz     short x_stretch_16_1to2_odd
        ret

x_stretch_16_1to2_odd:
        mov     bx,word ptr ds:[esi]
        mov     eax,ebx
        shl     eax,16
        mov     ax,bx
        mov     dword ptr es:[edi],eax
        add     esi,2
        add     edi,4
        ret

x_stretch_16_1to2 endp

;--------------------------------------------------------------------------;
;
;   x_stretch_16_Nto1
;
;   handle a shrink of a scanline  (DstXE < SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ecx     -   destination pixels to write
;       edx     -   source pixels to copy
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_16_Nto1 proc near
        mov     ecx,lDstXE            ; # loop count
        mov     ebx,x_stretch_dda_fract
        xor     edx,edx

        push    ebp
        mov     ebp,x_stretch_dda
        dec     ebp
        add     ebp,ebp
align 4
x_stretch_16_Nto1_loop:
        movs    word ptr es:[edi], word ptr ds:[esi]
        add     esi,ebp
        add     edx,ebx
        sbb     eax,eax
        and     eax,2
        add     esi,eax

        dec     ecx
        jnz     short x_stretch_16_Nto1_loop
        pop     ebp

x_stretch_16_Nto1_exit:
        ret

x_stretch_16_Nto1 endp

;--------------------------------------------------------------------------;
;
;   stretch_16_1to2
;
;   handle a x2 stretch of a entire image
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
stretch_16_1to2 proc near
        mov     ebx, DstWidth
        add     DstInc, ebx
align 4
stretch_16_1to2_outer_loop:
        mov     ecx,lSrcXE                 ; loop cout (/2)
        shr     ecx,1
        jz      short stretch_16_1to2_next
align 4
stretch_16_1to2_loop:
        mov     edx, dword ptr ds:[esi]   ; get 2 pixels

        mov     ax,dx
        shl     eax,16
        mov     ax,dx
        mov     dword ptr es:[edi], eax     ; store 2
        mov     dword ptr es:[edi+ebx], eax ; store 2

        shr     edx,16

        mov     ax,dx
        shl     eax,16
        mov     ax,dx
        mov     dword ptr es:[edi+4], eax     ; store 2
        mov     dword ptr es:[edi+4+ebx], eax ; store 2

        add     edi,8
        add     esi,4

        dec     ecx
        jnz     short stretch_16_1to2_loop

stretch_16_1to2_next:
        test    lSrcXE,1
        jnz     short stretch_16_1to2_odd

stretch_16_1to2_even:
        add     edi, DstInc
        add     esi, SrcInc

        dec     SrcYE
        jnz     short stretch_16_1to2_outer_loop
        ret

stretch_16_1to2_odd:
        mov     dx,word ptr ds:[esi]
        mov     ax,dx
        shl     eax,16
        mov     ax,dx
        mov     dword ptr es:[edi],eax
        mov     dword ptr es:[edi+ebx],eax
        add     esi,2
        add     edi,4
        jmp     short stretch_16_1to2_even

stretch_16_1to2 endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;24 BIT;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;--------------------------------------------------------------------------;
;
;   x_stretch_24_1toN
;
;   handle a stretch of a scanline  (DstXE > SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_24_1toN proc near

        mov     ecx,lDstXE            ; # loop count
        mov     ebx,x_stretch_dda_fract
        xor     edx,edx
align 4
x_stretch_24_1toN_loop:
        movs    word ptr es:[edi], word ptr ds:[esi]
        movs    byte ptr es:[edi], byte ptr ds:[esi]
        add     edx,ebx
        sbb     eax,eax
        not     eax
        and     eax,-3
        add     esi,eax

        dec     ecx
        jnz     short x_stretch_24_1toN_loop

x_stretch_24_1toN_exit:
        ret

x_stretch_24_1toN endp

;--------------------------------------------------------------------------;
;
;   x_stretch_24_Nto1
;
;   handle a shrink of a scanline  (DstXE < SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ecx     -   destination pixels to write
;       edx     -   source pixels to copy
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_24_Nto1 proc near
        mov     ecx,lDstXE            ; # loop count
        mov     ebx,x_stretch_dda_fract
        xor     edx,edx

        push    ebp
        mov     ebp,x_stretch_dda
        dec     ebp
        mov     eax,ebp
        add     ebp,ebp
        add     ebp,eax
align 4
x_stretch_24_Nto1_loop:
        movs    word ptr es:[edi], word ptr ds:[esi]
        movs    byte ptr es:[edi], byte ptr ds:[esi]
        add     esi,ebp
        add     edx,ebx
        sbb     eax,eax
        and     eax,3
        add     esi,eax

        dec     ecx
        jnz     short x_stretch_24_Nto1_loop
        pop     ebp

x_stretch_24_Nto1_exit:
        ret

x_stretch_24_Nto1 endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;32 BIT;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;--------------------------------------------------------------------------;
;
;   x_stretch_32_1toN
;
;   handle a stretch of a scanline  (DstXE > SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_32_1toN proc near
        xor     edx,edx

        mov     ecx,lDstXE
align 4
x_stretch_32_1toN_loop:
        mov     eax,word ptr ds:[esi]
        add     edx, x_stretch_dda_fract
        sbb     ebx, ebx                ; ebx = CF ? -1 : 0
        and     ebx, 4
        add     esi, ebx
        mov     dword ptr es:[edi],eax
        add     edi,4

        dec     ecx
        jnz     short x_stretch_32_1toN_loop

        ret

x_stretch_32_1toN endp

;--------------------------------------------------------------------------;
;
;   x_stretch_32_1to2
;
;   handle a stretch of a scanline  (DstXE > SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_32_1to2 proc near
        mov     ecx,lSrcXE          ; loop cout
align 4
x_stretch_32_1to2_loop:
        mov     eax, dword ptr ds:[esi]     ; get a pel
        mov     dword ptr es:[edi], eax     ; store it
        add     esi,4
        mov     dword ptr es:[edi+4], eax   ; store it again
        add     edi,8

        dec     ecx
        jnz     short x_stretch_32_1to2_loop

x_stretch_32_1to2_done:
        ret

x_stretch_32_1to2 endp

;--------------------------------------------------------------------------;
;
;   x_stretch_32_Nto1
;
;   handle a shrink of a scanline  (DstXE < SrcXE)
;
;   Entry:
;       ds:esi  --> begining of scan
;       es:edi  --> destination scan
;       ecx     -   destination pixels to write
;       edx     -   source pixels to copy
;       ss:bp   --> stack frame of StretchDIB
;   Returns:
;       ds:esi  --> at end of scan
;       es:edi  --> at end of scan
;   
;--------------------------------------------------------------------------;
align 4
x_stretch_32_Nto1 proc near
        mov     ecx,lDstXE            ; # loop count
        mov     ebx,x_stretch_dda_fract
        xor     edx,edx

        push    ebp
        mov     ebp,x_stretch_dda
        dec     ebp
        add     ebp,ebp
        add     ebp,ebp
align 4
x_stretch_32_Nto1_loop:
        movs    dword ptr es:[edi], dword ptr ds:[esi]
        add     esi,ebp
        add     edx,ebx
        sbb     eax,eax
        and     eax,4
        add     esi,eax

        dec     ecx
        jnz     short x_stretch_32_Nto1_loop
        pop     ebp

x_stretch_32_Nto1_exit:
        ret

x_stretch_32_Nto1 endp

sEnd    CodeSeg

end
