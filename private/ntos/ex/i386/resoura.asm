        title "Resource package x86 optimzations"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;   resoura.asm
;
;Abstract:
;
;   Optimized preambles for some HOT resource code paths.
;
;   Takes first crack at satisfying the ExResource API, if it
;   can't it passes it onto the full blown API.
;
;   Optimized for UP free builds only!
;
;Author:
;
;    Ken Reneris (kenr) 13-Jan-1992
;
;Revision History:
;
;--
.386p
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc


        EXTRNP  _ExpReleaseResourceForThread,2

ExculsiveWaiter     equ     1           ; From ddkresrc.c
SharedWaiter        equ     2           ; From ddkresrc.c
AnyWaiter           equ     (ExculsiveWaiter+SharedWaiter)

_TEXT$00   SEGMENT  PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

ifdef NT_UP
ife DBG
;++
;
; VOID
; ExReleaseResourceForThread(
;     IN PNTDDK_ERESOURCE Resource,
;     IN ERESOURCE_THREAD OurThread
;     )
;
;
; Routine Description:
;
;     This routine release the input resource the indcated thread.  The
;     resource can have been acquired for either shared or exclusive access.
;
; Arguments:
;
;     Resource - Supplies the resource to release
;
;     Thread - Supplies the thread that originally acquired the resource
;
; Return Value:
;
;     None.
;
;--

cPublicProc _ExReleaseResourceForThread,2
cPublicFpo 2,0
        mov     ecx, [esp+4]                    ; resource
        mov     eax, [esp+8]                    ; our thread

        cli

        test    byte ptr [ecx].RsFlag, AnyWaiter ; any waiter?
        jnz     short rrt_longway_ddk            ; yes, go long

        test    byte ptr [ecx].RsFlag, RsOwnedExclusive ; exclusive?
        jz      short rrt_longway_ddk           ; bad release, go abort

        mov     edx, [ecx].RsOwnerThreads       ; (edx) = ownerthread table
        cmp     [edx], eax                      ; our thread at table[0]?
        jne     short rrt_longway_ddk           ; bad release, go abort

        mov     eax, [ecx].RsOwnerCounts
        dec     byte ptr [eax]                  ; thread count -= 1
        jnz     short rrt_exit_ddk              ; if not zero, all done

        mov     dword ptr [edx], 0              ; clear thread table[0]
        dec     word ptr [ecx].RsActiveCount    ; resource count -= 1
        and     byte ptr [ecx].RsFlag, not RsOwnedExclusive

rrt_exit_ddk:
        sti
        stdRET  _ExReleaseResourceForThread

rrt_longway_ddk:
        sti
        jmp     _ExpReleaseResourceForThread@8


stdENDP _ExReleaseResourceForThread

endif
endif

_TEXT$00   ends

end
