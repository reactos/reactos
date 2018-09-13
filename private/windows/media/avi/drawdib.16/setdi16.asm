page    ,132
;----------------------------Module-Header------------------------------;
; Module Name: SETDI16.ASM
;
; move bits from one DIB format into another. doing color conversion if
; needed.
;
;   convert_8_16
;   convert_16_16
;   convert_24_16
;   convert_32_16
;
;   convert_8_565   (same as convert_8_16)
;   convert_16_565
;   convert_24_565
;   convert_32_565
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
createSeg %SEGNAME, CodeSeg, dword, public, CODE

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
;   convert_8_16
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   convert_8_16,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
        xor     ebx,ebx
        xor     edx,edx
        lfs     si,src_ptr
        les     di,dst_ptr
        lds     bx,xlat_table

        add     esi,src_offset
        add     edi,dst_offset

        mov     eax,pel_count
        sub     src_next_scan,eax
        add     eax,eax
        sub     dst_next_scan,eax

align 4
convert_8_16_start:
        mov     ecx,pel_count
        shr     ecx,2
        jz      short convert_8_16_ack
align 4
convert_8_16_loop:
        mov     eax,fs:[esi]        ; grab 4 pixels

        mov     dl,ah               ; get pel
        mov     bx,[edx+edx]        ; convert to 16bpp
        shl     ebx,16

        mov     dl,al               ; get pel
        mov     bx,[edx+edx]        ; convert to 16bpp
        mov     es:[edi],ebx        ; store 2 pels

        rol     eax,16

        mov     dl,ah               ; get pel
        mov     bx,[edx+edx]        ; convert to 16bpp
        rol     ebx,16

        mov     dl,al               ; get pel
        mov     bx,[edx+edx]        ; convert to 16bpp
        mov     es:[edi+4],ebx      ; store 2 pels

        add     esi,4
        add     edi,8
        dec     ecx
        jnz     short convert_8_16_loop

convert_8_16_ack:
        mov     ecx,pel_count
        and     ecx,3
        jnz     short convert_8_16_odd

convert_8_16_next:
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short convert_8_16_start
cEnd

convert_8_16_odd:
        mov     dl,fs:[esi]         ; get pel
        mov     bx,[edx+edx]        ; convert to 16bpp
        mov     es:[edi],bx         ; store pel
        inc     esi
        add     edi,2
        dec     ecx
        jnz     short convert_8_16_odd
        jz      short convert_8_16_next

;--------------------------------------------------------------------------;
;
;   convert_16_16
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_16_16,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
        sub     src_next_scan,eax
        sub     dst_next_scan,eax

        mov     ebx,eax
        mov     edx,eax
        shr     ebx,2
        and     edx,3
align 4
convert_16_16_start:
        mov     ecx,ebx
        rep     movs dword ptr es:[edi],dword ptr ds:[esi]
        mov     ecx,edx
        rep     movs byte ptr es:[edi],byte ptr ds:[esi]
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes
        dec     scan_count
        jnz     short convert_16_16_start
cEnd

;--------------------------------------------------------------------------;
;
;   convert_24_16
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_24_16,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
        sub     dst_next_scan,eax
        add     eax,pel_count
        sub     src_next_scan,eax

        mov     dl,0F8h
align 4
convert_24_16_start:
        mov     ecx,pel_count
align 4
convert_24_16_loop:
        mov     al,[esi+0]  ; get BLUE
        and     al,dl
        mov     bl,al
        mov     bh,[esi+1]  ; get GREEN
        shr     bh,3
        shr     ebx,3
        mov     al,[esi+2]  ; get RED
        and     al,dl
        shr     al,1
        or      bh,al

        mov     es:[edi],bx

        add     esi,3
        add     edi,2
        dec     ecx
        jnz     convert_24_16_loop

convert_24_16_next:
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     convert_24_16_start
cEnd

;--------------------------------------------------------------------------;
;
;   convert_32_16
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_32_16,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
        sub     dst_next_scan,eax
        add     eax,eax
        sub     src_next_scan,eax

        mov     dl,0F8h
align 4
convert_32_16_start:
        mov     ecx,pel_count
align 4
convert_32_16_loop:
        mov     al,[esi+0]  ; get BLUE
        and     al,dl
        mov     bl,al
        mov     bh,[esi+1]  ; get GREEN
        shr     bh,3
        shr     ebx,3
        mov     al,[esi+2]  ; get RED
        and     al,dl
        shr     al,1
        or      bh,al

        mov     es:[edi],bx

        add     esi,4
        add     edi,2
        dec     ecx
        jnz     short convert_32_16_loop

convert_32_16_next:
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short convert_32_16_start
cEnd

;--------------------------------------------------------------------------;
;
;   convert_8_565
;
;--------------------------------------------------------------------------;

public convert_8_565
convert_8_565 = convert_8_16

;--------------------------------------------------------------------------;
;
;   convert_16_565
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_16_565,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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

        and     pel_count,not 1     ;;!!!

        mov     eax,pel_count
        add     eax,eax
        sub     src_next_scan,eax
        sub     dst_next_scan,eax

        mov     ecx,pel_count
        shr     ecx,1
        jz      short convert_16_565_exit
        mov     pel_count,ecx

align 4
convert_16_565_start:
        mov     ecx,pel_count
align 4
convert_16_565_loop:
	mov	ebx,[esi]	;ebx=xRRRRRGGGGGBBBBBxRRRRRGGGGGBBBBB
	mov	eax,ebx		;eax=xRRRRRGGGGGBBBBBxRRRRRGGGGGBBBBB
	add	eax,eax		;eax=RRRRRGGGGGBBBBBxRRRRRGGGGGBBBBBx
	and	ebx,0001F001Fh	;ebx=00000000000BBBBB00000000000BBBBB
	and	eax,0FFC0FFC0h	;eax=RRRRRGGGGG000000RRRRRGGGGG000000
        or      eax,ebx         ;eax=RRRRRGGGGG0BBBBBRRRRRGGGGG0BBBBB

	mov	es:[edi],eax	;store both 565 pels

        add     esi,4
        add     edi,4
        dec     ecx
        jnz     short convert_16_565_loop

convert_16_565_next:
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short convert_16_565_start

convert_16_565_exit:
cEnd

;--------------------------------------------------------------------------;
;
;   convert_24_565
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_24_565,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
        sub     dst_next_scan,eax
        add     eax,pel_count
        sub     src_next_scan,eax

        mov     dl,0F8h
align 4
convert_24_565_start:
        mov     ecx,pel_count
align 4
convert_24_565_loop:
        mov     al,[esi+0]  ; get BLUE
        and     al,dl
        mov     bl,al
        mov     bh,[esi+1]  ; get GREEN
        shr     bh,2
        shr     ebx,3
        mov     al,[esi+2]  ; get RED
        and     al,dl
        or      bh,al

        mov     es:[edi],bx

        add     esi,3
        add     edi,2
        dec     ecx
        jnz     short convert_24_565_loop

convert_24_565_next:
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short convert_24_565_start
cEnd

;--------------------------------------------------------------------------;
;
;   convert_32_565
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_32_565,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
        sub     dst_next_scan,eax
        add     eax,eax
        sub     src_next_scan,eax

        mov     dl,0F8h
align 4
convert_32_565_start:
        mov     ecx,pel_count
align 4
convert_32_565_loop:
        mov     al,[esi+0]  ; get BLUE
        and     al,dl
        mov     bl,al
        mov     bh,[esi+1]  ; get GREEN
        shr     bh,2
        shr     ebx,3
        mov     al,[esi+2]  ; get RED
        and     al,dl
        or      bh,al

        mov     es:[edi],bx

        add     esi,4
        add     edi,2
        dec     ecx
        jnz     short convert_32_565_loop

convert_32_565_next:
        nxtscan si,src_next_scan
        nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short convert_32_565_start
cEnd

sEnd    CodeSeg

end
