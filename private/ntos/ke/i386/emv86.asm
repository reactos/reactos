        title  "Vdm Instuction Emulation"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    emv86.asm
;
; Abstract:
;
;    This module contains the routines for emulating instructions and
;    faults from v86 mode.
;
; Author:
;
;   sudeep bharati (sudeepb) 16-Nov-1992
;
; Environment:
;
;    Kernel mode only.
;
; Notes:
;
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
        extrn   _DbgPrint:proc
        extrn   _KeI386VdmIoplAllowed:dword
        extrn   _KeI386VirtualIntExtensions:dword
        EXTRNP  _Ki386VdmDispatchIo,5
        EXTRNP  _Ki386VdmDispatchStringIo,8
        EXTRNP  _KiDispatchException,5
        EXTRNP  _Ki386VdmReflectException,1
        EXTRNP  _VdmEndExecution,2
        extrn   VdmDispatchBop:near
        EXTRNP  _VdmPrinterStatus,3
        EXTRNP  _VdmPrinterWriteData, 3
        EXTRNP  _VdmDispatchInterrupts,2
        EXTRNP  _KeBugCheck,1
        EXTRNP  _VdmSkipNpxInstruction,4
ifdef VDMDBG
        EXTRNP  _VdmTraceEvent,4
endif

        extrn   _ExVdmOpcodeDispatchCounts:dword
        extrn   OpcodeIndex:byte
        extrn   _VdmUserCr0MapIn:byte

; SUPPORT Intel CPU/Non PC/AT machine
        extrn   _VdmFixedStateLinear:dword
        extrn   _KeI386MachineType:dword

        page ,132

ifdef VDMDBG
%out Debugging version
endif

;   Force assume into place

_PAGE   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING
_PAGE   ENDS

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING
_TEXT$00   ENDS

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

;
;  Instruction emulation emulates the following instructions.
;  The emulation affects the noted user mode registers.
;
;
;  In V86 mode, the following instruction are emulated in the kernel
;
;    Registers  (E)Flags (E)SP  SS  CS
;       PUSHF      X       X
;       POPF       X       X
;       INTnn      X       X         X
;       INTO       X       X         X
;       IRET       X       X         X
;       CLI        X
;       STI        X
;
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


;       OpcodeDispatchV86 - table of routines used to emulate instructions
;                           in v86 mode.

        public OpcodeDispatchV86
dtBEGIN OpcodeDispatchV86,OpcodeInvalidV86
        dtS    VDM_INDEX_0F              , Opcode0FV86
        dtS    VDM_INDEX_ESPrefix        , OpcodeESPrefixV86
        dtS    VDM_INDEX_CSPrefix        , OpcodeCSPrefixV86
        dtS    VDM_INDEX_SSPrefix        , OpcodeSSPrefixV86
        dtS    VDM_INDEX_DSPrefix        , OpcodeDSPrefixV86
        dtS    VDM_INDEX_FSPrefix        , OpcodeFSPrefixV86
        dtS    VDM_INDEX_GSPrefix        , OpcodeGSPrefixV86
        dtS    VDM_INDEX_OPER32Prefix    , OpcodeOPER32PrefixV86
        dtS    VDM_INDEX_ADDR32Prefix    , OpcodeADDR32PrefixV86
        dtS    VDM_INDEX_INSB            , OpcodeINSBV86
        dtS    VDM_INDEX_INSW            , OpcodeINSWV86
        dtS    VDM_INDEX_OUTSB           , OpcodeOUTSBV86
        dtS    VDM_INDEX_OUTSW           , OpcodeOUTSWV86
        dtS    VDM_INDEX_PUSHF           , OpcodePUSHFV86
        dtS    VDM_INDEX_POPF            , OpcodePOPFV86
        dtS    VDM_INDEX_INTnn           , OpcodeINTnnV86
        dtS    VDM_INDEX_INTO            , OpcodeINTOV86
        dtS    VDM_INDEX_IRET            , OpcodeIRETV86
        dts    VDM_INDEX_NPX             , OpcodeNPXV86
        dtS    VDM_INDEX_INBimm          , OpcodeINBimmV86
        dtS    VDM_INDEX_INWimm          , OpcodeINWimmV86
        dtS    VDM_INDEX_OUTBimm         , OpcodeOUTBimmV86
        dtS    VDM_INDEX_OUTWimm         , OpcodeOUTWimmV86
        dtS    VDM_INDEX_INB             , OpcodeINBV86
        dtS    VDM_INDEX_INW             , OpcodeINWV86
        dtS    VDM_INDEX_OUTB            , OpcodeOUTBV86
        dtS    VDM_INDEX_OUTW            , OpcodeOUTWV86
        dtS    VDM_INDEX_LOCKPrefix      , OpcodeLOCKPrefixV86
        dtS    VDM_INDEX_REPNEPrefix     , OpcodeREPNEPrefixV86
        dtS    VDM_INDEX_REPPrefix       , OpcodeREPPrefixV86
        dtS    VDM_INDEX_CLI             , OpcodeCLIV86
        dtS    VDM_INDEX_STI             , OpcodeSTIV86
        dtS    VDM_INDEX_HLT             , OpcodeHLTV86
dtEND   MAX_VDM_INDEX

_DATA   ENDS

_PAGE   SEGMENT DWORD USE32 PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

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
;       esi = address of reg info
;       edx = opcode
;
;   Returns
;       user mode Eip advanced
;       eax advanced
;       edx contains next byte of opcode
;
;   NOTE: This routine exits by dispatching through the table again.
;--
opPrefix macro name
        public Opcode&name&PrefixV86
Opcode&name&PrefixV86 proc

        or      ebx,PREFIX_&name


ifdef VDMDBG
_DATA segment
Msg&name&Prefix db 'NTVDM: Encountered override prefix &name& %lx at '
                db 'address %lx', 0ah, 0dh, 0
_DATA ends

        push    [ebp].TsEip
        push    [ebp].TsSegCs
        push    offset FLAT:Msg&name&Prefix
        call    _DbgPrint
        add     esp,12

endif

        jmp     OpcodeGenericPrefixV86   ; dispatch to next handler

Opcode&name&PrefixV86 endp
endm

irp prefix, <ES, CS, SS, DS, FS, GS, OPER32, ADDR32, LOCK, REPNE, REP>

        opPrefix prefix

endm

        page   ,132
        subttl "Instruction Emulation Dispatcher for V86"
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
;       EAX = 0 failure
;             1 success

cPublicProc _Ki386DispatchOpcodeV86,0

        mov     esi,[ebp].TsSegCs
        shl     esi,4
        add     esi,[ebp].TsEip
        movzx   ecx, byte ptr [esi]
        movzx   edx, OpcodeIndex[ecx]   ;get opcode index

        mov     edi,1
        xor     ebx,ebx

        ; All handler routines will get the following on entry
        ; ebx -> prefix flags
        ; ebp -> trap frame
        ; cl  -> byte at the faulting address
        ; interrupts enabled and Irql at APC level
        ; esi -> address of faulting instruction
        ; edi -> instruction length count
        ; All handler routines return
        ; EAX = 0 for failure
        ; EAX = 1 for success

if DEVL
        inc     _ExVdmOpcodeDispatchCounts[edx * type _ExVdmOpcodeDispatchCounts]
endif
ifdef VDMDBG
        pushad
        stdCall _VdmTraceEvent, <VDMTR_KERNEL_OP_V86,ecx,0,ebp>
        popad
endif
        jmp     dword ptr OpcodeDispatchV86[edx * type OpcodeDispatchV86]

stdENDP _Ki386DispatchOpcodeV86


        page   ,132
        subttl "Invalid Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an invalid opcode.  It prints the invalid
;       opcode message, and causes a GP fault to be reflected to the
;       debuger
;
;   Arguments:
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeInvalidV86
OpcodeInvalidV86 proc

        xor     eax,eax                 ; ret fail
        ret

OpcodeInvalidV86 endp


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
;
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeGenericPrefixV86
OpcodeGenericPrefixV86 proc

        inc     esi
        inc     edi
        movzx   ecx, byte ptr [esi]
        movzx   edx, OpcodeIndex[ecx]   ;get opcode index
if DEVL
        inc     _ExVdmOpcodeDispatchCounts[edx * type _ExVdmOpcodeDispatchCounts]
endif
        jmp     OpcodeDispatchV86[edx * type OpcodeDispatchV86]

OpcodeGenericPrefixV86 endp


        page   ,132
        subttl "Byte string in Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INSB opcode.  Currently, it prints
;       a message, and ignores the instruction.
;
;   Arguments:
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;
;  WARNING size override?  ds override?

        public OpcodeINSBV86
OpcodeINSBV86 proc

        push    ebp                     ; trap frame
        push    edi                     ; size of insb
        movzx   eax,word ptr [ebp].TsV86Es
        shl     eax,16
        movzx   ecx,word ptr [ebp].TsEdi
        or      eax,ecx
        push    eax                     ; address
        mov     eax,1
        xor     ecx, ecx
        test    ebx,PREFIX_REP          ; prefixREP
        jz      oisb20

        mov     ecx, 1
        movzx   eax,word ptr [ebp].TsEcx
oisb20:
        push    eax                     ; number of io ops
        push    TRUE                    ; read op
        push    ecx                     ; REP prefix ?
        push    1                       ; byte op
        movzx   eax,word ptr [ebp].TsEdx
        push    eax                     ; port number

        ; Ki386VdmDispatchStringIo enables interrupts
IFDEF STD_CALL
        call    _Ki386VdmDispatchStringIo@32 ; use retval
ELSE
        call    _Ki386VdmDispatchStringIo ; use retval
        add     esp,24
ENDIF
        ret

OpcodeINSBV86 endp

        page   ,132
        subttl "Word String In Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INSW opcode.  Currently, it prints
;       a message, and ignores the instruction.
;
;   Arguments:
;
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeINSWV86
OpcodeINSWV86 proc

        push    ebp                     ; trap frame
        push    edi                     ; size of insw
        movzx   eax,word ptr [ebp].TsV86Es
        shl     eax,16
        movzx   ecx,word ptr [ebp].TsEdi
        or      eax,ecx
        push    eax                     ; address
        mov     eax,1
        xor     ecx, ecx
        test    ebx,PREFIX_REP          ; prefixREP
        jz      oisw20

        mov     ecx, 1
        movzx   eax,word ptr [ebp].TsEcx
oisw20:
        push    eax                     ; number of io ops
        push    TRUE                    ; read op
        push    ecx                     ; REP prefix ?
        push    2                       ; word op
        movzx   eax,word ptr [ebp].TsEdx
        push    eax                     ; port number

        ; Ki386VdmDispatchStringIo enables interrupts
IFDEF STD_CALL
        call    _Ki386VdmDispatchStringIo@32 ; use retval
ELSE
        call    _Ki386VdmDispatchStringIo ; use retval
        add     esp,24
ENDIF
        ret

OpcodeINSWV86 endp

        page   ,132
        subttl "Byte String Out Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an OUTSB opcode.  Currently, it prints
;       a message, and ignores the instruction.
;
;   Arguments:
;
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeOUTSBV86
OpcodeOUTSBV86 proc

        push    ebp                     ; trap frame
        push    edi                     ; size of outsb
        movzx   eax,word ptr [ebp].TsV86Ds
        shl     eax,16
        movzx   ecx,word ptr [ebp].TsEsi
        or      eax,ecx
        push    eax                     ; address
        mov     eax,1
        xor     ecx, ecx
        test    ebx,PREFIX_REP          ; prefixREP
        jz      oosb20

        mov     ecx, 1
        movzx   eax,word ptr [ebp].TsEcx
oosb20:
        push    eax                     ; number of io ops
        push    FALSE                   ; write op
        push    ecx                     ; REP prefix ?
        push    1                       ; byte op
        movzx   eax,word ptr [ebp].TsEdx
        push    eax                     ; port number

        ; Ki386VdmDispatchStringIo enables interrupts
IFDEF STD_CALL
        call    _Ki386VdmDispatchStringIo@32 ; use retval
ELSE
        call    _Ki386VdmDispatchStringIo ; use retval
        add     esp,24
ENDIF
        ret

OpcodeOUTSBV86 endp

        page   ,132
        subttl "Word String Out Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an OUTSW opcode.  Currently, it prints
;       a message, and ignores the instruction
;
;   Arguments:
;
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeOUTSWV86
OpcodeOUTSWV86 proc

        push    ebp                     ; trap frame
        push    edi                     ; size of outsw
        movzx   eax,word ptr [ebp].TsV86Ds
        shl     eax,16
        movzx   ecx,word ptr [ebp].TsEsi
        or      eax,ecx
        push    eax                     ; address

        mov     eax,1
        xor     ecx, ecx
        test    ebx,PREFIX_REP          ; prefixREP
        jz      oosw20

        mov     ecx, 1
        movzx   eax,word ptr [ebp].TsEcx
oosw20:
        push    eax                     ; number of io ops
        push    FALSE                   ; write op
        push    ecx                     ; REP prefix ?
        push    2                       ; word op
        movzx   eax,word ptr [ebp].TsEdx
        push    eax                     ; port number

        ; Ki386VdmDispatchStringIo enables interrupts
IFDEF STD_CALL
        call    _Ki386VdmDispatchStringIo@32 ; use retval
ELSE
        call    _Ki386VdmDispatchStringIo ; use retval
        add     esp,24
ENDIF
        ret

OpcodeOUTSWV86 endp

        page   ,132
        subttl "PUSHF Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an PUSHF opcode.  Currently, it prints
;       a message, and simulates the instruction.
;
;       Get SS
;       shift left 4
;       get SP
;       subtract 2
;       get flags
;       put in virtual interrupt flag
;       put on stack
;       update sp
;
;   Arguments:
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;
        public OpcodePUSHFV86
OpcodePUSHFV86 proc

        mov     eax,_VdmFixedStateLinear  ; get pointer to VDM State
        mov     eax, dword ptr [eax]         ; get virtual int flag
        and     eax,VDM_VIRTUAL_INTERRUPTS OR VDM_VIRTUAL_AC OR VDM_VIRTUAL_NT
        mov     edx,dword ptr [ebp].TsEFlags
        and     edx,NOT EFLAGS_INTERRUPT_MASK
        or      eax,edx
        or      eax,EFLAGS_IOPL_MASK
        movzx   ecx,word ptr [ebp].TsHardwareSegSS
        shl     ecx,4
        movzx   edx,word ptr [ebp].TsHardwareEsp
        sub     dx,2

        test    ebx,PREFIX_OPER32               ; check operand size
        jnz     puf10

        mov     [ecx + edx],ax
puf05:
        mov     word ptr [ebp].TsHardwareEsp,dx ; update client esp
        add     dword ptr [ebp].TsEip,edi

        mov     eax,1
        ret

puf10:  sub     dx,2
        mov     [ecx + edx],eax
        jmp     puf05

OpcodePUSHFV86 endp

        page   ,132
        subttl "POPF Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an POPF opcode.  Currently, it prints
;       a message, and returns to the monitor.
;
;   Arguments:
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodePOPFV86
OpcodePOPFV86 proc

        mov     eax,_VdmFixedStateLinear  ; get pointer to VDM State
        mov     ecx,[ebp].TsHardwareSegSS
        shl     ecx,4
        movzx   edx,word ptr [ebp].TsHardwareEsp
        mov     ecx,[ecx + edx]          ; get flags from stack
        add     edx,4
        test    ebx,PREFIX_OPER32        ; check operand size
        jnz     pof10
        and     ecx,0ffffh
        sub     edx,2
pof10:
        mov     [ebp].TsHardwareEsp,edx
        and     ecx, NOT EFLAGS_IOPL_MASK
        mov     ebx,ecx
        and     ebx, NOT EFLAGS_NT_MASK

        test    _KeI386VirtualIntExtensions, dword ptr V86_VIRTUAL_INT_EXTENSIONS
        jz      short pof15
        and     ebx, NOT EFLAGS_VIP                 ; no need for this now
        or      ebx, EFLAGS_VIF
        test    ebx, EFLAGS_INTERRUPT_MASK
        jnz     pof15
        and     ebx, NOT EFLAGS_VIF

pof15:
        or      ebx, (EFLAGS_INTERRUPT_MASK OR EFLAGS_V86_MASK)
        mov     [ebp].TsEFlags,ebx
        and     ecx, (EFLAGS_INTERRUPT_MASK OR EFLAGS_ALIGN_CHECK OR EFLAGS_NT_MASK)
        MPLOCK and [eax],NOT (EFLAGS_INTERRUPT_MASK OR EFLAGS_ALIGN_CHECK OR EFLAGS_NT_MASK)
        MPLOCK or [eax],ecx
        add     dword ptr [ebp].TsEip,edi

        mov     eax,dword ptr [eax]
        test    eax,VDM_INTERRUPT_PENDING
        jz      pof25

        test    eax,VDM_VIRTUAL_INTERRUPTS
        jz      pof25

        call    VdmDispatchIntAck

pof25:
        mov     eax,1                    ; handled
        ret
OpcodePOPFV86 endp

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
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeINTnnV86
OpcodeINTnnV86 proc

;
; Int nn in v86 mode always disables interrupts
;

        mov     edx,[ebp].TsEflags
;
; If KeI386VdmIoplAllowed is true, direct IF manipulation is allowed
;
        test    _KeI386VdmIoplAllowed,1
        jz      oinnv10

        mov     eax,edx                      ; save original flags
        and     edx,NOT EFLAGS_INTERRUPT_MASK
        jmp     oinnv20

;
; Else, IF and some other flags bits are virtualized
;
oinnv10:
        mov     eax,_VdmFixedStateLinear  ; get pointer to VDM State
        mov     ecx,dword ptr [eax]
        MPLOCK and [eax],NOT VDM_VIRTUAL_INTERRUPTS
	mov	eax, edx		    ;eflags
	and	eax, NOT EFLAGS_INTERRUPT_MASK	;turn off IF
.errnz (EFLAGS_INTERRUPT_MASK - VDM_VIRTUAL_INTERRUPTS)
	and	ecx, VDM_VIRTUAL_INTERRUPTS OR VDM_VIRTUAL_AC
        test    _KeI386VirtualIntExtensions, dword ptr V86_VIRTUAL_INT_EXTENSIONS
	jz	oinnv15
;
;VIF extension is enabled, we should migrate EFLAGS_VIF instead of
;VDM_VIRTUAL_INTERRUPT to the iret frame eflags IF.
;When VIF extension is enabled, RI_BIT_MASK is turned on. This in turn,
;redirects the FCLI/FSTI macro to execute cli/sti directly instead
;of simulation. Without this, we might disable v86 mode interrupt
;without the applications knowing it.
;
	and	ecx, VDM_VIRTUAL_AC	    ;keep VDM_VIRTUAL_AC only
	or	eax, ecx		    ;eflags + ac (IF is off)
	mov	ecx, edx
	and	ecx, EFLAGS_VIF
.errnz	((EFLAGS_VIF SHR 10) - EFLAGS_INTERRUPT_MASK)
	ror	ecx, 10 		    ;VIF -> IF
oinnv15:
	or	eax, ecx		    ;merge IF

oinnv20:
        and     edx,NOT (EFLAGS_NT_MASK OR EFLAGS_TF_MASK)
        mov     [ebp].TsEflags,edx
        or      eax, EFLAGS_IOPL_MASK
        movzx   ecx,word ptr [ebp].TsHardwareSegSS
        shl     ecx,4
        movzx   edx,word ptr [ebp].TsHardwareEsp    ; ecx+edx is user stack
        sub     dx,2
        mov     word ptr [ecx+edx],ax       ; push flags
        mov     ax,word ptr [ebp].TsSegCS
        sub     dx,2
        mov     word ptr [ecx+edx],ax       ; push cs
        movzx   eax,word ptr [ebp].TsEip
        add     eax, edi
        inc     eax
        sub     dx,2
        mov     word ptr [ecx+edx],ax       ; push ip
        mov     [ebp].TsHardwareEsp,dx      ; update sp on trap frame

        inc     esi
        movzx   ecx,byte ptr [esi]          ; ecx is int#

;
;       Check if this is a v86 interrupt which must be reflected to a PM handler
;
        mov     eax,PCR[PcPrcbData+PbCurrentThread]
        mov     eax,[eax]+ThApcState+AsProcess
        mov     eax,[eax].EpVdmObjects
        mov     eax,[eax].VpVdmTib             ; get pointer to VdmTib
        lea     ebx,[eax].VtInterruptHandlers[ecx*8]
        test    [ebx].ViFlags, VDM_INT_HOOKED    ; need to reflect to PM?
        jz      oinnv30

        lea     ebx,[eax].VtDpmiInfo        ; point to DpmiInfo
        mov     ebx,[ebx].VpDosxRmReflector ; bop to reflect to PM
;
;       Encode interrupt number in cs
;
        mov     eax,ebx
        shr     eax,16                      ; bop cs
        sub     eax,ecx                     ; new cs
        shl     ecx,4
        add     ebx,ecx                     ; new ip
        jmp     oinnv40
oinnv30:
;
;       Not hooked, just pick up new vector from RM IVT
;
        mov     ebx,[ecx*4]
        mov     eax,ebx
        shr     eax,16                      ; new cs
oinnv40:
        mov     word ptr [ebp].TsEip,bx
        mov     [ebp].TsSegCs,ax            ; cs:ip on trap frame is updated

        mov     eax,1
        ret

OpcodeINTnnV86 endp

        page   ,132
        subttl "INTO Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INTO opcode.  Currently, it prints
;       a message, and reflects a GP fault to the debugger.
;
;   Arguments:
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeINTOV86
OpcodeINTOV86 proc

        xor     eax,eax                 ; ret fail
        ret

OpcodeINTOV86 endp

        page   ,132
        subttl "IRET Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an IRET opcode.  It retrieves the flags,
;       and new instruction pointer from the stack and puts them into
;       the user context.
;
;
;   Arguments:
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;
;

        public OpcodeIRETV86
OpcodeIRETV86 proc

        mov     eax,_VdmFixedStateLinear         ; get pointer to VDM State
        movzx   ecx,word ptr [ebp].TsHardwareSegSS
        shl     ecx,4
        movzx   edx,word ptr [ebp].TsHardwareEsp    ; ebx+edx is user stack
        add     ecx,edx
        test    ebx,PREFIX_OPER32
        jnz     irt50                               ; normally not

        movzx   edi,word ptr [ecx]                  ; get ip value
        mov     [ebp].TsEip,edi
        movzx   esi,word ptr [ecx+2]                ; get cs value
        mov     [ebp].TsSegCs,esi
        add     edx,6
        mov     [ebp].TsHardwareEsp,edx             ; update sp on trap frame
        movzx   ebx,word ptr [ecx+4]                ; get flag value

irt10:
        and     ebx, NOT (EFLAGS_IOPL_MASK OR EFLAGS_NT_MASK)
        mov     ecx,ebx

        test    _KeI386VirtualIntExtensions, dword ptr V86_VIRTUAL_INT_EXTENSIONS
        jz      short irt15
        and     ebx, NOT EFLAGS_VIP                 ; no need for this now
        or      ebx, EFLAGS_VIF
        test    ebx, EFLAGS_INTERRUPT_MASK
        jnz     irt15
        and     ebx, NOT EFLAGS_VIF

irt15:
        or      ebx, (EFLAGS_V86_MASK OR EFLAGS_INTERRUPT_MASK)
        mov     [ebp].TsEFlags,ebx                  ; update flags n trap frame
        and     ecx, EFLAGS_INTERRUPT_MASK
        MPLOCK and [eax],NOT VDM_VIRTUAL_INTERRUPTS
        MPLOCK or [eax],ecx
        mov     ebx,[eax]


        ; at this point esi is the cs and edi is the ip where v86 mode
        ; will return. Now we will check if this returning instruction
        ; is a bop. if so we will directly dispatch the bop from here
        ; saving a full round trip. This will be really helpful to
        ; com apps.

        shl     esi,4
        add     esi,edi
        mov     ax, word ptr [esi]
        cmp     ax, 0c4c4h
        je      irtbop

        test    ebx,VDM_INTERRUPT_PENDING
        jz      short irt25

        test    ebx,VDM_VIRTUAL_INTERRUPTS
        jz      short irt25

        call    VdmDispatchIntAck       ; VdmDispatchIntAck enables interrupts

irt25:
        mov     eax,1                   ; handled
        ret

        ; ireting to a bop
irtbop:
        call    VdmDispatchBop          ; this expects ebp to be trap frame
        jmp     short irt25

irt50:
        mov     edi, [ecx]                          ; get ip value
        mov     [ebp].TsEip,edi
        movzx   esi,word ptr [ecx+4]                ; get cs value
        mov     [ebp].TsSegCs,esi
        add     edx,12
        mov     [ebp].TsHardwareEsp,edx             ; update sp on trap frame
        mov     ebx, [ecx+8]                        ; get flag value
        jmp     irt10                               ; rejoin the common path

OpcodeIRETV86 endp


        page   ,132
        subttl "In Byte Immediate Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an in byte immediate opcode.  Currently, it
;       prints a message, and ignores the instruction.
;
;   Arguments:
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeINBimmV86
OpcodeINBimmV86 proc

        inc     esi
        inc     edi
        movzx   ecx,byte ptr [esi]

        ; Ki386VdmDispatchIo enables interrupts
        stdCall   _Ki386VdmDispatchIo, <ecx, 1, TRUE, edi, ebp>
        ret

OpcodeINBimmV86 endp

        page   ,132
        subttl "Word In Immediate Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an in word immediate opcode.  Currently, it
;       prints a message, and ignores the instruction.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeINWimmV86
OpcodeINWimmV86 proc

        inc     esi
        inc     edi
        movzx   ecx,byte ptr [esi]
; edi - instruction size
; TRUE - read op
; 2 - word op
; ecx - port number
        ; Ki386VdmDispatchIo enables interrupts
        stdCall   _Ki386VdmDispatchIo, <ecx, 2, TRUE, edi, ebp>

        ret

OpcodeINWimmV86 endp

        page   ,132
        subttl "Out Byte Immediate Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an invalid opcode.  Currently, it prints
;       a message, and ignores the instruction.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeOUTBimmV86
OpcodeOUTBimmV86 proc

        inc     edi
        inc     esi
        movzx   ecx,byte ptr [esi]
; edi - instruction size
; FALSE - write op
; 1 - byte op
; ecx - port #
        ; Ki386VdmDispatchIo enables interrupts
        stdCall   _Ki386VdmDispatchIo, <ecx, 1, FALSE, edi, ebp>

        ret

OpcodeOUTBimmV86 endp

        page   ,132
        subttl "Out Word Immediate Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an out word immediate opcode.  Currently,
;       it prints a message, and ignores the instruction.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;
;

        public OpcodeOUTWimmV86
OpcodeOUTWimmV86 proc

        inc     esi
        inc     edi
        movzx   ecx,byte ptr [esi]
; edi - instruction size
; FALSE - write op
; 2 - word op
; ecx - port number
        ; Ki386VdmDispatchIo enables interrupts
        stdCall   _Ki386VdmDispatchIo, <ecx, 2, FALSE, edi, ebp>

        ret

OpcodeOUTWimmV86 endp

        page   ,132
        subttl "INB Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INB opcode.  Currently, it prints
;       a message, and ignores the instruction.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeINBV86
OpcodeINBV86 proc

        movzx   ebx,word ptr [ebp].TsEdx


; JAPAN - SUPPORT Intel CPU/Non PC/AT compatible machine
; Get Hardware Id, PC_AT_COMPATIBLE is 0x00XX
        test    _KeI386MachineType, MACHINE_TYPE_MASK
        jnz     oib_reflect

; edi - instruction size
; TRUE - read op
; 1 - byte op
; ebx - port number

        cmp     ebx, 3bdh
        jz      oib_prt1
        cmp     ebx, 379h
        jz      oib_prt1
        cmp     ebx, 279h
        jz      oib_prt1

oib_reflect:
        ; Ki386VdmDispatchIo enables interrupts
        stdCall   _Ki386VdmDispatchIo, <ebx, 1, TRUE, edi, ebp>
oib_com:
        ret

oib_prt1:
        ; call printer status routine with port number, size, trap frame
        stdCall _VdmPrinterStatus, <ebx, edi, ebp>
        or      al,al
        jz      short oib_reflect
        jmp     short oib_com

OpcodeINBV86 endp

        page   ,132
        subttl "INW Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INW opcode.  Currently, it prints
;       a message, and ignores the instruction.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;
;

        public OpcodeINWV86
OpcodeINWV86 proc

        movzx   ebx,word ptr [ebp].TsEdx

; edi - instruction size
; TRUE - read operation
; 2 - word op
; ebx - port number
        ; Ki386VdmDispatchIo enables interrupts
        stdCall   _Ki386VdmDispatchIo, <ebx, 2, TRUE, edi, ebp>

        ret
OpcodeINWV86 endp

        page   ,132
        subttl "OUTB Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an OUTB opcode.  Currently, it prints
;       a message, and ignores the instruction.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;
;

        public OpcodeOUTBV86
OpcodeOUTBV86 proc

        movzx   ebx,word ptr [ebp].TsEdx

; JAPAN - SUPPORT Intel CPU/Non PC/AT compatible machine
; Get Hardware Id, PC_AT_COMPATIBLE is 0x00XX
        test    _KeI386MachineType, MACHINE_TYPE_MASK
        jnz     oob_reflect

        cmp     ebx, 3bch
        jz      oob_prt1
        cmp     ebx, 378h
        jz      oob_prt1
        cmp     ebx, 278h
        jz      oob_prt1

oob_reflect:

; edi - instruction size
; FALSE - write op
; 1 - byte op
; ebx - port number
        ; Ki386VdmDispatchIo enables interrupts
        stdCall   _Ki386VdmDispatchIo, <ebx, 1, FALSE, edi, ebp>

        ret
oob_prt1:
        ; call printer write data routine with port number, size, trap frame
        stdCall _VdmPrinterWriteData, <ebx, edi, ebp>
        or      al,al
        jz      short oob_reflect
                                        ;al already has TRUE
        ret
OpcodeOUTBV86 endp

        page   ,132
        subttl "OUTW Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an OUTW opcode.  Currently, it prints
;       a message, and ignores the instruction.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;
;

        public OpcodeOUTWV86
OpcodeOUTWV86 proc

        movzx   ebx,word ptr [ebp].TsEdx
; edi - instruction size
; FALSE - write op
; 2 - word op
; ebx - port #
        ; Ki386VdmDispatchIo enables interrupts
        stdCall   _Ki386VdmDispatchIo, <ebx, 2, FALSE, edi, ebp>

        ret

OpcodeOUTWV86 endp


        page   ,132
        subttl "CLI Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an CLI opcode.  Currently, it prints
;       a message, and clears the virtual interrupt flag in the VdmTeb.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;
;

        public OpcodeCLIV86
OpcodeCLIV86 proc

        mov     eax,_VdmFixedStateLinear  ; get pointer to VDM State
        MPLOCK and dword ptr [eax],NOT VDM_VIRTUAL_INTERRUPTS
        add     dword ptr [ebp].TsEip,edi

        mov     eax,1
        ret

OpcodeCLIV86 endp

        page   ,132
        subttl "STI Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an STI opcode.  Currently, it prints
;       a message, and sets the virtual interrupt flag in the VDM teb.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;
;

        public OpcodeSTIV86
OpcodeSTIV86 proc

        mov     eax,_VdmFixedStateLinear  ; get pointer to VDM State
        test    _KeI386VirtualIntExtensions, dword ptr V86_VIRTUAL_INT_EXTENSIONS
        jz      short os10

        or      [ebp].TsEFlags, dword ptr EFLAGS_VIF

os10:   MPLOCK or dword ptr [eax],EFLAGS_INTERRUPT_MASK
os20:   add     dword ptr [ebp].TsEip,edi
        mov     eax,dword ptr [eax]
        test    eax,VDM_INTERRUPT_PENDING
        jz      short os30

        call    VdmDispatchIntAck
os30:   mov     eax,1
        ret

OpcodeSTIV86 endp


;
;  If we get here, we have executed an NPX instruction in user mode
;  with the emulator installed.  If the EM bit was not set in CR0, the
;  app really wanted to execute the instruction for detection purposes.
;  In this case, we need to clear the TS bit, and restart the instruction.
;  Otherwise we need to reflect the exception
;
;
; Reginfo structure
;

    public Opcode0FV86
Opcode0FV86 proc

RI      equ     [ebp - REGINFOSIZE]

        push    ebp
        mov     ebp,esp
        sub     esp,REGINFOSIZE
        push    esi
        push    edi


        ; Initialize RegInfo
do10:   mov     esi,[ebp]


        ; initialize rest of the trap from which was'nt initialized for
        ; v86 mode
        mov     eax, [esi].TsV86Es
        mov     [esi].TsSegEs,eax
        mov     eax, [esi].TsV86Ds
        mov     [esi].TsSegDs,eax
        mov     eax, [esi].TsV86Fs
        mov     [esi].TsSegFs,eax
        mov     eax, [esi].TsV86Gs
        mov     [esi].TsSegGs,eax

        mov     RI.RiTrapFrame,esi
        mov     eax,[esi].TsHardwareSegSs
        mov     RI.RiSegSs,eax
        mov     eax,[esi].TsHardwareEsp
        mov     RI.RiEsp,eax
        mov     eax,[esi].TsEFlags
        mov     RI.RiEFlags,eax
        mov     eax,[esi].TsSegCs
        mov     RI.RiSegCs,eax
        mov     eax,[esi].TsEip
        dec     edi
        add     eax,edi                 ; for prefixes
        mov     RI.RiEip,eax

        mov     RI.RiPrefixFlags,ebx
        lea     esi,RI

        CsToLinearV86
        call    VdmOpcode0f                             ; enables interrupts

        test    eax,0FFFFh
        jz      do20

        mov     edi,RI.RiTrapFrame
        mov     eax,RI.RiEip                            ; advance eip
        mov     [edi].TsEip,eax
do19:   mov     eax,1
do20:
        pop     edi
        pop     esi
        mov     esp,ebp
        pop     ebp
        ret

Opcode0FV86 endp


;++
;
;   Routine Description: VdmDispatchIntAck
;       pushes stack arguments for VdmDispatchInterrupts
;       and invokes VdmDispatchInterrupts
;
;       Expects VDM_INTERRUPT_PENDING, and VDM_VIRTUAL_INTERRUPTS
;
;   Arguments:
;       EBP -> trap frame
;
;   Returns:
;       nothing
;
;
        public VdmDispatchIntAck
VdmDispatchIntAck proc

        mov     eax,_VdmFixedStateLinear  ; get pointer to VDM State
        test    [eax],VDM_INT_HARDWARE       ; check for hardware int
        mov     eax,PCR[PcPrcbData+PbCurrentThread]
        mov     eax,[eax]+ThApcState+AsProcess
        mov     eax,[eax].EpVdmObjects
        mov     eax,[eax].VpVdmTib             ; get pointer to VdmTib
        jz      short dia20

        ;
        ; dispatch hardware int directly from kernel
        ;
        stdCall _VdmDispatchInterrupts, <ebp, eax>  ; TrapFrame, VdmTib
dia10:
        ret


        ;
        ; Switch to monitor context to dispatch timer int
        ;
dia20:
        mov     dword ptr [eax].VtEIEvent,VdmIntAck
        mov     dword ptr [eax].VtEIInstSize,0
        mov     dword ptr [eax].VtEiIntAckInfo,0
        stdCall _VdmEndExecution, <ebp, eax>        ; TrapFrame, VdmTib
        jmp short dia10

VdmDispatchIntAck endp


        public vdmDebugPoint
vdmDebugPoint proc
        ret
vdmDebugPoint endp



        page   ,132
        subttl "HLT Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an HLT opcode.  If the halt instruction is
;       followed by the magic number (to be found in a crackerjack box),
;       we use the hlt + magic number as a prefix, and emulate the following
;       instruction.  This allows code running in segmented protected mode to
;       access the virtual interrupt flag.
;
;   Arguments:
;       EAX -> pointer to vdm state in DOS arena
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       interrupts disabled
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;

        public OpcodeHLTV86
OpcodeHLTV86 proc

        add     dword ptr [ebp].TsEip,edi
        mov     eax,1
        ret

OpcodeHLTV86 endp

_PAGE   ends

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING

        subttl "NPX Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates all NPX opcodes, when the system
;       has the R3 emulator installed and the c86 apps takes a
;       trap07.
;
;   Arguments:
;       EBX -> prefix flags
;       EBP -> trap frame
;       CL  -> byte at the faulting address
;       ESI -> address of faulting instruction
;       EDI -> instruction length count
;
;   Returns:
;       EAX = 0 for failure
;       EAX = 1 for success
;
;   All registers can be trashed except ebp/esp.
;   moved from emv86.asm as it must be non-pagable

    public OpcodeNPXV86
OpcodeNPXV86 proc
        mov     edx, PCR[PcInitialStack]
        mov     edx, [edx].FpCr0NpxState
        test    edx, CR0_EM             ; Does app want NPX traps?
        jnz     short onp40

    ; MP bit can never be set while the EM bit is cleared, so we know
    ; the faulting instruction is not an FWAIT

onp30:  and     ebx, PREFIX_ADDR32
        stdCall _VdmSkipNpxInstruction, <ebp, ebx, esi, edi>
        or      al, al                  ; was it handled?
        jnz     short onp60             ; no, go raise exception to app

onp40:  stdCall _Ki386VdmReflectException, <7>  ; trap #

onp60:  mov     eax,1
        ret


OpcodeNPXV86 endp


;++ KiVdmSetUserCR0
;
;       eax
;
        public KiVdmSetUserCR0
KiVdmSetUserCR0 proc

        and     eax, CR0_MP OR CR0_EM       ; Sanitize parameter
        shr     eax, 1
        movzx   eax, _VdmUserCr0MapIn[eax]

        push    esp                         ; Pass current Esp to handler
        push    offset scr_fault            ; Set Handler address
        push    PCR[PcExceptionList]        ; Set next pointer
        mov     PCR[PcExceptionList],esp    ; Link us on

        mov     edx,PCR[PcPrcbData+PbCurrentThread]
        mov     edx,[edx]+ThApcState+AsProcess
        mov     edx,[edx].EpVdmObjects
        mov     edx,[edx].VpVdmTib             ; get pointer to VdmTib
        mov     [edx].VtVdmContext.CsFloatSave.FpCtxtCr0NpxState, eax

scr10:  pop     PCR[PcExceptionList]        ; Remove our exception handle
        add     esp, 8                      ; clear stack

        mov     edx, PCR[PcInitialStack]    ; Get fp save area
        mov     ebx, PCR[PcPrcbData + PbCurrentThread]  ; (ebx) = current thread

scr20:  cli                                 ; sync with context swap
        and     [edx].FpCr0NpxState, NOT (CR0_MP+CR0_EM+CR0_PE)
        or      [edx].FpCr0NpxState,eax     ; set fp save area bits

        mov     eax,cr0
        and     eax, NOT (CR0_MP+CR0_EM+CR0_TS) ; turn off bits we will change
        or      al, [ebx].ThNpxState        ; set scheduler bits
        or      eax,[edx].FpCr0NpxState     ; set user's bits
        mov     cr0,eax
        sti
        ret

scr_fault:
;
; WARNING: Here we directly unlink the exception handler from the
; exception registration chain.  NO unwind is performed.  We can take
; this short cut because we know that our handler is a leaf-node.
;

        mov     esp, [esp+8]            ; (esp)-> ExceptionList
        jmp     short scr10


KiVdmSetUserCR0 endp

_TEXT$00   ENDS

        end
