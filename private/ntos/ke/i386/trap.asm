     title  "Trap Processing"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    trap.asm
;
; Abstract:
;
;    This module implements the code necessary to field and process i386
;    trap conditions.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 4-Feb-1990
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
KERNELONLY  equ     1
include ks386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc
include i386\mi.inc
include ..\..\vdm\i386\vdm.inc
include ..\..\..\inc\vdmtib.inc
include fastsys.inc
        .list

FAST_BOP        equ      1
FAST_V86_TRAP   equ      1


        page ,132
        extrn   _KeI386FxsrPresent:BYTE
        extrn   ExpInterlockedPopEntrySListFault:DWORD
        extrn   ExpInterlockedPopEntrySListResume:DWORD
        extrn   _KeGdiFlushUserBatch:DWORD
        extrn   _KeTickCount:DWORD
        extrn   _ExpTickCountMultiplier:DWORD
        extrn   _KiDoubleFaultTSS:dword
        extrn   _KiNMITSS:dword
        extrn   _KeServiceDescriptorTable:dword
        extrn   _KiHardwareTrigger:dword
        extrn   _KiBugCheckData:dword
        extrn   _KdpOweBreakpoint:dword
        extrn   Ki386BiosCallReturnAddress:near
        extrn   _PoHiberInProgress:byte
        extrn   _KiI386PentiumLockErrataPresent:BYTE
        EXTRNP  _KiDeliverApc,3
        EXTRNP  KfRaiseIrql,1,IMPORT,FASTCALL
        EXTRNP  KfLowerIrql,1,IMPORT,FASTCALL
        EXTRNP  _KeGetCurrentIrql,0,IMPORT
        EXTRNP  _PsConvertToGuiThread,0
        EXTRNP  _ZwUnmapViewOfSection,2

        EXTRNP  _HalHandleNMI,1,IMPORT
        EXTRNP  _HalBeginSystemInterrupt,3,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2,IMPORT
        EXTRNP  _KiDispatchException,5
if DEVL
        EXTRNP  _PsWatchWorkingSet,3
        extrn   _PsWatchEnabled:byte
endif
        EXTRNP  _MmAccessFault,4
        extrn   _MmUserProbeAddress:DWORD
        EXTRNP  _KeBugCheck,1
        EXTRNP  _KeBugCheckEx,5
        EXTRNP  _KeTestAlertThread,1
        EXTRNP  _KiContinue,3
        EXTRNP  _KiRaiseException,5
        EXTRNP  _Ki386DispatchOpcode,0
        EXTRNP  _Ki386DispatchOpcodeV86,0
        EXTRNP  _VdmDispatchPageFault,3
        EXTRNP  _Ki386VdmReflectException,1
        EXTRNP  _Ki386VdmSegmentNotPresent,0
        extrn   _DbgPrint:proc
        EXTRNP  _KdSetOwedBreakpoints
        extrn   _KiFreezeFlag:dword
        EXTRNP  _Ki386CheckDivideByZeroTrap,1
        EXTRNP  _Ki386CheckDelayedNpxTrap,2
        extrn   SwapContext:near
        EXTRNP  _VdmDispatchIRQ13, 1

        extrn   VdmDispatchBop:near
        extrn   _KeI386VdmIoplAllowed:dword
        extrn   _KeI386VirtualIntExtensions:dword
        EXTRNP  _NTFastDOSIO,2
        EXTRNP  _NtSetLdtEntries,6
        extrn   OpcodeIndex:byte
        extrn   _KeFeatureBits:DWORD
        extrn   _KeServiceDescriptorTableShadow:dword
        extrn   _KiIgnoreUnexpectedTrap07:byte

; JAPAN - SUPPORT Intel CPU/Non PC/AT machine
        extrn   _VdmFixedStateLinear:dword

;
; Equates for exceptions which cause system fatal error
;

EXCEPTION_DIVIDED_BY_ZERO       EQU     0
EXCEPTION_DEBUG                 EQU     1
EXCEPTION_NMI                   EQU     2
EXCEPTION_INT3                  EQU     3
EXCEPTION_BOUND_CHECK           EQU     5
EXCEPTION_INVALID_OPCODE        EQU     6
EXCEPTION_NPX_NOT_AVAILABLE     EQU     7
EXCEPTION_DOUBLE_FAULT          EQU     8
EXCEPTION_NPX_OVERRUN           EQU     9
EXCEPTION_INVALID_TSS           EQU     0AH
EXCEPTION_SEGMENT_NOT_PRESENT   EQU     0BH
EXCEPTION_STACK_FAULT           EQU     0CH
EXCEPTION_GP_FAULT              EQU     0DH
EXCEPTION_RESERVED_TRAP         EQU     0FH
EXCEPTION_NPX_ERROR             EQU     010H
EXCEPTION_ALIGNMENT_CHECK       EQU     011H

;
; Exception flags
;

EXCEPT_UNKNOWN_ACCESS           EQU     0H
EXCEPT_LIMIT_ACCESS             EQU     10H

;
; Equates for some opcodes and instruction prefixes
;

IOPL_MASK                       EQU     3000H
IOPL_SHIFT_COUNT                EQU     12

;
; page fault read/write mask
;

ERR_0E_STORE                    EQU     2

;
; Debug register 6 (dr6) BS (single step) bit mask
;

DR6_BS_MASK                     EQU     4000H

;
; EFLAGS single step bit
;

EFLAGS_TF_BIT                   EQU     100h
EFLAGS_OF_BIT                   EQU     4000H

;
; The mask of selecot's table indicator (ldt or gdt)
;

TABLE_INDICATOR_MASK            EQU     4

;
; Opcode for Pop SegReg and iret instructions
;

POP_DS                          EQU     1FH
POP_ES                          EQU     07h
POP_FS                          EQU     0A10FH
POP_GS                          EQU     0A90FH
IRET_OP                         EQU     0CFH
CLI_OP                          EQU     0FAH
STI_OP                          EQU     0FBH
PUSHF_OP                        EQU     9CH
POPF_OP                         EQU     9DH
INTNN_OP                        EQU     0CDH
FRSTOR_ECX                      EQU     021DD9Bh
FWAIT_OP                        EQU     09bh

;
;   Force assume into place
;

_TEXT$00   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING
_TEXT$00   ENDS

_DATA   SEGMENT DWORD PUBLIC 'DATA'

;
; Definitions for gate descriptors
;

GATE_TYPE_386INT        EQU     0E00H
GATE_TYPE_386TRAP       EQU     0F00H
GATE_TYPE_TASK          EQU     0500H
D_GATE                  EQU     0
D_PRESENT               EQU     8000H
D_DPL_3                 EQU     6000H
D_DPL_0                 EQU     0

;
; Definitions for present 386 trap and interrupt gate attributes
;

D_TRAP032               EQU     D_PRESENT+D_DPL_0+D_GATE+GATE_TYPE_386TRAP
D_TRAP332               EQU     D_PRESENT+D_DPL_3+D_GATE+GATE_TYPE_386TRAP
D_INT032                EQU     D_PRESENT+D_DPL_0+D_GATE+GATE_TYPE_386INT
D_INT332                EQU     D_PRESENT+D_DPL_3+D_GATE+GATE_TYPE_386INT
D_TASK                  EQU     D_PRESENT+D_DPL_0+D_GATE+GATE_TYPE_TASK

;
;       This is the protected mode interrupt descriptor table.
;

if DBG
;
; NOTE - embedded enlish messages won't fly for NLS! (OK for debug code only)
;

BadInterruptMessage db 0ah,7,7,'!!! Unexpected Interrupt %02lx !!!',0ah,00

KiNMIMessage        db 0ah,'Non-Maskable-Interrupt (NMI) EIP = %08lx',0ah,00

Ki16BitStackTrapMessage  db 0ah,'Exception inside of 16bit stack',0ah,00

public KiBiosReenteredAssert
KiBiosReenteredAssert   db 0ah,'Bios has been re-entered. Not safe. ',0ah,00
endif

;++
;
;   DEFINE_SINGLE_EMPTY_VECTOR - helper for DEFINE_EMPTY_VECTORS
;
;--

DEFINE_SINGLE_EMPTY_VECTOR macro    number
IDTEntry    _KiUnexpectedInterrupt&number, D_INT032
_TEXT$00   SEGMENT
        public  _KiUnexpectedInterrupt&number
_KiUnexpectedInterrupt&number proc
        push    dword ptr (&number + PRIMARY_VECTOR_BASE)
        ;;  jmp     _KiUnexpectedInterruptTail        ; replaced with following jmp which will then jump
        jmp _KiAfterUnexpectedRange                   ; to the _KiUnexpectedInterruptTail location
                                                      ; in a manner suitable for BBT, which needs to treat
                                                      ; this whole set of KiUnexpectedInterrupt&number
                                                      ; vectors as DATA, meaning a relative jmp will not
                                                      ; be adjusted properly in the BBT Instrumented or
                                                      ; Optimized code.
                                                      ;

_KiUnexpectedInterrupt&number endp
_TEXT$00   ENDS

        endm

FPOFRAME macro a, b
.FPO ( a, b, 0, 0, 0, FPO_TRAPFRAME )
endm

FXSAVE_ESI  macro
    db  0FH, 0AEH, 06
endm

FXSAVE_ECX  macro
    db  0FH, 0AEH, 01
endm

FXRSTOR_ECX macro
    db  0FH, 0AEH, 09
endm

;++
;
;   DEFINE_EMPTY_VECTORS emits an IDTEntry macro (and thus and IDT entry)
;   into the data segment.  It then emits an unexpected interrupt target
;   with push of a constant into the code segment.  Labels in the code
;   segment are defined to bracket the unexpected interrupt targets so
;   that KeConnectInterrupt can correctly test for them.
;
;   Empty vectors will be defined from 30 to ff, which is the hardware
;   vector set.
;
;--

NUMBER_OF_IDT_VECTOR    EQU     0ffH

DEFINE_EMPTY_VECTORS macro

;
;   Set up
;

        empty_vector = 00H

_TEXT$00   SEGMENT
IFDEF STD_CALL
        public  _KiStartUnexpectedRange@0
_KiStartUnexpectedRange@0   equ     $
ELSE
        public  _KiStartUnexpectedRange
_KiStartUnexpectedRange     equ     $
ENDIF
_TEXT$00   ENDS

        rept (NUMBER_OF_IDT_VECTOR - (($ - _IDT)/8)) + 1

        DEFINE_SINGLE_EMPTY_VECTOR  %empty_vector
        empty_vector = empty_vector + 1

        endm    ;; rept

_TEXT$00   SEGMENT
IFDEF STD_CALL
        public  _KiEndUnexpectedRange@0
_KiEndUnexpectedRange@0     equ     $
ELSE
        public  _KiEndUnexpectedRange
_KiEndUnexpectedRange       equ     $
ENDIF


        ;; added by to handle BBT unexpected interrupt problem
        ;;
_KiAfterUnexpectedRange     equ     $               ;;  BBT
        jmp     [KiUnexpectedInterruptTail]         ;;  BBT

KiUnexpectedInterruptTail dd offset _KiUnexpectedInterruptTail   ;;  BBT

        public _KiBBTUnexpectedRange
_KiBBTUnexpectedRange     equ     $               ;;  BBT

_TEXT$00   ENDS

        endm    ;; DEFINE_EMPTY_VECTORS macro

IDTEntry macro  name,access
        dd      offset FLAT:name
        dw      access
        dw      KGDT_R0_CODE
        endm

INIT    SEGMENT DWORD PUBLIC 'CODE'

;
; The IDT table is put into the INIT code segment so the memory
; can be reclaimed afer bootup
;

ALIGN 4
                public  _IDT, _IDTLEN, _IDTEnd
_IDT            label byte

IDTEntry        _KiTrap00, D_INT032             ; 0: Divide Error
IDTEntry        _KiTrap01, D_INT032             ; 1: DEBUG TRAP
IDTEntry        _KiTrap02, D_INT032             ; 2: NMI/NPX Error
IDTEntry        _KiTrap03, D_INT332             ; 3: Breakpoint
IDTEntry        _KiTrap04, D_INT332             ; 4: INTO
IDTEntry        _KiTrap05, D_INT032             ; 5: BOUND/Print Screen
IDTEntry        _KiTrap06, D_INT032             ; 6: Invalid Opcode
IDTEntry        _KiTrap07, D_INT032             ; 7: NPX Not Available
IDTEntry        _KiTrap08, D_INT032             ; 8: Double Exception
IDTEntry        _KiTrap09, D_INT032             ; 9: NPX Segment Overrun
IDTEntry        _KiTrap0A, D_INT032             ; A: Invalid TSS
IDTEntry        _KiTrap0B, D_INT032             ; B: Segment Not Present
IDTEntry        _KiTrap0C, D_INT032             ; C: Stack Fault
IDTEntry        _KiTrap0D, D_INT032             ; D: General Protection
IDTEntry        _KiTrap0E, D_INT032             ; E: Page Fault
IDTEntry        _KiTrap0F, D_INT032             ; F: Intel Reserved

IDTEntry        _KiTrap10, D_INT032             ;10: 486 coprocessor error
IDTEntry        _KiTrap11, D_INT032             ;11: 486 alignment
IDTEntry        _KiTrap0F, D_INT032             ;12: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;13: XMMI unmasked numeric exception
IDTEntry        _KiTrap0F, D_INT032             ;14: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;15: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;16: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;17: Intel Reserved

IDTEntry        _KiTrap0F, D_INT032             ;18: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;19: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;1A: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;1B: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;1C: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;1D: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;1E: Intel Reserved
IDTEntry        _KiTrap0F, D_INT032             ;1F: Reserved for APIC

;
; Note IDTEntry 0x21 is reserved for WOW apps.
;

        rept 2AH - (($ - _IDT)/8)
IDTEntry        0, 0                            ;invalid IDT entry
        endm
IDTEntry        _KiGetTickCount,  D_INT332          ;2A: KiGetTickCount service
IDTEntry        _KiCallbackReturn,  D_INT332        ;2B: KiCallbackReturn
IDTEntry        _KiSetLowWaitHighThread,  D_INT332  ;2C: KiSetLowWaitHighThread service
IDTEntry        _KiDebugService,  D_INT332          ;2D: debugger calls
IDTEntry        _KiSystemService, D_INT332          ;2E: system service calls
IDTEntry        _KiTrap0F, D_INT032                 ;2F: Reserved for APIC

;
;   Generate per-vector unexpected interrupt entries for 30 - ff
;
        DEFINE_EMPTY_VECTORS

_IDTLEN         equ     $ - _IDT
_IDTEnd         equ     $

INIT    ends

                public  _KiUnexpectedEntrySize
_KiUnexpectedEntrySize          dd  _KiUnexpectedInterrupt1 - _KiUnexpectedInterrupt0

;
; defines all the possible instruction prefix
;

PrefixTable     label   byte
        db      0f2h                    ; rep prefix
        db      0f3h                    ; rep ins/outs prefix
        db      67h                     ; addr prefix
        db      0f0h                    ; lock prefix
        db      66h                     ; operand prefix
        db      2eh                     ; segment override prefix:cs
        db      3eh                     ; ds
        db      26h                     ; es
        db      64h                     ; fs
        db      65h                     ; gs
        db      36h                     ; ss

PREFIX_REPEAT_COUNT     EQU     11      ; Prefix table length

;
; defines all the possible IO privileged IO instructions
;

IOInstructionTable      label byte
;       db      0fah                    ; cli
;       db      0fdh                    ; sti
        db      0e4h, 0e5h, 0ech, 0edh  ; IN
        db      6ch, 6dh                ; INS
        db      0e6h, 0e7h, 0eeh, 0efh  ; OUT
        db      6eh, 6fh                ; OUTS

IO_INSTRUCTION_TABLE_LENGTH     EQU     12

if FAST_V86_TRAP

ALIGN 4
;       V86DispatchTable - table of routines used to emulate instructions
;                          in v86 mode.

dtBEGIN V86DispatchTable,V86PassThrough
        dtS    VDM_INDEX_PUSHF           , V86Pushf
        dtS    VDM_INDEX_POPF            , V86Popf
        dtS    VDM_INDEX_INTnn           , V86Intnn
        dtS    VDM_INDEX_IRET            , V86Iret
        dtS    VDM_INDEX_CLI             , V86Cli
        dtS    VDM_INDEX_STI             , V86Sti
dtEND   MAX_VDM_INDEX

endif   ; FAST_V86_TRAP

;
; definition for  floating status word error mask
;

FSW_INVALID_OPERATION   EQU     1
FSW_DENORMAL            EQU     2
FSW_ZERO_DIVIDE         EQU     4
FSW_OVERFLOW            EQU     8
FSW_UNDERFLOW           EQU     16
FSW_PRECISION           EQU     32
FSW_STACK_FAULT         EQU     64
FSW_CONDITION_CODE_0    EQU     100H
FSW_CONDITION_CODE_1    EQU     200H
FSW_CONDITION_CODE_2    EQU     400H
FSW_CONDITION_CODE_3    EQU     4000H
_DATA   ENDS

_TEXT$00   SEGMENT
        ASSUME  DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

        page , 132
        subttl "Macro to Handle v86 trap d"
;++
;
; Macro Description:
;
;    This macro is a fast way to handle v86 bop instructions.
;    Note, all the memory write operations in this macro are done in such a
;    way that if a page fault occurs the memory will still be in a consistent
;    state.
;
;    That is, we must process the trapped instruction in the following order:
;
;    1. Read and Write user memory
;    2. Update VDM state flags
;    3. Update trap frame
;
; Arguments:
;
;    interrupts disabled
;
; Return Value:
;
;--

FAST_V86_TRAP_6  MACRO

local   DoFastIo, a, b

BOP_FOR_FASTWRITE       EQU     4350C4C4H
BOP_FOR_FASTREAD        EQU     4250C4C4H
TRAP6_IP                EQU     32              ; 8 * 4
TRAP6_CS                EQU     36              ; 8 * 4 + 4
TRAP6_FLAGS             EQU     40              ; 8 * 4 + 8
TRAP6_SP                EQU     44              ; 8 * 4 + 12
TRAP6_SS                EQU     48              ; 8 * 4 + 16
TRAP6_ES                EQU     52
TRAP6_DS                EQU     56
TRAP6_FS                EQU     60
TRAP6_GS                EQU     64
TRAP6_EAX               EQU     28
TRAP6_EDX               EQU     20

        pushad          ;eax, ecx, edx, ebx, old esp, ebp, esi, edi
        mov     eax, KGDT_R3_DATA OR RPL_MASK
        mov     ds, ax
        mov     es, ax

ifdef NT_UP
else
        mov     eax, KGDT_R0_PCR
        mov     fs, ax
endif
        mov     byte ptr PCR[PcVdmAlert], 6

        mov     ax, word ptr [esp+TRAP6_CS] ; [eax] = v86 user cs
        shl     eax, 4
        add     eax, [esp+TRAP6_IP]      ; [eax] = addr of BOP
        mov     edx, [eax]      ; [edx] = xxxxc4c4  bop + maj bop # + mi #
        cmp     edx, BOP_FOR_FASTREAD
        je      DoFastIo

        cmp     edx, BOP_FOR_FASTWRITE
        je      DoFastIo

        cmp     dx, 0c4c4h      ; Is it a bop?
        jne     V86Trap6PassThrough ; It's an error condition

        mov     eax,PCR[PcPrcbData+PbCurrentThread]
        shr     edx, 16
        mov     eax,[eax]+ThApcState+AsProcess
        mov     eax,[eax].EpVdmObjects
        mov     eax,[eax].VpVdmTib             ; get pointer to VdmTib
        and     edx, 0ffh
        mov     dword ptr [eax].VtEIEvent, VdmBop
        mov     dword ptr [eax].VtEIBopNumber, edx
        mov     dword ptr [eax].VtEIInstSize, 3
        lea     eax, [eax].VtVdmContext

;
;       Save V86 state to Vdm structure
;
        mov     edx, [esp+TRAP6_EDX]  ; get edx
        mov     [eax].CsEcx, ecx
        mov     [eax].CsEbx, ebx      ; Save non-volatile registers
        mov     [eax].CsEsi, esi
        mov     [eax].CsEdi, edi
        mov     ecx, [esp+TRAP6_EAX]  ; Get eax
        mov     [eax].CsEbp, ebp
        mov     [eax].CsEdx, edx
        mov     [eax].CsEax, ecx

        mov     ebx, [esp]+TRAP6_IP   ; (ebx) = iser ip
        mov     ecx, [esp]+TRAP6_CS   ; (ecx) = user cs
        mov     esi, [esp]+TRAP6_SP   ; (esi) = user esp
        mov     edi, [esp]+TRAP6_SS   ; (edi) = user ss
        mov     edx, [esp]+TRAP6_FLAGS; (edx) = user eflags
        mov     [eax].CsEip, ebx
        mov     [eax].CsSegCs, ecx
        mov     [eax].CsEsp, esi
        mov     [eax].CsSegSs, edi
        test    _KeI386VirtualIntExtensions, V86_VIRTUAL_INT_EXTENSIONS
        jz      short @f

        test    edx, EFLAGS_VIF
        jnz     short a

        and     edx, NOT EFLAGS_INTERRUPT_MASK
        jmp     short a

@@:     test    _KeI386VdmIoplAllowed, 0ffffffffh
        jnz     short a

        mov     ebx, _VdmFixedStateLinear    ; load ntvdm address
        test    ds:[ebx], VDM_VIRTUAL_INTERRUPTS ; check interrupt
        jnz     short a

        and     edx, NOT EFLAGS_INTERRUPT_MASK
a:
        mov     [eax].CsEFlags, edx
        mov     ebx, [esp]+TRAP6_DS   ; (ebx) = user ds
        mov     ecx, [esp]+TRAP6_ES   ; (ecx) = user es
        mov     edx, [esp]+TRAP6_FS   ; (edx) = user fs
        mov     esi, [esp]+TRAP6_GS   ; (esi) = user gs
        mov     [eax].CsSegDs, ebx
        mov     [eax].CsSegEs, ecx
        mov     [eax].CsSegFs, edx
        mov     [eax].CsSegGs, esi

;
;       Load Monitor context
;

        add     eax, VtMonitorContext - VtVdmContext ; (eax)->monitor context
        mov     ebx, [eax].CsSegSs
        mov     esi, [eax].CsEsp
        mov     edi, [eax].CsEFlags
        mov     edx, [eax].CsSegCs
        mov     ecx, [eax].CsEip
        sub     esp, 20          ; allocate stack space
        mov     [esp + 16], ebx  ; Build Iret frame (can not single step!)
        mov     [esp + 12], esi
        mov     [esp + 8], edi
        mov     [esp + 4], edx
        mov     [esp + 0], ecx
        mov     ebx, [eax].CsEbx ; We don't need to load volatile registers.
        mov     esi, [eax].CsEsi ; because monitor uses SystemCall to return
        mov     edi, [eax].CsEdi ; back to v86.  C compiler knows that
        mov     ebp, [eax].CsEbp ; SystemCall does not preserve volatile
                                 ; registers.
                                 ; fs, ds are set up already.

;
; Adjust Tss esp0 value and set return value to SUCCESS
;
        mov     ecx, PCR[PcPrcbData+PbCurrentThread]
        mov     ecx, [ecx].thInitialStack
        mov     edx, PCR[PcTss]
        sub     ecx, NPX_FRAME_LENGTH + TsV86Gs - TsHardwareSegSs
        xor     eax, eax         ; ret status = SUCCESS
        mov     [edx].TssEsp0, ecx
        mov     byte ptr PCR[PcVdmAlert], al
        mov     edx, KGDT_R3_TEB OR RPL_MASK
        mov     fs, dx
        iretd

DoFastIo:
        xor     eax, eax
        mov     edx, [esp]+TRAP6_EDX    ; Restore edx
        add     esp, 7 * 4              ; leave eax in the TsErrCode
        xchg    [esp], eax              ; Restore eax, sore a zero errcode
        sub     esp, TsErrcode          ; build a trap frame
        mov     [esp].TsEbx, ebx
        mov     [esp].TsEax, eax
        mov     [esp].TsEbp, ebp
        mov     [esp].TsEsi, esi
        mov     [esp].TsEdi, edi
        mov     [esp].TsEcx, ecx
        mov     [esp].TsEdx, edx
if DBG
        mov     [esp].TsPreviousPreviousMode, -1
        mov     [esp]+TsDbgArgMark, 0BADB0D00h
endif
ifdef NT_UP
        mov     ebx, KGDT_R0_PCR
        mov     fs, bx
endif
        mov     byte ptr PCR[PcVdmAlert], 0
        mov     ebp, esp
        cld
        test    byte ptr PCR[PcDebugActive], -1
        jz      short @f

        mov     ebx,dr0
        mov     esi,dr1
        mov     edi,dr2
        mov     [ebp]+TsDr0,ebx
        mov     [ebp]+TsDr1,esi
        mov     [ebp]+TsDr2,edi
        mov     ebx,dr3
        mov     esi,dr6
        mov     edi,dr7
        mov     [ebp]+TsDr3,ebx
        mov     [ebp]+TsDr6,esi
        mov     [ebp]+TsDr7,edi
        ;
        ; Load KernelDr* into processor
        ;
        mov     edi,dword ptr fs:[PcPrcb]
        mov     ebx,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr0
        mov     esi,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr1
        mov     dr0,ebx
        mov     dr1,esi
        mov     ebx,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr2
        mov     esi,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr3
        mov     dr2,ebx
        mov     dr3,esi
        mov     ebx,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr6
        mov     esi,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr7
        mov     dr6,ebx
        mov     dr7,esi
@@:
        ; Raise Irql to APC level before enabling interrupts
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql
        push    eax                     ; Save OldIrql
        sti

        xor     edx, edx
        mov     dx, word ptr [ebp].TsSegCs
        shl     edx, 4
        xor     ebx, ebx
        add     edx, [ebp].TsEip
        mov     bl, [edx+3]             ; [bl] = minor BOP code
        push    ebx
        push    ebp                     ; (ebp)->TrapFrame
        call    _NTFastDOSIO@8
        jmp     Kt061i

V86Trap6PassThrough:
        mov     byte ptr PCR[PcVdmAlert], 0
V86Trap6Recovery:
        popad
        jmp     Kt6SlowBop              ; Fall through

endm
        page , 132
        subttl "Macro to Handle v86 trap d"
;++
;
; Macro Description:
;
;    This macro is a fast way to handle SOME v86 mode sensitive instructions.
;    Note, all the memory write operations in this macro are done in such a
;    way that if a page fault occurs the memory will still be in a consistent
;    state.
;
;    That is, we must process the trapped instruction in the following order:
;
;    1. Write user memory (prefer do read here)
;    2. Update VDM state flags
;    3. Update trap frame
;
; Arguments:
;
;    interrupts disabled
;
; Return Value:
;
;--

FAST_V86_TRAP_D  MACRO

local   V86Exit, V86Exit_1, IntnnExit

        sub     esp, TsErrCode
        mov     [esp].TsEdx, edx
        mov     [esp].TsEcx, ecx
        mov     [esp].TsEbx, ebx
        mov     [esp].TsEax, eax

        mov     ebx, _VdmFixedStateLinear

ifdef NT_UP
        mov     byte ptr SS:[P0PCRADDRESS][PcVdmAlert], 0dh
else
        mov     eax, KGDT_R0_PCR
        mov     fs, ax
        mov     byte ptr PCR[PcVdmAlert], 0dh
endif
        mov     eax, [esp].TsSegCs      ; (eax) = H/W Cs
        shl     eax,4
        add     eax,[esp].TsEip         ; (eax) -> flat faulted addr
        xor     edx, edx
        mov     ecx, ss:[eax]           ; (ecx) = faulted instruction
        mov     dl, cl
        mov     dl, ss:OpcodeIndex[edx]    ; (edx) =  opcode index
        jmp     ss:V86DispatchTable[edx * type V86DispatchTable]

;
; (ecx) = faulted instructions
; (eax) = Flat faulted addr
; (edx) = Opcode Index
;

ALIGN 4
V86Pushf:
        mov     eax, ss:[ebx]                 ; get ntvdm address
        mov     edx, dword ptr [esp].TsEflags ; (edx) = Hardware Eflags
        and     eax,VDM_VIRTUAL_INTERRUPTS OR VDM_VIRTUAL_AC OR VDM_VIRTUAL_NT
        or      eax,EFLAGS_IOPL_MASK
        and     edx,NOT EFLAGS_INTERRUPT_MASK
        mov     [esp].TsSegDs, ecx      ; save ecx
        or      eax,edx                 ; (ax) = client flags
        xor     ecx, ecx
        mov     cx, word ptr [esp].TsHardwareSegSs ; (edx)= hardware SS
        xor     edx, edx
        shl     ecx,4
        mov     dx, word ptr [esp].TsHardwareEsp ; (edx)= Hardware sp
        sub     edx, 2
        mov     ss:[ecx + edx],ax
        mov     ecx, [esp].TsSegDs      ; restore ecx
;
; Usually, pushf is followed by cli.  So, here we check for this case.
; If yes, we will handle the cli to save a round trip.
; It is very important that we first update user stack, Fixed VDM state and
; finally hardware esp.
;

        cmp     cx, (CLI_OP SHL 8) OR PUSHF_OP ; Is there a cli following pushf?
        jnz     short @f

        MPLOCK and dword ptr ss:[ebx],NOT VDM_VIRTUAL_INTERRUPTS
        inc     dword ptr [esp].TsEip ; skip cli
@@:
        mov     word ptr [esp].TsHardwareEsp, dx ; update client esp
V86Exit:
        inc     dword ptr [esp].TsEip          ; skip pushf
V86Exit_1:
        mov     ecx, [esp].TsEcx
        mov     ebx, [esp].TsEbx
        mov     eax, [esp].TsEax
        mov     edx, [esp].TsEdx
        add     esp, TsEip
ifdef NT_UP
        mov     byte ptr SS:[P0PCRADDRESS][PcVdmAlert], 0
else
        mov     byte ptr PCR[PcVdmAlert], 0
endif
        iretd

ALIGN 4
V86Cli:
        MPLOCK and dword ptr ss:[ebx],NOT VDM_VIRTUAL_INTERRUPTS
        jmp     short V86Exit

ALIGN 4
V86Sti:
        test    ss:[ebx], VDM_INTERRUPT_PENDING
                                          ; Can we handle it in fast way?
        jnz     V86PassThrough            ; if nz, no, we need to dispatch int

        ;; Pentium CPU traps sti if
        ;; 1). client's TF is ON or 2). VIP is ON
        ;; we must set EFLAGS_VIF in this case.
        test    dword ptr _KeI386VirtualIntExtensions, V86_VIRTUAL_INT_EXTENSIONS
        jz      short v86_sti_01
        or      dword ptr [esp].TsEflags, EFLAGS_VIF
v86_sti_01:
        MPLOCK or dword ptr ss:[ebx], EFLAGS_INTERRUPT_MASK
        jmp     short V86Exit

ALIGN 4
V86Popf:
        test    ss:[ebx], VDM_INTERRUPT_PENDING
                                          ; Can we handle it in fast way?
        jnz     V86PassThrough

        xor     edx, edx
        mov     dx, word ptr [esp].TsHardwareSegSs ; (edx)= hardware SS
        xor     eax, eax
        shl     edx,4
        mov     ax, word ptr [esp].TsHardwareEsp ; (ecx)= Hardware sp
        mov     edx, ss:[edx + eax]       ; (edx) = Client flags
        add     ax, 2                     ; (ax) = Client sp
        and     edx, 0FFFFH AND (NOT EFLAGS_IOPL_MASK)
        MPLOCK and ss:[ebx],NOT (EFLAGS_INTERRUPT_MASK OR EFLAGS_ALIGN_CHECK OR EFLAGS_NT_MASK)
        mov     ecx, edx
        and     edx, (EFLAGS_INTERRUPT_MASK OR EFLAGS_ALIGN_CHECK OR EFLAGS_NT_MASK)
        and     ecx, NOT EFLAGS_NT_MASK
        MPLOCK or ss:[ebx],edx
        or      ecx, (EFLAGS_INTERRUPT_MASK OR EFLAGS_V86_MASK)

        ;; Pentium CPU traps popf if
        ;; 1)client's TF is ON or 2) VIP is on and the client's IF is ON
        ;; We have to propagate IF to VIF if virtual interrupt extension is
        ;; enabled.
        test    dword ptr _KeI386VirtualIntExtensions, V86_VIRTUAL_INT_EXTENSIONS
        jz      v86_popf_01
        and     ecx, NOT EFLAGS_VIF             ;clear it first
        and     edx, EFLAGS_INTERRUPT_MASK      ;isolate and move IF to VIF
        rol     edx, 10                         ;position
.errnz  (EFLAGS_INTERRUPT_MASK SHL 10) - EFLAGS_VIF
        or      ecx, edx                        ;propagate it!

v86_popf_01:
        mov     [esp].TsEflags,ecx
        mov     [esp].TsHardwareEsp, eax  ; update client esp
        jmp     V86Exit

ALIGN 4
V86Intnn:
        shr     ecx, 8
        and     ecx, 0FFH                   ; ecx is int#

        mov     eax,PCR[PcPrcbData+PbCurrentThread]
        mov     eax,[eax]+ThApcState+AsProcess
        mov     eax,[eax].EpVdmObjects
        mov     eax,[eax].VpVdmTib             ; get pointer to VdmTib
        lea     eax,[eax].VtInterruptHandlers[ecx*8]
        test    [eax].ViFlags, VDM_INT_HOOKED    ; need to reflect to PM?
        jnz     V86PassThrough            ; if nz, no, we need to dispatch int

        xor     eax, eax
        mov     ecx, ss:[ecx*4]             ; (ecx) = Int nn handler
        xor     edx, edx
        mov     [esp].TsSegDs, ecx          ; [esp].Ds = intnn handler
        mov     ax, word ptr [esp].TsHardwareSegSs ; (eax)= hardware SS
        shl     eax,4
        mov     dx, word ptr [esp].TsHardwareEsp ; (edx)= Hardware sp
        sub     dx, 6
        add     eax, edx                    ; (eax) = User stack
        mov     ecx, [esp].TsEflags
        test    ss:_KeI386VdmIoplAllowed,1
        jnz     short @f

        mov     edx, ss:[ebx]              ; get vdm states
        and     ecx, NOT EFLAGS_INTERRUPT_MASK
.errnz (EFLAGS_INTERRUPT_MASK - VDM_VIRTUAL_INTERRUPTS)
        and     edx,VDM_VIRTUAL_INTERRUPTS OR VDM_VIRTUAL_AC
        test    _KeI386VirtualIntExtensions, dword ptr V86_VIRTUAL_INT_EXTENSIONS
        jz      IntnnMergeIF
;VIF extension is enabled, we should migrate EFLAGS_VIF instead of
;VDM_VIRTUAL_INTERRUPT to the iret frame eflags IF.
;When VIF extension is enabled, RI_BIT_MASK is turned on. This in turn,
;redirects the FCLI/FSTI macro to execute cli/sti directly instead
;of simulation. Without this, we might disable v86 mode interrupt
;without the applications knowing it.
;
        and     edx, VDM_VIRTUAL_AC         ;keep VDM_VIRTUAL_AC only
        or      ecx, edx                    ;eflags + ac (IF is off)
        mov     edx, ecx
        and     edx, EFLAGS_VIF
.errnz  ((EFLAGS_VIF SHR 10) - EFLAGS_INTERRUPT_MASK)
        ror     edx, 10                     ;VIF -> IF
IntnnMergeIF:
        or      ecx, edx                    ;merge IF
        or      ecx, IOPL_MASK
        mov     word ptr ss:[eax+4], cx     ; push flags
        mov     ecx, [esp].TsSegCs
        mov     edx,  [esp].TsEip
        mov     word ptr ss:[eax+2], cx     ; push cs
        add     dx, 2
        MPLOCK and ss:[ebx], NOT VDM_VIRTUAL_INTERRUPTS
        mov     word ptr ss:[eax], dx       ; push ip (skip int nn)
        and     [esp].TsEflags, NOT (EFLAGS_NT_MASK OR EFLAGS_TF_MASK)
IntnnExit:
        mov     ecx, [esp].TsSegDs          ; (ecx) = V86 intnn handler
        sub     word ptr [esp].TsHardwareEsp, 6
        mov     [esp].TsEip, ecx
        shr     ecx,16
        mov     [esp].TsSegCs, cx           ; cs:ip on trap frame is updated
        jmp     V86Exit_1

@@:
        or      ecx, IOPL_MASK
        mov     word ptr ss:[eax+4], cx     ; push flags
        mov     ecx, [esp].TsSegCs
        mov     edx, [esp].TsEip
        mov     word ptr ss:[eax+2], cx     ; push cs
        add     dx, 2
        mov     word ptr ss:[eax], dx       ; push ip (skip int nn)
        and     [esp].TsEflags, NOT (EFLAGS_INTERRUPT_MASK OR EFLAGS_NT_MASK OR EFLAGS_TF_MASK)
        jmp     short IntnnExit

ALIGN 4
V86Iret:
        test    ss:[ebx], VDM_INTERRUPT_PENDING
        jnz     V86PassThrough

        xor     ecx, ecx
        mov     cx,word ptr [esp].TsHardwareSegSS
        xor     edx, edx
        shl     ecx,4
        mov     dx,word ptr [esp].TsHardwareEsp
        add     ecx,edx                            ; (ecx) -> User stack
        mov     dx,word ptr ss:[ecx+4]             ; get flag value
        mov     ecx, ss:[ecx]                      ; (ecx) = ret cs:ip

        and     edx, NOT (EFLAGS_IOPL_MASK OR EFLAGS_NT_MASK)
        mov     eax,edx
        or      edx, (EFLAGS_V86_MASK OR EFLAGS_INTERRUPT_MASK)
        and     eax, EFLAGS_INTERRUPT_MASK
        MPLOCK and ss:[ebx],NOT VDM_VIRTUAL_INTERRUPTS
        MPLOCK or ss:[ebx],eax

        ;; Pentium CPU traps iret if
        ;; 1)client's TF is ON or 2) VIP is on and the client's IF is ON
        ;; We have to propagate IF to VIF if virtual interrupt extension is
        ;; enabled
        test    dword ptr _KeI386VirtualIntExtensions, V86_VIRTUAL_INT_EXTENSIONS
        jz      v86_iret_01
        and     edx, NOT EFLAGS_VIF             ;eax must contain ONLY
                                                ;INTERRUPT_MASK!!!!
.errnz  (EFLAGS_INTERRUPT_MASK SHL 10) - EFLAGS_VIF
        rol     eax, 10
        or      edx, eax

v86_iret_01:
        mov     [esp].TsEFlags,edx                 ; update flags in trap frame
        mov     eax, ecx
        shr     ecx, 16
        and     eax, 0ffffh
        add     word ptr [esp].TsHardwareEsp, 6    ; update sp on trap frame
        mov     [esp].TsSegCs,ecx                  ; update cs
        mov     [esp].TsEip, eax                   ; update ip

        ; at this point cx:ax is the addr of the ip where v86 mode
        ; will return. Now we will check if this returning instruction
        ; is a bop. if so we will directly dispatch the bop from here
        ; saving a full round trip. This will be really helpful to
        ; com apps.
ifdef NT_UP
        mov     byte ptr SS:[P0PCRADDRESS][PcVdmAlert], 10h
else
        mov     byte ptr PCR[PcVdmAlert], 010h
endif
        shl     ecx, 4
        mov     ax, ss:[ecx+eax]                ; Could fault
        cmp     ax, 0c4c4h
        jne     V86Exit_1

        mov     ecx, [esp].TsEcx
        mov     ebx, [esp].TsEbx
        mov     eax, [esp].TsEax
        mov     edx, [esp].TsEdx
        add     esp, TsEip
if FAST_BOP eq 0
ifdef NT_UP
        mov     byte ptr SS:[P0PCRADDRESS][PcVdmAlert], 0
else
        mov     byte ptr PCR[PcVdmAlert], 0
endif
endif
        jmp     _KiTrap06

V86Trap10Recovery:
        jmp     V86Exit_1

;
; If we come here, it means we hit some kind of trap while processing the
; V86 trap in a fast way.  We need to process the instruction in a normal
; way.  For this case, we simply abort the current processing and restart
; it via reguar v86 trap processing. (This is a very rare case.)
;
;;        public  V86TrapDRecovery
V86TrapDRecovery:
        mov     ecx, [esp].TsEcx
        mov     ebx, [esp].TsEbx
        mov     eax, [esp].TsEax
        mov     edx, [esp].TsEdx
        add     esp, TsErrCode
        jmp     KtdV86Slow

;
; If we come here, it means we can not process the trapped instruction.
; We will build a trap frame and use regular way to process the instruction.
; Since this could happen if interrupt is pending or the instruction trapped
; is not in the list of instructions which we handle.  We need to "continue"
; the processing instead of abort it.
;

ALIGN 4
V86PassThrough:
ifdef NT_UP
        mov     byte ptr SS:[P0PCRADDRESS][PcVdmAlert], 0
else
        mov     byte ptr PCR[PcVdmAlert], 0
endif
        ;
        ; eax, ebx, ecx, edx have been saved already
        ; Note, we don't want to destroy ecx, and edx
        ;
        mov     [esp].TsEbp, ebp
        mov     [esp].TsEsi, esi
        mov     [esp].TsEdi, edi
        mov     ebx, KGDT_R0_PCR
        mov     esi, KGDT_R3_DATA OR RPL_MASK
if DBG
        mov     [esp].TsPreviousPreviousMode, -1
        mov     [esp]+TsDbgArgMark, 0BADB0D00h
endif
        mov     fs, bx
        mov     ds, esi
        mov     es, esi
        mov     ebp, esp
        cld
        test    byte ptr PCR[PcDebugActive], -1
        jz      short @f

        mov     ebx,dr0
        mov     esi,dr1
        mov     edi,dr2
        mov     [ebp]+TsDr0,ebx
        mov     [ebp]+TsDr1,esi
        mov     [ebp]+TsDr2,edi
        mov     ebx,dr3
        mov     esi,dr6
        mov     edi,dr7
        mov     [ebp]+TsDr3,ebx
        mov     [ebp]+TsDr6,esi
        mov     [ebp]+TsDr7,edi
        ;
        ; Load KernelDr* into processor
        ;
        mov     edi,dword ptr fs:[PcPrcb]
        mov     ebx,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr0
        mov     esi,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr1
        mov     dr0,ebx
        mov     dr1,esi
        mov     ebx,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr2
        mov     esi,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr3
        mov     dr2,ebx
        mov     dr3,esi
        mov     ebx,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr6
        mov     esi,[edi].PbProcessorState.PsSpecialRegisters.SrKernelDr7
        mov     dr6,ebx
        mov     dr7,esi
@@:
        jmp     KtdV86Slow2
endm

        page , 132
        subttl "Macro to dispatch user APC"

;++
;
; Macro Description:
;
;    This macro is called before returning to user mode.  It dispatches
;    any pending user mode APCs.
;
; Arguments:
;
;    TFrame - TrapFrame
;    interrupts disabled
;
; Return Value:
;
;--

DISPATCH_USER_APC   macro   TFrame, ReturnCurrentEax
local   a, b
        test    dword ptr [TFrame]+TsEflags, EFLAGS_V86_MASK ; is previous mode v86?
        jnz     short b                             ; if nz, yes, go check for APC
        test    byte ptr [TFrame]+TsSegCs,MODE_MASK ; is previous mode user mode?
        jz      short a                             ; No, previousmode=Kernel, jump out
b:      mov     ebx, PCR[PcPrcbData+PbCurrentThread]; get addr of current thread
        mov     byte ptr [ebx]+ThAlerted, 0         ; clear kernel mode alerted
        cmp     byte ptr [ebx]+ThApcState.AsUserApcPending, 0
        je      short a                 ; if eq, no user APC pending

        mov     ebx, TFrame
ifnb <ReturnCurrentEax>
        mov     [ebx].TsEax, eax        ; Store return code in trap frame
        mov     dword ptr [ebx]+TsSegFs, KGDT_R3_TEB OR RPL_MASK
        mov     dword ptr [ebx]+TsSegDs, KGDT_R3_DATA OR RPL_MASK
        mov     dword ptr [ebx]+TsSegEs, KGDT_R3_DATA OR RPL_MASK
        mov     dword ptr [ebx]+TsSegGs, 0
endif

;
; Save previous IRQL and set new priority level
;
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql
        push    eax                     ; Save OldIrql

        sti                             ; Allow higher priority ints

;
; call the APC delivery routine.
;
; ebx - Trap frame
; 0 - Null exception frame
; 1 - Previous mode
;
; call APC deliver routine
;

        stdCall _KiDeliverApc, <1, 0, ebx>

        pop     ecx                     ; (ecx) = OldIrql
        fstCall KfLowerIrql

ifnb <ReturnCurrentEax>
        mov     eax, [ebx].TsEax        ; Restore eax, just in case
endif

        cli
        jmp     short b

    ALIGN 4
a:
endm


if DBG
        page ,132
        subttl "Processing Exception occurred in a 16 bit stack"
;++
;
; Routine Description:
;
;    This routine is called after an exception being detected during
;    a 16 bit stack.  The system will switch 16 stack to 32 bit
;    stack and bugcheck.
;
; Arguments:
;
;    None.
;
; Return value:
;
;    system stopped.
;
;--

align dword
        public  _Ki16BitStackException
_Ki16BitStackException proc

.FPO (2, 0, 0, 0, 0, FPO_TRAPFRAME)

        push    ss
        push    esp
        mov     eax, esp
        add     eax, fs:PcstackLimit
        mov     esp, eax
        mov     eax, KGDT_R0_DATA
        mov     ss, ax

        lea     ebp, [esp+8]
        cld
        SET_DEBUG_DATA

if DBG
        push    offset FLAT:Ki16BitStackTrapMessage
        call    _dbgPrint
        add     esp, 4
endif
        stdCall _KeBugCheck, <0F000FFFFh> ; Never return
        ret

_Ki16BitStackException endp

endif


        page    ,132
        subttl "System Service Call"
;++
;
; Routine Description:
;
;    This routine gains control when trap occurs via vector 2EH.
;    INT 2EH is reserved for system service calls.
;
;    The system service is executed by locating its routine address in
;    system service dispatch table and calling the specified function.
;    On return necessary state is restored.
;
; Arguments:
;
;    eax - System service number.
;    edx - Pointer to arguments
;
; Return Value:
;
;    eax - System service status code.
;
;--

if 0
;
;   Error and exception blocks for KiSystemService
;

Kss_ExceptionHandler:

;
; WARNING: Here we directly unlink the exception handler from the
; exception registration chain.  NO unwind is performed.
;

        mov     eax, [esp+4]            ; (eax)-> ExceptionRecord
        mov     eax, [eax].ErExceptionCode ; (eax) = Exception code
        mov     esp, [esp+8]            ; (esp)-> ExceptionList

        pop     eax
        mov     PCR[PcExceptionList],eax

        add     esp, 4
        pop     ebp
        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     kss60           ; v86 mode => usermode

        test    dword ptr [ebp].TsSegCs, MODE_MASK ; if premode=kernl
        jnz     kss60                   ; nz, prevmode=user, go return

; raise bugcheck if prevmode=kernel
        stdCall   _KeBugCheck, <KMODE_EXCEPTION_NOT_HANDLED>
endif

;
; The specified system service number is not within range. Attempt to
; convert the thread to a GUI thread if the specified system service is
; not a base service and the thread has not already been converted to a
; GUI thread.
;

Kss_ErrorHandler:
        cmp     ecx, SERVICE_TABLE_TEST ; test if GUI service
        jne     short Kss_LimitError    ; if ne, not GUI service
        push    edx                     ; save argument registers
        push    ebx                     ;
        stdcall _PsConvertToGuiThread   ; attempt to convert to GUI thread
        or      eax, eax                ; check if service was successful
        pop     eax                     ; restore argument registers
        pop     edx                     ;
        mov     ebp, esp                ; reset trap frame address
        mov     [esi]+ThTrapFrame, ebp  ; save address of trap frame
        jz      _KiSystemServiceRepeat  ; if eq, successful conversion

;
; The conversion to a Gui thread failed. The correct return value is encoded
; in a byte table indexed by the service number that is at the end of the
; service address table. The encoding is as follows:
;
;     0 - return 0.
;    -1 - return -1.
;     1 - return status code.
;

        lea     edx, _KeServiceDescriptorTableShadow + SERVICE_TABLE_TEST ;
        mov     ecx, [edx]+SdLimit      ; get service number limit
        mov     edx, [edx]+SdBase       ; get service table base
        lea     edx, [edx][ecx*4]       ; get ending service table address
        and     eax, SERVICE_NUMBER_MASK ; isolate service number
        add     edx, eax                ; compute return value address
        movsx   eax, byte ptr [edx]     ; get status byte
        or      eax, eax                ; check for 0 or -1
        jle     Kss70                   ; if le, return value set

Kss_LimitError:                         ;
        mov     eax, STATUS_INVALID_SYSTEM_SERVICE ; set return status
        jmp     kss70                   ;

        ENTER_DR_ASSIST kfce_a, kfce_t,NoAbiosAssist,NoV86Assist

align 16
        PUBLIC _KiFastCallEntry
_KiFastCallEntry        proc
    ;   At entry:
    ;   EAX = service number
    ;   EDX = Pointer to caller's arguments
    ;   ECX = User return address
    ;

    ; Create a stack frame like a call to inner privilege
    ; Load ESP from Tss.Esp0. ints are disabled and esp is not loaded.
ifndef NT_UP
    mov     esp, KGDT_R0_PCR
    mov     fs, sp
    mov     esp, fs:PCR[PcTss]
else
    mov     esp, ss:PCR[PcTss]
endif ;; NT_UP
    mov     esp, ss:[esp].TssEsp0

    push    KGDT_R3_DATA  OR RPL_MASK   ; Push user SS
    push    edx                         ; Push ESP+4
    sub     dword ptr [esp], 4
    pushfd                              ; Push EFlags
    or      dword ptr [esp], EFLAGS_INTERRUPT_MASK
    push    KGDT_R3_CODE OR RPL_MASK    ; Push user CS
    push    ecx                         ; Push Return EIP

ifndef NT_UP
    ; For the MP case, FS is already loaded above
    ENTER_SYSCALL   kfce_a, kfce_t, NoFSLoad
else
    ; set up trap frame and save state
    ENTER_SYSCALL   kfce_a, kfce_t
endif ;; NT_UP

    jmp     _KiSystemServiceRepeat

_KiFastCallEntry endp


        ENTER_DR_ASSIST kss_a, kss_t,NoAbiosAssist,NoV86Assist

;
; General System service entrypoint
;

align 16
        PUBLIC  _KiSystemService
_KiSystemService        proc

        ENTER_SYSCALL   kss_a, kss_t    ; set up trap frame and save state

?FpoValue = 0

;
; (eax) = Service number
; (edx) = Callers stack pointer
; (esi) = Current thread address
;
; All other registers have been saved and are free.
;
; Check if the service number within valid range
;

_KiSystemServiceRepeat:
        mov     edi, eax                ; copy system service number
        shr     edi, SERVICE_TABLE_SHIFT ; isolate service table number
        and     edi, SERVICE_TABLE_MASK ;
        mov     ecx, edi                ; save service table number
        add     edi, [esi]+ThServiceTable ; compute service descriptor address
        mov     ebx, eax                ; save system service number
        and     eax, SERVICE_NUMBER_MASK ; isolate service table offset

;
; If the specified system service number is not within range, then attempt
; to convert the thread to a GUI thread and retry the service dispatch.
;

        cmp     eax, [edi]+SdLimit      ; check if valid service
        jae     Kss_ErrorHandler        ; if ae, try to convert to GUI thread

;
; If the service is a GUI service and the GDI user batch queue is not empty,
; then call the appropriate service to flush the user batch.
;

        cmp     ecx, SERVICE_TABLE_TEST ; test if GUI service
        jne     short Kss40             ; if ne, not GUI service
        mov     ecx, PCR[PcTeb]         ; get current thread TEB address
        xor     ebx, ebx                ; get number of batched GDI calls
        or      ebx, [ecx]+TbGdiBatchCount ;
        jz      short Kss40             ; if z, no batched calls
        push    edx                     ; save address of user arguments
        push    eax                     ; save service number
        call    [_KeGdiFlushUserBatch]  ; flush GDI user batch
        pop     eax                     ; restore service number
        pop     edx                     ; restore address of user arguments

;
; The arguments are passed on the stack. Therefore they always need to get
; copied since additional space has been allocated on the stack for the
; machine state frame.  Note that we don't check for zero argument.  Copy
; is alway done reguardless of number of arguments.  This is because zero
; argument is very rare.
;

Kss40:  inc     dword ptr PCR[PcPrcbData+PbSystemCalls] ; system calls

if DBG
        mov     ecx, [edi]+SdCount      ; get count table address
        jecxz   Kss45                   ; if zero, table not specified
        inc     dword ptr [ecx+eax*4]   ; increment service count
Kss45:  push    dword ptr [esi]+ThApcStateIndex ; (ebp-4)
        push    dword ptr [esi]+ThKernelApcDisable ; (ebp-8)

        ;
        ; work around errata 19 which can in some cases cause an
        ; extra dword to be moved in the rep movsd below. In the DBG
        ; build, this will usually case a bugcheck 1 where ebp-8 is no longer
        ; the kernel apc disable count
        ;

        sub     esp,4


?FpoValue = ?FpoValue+3
endif

FPOFRAME ?FpoValue, 0

        mov     esi, edx                ; (esi)->User arguments
        mov     ebx, [edi]+SdNumber     ; get argument table address
        xor     ecx, ecx
        mov     cl, byte ptr [ebx+eax]  ; (ecx) = argument size
        mov     edi, [edi]+SdBase       ; get service table address
        mov     ebx, [edi+eax*4]        ; (ebx)-> service routine
        sub     esp, ecx                ; allocate space for arguments
        shr     ecx, 2                  ; (ecx) = number of argument DWORDs
        mov     edi, esp                ; (es:edi)->location to receive 1st arg
        cmp     esi, _MmUserProbeAddress ; check if user address
        jae     kss80                   ; if ae, then not user address

KiSystemServiceCopyArguments:
        rep     movsd                   ; copy the arguments to top of stack.
                                        ; Since we usually copy more than 3
                                        ; arguments.  rep movsd is faster than
                                        ; mov instructions.
if DBG
;
; Check for user mode call into system at elevated IRQL.
;

        test    byte ptr [ebp]+TsSegCs,MODE_MASK
        jz      short kss50a                  ; kernel mode, skip test
        stdCall _KeGetCurrentIrql
        or      al, al                  ; bogus irql, go bugcheck
        jnz     kss100
kss50a:
endif

;
; Make actual call to system service
;
kssdoit:
        call    ebx                     ; call system service
kss60:

if DBG
        mov     ebx,PCR[PcPrcbData+PbCurrentThread] ; (ebx)-> Current Thread

;
; Check for return to user mode at elevated IRQL.
;
        test    byte ptr [ebp]+TsSegCs,MODE_MASK
        jz      short kss50b
        mov     esi, eax
        stdCall _KeGetCurrentIrql
        or      al, al
        jnz     kss100                  ; bogus irql, go bugcheck
        mov     eax, esi
kss50b:

;
; Check that APC state has not changed
;
        mov     edx, [ebp-4]
        cmp     dl, [ebx]+ThApcStateIndex
        jne     kss120

        mov     edx, [ebp-8]
        cmp     dl, [ebx]+ThKernelApcDisable
        jne     kss120

endif

;
; Upon return, (eax)= status code
;

        mov     esp, ebp                ; deallocate stack space for arguments

;
; Restore old trap frame address from the current trap frame.
;

kss70:  mov     ecx, PCR[PcPrcbData+PbCurrentThread] ; get current thread address
        mov     edx, [ebp].TsEdx        ; restore previous trap frame address
        mov     [ecx].ThTrapFrame, edx  ;

;
;   System service's private version of KiExceptionExit
;   (Also used by KiDebugService)
;
;   Check for pending APC interrupts, if found, dispatch to them
;   (saving eax in frame first).
;
        public  _KiServiceExit
_KiServiceExit:

        cli                                         ; disable interrupts
        DISPATCH_USER_APC   ebp, ReturnCurrentEax

;
; Exit from SystemService
;

        EXIT_ALL    NoRestoreSegs, NoRestoreVolatile

;
; The address of the argument list is not a user address. If the previous mode
; is user, then return an access violation as the status of the system service.
; Otherwise, copy the argument list and execute the system service.
;

kss80:  test    byte ptr [ebp].TsSegCs, MODE_MASK ; test previous mode
        jz      KiSystemServiceCopyArguments ; if z, previous mode kernel
        mov     eax, STATUS_ACCESS_VIOLATION ; set service status
        jmp     kss60                   ;

;++
;
;   _KiServiceExit2 - same as _KiServiceExit BUT the full trap_frame
;       context is restored
;
;--
        public  _KiServiceExit2
_KiServiceExit2:

        cli                             ; disable interrupts
        DISPATCH_USER_APC   ebp

;
; Exit from SystemService
;
        EXIT_ALL                            ; RestoreAll




if DBG
kss100: push    PCR[PcIrql]                 ; put bogus value on stack for dbg
?FpoValue = ?FpoValue + 1
FPOFRAME ?FpoValue, 0
        mov     byte ptr PCR[PcIrql],0      ; avoid recursive trap
        cli

        stdCall   _KeBugCheck,<IRQL_GT_ZERO_AT_SYSTEM_SERVICE>

kss120: stdCall   _KeBugCheck,<APC_INDEX_MISMATCH>

endif
        ret

_KiSystemService endp

;
; BBT cannot instrument code between this label and BBT_Exclude_Trap_Code_End
;
        public  _BBT_Exclude_Trap_Code_Begin
_BBT_Exclude_Trap_Code_Begin  equ     $
        int 3

;
; Fast path NtGetTickCount
;

align 16
        ENTER_DR_ASSIST kitx_a, kitx_t,NoAbiosAssist
        PUBLIC  _KiGetTickCount
_KiGetTickCount proc

        cmp     [esp+4], KGDT_R3_CODE OR RPL_MASK
        jnz     short @f

Kgtc00:
        mov     eax,dword ptr cs:[_KeTickCount]
        mul     dword ptr cs:[_ExpTickCountMultiplier]
        shrd    eax,edx,24                  ; compute resultant tick count

        iretd
@@:
        ;
        ; if v86 mode, we dont handle it
        ;

        test    dword ptr [esp+8], EFLAGS_V86_MASK
        jnz     ktgc20

        ;
        ; if kernel mode, must be get tick count
        ;

        test    [esp+4], MODE_MASK
        jz      short Kgtc00

        ;
        ; else check if the caller is USER16
        ;   if eax = ebp = 0xf0f0f0f0  it is get-tick-count
        ;   if eax = ebp = 0xf0f0f0f1  it is set-ldt-entry
        ;

        cmp     eax, ebp                ; if eax != ebp, not USER16
        jne     ktgc20

        and     eax, 0fffffff0h
        cmp     eax, 0f0f0f0f0h
        jne     ktgc20

        cmp     ebp, 0f0f0f0f0h         ; Is it user16 gettickcount?
        je      short Kgtc00            ; if z, yes


        cmp     ebp, 0f0f0f0f1h         ; If this is setldt entry
        jne     ktgc20                  ; if nz, we don't know what
                                        ; it is.

        ;
        ; The idea here is that user16 can call 32 bit api to
        ; update LDT entry without going through the penalty
        ; of DPMI.  For Daytona beta.
        ;

        push    0                       ; push dummy error code
        ENTER_TRAP      kitx_a, kitx_t
        sti

        xor     eax, eax
        mov     ebx, [ebp+TsEbx]
        mov     ecx, [ebp+TsEcx]
        mov     edx, [ebp+TsEdx]
        stdCall _NtSetLdtEntries <ebx, ecx, edx, eax, eax, eax>
        mov     [ebp+TsEax], eax
        and     dword ptr [ebp+TsEflags], 0FFFFFFFEH ; clear carry flag
        cmp     eax, 0                  ; success?
        je      short ktgc10

        or      dword ptr [ebp+TsEflags], 1 ; set carry flag
ktgc10:
        jmp     _KiExceptionExit

ktgc20:
        ;
        ; We need to *trap* this int 2a.  For exception, the eip should
        ; point to the int 2a instruction not the instruction after it.
        ;

        sub     word ptr [esp], 2
        push    0
        jmp     _KiTrap0D

_KiGetTickCount endp

        page ,132
        subttl  "Return from User Mode Callback"
;++
;
; NTSTATUS
; NtCallbackReturn (
;    IN PVOID OutputBuffer OPTIONAL,
;    IN ULONG OutputLength,
;    IN NTSTATUS Status
;    )
;
; Routine Description:
;
;    This function returns from a user mode callout to the kernel mode
;    caller of the user mode callback function.
;
;    N.B. This service uses a nonstandard calling sequence.
;
; Arguments:
;
;    OutputBuffer (ecx) - Supplies an optional pointer to an output buffer.
;
;    OutputLength (edx) - Supplies the length of the output buffer.
;
;    Status (esp + 4) - Supplies the status value returned to the caller of
;        the callback function.
;
; Return Value:
;
;    If the callback return cannot be executed, then an error status is
;    returned. Otherwise, the specified callback status is returned to
;    the caller of the callback function.
;
;    N.B. This function returns to the function that called out to user
;         mode is a callout is currently active.
;
;--

align 16
        PUBLIC  _KiCallbackReturn
_KiCallbackReturn proc

        push    fs                      ; save segment register
        push    ecx                     ; save buffer address and return status
        push    eax                     ;
        mov     ecx,KGDT_R0_PCR         ; set PCR segment number
        mov     fs,cx                   ;
        mov     eax,PCR[PcPrcbData + PbCurrentThread] ; get current thread address
        mov     ecx,[eax].ThCallbackStack ; get callback stack address
        or      ecx,ecx                 ; check if callback active
        jz      _KiCbExit               ; if z, no callback active
        mov     edi,[esp] + 4           ; set output buffer address
        mov     esi,edx                 ; set output buffer length
        mov     ebp,[esp] + 0           ; set return status

;
; N.B. The following code is entered with:
;
;    eax - The address of the current thread.
;    ecx - The callback stack address.
;    edi - The output buffer address.
;    esi - The output buffer length.
;    ebp - The callback service status.
;
; Restore the trap frame and callback stack addresses,
; store the output buffer address and length, and set the service status.
;

        cld                             ; clear the direction flag
        mov     ebx,[ecx].CuOutBf       ; get address to store output buffer
        mov     [ebx],edi               ; store output buffer address
        mov     ebx,[ecx].CuOutLn       ; get address to store output length
        mov     [ebx],esi               ; store output buffer length
        mov     esi,PCR[PcInitialStack] ; get source NPX save area address
        mov     ebx,[ecx]               ; get previous initial stack address
        mov     [eax].ThInitialStack,ebx ; restore initial stack address
        sub     ebx,NPX_FRAME_LENGTH    ; compute destination NPX save area

; All we want to do is to copy ControlWord, StatusWord and TagWord from
; the source NPX save area. So we always copy first 3 dwords, irrespective
; of the fact whether the save was done using fxsave or fnsave.

        mov     edx,[esi].FpControlWord ; copy NPX state to previous frame
        mov     [ebx].FpControlWord,edx ;
        mov     edx,[esi].FpStatusWord  ;
        mov     [ebx].FpStatusWord,edx  ;
        mov     edx,[esi].FpTagWord     ;
        mov     [ebx].FpTagWord,edx     ;
        mov     edx,[esi].FpCr0NpxState ;
        mov     [ebx].FpCr0NpxState,edx ;
        mov     edx,PCR[PcTss]          ; get address of task switch segment
        mov     PCR[PcInitialStack],ebx ; restore stack check base address
        sub     ebx,TsV86Gs - TsHardwareSegSs ; bias for missing V86 fields
        mov     [edx].TssEsp0,ebx       ; restore kernel entry stack address
        mov     esp,ecx                 ; trim stack back to callback frame
        add     esp,4                   ;
        sti                             ; enable interrupts
        pop     [eax].ThTrapFrame       ; restore current trap frame address
        pop     [eax].ThCallbackStack   ; restore callback stack address
        mov     eax,ebp                 ; set callback service status

;
; Restore nonvolatile registers, clean call parameters from stack, and
; return to callback caller.
;

        pop     edi                     ; restore nonvolatile registers
        pop     esi                     ;
        pop     ebx                     ;
        pop     ebp                     ;
        pop     edx                     ; save return address
        add     esp,8                   ; remove parameters from stack
        jmp     edx                     ; return to callback caller

;
; Restore segment register, set systerm service status, and return.
;

_KiCbExit:                              ;
        add     esp, 2 * 4              ; remove saved registers from stack
        pop     fs                      ; restore segment register
        mov     eax,STATUS_NO_CALLBACK_ACTIVE ; set service status
        iretd                           ;

_KiCallbackReturn endp

;
; Fast path Nt/Zw SetLowWaitHighThread
;

        ENTER_DR_ASSIST kslwh_a, kslwh_t,NoAbiosAssist,NoV86Assist
align 16
        PUBLIC  _KiSetLowWaitHighThread
_KiSetLowWaitHighThread proc

        ENTER_SYSCALL   kslwh_a, kslwh_t ; Set up trap frame

        mov     eax,STATUS_NO_EVENT_PAIR ; set service status
        mov     edx,[ebp].TsEdx         ; restore old trap frame address
        mov     [esi].ThTrapFrame,edx   ;
        cli                             ; disable interrupts

        DISPATCH_USER_APC   ebp, ReturnCurrentEax

        EXIT_ALL    NoRestoreSegs, NoRestoreVolatile

_KiSetLowWaitHighThread endp

        page ,132
        subttl  "Common Trap Exit"
;++
;
;   KiExceptionExit
;
;   Routine Description:
;
;       This code is transfered to at the end of the processing for
;       an exception.  Its function is to restore machine state, and
;       continue thread execution.  If control is returning to user mode
;       and there is a user APC pending, then control is transfered to
;       the user APC delivery routine.
;
;       N.B. It is assumed that this code executes at IRQL zero or APC_LEVEL.
;          Therefore page faults and access violations can be taken.
;
;       NOTE: This code is jumped to, not called.
;
;   Arguments:
;
;       (ebp) -> base of trap frame.
;
;   Return Value:
;
;       None.
;
;--
align 4
        public  _KiExceptionExit
_KiExceptionExit proc
.FPO (0, 0, 0, 0, 0, FPO_TRAPFRAME)

        cli                             ; disable interrupts
        DISPATCH_USER_APC   ebp

;
; Exit from Exception
;

        EXIT_ALL    ,,NoPreviousMode

_KiExceptionExit endp


;++
;
;   Kei386EoiHelper
;
;   Routine Description:
;
;       This code is transfered to at the end of an interrupt.  (via the
;       exit_interrupt macro).  It checks for user APC dispatching and
;       performs the exit_all for the interrupt.
;
;       NOTE: This code is jumped to, not called.
;
;   Arguments:
;
;       (esp) -> base of trap frame.
;       interrupts are disabled
;
;   Return Value:
;
;       None.
;
;--
align 4
cPublicProc Kei386EoiHelper, 0
.FPO (0, 0, 0, 0, 0, FPO_TRAPFRAME)
        ASSERT_FS
        DISPATCH_USER_APC   esp
        EXIT_ALL    ,,NoPreviousMode
stdENDP Kei386EoiHelper


;++
;
;   KiUnexpectedInterruptTail
;
;   Routine Description:
;       This function is jumped to by an IDT entry who has no interrupt
;       handler.
;
;   Arguments:
;
;       (esp)   - Dword, vector
;       (esp+4) - Processor generated IRet frame
;
;--

        ENTER_DR_ASSIST kui_a, kui_t

        public  _KiUnexpectedInterruptTail
_KiUnexpectedInterruptTail  proc
        ENTER_INTERRUPT kui_a, kui_t, PassDwordParm

        inc     dword ptr PCR[PcPrcbData+PbInterruptCount]

        mov     ebx, [esp]      ; get vector & leave it on the stack
        sub     esp, 4          ; make space for OldIrql

; esp - ptr to OldIrql
; ebx - Vector
; HIGH_LEVEL - Irql
        stdCall   _HalBeginSystemInterrupt, <HIGH_LEVEL,ebx,esp>
        or      eax, eax
        jnz     kui10

;
; spurious interrupt
;
        add     esp, 8
        EXIT_ALL ,,NoPreviousMode

kui10:
if DBG
        push    dword ptr [esp+4]       ; Vector #
        push    offset FLAT:BadInterruptMessage
        call    _DbgPrint               ; display unexpected interrupt message
        add     esp, 8
endif
;
; end this interrupt
;
        INTERRUPT_EXIT

_KiUnexpectedInterruptTail  endp



        page , 132
        subttl "trap processing"

;++
;
; Routine Description:
;
;    _KiTrapxx - protected mode trap entry points
;
;    These entry points are for internally generated exceptions,
;    such as a general protection fault.  They do not handle
;    external hardware interrupts, or user software interrupts.
;
; Arguments:
;
;    On entry the stack looks like:
;
;       [ss]
;       [esp]
;       eflags
;       cs
;       eip
;    ss:sp-> [error]
;
;    The cpu saves the previous SS:ESP, eflags, and CS:EIP on
;    the new stack if there was a privilige transition. If no
;    priviledge level transition occurred, then there is no
;    saved SS:ESP.
;
;    Some exceptions save an error code, others do not.
;
; Return Value:
;
;       None.
;
;--


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

.FPO ( ExceptionRecordSize/4+NumParms, 0, 0, 0, 0, FPO_TRAPFRAME )

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
        mov     eax,[ebp]+TsSegCs

        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jz      de10

        mov     eax,0FFFFh
de10:   and     eax,MODE_MASK

; 1 - first chance TRUE
; eax - PreviousMode
; ebp - trap frame addr
; 0 - Null exception frame
; ecx - exception record addr

; dispatchexception as appropriate
        stdCall   _KiDispatchException, <ecx, 0, ebp, eax, 1>

        mov     esp, ebp                ; (esp) -> trap frame

        ENDM

        page , 132
        subttl "dispatch exception"

;++
;
; CommonDispatchException
;
; Routine Description:
;
;    This routine allocates exception record on stack, sets up exception
;    record using specified parameters and finally sets up arguments
;    and calls _KiDispatchException.
;
;    NOTE:
;
;    The purpose of this routine is to save code space.  Use this routine
;    only if:
;    1. ExceptionRecord is NULL
;    2. ExceptionFlags is 0
;    3. Number of parameters is less or equal than 3.
;
;    Otherwise, you should use DISPATCH_EXCEPTION macro to set up your special
;    exception record.
;
; Arguments:
;
;    (eax) = ExcepCode - Exception code to put into exception record
;    (ebx) = ExceptAddress - Addr of instruction which the hardware exception occurs
;    (ecx) = NumParms - Number of additional parameters
;    (edx) = Parameter1
;    (esi) = Parameter2
;    (edi) = Parameter3
;
; Return Value:
;
;    None.
;
;--
CommonDispatchException0Args:
        xor     ecx, ecx                ; zero arguments
        call    CommonDispatchException

CommonDispatchException1Arg0d:
        xor     edx, edx                ; zero edx
CommonDispatchException1Arg:
        mov     ecx, 1                  ; one argument
        call    CommonDispatchException ; there is no return

CommonDispatchException2Args0d:
        xor     edx, edx                ; zero edx
CommonDispatchException2Args:
        mov     ecx, 2                  ; two arguments
        call    CommonDispatchException ; there is no return

      public CommonDispatchException
align dword
CommonDispatchException proc
cPublicFpo 0, ExceptionRecordLength / 4
;
;       Set up exception record for raising exception
;

        sub     esp, ExceptionRecordLength
                                        ; allocate exception record
        mov     dword ptr [esp]+ErExceptionCode, eax
                                        ; set up exception code
        xor     eax, eax
        mov     dword ptr [esp]+ErExceptionFlags, eax
                                        ; set exception flags
        mov     dword ptr [esp]+ErExceptionRecord, eax
                                        ; set associated exception record
        mov     dword ptr [esp]+ErExceptionAddress, ebx
        mov     dword ptr [esp]+ErNumberParameters, ecx
                                        ; set number of parameters
        cmp     ecx, 0
        je      short de00

        lea     ebx, [esp + ErExceptionInformation]
        mov     [ebx], edx
        mov     [ebx+4], esi
        mov     [ebx+8], edi
de00:
;
; set up arguments and call _KiDispatchException
;

        mov     ecx, esp                ; (ecx)->exception record
        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jz      short de10

        mov     eax,0FFFFh
        jmp     short de20

de10:   mov     eax,[ebp]+TsSegCs
de20:   and     eax,MODE_MASK

; 1 - first chance TRUE
; eax - PreviousMode
; ebp - trap frame addr
; 0 - Null exception frame
; ecx - exception record addr

        stdCall _KiDispatchException,<ecx, 0, ebp, eax, 1>

        mov     esp, ebp                ; (esp) -> trap frame
        jmp     _KiExceptionExit

CommonDispatchException endp

        page , 132
        subttl "Macro to verify base trap frame"

;++
;
; Macro Description:
;
;   This macro verifies the base trap frame is intact.
;
;   It is possible while returing to UserMode that we take an exception.
;   Any expection which may block, such as not-present, needs to verify
;   that the base trap frame is not partially dismantled.
;
; Arguments:
;   The macro MUST be used directly after ENTER_TRAP macro.
;   assumses all sorts of stuff about ESP!
;
; Return Value:
;
;   If the base frame was incomplete it is totally restored and the
;   return EIP of the current frame is (virtually) backed up to the
;   begining of the exit_all - the effect is that the base frame
;   will be completely exited again.  (ie, the exit_all of the base
;   frame is atomic, if it's interrupted we restore it and do it over).
;
;    None.
;
;--

VERIFY_BASE_TRAP_FRAME macro
       local    vbfdone

        mov     eax, esp
        sub     eax, PCR[PcInitialStack] ; Bias out this stack
        add     eax, KTRAP_FRAME_LENGTH ; adjust for base frame
        je      short vbfdone           ; if eq, then this is the base frame

        cmp     eax, -TsEflags          ; second frame is only this big
        jc      short vbfdone           ; is stack deeper then 2 frames?
                                        ; yes, then done
    ;
    ; Stack usage is not exactly one frame, and it's not large enough
    ; to be two complete frames; therefore, we may have a partial base
    ; frame. (unless it's a kernel thread)
    ;
    ; See if this is a kernel thread - Kernel threads don't have a base
    ; frame (and therefore don't need correcting).
    ;

        mov     eax, PCR[PcTeb]
        or      eax, eax                ; Any Teb?
        jle     short vbfdone           ; Br if zero or kernel thread address

        call    KiRestoreBaseFrame

    align 4
vbfdone:
        ENDM

;++ KiRestoreBaseFrame
;
; Routine Description:
;
;   Only to be used from VERIFY_BASE_TRAP_FRAME macro.
;   Makes lots of assumptions about esp & trap frames
;
; Arguments:
;
;   Stack:
;       +-------------------------+
;       |                         |
;       |                         |
;       | Npx save area           |
;       |                         |
;       |                         |
;       +-------------------------+
;       | (possible mvdm regs)    |
;       +-------------------------+  <- fs:PcInitialStack
;       |                         |
;       | Partial base trap frame |
;       |                         |
;       |             ------------+
;       +------------/            |  <- Esp @ time of current frame. Location
;       |                         |     where base trap frame is incomplete
;       | Completed 'current'     |
;       |  trap frame             |
;       |                         |
;       |                         |
;       |                         |
;       |                         |
;       +-------------------------+  <- EBP
;       | return address (dword)  |
;       +-------------------------+  <- current ESP
;       |                         |
;       |                         |
;
; Return:
;
;   Stack:
;       +-------------------------+
;       |                         |
;       |                         |
;       | Npx save area           |
;       |                         |
;       |                         |
;       +-------------------------+
;       | (possible mvdm regs)    |
;       +-------------------------+  <- fs:PcInitialStack
;       |                         |
;       | Base trap frame         |
;       |                         |
;       |                         |
;       |                         |
;       |                         |
;       |                         |
;       +-------------------------+  <- return esp & ebp
;       |                         |
;       | Current trap frame      |
;       |                         |  EIP set to begining of
;       |                         |  exit_all code
;       |                         |
;       |                         |
;       |                         |
;       +-------------------------+  <- EBP, ESP
;       |                         |
;       |                         |
;
;--

KiRestoreBaseFrame proc
        pop     ebx                     ; Get return address
IF DBG
        mov     eax, [esp].TsEip        ; EIP of trap
    ;
    ; This code is to handle a very specific problem of a not-present
    ; fault during an exit_all.  If it's not this problem then stop.
    ;
        cmp     word ptr [eax], POP_GS
        je      short @f
        cmp     byte ptr [eax], POP_ES
        je      short @f
        cmp     byte ptr [eax], POP_DS
        je      short @f
        cmp     word ptr [eax], POP_FS
        je      short @f
        cmp     byte ptr [eax], IRET_OP
        je      short @f
        int 3
@@:
ENDIF
    ;
    ; Move current trap frame out of the way to make space for
    ; a full base trap frame
    ;
        mov     edi, PCR[PcInitialStack]
        sub     edi, KTRAP_FRAME_LENGTH + TsEFlags + 4 ; (edi) = bottom of target
        mov     esi, esp                ; (esi) = bottom of source
        mov     esp, edi                ; make space before copying the data
        mov     ebp, edi                ; update location of our trap frame
        push    ebx                     ; put return address back on stack

        mov     ecx, (TsEFlags+4)/4     ; # of dword to move
        rep     movsd                   ; Move current trap frame

    ;
    ; Part of the base frame was destoryed when the current frame was
    ; originally pushed.  Now that the current frame has been moved out of
    ; the way restore the base frame.  We know that any missing data from
    ; the base frame was reloaded into it's corrisponding registers which
    ; were then pushed into the current frame.  So we can restore the missing
    ; data from the current frame.
    ;
        mov     ecx, esi                ; Location of esp at time of fault
        mov     edi, PCR[PcInitialStack]
        sub     edi, KTRAP_FRAME_LENGTH ; (edi) = base trap frame
        mov     ebx, edi

        sub     ecx, edi                ; (ecx) = # of bytes which were
                                        ; removed from base frame before
                                        ; trap occured
IF DBG
        test    ecx, 3
        jz      short @f                ; assume dword alignments only
        int 3
@@:
ENDIF
        mov     esi, ebp                ; (esi) = current frame
        shr     ecx, 2                  ; copy in dwords
        rep     movsd
    ;
    ; The base frame is restored.  Instead of backing EIP up to the
    ; start of the interrupted EXIT_ALL, we simply move the EIP to a
    ; well known EXIT_ALL.  However, this causes a couple of problems
    ; since this exit_all retores every register whereas the original
    ; one may not.  So:
    ;
    ;   - When exiting from a system call, eax is normally returned by
    ;     simply not restoring it.  We 'know' that the current trap frame's
    ;     EAXs is always the correct one to return.  (We know this because
    ;     exit_all always restores eax (if it's going to) before any other
    ;     instruction which may cause a fault).
    ;
    ;   - Not all enter's push the PreviousPreviousMode.  Since this is
    ;     the base trap frame we know that this must be UserMode.
    ;
        mov     eax, [ebp].TsEax                    ; make sure correct
        mov     [ebx].TsEax, eax                    ; eax is in base frame
        mov     byte ptr [ebx].TsPreviousPreviousMode, 1    ; UserMode

        mov     [ebp].TsEbp, ebx
        mov     [ebp].TsEip, offset _KiServiceExit2 ; ExitAll which

                                                    ; restores everything
    ;
    ; Since we backed up Eip we need to reset some of the kernel selector
    ; values in case they were already restored by the attempted base frame pop
    ;
        mov     dword ptr [ebp].TsSegDs, KGDT_R3_DATA OR RPL_MASK
        mov     dword ptr [ebp].TsSegEs, KGDT_R3_DATA OR RPL_MASK
        mov     dword ptr [ebp].TsSegFs, KGDT_R0_PCR

    ;
    ; The backed up EIP is before interrupts were disabled.  Re-enable
    ; interrupts for the current trap frame
    ;
        or      [ebp].TsEFlags, EFLAGS_INTERRUPT_MASK

        ret

KiRestoreBaseFrame endp

        page ,132
        subttl "Divide error processing"
;++
;
; Routine Description:
;
;    Handle divide error fault.
;
;    The divide error fault occurs if a DIV or IDIV instructions is
;    executed with a divisor of 0, or if the quotient is too big to
;    fit in the result operand.
;
;    An INTEGER DIVIDED BY ZERO exception will be raised for the fault.
;    If the fault occurs in kernel mode, the system will be terminated.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the faulting instruction.
;    No error code is provided with the divide error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING
        ENTER_DR_ASSIST kit0_a, kit0_t,NoAbiosAssist
align dword
        public  _KiTrap00
_KiTrap00       proc

        push    0                       ; push dummy error code
        ENTER_TRAP      kit0_a, kit0_t

        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     Kt0040            ; trap occured in V86 mode

        test    byte ptr [ebp]+TsSegCs, MODE_MASK  ; Is previous mode = USER
        jz      short Kt0000

        cmp     word ptr [ebp]+TsSegCs,KGDT_R3_CODE OR RPL_MASK
        jne     Kt0020
;
; Set up exception record for raising Integer_Divided_by_zero exception
; and call _KiDispatchException
;

Kt0000:

if DBG
        test    [ebp]+TsEFlags, EFLAGS_INTERRUPT_MASK   ; faulted with
        jnz     short @f                                ; interrupts disabled?

        xor     eax, eax
        mov     esi, [ebp]+TsEip        ; [esi] = faulting instruction
        stdCall _KeBugCheckEx,<IRQL_NOT_LESS_OR_EQUAL,eax,-1,eax,esi>
@@:
endif

        sti


;
; Flat mode
;
; The intel processor raises a divide by zero expcetion on DIV instruction
; which overflows. To be compatible we other processors we want to
; return overflows as such and not as divide by zero's.  The operand
; on the div instruction is tested to see if it's zero or not.
;
        stdCall _Ki386CheckDivideByZeroTrap,<ebp>
        mov     ebx, [ebp]+TsEip        ; (ebx)-> faulting instruction
        jmp     CommonDispatchException0Args ; Won't return

Kt0010:
;
; 16:16 mode
;
        sti
        mov     ebx, [ebp]+TsEip        ; (ebx)-> faulting instruction
        mov     eax, STATUS_INTEGER_DIVIDE_BY_ZERO
        jmp     CommonDispatchException0Args ; never return

Kt0020:
; Check to see if this process is a vdm
        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      Kt0010

Kt0040:
        stdCall _Ki386VdmReflectException_A, <0>

        or      al,al
        jz      short Kt0010             ; couldn't reflect, gen exception
        jmp     _KiExceptionExit

_KiTrap00       endp


        page ,132
        subttl "Debug Exception"
;++
;
; Routine Description:
;
;    Handle debug exception.
;
;    The processor triggers this exception for any of the following
;    conditions:
;
;    1. Instruction breakpoint fault.
;    2. Data address breakpoint trap.
;    3. General detect fault.
;    4. Single-step trap.
;    5. Task-switch breadkpoint trap.
;
;
; Arguments:
;
;    At entry, the values of saved CS and EIP depend on whether the
;    exception is a fault or a trap.
;    No error code is provided with the divide error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING
        ENTER_DR_ASSIST kit1_a, kit1_t, NoAbiosAssist
align dword
        public  _KiTrap01
_KiTrap01       proc

; Set up machine state frame for displaying

        push    0                       ; push dummy error code
        ENTER_TRAP      kit1_a, kit1_t

;
; If caller is user mode, we want interrupts back on.
;   . all relevent state has already been saved
;   . user mode code always runs with ints on
;
; If caller is kernel mode, we want them off!
;   . some state still in registers, must prevent races
;   . kernel mode code can run with ints off
;
;

        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     kit01_30                ; fault occured in V86 mode => Usermode

        test    word ptr [ebp]+TsSegCs,MODE_MASK
        jz      kit01_10

        cmp     word ptr [ebp]+TsSegCs,KGDT_R3_CODE OR RPL_MASK
        jne     kit01_30
kit01_05:
        sti
kit01_10:

;
; Set up exception record for raising single step exception
; and call _KiDispatchException
;

kit01_20:
        and     dword ptr [ebp]+TsEflags, not EFLAGS_TF_BIT
        mov     ebx, [ebp]+TsEip                ; (ebx)-> faulting instruction
        mov     eax, STATUS_SINGLE_STEP
        jmp     CommonDispatchException0Args    ; Never return

kit01_30:

; Check to see if this process is a vdm

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      kit01_05

        stdCall _Ki386VdmReflectException_A, <01h>
        test    ax,0FFFFh
        jz      Kit01_20

        jmp     _KiExceptionExit

_KiTrap01       endp

        page ,132
        subttl "Nonmaskable Interrupt"
;++
;
; Routine Description:
;
;    Handle Nonmaskable interrupt.
;
;    An NMI is typically used to signal serious system conditions
;    such as bus time-out, memory parity error, and so on.
;
;    Upon detection of the NMI, the system will be terminated, ie a
;    bugcheck will be raised, no matter what previous mode is.
;
; Arguments:
;
;    No error code is provided with the error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING
;        ENTER_DR_ASSIST kit2_a, kit2_t, NoAbiosAssist
align dword
        public  _KiTrap02
_KiTrap02       proc
.FPO (1, 0, 0, 0, 0, 2)
        cli
;
; Update the TSS pointer in the PCR to point to the NMI TSS
; (which is what we're running on, or else we wouldn't be here)
;
        push    dword ptr PCR[PcTss]

        mov     eax, PCR[PcGdt]
        mov     ch, [eax+KGDT_NMI_TSS+KgdtBaseHi]
        mov     cl, [eax+KGDT_NMI_TSS+KgdtBaseMid]
        shl     ecx, 16
        mov     cx, [eax+KGDT_NMI_TSS+KgdtBaseLow]
        mov     PCR[PcTss], ecx

;
; Clear Nested Task bit in EFLAGS
;
        pushfd
        and     [esp], not 04000h
        popfd

;
; Clear the busy bit in the TSS selector
;
        mov     ecx, PCR[PcGdt]
        lea     eax, [ecx] + KGDT_NMI_TSS
        mov     byte ptr [eax+5], 089h  ; 32bit, dpl=0, present, TSS32, not busy

;
; Let the HAL have a crack at it before we crash
;
        stdCall _HalHandleNMI,<0>

        mov     eax, offset FLAT:_ZwUnmapViewOfSection@8
        sub     eax, esp
        cmp     eax, 0a00h
        jnc     short @f

        ; not on a real stack, crash
        stdCall _KeBugCheckEx,<UNEXPECTED_KERNEL_MODE_TRAP,2,0,0,0>
@@:

;
; We're back, therefore the Hal has dealt with the NMI.  (Crashing
; is done in the Hal for this special case.)
;

        pop     dword ptr PCR[PcTss]    ; restore PcTss

        mov     ecx, PCR[PcGdt]
        lea     eax, [ecx] + KGDT_TSS
        mov     byte ptr [eax+5], 08bh  ; 32bit, dpl=0, present, TSS32, *busy*

        pushfd                          ; Set Nested Task bit in EFLAGS
        or      [esp], 04000h           ; so iretd will do a tast switch
        popfd

        iretd                           ; Return from NMI
        jmp     short _KiTrap02         ; in case we NMI again

_KiTrap02       endp

        page ,132
        subttl "DebugService Breakpoint"
;++
;
; Routine Description:
;
;    Handle INT 2d DebugService
;
;    The trap is caused by an INT 2d.  This is used instead of a
;    BREAKPOINT exception so that parameters can be passed for the
;    requested debug service.  A BREAKPOINT instruction is assumed
;    to be right after the INT 2d - this allows this code to share code
;    with the breakpoint handler.
;
; Arguments:
;     eax - ServiceClass - which call is to be performed
;     ecx - Arg1 - generic first argument
;     edx - Arg2 - generic second argument
;
;--

        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kids_a, kids_t, NoAbiosAssist
align dword
        public  _KiDebugService
_KiDebugService  proc
        push    0                           ; push dummy error code
        ENTER_TRAP      kids_a, kids_t
;       sti                                 ; *NEVER sti here*

        inc     dword ptr [ebp]+TsEip
        mov     eax, [ebp]+TsEax            ; ServiceClass
        mov     ecx, [ebp]+TsEcx            ; Arg1      (already loaded)
        mov     edx, [ebp]+TsEdx            ; Arg2      (already loaded)
        jmp     KiTrap03DebugService

_KiDebugService  endp

        page ,132
        subttl "Single Byte INT3 Breakpoin"
;++
;
; Routine Description:
;
;    Handle INT 3 breakpoint.
;
;    The trap is caused by a single byte INT 3 instruction.  A
;    BREAKPOINT exception with additional parameter indicating
;    READ access is raised for this trap if previous mode is user.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the instruction immediately
;    following the INT 3 instruction.
;    No error code is provided with the error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kit3_a, kit3_t, NoAbiosAssist
align dword
        public  _KiTrap03
_KiTrap03       proc
        push    0                       ; push dummy error code
        ENTER_TRAP      kit3_a, kit3_t

        cmp     ds:_PoHiberInProgress, 0
        jnz     short kit03_01

   lock inc     ds:_KiHardwareTrigger   ; trip hardware analyzer

kit03_01:
        mov     eax, BREAKPOINT_BREAK

KiTrap03DebugService:
;
; If caller is user mode, we want interrupts back on.
;   . all relevent state has already been saved
;   . user mode code always runs with ints on
;
; If caller is kernel mode, we want them off!
;   . some state still in registers, must prevent races
;   . kernel mode code can run with ints off
;
;
; Arguments:
;     eax - ServiceClass - which call is to be performed
;     ecx - Arg1 - generic first argument
;     edx - Arg2 - generic second argument

        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     kit03_30                ; fault occured in V86 mode => Usermode

        test    word ptr [ebp]+TsSegCs,MODE_MASK
        jz      kit03_10

        cmp     word ptr [ebp]+TsSegCs,KGDT_R3_CODE OR RPL_MASK
        jne     kit03_30

kit03_05:
        sti
kit03_10:


;
; Set up exception record and arguments for raising breakpoint exception
;

        mov     esi, ecx                ; ExceptionInfo 2
        mov     edi, edx                ; ExceptionInfo 3
        mov     edx, eax                ; ExceptionInfo 1

        mov     ebx, [ebp]+TsEip
        dec     ebx                     ; (ebx)-> int3 instruction
        mov     ecx, 3
        mov     eax, STATUS_BREAKPOINT
        call    CommonDispatchException ; Never return

kit03_30:
; Check to see if this process is a vdm

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      kit03_05


        stdCall _Ki386VdmReflectException_A, <03h>
        test    ax,0FFFFh
        jz      Kit03_10

        jmp     _KiExceptionExit

_KiTrap03       endp

        page ,132
        subttl "Integer Overflow"
;++
;
; Routine Description:
;
;    Handle INTO overflow.
;
;    The trap occurs when the processor encounters an INTO instruction
;    and the OF flag is set.
;
;    An INTEGER_OVERFLOW exception will be raised for this fault.
;
;    N.B. i386 will not generate fault if only OF flag is set.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the instruction immediately
;    following the INTO instruction.
;    No error code is provided with the error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kit4_a, kit4_t, NoAbiosAssist
align dword
        public  _KiTrap04
_KiTrap04       proc

        push    0                       ; push dummy error code
        ENTER_TRAP      kit4_a, kit4_t

        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     short Kt0430            ; in a vdm, reflect to vdm

        test    byte ptr [ebp]+TsSegCs,MODE_MASK
        jz      short Kt0410            ; in kernel mode, gen exception

        cmp     word ptr [ebp]+TsSegCs,KGDT_R3_CODE OR RPL_MASK
        jne     short Kt0420            ; maybe in a vdm

; Set up exception record and arguments for raising exception

Kt0410: sti
        mov     ebx, [ebp]+TsEip        ; (ebx)-> instr. after INTO
        dec     ebx                     ; (ebx)-> INTO
        mov     eax, STATUS_INTEGER_OVERFLOW
        jmp     CommonDispatchException0Args ; Never return

Kt0430:
        stdCall _Ki386VdmReflectException_A, <04h>
        test    al,0fh
        jz      Kt0410                  ; couldn't reflect, gen exception
        jmp     _KiExceptionExit

Kt0420:
; Check to see if this process is a vdm

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      Kt0410
        jmp     Kt0430

_KiTrap04       endp

        page ,132
        subttl "Bound Check fault"
;++
;
; Routine Description:
;
;    Handle bound check fault.
;
;    The bound check fault occurs if a BOUND instruction finds that
;    the tested value is outside the specified range.
;
;    For bound check fault, an ARRAY BOUND EXCEEDED exception will be
;    raised.
;    For kernel mode exception, it causes system to be terminated.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the faulting BOUND
;    instruction.
;    No error code is provided with the error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING
        ENTER_DR_ASSIST kit5_a, kit5_t, NoAbiosAssist
align dword
        public  _KiTrap05
_KiTrap05       proc

        push    0                       ; push dummy error code
        ENTER_TRAP      kit5_a, kit5_t

        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     Kt0530                  ; fault in V86 mode

        test    byte ptr [ebp]+TsSegCs, MODE_MASK  ; Is previous mode = USER
        jnz     short Kt0500            ; if nz, previous mode = user
        mov     eax, EXCEPTION_BOUND_CHECK ; (eax) = exception type
        jmp     _KiSystemFatalException ; go terminate the system


kt0500: cmp     word ptr [ebp]+TsSegCs,KGDT_R3_CODE OR RPL_MASK
        jne     short Kt0520            ; maybe in a vdm
;
; set exception record and arguments and call _KiDispatchException
;
Kt0510: sti
        mov     ebx, [ebp]+TsEip        ; (ebx)->BOUND instruction
        mov     eax, STATUS_ARRAY_BOUNDS_EXCEEDED
        jmp     CommonDispatchException0Args ; Won't return

Kt0520:
; Check to see if this process is a vdm

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      Kt0510

Kt0530:

        stdCall _Ki386VdmReflectException_A, <05h>
        test    al,0fh
        jz      Kt0510            ; couldn't reflect, gen exception
        jmp     _KiExceptionExit

_KiTrap05       endp

        page ,132
        subttl "Invalid OP code"
;++
;
; Routine Description:
;
;    Handle invalid op code fault.
;
;    The invalid opcode fault occurs if CS:EIP point to a bit pattern which
;    is not recognized as an instruction by the 386.  This may happen if:
;
;    1. the opcode is not a valid 80386 instruction
;    2. a register operand is specified for an instruction which requires
;       a memory operand
;    3. the LOCK prefix is used on an instruction that cannot be locked
;
;    If fault occurs in USER mode:
;       an Illegal_Instruction exception will be raised
;    if fault occurs in KERNEL mode:
;       system will be terminated.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the first byte of the invalid
;    instruction.
;    No error code is provided with the error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:FLAT, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kit6_a, kit6_t, NoAbiosAssist,, kit6_v
align dword
        public  _KiTrap06
_KiTrap06       proc


; sudeepb 08-Dec-1992 KiTrap06 is performance critical for VDMs,
; while it hardly ever gets executed in native mode. So this whole
; code is tuned for VDMs.

        test    dword ptr [esp]+8h,EFLAGS_V86_MASK
        jz      Kt060i

if FAST_BOP

        FAST_V86_TRAP_6
endif

Kt6SlowBop:
        push    0                       ; push dummy error code
        ENTER_TRAPV86 kit6_a, kit6_v
Kt06VMpf:

        ; Raise Irql to APC level before enabling interrupts
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql
        push    eax                     ; Save OldIrql
        sti

        call    VdmDispatchBop
        test    al,0fh
        jnz     short Kt061i

        stdCall   _Ki386VdmReflectException,<6>
        test    al,0fh
        jnz     Kt061i
        pop     ecx                     ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     Kt0635
Kt061i:
        pop     ecx                     ; (TOS) = OldIrql
        fstCall KfLowerIrql
        cli
        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jz      Kt062i

        EXIT_TRAPV86
        ;
        ; EXIT_TRAPv86 does not exit if a user mode apc has switched
        ; the context from V86 mode to flat mode (VDM monitor context)
        ;

Kt062i:
        jmp     _KiExceptionExit

Kt060i:
        push    0                       ; Push dummy error code
        ENTER_TRAP      kit6_a, kit6_t
Kt06pf:

        test    byte ptr [ebp]+TsSegCs, MODE_MASK  ; Is previous mode = USER
        jz      short Kt0635            ; if z, kernel mode - go dispatch exception

;
; UserMode. Did the fault happen in a vdm running in protected mode?
;

        cmp     word ptr [ebp]+TsSegCs, KGDT_R3_CODE OR RPL_MASK
        jz      short kt0605

; Check to see if this process is a vdm

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jnz     Kt0650

;
; Invalid Opcode exception could be either INVALID_LOCK_SEQUENCE or
; ILLEGAL_INSTRUCTION.
;

kt0605: sti
        mov     eax, [ebp]+TsSegCs
        push    ds
        mov     ds, ax
        mov     esi, [ebp]+TsEip        ; (ds:esi) -> address of faulting instruction
        mov     ecx, MAX_INSTRUCTION_PREFIX_LENGTH
Kt0610:
        lods    byte ptr [esi]          ; (al)= instruction byte
        cmp     al, MI_LOCK_PREFIX      ; Is it a lock prefix?
        je      short Kt0640            ; Yes, raise Invalid_lock exception
        loop    short Kt0610            ; keep on looping

;
; Set up exception record for raising Illegal instruction exception
;

Kt0630: pop     ds
Kt0635: sti
        mov     ebx, [ebp]+TsEip        ; (ebx)-> invalid instruction
        mov     eax, STATUS_ILLEGAL_INSTRUCTION
        jmp     CommonDispatchException0Args ; Won't return

;
; Set up exception record for raising Invalid lock sequence exception
;

Kt0640: pop     ds
        mov     ebx, [ebp]+TsEip        ; (ebx)-> invalid instruction
        mov     eax, STATUS_INVALID_LOCK_SEQUENCE
        jmp     CommonDispatchException0Args ; Won't return

Kt0650:

        ; Raise Irql to APC level before enabling interrupts
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql
        push    eax                                 ; SaveOldIrql
        sti
        call    VdmDispatchBop
        test    al,0fh
        jnz     short Kt0660

        stdCall   _Ki386VdmReflectException,<6>
        test    al,0fh
        jnz      Kt0660
        pop     ecx                         ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp      short Kt0635

Kt0660:
        pop     ecx                         ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     _KiExceptionExit

_KiTrap06       endp

        page ,132
        subttl "Coprocessor Not Avalaible"
;++
;
; Routine Description:
;
;   Handle Coprocessor not avaliable exception.
;
;   If we are REALLY emulating the 80387, the trap 07 vector is edited
;   to point directly at the emulator's entry point.  So this code is
;   only hit when an 80387 DOES exist.
;
;   The current threads coprocessor state is loaded into the
;   coprocessor.  If the coprocessor has a different threads state
;   in it (UP only) it is first saved away.  The thread is then continued.
;   Note: the threads state may contian the TS bit - In this case the
;   code loops back to the top of the Trap07 handler.  (which is where
;   we would end up if we let the thread return to user code anyway).
;
;   If the threads NPX context is in the coprocessor and we hit a Trap07
;   there is an NPX error which needs to be processed.  If the trap was
;   from usermode the error is dispatched.  If the trap was from kernelmode
;   the error is remembered, but we clear CR0 so the kernel code can
;   continue.  We can do this because the kernel mode code will restore
;   CR0 (and set TS) to signal a delayed error for this thread.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the first byte of the faulting
;    instruction.
;    No error code is provided with the error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kit7_a, kit7_t, NoAbiosAssist
align dword
        public  _KiTrap07
_KiTrap07       proc

        push    0                       ; push dummy error code
        ENTER_TRAP      kit7_a, kit7_t
Kt0700:
        mov     eax, PCR[PcPrcbData+PbCurrentThread]
        mov     ecx, PCR[PcInitialStack] ; (ecx) -> top of kernel stack
        cli                             ; don't context switch
        test    dword ptr [ecx].FpCr0NpxState,CR0_EM
        jnz     Kt07140

Kt0701: cmp     byte ptr [eax].ThNpxState, NPX_STATE_LOADED
        je      Kt0715

;
; Trap occured and this threads NPX state is not loaded.  Load it now
; and resume the application.  If someone elses state is in the coprocessor
; (uniprocessor implementation only) then save it first.
;

        mov     ebx, cr0
        and     ebx, NOT (CR0_MP+CR0_TS+CR0_EM)
        mov     cr0, ebx                ; allow frstor (& fnsave) to work

ifdef NT_UP
Kt0702:
        mov     edx, PCR[PcPrcbData+PbNpxThread] ; Owner of NPX state
        or      edx, edx                ; NULL?
        jz      Kt0704                  ; Yes - skip save

;
; Due to an hardware errata we need to know that the coprocessor
; doesn't generate an error condition once interrupts are disabled and
; trying to perform an fnsave which could wait for the error condition
; to be handled.
;
; The fix for this errata is that we "know" that the coprocessor is
; being used by a different thread then the one which may have caused
; the error condition.  The round trip time to swap to a new thread
; is longer then ANY floating point instruction.  We therefore know
; that any possible coprocessor error has already occured and been
; handled.
;
        mov     esi,[edx].ThInitialStack
        sub     esi, NPX_FRAME_LENGTH   ; Space for NPX_FRAME

        test    byte ptr _KeI386FxsrPresent, 1  ; Is FXSR feature present
        jz      short Kt0703a
        FXSAVE_ESI
        jmp     short Kt0703b
Kt0703a:
        fnsave  [esi]                   ; Save threads coprocessor state
Kt0703b:
        mov     byte ptr [edx].ThNpxState, NPX_STATE_NOT_LOADED
Kt0704:
endif

;
; Load current threads coprocessor state into the coprocessor
;
; (eax) - CurrentThread
; (ecx) - CurrentThreads NPX save area
; (ebx) - CR0
; (ebp) - trap frame
; Interrupts disabled
;

;
; frstor might generate a NPX execption if there's an error image being
; loaded.  The handler will simply set the TS bit for this context an iret.
;

        test    byte ptr _KeI386FxsrPresent, 1  ; Is FXSR feature present
        jz      short Kt0704b

ifndef NT_UP
if 0            ; FpNpxSavedCpu is broken - disable it
;
; We need not load the NPX state if
;   - PCR[PbNpxThread] matches new thread AND
;   - FpNpxSavedCpu matches the current processor

        mov     edx, PCR[PcSelfPcr]
        cmp     [edx+PcPrcbData+PbNpxThread], eax
        jne     Kt0704a

        cmp     dword ptr [ecx].FpNpxSavedCpu, edx
        jne     short Kt0704a
        jmp     short Kt0704c
Kt0704a:
        mov     dword ptr [ecx].FpNpxSavedCpu, edx ; Remember processor
endif
endif
        FXRSTOR_ECX                     ; reload NPX context
        jmp     short Kt0704c
Kt0704b:
        frstor  [ecx]                   ; reload NPX context

Kt0704c:
        mov     byte ptr [eax].ThNpxState, NPX_STATE_LOADED
        mov     PCR[PcPrcbData+PbNpxThread], eax  ; owner of coprocessors state

        sti                             ; Allow interrupts & context switches
        nop                             ; sti needs one cycle

        cmp     dword ptr [ecx].FpCr0NpxState, 0
        jz      _KiExceptionExit        ; nothing to set, skip CR0 reload

;
; Note: we have to get the CR0 value again to insure that we have the
;       correct state for TS.  We may have context switched since
;       the last move from CR0, and our npx state may have been moved off
;       of the npx.
;
        cli
if DBG
        test    dword ptr [ecx].FpCr0NpxState, NOT (CR0_MP+CR0_EM+CR0_TS)
        jnz short Kt07dbg1
endif
        mov     ebx,CR0
        or      ebx, [ecx].FpCr0NpxState
        mov     cr0, ebx                ; restore threads CR0 NPX state
        sti
        test    ebx, CR0_TS             ; Setting TS?  (delayed error)
        jz      _KiExceptionExit        ; No - continue

        jmp     Kt0700                  ; Dispatch delayed exception
if DBG
Kt07dbg1:    int 3
Kt07dbg2:    int 3
Kt07dbg3:    int 3
        sti
        jmp short $-2
endif

Kt0705:
;
; A Trap07 or Trap10 has occured from a ring 0 ESCAPE instruction.  This
; may occur when trying to load the coprocessors state.  These
; code paths rely on Cr0NpxState to signal a delayed error (not CR0) - we
; set CR0_TS in Cr0NpxState to get a delayed error, and make sure CR0 CR0_TS
; is not set so the R0 ESC instruction(s) can complete.
;
; (ecx) - CurrentThreads NPX save area
; (ebp) - trap frame
; Interrupts disabled
;

if DBG
        mov     eax, cr0                    ; Did we fault because some bit in CR0
        test    eax, (CR0_TS+CR0_MP+CR0_EM)
        jnz     short Kt07dbg3
endif

        or      dword ptr [ecx].FpCr0NpxState, CR0_TS   ; signal a delayed error

        cmp     dword ptr [ebp]+TsEip, Kt0704b  ; Is this fault on reload a thread's context?
        jne     short Kt0716                ; No, dispatch exception

        add     dword ptr [ebp]+TsEip, 3    ; Skip frstor ecx instruction
        jmp     _KiExceptionExit

Kt0710:
        mov     eax, PCR[PcPrcbData+PbCurrentThread]
        mov     ecx, PCR[PcInitialStack] ; (ecx) -> top of kernel stack

Kt0715:
        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     Kt07130                 ; v86 mode

        test    byte ptr [ebp]+TsSegCs, MODE_MASK ; Is previousMode=USER?
        jz      Kt0705                  ; if z, previousmode=SYSTEM

        cmp     word ptr [ebp]+TsSegCs,KGDT_R3_CODE OR RPL_MASK
        jne     Kt07110

;
; We are about to dispatch a floating point exception to user mode.
; We need to check to see if the user's NPX instruction is suppose to
; cause an exception or not.
;
; (ecx) - CurrentThreads NPX save area
;

Kt0716: stdCall _Ki386CheckDelayedNpxTrap,<ebp,ecx>
        or      al, al
        jnz     _KiExceptionExit        ; Already handled

        mov     eax, PCR[PcPrcbData+PbCurrentThread]
        mov     ecx, PCR[PcInitialStack] ; (ecx) -> top of kernel stack

Kt0720:
;
; Some type of coprocessor exception has occured for the current thread.
;
; (eax) - CurrentThread
; (ecx) - CurrentThreads NPX save area
; (ebp) - TrapFrame
; Interrupts disabled
;
        mov     ebx, cr0
        and     ebx, NOT (CR0_MP+CR0_EM+CR0_TS)
        mov     cr0, ebx                ; Clear MP+TS+EM to do fnsave & fwait

;
; Save the faulting state so we can inspect the cause of the floating
; point fault
;

        test    byte ptr _KeI386FxsrPresent, 1  ; Is FXSR feature present
        jz      short Kt0725a
        FXSAVE_ECX
        jmp     short Kt0725b
Kt0725a:
        fnsave  [ecx]                   ; Save threads coprocessor state
        fwait                           ; in case fnsave hasn't finished yet
Kt0725b:

if DBG
        test    dword ptr [ecx].FpCr0NpxState, NOT (CR0_MP+CR0_EM+CR0_TS)
        jnz     Kt07dbg2
endif
        or      ebx, NPX_STATE_NOT_LOADED
        or      ebx,[ecx]+FpCr0NpxState ; restore this threads CR0 NPX state
        mov     cr0, ebx                ; set TS so next ESC access causes trap

;
; Clear TS bit in Cr0NpxFlags in case it was set to trigger this trap.
;
        and     dword ptr [ecx].FpCr0NpxState, NOT CR0_TS

;
; The state is no longer in the coprocessor.  Clear ThNpxState and
; re-enable interrupts to allow context switching.
;
        mov     byte ptr [eax].ThNpxState, NPX_STATE_NOT_LOADED
        mov     dword ptr PCR[PcPrcbData+PbNpxThread], 0  ; No state in coprocessor
        sti

;
; According to the floating error priority, we test what is the cause of
; the NPX error and raise an appropriate exception.
;

        test    byte ptr _KeI386FxsrPresent, 1  ; Is FXSR feature present
        jz      short Kt0727a

        mov     ebx, [ecx] + FxErrorOffset
        movzx   eax, word ptr [ecx] + FxControlWord
        movzx   edx, word ptr [ecx] + FxStatusWord
        mov     esi, [ecx] + FxDataOffset ; (esi) = operand addr
        jmp     short Kt0727b

Kt0727a:
        mov     ebx, [ecx] + FpErrorOffset
        movzx   eax, word ptr [ecx] + FpControlWord
        movzx   edx, word ptr [ecx] + FpStatusWord
        mov     esi, [ecx] + FpDataOffset ; (esi) = operand addr

Kt0727b:

        and     eax, FSW_INVALID_OPERATION + FSW_DENORMAL + FSW_ZERO_DIVIDE + FSW_OVERFLOW + FSW_UNDERFLOW + FSW_PRECISION
        not     eax                        ; ax = mask of enabled exceptions
        and     eax, edx
        test    eax, FSW_INVALID_OPERATION ; Is it an invalid op exception?
        jz      short Kt0740               ; if z, no, go Kt0740
        test    eax, FSW_STACK_FAULT       ; Is it caused by stack fault?
        jnz     short Kt0730               ; if nz, yes, go Kt0730

; Raise Floating reserved operand exception
;

        mov     eax, STATUS_FLOAT_INVALID_OPERATION
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt0730:
;
; Raise Access Violation exception for stack overflow/underflow
;

        mov     eax, STATUS_FLOAT_STACK_CHECK
        jmp     CommonDispatchException2Args0d ; Won't return

Kt0740:

; Check for floating zero divide exception

        test    eax, FSW_ZERO_DIVIDE    ; Is it a zero divide error?
        jz      short Kt0750            ; if z, no, go Kt0750

; Raise Floating divided by zero exception

        mov     eax, STATUS_FLOAT_DIVIDE_BY_ZERO
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt0750:

; Check for denormal error

        test    eax, FSW_DENORMAL       ; Is it a denormal error?
        jz      short Kt0760            ; if z, no, go Kt0760

; Raise floating reserved operand exception

        mov     eax, STATUS_FLOAT_INVALID_OPERATION
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt0760:

; Check for floating overflow error

        test    eax, FSW_OVERFLOW       ; Is it an overflow error?
        jz      short Kt0770            ; if z, no, go Kt0770

; Raise floating overflow exception

        mov     eax, STATUS_FLOAT_OVERFLOW
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt0770:

; Check for floating underflow error

        test    eax, FSW_UNDERFLOW      ; Is it a underflow error?
        jz      short Kt0780            ; if z, no, go Kt0780

; Raise floating underflow exception

        mov     eax, STATUS_FLOAT_UNDERFLOW
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt0780:

; Check for precision (IEEE inexact) error

        test    eax, FSW_PRECISION      ; Is it a precision error
        jz      short Kt07100           ; if z, no, go Kt07100

        mov     eax, STATUS_FLOAT_INEXACT_RESULT
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt07100:

; If status word does not indicate error, then something is wrong...
;
; There is a known bug on Cyrix processors, upto and including
; the MII that causes Trap07 for no real reason (INTR is asserted
; during an FP instruction and is held high too long, we end up
; in the Trap07 handler with not exception set).   Bugchecking seems
; a little heavy handed, if this is a Cyrix processor, just ignore
; the error.

        cmp     _KiIgnoreUnexpectedTrap07, 0
        jnz     _KiExceptionExit

; stop the system
        sti
        stdCall   _KeBugCheck, <TRAP_CAUSE_UNKNOWN>

Kt07110:
; Check to see if this process is a vdm

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      Kt0720                  ; no, dispatch exception

Kt07130:
; Turn off TS
        mov     ebx,CR0
        and     ebx,NOT CR0_TS
        mov     CR0,ebx
        and     dword ptr [ecx]+FpCr0NpxState,NOT CR0_TS


; Reflect the exception to the vdm, the VdmHandler enables interrupts

        ; Raise Irql to APC level before enabling interrupts
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql
        push    eax                         ; Save OldIrql
        sti

        stdCall   _VdmDispatchIRQ13, <ebp> ; ebp - Trapframe
        test    al,0fh
        jnz     Kt07135
        pop     ecx                         ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     Kt0720            ; could not reflect, gen exception

Kt07135:
        pop     ecx                         ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     _KiExceptionExit

Kt07140:

;
; Insure that this is not an NPX instruction in the kernel. (If
; an app, such as C7, sets the EM bit after executing NPX instructions,
; the fsave in SwapContext will catch an NPX exception
;
        cmp     [ebp].TsSegCS, word ptr KGDT_R0_CODE
        je      Kt0701

;
; Check to see if it really is a VDM
;
        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      Kt07100

; A vdm is emulating NPX instructions on a machine with an NPX.

        stdCall _Ki386VdmReflectException_A, <07h>
        test    al,0fh
        jnz     _KiExceptionExit

        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
        mov     eax, STATUS_ACCESS_VIOLATION
        mov     esi, -1
        jmp     CommonDispatchException2Args0d ; Won't return

_KiTrap07       endp


        page ,132
        subttl "Double Fault"
;++
;
; Routine Description:
;
;    Handle double exception fault.
;
;    Normally, when the processor detects an exception while trying to
;    invoke the handler for a prior exception, the two exception can be
;    handled serially.  If, however, the processor cannot handle them
;    serially, it signals the double-fault exception instead.
;
;    If double exception is detected, no matter previous mode is USER
;    or kernel, a bugcheck will be raised and the system will be terminated.
;
; Arguments:
;
;    error code, which is always zero, is pushed on stack.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING
align dword
        public  _KiTrap08
_KiTrap08       proc
.FPO (0, 0, 0, 0, 0, 2)

        cli
;
; Update the TSS pointer in the PCR to point to the double-fault TSS
; (which is what we're running on, or else we wouldn't be here)
;
        mov     eax, PCR[PcGdt]
        mov     ch, [eax+KGDT_DF_TSS+KgdtBaseHi]
        mov     cl, [eax+KGDT_DF_TSS+KgdtBaseMid]
        shl     ecx, 16
        mov     cx, [eax+KGDT_DF_TSS+KgdtBaseLow]
        mov     PCR[PcTss], ecx

;
; Clear the busy bit in the TSS selector
;
        mov     ecx, PCR[PcGdt]
        lea     eax, [ecx] + KGDT_DF_TSS
        mov     byte ptr [eax+5], 089h  ; 32bit, dpl=0, present, TSS32, not busy

;
; Clear Nested Task bit in EFLAGS
;
        pushfd
        and     [esp], not 04000h
        popfd

;
; The original machine context is in original task's TSS
;
@@:     stdCall _KeBugCheckEx,<UNEXPECTED_KERNEL_MODE_TRAP,8,0,0,0>
        jmp     short @b        ; do not remove - for debugger

_KiTrap08       endp

        page ,132
        subttl "Coprocessor Segment Overrun"
;++
;
; Routine Description:
;
;    Handle Coprocessor Segment Overrun exception.
;
;    This exception only occurs on the 80286 (it's a trap 0d on the 80386),
;    so choke if we get here.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the aborted instruction.
;    No error code is provided with the error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kit9_a, kit9_t, NoAbiosAssist
align dword
        public  _KiTrap09
_KiTrap09       proc

        push    0                       ; push dummy error code
        ENTER_TRAP  kit9_a, kit9_t
        sti

        mov     eax, EXCEPTION_NPX_OVERRUN ; (eax) = exception type
        jmp     _KiSystemFatalException ; go terminate the system

_KiTrap09       endp

        page ,132
        subttl "Invalid TSS exception"
;++
;
; Routine Description:
;
;    Handle Invalid TSS fault.
;
;    This exception occurs if a segment exception other than the
;    not-present exception is detected when loading a selector
;    from the TSS.
;
;    If the exception is caused as a result of the kernel, device
;    drivers, or user incorrectly setting the NT bit in the flags
;    while the back-link selector in the TSS is invalid and the
;    IRET instruction being executed, in this case, this routine
;    will clear the NT bit in the trap frame and restart the iret
;    instruction.  For other causes of the fault, the user process
;    will be terminated if previous mode is user and the system
;    will stop if the exception occurs in kernel mode.  No exception
;    is raised.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the faulting instruction or
;    the first instruction of the task if the fault occurs as part of
;    a task switch.
;    Error code containing the segment causing the exception is provided.
;
;    NT386 does not use TSS for context switching.  So, the invalid tss
;    fault should NEVER occur.  If it does, something is wrong with
;    the kernel.  We simply shutdown the system.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kita_a, kita_t, NoAbiosAssist
align dword
        public  _KiTrap0A
_KiTrap0A       proc

        ENTER_TRAP  kita_a, kita_t

        ; We can not enable interrupt here.  If we came here because DOS/WOW
        ; iret with NT bit set, it is possible that vdm will swap the trap frame
        ; with their monitor context.  If this happen before we check the NT bit
        ; we will bugcheck.

;       sti

;
;   If the trap occur in USER mode and is caused by iret instruction with
;   OF bit set, we simply clear the OF bit and restart the iret.
;   Any other causes of Invalid TSS cause system to be shutdown.
;

        test    dword ptr [ebp]+TsEFlags, EFLAGS_V86_MASK
        jnz     short Kt0a10

        test    byte ptr [ebp]+TsSegCs, MODE_MASK  ; Is previous mode = USER
        jz      short Kt0a20

Kt0a10:
        test    dword ptr [ebp]+TsEFlags, EFLAGS_OF_BIT
        sti
        jz      short Kt0a20

        and     dword ptr [ebp]+TsEFlags, NOT EFLAGS_OF_BIT
        jmp     _KiExceptionExit                ; restart the instruction

Kt0a20:
        mov     eax, EXCEPTION_INVALID_TSS ; (eax) = trap type
        jmp     _KiSystemFatalException ; go terminate the system

_KiTrap0A       endp

        page ,132
        subttl "Segment Not Present"
;++
;
; Routine Description:
;
;    Handle Segment Not Present fault.
;
;    This exception occurs when the processor finds the P bit 0
;    when accessing an otherwise valid descriptor that is not to
;    be loaded in SS register.
;
;    The only place the fault can occur (in kernel mode) is Trap/Exception
;    exit code.  Otherwise, this exception causes system to be terminated.
;    NT386 uses flat mode, the segment not present fault in Kernel mode
;    indicates system malfunction.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the faulting instruction or
;    the first instruction of the task if the fault occurs as part of
;    a task switch.
;    Error code containing the segment causing the exception is provided.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kitb_a, kitb_t, NoAbiosAssist

align dword
        public  _KiTrap0B
_KiTrap0B       proc

; Set up machine state frame for displaying

        ENTER_TRAP      kitb_a, kitb_t

;
;   Did the trap occur in a VDM?
;

        test    byte ptr [ebp]+TsSegCs, MODE_MASK  ; Is previous mode = USER
        jz      Kt0b30

        cmp     word ptr [ebp]+TsSegCs,KGDT_R3_CODE OR RPL_MASK
        je      Kt0b20

Kt0b10:

; Check to see if this process is a vdm

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      short Kt0b20

        ; Raise Irql to APC level before enabling interrupts
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql
        push    eax                             ; Save OldIrql
        sti

        stdCall   _Ki386VdmSegmentNotPresent
        test    eax, 0ffffh
        jz      short Kt0b15

        pop     ecx                                ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     _KiExceptionExit

Kt0b15:
        pop     ecx                                ; (TOS) = OldIrql
        fstCall KfLowerIrql

Kt0b20: sti
        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
        mov     esi, [ebp]+TsErrCode
        and     esi, 0FFFFh
        or      esi, RPL_MASK
        mov     eax, STATUS_ACCESS_VIOLATION
        jmp     CommonDispatchException2Args0d ; Won't return

Kt0b30:
;
; Check if the exception is caused by pop SegmentRegister.
; We need to deal with the case that user puts a NP selector in fs, ds, cs
; or es through kernel debugger. (kernel will trap while popping segment
; registers in trap exit code.)
; Note: We assume if the faulted instruction is pop segreg.  It MUST be
; in trap exit code.  So there MUST be a valid trap frame for the trap exit.
;

        mov     eax, [ebp]+TsEip        ; (eax)->faulted Instruction
        mov     eax, [eax]              ; (eax)= opcode of faulted instruction
        mov     edx, [ebp]+TsEbp        ; (edx)->previous trap exit trapframe

        add     edx, TsSegDs            ; [edx] = prev trapframe + TsSegDs
        cmp     al, POP_DS              ; Is it pop ds instruction?
        jz      Kt0b90                  ; if z, yes, go Kt0b90

        add     edx, TsSegEs - TsSegDs  ; [edx] = prev trapframe + TsSegEs
        cmp     al, POP_ES              ; Is it pop es instruction?
        jz      Kt0b90                  ; if z, yes, go Kt0b90

        add     edx, TsSegFs - TsSegEs  ; [edx] = prev trapframe + TsSegFs
        cmp     ax, POP_FS              ; Is it pop fs (2-byte) instruction?
        jz      Kt0b90                  ; If z, yes, go Kt0b90

        add     edx, TsSegGs - TsSegFs  ; [edx] = prev trapframe + TsSegGs
        cmp     ax, POP_GS              ; Is it pop gs (2-byte) instruction?
        jz      Kt0b90                  ; If z, yes, go Kt0b90

;
; The exception is not caused by pop instruction.  We still need to check
; if it is caused by iret (to user mode.)  Because user may have a NP
; cs and we will trap at iret in trap exit code.
;

        sti
        cmp     al, IRET_OP             ; Is it an iret instruction?
        jne     Kt0b199                 ; if ne, not iret, go bugcheck

        lea     edx, [ebp]+TsHardwareEsp ; (edx)->trapped iret frame
        test    dword ptr [edx]+4, RPL_MASK ; Check CS of iret addr
                                        ; Does the iret have ring transition?
        jz      Kt0b199                 ; if z, it's a real fault

;
; we trapped at iret while returning back to user mode. We will dispatch
; the exception back to user program.
;

        mov     ecx, (KTRAP_FRAME_LENGTH - 12) / 4
        lea     edx, [ebp]+TsErrCode
Kt0b40:
        mov     eax, [edx]
        mov     [edx+12], eax
        sub     edx, 4
        loop    Kt0b40

        add     esp, 12                 ; adjust esp and ebp
        add     ebp, 12
        jmp     Kt0b10

;        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
;        xor     edx, edx
;        mov     esi, [ebp]+TsErrCode
;        or      esi, RPL_MASK
;        and     esi, 0FFFFh
;        mov     ecx, 2
;        mov     eax, STATUS_ACCESS_VIOLATION
;        call    CommonDispatchException ; WOn't return

;
; The faulted instruction is pop seg
;

Kt0b90:
        mov     dword ptr [edx], 0      ; set the segment reg to 0 such that
                                        ; we will trap in user mode.
        EXIT_ALL NoRestoreSegs,,NoPreviousMode  ; RETURN

Kt0b199:
        mov     eax, EXCEPTION_SEGMENT_NOT_PRESENT ; (eax) = exception type
        jmp     _KiSystemFatalException ; terminate the system

_KiTrap0B       endp

        page ,132
        subttl "Stack segment fault"
;++
;
; Routine Description:
;
;    Handle Stack Segment fault.
;
;    This exception occurs when the processor detects certain problem
;    with the segment addressed by the SS segment register:
;
;    1. A limit violation in the segment addressed by the SS (error
;       code = 0)
;    2. A limit vioalation in the inner stack during an interlevel
;       call or interrupt (error code = selector for the inner stack)
;    3. If the descriptor to be loaded into SS has its present bit 0
;       (error code = selector for the not-present segment)
;
;    The exception should never occurs in kernel mode except when we
;    perform the iret back to user mode.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the faulting instruction or
;    the first instruction of the task if the fault occurs as part of
;    a task switch.
;    Error code (whose value depends on detected condition) is provided.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kitc_a, kitc_t, NoAbiosAssist
align dword
        public  _KiTrap0C
_KiTrap0C       proc

        ENTER_TRAP kitc_a, kitc_t

        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     Kt0c30

        test    byte ptr [ebp]+TsSegCs, MODE_MASK  ; Is previous mode = USER
        jz      Kt0c10

        cmp     word ptr [ebp]+TsSegCs, KGDT_R3_CODE OR RPL_MASK
        jne     Kt0c20                  ; maybe in a vdm

Kt0c00: sti
        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
        mov     edx, EXCEPT_LIMIT_ACCESS; assume it is limit violation
        mov     esi, [ebp]+TsHardwareEsp; (ecx) = User Stack pointer
        cmp     word ptr [ebp]+TsErrCode, 0 ; Is errorcode = 0?
        jz      short kt0c05            ; if z, yes, go dispatch exception

        mov     esi, [ebp]+TsErrCode    ; Otherwise, set SS segment value
                                        ;   to be the address causing the fault
        mov     edx, EXCEPT_UNKNOWN_ACCESS
        or      esi, RPL_MASK
        and     esi, 0FFFFh
kt0c05: mov     eax, STATUS_ACCESS_VIOLATION
        jmp     CommonDispatchException2Args ; Won't return

kt0c10:
        sti
;
; Check if the exception is caused by kernel mode iret to user code.
; We need to deal with the case that user puts a bogus value in ss
; through SetContext call. (kernel will trap while iret to user code
; in trap exit code.)
; Note: We assume if the faulted instruction is iret.  It MUST be in
; trap/exception exit code.  So there MUST be a valid trap frame for
; the trap exit.
;

        mov     eax, [ebp]+TsEip        ; (eax)->faulted Instruction
        mov     eax, [eax]              ; (eax)= opcode of faulted instruction

;
; Check if the exception is caused by iret (to user mode.)
; Because user may have a NOT PRESENT ss and we will trap at iret
; in trap exit code. (If user put a bogus/not valid SS in trap frame, we
; will catch it in trap 0D handler.
;

        cmp     al, IRET_OP             ; Is it an iret instruction?
        jne     Kt0c15            ; if ne, not iret, go bugcheck

        lea     edx, [ebp]+TsHardwareEsp ; (edx)->trapped iret frame
        test    dword ptr [edx]+4, RPL_MASK ; Check CS of iret addr
                                        ; Does the iret have ring transition?
        jz      Kt0c15            ; if z, no SS involed, it's a real fault

;
; we trapped at iret while returning back to user mode. We will dispatch
; the exception back to user program.
;

        mov     ecx, (KTRAP_FRAME_LENGTH - 12) / 4
        lea     edx, [ebp]+TsErrCode
@@:
        mov     eax, [edx]
        mov     [edx+12], eax
        sub     edx, 4
        loop    @b

        add     esp, 12                 ; adjust esp and ebp
        add     ebp, 12

        ;
        ; Now, we have user mode trap frame set up
        ;

        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
        mov     edx, EXCEPT_LIMIT_ACCESS; assume it is limit violation
        mov     esi, [ebp]+TsHardwareEsp; (ecx) = User Stack pointer
        cmp     word ptr [ebp]+TsErrCode, 0 ; Is errorcode = 0?
        jz      short @f                ; if z, yes, go dispatch exception

        mov     esi, [ebp]+TsErrCode    ; Otherwise, set SS segment value
                                        ;   to be the address causing the fault
        and     esi, 0FFFFh
        mov     edx, EXCEPT_UNKNOWN_ACCESS
        or      esi, RPL_MASK
@@:
        mov     eax, STATUS_ACCESS_VIOLATION
        jmp     CommonDispatchException2Args ; Won't return

Kt0c15:
        mov     eax, EXCEPTION_STACK_FAULT      ; (eax) = trap type
        jmp     _KiSystemFatalException

Kt0c20:
; Check to see if this process is a vdm

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      Kt0c00

Kt0c30:
        stdCall _Ki386VdmReflectException_A,<0ch>
        test    al,0fh
        jz      Kt0c00
        jmp     _KiExceptionExit

_KiTrap0C       endp


        page ,132
        subttl "General Protection Fault"
;++
;
; Routine Description:
;
;    Handle General protection fault.
;
;    First, check to see if the fault occured in kernel mode with
;    incorrect selector values.  If so, this is a lazy segment load.
;    Correct the selector values and restart the instruction.  Otherwise,
;    parse out various kinds of faults and report as exceptions.
;
;    All protection violations that do not cause another exception
;    cause a general exception.  If the exception indicates a violation
;    of the protection model by an application program executing a
;    previleged instruction or I/O reference, a PRIVILEGED INSTRUCTION
;    exception will be raised.  All other causes of general protection
;    fault cause a ACCESS VIOLATION exception to be raised.
;
;    If previous mode = Kernel;
;        the system will be terminated  (assuming not lazy segment load)
;    Else previous mode = USER
;        the process will be terminated if the exception was not caused
;        by privileged instruction.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the faulting instruction or
;    the first instruction of the task if the fault occurs as part of
;    a task switch.
;    Error code (whose value depends on detected condition) is provided.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:FLAT, SS:NOTHING, ES:FLAT


;
;   Error and exception blocks for KiTrap0d
;

Ktd_ExceptionHandler:

;
; WARNING: Here we directly unlink the exception handler from the
; exception registration chain.  NO unwind is performed.
;

        mov     esp, [esp+8]            ; (esp)-> ExceptionList
        pop     PCR[PcExceptionList]
        add     esp, 4                  ; pop out except handler
        pop     ebp                     ; (ebp)-> trap frame

        test    dword ptr [ebp].TsSegCs, MODE_MASK ; if premode=kernl
        jnz     Kt0d103                 ; nz, prevmode=user, go return

; raise bugcheck if prevmode=kernel
        stdCall   _KeBugCheck, <KMODE_EXCEPTION_NOT_HANDLED>

        ENTER_DR_ASSIST kitd_a, kitd_t, NoAbiosAssist,, kitd_v
align dword
        public  _KiTrap0D
_KiTrap0D       proc



;
; Did the trap occur in a VDM in V86 mode? Trap0d is not critical from
; performance point of view to native NT, but its super critical to
; VDMs. So here we are doing every thing to make v86 mode most efficient.
;
        test    dword ptr [esp]+0ch,EFLAGS_V86_MASK
        jz      Ktdi


if FAST_V86_TRAP

        FAST_V86_TRAP_D
endif


KtdV86Slow:
        ENTER_TRAPV86 kitd_a, kitd_v

KtdV86Slow2:

        ; Raise Irql to APC level, before enabling interrupts
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql
        push    eax                             ; Save OldIrql
        sti

        stdCall   _Ki386DispatchOpcodeV86
KtdV86Exit:
        test    al,0FFh
        jnz     short Ktdi2

        stdCall   _Ki386VdmReflectException,<0dh>
        test    al,0fh
        jnz     short Ktdi2
        pop     ecx                                ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     Kt0d105
Ktdi2:
        pop     ecx                                ; (TOS) = OldIrql
        fstCall KfLowerIrql
        cli
        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jz      Ktdi3

        EXIT_TRAPV86
        ;
        ; EXIT_TRAPv86 does not exit if a user mode apc has switched
        ; the context from V86 mode to flat mode (VDM monitor context)
        ;

Ktdi3:
        jmp     _KiExceptionExit

Ktdi:
        ENTER_TRAP kitd_a, kitd_t

;
; DO NOT TURN INTERRUPTS ON!  If we're doing a lazy segment load,
; could be in an ISR or other code that needs ints off!
;

;
; Is this just a lazy segment load?  First make sure the exception occurred
; in kernel mode.
;

        test    dword ptr [ebp]+TsSegCs,MODE_MASK
        jnz     Kt0d02                  ; not kernel mode, go process normally

;
; Before handling kernel mode trap0d, we need to do some checks to make
; sure the kernel mode code is the one to blame.
;

if FAST_BOP or FAST_V86_TRAP

        cmp     byte ptr PCR[PcVdmAlert], 0      ; See Kt0eVdmAlert
        jne     Kt0eVdmAlert
endif

;
; Check if the exception is caused by the handler trying to examine offending
; instruction.  If yes, we raise exception to user mode program.  This occurs
; when user cs is bogus.  Note if cs is valid and eip is bogus, the exception
; will be caught by page fault and out Ktd_ExceptionHandler will be invoked.
; Both cases, the exception is dispatched back to user mode.
;

        mov     eax, [ebp]+TsEip
        cmp     eax, offset FLAT:Kt0d03
        jbe     short Kt0d000
        cmp     eax, offset FLAT:Kt0d60
        jae     short Kt0d000

        sti
        mov     ebp, [ebp]+TsEbp        ; remove the current trap frame
        mov     esp, ebp                ; set ebp, esp to previous trap frame
        jmp     Kt0d105                 ; and dispatch exception to user mode.

;
; Check if the exception is caused by pop SegmentRegister.
; We need to deal with the case that user puts a bogus value in fs, ds,
; or es through kernel debugger. (kernel will trap while popping segment
; registers in trap exit code.)
; Note: We assume if the faulted instruction is pop segreg.  It MUST be
; in trap exit code.  So there MUST be a valid trap frame for the trap exit.
;

Kt0d000:
        mov     eax, [ebp]+TsEip        ; (eax)->faulted Instruction
        mov     eax, [eax]              ; (eax)= opcode of faulted instruction
        mov     edx, [ebp]+TsEbp        ; (edx)->previous trap exit trapframe

        add     edx, TsSegDs            ; [edx] = prev trapframe + TsSegDs
        cmp     al, POP_DS              ; Is it pop ds instruction?
        jz      Kt0d005                 ; if z, yes, go Kt0d005

        add     edx, TsSegEs - TsSegDs  ; [edx] = prev trapframe + TsSegEs
        cmp     al, POP_ES              ; Is it pop es instruction?
        jz      Kt0d005                 ; if z, yes, go Kt0d005

        add     edx, TsSegFs - TsSegEs  ; [edx] = prev trapframe + TsSegFs
        cmp     ax, POP_FS              ; Is it pop fs (2-byte) instruction?
        jz      Kt0d005                 ; If z, yes, go Kt0d005

        add     edx, TsSegGs - TsSegFs  ; [edx] = prev trapframe + TsSegGs
        cmp     ax, POP_GS              ; Is it pop gs (2-byte) instruction?
        jz      Kt0d005                 ; If z, yes, go Kt0d005

;
; The exception is not caused by pop instruction.  We still need to check
; if it is caused by iret (to user mode.)  Because user may have a bogus
; ss and we will trap at iret in trap exit code.
;

        cmp     al, IRET_OP             ; Is it an iret instruction?
        jne     Kt0d002                 ; if ne, not iret, go check lazy load

        lea     edx, [ebp]+TsHardwareEsp ; (edx)->trapped iret frame
        mov     ax, [ebp]+TsErrCode     ; (ax) = Error Code
        and     ax, NOT RPL_MASK        ; No need to do this ...
        mov     cx, word ptr [edx]+4    ; [cx] = cs selector
        and     cx, NOT RPL_MASK
        cmp     cx, ax                  ; is it faulted in CS?
        jne     short Kt0d0008          ; No

;
; Check if this is the code which we use to return to Ki386CallBios
; (see biosa.asm):
;    cs should be KGDT_R0_CODE OR RPL_MASK
;    eip should be Ki386BiosCallReturnAddress
;    esi should be the esp of function Ki386SetUpAndExitToV86Code
; (edx) -> trapped iret frame
;

        mov     eax, OFFSET FLAT:Ki386BiosCallReturnAddress
        cmp     eax, [edx]              ; [edx]= trapped eip
                                        ; Is eip what we're expecting?
        jne     short Kt0d0005          ; No, continue

        mov     eax, [edx]+4            ; (eax) = trapped cs
        cmp     ax, KGDT_R0_CODE OR RPL_MASK ; Is Cs what we're exptecting?
        jne     short Kt0d0005          ; No

;       add     edx, 5 * 4 + NPX_FRAME_LENGTH + (TsV86Gs - TsHardwareSegSs)
;       cmp     [ebp]+TsEsi, edx        ; esi should be the esp of
                                        ;   Ki386SetupAndExitToV86Code
        mov     edx, [ebp].TsEsi
;       jne     short Kt0d0005
        mov     esp, edx
        jmp     Ki386BiosCallReturnAddress ; with interrupts off

Kt0d0005:
;
; Since the CS is bogus, we can not tell if we are going back
; to user mode...
;

        mov     ebx,PCR[PcPrcbData+PbCurrentThread] ; if previous mode is
        test    byte ptr [ebx]+ThPreviousMode, 0ffh ;   kernel, we bugcheck
        jz      Kt0d02

        or      word ptr [edx]+4, RPL_MASK
Kt0d0008:
        test    dword ptr [edx]+4, RPL_MASK ; Check CS of iret addr
                                        ; Does the iret have ring transition?
        jz      Kt0d02                  ; if z, no SS involed, it's a real fault

        sti

;
; we trapped at iret while returning back to user mode. We will dispatch
; the exception back to user program.
;

        mov     ecx, (KTRAP_FRAME_LENGTH - 12) / 4
        lea     edx, [ebp]+TsErrCode
Kt0d001:
        mov     eax, [edx]
        mov     [edx+12], eax
        sub     edx, 4
        loop    Kt0d001

        add     esp, 12                 ; adjust esp and ebp
        add     ebp, 12
        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
        mov     esi, [ebp]+TsErrCode
        and     esi, 0FFFFh
        mov     eax, STATUS_ACCESS_VIOLATION
        jmp     CommonDispatchException2Args0d ; Won't return

;
; Kernel mode, first opcode byte is 0f, check for rdmsr or wrmsr instruction
;

Kt0d001a:
        shr     eax, 8
        cmp     al, 30h
        je      short Kt0d001b
        cmp     al, 32h
        jne     short Kt0d002a

Kt0d001b:
        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
        mov     eax, STATUS_ACCESS_VIOLATION
        jmp     CommonDispatchException0Args ; Won't return

;
; The Exception is not caused by pop instruction.  Check to see
; if the instruction is a rdmsr or wrmsr
;
Kt0d002:
        cmp     al, 0fh
        je      short Kt0d001a

;
; We now check if DS and ES contain correct value.  If not, this is lazy
; segment load, we simply set them to valid selector.
;

Kt0d002a:
        cmp     word ptr [ebp]+TsSegDs, KGDT_R3_DATA OR RPL_MASK
        je      short Kt0d003

        mov     dword ptr [ebp]+TsSegDs, KGDT_R3_DATA OR RPL_MASK
        jmp     short Kt0d01

Kt0d003:
        cmp     word ptr [ebp]+TsSegEs, KGDT_R3_DATA OR RPL_MASK
        je      Kt0d02                  ; Real fault, go process it

        mov     dword ptr [ebp]+TsSegEs, KGDT_R3_DATA OR RPL_MASK
        jmp     short Kt0d01

;
; The faulted instruction is pop seg
;

Kt0d005:
        xor     eax, eax
        mov     dword ptr [edx], eax    ; set the segment reg to 0 such that
                                        ; we will trap in user mode.
Kt0d01:
        EXIT_ALL NoRestoreSegs,,NoPreviousMode  ; RETURN

;
;   Caller is not kernel mode, or DS and ES are OK.  Therefore this
;   is a real fault rather than a lazy segment load.  Process as such.
;   Since this is not a lazy segment load is now safe to turn interrupts on.
;
Kt0d02: mov     eax, EXCEPTION_GP_FAULT ; (eax) = trap type
        test    byte ptr [ebp]+TsSegCs, MODE_MASK ; Is prevmode=User?
        jz      _KiSystemFatalException ; If z, prevmode=kernel, stop...


;       preload pointer to process
        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess

;       flat or protect mode ?
        cmp     word ptr [ebp]+TsSegCs, KGDT_R3_CODE OR RPL_MASK
        jz      kt0d0201

;
; if vdm running in protected mode, handle instruction
;
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jnz     Kt0d110

        sti
        test    word ptr [ebp]+TsErrCode, 0 ; if errcode<>0, raise access
                                        ; violation exception
        jnz     Kt0d105                 ; if nz, raise access violation
        jmp     short Kt0d03


;
; if vdm running in flat mode, handle pop fs,gs by setting to Zero
;
kt0d0202:
        add     dword ptr [ebp].TsEip, 2
        jmp     Kt0d005

kt0d0201:
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jz      short Kt0d03

        mov     eax, [ebp]+TsEip        ; (eax)->faulted Instruction
        mov     eax, [eax]              ; (eax)= opcode of faulted instruction
        mov     edx, ebp                ; (edx)-> trap frame

        add     edx, TsSegFs            ; [edx] = prev trapframe + TsSegFs
        cmp     ax, POP_FS              ; Is it pop fs (2-byte) instruction?
        jz      short kt0d0202

        add     edx, TsSegGs - TsSegFs  ; [edx] = prev trapframe + TsSegGs
        cmp     ax, POP_GS              ; Is it pop gs (2-byte) instruction?
        jz      short kt0d0202

;
; we need to determine if the trap0d was caused by privileged instruction.
; First, we need to skip all the instruction prefix bytes
;

Kt0d03: sti
        push    ds

;
; First we need to set up an exception handler to handle the case that
; we fault while reading user mode instruction.
;

        push    ebp                     ; pass trapframe to handler
        push    offset FLAT:ktd_ExceptionHandler
                                        ; set up exception registration record
        push    PCR[PcExceptionList]
        mov     PCR[PcExceptionList], esp

        mov     esi, [ebp]+TsEip        ; (esi) -> flat address of faulting instruction
        mov     ax, [ebp]+TsSegCs
        mov     ds, ax
        mov     ecx, MAX_INSTRUCTION_LENGTH
Kt0d05: push    ecx                     ; save ecx for loop count
        lods    byte ptr [esi]          ; (al)= instruction byte
        mov     ecx, PREFIX_REPEAT_COUNT
        mov     edi, offset FLAT:PrefixTable ; (ES:EDI)->prefix table
        repnz   scasb                   ; search for matching (al)
        pop     ecx                     ; restore loop count
        jnz     short Kt0d10            ; (al) not a prefix byte, go kt0d10
        loop    short Kt0d05            ; go check for prefix byte again
        pop     PCR[PcExceptionList]
        add     esp, 8                  ; clear stack
        jmp     Kt0630                  ; exceed max instruction length,
                                        ; raise ILLEGALINSTRUCTION exception

;
; (al) = first opcode which is NOT prefix byte
; (ds:esi)= points to the first opcode which is not prefix byte + 1
; We need to check if it is one of the privileged instructions
;

Kt0d10: cmp     al, MI_HLT              ; Is it a HLT instruction?
        je      Kt0d80                  ; if e, yes, go kt0d80

        cmp     al, MI_TWO_BYTE         ; Is it a two-byte instruction?
        jne     short Kt0d50            ; if ne, no, go check for IO inst.

        lods    byte ptr [esi]          ; (al)= next instruction byte
        cmp     al, MI_LTR_LLDT         ; Is it a LTR or LLDT ?
        jne     short Kt0d20            ; if ne, no, go kt0d20

        lods    byte ptr [esi]          ; (al)= ModRM byte of instruction
        and     al, MI_MODRM_MASK       ; (al)= bit 3-5 of ModRM byte
        cmp     al, MI_LLDT_MASK        ; Is it a LLDT instruction?
        je      Kt0d80                  ; if e, yes, go Kt0d80

        cmp     al, MI_LTR_MASK         ; Is it a LTR instruction?
        je      Kt0d80                  ; if e, yes, go Kt0d80

        jmp     Kt0d100                 ; if ne, go raise access vioalation

Kt0d20: cmp     al, MI_LGDT_LIDT_LMSW   ; Is it one of these instructions?
        jne     short Kt0d30            ; if ne, no, go check special mov inst.

        lods    byte ptr [esi]          ; (al)= ModRM byte of instruction
        and     al, MI_MODRM_MASK       ; (al)= bit 3-5 of ModRM byte
        cmp     al, MI_LGDT_MASK        ; Is it a LGDT instruction?
        je      short Kt0d80            ; if e, yes, go Kt0d80

        cmp     al, MI_LIDT_MASK        ; Is it a LIDT instruction?
        je      short Kt0d80            ; if e, yes, go Kt0d80

        cmp     al, MI_LMSW_MASK        ; Is it a LMSW instruction?
        je      short Kt0d80            ; if e, yes, go Kt0d80

        jmp     Kt0d100                 ; else, raise access violation except

Kt0d30: and     al, MI_SPECIAL_MOV_MASK ; Is it a special mov instruction?
        jnz     kt0d80                  ; if nz, yes, go raise priv instr
                                        ; (Even though the regular mov may
                                        ; have the special_mov_mask bit set,
                                        ; they are NOT 2 byte opcode instr.)
        jmp     Kt0d100                 ; else, no, raise access violation

;
; Now, we need to check if the trap 0d was caused by IO privileged instruct.
; (al) = first opcode which is NOT prefix byte
; Also note, if we come here, the instruction has 1 byte opcode (still need to
; check REP case.)
;

Kt0d50: mov     ebx, [ebp]+TsEflags     ; (ebx) = client's eflags
        and     ebx, IOPL_MASK          ;
        shr     ebx, IOPL_SHIFT_COUNT   ; (ebx) = client's IOPL
        mov     ecx, [ebp]+TsSegCs
        and     ecx, RPL_MASK           ; RPL_MASK NOT MODE_MASK!!!
                                        ; (ecx) = CPL, 1/2 of computation of
                                        ; whether IOPL applies.

        cmp     ebx,ecx                 ; compare IOPL with CPL of caller
        jge     short Kt0d100           ; if ge, not IO privileged,
                                        ;        go raise access violation

Kt0d60: cmp     al, CLI_OP              ; Is it a CLI instruction
        je      short Kt0d80            ; if e, yes. Report it.

        cmp     al, STI_OP              ; Is it a STI?
        je      short Kt0d80            ; if e, yes, report it.

        mov     ecx, IO_INSTRUCTION_TABLE_LENGTH
        mov     edi, offset FLAT:IOInstructionTable
        repnz   scasb                   ; Is it a IO instruction?
        jnz     short Kt0d100           ; if nz, not io instrct.

;
; We know the instr is an IO instr without IOPL.  But, this doesn't mean
; this is a privileged instruction exception.  We need to make sure the
; IO port access is not granted in the bit map
;


        mov     edi, fs:PcSelfPcr       ; (edi)->Pcr
        mov     esi, [edi]+PcGdt        ; (esi)->Gdt addr
        add     esi, KGDT_TSS
        movzx   ebx, word ptr [esi]     ; (ebx) = Tss limit

        mov     edx, [ebp].TsEdx        ; [edx] = port addr
        mov     ecx, edx
        and     ecx, 07                 ; [ecx] = Bit position
        shr     edx, 3                  ; [edx] = offset to the IoMap

        mov     edi, [edi]+PcTss        ; (edi)->TSS
        movzx   eax, word ptr [edi + TssIoMapBase] ; [eax] = Iomap offset
        add     edx, eax
        cmp     edx, ebx                ; is the offset addr beyond tss limit?
        ja      short Kt0d80            ; yes, no I/O priv.

        add     edi, edx                ; (edi)-> byte correspons to the port addr
        mov     edx, 1
        shl     edx, cl
        test    dword ptr [edi], edx    ; Is the bit of the port disabled?
        jz      short Kt0d100           ; if z, no, then it is access violation

Kt0d80:
        pop     PCR[PcExceptionList]
        add     esp, 8                  ; clear stack
        pop     ds
        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
        mov     eax, STATUS_PRIVILEGED_INSTRUCTION
        jmp     CommonDispatchException0Args ; Won't return

;
;       NOTE    All the GP fault (except the ones we can
;               easily detect now) will cause access violation exception
;               AND, all the access violation will be raised with additional
;               parameters set to "read" and "virtual address which caused
;               the violation = unknown (-1)"
;

Kt0d100:
        pop     PCR[PcExceptionList]
        add     esp, 8                  ; clear stack
Kt0d103:
        pop     ds
Kt0d105:
        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
        mov     esi, -1
        mov     eax, STATUS_ACCESS_VIOLATION
        jmp     CommonDispatchException2Args0d ; Won't return

Kt0d110:
        ; Raise Irql to APC level, before enabling interrupts
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql
        push    eax                             ; Save OldIrql
        sti

        stdCall   _Ki386DispatchOpcode
        test    eax,0FFFFh
        jnz     short Kt0d120

        stdCall   _Ki386VdmReflectException,<0dh>
        test    al,0fh
        jnz     short Kt0d120
        pop     ecx                             ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     short Kt0d105

Kt0d120:
        pop     ecx                                ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     _KiExceptionExit

_KiTrap0D       endp

        page ,132
        subttl "Page faults on invalid addresses allowed in certain instances"
;++
; BOOLEAN
; FASTCALL
; KeInvalidAccessAllowed (
;    IN PVOID TrapInformation
;    )
;
;
; Routine Description:
;
;    Mm will pass a pointer to a trap frame prior to issuing a bug check on
;    a pagefault.  This routine lets Mm know if it is ok to bugcheck.  The
;    specific case we must protect are the interlocked pop sequences which can
;    blindly access memory that may have been freed and/or reused prior to the
;    access.  We don't want to bugcheck the system in these cases, so we check
;    the instruction pointer here.
;
; Arguments:
;
;    (ecx) - Trap frame pointer.  NULL means return False.
;
; Return value:
;
;    True if the invalid access should be ignored.
;    False which will usually trigger a bugcheck.
;
;--

cPublicFastCall KeInvalidAccessAllowed, 1
cPublicFpo 0,0

        cmp     ecx, 0                  ; caller gave us a trap frame?
        jne     Ktia01                  ; yes, so examine the frame
        mov     al, 0
        jmp     Ktia02                  ; no frame, go bugcheck
Ktia01:
        mov     edx, offset FLAT:ExpInterlockedPopEntrySListFault
        cmp     [ecx].TsEip, edx        ; check if fault at pop code address
        sete    al                      ; if yes, then don't bugcheck
Ktia02:
        fstRET  KeInvalidAccessAllowed

fstENDP KeInvalidAccessAllowed


;
; The following code it to fix a bug in the Pentium Proccesor dealing with
; Invalid Opcodes.
;


PentiumTest:                            ; Is this access to the write protect
                                        ; IDT page?
        test    [ebp]+TsErrCode, 04h    ; Do not allow user mode access to trap 6
        jne     NoPentiumFix            ; vector. Let page fault code deal with it
        mov     eax, PCR[PcIDT]         ; Get address of trap 6 IDT entry
        add     eax, (6 * 8)
        cmp     eax, edi                ; Is that the faulting address?
        jne     NoPentiumFix            ; No.  Back to page fault code

                                        ; Yes. We have accessed the write
                                        ; protect page of the IDT
        mov     [ebp]+TsErrCode, 0      ; Overwrite error code
        test    dword ptr [ebp]+TsEFlags, EFLAGS_V86_MASK ; Was it a VM?
        jne     Kt06VMpf                ; Yes.  Go to VM part of trap 6
        jmp     Kt06pf                  ; Not from a VM


        page ,132
        subttl "Page fault processing"
;++
;
; Routine Description:
;
;    Handle page fault.
;
;    The page fault occurs if paging is enabled and any one of the
;    conditions is true:
;
;    1. page not present
;    2. the faulting procedure does not have sufficient privilege to
;       access the indicated page.
;
;    For case 1, the referenced page will be loaded to memory and
;    execution continues.
;    For case 2, registered exception handler will be invoked with
;    appropriate error code (in most cases STATUS_ACCESS_VIOLATION)
;
;    N.B. It is assumed that no page fault is allowed during task
;    switches.
;
;    N.B. INTERRUPTS MUST REMAIN OFF UNTIL AFTER CR2 IS CAPTURED.
;
; Arguments:
;
;    Error code left on stack.
;    CR2 contains faulting address.
;    Interrupts are turned off at entry by use of an interrupt gate.
;
; Return value:
;
;    None
;
;--

        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING
        ENTER_DR_ASSIST kite_a, kite_t, NoAbiosAssist
align dword
        public  _KiTrap0E
_KiTrap0E       proc

        ENTER_TRAP      kite_a, kite_t

if FAST_V86_TRAP or FAST_BOP

        cmp     byte ptr PCR[PcVdmAlert], 0
        jne     Kt0eVdmAlert
endif

        VERIFY_BASE_TRAP_FRAME

        mov     edi,cr2
;
; Now that everything is in a sane state check for rest of the Pentium
; Processor bug work around for illegal operands
;

        cmp     _KiI386PentiumLockErrataPresent, 0
        jne     PentiumTest              ; Check for special problems

NoPentiumFix:                            ; No.  Skip it
        sti


        test    [ebp]+TsEFlags, EFLAGS_INTERRUPT_MASK   ; faulted with
        jz      Kt0e12b                 ; interrupts disabled?
Kt0e01:

;
; call _MmAccessFault to page in the not present page.  If the cause
; of the fault is 2, _MmAccessFault will return approriate error code
;

        sub     esp, 16
        mov     eax,[ebp]+TsSegCs
        and     eax,MODE_MASK           ; (eax) = arg3: PreviousMode
        mov     [esp+12], ebp           ; pass in the trap frame base
        mov     [esp+8], eax
        mov     [esp+4], edi
        mov     eax, [ebp]+TsErrCode    ; (eax)= error code
        and     eax, ERR_0E_STORE       ; (eax)= 0 if fault caused by read
                                        ;      = 2 if fault caused by write
        shr     eax, 1                  ; (eax) = 0 if read fault, 1 if write fault
        mov     [esp+0], eax            ; arg3: load/store indicator

        call    _MmAccessFault@16

if DEVL
        cmp     _PsWatchEnabled,0
        jz      short xxktskip
        mov     ebx, [ebp]+TsEip
        stdCall _PsWatchWorkingSet,<eax, ebx, edi>
xxktskip:
endif

        or      eax, eax                ; sucessful?
        jge     Kt0e10                  ; yes, go exit

        mov     ecx,PCR[PcGdt]

        ; Form Ldt Base
        movzx   ebx,byte ptr [ecx + KGDT_LDT].KgdtBaseHi
        shl     ebx,8
        or      bl,byte ptr [ecx + KGDT_LDT].KgdtBaseMid
        shl     ebx,16
        or      bx,word ptr [ecx + KGDT_LDT].KgdtBaseLow
        or      ebx,ebx                 ; check for zero
        jz      short Kt0e05            ; no ldt

        cmp     edi,ebx
        jb      short Kt0e05            ; address not in LDT

        ; Form Ldt limit
        movzx   edx,byte ptr [ecx + KGDT_LDT].KgdtLimitHi
        and     edx,000000FFh
        shl     edx,16
        or      dx,word ptr [ecx + KGDT_LDT].KgdtLimitLow
        add     ebx,edx
        cmp     edi,ebx
        jae     short Kt0e05            ; too high to be an ldt address

        sldt    cx                      ; Verify this process has an LDT
        test    ecx, 0ffffh             ; Check CX
        jz      short Kt0e05            ; If ZY, no LDT

        mov     eax, [ebp]+TsErrCode    ; (eax)= error code
        and     eax, ERR_0E_STORE       ; (eax)= 0 if fault caused by read
                                        ;      = 2 if fault caused by write
        shr     eax, 1                  ; (eax) = 0 if read fault, 1 if write fault
                                        ; call page fault handler
        stdCall   _MmAccessFault, <eax, edi, 0, ebp>

        or      eax, eax                ; successful?
        jge     Kt0e10                  ; if z, yes, go exit

        mov     ebx,[ebp]+TsSegCs       ; restore previous mode
        and     ebx,MODE_MASK
        mov     [esp + 8],ebx

;
; Check to determine if the fault occured in the interlocked pop entry slist
; code. There is a case where a fault may occur in this code when the right
; set of circumstances occurs. The fault can be ignored by simply skipping
; the faulting instruction.
;

Kt0e05: mov     ecx, offset FLAT:ExpInterlockedPopEntrySListFault ; get pop code address
        cmp     [ebp].TsEip, ecx        ; check if fault at pop code address
        je      Kt0e10a                 ; if eq, skip faulting instruction

;
;   Did the fault occur in KiSystemService while copying arguments from
;   user stack to kernel stack?
;

        mov     ecx, offset FLAT:KiSystemServiceCopyArguments
        cmp     [ebp].TsEip, ecx
        jne     short @f

        mov     ecx, [ebp].TsEbp        ; (eax)->TrapFrame of SysService
        test    [ecx].TsSegCs, MODE_MASK
        jz      short @f                ; caller of SysService is k mode, we
                                        ; will let it bugchecked.
        mov     [ebp].TsEip, offset FLAT:kss60
        mov     eax, STATUS_ACCESS_VIOLATION
        mov     [ebp].TsEax, eax
        jmp     _KiExceptionExit
@@:

        mov     ecx, [ebp]+TsErrCode    ; (ecx) = error code
        and     ecx, ERR_0E_STORE       ; (ecx) = 0 if fault caused by read
                                        ;         2 if fault caused by write
        shr     ecx,1                   ; (ecx) = load/store indicator
;
;   Did the fault occur in a VDM?
;
        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     Kt0e7

;
;   Did the fault occur in a VDM while running in protected mode?
;

        mov     esi,PCR[PcPrcbData+PbCurrentThread]
        mov     esi,[esi]+ThApcState+AsProcess
        test    byte ptr [esi]+PrVdmFlag,0fh ; is this a vdm process?
        jz      short Kt0e9             ; z -> not vdm

        test    dword ptr [ebp]+TsSegCs, MODE_MASK
        jz      short kt0e8

        cmp     word ptr [ebp]+TsSegCs, KGDT_R3_CODE OR RPL_MASK
        jz      kt0e9                   ; z -> not vdm

Kt0e7:  mov     esi, eax
        stdCall _VdmDispatchPageFault, <ebp,ecx,edi>
        test    al,0fh                  ; returns TRUE, if success
        jnz     Kt0e11                  ; Exit,No need to call the debugger

        mov     eax, esi
        jmp     short Kt0e9

Kt0e8:
;
;   Did the fault occur in our kernel VDM support code?
;   At this point, we know:
;      . Current process is VDM process
;      . this is a unresolvable pagefault
;      . the fault occurred in kernel mode.
;

;
;   First make sure this is not pagefault at irql > 1
;   which should be bugchecked.
;

        cmp     eax, STATUS_IN_PAGE_ERROR or 10000000h
        je      Kt0e12

        cmp     word ptr [ebp]+TsSegCs, KGDT_R0_CODE ; Did fault occur in kernel?
        jnz     short Kt0e9             ; if nz, no, not ours

        cmp     PCR[PcExceptionList], EXCEPTION_CHAIN_END
        jnz     short Kt0e9             ; there is at least a handler to
                                        ; handle this exception

        mov     ebp, PCR[PcInitialStack]
        xor     ecx, ecx                ; set to fault-at-read
        sub     ebp, KTRAP_FRAME_LENGTH
        mov     esp, ebp                ; clear stack (ebp)=(esp)->User trap frame
        mov     esi, [ebp]+TsEip        ; (esi)-> faulting instruction
        jmp     Kt0e9b                  ; go dispatching the exception to user

Kt0e9:
; Set up exception record and arguments and call _KiDispatchException
        mov     esi, [ebp]+TsEip        ; (esi)-> faulting instruction

        cmp     eax, STATUS_ACCESS_VIOLATION ; dispatch access violation or
        je      short Kt0e9b                 ; or in_page_error?

        cmp     eax, STATUS_GUARD_PAGE_VIOLATION
        je      short Kt0e9b

        cmp     eax, STATUS_STACK_OVERFLOW
        je      short Kt0e9b


;
; test to see if davec's reserved status code bit is set. If so, then bugchecka
;

        cmp     eax, STATUS_IN_PAGE_ERROR or 10000000h
        je      Kt0e12                  ; bugchecka

;
; (ecx) = ExceptionInfo 1
; (edi) = ExceptionInfo 2
; (eax) = ExceptionInfo 3
; (esi) -> Exception Addr
;

        mov     edx, ecx
        mov     ebx, esi
        mov     esi, edi
        mov     ecx, 3
        mov     edi, eax
        mov     eax, STATUS_IN_PAGE_ERROR
        call    CommonDispatchException ; Won't return

Kt0e9b:
        mov     ebx, esi
        mov     edx, ecx
        mov     esi, edi
        jmp     CommonDispatchException2Args ; Won't return

.FPO ( 0, 0, 0, 0, 0, FPO_TRAPFRAME )

;
; The fault occured in the interlocked pop slist function and the faulting
; instruction should be skipped.
;

Kt0e10a:mov     ecx, offset FLAT:ExpInterlockedPopEntrySListResume ; get resume address
        mov     [ebp].TsEip, ecx        ; set continuation address

Kt0e10:

if DEVL
        mov     esp,ebp                 ; (esp) -> trap frame
        test    _KdpOweBreakpoint, 1    ; do we have any owed breakpoints?
        jz      _KiExceptionExit        ; No, all done

        stdCall _KdSetOwedBreakpoints   ; notify the debugger
endif

Kt0e11: mov     esp,ebp                 ; (esp) -> trap frame
        jmp     _KiExceptionExit        ; join common code


Kt0e12:
        stdCall _KeGetCurrentIrql       ; (eax) = OldIrql
Kt0e12a:
   lock inc     ds:_KiHardwareTrigger   ; trip hardware analyzer

;
; bugcheck a, addr, irql, load/store, pc
;
        mov     ecx, [ebp]+TsErrCode    ; (ecx)= error code
        and     ecx, ERR_0E_STORE       ; (ecx)= 0 if fault caused by read
        shr     ecx, 1                  ; (ecx) = 0 if read fault, 1 if write fault

        mov     esi, [ebp]+TsEip        ; [esi] = faulting instruction

        stdCall _KeBugCheckEx,<IRQL_NOT_LESS_OR_EQUAL,edi,eax,ecx,esi>

Kt0e12b:
        ; In V86 mode with iopl allowed it is OK to handle
        ; a page fault with interrupts disabled

        test    dword ptr [ebp]+TsEFlags, EFLAGS_V86_MASK
        jz      short Kt0e12c

        cmp     _KeI386VdmIoplAllowed, 0
        jnz     Kt0e01

Kt0e12c:
        cmp     _KiFreezeFlag,0         ; during boot we can take
        jnz     Kt0e01                  ; 'transistion failts' on the
                                        ; debugger before it's been locked

        cmp     _KiBugCheckData, 0      ; If crashed, handle trap in
        jnz     Kt0e01                  ; normal manner


        mov     eax, 0ffh               ; OldIrql = -1
        jmp     short Kt0e12a

if FAST_BOP or FAST_V86_TRAP

Kt0eVdmAlert:
        ; If a page fault occured while we are in VDM alert mode (processing
        ; v86 trap without building trap frame), we will restore all the
        ; registers and return to its recovery routine which is stored in
        ; the TsSegGs of original trap frame.
        ;
        mov     eax, PCR[PcVdmAlert]
        mov     byte ptr PCR[PcVdmAlert], 0

IF FAST_V86_TRAP
        cmp     al, 0Dh
        jne     short @f

        mov     eax, offset FLAT:V86TrapDRecovery
        jmp     short KtvaExit

@@:
        cmp     al, 10h
        jne     short @f

        mov     eax, offset FLAT:V86Trap10Recovery
        jmp     short KtvaExit

@@:
ENDIF   ; FAST_V86_TRAP

IF FAST_BOP
        cmp     al, 6
        jne     short @f

        mov     eax, offset FLAT:V86Trap6Recovery
        jmp     short KtvaExit

@@:
ENDIF   ; FAST_BOP

        mov     eax, [ebp].TsEip
KtvaExit:
        mov     [ebp].TsEip, eax
        mov     esp,ebp                 ; (esp) -> trap frame
        jmp     _KiExceptionExit        ; join common code

if DBG
@@:     int 3
endif
ENDIF   ; FAST_BOP OR FAST_V86_TRAP
_KiTrap0E       endp

        page ,132
        subttl "Trap0F -- Intel Reserved"
;++
;
; Routine Description:
;
;    The trap 0F should never occur.  If, however, the exception occurs in
;    USER mode, the current process will be terminated.  If the exception
;    occurs in KERNEL mode, a bugcheck will be raised.  NO registered
;    handler, if any, will be inviked to handle the exception.
;
; Arguments:
;
;    None
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kitf_a, kitf_t, NoAbiosAssist
align dword
        public  _KiTrap0F
_KiTrap0F       proc

        push    0                       ; push dummy error code
        ENTER_TRAP      kitf_a, kitf_t
        sti

        mov     eax, EXCEPTION_RESERVED_TRAP ; (eax) = trap type
        jmp     _KiSystemFatalException ; go terminate the system

_KiTrap0F       endp


        page ,132
        subttl "Coprocessor Error"

;++
;
; Routine Description:
;
;    Handle Coprocessor Error.
;
;    This exception is used on 486 or above only.  For i386, it uses
;    IRQ 13 instead.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the aborted instruction.
;    No error code is provided with the error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kit10_a, kit10_t, NoAbiosAssist

align dword
        public  _KiTrap10
_KiTrap10       proc

        push    0                       ; push dummy error code
        ENTER_TRAP      kit10_a, kit10_t

        mov     eax, PCR[PcPrcbData+PbNpxThread]  ; Correct context for fault?
        cmp     eax, PCR[PcPrcbData+PbCurrentThread]
        je      Kt0710                  ; Yes - go try to dispatch it

;
; We are in the wrong NPX context and can not dispatch the exception right now.
; Set up the target thread for a delay exception.
;
; Note: we don't think this is a possible case, but just to be safe...
;
        mov     eax, [eax].ThInitialStack
        sub     eax, NPX_FRAME_LENGTH   ; Space for NPX_FRAME
        or      dword ptr [eax].FpCr0NpxState, CR0_TS   ; Set for delayed error

        jmp     _KiExceptionExit

_KiTrap10       endp

        page ,132
        subttl "Alignment fault"
;++
;
; Routine Description:
;
;    Handle alignment faults.
;
;    This exception occurs when an unaligned data access is made by a thread
;    with alignment checking turned on.
;
;    This exception will only occur on 486 machines.  The 386 will not do
;    any alignment checking.  Only threads which have the appropriate bit
;    set in EFLAGS will generate alignment faults.
;
;    The exception will never occur in kernel mode.  (hardware limitation)
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the faulting instruction.
;    Error code is provided.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kit11_a, kit11_t, NoAbiosAssist
align dword
        public  _KiTrap11
_KiTrap11       proc

        ENTER_TRAP kit11_a, kit11_t
        sti

        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     Kt11_01                 ; v86 mode => usermode

        test    byte ptr [ebp]+TsSegCs, MODE_MASK  ; Is previous mode = USER
        jz      Kt11_10

;
; Check to make sure that the AutoAlignment state of this thread is FALSE.
; If not, this fault occurred because the thread messed with its own EFLAGS.
; In order to "fixup" this fault, we just clear the ALIGN_CHECK bit in the
; EFLAGS and restart the instruction.  Exceptions will only be generated if
; AutoAlignment is FALSE.
;
Kt11_01:
        mov     ebx,PCR[PcPrcbData+PbCurrentThread] ; (ebx)-> Current Thread
        test    byte ptr [ebx].ThAutoAlignment, -1
        jz      kt11_00
;
; This fault was generated even though the thread had AutoAlignment set to
; TRUE.  So we fix it up by setting the correct state in his EFLAGS and
; restarting the instruction.
;
        and     dword ptr [ebp]+TsEflags, NOT EFLAGS_ALIGN_CHECK
        jmp     _KiExceptionExit

kt11_00:
        mov     ebx, [ebp]+TsEip        ; (ebx)->faulting instruction
        mov     edx, EXCEPT_LIMIT_ACCESS; assume it is limit violation
        mov     esi, [ebp]+TsHardwareEsp; (esi) = User Stack pointer
        cmp     word ptr [ebp]+TsErrCode, 0 ; Is errorcode = 0?
        jz      short kt11_05            ; if z, yes, go dispatch exception

        mov     edx, EXCEPT_UNKNOWN_ACCESS
kt11_05:
        mov     eax, STATUS_DATATYPE_MISALIGNMENT
        jmp     CommonDispatchException2Args ; Won't return

kt11_10:
;
; We should never be here, since the 486 will not generate alignment faults
; in kernel mode.
;
        mov     eax, EXCEPTION_ALIGNMENT_CHECK      ; (eax) = trap type
        jmp     _KiSystemFatalException

_KiTrap11       endp

;++
;
; Routine Description:
;
;    Handle XMMI Exception.
;;
; Arguments:
;
;    At entry, the saved CS:EIP point to the aborted instruction.
;    No error code is provided with the error.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        ENTER_DR_ASSIST kit13_a, kit13_t, NoAbiosAssist

align dword
        public  _KiTrap13
_KiTrap13       proc

        push    0                       ; push dummy error code
        ENTER_TRAP      kit13_a, kit13_t

        mov     eax, PCR[PcPrcbData+PbNpxThread]  ; Correct context for fault?
        cmp     eax, PCR[PcPrcbData+PbCurrentThread]
        je      Kt13_10                 ; Yes - go try to dispatch it

;
;       Katmai New Instruction exceptions are precise and occur immediately.
;       if we are in the wrong NPX context, bugcheck the system.
;
        ; stop the system
        stdCall _KeBugCheckEx,<TRAP_CAUSE_UNKNOWN,13,eax,0,0>

Kt13_10:
        mov     ecx, PCR[PcInitialStack] ; (ecx) -> top of kernel stack

;
;       TrapFrame is built by ENTER_TRAP.
;       XMMI are accessible from all IA execution modes:
;       Protected Mode, Real address mode, Virtual 8086 mode
;
Kt13_15:
        test    dword ptr [ebp]+TsEFlags,EFLAGS_V86_MASK
        jnz     Kt13_130                ; v86 mode

;
;       eflags.vm=0 (not v86 mode)
;
        test    byte ptr [ebp]+TsSegCs, MODE_MASK ; Is previousMode=USER?
        jz      Kt13_05                 ; if z, previousmode=SYSTEM

;
;       eflags.vm=0 (not v86 mode)
;       previousMode=USER
;
        cmp     word ptr [ebp]+TsSegCs,KGDT_R3_CODE OR RPL_MASK
        jne     Kt13_110                ; May still be a vdm...

;
;       eflags.vm=0 (not v86 mode)
;       previousMode=USER
;       Cs=00011011
;
; We are about to dispatch a XMMI floating point exception to user mode.
;
; (ebp) - Trap frame
; (ecx) - CurrentThreads NPX save area (PCR[PcInitialStack])
; (eax) - CurrentThread

; Dispatch
Kt13_20:
;
; Some type of coprocessor exception has occured for the current thread.
;
; Interrupts disabled
;
        mov     ebx, cr0
        and     ebx, NOT (CR0_MP+CR0_EM+CR0_TS)
        mov     cr0, ebx                ; Clear MP+TS+EM to do fxsave

;
; Save the faulting state so we can inspect the cause of the floating
; point fault
;
        FXSAVE_ECX

if DBG
        test    dword ptr [ecx].FpCr0NpxState, NOT (CR0_MP+CR0_EM+CR0_TS)
        jnz     Kt13_dbg2
endif

        or      ebx, NPX_STATE_NOT_LOADED ; CR0_TS | CR0_MP
        or      ebx,[ecx]+FpCr0NpxState ; restore this threads CR0 NPX state
        mov     cr0, ebx                ; set TS so next ESC access causes trap

;
; Clear TS bit in Cr0NpxFlags in case it was set to trigger this trap.
;
        and     dword ptr [ecx].FpCr0NpxState, NOT CR0_TS

;
; The state is no longer in the coprocessor.  Clear ThNpxState and
; re-enable interrupts to allow context switching.
;
        mov     byte ptr [eax].ThNpxState, NPX_STATE_NOT_LOADED
        mov     dword ptr PCR[PcPrcbData+PbNpxThread], 0  ; No state in coprocessor
        sti

; (eax) = ExcepCode - Exception code to put into exception record
; (ebx) = ExceptAddress - Addr of instruction which the hardware exception occurs
; (ecx) = NumParms - Number of additional parameters
; (edx) = Parameter1
; (esi) = Parameter2
; (edi) = Parameter3
        mov     ebx, [ebp].TsEip          ; Eip is from trap frame, not from FxErrorOffset
        movzx   eax, word ptr [ecx] + FxMXCsr
        mov     edx, eax
        shr     edx, 7                    ; get the mask
        not     edx
        mov     esi, 0                    ; (esi) = operand addr, addr is computed from
                                          ; trap frame, not from FxDataOffset
;
;       Exception will be handled in user's handler if there is one declared.
;
        and     eax, FSW_INVALID_OPERATION + FSW_DENORMAL + FSW_ZERO_DIVIDE + FSW_OVERFLOW + FSW_UNDERFLOW + FSW_PRECISION
        and     eax, edx
        test    eax, FSW_INVALID_OPERATION ; Is it an invalid op exception?
        jz      short Kt13_40              ; if z, no, go Kt13_40
;
; Invalid Operation Exception - Invalid arithmetic operand
; Raise exception
;
        mov     eax, STATUS_FLOAT_MULTIPLE_TRAPS
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt13_40:
; Check for floating zero divide exception
;
        test    eax, FSW_ZERO_DIVIDE    ; Is it a zero divide error?
        jz      short Kt13_50           ; if z, no, go Kt13_50
;
; Division-By-Zero Exception
; Raise exception
;
        mov     eax, STATUS_FLOAT_MULTIPLE_TRAPS
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt13_50:
; Check for denormal error
;
        test    eax, FSW_DENORMAL       ; Is it a denormal error?
        jz      short Kt13_60           ; if z, no, go Kt13_60
;
; Denormal Operand Excpetion
; Raise exception
;
        mov     eax, STATUS_FLOAT_MULTIPLE_TRAPS
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt13_60:
; Check for floating overflow error
;
        test    eax, FSW_OVERFLOW       ; Is it an overflow error?
        jz      short Kt13_70           ; if z, no, go Kt13_70
;
; Numeric Overflow Exception
; Raise exception
;
        mov     eax, STATUS_FLOAT_MULTIPLE_FAULTS
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt13_70:
; Check for floating underflow error
;
        test    eax, FSW_UNDERFLOW      ; Is it a underflow error?
        jz      short Kt13_80           ; if z, no, go Kt13_80
;
; Numeric Underflow Exception
; Raise exception
;
        mov     eax, STATUS_FLOAT_MULTIPLE_FAULTS
        jmp     CommonDispatchException1Arg0d ; Won't return

Kt13_80:
; Check for precision (IEEE inexact) error
;
        test    eax, FSW_PRECISION      ; Is it a precision error
        jz      short Kt13_100          ; if z, no, go Kt13_100
;
; Inexact-Result (Precision) Exception
; Raise exception
;
        mov     eax, STATUS_FLOAT_MULTIPLE_FAULTS
        jmp     CommonDispatchException1Arg0d ; Won't return

; Huh?
Kt13_100:
; If status word does not indicate error, then something is wrong...
; (Note: that we have done a sti, before the status is examined)
        sti
; stop the system
        stdCall _KeBugCheckEx,<TRAP_CAUSE_UNKNOWN,13,eax,0,1>

;
;       eflags.vm=0 (not v86 mode)
;       previousMode=USER
;       Cs=!00011011
;       (We should have (eax) -> CurrentThread)
;
Kt13_110:
; Check to see if this process is a vdm
        mov     ebx,PCR[PcPrcbData+PbCurrentThread] ; (ebx) -> CurrentThread
        mov     ebx,[ebx]+ThApcState+AsProcess      ; (ebx) -> CurrentProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh        ; is this a vdm process?
        jz      Kt13_20                             ; no, dispatch exception
                                                    ; yes, drop down to v86 mode
;
;       eflags.vm=1 (v86 mode)
;
Kt13_130:
; Turn off TS
        mov     ebx,CR0
        and     ebx,NOT CR0_TS
        mov     CR0,ebx
        and     dword ptr [ecx]+FpCr0NpxState,NOT CR0_TS

;
; Reflect the exception to the vdm, the VdmHandler enables interrupts
;

        ; Raise Irql to APC level before enabling interrupts
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql
        push    eax                     ; Save OldIrql
        sti

        stdCall   _VdmDispatchIRQ13, <ebp> ; ebp - Trapframe
        test    al,0fh
        jnz     Kt13_135
        pop     ecx                     ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     Kt13_20                 ; could not reflect, gen exception

Kt13_135:
        pop     ecx                     ; (TOS) = OldIrql
        fstCall KfLowerIrql
        jmp     _KiExceptionExit

;
;       eflags.vm=0 (not v86 mode)
;       previousMode=SYSTEM
;
Kt13_05:
        ; stop the system
        stdCall _KeBugCheckEx,<TRAP_CAUSE_UNKNOWN,13,0,0,2>

if DBG
Kt13_dbg1:    int 3
Kt13_dbg2:    int 3
Kt13_dbg3:    int 3
        sti
        jmp short $-2
endif

_KiTrap13       endp


        page ,132
        subttl "Coprocessor Error Handler"
;++
;
; Routine Description:
;
;    When the 80387 detects an error, it raises its error line.  This
;    was supposed to be routed directly to the 386 to cause a trap 16
;    (which would actually occur when the 386 encountered the next FP
;    instruction).
;
;    However, the ISA design routes the error line to IRQ13 on the
;    slave 8259.  So an interrupt will be generated whenever the 387
;    discovers an error.  Unfortunately, we could be executing system
;    code at the time, in which case we can't dispatch the exception.
;
;    So we force emulation of the intended behaviour.  This interrupt
;    handler merely sets TS and Cr0NpxState TS and dismisses the interrupt.
;    Then, on the next user FP instruction, a trap 07 will be generated, and
;    the exception can be dispatched then.
;
;    Note that we don't have to clear the FP exeception here,
;    since that will be done in the trap 07 handler.  The 386 will
;    generate the trap 07 before the 387 gets a chance to raise another
;    error interrupt.  We'll want to save the 387 state in the trap 07
;    handler WITH the error information.
;
;    Note the caller must clear the 387 error latch.  (this is done in
;    the hal).
;
; Arguments:
;
;    None
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:FLAT, SS:NOTHING, ES:NOTHING

align dword
cPublicProc _KiCoprocessorError     ,0

;
; Set TS in Cr0NpxState - the next time this thread runs an ESC
; instruction the error will be dispatched.  We also need to set TS
; in CR0 in case the owner of the NPX is currently running.
;
; Bit must be set in FpCr0NpxState before CR0.
;
        mov     eax, PCR[PcPrcbData+PbNpxThread]
        mov     eax, [eax].ThInitialStack
        sub     eax, NPX_FRAME_LENGTH   ; Space for NPX_FRAME
        or      dword ptr [eax].FpCr0NpxState, CR0_TS

        mov     eax, cr0
        or      eax, CR0_TS
        mov     cr0, eax
        stdRET    _KiCoprocessorError

stdENDP _KiCoprocessorError

;
; BBT cannot instrument code between BBT_Exclude_Trap_Code_Begin and this label
;
        public  _BBT_Exclude_Trap_Code_End
_BBT_Exclude_Trap_Code_End  equ     $
        int 3

;++
;
; VOID
; KiFlushNPXState (
;     PFLOATING_SAVE_AREA SaveArea
;     )
;
; Routine Description:
;
;   When a threads NPX context is requested (most likely by a debugger)
;   this function is called to flush the threads NPX context out of the
;   compressor if required.
;
; Arguments:
;
;    Pointer to a location where this function must do fnsave for the
;    current thread.
;
;    NOTE that this pointer can be NON-NULL only if KeI386FxsrPresent is
;         set (FXSR feature is present)
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:FLAT, SS:NOTHING, ES:NOTHING
align dword

SaveArea    equ     [esp + 20]

cPublicProc _KiFlushNPXState    ,1
cPublicFpo 1, 4

        push    esi
        push    edi
        push    ebx
        pushfd
        cli                             ; don't context switch

        mov     edi, PCR[PcSelfPcr]
        mov     esi, [edi].PcPrcbData+PbCurrentThread

        cmp     byte ptr [esi].ThNpxState, NPX_STATE_LOADED
        je      short fnpx20

fnpx00:
    ; NPX state is not loaded. If SaveArea is non-null, we need to return
    ; the saved FP state in fnsave format.

        cmp     dword ptr SaveArea, 0
        je      fnpx70

if DBG
    ;
    ; SaveArea can be NON-NULL ONLY when FXSR feature is present
    ;

        test    byte ptr _KeI386FxsrPresent, 1
        jnz     @f
        int     3
@@:
endif
    ;
    ; Need to convert the (not loaded) NPX state of the current thread
    ; to FNSAVE format into the SaveArea
    ;
        mov     ebx, cr0
        test    ebx, CR0_MP+CR0_TS+CR0_EM
        jz      short fnpx07
        and     ebx, NOT (CR0_MP+CR0_TS+CR0_EM)
        mov     cr0, ebx                ; allow frstor (& fnsave) to work
fnpx07:
    ;
    ; If NPX state is for some other thread, save it away
    ;

        mov     eax, [edi].PcPrcbData+PbNpxThread   ; Owner of NPX state
        or      eax, eax
        jz      short fnpx10            ; no - skip save

        cmp     byte ptr [eax].ThNpxState, NPX_STATE_LOADED
        jne     short fnpx10            ; not loaded, skip save

ifndef NT_UP
if DBG
    ; This can only happen UP where the context is not unloaded on a swap
        int 3
endif
endif

    ;
    ; Save current owners NPX state
    ;

        mov     ecx, [eax].ThInitialStack
        sub     ecx, NPX_FRAME_LENGTH   ; Space for NPX_FRAME
        FXSAVE_ECX
        mov     byte ptr [eax].ThNpxState, NPX_STATE_NOT_LOADED

fnpx10:
    ;
    ; Load current threads NPX state
    ;

        mov     ecx, [edi].PcInitialStack ; (ecx) -> top of kernel stack
        FXRSTOR_ECX                       ; reload NPX context
        mov     ecx, SaveArea
        jmp     short fnpx40

fnpx20:
    ;
    ; Current thread has NPX state in the coprocessor, flush it
    ;
        mov     ebx, cr0
        test    ebx, CR0_MP+CR0_TS+CR0_EM
        jz      short fnpx30
        and     ebx, NOT (CR0_MP+CR0_TS+CR0_EM)
        mov     cr0, ebx                   ; allow frstor (& fnsave) to work
fnpx30:
        mov     ecx, [edi].PcInitialStack  ; (ecx) -> top of kernel stack

        test    byte ptr _KeI386FxsrPresent, 1
        jz      short fnpx40
        FXSAVE_ECX

        ; Do fnsave to SaveArea if it is non-null
        mov     ecx,  SaveArea
        jecxz   short fnpx50

fnpx40:
        fnsave  [ecx]                   ; NPX state to save area
        fwait                           ; Make sure data is in save area
fnpx50:
        xor     eax, eax
        mov     ecx, [edi+PcInitialStack]  ; (ecx) -> top of kernel stack
        mov     byte ptr [esi].ThNpxState, NPX_STATE_NOT_LOADED
        mov     [edi].PcPrcbData+PbNpxThread, eax  ; clear npx owner
;;      mov     [ecx].FpNpxSavedCpu, eax        ; clear last npx processor

        or      ebx, NPX_STATE_NOT_LOADED       ; or in new threads cr0
        or      ebx, [ecx]+FpCr0NpxState        ; merge new thread setable state
        mov     cr0, ebx

fnpx70:
        popfd                           ; enable interrupts
        pop     ebx
        pop     edi
        pop     esi
        stdRET    _KiFlushNPXState

stdENDP _KiFlushNPXState

;++
;
; VOID
; KiSetHardwareTrigger (
;     VOID
;     )
;
; Routine Description:
;
;   This function sets KiHardwareTrigger such that an analyzer can sniff
;   for this access.   It needs to occur with a lock cycle such that
;   the processor won't speculatively read this value.   Interlocked
;   functions can't be used as in a UP build they do not use a
;   lock prefix.
;
; Arguments:
;
;    None
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:FLAT, SS:NOTHING, ES:NOTHING
cPublicProc _KiSetHardwareTrigger,0
   lock inc     ds:_KiHardwareTrigger   ; trip hardware analyzer
        stdRet  _KiSetHardwareTrigger
stdENDP _KiSetHardwareTrigger


        page ,132
        subttl "Processing System Fatal Exceptions"
;++
;
; Routine Description:
;
;    This routine processes the system fatal exceptions.
;    The machine state and trap type will be displayed and
;    System will be stopped.
;
; Arguments:
;
;    (eax) = Trap type
;    (ebp) -> machine state frame
;
; Return value:
;
;    system stopped.
;
;--
        assume  ds:nothing, es:nothing, ss:nothing, fs:nothing, gs:nothing

align dword
        public  _KiSystemFatalException
_KiSystemFatalException proc
.FPO (0, 0, 0, 0, 0, FPO_TRAPFRAME)

        stdCall _KeBugCheckEx,<UNEXPECTED_KERNEL_MODE_TRAP, eax, 0, 0, 0>
        ret

_KiSystemFatalException endp

        page
        subttl  "Continue Execution System Service"
;++
;
; NTSTATUS
; NtContinue (
;    IN PCONTEXT ContextRecord,
;    IN BOOLEAN TestAlert
;    )
;
; Routine Description:
;
;    This routine is called as a system service to continue execution after
;    an exception has occurred. Its function is to transfer information from
;    the specified context record into the trap frame that was built when the
;    system service was executed, and then exit the system as if an exception
;    had occurred.
;
;   WARNING - Do not call this routine directly, always call it as
;             ZwContinue!!!  This is required because it needs the
;             trapframe built by KiSystemService.
;
; Arguments:
;
;    KTrapFrame (ebp+0: after setup) -> base of KTrapFrame
;
;    ContextRecord (ebp+8: after setup) = Supplies a pointer to a context rec.
;
;    TestAlert (esp+12: after setup) = Supplies a boolean value that specifies
;       whether alert should be tested for the previous processor mode.
;
; Return Value:
;
;    Normally there is no return from this routine. However, if the specified
;    context record is misaligned or is not accessible, then the appropriate
;    status code is returned.
;
;--

NcTrapFrame             equ     [ebp + 0]
NcContextRecord         equ     [ebp + 8]
NcTestAlert             equ     [ebp + 12]

align dword
cPublicProc _NtContinue     ,2

        push    ebp

;
; Restore old trap frame address since this service exits directly rather
; than returning.
;

        mov     ebx, PCR[PcPrcbData+PbCurrentThread] ; get current thread address
        mov     edx, [ebp].TsEdx        ; restore old trap frame address
        mov     [ebx].ThTrapFrame, edx  ;

;
; Call KiContinue to load ContextRecord into TrapFrame.  On x86 TrapFrame
; is an atomic entity, so we don't need to allocate any other space here.
;
; KiContinue(NcContextRecord, 0, NcTrapFrame)
;

        mov     ebp,esp
        mov     eax, NcTrapFrame
        mov     ecx, NcContextRecord
        stdCall  _KiContinue, <ecx, 0, eax>
        or      eax,eax                 ; return value 0?
        jnz     short Nc20              ; KiContinue failed, go report error

;
; Check to determine if alert should be tested for the previous processor mode.
;

        cmp     byte ptr NcTestAlert,0  ; Check test alert flag
        je      short Nc10              ; if z, don't test alert, go Nc10
        mov     al,byte ptr [ebx]+ThPreviousMode ; No need to xor eax, eax.
        stdCall _KeTestAlertThread, <eax> ; test alert for current thread

Nc10:   pop     ebp                     ; (ebp) -> TrapFrame
        mov     esp,ebp                 ; (esp) = (ebp) -> trapframe
        jmp     _KiServiceExit2         ; common exit

Nc20:   pop     ebp                     ; (ebp) -> TrapFrame
        mov     esp,ebp                 ; (esp) = (ebp) -> trapframe
        jmp     _KiServiceExit          ; common exit

stdENDP _NtContinue

        page
        subttl  "Raise Exception System Service"
;++
;
; NTSTATUS
; NtRaiseException (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PCONTEXT ContextRecord,
;    IN BOOLEAN FirstChance
;    )
;
; Routine Description:
;
;    This routine is called as a system service to raise an exception. Its
;    function is to transfer information from the specified context record
;    into the trap frame that was built when the system service was executed.
;    The exception may be raised as a first or second chance exception.
;
;   WARNING - Do not call this routine directly, always call it as
;             ZwRaiseException!!!  This is required because it needs the
;             trapframe built by KiSystemService.
;
;   NOTE - KiSystemService will terminate the ExceptionList, which is
;          not what we want for this case, so we will fish it out of
;          the trap frame and restore it.
;
; Arguments:
;
;    TrapFrame (ebp+0: before setup) -> System trap frame for this call
;
;    ExceptionRecord (ebp+8: after setup) -> An exception record.
;
;    ContextRecord (ebp+12: after setup) -> Points to a context record.
;
;    FirstChance (epb+16: after setup) -> Supplies a boolean value that
;       specifies whether the exception is to be raised as a first (TRUE)
;       or second chance (FALSE) exception.
;
; Return Value:
;
;    None.
;--
align dword
cPublicProc _NtRaiseException ,3

        push    ebp

;
; Restore old trap frame address since this service exits directly rather
; than returning.
;

        mov     ebx, PCR[PcPrcbData+PbCurrentThread] ; get current thread address
        mov     edx, [ebp].TsEdx        ; restore old trap frame address
        mov     [ebx].ThTrapFrame, edx  ;

;
;   Put back the ExceptionList so the exception can be properly
;   dispatched.
;

        mov     ebp,esp                 ; [ebp+0] -> TrapFrame
        mov     ebx, [ebp+0]            ; (ebx)->TrapFrame
        mov     edx, [ebp+16]           ; (edx) = First chance indicator
        mov     eax, [ebx]+TsExceptionList ; Old exception list
        mov     ecx, [ebp+12]           ; (ecx)->ContextRecord
        mov     PCR[PcExceptionList],eax
        mov     eax, [ebp+8]            ; (eax)->ExceptionRecord

;
;   KiRaiseException(ExceptionRecord, ContextRecord, ExceptionFrame,
;           TrapFrame, FirstChance)
;

        stdCall   _KiRaiseException,<eax, ecx, 0, ebx, edx>

        pop     ebp
        mov     esp,ebp                 ; (esp) = (ebp) -> trap frame

;
;   If the exception was handled, then the trap frame has been edited to
;   reflect new state, and we'll simply exit the system service to get
;   the effect of a continue.
;
;   If the exception was not handled, we'll return to our caller, who
;   will raise a new exception.
;
        jmp     _KiServiceExit2

stdENDP _NtRaiseException



        page
        subttl "Reflect Exception to a Vdm"
;++
;
;   Routine Description:
;       Local stub which reflects an exception to a VDM using
;       Ki386VdmReflectException,
;
;   Arguments:
;
;       ebp -> Trap frame
;       ss:esp + 4 = trap number
;
;   Returns
;       ret value from Ki386VdmReflectException
;       interrupts are disabled uppon return
;
cPublicProc _Ki386VdmReflectException_A, 1

        sub     esp, 4*2

        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql

        sti
        mov     [esp+4], eax                ; Save OldIrql
        mov     eax, [esp+12]               ; Pick up trap number
        mov     [esp+0], eax

        call    _Ki386VdmReflectException@4 ; pops one dword

        mov     ecx, [esp+0]                ; (ecx) = OldIrql
        mov     [esp+0], eax                ; Save return code

        fstCall KfLowerIrql

        pop     eax                         ; pops second dword
        ret     4

stdENDP _Ki386VdmReflectException_A


_TEXT$00   ends
        end
