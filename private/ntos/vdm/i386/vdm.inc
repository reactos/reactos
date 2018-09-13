;++
;
;  Copyright (c) 1989  Microsoft Corporation
;
;  Module Name:
;
;     vdm.inc
;
;  Abstract:
;
;     This module contains the structure and constant definitions for
;     the vdm component
;
;  Author:
;
;     Dave Hastings (daveh) 20-Feb-1992
;
;  Revision History
;     Sudeepb (30-Nov-1992)
;     Merged all the equates in one place
;
;--
.386p

TRUE                    equ 1
FALSE                   equ 0

;
; Macro's to only do locked memory operations on MP systems
;

IFDEF NT_UP
MPLOCK equ <>
ELSE
MPLOCK equ <lock>
ENDIF

        page , 132
        subttl "Macro to dispatch exception"

;++
;
; Macro Description:
;
;    This macro allocates exception record on stack, sets up exception
;    record using specified parameters and finally sets up arguments
;    and calls _KiDispatchException.
;
; Arguments:
;
;    ExcepCode - Exception code to put into exception record
;    ExceptFlags - Exception flags to put into exception record
;    ExceptRecord - Associated exception record
;    ExceptAddress - Addr of instruction which the hardware exception occurs
;    NumParms - Number of additional parameters
;    ParameterList - the additional parameter list
;
; Return Value:
;
;    None.
;
;--

DISPATCH_EXCEPTION macro ExceptCode, ExceptFlags, ExceptRecord, ExceptAddress,\
                         NumParms, ParameterList
        local de10, de20

; Set up exception record for raising exception

?i      =       0
        sub     esp, ExceptionRecordSize + NumParms * 4
                                        ; allocate exception record
        mov     dword ptr [esp]+ErExceptionCode, ExceptCode
                                        ; set up exception code
        mov     dword ptr [esp]+ErExceptionFlags, ExceptFlags
                                        ; set exception flags
        mov     dword ptr [esp]+ErExceptionRecord, ExceptRecord
                                        ; set associated exception record
        mov     dword ptr [esp]+ErExceptionAddress, ExceptAddress
        mov     dword ptr [esp]+ErNumberParameters, NumParms
                                        ; set number of parameters
        IRP     z, <ParameterList>
        mov     dword ptr [esp]+(ErExceptionInformation+?i*4), z
?i      =       ?i + 1
        ENDM

; set up arguments and call _KiDispatchException

        mov     ecx, esp                ; (ecx)->exception record

        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jz      de10

        mov     eax,0FFFFh
        jmp     de20

de10:   mov     eax,[ebp]+TsSegCs
de20:   and     eax,MODE_MASK

; 1 - set first chance TRUE
; eax - PreviousMode
; ebp - trap frame addr
; 0 - Null exception frame
; ecx - exception record addr

; dispatchexception as appropriate
        stdCall _KiDispatchException, <ecx, 0, ebp, eax, 1>
IFDEF STD_CALL
        add     esp,ExceptionRecordSize + NumParms*4
ELSE
        add     esp,20 + ExceptionRecordSize + NumParms*4
ENDIF

        ENDM


;
; Prefix Flags
;

PREFIX_ES               equ 00000100h
PREFIX_CS               equ 00000200h
PREFIX_SS               equ 00000400h
PREFIX_DS               equ 00000800h
PREFIX_FS               equ 00001000h
PREFIX_GS               equ 00002000h
PREFIX_OPER32           equ 00004000h
PREFIX_ADDR32           equ 00008000h
PREFIX_LOCK             equ 00010000h
PREFIX_REPNE            equ 00020000h
PREFIX_REP              equ 00040000h
PREFIX_SEG_ALL          equ 00003f00h

NUM_OPCODE              equ 255
;
; Reginfo structure
;

RegInfo struc
RiSegSs         dd 0
RiEsp           dd 0
RiEFlags        dd 0
RiSegCs         dd 0
RiEip           dd 0
RiTrapFrame     dd 0
RiCsLimit       dd 0
RiCsBase        dd 0
RiCsFlags       dd 0
RiSsLimit       dd 0
RiSsBase        dd 0
RiSsFlags       dd 0
RiPrefixFlags   dd 0
RiOperand       dd 0
RegInfo ends
REGINFOSIZE     EQU 56

MAX_VDM_ADDR    EQU 0FFFFFFH

DR7_GLOBAL      EQU 002AAH

;
;Fast Read/Write Defines
;

SVC_DEMFASTREAD     equ 42h
SVC_DEMFASTWRITE    equ 43h
DOS_BOP             equ 50h

;
;
; Size of Exception Record
;

ExceptionRecordSize = (ErNumberParameters + 4 + 3) AND 0fffffffcH ;

;
; Magic value
;

OPCODE_MAGIC    equ  00f5ah

;++
;       diBEGIN - BEGIN index table
;
;       ENTRY   name = name of di table
;               badindex = local label for unspecified entries
;
;       EXIT    ?sindex = specific entry index
;               ?badindex = default index for unspecified entries
;--

diBEGIN macro   name,badindex           ;; Beginning of dispatch table
        ?sopc  = 0
        ?badindex = badindex
name    label   byte                    ;; Start of dispatch table
endm

;++
;       dtI - SPECIFIC index table entry
;
;       ENTRY   opc = opcode value
;               index  = INDEX_ value corresponding to opcode
;               ?badindex = unspecified entry index
;               ?sopc = specific entry index
;
;       EXIT    Unspecified entries prior to "index" filled in;
;               Specified entry filled in.
;               ?sopc = next index to fill in;
;--

dtI     macro   opc,index               ;; Specific entry in dispatch table
        if ?sopc gt opc
            %out dtI: opcode out of order
            .err
        else
            if ?sopc lt opc             ;; Fill unspecified entries
                rept opc - ?sopc
                   db   ?badindex
                endm
            endif
            db    index                 ;; Specified entry
            ?sopc = opc+1               ;; Set new start index
        endif
endm

;++
;       diEND - END index table
;
;       ENTRY   index = highest entry in table
;               ?sopc = specific entry index
;
;       EXIT    rest of the table filled in
;--

diEND   macro   index                   ;; End of dispatch table
        if ?sopc lt index               ;; Fill in rest of table
            rept index - ?sopc
                db    ?badindex
            endm
        db      ?badindex               ;; fill in last entry of table!
        endif
endm


;++
;       dtBEGIN - BEGIN dispatch table
;
;       ENTRY   name = name of dt table
;               badaddr = local label for unspecified entries
;
;       EXIT    ?sindex = specific entry index
;               ?badaddr = default handler for unspecified entries
;--

dtBEGIN macro   name,badaddr            ;; Beginning of dispatch table
        ?sindex  = 0
        ?badaddr = badaddr
name    label   dword                   ;; Start of dispatch table
endm

;++
;       dtS - SPECIFIC dispatch table entry
;
;       ENTRY   index = index of entry
;               addr  = address of handler
;               ?badaddr = unspecified entry handler
;               ?sindex = specific entry index
;
;       EXIT    Unspecified entries prior to "index" filled in;
;               Specified entry filled in.
;               ?sindex = next index to fill in;
;--

dtS     macro   index,addr              ;; Specific entry in dispatch table
        if ?sindex gt index
            %out dtS: index out of order
            .err
        else
            if ?sindex lt index         ;; Fill unspecified entries
                rept index - ?sindex
                   dd   offset FLAT:?badaddr
                endm
            endif
            dd    offset FLAT:addr      ;; Specified entry
            ?sindex = index+1           ;; Set new start index
        endif
endm

;++
;       dtEND - END dispatch table
;
;       ENTRY   index = highest entry in table
;               ?sindex = specific entry index
;
;       EXIT    rest of the table filled in
;--

dtEND   macro   index                   ;; End of dispatch table
        if ?sindex lt index             ;; Fill in rest of table
            rept index - ?sindex
                dd    offset FLAT:?badaddr
            endm
        dd offset FLAT:?badaddr         ;; fill in last entry of table!
        endif
endm

;++
;       CsToLinearPM
;
;--

CsToLinearPM    macro sel, erraddr
        lea     eax,[esi].RiCsLimit
        push    eax
        lea     eax,[esi].RiCsBase
        push    eax
        lea     eax,[esi].RiCsFlags
        push    eax
        push    sel

IFDEF STD_CALL
        call    _Ki386GetSelectorParameters@16
ELSE
        call    _Ki386GetSelectorParameters
        add     esp,10h
ENDIF
        or      al,al
        jz      erraddr

        test    [esi].RiCsFlags,SEL_TYPE_EXECUTE
        jz      erraddr

        test    [esi].RiCsFlags,SEL_TYPE_2GIG
        jz      @f

        ; Correct limit value for granularity
        shl     [esi].RiCsLimit,12
        or      [esi].RiCsLimit,0FFFh
@@:
endm

;++
;       CsToLinearV86
;
;--

CsToLinearV86 macro
        movzx   eax,word ptr [esi].RiSegCs
        shl     eax,4
	mov	[esi].RiCsBase,eax
        mov     [esi].RiCsLimit,0FFFFh
        mov     [esi].RiCsFlags,0
endm
