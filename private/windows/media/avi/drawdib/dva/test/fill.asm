        .xlist
        include cmacros.inc
        .list

sBegin  Code
        .386
        assumes cs,Code
        assumes ds,nothing
        assumes es,nothing

;--------------------------Private-Routine-------------------------------;
; rect
;
; draws a solid rect
;
; Entry:
;       lpBits          bits pointer
;       width_bytes     width in bytes to next scan
;       lpPoints        points to draw.
; Return:
;       none
; Error Returns:
;       none
; Registers Preserved:
;       none
; Registers Destroyed:
;       AX,BX,CX,DX,DS,ES,SI,DI,FLAGS
; Calls:
;       non
; History:
;       Mon 26-Mar-1990 -by-  Todd Laney [ToddLa]
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   DIBRect,<FAR,PUBLIC>,<ds>
        parmD   lpBits
        parmD   WidthBytes
        parmW   RectX
        parmW   RectY
        parmW   RectDX
        parmW   RectDY
        parmW   color
cBegin
        push    edi

        xor     edi,edi
        les     di,lpBits               ; es:edi --> bits
        add     di,RectX
        movzx   eax,RectY               ; get Y value of first scan
        imul    WidthBytes              ; eax - offset to scan
        add     edi,eax                 ; es:edi --> scan

        xor     ecx,ecx
        mov     dx,RectDY
        or      dx,dx
        jz      DIBRectExit

        mov     cx,RectDX
        and     cx,not 3
        or      cx,cx
        jz      DIBRectExit
        sub     WidthBytes,ecx

        mov     al,byte ptr color

;       test    di,011b
;       jnz     short DIBRectLoopOdd

        mov     bx,RectDX
;       test    bx,011b
;       jnz     short DIBRectLoopOdd

        shr     bx,2
        mov     ah,al
        mov     cx,ax
        shl     eax,16
        mov     ax,cx
DIBRectLoop:
        mov     cx,bx
        rep     stos dword ptr es:[edi]
        add     edi,WidthBytes
        dec     dx
        jnz     short DIBRectLoop
        jz      short DIBRectExit

DIBRectLoopOdd:
        mov     cx,RectDX
        rep stos byte ptr es:[edi]
        add     edi,WidthBytes
        dec     dx
        jnz     short DIBRectLoopOdd

DIBRectExit:
        pop    edi
cEnd

sEnd
end
