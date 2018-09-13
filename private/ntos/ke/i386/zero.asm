        title  "Zero memory pages using fastest means available"
;++
;
; Copyright (c) 1998  Microsoft Corporation
;
; Module Name:
;
;    zero.asm
;
; Abstract:
;
;    Zero memory pages using the fastest means available.
;
; Author:
;
;    Peter Johnston (peterj) 20-Jun-1998.
;        Critical sections of Katmai code adapted from in-line
;        assembly version by Shiv Kaushik or Intel Corp.
;
; Environment:
;
;    x86
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc
include mac386.inc
        .list

;
; Register Definitions (for instruction macros).
;

rEAX            equ     0
rECX            equ     1
rEDX            equ     2
rEBX            equ     3
rESP            equ     4
rEBP            equ     5
rESI            equ     6
rEDI            equ     7

;
; Define SIMD instructions used in this module.
;

if 0

; these remain for reference only.   In theory the stuff following
; should generate the right code.

xorps_xmm0_xmm0 macro
                db      0FH, 057H, 0C0H
                endm

movntps_edx     macro   Offset
                db      0FH, 02BH, 042H, Offset
                endm

movaps_esp_xmm0 macro
                db      0FH, 029H, 004H, 024H
                endm

movaps_xmm0_esp macro
                db      0FH, 028H, 004H, 024H
                endm

endif

xorps           macro   XMMReg1, XMMReg2
                db      0FH, 057H, 0C0H + (XMMReg1 * 8) + XMMReg2
                endm

movntps         macro   GeneralReg, Offset, XMMReg
                db      0FH, 02BH, 040H + (XmmReg * 8) + GeneralReg, Offset
                endm

sfence          macro
                db      0FH, 0AEH, 0F8H
                endm

movaps_load     macro   XMMReg, GeneralReg
                db      0FH, 028H, (XMMReg * 8) + 4, (4 * 8) + GeneralReg
                endm

movaps_store    macro   GeneralReg, XMMReg
                db      0FH, 029H, (XMMReg * 8) + 4, (4 * 8) + GeneralReg
                endm


;
; NPX Save and Restore
;

fxsave          macro   Register
                db      0FH, 0AEH, Register
                endm

fxrstor         macro   Register
                db      0FH, 0AEH, 8+Register
                endm


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; KeZeroPage(
;    PageBase
;    )
;
; Routine Description:
;
;     KeZeroPage is really just a function pointer that points at
;     either KiZeroPage or KiXMMIZeroPage depending on whether or
;     not XMMI instructions are available.
;
; Arguments:
;
;     (ecx) PageBase    Base address of page to be zeroed.
;
;
; Return Value:
;
;--


        page    ,132
        subttl  "KiXMMIZeroPageNoSave - Use XMMI to zero memory (XMMI owned)"

;++
;
; VOID
; KiXMMIZeroPageNoSave (
;     IN PVOID PageBase
;     )
;
; Routine Description:
;
;     Use XMMI to zero a page of memory 16 bytes at a time while
;     at the same time minimizing cache polution.
;
;     Note: The XMMI register set belongs to this thread.  It is neither
;     saved nor restored by this procedure.
;
; Arguments:
;
;     (ecx) PageBase    Virtual address of the base of the page to be zeroed.
;
; Return Value:
;
;     None.
;
;--

INNER_LOOP_BYTES    equ 64

cPublicFastCall KiXMMIZeroPageNoSave,1
cPublicFpo 0, 1

        xorps   0, 0                            ; zero xmm0 (128 bits)
        mov     eax, PAGE_SIZE/INNER_LOOP_BYTES ; Number of Iterations

inner:

        movntps rECX, 0,  0                     ; store bytes  0 - 15
        movntps rECX, 16, 0                     ;             16 - 31
        movntps rECX, 32, 0                     ;             32 - 47
        movntps rECX, 48, 0                     ;             48 - 63

        add     ecx, 64                         ; increment base
        dec     eax                             ; decrement loop count
        jnz     short inner

        ; Force all stores to complete before any other
        ; stores from this processor.
        ;
        ; BUGBUG Does this mean we need an sfence on context switch?
        ; (I suspect yes if the processor owns the XMMI context - peterj).

        sfence

ifndef SFENCE_IS_NOT_BUSTED

        ; BUGBUG the next uncached write to this processor's apic 
        ; may fail unless the store pipes have drained.  sfence by
        ; itself is not enough.   Force drainage now by doing an
        ; interlocked exchange.

        xchg    [esp-4], eax

endif

        fstRET  KiXMMIZeroPageNoSave

fstENDP KiXMMIZeroPageNoSave


        page    ,132
        subttl  "KiXMMIZeroPage - Use XMMI to zero memory"

;++
;
; VOID
; KiXMMIZeroPage (
;     IN PVOID PageBase
;     )
;
; Routine Description:
;
;     Use XMMI to zero a page of memory 16 bytes at a time.  This
;     routine is a wrapper around KiXMMIZeroPageNoSave.  In this
;     case we don't have the luxury of not saving/restoring context.
;
; Arguments:
;
;     (ecx) PageBase    Virtual address of the base of the page to be zeroed.
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KiXMMIZeroPage,1
cPublicFpo 0, 2

        mov     eax, PCR[PcInitialStack]
        mov     edx, PCR[PcPrcbData+PbCurrentThread]
        push    ebp
        push    ebx
        mov     ebp, esp                        ; save stack pointer
        sub     esp, 16                         ; reserve space for xmm0
        and     esp, 0FFFFFFF0H                 ; 16 byte aligned
        cli                                     ; don't context switch
        test    [eax].FpCr0NpxState, CR0_EM     ; if FP explicitly disabled
        jnz     short kxzp90                    ; do it the old way
        cmp     byte ptr [edx].ThNpxState, NPX_STATE_LOADED
        je      short kxzp80                    ; jiff, NPX stated loaded

        ; NPX state is not loaded on this thread, it will be by
        ; the time we reenable context switching.

        mov     byte ptr [edx].ThNpxState, NPX_STATE_LOADED

        ; enable use of FP instructions

        mov     ebx, cr0
        and     ebx, NOT (CR0_MP+CR0_TS+CR0_EM)
        mov     cr0, ebx                        ; enable NPX

ifdef NT_UP

        ; if this is a UP machine, the state might be loaded for
        ; another thread in which case it needs to be saved.

        mov     ebx, PCR[PcPrcbData+PbNpxThread]; Owner of NPX state
        or      ebx, ebx                        ; NULL?
        jz      short @f                        ; yes, skip save.

        mov     byte ptr [ebx].ThNpxState, NPX_STATE_NOT_LOADED
        mov     ebx, [ebx].ThInitialStack       ; get address of save
        sub     ebx, NPX_FRAME_LENGTH           ; area.
        fxsave  rEBX                            ; save NPX
@@:

endif

        ; Now load the NPX context for this thread.  This is because
        ; if we switch away from this thread it will get saved again
        ; in this save area and destroying it would be bad.

        fxrstor rEAX

        mov     PCR[PcPrcbData+PbNpxThread], edx

kxzp80:
        sti                                     ; reenable context switching
        movaps_store rESP, 0                    ; save xmm0
        fstCall KiXMMIZeroPageNoSave            ; zero the page
        movaps_load  0, rESP                    ; restore xmm

        ; restore stack pointer, non-volatiles and return

        mov     esp, ebp
        pop     ebx
        pop     ebp
        fstRET  KiXMMIZeroPage


        ; FP is explicitly disabled for this thread (probably a VDM
        ; thread).  Restore stack pointer, non-volatiles and jump into
        ; KiZeroPage to do the work the old fashioned way.

kxzp90:
        sti
        mov     esp, ebp
        pop     ebx
        pop     ebp
        jmp     short @KiZeroPage@4

fstENDP KiXMMIZeroPage


        page    ,132
        subttl  "KiZeroPage - Available to all X86 processors"

;++
;
; KiZeroPage(
;     PVOID PageBase
;     )
;
; Routine Description:
;
;     Generic Zero Page routine, used on processors that don't have
;     a more effecient way to zero large blocks of memory.
;     (Same as RtlZeroMemory).
;
; Arguments:
;
;     (ecx) PageBase    Base address of page to be zeroed.
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KiZeroPage,1
cPublicFpo 0, 0

        push    edi                             ; save EDI (non-volatile)
        xor     eax, eax                        ; 32 bit zero
        mov     edi, ecx                        ; setup for repsto
        mov     ecx, PAGE_SIZE/4                ; iteration count

        ; store eax, ecx times starting at edi

        rep stosd

        pop     edi                             ; restore edi and return
        fstRET  KiZeroPage

fstENDP KiZeroPage


_TEXT   ends
        end
