        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  VFLAT.ASM
;
;   module for doing direct video access under windows.
;
;   we will talk to VflatD to get the linear address of the video buffer.
;
;   we MUST not use these API in the background, how do we do this?
;
;   we support the following modes: (same as SVGA256...)
;
;        VRAM II 640x480x8bpp
;        VRAM II 720x512x8bpp
;        VRAM II 800x600x8bpp
;        VRAM II 1024x768x8bpp
;
;        V7 VGA 640x480x8bpp
;        V7 VGA 720x512x8bpp
;        V7 VGA 800x600x8bpp
;        V7 VGA 1024x768x8bpp
;
;        WD VGA 640x480x8bpp
;        WD VGA 800x600x8bpp
;        WD VGA 1024x768x8bpp
;        WD VGA 640x480x16bpp
;        WD VGA 800x600x16bpp
;
;        Trident 640x480x8bpp
;        Trident 800x600x8bpp
;        Trident 1024x768x8bpp
;
;        Oak 640x480x8bpp
;        Oak 800x600x8bpp
;        Oak 1024x768x8bpp
;
;        ATI 640x480x8bpp
;        ATI 800x600x8bpp
;        ATI 1024x768x8bpp
;        ATI 640x480x24bpp
;
;        Compaq AVGA 640x480x8bpp
;
;        Compaq QVision 640x480x8bpp
;        Compaq QVision 800x600x8bpp
;        Compaq QVision 1024x768x8bpp
;
;        Compaq QVision 640x480x16bpp
;        Compaq QVision 800x600x16bpp
;        Compaq QVision 1024x768x16bpp
;
;        Tseng ET4000 640x480x8bpp
;        Tseng ET4000 800x600x8bpp
;        Tseng ET4000 1024x768x8bpp
;        Tseng ET4000 640x480x16bpp
;        Tseng ET4000 800x600x16bpp
;
;        Everex 640x480x8bpp
;        Everex 800x600x8bpp
;        Everex 1024x768x8bpp
;
;        Cirrus 542x 640x480x8bpp
;        Cirrus 542x 800x600x8bpp
;        Cirrus 542x 1024x768x8bpp
;
;        Cirrus 6420 640x480x8bpp
;        Cirrus 6420 800x600x8bpp
;        Cirrus 6420 1024x768x8bpp
;
; Created:  03-20-90
; Author:   Todd Laney [ToddLa]
;
; Copyright (c) 1984-1994 Microsoft Corporation
;
; Public Functions:
;
;       VFlatInit()
;
; Public Data:
;
; General Description:
;
; Restrictions:
;
;-----------------------------------------------------------------------;

?PLM = 1
?WIN = 0
.386
	.xlist
	include cmacros.inc
        include windows.inc
        WIN31=1
        include VflatD.inc
        .list

        externFP        GetDC
        externFP        ReleaseDC
        externFP        GetDeviceCaps
        externFP        OutputDebugString
        externFP        WriteProfileString

        externA         __C000h
        externA         __A000h

sBegin  Data

        ScreenMode  dw      0                   ; current mode (index)
        VflatD_Proc dd      0                   ; VflatD entry point

        bank_save           dw      0           ; saved bank...

sEnd    Data

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------

ifdef DEBUG
DPF     macro   text
        local   string, string_end
        jmp short string_end
string label byte
        db      "&text&",13,10,0
string_end label byte
        pusha
        push    es
        push    cs
        push    offset string
        call    OutputDebugString
        pop     es
        popa
        endm
else
DPF     macro text
        endm
endif

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin CodeSeg
        .386p
        assumes cs,CodeSeg
        assumes ds,Data
        assumes es,nothing

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------

ifdef DEBUG
szDebug     db "Debug", 0
szDrawDib   db "DrawDib", 0
szDetect    db "detect", 0
szDetectDVA db "DetectDVA: ", 0
szNone      db "None", 0
endif

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------

ModeInfo STRUC

    ModeNext            dw      ?
    ModeDetect          dw      ?
    ModeNum             dw      ?
    ModeWidth           dw      ?
    ModeHeight          dw      ?
    ModeDepth           dw      ?
    ModeSetBank         dw      ?
    ModeGetBank         dw      ?
    ModeSetBank32       dw      ?
    ModeSetBank32Size   dw      ?
    ModeName            db      ?

ModeInfo ENDS

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------

Mode macro y, n, w, h, b, x, name
        local   l1,l2
l1:
        dw      l2 - l1
        dw      Detect&y, n, w, h, b, SetBank&x, GetBank&x, SetBank32&x, SetBank32&x&Size

ifdef DEBUG
        db      name
        db      0
endif

l2:
        endm

ModeInfoTable label byte

        Mode VRAM, 67h, 640, 480, 8, VRAM, "VRAM II 640x480x8bpp"
        Mode VRAM, 68h, 720, 512, 8, VRAM, "VRAM II 720x512x8bpp"
        Mode VRAM, 69h, 800, 600, 8, VRAM, "VRAM II 800x600x8bpp"
        Mode VRAM, 6Ah, 1024,768, 8, VRAM, "VRAM II 1024x768x8bpp"

        Mode V7,   67h, 640, 480, 8, V7, "V7 VGA 640x480x8bpp"
        Mode V7,   68h, 720, 512, 8, V7, "V7 VGA 720x512x8bpp"
        Mode V7,   69h, 800, 600, 8, V7, "V7 VGA 800x600x8bpp"
        Mode V7,   6Ah, 1024,768, 8, V7, "V7 VGA 1024x768x8bpp"

        Mode WD,   5Fh, 640, 480, 8, WD, "WD VGA 640x480x8bpp"
        Mode WD,   5Ch, 800, 600, 8, WD, "WD VGA 800x600x8bpp"
        Mode WD,   60h, 1024,768, 8, WD, "WD VGA 1024x768x8bpp"
        Mode WD,   64h, 640, 480,16, WD, "WD VGA 640x480x16bpp"
        Mode WD,   65h, 800, 600,16, WD, "WD VGA 800x600x16bpp"

        Mode Trident, 5Dh, 640, 480, 8, Trident, "Trident 640x480x8bpp"
        Mode Trident, 5Eh, 800, 600, 8, Trident, "Trident 800x600x8bpp"
        Mode Trident, 62h, 1024,768, 8, Trident, "Trident 1024x768x8bpp"

        Mode Oak,  53h, 640, 480, 8, Oak, "Oak 640x480x8bpp"
        Mode Oak,  54h, 800, 600, 8, Oak, "Oak 800x600x8bpp"
        Mode Oak,  59h, 1024,768, 8, Oak, "Oak 1024x768x8bpp"

        Mode ATI,  12h, 640, 480, 8, ATI, "ATI 640x480x8bpp"
        Mode ATI,  12h, 800, 600, 8, ATI, "ATI 800x600x8bpp"
        Mode ATI,  12h, 1024,768, 8, ATI, "ATI 1024x768x8bpp"
        Mode ATI,  12h, 2048,1024,8, ATI, "ATI 2048x1024x8bpp"

        Mode ATI,  12h, 640, 480,16, ATI, "ATI 640x480x16bpp"
        Mode ATI,  12h, 800, 600,16, ATI, "ATI 800x600x16bpp"
        Mode ATI,  12h, 1024,768,16, ATI, "ATI 1024x768x16bpp"
        Mode ATI,  12h, 2048,1024,16,ATI, "ATI 2048x1024x16bpp"

        Mode ATI,  12h, 640, 480,24, ATI, "ATI 640x480x24bpp"
        Mode ATI,  12h, 800, 600,24, ATI, "ATI 800x600x24bpp"
        Mode ATI,  12h, 1024,768,24, ATI, "ATI 1024x768x24bpp"
        Mode ATI,  12h, 2048,1024,24,ATI, "ATI 2048x1024x24bpp"

        Mode ATI,  62h, 640, 480, 8, ATI, "ATI 640x480x8bpp"
        Mode ATI,  63h, 800, 600, 8, ATI, "ATI 800x600x8bpp"
        Mode ATI,  64h, 1024,768, 8, ATI, "ATI 1024x768x8bpp"
        Mode ATI,  75h, 640, 480,24, ATI, "ATI 640x480x24bpp"

        Mode Compaq,2Eh, 640, 480, 8, Compaq, "Compaq AVGA 640x480x8bpp"
        Mode Compaq,12h, 640, 480, 8, Compaq, "Compaq AVGA 640x480x8bpp"

        Mode Compaq,06h, 640, 480, 8, Compaq, "Compaq QVision 640x480x8bpp"
        Mode Compaq,06h, 800, 600, 8, Compaq, "Compaq QVision 800x600x8bpp"
        Mode Compaq,06h, 1024,768, 8, Compaq, "Compaq QVision 1024x768x8bpp"

        Mode Compaq,06h, 640, 480, 16, Compaq, "Compaq QVision 640x480x16bpp"
        Mode Compaq,06h, 800, 600, 16, Compaq, "Compaq QVision 800x600x16bpp"
        Mode Compaq,06h, 1024,768, 16, Compaq, "Compaq QVision 1024x768x16bpp"

        Mode Tseng, 2Eh, 640, 480, 8, Tseng, "Tseng ET4000 640x480x8bpp"
        Mode Tseng, 30h, 800, 600, 8, Tseng, "Tseng ET4000 800x600x8bpp"
        Mode Tseng, 38h, 1024,768, 8, Tseng, "Tseng ET4000 1024x768x8bpp"
        Mode Tseng, 2Eh, 640, 480,16, Tseng, "Tseng ET4000 640x480x16bpp"
        Mode Tseng, 30h, 800, 600,16, Tseng, "Tseng ET4000 800x600x16bpp"

        Mode Everex, 2Eh, 640, 480, 8, Tseng, "Everex 640x480x8bpp"
        Mode Everex, 30h, 800, 600, 8, Tseng, "Everex 800x600x8bpp"
        Mode Everex, 38h, 1024,768, 8, Tseng, "Everex 1024x768x8bpp"

; Until we get a detect routine for Cirrus, we won't include these.

        ;Mode C542x, 5Fh, 640, 480, 8, C542x, "C542x 640x480x8bpp"
        ;Mode C542x, 5Ch, 800, 600, 8, C542x, "C542x 800x600x8bpp"
        ;Mode C542x, 60h, 1024,768, 8, C542x, "C542x 1024x768x8bpp"

        ;Mode C6420, 2Eh, 640, 480, 8, C6420, "C6420 640x480x8bpp"
        ;Mode C6420, 30h, 800, 600, 8, C6420, "C6420 800x600x8bpp"
        ;Mode C6420, 38h, 1024,768, 8, C6420, "C6420 1024x768x8bpp"

ModeInfoTableEnd label byte

;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;

szTseng:    db  "Tseng", 0
szOAK:      db  " OAK", 0
szTrident:  db  "TRIDENT", 0
szEverex:   db  "Everex", 0
szParadise: db  "PARADISE", 0
szWD:       db  "WESTERN DIGITAL", 0
szWeitek:   db  "WEITEK",0
szViper:    db  "VIPER VLB",0

;---------------------------Public-Routine------------------------------;
; VFlatInit
;
;       initialize for a banked display
;
; Returns:
;       C       if error
;       NC      if success
;-----------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

cProc   VFlatInit, <NEAR, PASCAL, PUBLIC>, <si,di,ds>
        localW  hdc
        localW  ScreenWidth
        localW  ScreenHeight
        localW  ScreenDepth
        localW  BiosMode
cBegin

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;   get a display DC and get resolution info
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
        cCall   GetDC, <0>
        mov     hdc,ax

        cCall   GetDeviceCaps, <hdc,HORZRES>
        mov     ScreenWidth,ax

        cCall   GetDeviceCaps, <hdc,VERTRES>
        mov     ScreenHeight,ax

        cCall   GetDeviceCaps, <hdc,BITSPIXEL>
        push    ax
        cCall   GetDeviceCaps, <hdc,PLANES>
        pop     dx
        mul     dx
        mov     ScreenDepth,ax

        cCall   ReleaseDC, <0, hdc>

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;   scan our mode table
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;

        mov     ax, 6F04h               ;Get V7 mode
        int     10h
        cmp     al,04h
        jne     short @f
	mov	ax,0F00h		;Call BIOS to get mode back.
	int	10h			;al = mode we are in.
@@:     xor     ah,ah
        mov     BiosMode,ax

        lea     bx,ModeInfoTable

mode_search:
        mov     ax,BiosMode
        cmp     cs:[bx].ModeNum,ax
        jne     short mode_search_next

        mov     ax,ScreenWidth
        cmp     cs:[bx].ModeWidth,ax
        jne     short mode_search_next

        mov     ax,ScreenHeight
        cmp     cs:[bx].ModeHeight,ax
        jne     short mode_search_next

        mov     ax,ScreenDepth
        cmp     cs:[bx].ModeDepth,ax
        jne     short mode_search_next

        push    bx
        call    cs:[bx].ModeDetect
        pop     bx
        or      ax,ax
        jnz     short mode_search_found
        errn$   mode_search_next

mode_search_next:
        add     bx,cs:[bx].ModeNext
        cmp     bx,offset ModeInfoTableEnd
        jl      mode_search
        jge     mode_search_fail

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
mode_search_found:
        mov     ScreenMode,bx               ; save this for later.

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;   this is a banked display, we need to talk to VflatD in order for
;   anything to work.
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
	xor	di,di
	mov	es,di
	mov	ax,1684h
        mov     bx,VflatD_Windows_ID
	int	2fh			    ;returns with es:di-->VFlatD Entry point
        mov     word ptr [VflatD_Proc][0],di
        mov     word ptr [VflatD_Proc][2],es
	mov	ax,es
        or      ax,di
        jne     short mode_search_vflat

	xor	di,di
	mov	es,di
	mov	ax,1684h
        mov     bx,VflatD_Chicago_ID
	int	2fh			    ;returns with es:di-->VFlatD Entry point
        mov     word ptr [VflatD_Proc][0],di
        mov     word ptr [VflatD_Proc][2],es
	mov	ax,es
        or      ax,di
        jz      short mode_search_fail

mode_search_vflat:
	xor	ax,ax
	mov	dx,VflatD_Get_Version
	call	[VflatD_Proc]
        cmp     ax,0100h
        jb      short mode_search_fail

        ;
        ;   estimate the required framebuffer memory
        ;
        mov     ax,ScreenDepth              ; bitdepth
        mul     ScreenWidth                 ; * width = bit width
        shr     ax,3                        ; / 8 = width bytes
        add     ax,1024-1                   ; round up to nearest K
        and     ax,not (1024-1)             ; now we have scan width
        mul     ScreenHeight                ; * number of scans = total bytes
        add     ax,0FFFFh                   ; round up to nearest MB
        adc     dx,0000Fh
        and     dx,0FFF0h
        shl     dx,4                        ; convert to 4K pages.
        mov     ax,dx

        mov     dx,VflatD_Get_Sel           ; get selector
;;;;;;;;mov     ax,512                      ; size in pages of video memory?
	mov	bx,ScreenMode
        mov     cx,cs:[bx].ModeSetBank32Size; size of bank code.
        mov     di,cs:[bx].ModeSetBank32    ; point es:di to bank code.
	push	cs
	pop	es
        call    [VflatD_Proc]               ; let VflatD init things.
        jc      short mode_search_fail
        or      ax,ax
        jz      short mode_search_fail

        errn$   mode_search_ok

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
mode_search_ok:

ifdef DEBUG
        pusha
        mov     bx,ScreenMode
        lea     ax,[bx].ModeName
        lea     bx,szDrawDib
        lea     cx,szDetect

        cCall   WriteProfileString, <cs,bx, cs,cx, cs,ax>
        popa
endif
        mov     dx, ax
        xor     ax, ax
        jmp     short mode_search_exit

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
mode_search_fail:
ifdef DEBUG
        pusha
        lea     ax,szDrawDib
        lea     bx,szDetect
        lea     cx,szNone
        cCall   WriteProfileString, <cs,ax, cs,bx, cs,cx>
        popa
endif
        xor     ax,ax
        mov     dx,ax
        mov     ScreenMode,ax
        mov     word ptr [VflatD_Proc][0],ax
        mov     word ptr [VflatD_Proc][2],ax
        errn$   mode_search_exit

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
mode_search_exit:

cEnd

;---------------------------Public-Routine------------------------------;
; VFlatBegin - start direct frame buffer access
;
; Returns:
;       Wed 04-Jan-1993 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

cProc   VFlatBegin, <NEAR, PUBLIC>, <>
cBegin
        mov     bx, ScreenMode
        or      bx, bx
        jz      short BeginExit

%out is CLI/STI needed?
;;;;;;;;cli
        call    cs:[bx].ModeGetBank
        xchg    bank_save,ax
        mov     dx,ax
        call    cs:[bx].ModeSetBank
;;;;;;;;sti
BeginExit:
cEnd

;---------------------------Public-Routine------------------------------;
; VFlatEnd - end direct frame buffer access
;
; Returns:
;       Wed 04-Jan-1993 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

cProc   VFlatEnd, <NEAR, PUBLIC>, <>
cBegin
        mov     bx, ScreenMode
        or      bx, bx
        jz      short EndExit

%out is CLI/STI needed?
;;;;;;;;cli
        call    cs:[bx].ModeGetBank
        xchg    bank_save,ax
        mov     dx,ax
        call    cs:[bx].ModeSetBank
;;;;;;;;sti
EndExit:
cEnd

;---------------------------Public-Routine------------------------------;
; ScanROM   - scan the video bios ROM looking for a string
;
; Entry:
;       cs:ax   - string to look for.
;
; Returns:
;       Wed 04-Jan-1993 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

ScanROM proc near
        push    si
        mov     si,ax

        mov     ax,__C000h
        mov     es,ax

        xor     bx,bx               ; start at zero
        mov     cx,512              ; search first 512 bytes.

        mov     dx,si
scan_start:
        mov     si,dx
        mov     al,cs:[si]
scan_cmp:
        cmp     byte ptr es:[bx], al
        je      short scan_found
        inc     bx
        loop    scan_start
        xor     ax,ax
        pop     si
        ret

scan_next:
        inc     bx
        loop    scan_cmp
        xor     ax,ax
        pop     si
        ret

scan_found:
        inc     si
        mov     al,cs:[si]
        or      al,al
        jnz     scan_next
        inc     ax
        pop     si
        ret

ScanROM  endp

;----------------------------------------------------------------------------
; BANK SWITCH TEMPLATES
;  Each template is given to vflatd.386 which copies it inline in to the 
;  page fault handling code.
; NOTE: This code runs at ring 0 in a USE32 code segment, so be carefull!!!
; ALL REGISTERS MUST BE PRESERVED (except for dx)
;----------------------------------------------------------------------------

;****************************************************************************
; V7
;****************************************************************************

DetectV7 proc near

        mov     ax,6f00h                ;Test for Video 7
	xor	bx,bx
	cld
        int     10h
        xor     ax,ax
        cmp     bx,'V7'
        jne     short @f
        inc     ax
@@:     ret

DetectV7 endp

SetBank32V7 label byte
	push	ax
	push	bx

        mov     bl,al
        and     bl,1                    ; BL = extended page select

        mov     ah,al
        and     ah,2
        shl     ah,4                    ; AH = page select bit

        and     al,00ch
        mov     bh,al
        shr     al,2
        or      bh,al                   ; BH = 256K bank select

        db      66h,0bah,0cch,03h       ;mov dx, 3CCh
        in      al,dx                   ; Get Miscellaneous Output Register
        and     al,not 20h              ; Clear page select bit
        or      al,ah                   ; Set page select bit (maybe)
        mov     dl,0c2h                 ; Write Miscellaneous Output Register
        out     dx,al

        mov     dl,0c4h                 ; Sequencer
        mov     al,0f9h                 ; Extended page select register
        mov     ah,bl                   ; Extended page select value
        out     dx,eax			; out dx,ax

        mov     al,0f6h                 ; 256K bank select
        out     dx,al
        inc     dx                      ; Point to data
        in      al,dx
        and     al,0f0h                 ; Clear out bank select banks
        or      al,bh                   ; Set bank select banks (maybe)
        out     dx,al
	pop	bx
        pop     ax
SetBank32V7Size = $ - SetBank32V7

SetBankV7 proc near
        mov     bl,dl
        and     bl,1                    ; BL = extended page select

        mov     ah,dl
        and     ah,2
        shl     ah,4                    ; AH = page select bit

        and     dl,00ch
        mov     bh,dl
        shr     dl,2
        or      bh,dl                   ; BH = 256K bank select

        mov     dx,03cch
        in      al,dx                   ; Get Miscellaneous Output Register
        and     al,not 20h              ; Clear page select bit
        or      al,ah                   ; Set page select bit (maybe)
        mov     dl,0c2h                 ; Write Miscellaneous Output Register
        out     dx,al

        mov     dl,0c4h                 ; Sequencer
        mov     al,0f9h                 ; Extended page select register
        mov     ah,bl                   ; Extended page select value
        out     dx,ax

        mov     al,0f6h                 ; 256K bank select
        out     dx,al
        inc     dx                      ; Point to data
        in      al,dx
        and     al,0f0h                 ; Clear out bank select banks
        or      al,bh                   ; Set bank select banks (maybe)
        out     dx,al
	ret
SetBankV7 endp

GetBankV7      proc    near
	mov	dx,3cch
        in      al,dx
        and     al,20h                  ; page select bit
        shr     al,4
        mov     ah,al

	mov	dx,3C4h
	mov	al,0f9h
	out	dx,al
	inc	dx
        in      al,dx
        and     al,1
        or      ah,al

	dec	dx
	mov	al,0F6h
	out	dx,al
	inc	dx
        in      al,dx
        and     al,0ch
        or      al,ah
        xor     ah,ah
	ret
GetBankV7      endp

;****************************************************************************
; V7 II
;****************************************************************************

DetectVRAM proc near
if 0    ; ack!
        call    DetectV7
        or      ax,ax
        jz      short novram

	mov	dx,03C4H
	mov	al,08FH
	out	dx,al
	inc	dx
	in	al,dx
	mov	ah,al

	dec	dx
	mov	al,08EH
	out	dx,al
	inc	dx
	in	al,dx

        cmp     ax,07151H               ;VRAMII rev B id
        je      short isvram
        cmp     ax,07152H               ;VRAMII rev C and D id
        je      short isvram
;       cmp     ax,07760H               ;HT216 rev B and C
;       je      short isvram
;       cmp     ax,07763H               ;HT216 rev B, C, and D
;       je      short isvram
;       cmp     ax,07764H               ;HT216 rev E
;       je      short isvram
endif
novram:
        xor     ax,ax
        ret
isvram:
        mov     ax,1
        ret

DetectVRAM endp

SetBank32VRAM label byte
        push    ax                      ;push eax
        shl     dl,4
        mov     ah,dl
        db      66h,0bah,0c4h,03h       ; mov dx, 3C4h
	mov	al,0e8h
        out     dx,eax			; out dx,ax
        pop     ax                      ; pop eax
SetBank32VRAMSize = $ - SetBank32VRAM

SetBankVRAM proc near
        shl     dl,4
        mov     ah,dl
	mov	dx,03c4h
	mov	al,0e8h
        out     dx,ax
        ret
SetBankVRAM endp

GetBankVRAM proc near
        mov     dx,3c4h
        mov     al,0e8h
	out	dx,al
	inc	dx
        in      al,dx
        shr     al,4
        ret
GetBankVRAM endp

;****************************************************************************
; ATI
;****************************************************************************

public DetectATI
DetectATI proc near
        mov     ax,__C000h              ;ATI VGA detect (largely from ATI example code)
        mov     es,ax
        xor     ax,ax
	cmp	word ptr es:[40h],'13'	;ATI Signiture on the Video BIOS
        jne     short @f
        inc     ax
@@:     ret
DetectATI endp

SetBank32ATI label byte
	push	ax
	mov	ah,al
	shl	ah,1
	mov	al,0B2h
        db      66h,0bah,0ceh,01h       ;mov dx, 1CEh
	out	dx,eax
	pop	ax
SetBank32ATISize = $ - SetBank32ATI

SetBankATI proc near
	mov	ah,dl
	shl	ah,1
	mov	al,0b2h		;Page select register index
	mov	dx,1ceh		;
	out	dx,ax		;
	ret
SetBankATI endp

GetBankATI     proc    near
	mov	dx,1ceh
	mov	al,0b2h
	out	dx,al
	inc	dx
        in      al,dx
        shr     al,1
	ret
GetBankATI     endp

;****************************************************************************
; OAK
;****************************************************************************

DetectOAK   proc    near

        lea     ax,szOAK
        jmp     ScanROM

DetectOAK   endp

SetBank32Oak label byte
	push	ax
	mov	ah,al
	shl	al,4
	or	ah,al
        db      66h,0bah,0deh,03h       ;mov dx, 3DEh
	mov	al,11h
	out	dx,eax
	pop	ax
SetBank32OakSize = $ - SetBank32Oak

SetBankOAK proc near
	mov	al,dl
	mov	ah,al
	shl	al,4
	or	ah,al
	mov	dx,3deh
	mov	al,11h
	out	dx,ax
	ret
SetBankOAK endp

GetBankOAK     proc    near
	mov	dx,3deh
	mov	al,11h
	out	dx,al
	inc	dx
        in      al,dx
        and     al,0Fh
	ret
GetBankOAK        endp

;****************************************************************************
; Everex
;****************************************************************************

DetectEverex  proc    near

        lea     ax,szEverex
        jmp     ScanROM

DetectEverex    endp

;****************************************************************************
; Tseng
;****************************************************************************

DetectTseng  proc    near

        lea     ax,szTseng
        jmp     ScanROM

DetectTseng   endp

SetBank32Tseng label byte
	mov	dx, ax                  ;mov edx,eax
	shl	al, 4
	or	al, dl
        db      66h,0bah,0cdh,03h       ;mov dx, 3CDh
	out	dx, al                              
	shr	al, 4                   ;shr al,4
SetBank32TsengSize = $ - SetBank32Tseng

SetBankTseng proc near
        and     al,0fh
	mov	al,dl
	mov	ah,al
	shl	al,4
	or	al,ah
	mov	dx,3cdh
	out	dx,al
	ret
SetBankTseng endp

GetBankTseng proc near
	mov	dx,3cdh
        in      al,dx
	shr	al, 4                   ;shr al,4
	ret
GetBankTseng endp

;****************************************************************************
; WD
;****************************************************************************

DetectWD  proc    near

        lea     ax,szWD
        call    ScanROM
        or      ax,ax
        jz      short @f
        ret

@@:     lea     ax,szParadise
        jmp     ScanROM

DetectWD endp

SetBank32WD label byte
        push    ax
	mov	ah,al			;ah = bank number
        mov     al,9                    ;select the primary "bank adder" reg
        shl     ah,4
        db      66h,0bah,0ceh,03h       ;mov dx, 3CEh
        out     dx,eax                  ;out dx,ax (write 3cf:09, desired bank)
        pop     ax
SetBank32WDSize = $ - SetBank32WD

SetBankWD proc near
        mov     al,9                    ;select the primary "bank adder" reg
        mov     ah,dl
        shl     ah,4
        mov     dx,3ceh
        out     dx,ax                   ;write 3cf:09, desired bank
        ret
SetBankWD endp

GetBankWD      proc near
	mov	dx,3ceh
        mov     al,9
	out	dx,al
	inc	dx
        in      al,dx
        shr     al,4
	ret
GetBankWD      endp

;****************************************************************************
; Weitek
;****************************************************************************

DetectWeitek  proc    near

        lea     ax,szWeitek
        jmp     ScanROM

DetectWeitek   endp

;****************************************************************************
; Trident
;****************************************************************************

DetectTrident proc    near

        lea     ax,szTrident
        jmp     ScanROM

DetectTrident endp

SetBank32Trident label byte
	push	ax
	mov	ah,al
	xor	ah,2
	mov	al,0EH
        db      66h,0bah,0c4h,03h       ;mov dx, 3C4h
	out	dx,eax			
;;      db      66h,0bah,0ceh,03h       ;mov dx, 3CEh
;;	out	dx,eax			;for 8900c or better only.
	pop	ax
SetBank32TridentSize = $ - SetBank32Trident

SetBankTrident proc near
	mov	ah,dl
	xor	ah,2
	mov	al,0EH
	mov	dx,3c4h
	out	dx,ax
;;	mov	dx,3ceh
;;	out	dx,ax			;for 8900c or better only.
	ret
SetBankTrident endp

GetBankTrident proc near
	mov	dx,3c4h
	mov	al,0eh
	out	dx,al
	inc	dx
	in	al,dx
;;      xor     al,2			; removed for build 190!
	ret
GetBankTrident endp

;****************************************************************************
; Compaq
;****************************************************************************

DetectCompaq proc near
        mov     ax,__C000h
        mov     es,ax
        xor     ax,ax
        cmp     word ptr es:[2],0E930h
        jne     short @f
        inc     ax
@@:     ret
DetectCompaq endp

SetBank32Compaq label byte
        push    ax
        mov     ah,al
        shl     ah,4
        mov     al,45h
        db      66h,0bah,0ceh,03h       ;mov dx, 3CEh
        out     dx,eax
        inc     al
        add     ah,08h
        out     dx,eax
	pop	ax
SetBank32CompaqSize = $ - SetBank32Compaq

SetBankCompaq proc near
        mov     ah,dl
        shl     ah,4
        mov     dx,03CEh
        mov     al,45h
        out     dx,ax
        inc     al
        add     ah,08h
        out     dx,ax
	ret
SetBankCompaq endp

GetBankCompaq proc near
        mov     dx,03CEh
        mov     al,45h
	out	dx,al
	inc	dx
	in	al,dx
        shr     al,4
	ret
GetBankCompaq endp

;****************************************************************************
; Cirrus 6420
;****************************************************************************

DetectC6420 proc near
        %out *** need a Detect function for Cirrus 6420
        mov     ax,1        ;!!!
        ret
DetectC6420 endp

SetBank32C6420 label byte
        push    ax
        mov     ah,al
	shl	ah,4
        mov     al,0eh
        db      66h,0bah,0ceh,03h       ;mov dx, 3CEh
        out     dx,eax                  ;
        pop     ax

SetBank32C6420Size = $ - SetBank32C6420

SetBankC6420    proc near
        mov     ah,dl
	shl	ah,4
        mov     al,0eh
        mov     dx, 3CEh
        out     dx,ax
        ret

SetBankC6420    endp

GetBankC6420    proc near
        mov     dx,03CEh
        mov     al,0Eh
	out	dx,al
	inc	dx
	in	al,dx
        shr     al,4
	ret

GetBankC6420    endp

;****************************************************************************
; Cirrus 542x
;****************************************************************************

DetectC542x proc near
        %out *** need a Detect function for Cirrus 542x
        mov     ax,1        ;!!!
        ret
DetectC542x endp

SetBank32C542x label byte
        push    ax
        mov     ah,al
	shl	ah,4
        mov     al,09h
        db      66h,0bah,0ceh,03h       ;mov dx, 3CEh
        out     dx,eax
        pop     ax

SetBank32C542xSize = $ - SetBank32C542x

SetBankC542x    proc near
        mov     ah,dl
	shl	ah,4
        mov     al,09h
        mov     dx,3CEh
        out     dx,ax
        ret

SetBankC542x    endp

GetBankC542x    proc near
        mov     dx,03CEh
        mov     al,09h
	out	dx,al
	inc	dx
	in	al,dx
        shr     al,4
	ret

GetBankC542x    endp

;****************************************************************************
; Viper Vesa Local bus
;****************************************************************************

public DetectViper
DetectViper proc near
        lea     ax,szViper
        jmp     ScanROM
DetectViper endp

sEnd

end
