        title  "Ldt Support 2 - Low Level"
;++
;
; Copyright (c) 1991  Microsoft Corporation
;
; Module Name:
;
;    ldtsup2.asm
;
; Abstract:
;
;    This module implements procedures to load a new ldt and to flush
;    segment descriptors.
;
; Author:
;
;    Bryan M. Willman (bryanwi)  14-May-1991
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include i386\kimacro.inc
include mac386.inc
include callconv.inc
        .list

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
;++
;
; VOID
; KiLoadLdtr(
;    VOID
;    )
;
; Routine Description:
;
;    This routine copies the Ldt descriptor image out of the currently
;    executing process object into the Ldt descriptor, and reloads the
;    the Ldt descriptor into the Ldtr.  The effect of this is to provide
;    a new Ldt.
;
;    If the Ldt descriptor image has a base or limit of 0, then NULL will
;    be loaded into the Ldtr, and no copy to the Gdt will be done.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiLoadLdtr, 0

        push    esi
        push    edi

        mov     eax,fs:PcPrcbData+PbCurrentThread   ; (eax)->CurrentThread
        mov     eax,[eax]+(ThApcState+AsProcess)    ; (eax)->CurrentProcess

        lea     esi,[eax]+PrLdtDescriptor           ; (esi)->Ldt value
        xor     dx,dx                               ; assume null value
        cmp     word ptr [esi],0                    ; limit == 0?
        jz      kill10                              ; yes limit 0, go load null

;
;   We have a non-null Ldt Descriptor, copy it into the Gdt
;

        mov     edi,fs:PcGdt
        add     edi,KGDT_LDT                        ; (edi)->Ldt descriptor

        movsd
        movsd                                       ; descrip. now matches value

        mov     dx,KGDT_LDT

kill10: lldt    dx

        pop     edi
        pop     esi

        stdCall   _KiFlushDescriptors

        stdRET    _KiLoadLdtr

stdENDP _KiLoadLdtr



;++
;
; VOID
; KiFlushDescriptors(
;    VOID
;    )
;
; Routine Description:
;
;    Flush the in-processor descriptor registers for the segment registers.
;    We do this by reloading each segment register.
;
;    N.B.
;
;       This procedure is only intended to support Ldt operations.
;       It does not support operations on the Gdt.  In particular,
;       neither it nor Ke386SetDescriptorProcess are appropriate for
;       editing descriptors used by 16bit kernel code (i.e. ABIOS.)
;
;       Since we are in kernel mode, we know that CS and SS do NOT
;       contain Ldt selectors, any such selectors will be save/restored
;       by the interrupt that brought us here from user space.
;
;       Since we are in kernel mode, DS must contain a flat GDT descriptor,
;       since all entry sequences would have forced a reference to it.
;
;       Since we are in kernel mode, FS points to the PCR, since all
;       entry sequences force it to.
;
;       Therefore, only ES and GS need to be flushed.
;
;       Since no inline kernel code ever uses GS, we know it will be
;       restored from a frame of some caller, or nobody cares.  Therefore,
;       we load null into GS.  (Fastest possible load.)
;
;       ES is restored to KGDT_R3_DATA, because kernel exit will not restore
;       it for us.  If we do not put the correct value in ES, we may wind
;       up with zero in ES in user mode.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiFlushDescriptors ,0

        xor     ax,ax
        mov     gs,ax
        push    ds
        pop     es
        stdRET    _KiFlushDescriptors

stdENDP _KiFlushDescriptors


_TEXT$00   ends
        end

