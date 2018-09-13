        externFP    GetSelectorBase         ; in KERNEL

sBegin Data
        externD flatBase
        externW flatSel
sEnd

;--------------------------------------------------------------------------;
;
;   MapFlat - convert a 16:16 pointer into a 0:32 pointer
;
;       note this macro assumes the memory will *not* move
;       while being accessed.  If this is not true call GlobalFix
;       on the memory
;
;   INPUT:
;       ptr16       16:16 pointer to map flat.
;       ptr32       put flat pointer here.
;
;   OUTPUT:
;       eax         flat pointer (or pointer stored)
;
;--------------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

MapFlat macro ptr16, ptr32

        ; is this call safe from a 32-bit seg?
        push    word ptr ptr16[2]

        db 66h,9Ah                      ; call <far ptr GetSelectorBase>
        dw OFFSET GetSelectorBase
        dw SEG    GetSelectorBase

        ror     eax,16                  ; xchg HIWORD(eax) and LOWORD(eax)
        shrd    eax,edx,16              ; move LOWORD(edx) to HIWORD(eax)

        movzx   edx,word ptr ptr16[0]   ; add pointer offset
        add     eax,edx
        sub     eax,[flatBase]          ; make relative to base offset

ifb <ptr32>
        mov     ptr16,eax
else
        mov     ptr32,eax
endif

endm

FlatDS macro
        mov     ds,[flatSel]
        mov     es,[flatSel]
endm
