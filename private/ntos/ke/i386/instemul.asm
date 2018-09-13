        title  "Vdm Instuction Emulation"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    instemul.asm
;
; Abstract:
;
;    This module contains the routines for emulating instructions and
;    faults to a VDM.
;
; Author:
;
;   Dave Hastings (daveh) 29-March-1991
;
; Environment:
;
;    Kernel mode only.
;
; Notes:
;
;
;sudeepb 09-Dec-1992 Very Sonn this file will be deleted and protected
;                    mode instruction emulation will be merged in
;                    emv86.asm. Particularly following routines will
;                    simply become OpcodeInvalid.
;               OpcodeIret
;               OpcodePushf
;               OpcodePopf
;               OpcodeHlt
;                    Other routines such as
;               OpcodeCli
;               OpcodeSti
;               OpcodeIN/OUT/SB/Immb etc
;                    will map exactly like emv86.asm
;               OpcodeInt will be the main differeing routine.
;
;               OpcodeDispatch Table will be deleted.
;
;       So before making any major changes in this file please see
;       Sudeepb or Daveh.
;
;neilsa 19-Oct-1993 Size and performance enhancements
;jonle 15-Nov-1993 - The Debug messages for each opcode may no longer work
;             correctly, because interrupts may not have been enabled
;
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include i386\kimacro.inc
include mac386.inc
include i386\mi.inc
include callconv.inc
include ..\..\vdm\i386\vdm.inc
include ..\..\..\inc\vdmtib.inc
        .list

        extrn   VdmOpcode0f:proc
        extrn   OpcodeNPXV86:proc
        extrn   VdmDispatchIntAck:proc   ;; only OpcodeSti uses this
ifdef VDMDBG
        EXTRNP  _VdmTraceEvent,4
endif
        extrn   CommonDispatchException:proc ;; trap.asm
        extrn   _DbgPrint:proc
        extrn   _KeI386VdmIoplAllowed:dword
        extrn   _KeI386VirtualIntExtensions:dword
        EXTRNP  _KeBugCheck,1
        EXTRNP  _Ki386GetSelectorParameters,4
        EXTRNP  _Ki386VdmDispatchIo,5
        EXTRNP  _Ki386VdmDispatchStringIo,8
        EXTRNP  _KiDispatchException,5
        EXTRNP  _VdmPrinterStatus,3
        EXTRNP  KfLowerIrql,1,IMPORT, FASTCALL
        EXTRNP  _VdmPrinterWriteData, 3

; JAPAN - SUPPORT Intel CPU/Non PC/AT machine
        extrn   _VdmFixedStateLinear:dword
        extrn   _KeI386MachineType:dword

        page ,132

ifdef VDMDBG
%out Debugging version
endif

;
;   Force assume into place
;

_PAGE   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING
_PAGE   ENDS

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING
_TEXT$00   ENDS

_DATA   SEGMENT  DWORD PUBLIC 'DATA'


;
;  Instruction emulation emulates the following instructions.
;  The emulation affects the noted user mode registers.
;
;  In protected mode, the following instructions are emulated in the kernel
;
;    Registers  (E)Flags (E)SP  SS  CS
;       INTnn      X       X     X   X
;       INTO       X       X     X   X
;       CLI        X
;       STI        X
;
;  The following instructions are always emulated by reflection to the
;  Usermode VDM monitor
;
;       INSB
;       INSW
;       OUTSB
;       OUTSW
;       INBimm
;       INWimm
;       OUTBimm
;       OUTWimm
;       INB
;       INW
;       OUTB
;       OUTW
;
;  WARNING What do we do about 32 bit io instructions??


;
;       OpcodeIndex - packed 1st level table to index OpcodeDispatch table
;
        public OpcodeIndex
diBEGIN OpcodeIndex,VDM_INDEX_Invalid
        dtI      0fh, VDM_INDEX_0F
        dtI      26h, VDM_INDEX_ESPrefix
        dtI      2eh, VDM_INDEX_CSPrefix
        dtI      36h, VDM_INDEX_SSPrefix
        dtI      3eh, VDM_INDEX_DSPrefix
        dtI      64h, VDM_INDEX_FSPrefix
        dtI      65h, VDM_INDEX_GSPrefix
        dtI      66h, VDM_INDEX_OPER32Prefix
        dtI      67h, VDM_INDEX_ADDR32Prefix
        dtI      6ch, VDM_INDEX_INSB
        dtI      6dh, VDM_INDEX_INSW
        dtI      6eh, VDM_INDEX_OUTSB
        dtI      6fh, VDM_INDEX_OUTSW
        dtI      9bh, VDM_INDEX_NPX
        dtI      9ch, VDM_INDEX_PUSHF
        dtI      9dh, VDM_INDEX_POPF
        dtI     0cdh, VDM_INDEX_INTnn
        dtI     0ceh, VDM_INDEX_INTO
        dtI     0cfh, VDM_INDEX_IRET
        dtI     0d8h, VDM_INDEX_NPX
        dtI     0d9h, VDM_INDEX_NPX
        dtI     0dah, VDM_INDEX_NPX
        dtI     0dbh, VDM_INDEX_NPX
        dtI     0dch, VDM_INDEX_NPX
        dtI     0ddh, VDM_INDEX_NPX
        dtI     0deh, VDM_INDEX_NPX
        dtI     0dfh, VDM_INDEX_NPX
        dtI     0e4h, VDM_INDEX_INBimm
        dtI     0e5h, VDM_INDEX_INWimm
        dtI     0e6h, VDM_INDEX_OUTBimm
        dtI     0e7h, VDM_INDEX_OUTWimm
        dtI     0ech, VDM_INDEX_INB
        dtI     0edh, VDM_INDEX_INW
        dtI     0eeh, VDM_INDEX_OUTB
        dtI     0efh, VDM_INDEX_OUTW
        dtI     0f0h, VDM_INDEX_LOCKPrefix
        dtI     0f2h, VDM_INDEX_REPNEPrefix
        dtI     0f3h, VDM_INDEX_REPPrefix
        dtI     0f4h, VDM_INDEX_HLT
        dtI     0fah, VDM_INDEX_CLI
        dtI     0fbh, VDM_INDEX_STI
diEND   NUM_OPCODE

;
;       OpcodeDispatch - table of routines used to emulate instructions
;

        public OpcodeDispatch
dtBEGIN OpcodeDispatch,OpcodeInvalid
        dtS     VDM_INDEX_0F          , Opcode0F
        dtS     VDM_INDEX_ESPrefix    , OpcodeESPrefix
        dtS     VDM_INDEX_CSPrefix    , OpcodeCSPrefix
        dtS     VDM_INDEX_SSPrefix    , OpcodeSSPrefix
        dtS     VDM_INDEX_DSPrefix    , OpcodeDSPrefix
        dtS     VDM_INDEX_FSPrefix    , OpcodeFSPrefix
        dtS     VDM_INDEX_GSPrefix    , OpcodeGSPrefix
        dtS     VDM_INDEX_OPER32Prefix, OpcodeOPER32Prefix
        dtS     VDM_INDEX_ADDR32Prefix, OpcodeADDR32Prefix
        dtS     VDM_INDEX_INSB        , OpcodeINSB
        dtS     VDM_INDEX_INSW        , OpcodeINSW
        dtS     VDM_INDEX_OUTSB       , OpcodeOUTSB
        dtS     VDM_INDEX_OUTSW       , OpcodeOUTSW
        dtS     VDM_INDEX_INTnn       , OpcodeINTnn
        dtS     VDM_INDEX_INTO        , OpcodeINTO
        dtS     VDM_INDEX_INBimm      , OpcodeINBimm
        dtS     VDM_INDEX_INWimm      , OpcodeINWimm
        dtS     VDM_INDEX_OUTBimm     , OpcodeOUTBimm
        dtS     VDM_INDEX_OUTWimm     , OpcodeOUTWimm
        dtS     VDM_INDEX_INB         , OpcodeINB
        dtS     VDM_INDEX_INW         , OpcodeINW
        dtS     VDM_INDEX_OUTB        , OpcodeOUTB
        dtS     VDM_INDEX_OUTW        , OpcodeOUTW
        dtS     VDM_INDEX_LOCKPrefix  , OpcodeLOCKPrefix
        dtS     VDM_INDEX_REPNEPrefix , OpcodeREPNEPrefix
        dtS     VDM_INDEX_REPPrefix   , OpcodeREPPrefix
        dtS     VDM_INDEX_CLI         , OpcodeCLI
        dtS     VDM_INDEX_STI         , OpcodeSTI
dtEND   MAX_VDM_INDEX

        public  _ExVdmOpcodeDispatchCounts,_ExVdmSegmentNotPresent
_ExVdmOpcodeDispatchCounts dd      MAX_VDM_INDEX dup(0)
_ExVdmSegmentNotPresent    dd      0

_DATA   ENDS

_PAGE   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

        page   ,132
        subttl "Overide Prefix Macro"
;++
;
;   Routine Description:
;
;       This macro generates the code for handling override prefixes
;       The routine name generated is OpcodeXXXXPrefix, where XXXX is
;       the name used in the macro invocation.  The code will set the
;       PREFIX_XXXX bit in the Prefix flags.
;
;   Arguments
;       name = name of prefix
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns
;       user mode Eip advanced
;       eax advanced
;       edx contains next byte of opcode
;
;   NOTE: This routine exits by dispatching through the table again.
;--
opPrefix macro name
        public Opcode&name&Prefix
Opcode&name&Prefix proc

        or      [esi].RiPrefixFlags,PREFIX_&name
        jmp     OpcodeGenericPrefix     ; dispatch to next handler

Opcode&name&Prefix endp
endm

irp prefix, <ES, CS, SS, DS, FS, GS, OPER32, ADDR32, LOCK, REPNE, REP>

        opPrefix prefix

endm

        page   ,132
        subttl "Instruction Emulation Dispatcher"
;++
;
;   Routine Description:
;
;       This routine dispatches to the opcode specific emulation routine,
;       based on the first byte of the opcode.  Two byte opcodes, and prefixes
;       result in another level of dispatching, from the handling routine.
;
;   Arguments:
;
;       ebp = pointer to trap frame
;
;   Returns:
;
;       Nothing
;
;

cPublicProc _Ki386DispatchOpcode,0

        sub     esp,REGINFOSIZE
        mov     esi, esp                        ; scratch area

        CsToLinearPM [ebp].TsSegCs, doerr       ; initialize reginfo

        mov     edi,[ebp].TsEip                 ; get fault instruction address
        cmp     edi,[esi].RiCsLimit             ; check eip
        ja      doerr

        add     edi,[esi].RiCsBase
        movzx   ecx,byte ptr [edi]              ; get faulting opcode

        mov     eax,ecx
        and     eax,0F8h                                ; check for npx instr
        cmp     eax,0D8h
        je      do30                                    ; dispatch

        movzx   eax, OpcodeIndex[ecx]
        mov     ebx,1                           ; length count, flags

        ; All handler routines will get the following on entry
        ; ebp -> trap frame
        ; ebx -> prefix flags, instruction length count
        ; ecx -> byte at the faulting address
        ; edx -> pointer to vdm state in DOS arena
        ; interrupts enabled and Irql at APC level
        ; edi -> address of faulting instruction
        ; esi -> reginfo struct
        ; All handler routines will return
        ; EAX = 0 for failure
        ; EAX = 1 for success
if DEVL
        inc     _ExVdmOpcodeDispatchCounts[eax * type _ExVdmOpcodeDispatchCounts]
endif
ifdef VDMDBG
        pushad
        stdCall _VdmTraceEvent, <VDMTR_KERNEL_OP_PM,ecx,0,ebp>
        popad
endif

        call    OpcodeDispatch[eax * type OpcodeDispatch]
do20:
        add     esp,REGINFOSIZE
        stdRET  _Ki386DispatchOpcode

doerr:  xor     eax,eax
        jmp     do20

        ;
        ; If we get here, we have executed an NPX instruction in user mode
        ; with the emulator installed.  If the EM bit was not set in CR0, the
        ; app really wanted to execute the instruction for detection purposes.
        ; In this case, we need to clear the TS bit, and restart the instruction.
        ; Otherwise we need to reflect the exception
        ;
do30:
        call OpcodeNPXV86
        jmp  short do20

stdENDP _Ki386DispatchOpcode


        page   ,132
        subttl "Invalid Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine causes a GP fault to be reflected to the vdm
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeInvalid
OpcodeInvalid proc
        xor     eax,eax                 ; ret fail
        ret

OpcodeInvalid endp


        page   ,132
        subttl "Generic Prefix Handler"
;++
;
;   Routine Description:
;
;       This routine handles the generic portion of all of the prefixes,
;       and dispatches the next byte of the opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       user mode Eip advanced
;       edx contains next byte of opcode
;

        public OpcodeGenericPrefix
OpcodeGenericPrefix proc

        inc     edi                             ; increment eip
        inc     ebx                             ; increment size
        cmp     bl, 128                         ; set arbitrary inst size limit
        ja      ogperr                          ; in case of pointless prefixes

        mov     eax,edi                         ; current linear address
        sub     eax,[esi].RiCsBase              ; make address eip
        cmp     eax,[esi].RiCsLimit             ; check eip
        ja      ogperr

        mov     cl,byte ptr [edi]               ; get next opcode

        movzx   eax, OpcodeIndex[ecx]
if DEVL
        inc     _ExVdmOpcodeDispatchCounts[eax * type _ExVdmOpcodeDispatchCounts]
endif
        jmp     OpcodeDispatch[eax * type OpcodeDispatch]

ogperr:
        xor     eax,eax             ; opcode was NOT handled
        ret

OpcodeGenericPrefix endp


        page   ,132
        subttl "0F Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates a 0Fh opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public Opcode0F
Opcode0F proc

        mov     eax,[ebp].TsEip                 ; get fault instruction address
        mov     [esi].RiEip,eax
        mov     [esi].RiTrapFrame,ebp
        mov     [esi].RiPrefixFlags,ebx
        mov     eax,dword ptr [ebp].TsEFlags
        mov     [esi].RiEFlags,eax

        call    VdmOpcode0F                     ; enables interrupts
        test    eax,0FFFFh
        jz      o0f20

        mov     eax,[esi].RiEip
        mov     [ebp].TsEip,eax
        mov     eax,1
o0f20:
        ret

Opcode0F endp

        page   ,132
        subttl "Byte string in Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INSB opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;
;  WARNING what to do about size override?  ds override?

        public OpcodeINSB
OpcodeINSB proc

        push    ebp                          ; Trap Frame
        push    ebx                          ; size of insb

        movzx   eax,word ptr [ebp].TsSegEs
        shl     eax,16
        ; WARNING no support for 32bit edi
        mov     ax,word ptr [ebp].TsEdi      ; don't support 32bit'ness
        push    eax                          ; address

        xor     eax, eax
        mov     ecx,1
        test    ebx,PREFIX_REP
        jz      @f

        mov     eax, 1
        ; WARNING no support for 32bit ecx
        movzx   ecx,word ptr [ebp].TsEcx
@@:

        push    ecx                          ; number of io ops
        push    TRUE                         ; read op
        push    eax                          ; REP prefix
        push    1                            ; byte op
        movzx   edx,word ptr [ebp].TsEdx
        push    edx                          ; port number
        call    _Ki386VdmDispatchStringIo@32 ; use retval

        ret

OpcodeINSB endp

        page   ,132
        subttl "Word String In Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INSW opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeINSW
OpcodeINSW proc

        push    ebp                             ; Trap frame
        push    ebx                             ; sizeof insw

        movzx   eax,word ptr [ebp].TsSegEs
        shl     eax,16
        ; WARNING no support for 32bit edi
        mov     ax,word ptr [ebp].TsEdi
        push    eax                             ; address

        xor     eax, eax
        mov     ecx,1
        test    ebx,PREFIX_REP
        jz      @f

        mov     eax, 1
        ; WARNING no support for 32bit ecx
        movzx   ecx,word ptr [ebp].TsEcx
@@:
        movzx   edx,word ptr [ebp].TsEdx
        push    ecx                             ; number of io ops
        push    TRUE                            ; read op
        push    eax                             ; REP prefix
        push    2                               ; word size
        push    edx                             ; port number
        call    _Ki386VdmDispatchStringIo@32 ; use retval

        ret

OpcodeINSW endp

        page   ,132
        subttl "Byte String Out Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an OUTSB opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeOUTSB
OpcodeOUTSB proc

        push    ebp                           ; Trap Frame
        push    ebx                           ; size of outsb

        movzx   eax,word ptr [ebp].TsSegDs
        shl     eax,16
        ; WARNING don't support 32bit'ness, esi
        mov     ax,word ptr [ebp].TsEsi
        push    eax                           ; address

        xor     eax, eax
        mov     ecx,1
        test    ebx,PREFIX_REP
        jz      @f

        mov     eax, 1
        ; WARNING don't support 32bit'ness ecx
        movzx   ecx,word ptr [ebp].TsEcx
@@:
        movzx   edx,word ptr [ebp].TsEdx
        push    ecx                           ; number of io ops
        push    FALSE                         ; write op
        push    eax                           ; REP prefix
        push    1                             ; byte op
        push    edx                           ; port number
        call    _Ki386VdmDispatchStringIo@32 ; use retval

        ret

OpcodeOUTSB endp

        page   ,132
        subttl "Word String Out Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an OUTSW opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeOUTSW
OpcodeOUTSW proc

        push    ebp                               ; Trap Frame
        push    ebx                               ; size of outsb

        movzx   eax,word ptr [ebp].TsSegDs
        shl     eax,16
        ; WARNING don't support 32bit'ness esi
        mov     ax,word ptr [ebp].TsEsi
        push    eax                               ; address

        xor     eax, eax
        mov     ecx,1
        test    ebx,PREFIX_REP
        jz      @f

        mov     eax, 1
        ; WARNING don't support 32bit'ness ecx
        movzx   ecx,word ptr [ebp].TsEcx
@@:
        movzx   edx,word ptr [ebp].TsEdx

        push    ecx                               ; number of io ops
        push    FALSE                             ; write op
        push    eax                               ; REP prefix
        push    2                                 ; byte op
        push    edx                               ; port number
        call    _Ki386VdmDispatchStringIo@32 ; use retval

        ret

OpcodeOUTSW endp

        page   ,132
        subttl "INTnn Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INTnn opcode.  It retrieves the handler
;       from the IVT, pushes the current cs:ip and flags on the stack,
;       and dispatches to the handler.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       Current CS:IP on user stack
;       RiCs:RiEip -> handler from IVT
;

        public OpcodeINTnn
OpcodeINTnn proc

        mov     eax,dword ptr [ebp].TsEFlags
        call    GetVirtualBits                   ; set interrupt flag
        mov     [esi].RiEFlags,eax
        movzx   eax,word ptr [ebp].TsHardwareSegSs
        call    SsToLinear
        test    al,0FFh
        jz      oinerr

        inc     edi                             ; point to int #
        mov     eax,edi                         ; current linear address
        sub     eax,[esi].RiCsBase              ; make address eip
        cmp     eax,[esi].RiCsLimit             ; check eip
        ja      oinerr

        movzx   ecx,byte ptr [edi]              ; get int #
        inc     eax                             ; inc past end of instruction
        mov     [esi].RiEip,eax                 ; save for pushint's benefit
        call    PushInt                         ; will return retcode in al
        test    al,0FFh
        jz      oinerr                          ; error!

        mov     eax,[esi].RiEsp
        mov     [ebp].TsHardwareEsp,eax
        mov     ax,word ptr [esi].RiSegCs
        or      ax, 7                           ; R3 LDT selectors only
        mov     word ptr [ebp].TsSegCs,ax
        mov     eax,[esi].RiEFlags
        mov     [ebp].TsEFlags,eax
        mov     eax,[esi].RiEip
        mov     [ebp].TsEip,eax
        mov     eax,1
        ret

oinerr:
        xor     eax,eax
        ret


OpcodeINTnn endp

        page   ,132
        subttl "INTO Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INTO opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeINTO
OpcodeINTO proc

        xor     eax,eax
        ret

OpcodeINTO endp


        page   ,132
        subttl "In Byte Immediate Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an in byte immediate opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeINBimm
OpcodeINBimm proc

        inc     ebx                             ; length count
        inc     edi
        mov     eax,edi                         ; current linear address
        sub     eax,[esi].RiCsBase              ; make address eip
        cmp     eax,[esi].RiCsLimit             ; check eip
        ja      oibi20

        movzx   ecx,byte ptr [edi]

; (eax) = inst. size
; read op
; I/O size = 1
; (ecx) = port number

        stdCall   _Ki386VdmDispatchIo, <ecx, 1, TRUE, ebx, ebp>
        ret
oibi20:
        xor     eax, eax                        ; not handled
        ret

OpcodeINBimm endp

        page   ,132
        subttl "Word In Immediate Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an in word immediate opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeINWimm
OpcodeINWimm proc

        inc     ebx                             ; length count
        inc     edi
        mov     eax,edi                         ; current linear address
        sub     eax,[esi].RiCsBase              ; make address eip
        cmp     eax,[esi].RiCsLimit             ; check eip
        ja      oiwi20

        movzx   ecx,byte ptr [edi]

; TRUE - read op
; 2 - word op
; ecx - port number
        stdCall   _Ki386VdmDispatchIo, <ecx, 2, TRUE, ebx, ebp>
        ret
oiwi20:
        xor     eax, eax                        ; not handled
        ret

OpcodeINWimm endp

        page   ,132
        subttl "Out Byte Immediate Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an invalid opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeOUTBimm
OpcodeOUTBimm proc

        inc     ebx                             ; length count
        inc     edi
        mov     eax,edi                         ; current linear address
        sub     eax,[esi].RiCsBase              ; make address eip
        cmp     eax,[esi].RiCsLimit             ; check eip
        ja      oobi20

        movzx   ecx,byte ptr [edi]

; FALSE - write op
; 1 - byte op
; ecx - port #

        stdCall   _Ki386VdmDispatchIo, <ecx, 1, FALSE, ebx, ebp>
        ret
oobi20:
        xor     eax, eax                        ; not handled
        ret

OpcodeOUTBimm endp

        page   ,132
        subttl "Out Word Immediate Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an out word immediate opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeOUTWimm
OpcodeOUTWimm proc

        inc     ebx                             ; length count
        inc     edi
        mov     eax,edi                         ; current linear address
        sub     eax,[esi].RiCsBase              ; make address eip
        cmp     eax,[esi].RiCsLimit             ; check eip
        ja      oowi20

        movzx   ecx,byte ptr [edi]

; FALSE - write op
; 2 - word op
; ecx - port number
        stdCall   _Ki386VdmDispatchIo, <ecx, 2, FALSE, ebx, ebp>
        ret

oowi20:
        xor     eax, eax                        ; not handled
        ret

OpcodeOUTWimm endp

        page   ,132
        subttl "INB Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INB opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeINB
OpcodeINB proc

        movzx   eax,word ptr [ebp].TsEdx

; JAPAN - SUPPORT Intel CPU/Non PC/AT compatible machine
; Get Hardware Id, PC_AT_COMPATIBLE is 0x00XX
        test    _KeI386MachineType, MACHINE_TYPE_MASK
        jnz     oib_reflect

; TRUE - read op
; 1 - byte op
; eax - port number

        cmp     eax, 3bdh
        jz      oib_prt1
        cmp     eax, 379h
        jz      oib_prt1
        cmp     eax, 279h
        jz      oib_prt1

oib_reflect:
        stdCall   _Ki386VdmDispatchIo, <eax, 1, TRUE, ebx, ebp>
        ret
oib20:
        xor     eax, eax                        ; not handled
        ret

oib_prt1:
        ; call printer status routine with port number, size, trap frame
        movzx   ebx, bl                     ;clear prefix flags
        push    eax
        stdCall _VdmPrinterStatus, <eax, ebx, ebp>
        or      al,al
        pop     eax
        jz      short oib_reflect
        mov     al, 1
        ret

OpcodeINB endp

        page   ,132
        subttl "INW Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INW opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeINW
OpcodeINW proc

        movzx   eax,word ptr [ebp].TsEdx

; TRUE - read operation
; 2 - word op
; eax - port number
        stdCall   _Ki386VdmDispatchIo, <eax, 2, TRUE, ebx, ebp>
        ret

OpcodeINW endp

        page   ,132
        subttl "OUTB Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an OUTB opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeOUTB
OpcodeOUTB proc

        movzx   eax,word ptr [ebp].TsEdx

; JAPAN - SUPPORT Intel CPU/Non PC/AT compatible machine
; Get Hardware Id, PC_AT_COMPATIBLE is 0x00XX
        test    _KeI386MachineType, MACHINE_TYPE_MASK
        jnz     oob_reflect

        cmp     eax, 03BCh
        je      short oob_printerVDD
        cmp     eax, 0378h
        je      short oob_printerVDD
        cmp     eax, 0278h
        jz      short oob_printerVDD

oob_reflect:
; FALSE - write op
; 1 - byte op
; eax - port number
        stdCall   _Ki386VdmDispatchIo, <eax, 1, FALSE, ebx, ebp>
        ret

oob_printerVDD:
        movzx   ebx, bl                   ; instruction size
        push    eax                       ; save port address
        stdCall _VdmPrinterWriteData, <eax, ebx, ebp>
        or      al,al                     ;
        pop     eax
        jz      short oob_reflect
        mov     al, 1
        ret

OpcodeOUTB endp

        page   ,132
        subttl "OUTW Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an OUTW opcode.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeOUTW
OpcodeOUTW proc

        movzx   eax,word ptr [ebp].TsEdx

; FALSE - write op
; 2 - word op
; edi - port #
        stdCall   _Ki386VdmDispatchIo, <eax, 2, FALSE, ebx, ebp>
        ret

OpcodeOUTW endp

        page   ,132
        subttl "CLI Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an CLI opcode. It clears the virtual
;       interrupt flag in the VdmTeb.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeCLI
OpcodeCLI proc

        mov     eax,[ebp].TsEFlags
        and     eax,NOT EFLAGS_INTERRUPT_MASK
        call    SetVirtualBits
        inc     dword ptr [ebp].TsEip

        mov     eax,1
        ret

OpcodeCLI endp

        page   ,132
        subttl "STI Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an STI opcode.  It sets the virtual
;       interrupt flag in the VDM teb.
;
;   Arguments:
;       EBP -> trap frame
;       EBX -> prefix flags, BL = instruction length count
;       ECX -> byte at the faulting address
;       EDX -> pointer to vdm state in DOS arena
;       ESI -> Reginfo struct
;       EDI -> address of faulting instruction
;
;   Returns:
;
;       nothing
;

        public OpcodeSTI
OpcodeSTI proc

        mov     eax,[ebp].TsEFlags
        or      eax,EFLAGS_INTERRUPT_MASK
        call    SetVirtualBits
        inc     dword ptr [ebp].TsEip
        mov     eax,_VdmFixedStateLinear
        mov     eax,dword ptr [eax]
        test    eax,VDM_INTERRUPT_PENDING
        jz      os10

        call    VdmDispatchIntAck
os10:
        mov     eax,1
        ret

OpcodeSTI endp

        page   ,132
        subttl "Check Vdm Flags"
;++
;
;   Routine Description:
;
;       This routine checks the flags that are going to be used for the
;       dos or windows application.
;
;   Arguments:
;
;       ecx = EFlags to be set
;       esi = address of reg info
;
;   Returns:
;
;       ecx = fixed flags
;
        public CheckVdmFlags
CheckVdmFlags proc

        push    eax
        mov     eax,[esi].RiEFlags
        and     eax,EFLAGS_V86_MASK
        test    _KeI386VdmIoplAllowed,1
        jnz     cvf30

        test    _KeI386VirtualIntExtensions, V86_VIRTUAL_INT_EXTENSIONS OR PM_VIRTUAL_INT_EXTENSIONS
        jnz     cvf40

cvf10:  or      ecx,EFLAGS_INTERRUPT_MASK
cvf20:  and     ecx,NOT (EFLAGS_IOPL_MASK OR EFLAGS_NT_MASK OR EFLAGS_V86_MASK OR EFLAGS_VIF OR EFLAGS_VIP)
        or      ecx,eax                 ; restore original v86 bit
        pop     eax
        ret

cvf30:  test    eax,EFLAGS_V86_MASK
        jz      cvf10

        jmp     cvf20

cvf40:  test    eax,EFLAGS_V86_MASK
        jz      cvf60

        test    _KeI386VirtualIntExtensions,V86_VIRTUAL_INT_EXTENSIONS
        jz      cvf10

cvf50:  push    eax
        mov     eax,ecx
        and     eax,EFLAGS_INTERRUPT_MASK
        shl     eax,0ah
        pop     eax
        jmp     cvf10

cvf60:  test    _KeI386VirtualIntExtensions,PM_VIRTUAL_INT_EXTENSIONS
        jz      cvf10

        jmp     cvf50
CheckVdmFlags endp

        page   ,132
        subttl "Get Virtual Interrupt Flag"
;++
;
;   Routine Description:
;
;       This routine correctly gets the VDMs virtual interrupt flag and
;       puts it into an EFlags image to be put on the stack.
;
;   Arguments:
;
;       eax = EFlags value
;
;   Returns:
;
;       eax = EFlags value with correct setting for IF
;
;   Uses:
;       ecx
;
        public GetVirtualBits
GetVirtualBits proc

        test    _KeI386VdmIoplAllowed,1
        jnz     gvb60

        test    _KeI386VirtualIntExtensions, V86_VIRTUAL_INT_EXTENSIONS OR PM_VIRTUAL_INT_EXTENSIONS
        jnz     gvb30

gvb10:  and     eax,NOT EFLAGS_INTERRUPT_MASK
        mov     ecx,_VdmFixedStateLinear            ; get pointer to Teb
        mov     ecx,dword ptr [ecx]          ; get virtual int flag
        and     ecx,VDM_VIRTUAL_INTERRUPTS OR VDM_VIRTUAL_AC
        or      eax,ecx                 ; put virtual int flag into flags
        or      eax,EFLAGS_IOPL_MASK    ; make it look like a 386
        ret

gvb30:  test    eax, EFLAGS_V86_MASK
        jz      gvb50

        test    _KeI386VirtualIntExtensions, V86_VIRTUAL_INT_EXTENSIONS
        jz      gvb10

gvb40:  mov     ecx,eax
        and     ecx,EFLAGS_VIF
        shr     ecx,0ah                 ; mov vif to if posn
        and     eax,NOT EFLAGS_INTERRUPT_MASK
        or      eax,ecx

        mov     ecx,_VdmFixedStateLinear
        mov     ecx,dword ptr [ecx]
        and     ecx,VDM_VIRTUAL_AC
        and     eax,NOT EFLAGS_ALIGN_CHECK
        or      eax,ecx
        or      eax,EFLAGS_IOPL_MASK
        ret

gvb50:  test    _KeI386VirtualIntExtensions, PM_VIRTUAL_INT_EXTENSIONS
        jz      gvb10

        jmp     gvb40

gvb60:  test    eax,EFLAGS_V86_MASK
        jz      gvb10

        mov     ecx,_VdmFixedStateLinear
        mov     ecx,dword ptr [ecx]
        and     ecx,VDM_VIRTUAL_AC
        and     eax,NOT EFLAGS_ALIGN_CHECK
        or      eax,ecx
        or      eax,EFLAGS_IOPL_MASK
        ret

GetVirtualBits endp

        page   ,132
        subttl "Set Virtual Interrupt Flag"
;++
;
;   Routine Description:
;
;       This routine correctly sets the VDMs virtual interrupt flag.
;
;   Arguments:
;
;       eax = EFlags value
;
;   Returns:
;
;       Virtual interrupt flag set
;
        public SetVirtualBits
SetVirtualBits proc
Flags   equ [ebp - 4]

        push    ebp
        mov     ebp,esp
        sub     esp,4

        push    edx
        mov     Flags,eax
        mov     edx,_VdmFixedStateLinear            ; get pointer to Teb
        and     eax,EFLAGS_INTERRUPT_MASK ; isolate int flag
        MPLOCK and [edx],NOT VDM_VIRTUAL_INTERRUPTS
        MPLOCK or [edx],eax             ; place virtual int flag value
        test    _KeI386VirtualIntExtensions, V86_VIRTUAL_INT_EXTENSIONS OR PM_VIRTUAL_INT_EXTENSIONS
        jnz     svb40
svb20:
        ; WARNING 32 bit support!
        test    ebx,PREFIX_OPER32
        jz      svb30                   ; 16 bit instr

        mov     eax,Flags
        and     eax,EFLAGS_ALIGN_CHECK
        MPLOCK  and     dword ptr [edx],NOT EFLAGS_ALIGN_CHECK
        MPLOCK  or      [edx],eax
svb30:  pop     edx
        mov     esp,ebp
        pop     ebp
        ret

svb40:  test    Flags,dword ptr EFLAGS_V86_MASK
        jz      svb60

        test    _KeI386VirtualIntExtensions,V86_VIRTUAL_INT_EXTENSIONS
        jz      svb20

svb50:  shl     eax,0ah
        jmp     svb20

svb60:  test    _KeI386VirtualIntExtensions,PM_VIRTUAL_INT_EXTENSIONS
        jz      svb20

        jmp     svb50
SetVirtualBits endp


        page   ,132
        subttl "Reflect Exception to a Vdm"
;++
;
;   Routine Description:
;
;       This routine reflects an exception to a VDM.  It uses the information
;       in the trap frame to determine what exception to reflect, and updates
;       the trap frame with the new CS, EIP, SS, and SP values
;
;   Arguments:
;
;       ebp -> Trap frame
;       ss:esp + 4 = trap number
;
;   Returns
;
;       Nothing
;
;   Notes:
;       Interrupts  are enable upon entry, Irql is at APC level
;       This routine may not preserve all of the non-volatile registers if
;       a fault occurs.
;
cPublicProc _Ki386VdmReflectException,1

RI      equ     [ebp - REGINFOSIZE]

        push    ebp
        mov     ebp,esp
        sub     esp,REGINFOSIZE

        pushad

        mov     esi,_VdmFixedStateLinear

        ;
        ; Look to see if the debugger wants exceptions
        ;
        test    dword ptr [esi],VDM_BREAK_EXCEPTIONS
        jz      vredbg                          ; no, check for debug events

        mov     ebx,DBG_STACKFAULT
        cmp     word ptr [ebp + 8],0ch          ; stack fault?
        jz      @f                              ; yes, check dbg flag
        mov     ebx,DBG_GPFAULT
        cmp     word ptr [ebp + 8],0dh          ; gp fault?
        jne     vredbg                          ; no, continue

@@:
        test    dword ptr [esi],VDM_USE_DBG_VDMEVENT
        jnz     vrexc_event
        jmp     vrexcd                          ; reflect the exception to 32

        ;
        ; Look to see if the debugger wants debug events
        ;
vredbg:
        test    dword ptr [esi],VDM_BREAK_DEBUGGER
        jz      vrevdm                          ; no debug events, reflect to vdm

        mov     ebx,DBG_SINGLESTEP
        cmp     word ptr [ebp + 8],1
        jnz     @f
        test    dword ptr [esi],VDM_USE_DBG_VDMEVENT
        jnz     vrexc_event
        jmp     vrexc1

@@:
        mov     ebx,DBG_BREAK
        cmp     word ptr [ebp + 8],3
        jnz     vrevdm
        test    dword ptr [esi],VDM_USE_DBG_VDMEVENT
        jnz     vrexc_event
        jmp     vrexc3

        ;
        ; reflects the exception to the VDM
        ;
vrevdm:
        mov     esi,[ebp]
        cmp     word ptr [esi].TsSegCs, KGDT_R3_CODE OR RPL_MASK  ; int sim after fault?
        je      vre28
if DEVL
        cmp     word ptr [ebp + 8],11
        jne     xyzzy1
        inc     _ExVdmSegmentNotPresent
xyzzy1:
endif

        mov     RI.RiTrapFrame,esi
        mov     eax,[esi].TsHardwareSegSs
        mov     RI.RiSegSs,eax
        mov     eax,[esi].TsHardwareEsp
        mov     RI.RiEsp,eax
        mov     eax,[esi].TsEFlags
        mov     RI.RiEFlags,eax
        mov     eax,[esi].TsEip
        mov     RI.RiEip,eax
        mov     eax,[esi].TsSegCs
        mov     RI.RiSegCs,eax
        lea     esi,RI
        call    CsToLinear                      ; uses eax as selector
        test    al,0FFh
        jz      vrerr

        mov     eax,[esi].RiSegSs
        call    SsToLinear
        test    al,0FFh
        jz      vrerr

        mov     ecx,[ebp + 8]
        call    PushException
        test    al,0FFh
        jz      vrerr

        mov     esi,RI.RiTrapFrame
        mov     eax,RI.RiEsp
        mov     [esi].TsHardwareEsp,eax
        xor     bl, bl                           ; R3 mask. 0 on V86 mode
        test    dword ptr [esi].TsEFlags, EFLAGS_V86_MASK ;
        jnz     @F                               ;
        mov     bl, 7                            ; protected mode, R3 LDT selectors only
@@:
        mov     eax,RI.RiSegSs
        or      al, bl
        mov     [esi].TsHardwareSegSs,eax
        mov     eax,RI.RiEFlags
        mov     [esi].TsEFlags,eax
        mov     eax,RI.RiSegCs
        or      al, bl
        mov     [esi].TsSegCs,eax
        mov     eax,RI.RiEip
        mov     [esi].TsEip,eax
        cmp     word ptr [ebp + 8],1
        jne     vre28
        and     dword ptr [esi].TsEFlags, NOT EFLAGS_TF_MASK

vre28:
        popad
        mov     eax,1                           ; handled

vre30:
        mov     esp,ebp
        pop     ebp
        stdRET  _Ki386VdmReflectException

vrerr:
        popad
        xor     eax,eax
        jmp     vre30

vrexc1:
        mov     eax, [ebp]
        and     dword ptr [eax]+TsEflags, not EFLAGS_TF_MASK
        mov     eax, [ebp]+TsEip        ; (eax)-> faulting instruction
        stdCall _VdmDispatchException <[ebp],STATUS_SINGLE_STEP,eax,0,0,0,0>
        jmp     vre28

vrexc3:
        mov     eax,BREAKPOINT_BREAK
        mov     ebx, [ebp]
        mov     ebx, [ebx]+TsEip
        dec     ebx                     ; (eax)-> int3 instruction
        stdCall _VdmDispatchException <[ebp],STATUS_BREAKPOINT,ebx,3,eax,ecx,edx>
        jmp     vre28

vrexcd:
        mov     eax, [ebp]
        mov     eax, [eax]+TsEip
        stdCall _VdmDispatchException <[ebp],STATUS_ACCESS_VIOLATION,eax,2,0,-1,0>
        jmp     vre28

vrexc_event:
        mov     eax, [ebp]
        cmp     ebx, DBG_SINGLESTEP
        jnz     vrexc_event2
        and     dword ptr [eax]+TsEflags, not EFLAGS_TF_MASK
vrexc_event2:
        mov     eax, [eax]+TsEip
        stdCall _VdmDispatchException <[ebp],STATUS_VDM_EVENT,eax,1,ebx,0,0>
        jmp     vre28


stdENDP _Ki386VdmReflectException


        page   ,132
        subttl "Reflect Segment Not Present Exception to a Vdm"
;++
;
;   Routine Description:
;
;       This routine reflects an TRAP B to a VDM.  It uses the information
;       in the trap frame to determine what exception to reflect, and updates
;       the trap frame with the new CS, EIP, SS, and SP values
;
;   Arguments:
;
;       ebp -> Trap frame
;
;   Returns
;
;       0 is returned if the reflection fails.
;

cPublicProc _Ki386VdmSegmentNotPresent,0

        mov     edi,PCR[PcPrcbData+PbCurrentThread]
        mov     ecx,VDM_FAULT_HANDLER_SIZE * 0Bh
        mov     edi,[edi]+ThApcState+AsProcess
        mov     edi,[edi].EpVdmObjects
        mov     edi,[edi].VpVdmTib             ; get pointer to VdmTib
        xor     ebx, ebx
        lea     esi,[edi].VtDpmiInfo         ; (edi)->dpmi info struct
        lea     edi,[edi+ecx].VtFaultHandlers ; (edi)->FaultHandler
        cmp     word ptr [esi].VpLockCount, 0 ; switching stacks?
        jz      short @f                     ; yes, we can handle it
                                             ; no, let normal code check
                                             ; for stack faults

        pop     eax                          ; (eax) = return addr
        push    0bh
        push    eax
        jmp     _Ki386VdmReflectException

@@:
if DEVL
        inc     _ExVdmSegmentNotPresent
endif
        inc     word ptr [esi].VpLockCount

; save stuff just like SwitchToHandlerStack does
        mov     eax, [ebp].TsEip
        mov     [esi].VpSaveEip, eax
        mov     eax, [ebp].TsHardwareEsp
        mov     [esi].VpSaveEsp, eax
        mov     ax, [ebp].TsHardwareSegSs
        mov     [esi].VpSaveSsSelector, ax

        mov     bx,word ptr [esi].VpSsSelector
        mov     eax, PCR[PcPrcbData+PbCurrentThread]
        mov     eax, [eax+ThApcState+AsProcess]
        lea     eax, [eax+PrLdtDescriptor]
        mov     ch, [eax+KgdtBaseHi]
        mov     cl, [eax+KgdtBaseMid]
        shl     ecx, 16
        and     ebx, 0fffffff8h
        mov     cx, [eax+KgdtBaseLow]
        lea     eax, [ecx+ebx]
        mov     bh, [eax+KgdtBaseHi]
        mov     bl, [eax+KgdtBaseMid]
        shl     ebx, 16
        mov     bx, [eax+KgdtBaseLow]        ; (ebx) = Base of SS

        mov     eax, [ebp].TsEFlags
        call    GetVirtualBits               ; (eax) = app's eflags
        push    esi
        mov     edx, 0fe0h                   ; dpmistack offset (per win31)
        test    word ptr [esi].VpFlags, 1    ; 32-bit frame?
        jz      short @f
        sub     edx, 8 * 4
        add     edx, ebx
        mov     esi, [ebp].TsHardwareEsp
        mov     ecx, [ebp].TsHardwareSegSs
        mov     [edx + 20], eax              ; push flags
        mov     [edx + 24], esi              ; put esp on new stack
        mov     [edx + 28], ecx              ; put ss on new stack
        mov     ecx, [ebp].TsSegCs
        mov     eax, [ebp].TsEip
        mov     esi, [ebp].TsErrCode
        mov     [edx + 16], ecx              ; push cs
        mov     [edx + 12], eax              ; push ip
        mov     [edx + 8], esi               ; push error code
        pop     esi
        mov     ecx, [esi].VpDosxFaultIretD
        mov     eax, ecx
        shr     eax, 16
        and     ecx, 0ffffh
        mov     [edx + 4], eax               ; push fault iret seg
        mov     [edx], ecx                   ; push fault iret offset
        jmp     short vsnp_update
@@:
        sub     edx, 8 * 2
        add     edx, ebx
        mov     esi, [ebp].TsHardwareEsp
        mov     ecx, [ebp].TsHardwareSegSs
        mov     [edx + 10], ax               ; push flags
        mov     [edx + 12], si               ; put esp on new stack
        mov     [edx + 14], cx               ; put ss on new stack
        mov     ecx, [ebp].TsSegCs
        mov     eax, [ebp].TsEip
        mov     esi, [ebp].TsErrCode
        mov     [edx + 8], cx                ; push cs
        mov     [edx + 6], ax                ; push ip
        mov     [edx + 4], si                ; push error code
        pop     esi
        mov     ecx, [esi].VpDosxFaultIret
        mov     eax, ecx
        shr     eax, 16
        mov     [edx + 2], ax                ; push fault iret seg
        mov     [edx], cx                    ; push fault iret offset

vsnp_update:
        mov     eax,[edi].VfEip
        sub     edx, ebx
        mov     cx, word ptr [edi].VfCsSelector
        mov     bx, word ptr [esi].VpSsSelector
        test    dword ptr [edi].VfFlags, VDM_INT_INT_GATE
        jz      short @f

        mov     esi,_VdmFixedStateLinear  ; get pointer to Teb
        MPLOCK and      [esi],NOT VDM_VIRTUAL_INTERRUPTS
        and     dword ptr [ebp].TsEflags, 0FFF7FFFFH ; clear VIF
@@:
        or      cx, 7                       ; R3 LDT selectors only
        or      bx, 7                       ; R3 LDT selectors only
        mov     [ebp].TsSegCs, cx
        mov     [ebp].TsEip, eax
        mov     [ebp].TsHardwareEsp,edx
        mov     [ebp].TsHardwareSegSs,bx

        mov     eax, 1
        stdRET    _Ki386VdmSegmentNotPresent

stdENDP _Ki386VdmSegmentNotPresent

        page   ,132
        subttl "Dispatch UserMode Exception to a Vdm"
;++
;
;   Routine Description:
;
;   Dispatches exception for vdm in from the kernel, by invoking
;   CommonDispatchException.
;
;   Arguments: See CommonDispatchException for parameter description
;
;   VOID
;   VdmDispatchException(
;        PKTRAP_FRAME TrapFrame,
;        NTSTATUS     ExcepCode,
;        PVOID        ExcepAddr,
;        ULONG        NumParms,
;        ULONG        Parm1,
;        ULONG        Parm2,
;        ULONG        Parm3
;        )
;
;   Returns
;
;       Nothing
;
;   Notes:
;
;       This routine may not preserve all of the non-volatile registers if
;       a fault occurs.
;
cPublicProc _VdmDispatchException,7

TrapFrame equ [ebp+8]
ExcepCode equ [ebp+12]
ExcepAddr equ [ebp+16]
NumParms  equ [ebp+20]
Parm1     equ [ebp+24]
Parm2     equ [ebp+28]
Parm3     equ [ebp+32]

        push    ebp
        mov     ebp,esp
        pushad

        xor     ecx, ecx            ; lower irql to 0
        fstCall KfLowerIrql         ; allow apc's and debuggers in!

        mov    eax, ExcepCode
        mov    ebx, ExcepAddr
        mov    ecx, NumParms
        mov    edx, Parm1
        mov    esi, Parm2
        mov    edi, Parm3
        mov    ebp, TrapFrame
        call   CommonDispatchException

        popad
        stdRET  _VdmDispatchException

stdENDP _VdmDispatchException




        page   ,132
        subttl "Push Interrupt frame on user stack"
;++
;
;   Routine Description:
;
;       This routine pushes an interrupt frame on the user stack
;
;   Arguments:
;
;       ecx = interrupt #
;       esi = address of reg info
;   Returns:
;
;       interrupt frame pushed on stack
;       reg info updated
;
        public PushInt
PushInt proc

        push    ebx
        push    edi

pi100:
;
; Handle dispatching interrupts directly to the handler, rather than
; to the dos extender
;
        ;
        ; Get a the information on the interrupt handler
        ;
        .errnz (VDM_INTERRUPT_HANDLER_SIZE - 8)
        mov     eax,PCR[PcTeb]
        mov     eax,[eax].TbVdm
        lea     eax,[eax].VtInterruptHandlers[ecx*8]

        ;
        ; Get SP
        ;
        mov     edi,[ebp].TsHardwareEsp
        test    [esi].RiSsFlags,SEL_TYPE_BIG
        jnz     pi110

        movzx   edi,di                          ; zero high bits for 64k stack

        ;
        ; Update SP
        ;
pi110:  test    [eax].ViFlags,dword ptr VDM_INT_32
        jz      pi120

        ;
        ; 32 bit iret frame
        ;
        cmp     edi,12                          ; enough space on stack?
        jb      pierr                           ; no, go fault

        sub     edi,12
        mov     [esi].RiEsp,edi
        jmp     pi130

        ;
        ; 16 bit iret frame
        ;
pi120:  cmp     edi,6                           ; enough space on stack?
        jb      pierr                           ; no, go fault

        sub     edi,6
        mov     [esi].RiEsp,edi

        ;
        ; Check limit
        ;
pi130:  test    [esi].RiSsFlags,SEL_TYPE_ED
        jz      pi140

        ;
        ; Expand down, Sp must be above limit
        ;
        cmp     edi,[esi].RiSsLimit
        jna     pierr
        jmp     pi150

        ;
        ; Normal, Sp must be below limit
        ;
pi140:  cmp     edi,[esi].RiSsLimit
        jnb     pierr

        ;
        ; Get base of ss
        ;
pi150:  mov     ebx,[esi].RiSsBase
        test    [eax].ViFlags,dword ptr VDM_INT_32
        jz      pi160

        ;
        ; "push" 32 bit iret frame
        ;
        mov     edx,[esi].RiEip
        mov     [edi + ebx],edx
        mov     dx,word ptr [ebp].TsSegCs
        mov     [edi + ebx] + 4,edx
        push    eax
        mov     eax,[esi].RiEFlags
        call    GetVirtualBits

        mov     [edi + ebx] + 8,eax
        pop     eax
        jmp     pi170

        ;
        ; push 16 bit iret frame
        ;
pi160:  mov     dx,word ptr [esi].RiEip
        mov     [edi + ebx],dx
        mov     dx,word ptr [ebp].TsSegCs
        mov     [edi + ebx] + 2,dx
        push    eax
        mov     eax,[esi].RiEFlags
        call    GetVirtualBits

        mov     [edi + ebx] + 4,ax
        pop     eax

        ;
        ; Update CS and IP
        ;
pi170:  mov     ebx,eax                                 ; save int info
        mov     dx,[eax].ViCsSelector
        mov     word ptr [esi].RiSegCs,dx
        mov     edx,[eax].ViEip
        mov     [esi].RiEip,edx

        movzx   eax, word ptr [esi].RiSegCs
        call    CsToLinear                      ; uses eax as selector

        test    al,0ffh
        jnz     pi175

        ;
        ; Check for destination not present
        ;
        test    [esi].RiCsFlags,SEL_TYPE_NP
        jz      pierr

        mov     al,0ffh                         ; succeeded
        jmp     pi180

        ;
        ; Check handler address
        ;
pi175:  mov     edx,[esi].RiEip
        cmp     edx,[esi].RiCsLimit
        jnb     pierr

        ;
        ; Turn off the trap flag
        ;
pi180:  and     [esi].RiEFlags,NOT EFLAGS_TF_MASK

        ;
        ; Turn off virtual interrupts if necessary
        ;
        test    [ebx].ViFlags,dword ptr VDM_INT_INT_GATE
        ; n.b. We know al is non-zero, because we succeeded in cstolinear
        jz      pi80

        test    _KeI386VirtualIntExtensions,PM_VIRTUAL_INT_EXTENSIONS
        jz      pi75

        and     [esi].RiEFlags, NOT (EFLAGS_VIF)

pi75:   mov     ebx,_VdmFixedStateLinear
        MPLOCK and [ebx], NOT EFLAGS_INTERRUPT_MASK

pi80:   and     [esi].RiEFlags,NOT (EFLAGS_IOPL_MASK OR EFLAGS_NT_MASK OR EFLAGS_V86_MASK)
        or      [esi].RiEFlags,EFLAGS_INTERRUPT_MASK

pi90:   pop     edi
        pop     ebx
        ret

pierr:  xor     eax,eax
        jmp     pi90

PushInt endp

        page   ,132
        subttl "Convert CS Segment or selector to linear address"
;++
;
;   Routine Description:
;
;       Convert CS segment or selector to linear address as appropriate
;       for the curret user mode processor mode.
;
;   Arguments:
;
;       esi = reg info
;
;   Returns:
;
;       reg info updated
;
        public CsToLinear
CsToLinear proc

        test    [esi].RiEFlags,EFLAGS_V86_MASK
        jz      ctl10

;;;     selector now passed in eax
;;;        movzx   eax,word ptr [esi].RiSegCs
        shl     eax,4
        mov     [esi].RiCsBase,eax
        mov     [esi].RiCsLimit,0FFFFh
        mov     [esi].RiCsFlags,0
        mov     eax,1
        ret


ctl10:
        push    edx                             ; WARNING volatile regs!!!
        lea     edx,[esi].RiCsLimit
        push    edx
        lea     edx,[esi].RiCsBase
        push    edx
        lea     edx,[esi].RiCsFlags
        push    edx
        push    eax                             ; push selector

IFDEF STD_CALL
        call    _Ki386GetSelectorParameters@16
ELSE
        call    _Ki386GetSelectorParameters
        add     esp,10h
ENDIF
        pop     edx

        or      al,al
        jz      ctlerr

        test    [esi].RiCsFlags,SEL_TYPE_EXECUTE
        jz      ctlerr

        test    [esi].RiCsFlags,SEL_TYPE_2GIG
        jz      ctl30

        ; Correct limit value for granularity
        shl     [esi].RiCsLimit,12
        or      [esi].RiCsLimit,0FFFh
ctl30:
        mov     eax,1
ctl40:  ret

ctlerr: xor     eax,eax
        jmp     ctl40

CsToLinear endp


        page   ,132
        subttl "Verify that EIP is still valid"
;++
;
;   Routine Description:
;
;       Verify that Eip is still valid and put it into the trap frame
;
;   Arguments:
;
;       esi = address of reg info
;
;   Returns:
;
;
        public CheckEip
CheckEip proc
        mov     eax,[esi].RiEip
        test    [esi].RiEFlags,EFLAGS_V86_MASK
        jz      ce20

        and     eax,[esi].RiCsLimit
        mov     [esi].RiEip,eax
        jmp     ce40

ce20:   cmp     eax,[esi].RiCsLimit
        ja      ceerr
ce40:   mov     eax,1
ce50:   ret

ceerr:  xor     eax,eax
        jmp     ce50

CheckEip endp

        page   ,132
        subttl "Convert Ss Segment or selector to linear address"
;++
;
;   Routine Description:
;
;       Convert Ss segment or selector to linear address as appropriate
;       for the curret user mode processor mode.
;
;   Arguments:
;
;       eax = selector to convert
;       esi = address of reg info
;
;   Returns:
;
;       reg info updated
;
        public SsToLinear
SsToLinear proc

        test    [esi].RiEFlags,EFLAGS_V86_MASK
        jz      stl10

        shl     eax,4
        mov     [esi].RiSsBase,eax
        mov     [esi].RiSsLimit,0FFFFh
        mov     [esi].RiSsFlags,0
        mov     eax,1
        ret

stl10:  push    ecx
        lea     ecx,[esi].RiSsLimit
        push    ecx
        lea     ecx,[esi].RiSsBase
        push    ecx
        lea     ecx,[esi].RiSsFlags
        push    ecx
        push    eax                             ;selector

IFDEF STD_CALL
        call    _Ki386GetSelectorParameters@16
ELSE
        call    _Ki386GetSelectorParameters
        add     esp,10h
ENDIF
        pop     ecx

        or      al,al
        jz      stlerr

        test    [esi].RiSsFlags,SEL_TYPE_WRITE
        jz      stlerr

        test    [esi].RiSsFlags,SEL_TYPE_2GIG
        jz      stl30

        ; Correct limit value for granularity

        mov     eax,[esi].RiSsLimit
        shl     eax,12
        or      eax,0FFFh
        mov     [esi].RiSsLimit,eax
stl30:
        mov     eax,1
stl40:  ret

stlerr: xor     eax,eax
        jmp     stl40

SsToLinear endp

        page   ,132
        subttl "Verify that Esp is still valid"
;++
;
;   Routine Description:
;
;       Verify that Esp is still valid
;
;   Arguments:
;
;       ecx = # of bytes needed for stack frame
;       esi = address of reg info
;
;   Returns:
;
;
;
        public CheckEsp
CheckEsp proc
        mov     eax,[esi].RiEsp
        test    [esi].RiEFlags,EFLAGS_V86_MASK
        jz      cs20

        and     eax,[esi].RiSsLimit
        mov     [esi].RiEsp,eax
        jmp     cs40

cs20:   test    [esi].RiSsFlags,SEL_TYPE_BIG
        jnz     cs25

        and     eax,0FFFFh                      ; only use 16 bit for 16 bit
cs25:
        cmp     ecx, eax                        ; StackOffset > SP?
        ja      cserr                           ; yes error
        dec     eax                             ; make limit checks work
        test    [esi].RiSsFlags,SEL_TYPE_ED     ; Expand down?
        jz      cs30                            ; jif no

;
;       Expand Down
;
        sub     eax, ecx                        ; New SP
        cmp     eax,[esi].RiSsLimit             ; NewSp < Limit?
        jb      cserr
        jmp     cs40

;
;       Not Expand Down
;
cs30:   cmp     eax,[esi].RiSsLimit
        ja      cserr

cs40:   mov     eax,1
cs50:   ret


cserr:  xor     eax,eax
        jmp     cs50

CheckEsp endp

        page   ,132
        subttl "Switch to protected mode interrupt stack"
;++
;
;   Routine Description:
;
;       Switch to protected mode interrupt handler stack
;
;   Arguments:
;
;       ecx = interrupt number
;       esi = address of reg info
;       edi = address of PM Stack info
;
;   Returns:
;
;       reg info updated
;
        public SwitchToHandlerStack
SwitchToHandlerStack proc


        cmp     word ptr [edi].VpLockCount, 0   ; already switched?
        jnz     short @f                        ; yes

        mov     eax, [esi].RiEip
        mov     [edi].VpSaveEip, eax
        mov     eax, [esi].RiEsp
        mov     [edi].VpSaveEsp, eax
        mov     eax, [esi].RiSegSs
        mov     [edi].VpSaveSsSelector, ax

        movzx   eax,word ptr [edi].VpSsSelector
        mov     [esi].RiSegSs,eax
        mov     dword ptr [esi].RiEsp,1000h     ; dpmi stack offset

        movzx   eax, word ptr [esi].RiSegSs
        push    ecx
        call    SsToLinear                      ; compute new base
        pop     ecx
        test    al,0FFh
        jz      shserr
@@:
        inc     word ptr [edi].VpLockCount      ; maintain lock count
        mov     eax,1
        ret

shserr:
        xor     eax,eax
        ret

SwitchToHandlerStack endp


        page   ,132
        subttl "Get protected mode interrupt handler address"
;++
;
;   Routine Description:
;
;       Get the address of the interrupt handler for the sepcified interrupt
;
;   Arguments:
;
;       ecx = interrupt number
;       esi = address of reg info
;
;   Returns:
;
;       reg info updated
;
        public GetHandlerAddress
GetHandlerAddress proc

        push    ecx
        push    edx
        mov     eax,VDM_FAULT_HANDLER_SIZE
        mul     ecx
        mov     edi,PCR[PcPrcbData+PbCurrentThread]
        mov     edi,[edi]+ThApcState+AsProcess
        mov     edi,[edi].EpVdmObjects
        mov     edi,[edi].VpVdmTib             ; get pointer to VdmTib
        lea     edi,[edi].VtFaultHandlers
        movzx   ecx,word ptr [edi + eax].VfCsSelector
        mov     [esi].RiSegCs,ecx
        mov     ecx,[edi + eax].VfEip
        mov     [esi].RiEip,ecx
        pop     edx
        pop     ecx
        mov     eax,1
        ret
GetHandlerAddress endp

        page   ,132
        subttl "Push processor exception"
;++
;
;   Routine Description:
;
;       Update the stack and registers to emulate the specified exception
;
;   Arguments:
;
;       ecx = interrupt number
;       esi = address of reg info
;
;   Returns:
;
;       reg info updated
;
        public PushException
PushException Proc

        push    ebx
        push    edi

        test    [esi].RiEflags,EFLAGS_V86_MASK
        jz      pe40

;
; Push V86 mode exception
;
        cmp     ecx, 7                  ; device not available fault
        ja      peerr                   ; per win3.1, no exceptions
                                        ; above 7 for v86 mode
        mov     edx,[esi].RiEsp
        mov     ebx,[esi].RiSsBase
        and     edx,0FFFFh              ; only use a 16 bit sp
        sub     dx,2
        mov     eax,[esi].RiEFlags
        push    ecx
        call    GetVirtualBits
        pop     ecx
        mov     [ebx+edx],ax            ; push flags
        sub     dx,2
        mov     ax,word ptr [esi].RiSegCs
        mov     [ebx+edx],ax            ; push cs
        sub     dx,2
        mov     ax,word ptr [esi].RiEip
        mov     [ebx+edx],ax            ; push ip
        mov     eax,[ecx*4]             ; get new cs:ip value
        push    eax
        movzx   eax,ax
        mov     [esi].RiEip,eax
        pop     eax
        shr     eax,16
        mov     [esi].RiSegCs,eax
        mov     word ptr [esi].RiEsp,dx
        jmp     pe60

;
; Push PM exception
;
pe40:
        push    [esi].RiEsp                     ; save for stack frame
        push    [esi].RiSegSs

        mov     edi,PCR[PcPrcbData+PbCurrentThread]
        mov     edi,[edi]+ThApcState+AsProcess
        mov     edi,[edi].EpVdmObjects
        mov     edi,[edi].VpVdmTib             ; get pointer to VdmTib
        lea     edi,[edi].VtDpmiInfo
        call    SwitchToHandlerStack
        test    al,0FFh
        jz      peerr1                          ; pop off stack and exit

        sub     [esi].RiEsp, 20h                ; win31 undocumented feature

        mov     ebx,[esi].RiSsBase
        mov     edx,[esi].RiEsp
        test    [esi].RiSsFlags,SEL_TYPE_BIG
        jnz     short @f
        movzx   edx,dx                          ; zero high bits for 64k stack
@@:

        test    word ptr [edi].VpFlags, 1       ; 32 bit app?
        jnz     short pe45                      ; yes

;
;       push 16-bit frame
;
        push    ecx
        mov     ecx, 8*2                        ; room for 8 words?
        call    CheckEsp
        pop     ecx
        test    al,0FFh
        jz      peerr1                          ; pop off stack and exit

        sub     edx,8*2
        mov     [esi].RiEsp,edx

        pop     eax                             ; ss
        mov     [ebx+edx+14], ax
        pop     eax                             ; esp
        mov     [ebx+edx+12], ax

        mov     eax,[esi].RiEFlags
        push    ecx
        call    GetVirtualBits
        pop     ecx
        mov     [ebx+edx+10],ax                 ; push flags
        movzx   eax,word ptr [esi].RiSegCs
        mov     [ebx+edx+8],ax                  ; push cs
        mov     eax,[esi].RiEip
        mov     [ebx+edx+6],ax                  ; push ip
        mov     eax,RI.RiTrapFrame
        mov     eax,[eax].TsErrCode
        mov     [ebx+edx+4],ax                  ; push error code
        mov     eax,[edi].VpDosxFaultIret
        mov     [ebx+edx],eax                   ; push iret address
        jmp     pe50
pe45:
;
;       push 32-bit frame
;
        push    ecx
        mov     ecx, 8*4                        ; room for 8 dwords?
        call    CheckEsp
        pop     ecx
        test    al,0FFh
        jz      peerr1                          ; pop off stack and exit

        sub     edx,8*4
        mov     [esi].RiEsp,edx

        pop     eax                             ; ss
        mov     [ebx+edx+28], eax
        pop     eax                             ; esp
        mov     [ebx+edx+24], eax

        mov     eax,[esi].RiEFlags
        push    ecx
        call    GetVirtualBits
        pop     ecx
        mov     [ebx+edx+20],eax                ; push flags
        movzx   eax,word ptr [esi].RiSegCs
        mov     [ebx+edx+16],eax                ; push cs
        mov     eax,[esi].RiEip
        mov     [ebx+edx+12],eax                ; push ip
        mov     eax,RI.RiTrapFrame
        mov     eax,[eax].TsErrCode
        mov     [ebx+edx+8],eax                 ; push error code
        mov     eax,[edi].VpDosxFaultIretD
        shr     eax, 16
        mov     [ebx+edx+4],eax                 ; push iret seg
        mov     eax,[edi].VpDosxFaultIretD
        and     eax, 0ffffh
        mov     [ebx+edx],eax                   ; push iret offset
pe50:
        call    GetHandlerAddress
        test    al,0FFh
        jz      peerr

pe60:   push    ecx
        movzx   eax,word ptr [esi].RiSegCs
        call    CsToLinear                      ; uses eax as selector
        pop     ecx
        test    al,0FFh
        jz      peerr

        mov     eax,[esi].RiEip
        cmp     eax,[esi].RiCsLimit
        ja      peerr

        mov     eax,VDM_FAULT_HANDLER_SIZE
        push    edx
        mul     ecx
        pop     edx
        mov     edi,PCR[PcTeb]
        mov     edi,[edi].TbVdm
        lea     edi,[edi].VtFaultHandlers
        add     edi,eax
        mov     eax,[esi].RiEFlags  ;WARNING 16 vs 32
        test    dword ptr [edi].VfFlags,VDM_INT_INT_GATE
        jz      pe70

        and     eax,NOT (EFLAGS_INTERRUPT_MASK OR EFLAGS_TF_MASK)
        push    eax
        xor     ebx, ebx                ;  clear prefix flags
        call    SetVirtualBits
        pop     eax
pe70:   push    ecx
        mov     ecx,eax
        call    CheckVdmFlags
        and     ecx,NOT EFLAGS_TF_MASK
        mov     [esi].RiEFlags,ecx
        pop     ecx
        mov     eax,1                   ; success
pe80:   pop     edi
        pop     ebx
        ret

peerr1: add     esp, 8                  ;throw away esp, ss
peerr:  xor     eax,eax
        jmp     pe80

PushException endp


_PAGE   ends


_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

;
; Non-pagable code
;

        page   ,132
        subttl "Ipi worker for enabling Pentium extensions"
;++
;
;   Routine Description:
;
;       This routine sets or resets the VME bit in CR4 for each processor
;
;   Arguments:
;
;       [esp + 4] -- 1 if VME is to be set, 0 if it is to be reset
;   Returns:
;
;       0
;
cPublicProc _Ki386VdmEnablePentiumExtentions, 1

Enable equ [ebp + 8]
        push    ebp
        mov     ebp,esp
;
;       Insure we do not get an interrupt in here.  We may
;       be called at IPI_LEVEL - 1 by KiIpiGenericCall.
;
        pushf
        cli

;       mov     eax,cr4
        db      0fh, 020h,0e0h

        test    Enable,1
        je      vepe20

        or      eax,CR4_VME
        jmp     vepe30

vepe20: and     eax,NOT CR4_VME

;       mov     cr4,eax
vepe30: db      0fh,022h,0e0h

        popf
        xor     eax,eax

        mov     esp,ebp
        pop     ebp
        stdRET _Ki386VdmEnablePentiumExtentions
stdENDP _Ki386VdmEnablePentiumExtentions

_TEXT$00 ends
        end
