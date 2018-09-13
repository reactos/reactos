page    ,132
;----------------------------Module-Header------------------------------;
; Module Name: FLAT.ASM
;
; init/term routines for using flat pointers
;
; LocalAlloc should go through here!
;
;-----------------------------------------------------------------------;
.286
?PLM=1
?WIN=0
	.xlist
        include cmacros.inc
        .list

;--------------------------------------------------------------------------;
;--------------------------------------------------------------------------;

sBegin Data
        public flatBase
        public flatSel
        flatSel             dw 0
        flatBase            dd 0
        KernelLocalAlloc    dd 0
sEnd

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        assumes cs,CodeSeg
        assumes ds,Data
        assumes es,nothing

        externFP    GetSelectorBase         ; in KERNEL
        externFP    SetSelectorBase         ; in KERNEL
        externFP    GetSelectorLimit        ; in KERNEL
        externFP    SetSelectorLimit        ; in KERNEL
        externFP    GlobalWire              ; in KERNEL
        externFP    GlobalUnwire            ; in KERNEL
        externFP    GlobalFix               ; in KERNEL
        externFP    GlobalUnfix             ; in KERNEL
;       externFP    KernelLocalAlloc        ; in KERNEL

        externFP    GetProcAddress          ; in KERNEL
        externFP    GetModuleHandle         ; in KERNEL

        externFP    AllocSelector           ; in KERNEL
        externFP    FreeSelector            ; in KERNEL

;--------------------------------------------------------------------------;
;
;   FlatInit
;
;--------------------------------------------------------------------------;

cProc   FlatInit, <FAR, PUBLIC, PASCAL>
cBegin
;       cCall   GlobalWire, <ds>
        cCall   GlobalFix, <ds>

        cCall   AllocSelector,<ds>
        mov     [flatSel],ax

        mov     bx,ax
        mov     ax,0008h   ; DPMI Set Limit
        mov     cx,-1
        mov     dx,-1
        int     31h

;       cCall   SetSelectorLimit,<ax,-1,-1>

        cCall   GetSelectorBase, <ds>
        mov     word ptr flatBase[0],ax
        mov     word ptr flatBase[2],dx
cEnd

;--------------------------------------------------------------------------;
;
;   FlatTerm
;
;--------------------------------------------------------------------------;

cProc   FlatTerm, <FAR, PUBLIC, PASCAL>
cBegin
        cCall   SetSelectorLimit,<[flatSel],0,0>
        cCall   FreeSelector,<[flatSel]>

;       cCall   GlobalUnwire, <ds>
        cCall   GlobalUnfix, <ds>
	mov	word ptr [flatBase][0],0
	mov	word ptr [flatBase][2],0
        mov     [flatSel],0
cEnd

;--------------------------------------------------------------------------;
;
;   LocalAlloc
;
;--------------------------------------------------------------------------;

szKernel: db  "KERNEL",0

cProc	LocalAlloc, <FAR, PUBLIC, PASCAL>
        ParmW   flags
        ParmW   bytes
cBegin
        mov     ax,word ptr [KernelLocalAlloc][0]
        or      ax,word ptr [KernelLocalAlloc][2]
        jnz     @f

        lea     ax,szKernel
        cCall   GetModuleHandle,<cs,ax>
        cCall   GetProcAddress,<ax, 0, 5>

        mov     word ptr [KernelLocalAlloc][0],ax
        mov     word ptr [KernelLocalAlloc][2],dx

@@:     mov     ax,word ptr [flatBase][0]
	or	ax,word ptr [flatBase][2]
	pushf
	jz	@f
        cCall   FlatTerm
@@:	cCall	KernelLocalAlloc, <flags, bytes>
	popf
	jz	@f
	push	ax
        cCall   FlatInit
	pop	ax
@@:
cEnd

;--------------------------------------------------------------------------;
;
;   MapFlat - convert a 16:16 pointer into a 0:32 pointer
;
;       note this function assumes the memory will *not* move
;       while being accessed.  If this is not true call GlobalFix
;       on the memory
;
;   INPUT:
;       ptr16       16:16 pointer to map flat.
;
;   OUTPUT:
;       dx:ax       flat pointer
;
;--------------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

cProc MapFlat, <NEAR>
        ParmD   ptr16
cBegin
        mov     ds,[flatSel]

        push    word ptr ptr16[2]
        cCall   GetSelectorBase

        add     ax,word ptr ptr16[0]
        adc     dx,0

        sub     ax,word ptr flatBase[0]
        sbb     dx,word ptr flatBase[2]
cEnd

sEnd

end
