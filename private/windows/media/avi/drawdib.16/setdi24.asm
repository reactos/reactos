page    ,132
;----------------------------Module-Header------------------------------;
; Module Name: SETDI24.ASM
;
; move bits from one DIB format into another. doing color conversion if
; needed.
;
;   convert_8_24
;   convert_16_24
;   convert_24_24
;   convert_32_24
;
; NOTES:
;
;  AUTHOR: ToddLa (Todd Laney) Microsoft
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

ifndef SEGNAME
    SEGNAME equ <_TEXT32>
endif

.386
createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        assumes cs,CodeSeg
        assumes ds,nothing
        assumes es,nothing

;--------------------------------------------------------------------------;
;--------------------------------------------------------------------------;

nxtscan macro reg, next_scan, fill_bytes
ifb <fill_bytes>
        add     e&reg,next_scan
else
        mov     eax,e&reg
        add     e&reg,next_scan
        cmp     ax,reg
        sbb     eax,eax
        and     eax,fill_bytes
        add     e&reg,eax
endif
        endm

;--------------------------------------------------------------------------;
;
;   convert_8_24
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   convert_8_24,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
        ParmD   dst_ptr             ; --> dst.
        ParmD   dst_offset          ; offset to start at
        ParmD   dst_next_scan       ; dst_next_scan.
        ParmD   dst_fill_bytes      ; dst_fill_bytes
        ParmD   src_ptr             ; --> src.
        ParmD   src_offset          ; offset to start at
        ParmD   src_next_scan       ; dst_next_scan.
        ParmD   pel_count               ; pixel count.
        ParmD   scan_count              ; scan count.
        ParmD   xlat_table          ; pixel convert table.
cBegin
        xor     esi,esi
        xor     edi,edi
        xor     ebx,ebx
        lfs     si,src_ptr
        les     di,dst_ptr
        lds     bx,xlat_table

        add     esi,src_offset
        add     edi,dst_offset

        mov     eax,pel_count
        sub     src_next_scan,eax
        add     eax,eax
        add     eax,pel_count
        sub     dst_next_scan,eax

        xor     eax,eax
align 4
convert_8_24_start:
        mov     ecx,pel_count
align 4
convert_8_24_loop:
        mov     bl,fs:[esi]     ; grab a pel
        mov     edx,[ebx*4]     ; convert to 24bpp
        mov     es:[edi],dl
        shr     edx,8
        mov     es:[edi+1],dx

        inc     esi
        add     edi,3
        dec     ecx
        jnz     short convert_8_24_loop     ;; ack

convert_8_24_next:
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short convert_8_24_start
cEnd

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
            mov     es:[edi], eax
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
            mov     eax,[esi]
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
        shr     ebx,16              ; get pel from last time
else
        mov     ebx, [esi]          ; grab two pels
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
        and     al,dh   ; F8h
        STOSB32

        mov     al,bh
        shl     al,3
        STOSB32

        endm

;--------------------------------------------------------------------------;
;
;   convert_16_24
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_16_24,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
        ParmD   dst_ptr             ; --> dst.
        ParmD   dst_offset          ; offset to start at
        ParmD   dst_next_scan       ; dst_next_scan.
        ParmD   dst_fill_bytes      ; dst_fill_bytes
        ParmD   src_ptr             ; --> src.
        ParmD   src_offset          ; offset to start at
        ParmD   src_next_scan       ; dst_next_scan.
        ParmD   pel_count           ; pixel count.
        ParmD   scan_count          ; scan count.
        ParmD   xlat_table          ; pixel convert table.
cBegin
        xor     esi,esi
        xor     edi,edi
        lds     si,src_ptr
        les     di,dst_ptr

        add     esi,src_offset
        add     edi,dst_offset

        and     pel_count,not 3         ;;!!!

        mov     eax,pel_count
        add     eax,eax
        sub     src_next_scan,eax
        add     eax,pel_count
        sub     dst_next_scan,eax

        mov     ecx,pel_count
        shr     ecx,2
        mov     pel_count,ecx
        mov     dh,0F8h
align 4
convert_16_24_start:
        mov     ecx,pel_count
align 4
convert_16_24_loop:
        MAP16   0
        MAP16   1
        MAP16   2
        MAP16   3
        dec     ecx
        jnz     convert_16_24_loop

convert_16_24_next:
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     convert_16_24_start
cEnd

;--------------------------------------------------------------------------;
;
;   convert_24_24
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_24_24,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
        ParmD   dst_ptr             ; --> dst.
        ParmD   dst_offset          ; offset to start at
        ParmD   dst_next_scan       ; dst_next_scan.
        ParmD   dst_fill_bytes      ; dst_fill_bytes
        ParmD   src_ptr             ; --> src.
        ParmD   src_offset          ; offset to start at
        ParmD   src_next_scan       ; dst_next_scan.
        ParmD   pel_count               ; pixel count.
        ParmD   scan_count              ; scan count.
        ParmD   xlat_table          ; pixel convert table.
cBegin
        xor     esi,esi
        xor     edi,edi
        lds     si,src_ptr
        les     di,dst_ptr

        add     esi,src_offset
        add     edi,dst_offset

        mov     eax,pel_count
        add     eax,eax
        add     eax,pel_count
        sub     dst_next_scan,eax
        sub     src_next_scan,eax

        mov     ebx,eax
        mov     edx,eax
        shr     ebx,2
        and     edx,3
align 4
convert_24_24_start:
        mov     ecx,ebx
        rep     movs dword ptr es:[edi],dword ptr ds:[esi]
        mov     ecx,edx
        rep     movs byte ptr es:[edi],byte ptr ds:[esi]

        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short convert_24_24_start
cEnd

;--------------------------------------------------------------------------;
;
;   convert_32_24
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_32_24,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
        ParmD   dst_ptr             ; --> dst.
        ParmD   dst_offset          ; offset to start at
        ParmD   dst_next_scan       ; dst_next_scan.
        ParmD   dst_fill_bytes      ; dst_fill_bytes
        ParmD   src_ptr             ; --> src.
        ParmD   src_offset          ; offset to start at
        ParmD   src_next_scan       ; dst_next_scan.
        ParmD   pel_count               ; pixel count.
        ParmD   scan_count              ; scan count.
        ParmD   xlat_table          ; pixel convert table.
cBegin
        xor     esi,esi
        xor     edi,edi
        lds     si,src_ptr
        les     di,dst_ptr

        add     esi,src_offset
        add     edi,dst_offset

        and     pel_count,not 3         ; !!!

        mov     eax,pel_count
        add     eax,eax
        add     eax,eax
        sub     src_next_scan,eax
        sub     eax,pel_count
        sub     dst_next_scan,eax

        mov     ecx,pel_count
        shr     ecx,2
        jz      short convert_32_24_exit
        mov     pel_count,ecx
align 4
convert_32_24_start:
        mov     ecx,pel_count
align 4
convert_32_24_loop:
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
        jnz     short convert_32_24_loop

convert_32_24_next:
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short convert_32_24_start

convert_32_24_exit:

cEnd

sEnd    CodeSeg

end
