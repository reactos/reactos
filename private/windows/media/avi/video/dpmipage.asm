        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  PAGELOCK
;
;   This module contains functions for page locking memory using DPMI
;
; Created:  03-20-90
; Author:   Todd Laney [ToddLa]
;
; Copyright (c) 1984-1990 Microsoft Corporation
;
; Exported Functions:	none
;
; Public Functions:     DpmiPageLock
;                       DpmiPageUnlock
;
; Public Data:          none
;
; General Description:
;
; Restrictions:
;
;-----------------------------------------------------------------------;

        ?PLM = 1
        ?WIN = 0
        ?NODATA = 1

        .286
        .xlist
        include cmacros.inc
        include int31.inc
        .list

        externA         __AHINCR                    ; KERNEL
        externFP        GlobalHandle                ; KERNEL
        externFP        GlobalHandleNoRip           ; KERNEL
        externFP        GlobalFix                   ; KERNEL
        externFP        GlobalUnFix                 ; KERNEL

ifndef SEGNAME
    SEGNAME equ <MSVIDEO>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

Int31_SelMgt_Get_Base     EQU ((Int31_Sel_Mgt shl 8) + SelMgt_Get_Base )
Int31_Lock_Region         EQU ((Int31_Page_Lock shl 8) + Lock_Region )
Int31_Unlock_Region       EQU ((Int31_Page_Lock shl 8) + Unlock_Region )

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

FARPOINTER      struc
off     dw      ?
sel     dw      ?
FARPOINTER      ends

sBegin CodeSeg
        assumes cs,CodeSeg
        assumes ds,nothing
        assumes es,nothing

;---------------------------Public-Routine------------------------------;
; DpmiPageLock
;
;   page lock a region using DPMI
;
; Entry:
;       lpBase      Selector:offset of base of region to lock
;       dwSize      size in bytes of region to lock
;
; Returns:
;       NZ
;       AX = TRUE if successful
;
; Error Returns:
;       Z
;       AX = FALSE if error
;
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       INT 31h
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   DpmiPageLock, <NEAR>, <>
;       parmD   lpBase
;       parmD   dwSize
cBegin  nogen
        mov     cx,Int31_Lock_Region
        jmp     short DpmiPageLockUnLock
cEnd    nogen

;---------------------------Public-Routine------------------------------;
; DpmiPageUnlock
;
;   un-page lock a region using DPMI
;
; Entry:
;       lpBase      Selector:offset of base of region to unlock
;       dwSize      size in bytes of region to unlock
;
; Returns:
;       NZ
;       AX = TRUE if successful
;
; Error Returns:
;       Z
;       AX = FALSE if error
;
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       INT 31h
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   DpmiPageUnlock, <NEAR>, <>
;       parmD   lpBase
;       parmD   dwSize
cBegin  nogen
        mov     cx,Int31_Unlock_Region
        errn$   DpmiPageLockUnLock
cEnd    nogen

cProc   DpmiPageLockUnLock, <NEAR>, <si,di>
        parmD   lpBase
        parmD   dwSize
cBegin
        mov     si,cx                       ; save lock/unlock flag

        mov     ax,Int31_SelMgt_Get_Base
        mov     bx,lpBase.sel
        int     31h                         ; returns CX:DX selector base
        jc      dpl_exit

        mov     bx,cx                       ; BX:CX is base
        mov     cx,dx

        add     cx,lpBase.off               ; add offset into selector base
        adc     bx,0

        mov     ax,si                       ; get lock/unlock flag
        mov     si,dwSize.hi                ; SI:DI length
        mov     di,dwSize.lo

        int     31h                         ; lock or unlock it
dpl_exit:
        cmc                                 ; set carry iff success
        sbb     ax,ax                       ; return TRUE/FALSE
cEnd

;---------------------------Public-Routine------------------------------;
; HugePageLock
;
;   page lock a range of windows allocated memory
;
; Entry:
;       lpBase      Selector:offset of base of region to lock
;       dwSize      size in bytes of region to lock
;
; Returns:
;       AX = TRUE if successful
;
; Error Returns:
;       AX = FALSE if error
;
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       INT 31h
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   HugePageLock, <FAR, PUBLIC>, <>
        parmD   lpBase
        parmD   dwSize
cBegin
        mov     ax,lpBase.sel               ; NULL pointer, invalid
        or      ax,ax
        jz      GPageLock_Exit

        call    HugeGlobalFix               ; fix the memory, then page lock
        cCall   DpmiPageLock,<lpBase, dwSize>
        jnz     GPageLock_Exit

        mov     ax,lpBase.sel               ; page lock failed, un-fix
        call    HugeGlobalUnFix             ; and return failure
        xor     ax,ax

GPageLock_Exit:
cEnd

;---------------------------Public-Routine------------------------------;
; HugePageUnlock
;
;   un-page lock a range of windows alocated memory, (locked with HugePageLock)
;
; Entry:
;       lpBase      Selector:offset of base of region to lock
;       dwSize      size in bytes of region to lock
;
; Returns:
;       none
;
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       INT 31h
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   HugePageUnlock, <FAR, PUBLIC>, <>
        parmD   lpBase
        parmD   dwSize
cBegin
        cCall   DpmiPageUnlock,<lpBase, dwSize>

        mov     ax,lpBase.sel
        call    HugeGlobalUnFix
cEnd

;---------------------------Public-Routine------------------------------;
; HugeGlobalFix
;
;   fix the global object that represents the passed selector
;
; Entry:
;       AX = SELECTOR
;
; Returns:
;       none
;
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       GlobalFix
;       HugeGlobalHandle
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

HugeGlobalFix proc near

        call    HugeGlobalHandle
        jz      HugeGlobalFixExit

        cCall   GlobalFix,<ax>

HugeGlobalFixExit:
        ret

HugeGlobalFix endp

;---------------------------Public-Routine------------------------------;
; HugeGlobalUnFix
;
;   un-fix the global object that represents the passed selector
;
; Entry:
;       AX = SELECTOR
;
; Returns:
;       none
;
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       HugeGlobalHandle
;       GlobalUnFix
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

HugeGlobalUnFix proc near

        call    HugeGlobalHandle
        jz      HugeGlobalUnFixExit

        cCall   GlobalUnFix,<ax>

HugeGlobalUnFixExit:
        ret

HugeGlobalUnFix endp

;---------------------------Public-Routine------------------------------;
; HugeGlobalHandle
;
; Entry:
;       AX = SELECTOR to global object
;
; Returns:
;       NZ
;       AX = HANDLE of global object
;
; Error Returns:
;       Z
;       AX = 0 if error
;
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;       GlobalHandleNoRip
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

HugeGlobalHandle proc near

        push    si
        mov     si,ax

        or      ax,ax                   ; test for NULL pointer!
        jz      HugeGlobalHandleExit

HugeGlobalHandleAgain:
        cCall   GlobalHandleNoRip,<si>
        sub     si,__AHINCR
        or      ax,ax
        jz      HugeGlobalHandleAgain

HugeGlobalHandleExit:
        pop     si
        ret

HugeGlobalHandle endp

sEnd

end
