page    ,132
;----------------------------Module-Header------------------------------;
; Module Name: DITH775.ASM
;
; code to dither 16 or 24 bit DIBs to a 8bit DIB with a fixed palette
;
; NOTES:
;       this is a ASM version of the code found in dith775.c
;
;-----------------------------------------------------------------------;
?PLM=1
?WIN=0
	.xlist
        include cmacro32.inc
        include windows.inc
        .list

        externA __AHINCR
        externA __AHSHIFT

sBegin  Data
        externB _lookup775      ; in look775.h
        externB _rdith775       ; in dtab775.h
        externB _gdith775       ; in dtab775.h
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
;   GET16  - read a RGB555 from the input
;
;   INPUT:
;       fs:[esi]        rgb555 DIB
;
;   OUTPUT:
;       bx              rgb555
;
;--------------------------------------------------------------------------;

GET16   macro col
if col and 1
        shr     ebx,16                      ; get pel from last time
else
        mov     ebx, dword ptr fs:[esi]     ; grab two pels
        add     esi,4
endif
        endm

;--------------------------------------------------------------------------;
;
;   DITH m1, m2, row, col
;
;       grab a 16 bit pel and dither it.
;
;       m1 and m2 are the magic dither table offsets.
;
;       here is the 'C' code we are emulating
;
;           w = *((WORD huge *)pbS)++;
;           r = (int)((w >> 7) & 0xF8);
;           g = (int)((w >> 2) & 0xF8);
;           b = (int)((w << 3) & 0xF8);
;
;           *pbD++ = (BYTE)lookup775[ rdith775[r + m1] + gdith775[g + m1] + ((b +  m2) >> 6) ];
;
;       the 'magic' values vary over a 4x4 block
;
;       m1:  1  17  25  41      m2:  2  26  38  62
;           31  36   7  12          46  54  19  18
;           20   4  39  23          30   6  58  34
;           33  28  15   9          50  42  22  14
;
;
;       for a 2x2 dither use the following numbers:
;
;       m1:  5  27  m2:  8  40
;           38  16      56  24
;
;       m1:  1  41  m2:  2  62
;           33   9      50  14
;
;   NOTE:
;       !!! the lookup tables should be BYTEs not INTs !!!
;
;   Entry:
;       m1, m2  - magic values
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

DITH16  macro m1, m2, row, col

        GET16 col

        mov     al, bl                      ; get blue
        and     al, 1Fh
        add     al, (m2 shr 3)
        shr     al, 3

        shr     bx, 2                       ; get green
        mov     dl, bl
        and     dx, 0F8h

        shr     bx, 5                       ; get red
        and     bx, 0F8h

        add     al, _rdith775[bx + m1]
        mov     bl, dl
        add     al, _gdith775[bx + m1]
        mov     bl, al

if col and 1
        mov     al, byte ptr _lookup775[bx]
        xchg    al, ah
        ror     eax,16                      ; rotate pixel into eax
else
        mov     ah, byte ptr _lookup775[bx]
endif
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

Dither16TJumpTable label dword
        dd      Dither16Scan0
        dd      Dither16Scan3
        dd      Dither16Scan2
        dd      Dither16Scan1

cProc   Dither16T,<FAR,PUBLIC,PASCAL>,<>
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
;       push    ds

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
        jmp     Dither16TJumpTable[ebx*4]

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        align   4
Dither16OuterLoop:

        movzx   ecx, DstXE
        align   4
Dither16Scan0:
        DITH16   1  2 0 0                  ;        DITH16   1  2 0 0
        DITH16  17 26 0 1                  ;        DITH16  41 62 0 1
        DITH16  25 38 0 2                  ;        DITH16   1  2 0 2
        DITH16  41 62 0 3                  ;        DITH16  41 62 0 3
        stos    dword ptr es:[edi]
        dec     ecx
        jnz     Dither16Scan0
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16Scan1:
        DITH16  31 46 1 0                  ;        DITH16  33 50 1 0
        DITH16  36 54 1 1                  ;        DITH16   9 14 1 1
        DITH16   7 19 1 2                  ;        DITH16  33 50 1 2
        DITH16  12 18 1 3                  ;        DITH16   9 14 1 3
        stos    dword ptr es:[edi]
        dec     ecx
        jnz     Dither16Scan1
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16Scan2:
        DITH16  20 30 2 0                  ;        DITH16   1  2 2 0
        DITH16   4  6 2 1                  ;        DITH16  41 62 2 1
        DITH16  39 58 2 2                  ;        DITH16   1  2 2 2
        DITH16  23 34 2 3                  ;        DITH16  41 62 2 3
        stos    dword ptr es:[edi]
        dec     ecx
        jnz     Dither16Scan2
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16Scan3:
        DITH16  33 50 3 0                  ;        DITH16  33 50 3 0
        DITH16  28 42 3 1                  ;        DITH16   9 14 3 1
        DITH16  15 22 3 2                  ;        DITH16  33 50 3 2
        DITH16   9 14 3 3                  ;        DITH16   9 14 3 3
        stos    dword ptr es:[edi]
        dec     ecx
        jnz     Dither16Scan3
        add     esi, SrcInc
        add     edi, DstInc

        dec     DstYE
        jnz     Dither16OuterLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Dither16Exit:
        xor     ax,ax
        mov     fs,ax       ; to make KRNL286.EXE and DOSX happy
;       pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   Dither16L() - dither using lookup table(s)
;
;       using dither matrix:
;
;           0 2 2 4
;           3 4 0 1 [or possibly 3 3 1 1 -- see which looks better]
;           2 0 4 2
;           3 3 1 1
;
;       the dither matrix determines which lookup table to use.
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

FOO     macro   reg, err
if err eq 0
        mov     reg, ds:[ebx]
elseif err eq 1
        mov     reg, ds:[ebx + edx]
elseif err eq 2
        mov     reg, ds:[ebx + 2*edx]
elseif err eq 3
        or      bh,80h
        mov     reg, ds:[ebx + 2*edx]
elseif err eq 4
        mov     reg, ds:[ebx + 4*edx]
else
        bark
endif
        endm


DITH16L macro m1, m2, m3, m4
        mov     bx, fs:[esi + 4]
        and     bh, ch
        FOO     al, m3

        mov     bx, fs:[esi + 6]
        and     bh, ch
        FOO     ah, m4

        shl     eax,16

        mov     bx, fs:[esi + 0]
        and     bh, ch
        FOO     al, m1

        mov     bx, fs:[esi + 2]
        and     bh, ch
        FOO     ah, m2

        mov     dword ptr es:[edi], eax

        add     edi, 4
        add     esi, 8
        endm

Dither16LJumpTable label dword
        dd      Dither16lScan0
        dd      Dither16lScan3
        dd      Dither16lScan2
        dd      Dither16lScan1

cProc   Dither16L,<FAR,PUBLIC,PASCAL>,<>
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
        jc      Dither16lExit

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   ecx, DstYE               ; divide by 4
        mov     ebx, ecx

        add     ecx, 3                   ; be sure to round up
        shr     ecx, 2
        jz      Dither16lExit
        mov     DstYE, cx

        movzx   ecx, DstXE               ; divide by 4
        shr     ecx,2
        jz      Dither16lExit
        mov     DstXE,cx

        mov     ds, lpDitherTable.sel   ; DS --> dither table
        mov     ax, ds
        add     ax, __AHINCR
        mov     gs, ax

        xor     ebx, ebx
        mov     edx, 32768
        mov     ch,7Fh

        and     ebx, 011b                ; Get height mod 4
        jmp     Dither16LJumpTable[ebx*4]

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        align   4
Dither16lOuterLoop:

        mov     cl, byte ptr DstXE
        align   4
Dither16lScan0:
        DITH16L 0 2 2 4
        dec     cl
        jnz     short Dither16lScan0
        add     esi, SrcInc
        add     edi, DstInc

        mov     cl, byte ptr DstXE
        align   4
Dither16lScan1:
        DITH16L 3 4 0 1
        dec     cl
        jnz     short Dither16lScan1
        add     esi, SrcInc
        add     edi, DstInc

        mov     cl, byte ptr DstXE
        align   4
Dither16lScan2:
        DITH16L 2 0 4 2
        dec     cl
        jnz     short Dither16lScan2
        add     esi, SrcInc
        add     edi, DstInc

        mov     cl, byte ptr DstXE
        align   4
Dither16lScan3:
        DITH16L 3 3 1 1
        dec     cl
        jnz     short Dither16lScan3
        add     esi, SrcInc
        add     edi, DstInc

        dec     DstYE
        jnz     Dither16lOuterLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Dither16lExit:
        xor     ax,ax
        mov     fs,ax       ; to make KRNL286.EXE and DOSX happy
        mov     gs,ax
        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   Dither16S() - dither using scale table(s)
;
;       pel8 = lookup[scale[rgb555] + error]
;
;   using error matrix:
;
;       0       3283    4924    8207
;       6565    6566    1641    1642
;       3283    0       8207    4924
;       6566    4925    3282    1641
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

DITH16S macro m1, m2, m3, m4
        mov     ebx, fs:[esi + 4]           ; grab rgb555
        add     bx, bx
        mov     bx, ds:[bx]                 ; scale it
        mov     al, gs:[bx + m3]            ; dither it with error

        shr     ebx,16
;       mov     bx, fs:[esi + 6]            ; grab rgb555
        add     bx, bx
        mov     bx, ds:[bx]                 ; scale it
        mov     ah, gs:[bx + m4]            ; dither it with error

        shl     eax,16

        mov     ebx, fs:[esi + 0]           ; grab rgb555
        add     bx, bx
        mov     bx, ds:[bx]                 ; scale it
        mov     al, gs:[bx + m1]            ; dither it with error

        shr     ebx,16
;       mov     bx, fs:[esi + 2]            ; grab rgb555
        add     bx, bx
        mov     bx, ds:[bx]                 ; scale it
        mov     ah, gs:[bx + m2]            ; dither it with error

        mov     dword ptr es:[edi], eax

        add     edi, 4
        add     esi, 8
        endm

Dither16SJumpTable label dword
        dd      Dither16sScan0
        dd      Dither16sScan3
        dd      Dither16sScan2
        dd      Dither16sScan1

cProc   Dither16S,<FAR,PUBLIC,PASCAL>,<>
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
	inc	DstXE			; Make the == 3 mod 4 case work
        and     DstXE, not 011b
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     eax,16
        mov     ebx,8
        call    dither_init             ; init all the frame variables
        jc      Dither16sExit

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   ecx, DstYE               ; divide by 4
        mov     ebx, ecx

        add     ecx, 3                   ; be sure to round up
        shr     ecx, 2
        jz      Dither16sExit
        mov     DstYE, cx

        movzx   ecx, DstXE               ; divide by 4
        shr     ecx,2
        jz      Dither16sExit
        mov     DstXE,cx

        mov     ds, lpDitherTable.sel   ; DS --> dither table
        mov     ax, ds
        add     ax, __AHINCR
        mov     gs, ax

        and     ebx, 011b                ; Get height mod 4
        jmp     Dither16SJumpTable[ebx*4]

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        align   4
Dither16sOuterLoop:

        movzx   ecx, DstXE
        align   4
Dither16sScan0:
        DITH16S 0       3283    4924    8207
        dec     ecx
        jnz     short Dither16sScan0
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16sScan1:
        DITH16S 6565    6566    1641    1642
        dec     ecx
        jnz     short Dither16sScan1
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16sScan2:
        DITH16S 3283    0       8207    4924
        dec     ecx
        jnz     short Dither16sScan2
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither16sScan3:
        DITH16S 6566    4925    3282    1641
        dec     ecx
        jnz     short Dither16sScan3
        add     esi, SrcInc
        add     edi, DstInc

        dec     DstYE
        jnz     Dither16sOuterLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Dither16sExit:
        xor     ax,ax
        mov     fs,ax       ; to make KRNL286.EXE and DOSX happy
        mov     gs,ax
        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   LODSB32 - get a byte, every four times doing a LODSD
;
;--------------------------------------------------------------------------;

LODSB32_N = 0

LODSB32 macro
        if (LODSB32_N mod 4) eq 0
;;          lods dword ptr ds:[esi]
            mov     eax,dword ptr fs:[esi]
            add     esi,4
        else
            ror     eax,8
        endif

        LODSB32_N = LODSB32_N + 1

        endm

;--------------------------------------------------------------------------;
;
;   Dither24S() - dither using scale table(s)
;
;       pel8 = lookup[scale[rgb555] + error]
;
;   using error matrix:
;
;       0       3283    4924    8207
;       6565    6566    1641    1642
;       3283    0       8207    4924
;       6566    4925    3282    1641
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

GET24   macro
        LODSB32             ; get BLUE
        and     al,0F8h
        mov     bl,al
        LODSB32             ; get GREEN
        mov     bh,al
        shr     bh,3
        shr     ebx,2
        LODSB32             ; get RED
        and     al,0F8h
        or      bh,al
        endm

DITH24S macro m1, m2, m3, m4

        GET24                               ; grab rgb555*2
        movzx   ebx, word ptr ds:[ebx]      ; scale it
        mov     dl, ds:[ebx + m1 + 65536]   ; dither it with error

        GET24                               ; grab rgb555*2
        movzx   ebx, word ptr ds:[ebx]      ; scale it
        mov     dh, ds:[ebx + m2 + 65536]   ; dither it with error

        ror     edx,16                      ; save ax

        GET24                               ; grab rgb555
        movzx   ebx, word ptr ds:[ebx]      ; scale it
        mov     dl, ds:[ebx + m3 + 65536]   ; dither it with error

        GET24                               ; grab rgb555
        movzx   ebx, word ptr ds:[ebx]      ; scale it
        mov     dh, ds:[ebx + m4 + 65536]   ; dither it with error

        ror     edx,16                      ; get eax right
        mov     dword ptr es:[edi], edx     ; store four pixels
        add     edi, 4
        endm

Dither24SJumpTable label dword
        dd      Dither24sScan0
        dd      Dither24sScan3
        dd      Dither24sScan2
        dd      Dither24sScan1

cProc   Dither24S,<FAR,PUBLIC,PASCAL>,<>
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
        jc      Dither24sExit

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   ecx, DstYE               ; divide by 4
        mov     ebx, ecx

        add     ecx, 3                   ; be sure to round up
        shr     ecx, 2
        jz      Dither24sExit
        mov     DstYE, cx

        movzx   ecx, DstXE               ; divide by 4
        shr     ecx,2
        jz      Dither24sExit
        mov     DstXE,cx

        mov     ds, lpDitherTable.sel   ; DS --> dither table
        mov     ax, ds
        add     ax, __AHINCR
        mov     gs, ax

        and     ebx, 011b                ; Get height mod 4
        jmp     Dither24SJumpTable[ebx*4]

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        align   4
Dither24sOuterLoop:

        movzx   ecx, DstXE
        align   4
Dither24sScan0:
        DITH24S 0       3283    4924    8207
        dec     ecx
        jnz     Dither24sScan0
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither24sScan1:
        DITH24S 6565    6566    1641    1642
        dec     ecx
        jnz     Dither24sScan1
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither24sScan2:
        DITH24S 3283    0       8207    4924
        dec     ecx
        jnz     Dither24sScan2
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither24sScan3:
        DITH24S 6566    4925    3282    1641
        dec     ecx
        jnz     Dither24sScan3
        add     esi, SrcInc
        add     edi, DstInc

        dec     DstYE
        jnz     Dither24sOuterLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Dither24sExit:
        xor     ax,ax
        mov     fs,ax       ; to make KRNL286.EXE and DOSX happy
        mov     gs,ax
        pop     ds
        pop     edi
        pop     esi
cEnd

;--------------------------------------------------------------------------;
;
;   Dither32S() - dither using scale table(s)
;
;       pel8 = lookup[scale[rgb555] + error]
;
;   using error matrix:
;
;       0       3283    4924    8207
;       6565    6566    1641    1642
;       3283    0       8207    4924
;       6566    4925    3282    1641
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

GET32   macro
if 1
        mov     bx,fs:[esi+0]   ; ebx = ????????????????GGGGGgggBBBBBbbb
        shr     bh,3            ; ebx = 000?????????????000GGGGGBBBBBbbb
        shr     bx,3            ; ebx = 000?????????????000000GGGGGBBBBB
        mov     al,fs:[esi+2]   ; eax = ????????????????????????RRRRRrrr
        and     al,0F8h         ; eax = ????????????????????????RRRRR000
        shr     al,1            ; eax = ????????????????????????0RRRRR00
        or      bh,al           ; ebx = 000?????????????0RRRRRGGGGGBBBBB
        add     ebx,ebx         ; ebx = 00?????????????0RRRRRGGGGGBBBBB0
        add     esi,4
else
        mov     eax,fs:[esi]        ; eax = RRRRRrrrGGGGGgggBBBBBbbb
        add     esi,2
        mov     bl,al               ; ebx = 00000000????????BBBBBbbb
        shr     eax,8               ; eax = 00000000RRRRRrrrGGGGGggg
        shr     ah,3                ; eax = 00000000000RRRRRGGGGGggg
        shl     ax,5                ; eax = 000000RRRRRGGGGGggg00000
endif
        endm

DITH32S macro m1, m2, m3, m4

        GET32                               ; grab rgb555*2
        mov     bx, word ptr ds:[ebx]       ; scale it
        mov     dl, ds:[ebx + m1 + 65536]   ; dither it with error

        GET32                               ; grab rgb555*2
        mov     bx, word ptr ds:[ebx]       ; scale it
        mov     dh, ds:[ebx + m2 + 65536]   ; dither it with error

        ror     edx,16                      ; save ax

        GET32                               ; grab rgb555
        mov     bx, word ptr ds:[ebx]       ; scale it
        mov     dl, ds:[ebx + m3 + 65536]   ; dither it with error

        GET32                               ; grab rgb555
        movzx   bx, word ptr ds:[ebx]       ; scale it
        mov     dh, ds:[ebx + m4 + 65536]   ; dither it with error

        ror     edx,16                      ; get eax right
        mov     dword ptr es:[edi], edx     ; store four pixels
        add     edi, 4
        endm

Dither32SJumpTable label dword
        dd      Dither32sScan0
        dd      Dither32sScan3
        dd      Dither32sScan2
        dd      Dither32sScan1

cProc   Dither32S,<FAR,PUBLIC,PASCAL>,<>
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
        jc      Dither32sExit

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        movzx   ecx, DstYE               ; divide by 4
        mov     ebx, ecx

        add     ecx, 3                   ; be sure to round up
        shr     ecx, 2
        jz      Dither32sExit
        mov     DstYE, cx

        movzx   ecx, DstXE               ; divide by 4
        shr     ecx,2
        jz      Dither32sExit
        mov     DstXE,cx

        mov     ds, lpDitherTable.sel   ; DS --> dither table
        mov     ax, ds
        add     ax, __AHINCR
        mov     gs, ax

        and     ebx, 011b                ; Get height mod 4
        jmp     Dither32SJumpTable[ebx*4]

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        align   4
Dither32sOuterLoop:

        movzx   ecx, DstXE
        align   4
Dither32sScan0:
        DITH32S 0       3283    4924    8207
        dec     ecx
        jnz     Dither32sScan0
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither32sScan1:
        DITH32S 6565    6566    1641    1642
        dec     ecx
        jnz     Dither32sScan1
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither32sScan2:
        DITH32S 3283    0       8207    4924
        dec     ecx
        jnz     Dither32sScan2
        add     esi, SrcInc
        add     edi, DstInc

        movzx   ecx, DstXE
        align   4
Dither32sScan3:
        DITH32S 6566    4925    3282    1641
        dec     ecx
        jnz     Dither32sScan3
        add     esi, SrcInc
        add     edi, DstInc

        dec     DstYE
        jnz     Dither32sOuterLoop

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
Dither32sExit:
        xor     ax,ax
        mov     fs,ax       ; to make KRNL286.EXE and DOSX happy
        mov     gs,ax
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
;       ss:bp   --> ditherdib frame
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

        mov     cx, es:[di].biBitCount    ; dest must be right
        cmp     cx, bx
        jne     dither_init_error

        mov     cx, fs:[si].biBitCount   ; source must be right
        cmp     cx, ax
        jne     dither_init_error

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
