page    ,132
;----------------------------Module-Header------------------------------;
; Module Name: DITHER.ASM
;
; dither a 256 color DIB down to a 16 color dib.
;
;-----------------------------------------------------------------------;
?PLM=1
?WIN=0
	.xlist
        include cmacro32.inc
        include windows.inc
	.list

sBegin  Data

sEnd    Data

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
;   DITHER
;
;   Entry:
;       dl  pixel to dither (0-256)
;       bl  pattern x
;       ah  pattern y
;       fs  --> 8x256x8 dither table.
;
;       HIWORD(eax) = 0
;       HIWORD(ebx) = 0
;
;   Returns:
;       dl  dithered pixel (0-15) (rotated into edx)
;       bl  pattern x (advanced mod 8)
;
;--------------------------------------------------------------------------;
DITH8   macro

        mov     al,dl       ; get pel
        mov     dl,fs:[eax*8+ebx]  ; get dithered version of the pixel.
        ror     edx,8

        inc     bl          ; increment x
        and     bl,07h      ; mod 8

        endm

;--------------------------------------------------------------------------;
;
;   DitherDIB()
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

cProc   Dither8,<FAR,PUBLIC,PASCAL>,<>
        parmD   biDst                   ;--> BITMAPINFO of the dest
        parmD   lpDst                   ;--> to destination bits
        parmW   DstX                    ;Destination origin - x coordinate
        parmW   DstY                    ;Destination origin - y coordinate
        parmW   DstXE                   ;x extent of the BLT
        parmW   DstYE                   ;y extent of the BLT
        parmD   biSrc                   ;--> BITMAPINFO of the source
        parmD   lpSrc                   ;--> to source bits
        parmW   SrcX                    ;Source origin - x coordinate
        parmW   SrcY                    ;Source origin - y coordinate
        parmD   lpDitherTable           ;dither table.

        LocalD  DstWidth
        LocalD  SrcWidth
cBegin
        cld

        push    esi
        push    edi
        push    ds

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   align everything on four pixel boundries, we realy should
;   not do this but should handle the general case instead,
;   but hey we are hackers.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        and     SrcX,  not 011b         ; align by four
        add     DstXE, 3
        and     DstXE, not 011b
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        call    dither_init             ; init all the frame variables

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   time to do the dither.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        mov     ax,DstY
        and     ax,07h
        mov     ah,al           ; ah has the scanline mod 8

        movzx   eax,ax
        movzx   ebx,bx
        movzx   ecx,cx

align 4
DitherOuterLoop:
        movzx   ebx,DstX
        and     ebx,07h
        movzx   ecx,DstXE
        shr     ecx,2

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;   we have a input pixel now look up the dithered version.
;
;   the dither table is a byte array like so.
;
;       lpDitherTable[y % 8][pixel][x % 8]
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
align 4
DitherInnerLoop:
        mov     edx, dword ptr ds:[esi] ; get four input pixel(s)

        DITH8
        DITH8
        DITH8
        DITH8

        mov     dword ptr es:[edi],edx  ; write four output pixel(s)

        add     esi,4
        add     edi,4

        dec     ecx
        jnz     short DitherInnerLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        inc     ah
        and     ah, 07h
        add     esi, SrcWidth
        add     edi, DstWidth

        dec     DstYE
        jnz     short DitherOuterLoop

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
;       ss:bp   --> ditherdib frame
;
;   EXIT:
;       DS:ESI  --> source DIB start x,y
;       ES:EDI  --> dest DIB start x,y
;       FS:EBX  --> dither table
;
;--------------------------------------------------------------------------;

dither_init proc near

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Set up the initial dest pointer
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        lds     si,biDst

        mov     eax,[si].biWidth
        add     eax,3
        and     eax,not 3
        mov     DstWidth,eax

        xor     edi,edi
        les     di,lpDst

        movzx   ebx,DstX
        movzx   edx,DstY
        mul     edx
        add     eax,ebx
        add     edi,eax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Set up the initial source pointer
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        lds     si,biSrc

        mov     eax,[si].biWidth
        add     eax,3
        and     eax,not 3
        mov     SrcWidth,eax

        xor     esi,esi
        lds     si,lpSrc

        movzx   ebx,SrcX
        movzx   edx,SrcY
        mul     edx
        add     eax,ebx
        add     esi,eax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        xor     ebx,ebx
        lfs     bx,lpDitherTable

        movzx   eax,DstXE
        sub     SrcWidth, eax

        movzx   eax,DstXE
        sub     DstWidth, eax

        ret

dither_init endp

sEnd    CodeSeg

end
