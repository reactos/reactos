        title  "Processor State Save Restore"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    procstat.asm
;
; Abstract:
;
;    This module implements procedures for saving and restoring
;    processor control state, and processor run&control state.
;    These procedures support debugging of UP and MP systems.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 30-Aug-1990
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
include callconv.inc
        .list

        EXTRNP  _KeContextToKframes,5
        EXTRNP  _KeContextFromKframes,3
        extrn   _KeFeatureBits:DWORD

        page ,132
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        subttl  "Save Processor State"
;++
;
; KiSaveProcessorState(
;       PKTRAP_FRAME TrapFrame,
;       PKEXCEPTION_FRAME   ExceptionFrame
;       );
;
; Routine Description:
;
;    This routine saves the processor state for debugger.  When the current
;    processor receives the request of IPI_FREEZE, it saves all the registers
;    in a save area in the PRCB so the debugger can get access to them.
;
; Arguments:
;
;    TrapFrame (esp+4) - Pointer to machine trap frame
;
;    ExceptionFrame (esp+8) - Pointer to exception frame
;           (IGNORED on the x86!)
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiSaveProcessorState   ,2

        mov     eax, [esp+4]                    ; (eax) -> TrapFrame

        mov     edx, PCR[PcPrcb]                ; (edx)->PrcbData
        add     edx, PbProcessorState           ; (edx)->ProcessorState
        push    edx
;
; Copy the whole TrapFrame to our ProcessorState
;

        lea     ecx, [edx].PsContextFrame
        mov     dword ptr [ecx].CsContextFlags, CONTEXT_FULL OR CONTEXT_DEBUG_REGISTERS

; ecx - ContextFrame
; 0 - ExceptionFrame == NULL
; eax - TrapFrame
        stdCall   _KeContextFromKframes, <eax, 0, ecx>

;
; Save special registers for debugger
;

        ; TOS = PKPROCESSOR_STATE
        call    _KiSaveProcessorControlState@4

        stdRET  _KiSaveProcessorState

stdENDP _KiSaveProcessorState


        page    ,132
        subttl  "Save Processor Control State"
;++
;
; KiSaveProcessorControlState(
;       PKPROCESSOR_STATE   ProcessorState
;       );
;
; Routine Description:
;
;    This routine saves the control subset of the processor state.
;    (Saves the same information as KiSaveProcessorState EXCEPT that
;     data in TrapFrame/ExceptionFrame=Context record is NOT saved.)
;    Called by the debug subsystem, and KiSaveProcessorState()
;
;   N.B.  This procedure will save Dr7, and then 0 it.  This prevents
;         recursive hardware trace breakpoints and allows debuggers
;         to work.
;
; Arguments:
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiSaveProcessorControlState   ,1

        mov     edx, [esp+4]                    ; ProcessorState

;
; Save special registers for debugger
;
        xor     ecx,ecx

        mov     eax, cr0
        mov     [edx].PsSpecialRegisters.SrCr0, eax
        mov     eax, cr2
        mov     [edx].PsSpecialRegisters.SrCr2, eax
        mov     eax, cr3
        mov     [edx].PsSpecialRegisters.SrCr3, eax

        mov     [edx].PsSpecialRegisters.SrCr4, ecx

        test    _KeFeatureBits, KF_CR4
        jz      short @f

.586p
        mov     eax, cr4
        mov     [edx].PsSpecialRegisters.SrCr4, eax
.486p

@@:
        mov     eax,dr0
        mov     [edx].PsSpecialRegisters.SrKernelDr0,eax
        mov     eax,dr1
        mov     [edx].PsSpecialRegisters.SrKernelDr1,eax
        mov     eax,dr2
        mov     [edx].PsSpecialRegisters.SrKernelDr2,eax
        mov     eax,dr3
        mov     [edx].PsSpecialRegisters.SrKernelDr3,eax
        mov     eax,dr6
        mov     [edx].PsSpecialRegisters.SrKernelDr6,eax

        mov     eax,dr7
        mov     dr7,ecx
        mov     [edx].PsSpecialRegisters.SrKernelDr7,eax

        sgdt    fword ptr [edx].PsSpecialRegisters.SrGdtr
        sidt    fword ptr [edx].PsSpecialRegisters.SrIdtr

        str     word ptr [edx].PsSpecialRegisters.SrTr
        sldt    word ptr [edx].PsSpecialRegisters.SrLdtr

        stdRET    _KiSaveProcessorControlState

stdENDP _KiSaveProcessorControlState

        page ,132
        subttl  "Restore Processor State"
;++
;
; KiRestoreProcessorState(
;       PKTRAP_FRAME TrapFrame,
;       PKEXCEPTION_FRAME ExceptionFrame
;       );
;
; Routine Description:
;
;    This routine Restores the processor state for debugger.  When the
;    control returns from debugger (UnFreezeExecution), this function
;    restores the entire processor state.
;
; Arguments:
;
;    TrapFrame (esp+4) - Pointer to machine trap frame
;
;    ExceptionFrame (esp+8) - Pointer to exception frame
;           (IGNORED on the x86!)
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiRestoreProcessorState   ,2

        mov     eax, [esp+4]                    ; (eax) -> TrapFrame

        mov     edx, PCR[PcPrcb]                ; (edx)->PrcbData
        add     edx, PbProcessorState           ; (edx)->ProcessorState
        push    edx

;
; Copy the whole ContextFrame to TrapFrame
;

        lea     ecx, [edx].PsContextFrame
        mov     edx, [edx].PsContextFrame.CsSegCs
        and     edx, MODE_MASK

; edx - Previous mode
; ecx - ContextFrame
; 0 - ExceptionFrame == NULL
; eax - TrapFrame
        stdCall   _KeContextToKframes, <eax,0,ecx,[ecx].CsContextFlags,edx>

;
; Save special registers for debugger
;

        ; TOS = KPROCESSOR_STATE
        call    _KiRestoreProcessorControlState@4

        stdRET  _KiRestoreProcessorState

stdENDP _KiRestoreProcessorState


        page    ,132
        subttl  "Restore Processor Control State"
;++
;
; KiRestoreProcessorControlState(
;       );
;
; Routine Description:
;
;    This routine restores the control subset of the processor state.
;    (Restores the same information as KiRestoreProcessorState EXCEPT that
;     data in TrapFrame/ExceptionFrame=Context record is NOT restored.)
;    Called by the debug subsystem, and KiRestoreProcessorState()
;
; Arguments:
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiRestoreProcessorControlState,1

        mov     edx, [esp+4]                    ; (edx)->ProcessorState

;
; Restore special registers for debugger
;

        mov     eax, [edx].PsSpecialRegisters.SrCr0
        mov     cr0, eax
        mov     eax, [edx].PsSpecialRegisters.SrCr2
        mov     cr2, eax
        mov     eax, [edx].PsSpecialRegisters.SrCr3
        mov     cr3, eax

        test    _KeFeatureBits, KF_CR4
        jz      short @f

.586p
        mov     eax, [edx].PsSpecialRegisters.SrCr4
        mov     cr4, eax
.486p
@@:
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr0
        mov     dr0, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr1
        mov     dr1, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr2
        mov     dr2, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr3
        mov     dr3, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr6
        mov     dr6, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr7
        mov     dr7, eax

        lgdt    fword ptr [edx].PsSpecialRegisters.SrGdtr
        lidt    fword ptr [edx].PsSpecialRegisters.SrIdtr

;
; Force the TSS descriptor into a non-busy state, so we don't fault
; when we load the TR.
;

        mov     eax, [edx].PsSpecialRegisters.SrGdtr+2  ; (eax)->GDT base
        xor     ecx, ecx
        mov     cx,  word ptr [edx].PsSpecialRegisters.SrTr
        add     eax, 5
        add     eax, ecx                                ; (eax)->TSS Desc. Byte
        and     byte ptr [eax],NOT 2
        ltr     word ptr [edx].PsSpecialRegisters.SrTr

        lldt    word ptr [edx].PsSpecialRegisters.SrLdtr

        stdRET    _KiRestoreProcessorControlState

stdENDP _KiRestoreProcessorControlState

_TEXT   ENDS
        END
