page    ,132
;----------------------------Module-Header------------------------------;
; Module Name: SETDI8.ASM
;
; move bits from one DIB format into another. doing color conversion if
; needed.
;
;   convert_8_8
;   convert_16_8
;   convert_24_8
;   convert_32_8
;   copy_8_8
;   dither_8_8
;
; NOTES:
;
;  dither needs to work!
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
;   convert_8_8
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_8_8,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
        lfs     si,src_ptr
        les     di,dst_ptr
        lds     bx,xlat_table

        add     esi,src_offset
        add     edi,dst_offset

        mov     eax,pel_count
        sub     src_next_scan,eax
        sub     dst_next_scan,eax

	mov	edx,pel_count
	xor	ebx,ebx
align 4
convert_8_8_start:
	mov	ecx,edx ;pel_count
	shr	ecx,2
        jz      short convert_8_8_ack
align 4
convert_8_8_loop:
        mov     eax,fs:[esi]        ; grab 4 pixels

	mov	bl,al		    ; get pel
	mov	al,[ebx]	    ; translate pel
	mov	bl,ah		    ; get pel
	mov	ah,[ebx]	    ; translate pel

        rol     eax,16

	mov	bl,al		    ; get pel
	mov	al,[ebx]	    ; translate pel
	mov	bl,ah		    ; get pel
	mov	ah,[ebx]	    ; translate pel

        rol     eax,16
        mov     es:[edi],eax        ; store four

        add     esi,4
        add     edi,4
	dec	ecx
	jnz	short convert_8_8_loop

convert_8_8_ack:
	mov	ecx,edx ;pel_count
	and	ecx,3
	jnz	short convert_8_8_odd

convert_8_8_next:
	nxtscan si,src_next_scan
	nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short convert_8_8_start
cEnd

convert_8_8_odd:
@@:	mov	bl,fs:[esi]
	mov	bl,[ebx]
        mov     es:[edi],bl
        inc     esi
        inc     edi
	dec	ecx
	jnz	short convert_8_8_odd
        jz      short convert_8_8_next

;--------------------------------------------------------------------------;
;
;   copy_8_8
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   copy_8_8,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
        sub     src_next_scan,eax
        sub     dst_next_scan,eax

        mov     eax,src_next_scan
        or      eax,dst_next_scan
        or      eax,dst_fill_bytes
	jz	short copy_8_8_all

copy_8_8_rect:
        mov     ebx,pel_count
        mov     edx,ebx

        shr     ebx,2
        and     edx,3
align 4
copy_8_8_start:
        mov     ecx,ebx
        rep     movs dword ptr es:[edi], dword ptr ds:[esi]
        mov     ecx,edx
        rep     movs byte ptr es:[edi], byte ptr ds:[esi]
	nxtscan si,src_next_scan
	nxtscan di,dst_next_scan,dst_fill_bytes
        dec     scan_count
        jnz     short copy_8_8_start
copy_8_8_exit:
cEnd

copy_8_8_all:
        mov     eax,pel_count
        mul     scan_count
        mov     ecx,eax
        shr     ecx,2
        rep     movs dword ptr es:[edi], dword ptr ds:[esi]
        mov     ecx,eax
        and     ecx,3
        rep     movs byte ptr es:[edi], byte ptr ds:[esi]
        jmp     short copy_8_8_exit

;--------------------------------------------------------------------------;
;
;   dither_8_8
;
;       pel = xlat[y&7][pel][x&7]
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   dither_8_8,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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

ifdef DEBUG
        or      ebx,ebx
        jz      @f
        int 3
@@:
endif
        add     esi,src_offset
        add     edi,dst_offset

        and     pel_count,not 3     ;;!!! round down to multiple of 4

        mov     eax,pel_count
        sub     src_next_scan,eax
        sub     dst_next_scan,eax

        shr     eax,2
        jz      short dither_8_8_exit
        mov     pel_count,eax

        xor     edx,edx             ; y = 0
align 4
dither_8_8_start:
        mov     ecx,pel_count
        xor     ebx,ebx             ; x = 0
align 4
dither_8_8_loop:
        mov     eax,fs:[esi]        ; grab 4 pixels

        mov     dl,al               ; get pel
        mov     al,[edx*8+ebx]      ; get dithered version of the pixel.
        inc     bl

        mov     dl,ah               ; get pel
        mov     ah,[edx*8+ebx]      ; get dithered version of the pixel.
        inc     bl

        rol     eax,16

        mov     dl,al               ; get pel
        mov     al,[edx*8+ebx]      ; get dithered version of the pixel.
        inc     bl

        mov     dl,ah               ; get pel
        mov     ah,[edx*8+ebx]      ; get dithered version of the pixel.
        inc     bl
        and     bl,7

        rol     eax,16
        mov     es:[edi],eax

        add     esi,4
        add     edi,4
        dec     ecx
        jnz     short dither_8_8_loop

dither_8_8_next:
        inc     dh
        and     dh,7
	nxtscan si,src_next_scan
	nxtscan di,dst_next_scan,dst_fill_bytes

        dec     scan_count
        jnz     short dither_8_8_start

dither_8_8_exit:
cEnd

;--------------------------------------------------------------------------;
;
;   convert_16_8
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_16_8,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
        ParmD   dst_ptr             ; --> dst.
        ParmD   dst_offset          ; offset to start at
        ParmD   dst_next_scan       ; dst_next_scan.
        ParmD   dst_fill_bytes      ; dst_fill_bytes
        ParmD   src_ptr             ; --> src.
        ParmD   src_offset          ; offset to start at
        ParmD   src_next_scan       ; dst_next_scan.
	ParmD	pel_count	    ; pixel count.
	ParmD	scan_count	    ; scan count.
        ParmD   xlat_table          ; pixel convert table.
cBegin
	; we need dither code here!
cEnd

;--------------------------------------------------------------------------;
;
;   convert_24_8
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_24_8,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
	; we need dither code here!
cEnd

;--------------------------------------------------------------------------;
;
;   convert_32_8
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_32_8,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
	; we need dither code here!
cEnd

sEnd    CodeSeg

end
