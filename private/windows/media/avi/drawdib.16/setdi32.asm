page    ,132
;----------------------------Module-Header------------------------------;
; Module Name: SETDI32.ASM
;
; move bits from one DIB format into another. doing color conversion if
; needed.
;
;   convert_8_32
;   convert_16_32
;   convert_24_32
;   convert_32_32
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
;   convert_8_32
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   convert_8_32,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
cEnd

;--------------------------------------------------------------------------;
;
;   convert_16_32
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_16_32,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
cEnd

;--------------------------------------------------------------------------;
;
;   convert_24_32
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_24_32,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
cEnd

;--------------------------------------------------------------------------;
;
;   convert_32_32
;
;--------------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing        

cProc   convert_32_32,<FAR,PUBLIC,PASCAL>,<esi,edi,ds>
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
cEnd

sEnd    CodeSeg

end
