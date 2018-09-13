        TITLE   "Call Bios support"
;++
;
;  Copyright (c) 1989  Microsoft Corporation
;
;  Module Name:
;
;     spinlock.asm
;
;  Abstract:
;
;     This module implements the support routines for executing int bios
;     call in v86 mode.
;
;  Author:
;
;     Shie-Lint Tzong (shielint) Sept 10, 1992
;
;  Environment:
;
;     Kernel mode only.
;
;  Revision History:
;
;--

.386p

include ks386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc

VdmStartExecution       EQU     0
V86_STACK_POINTER       equ     11ffeh  ; see BIOSC.C

        EXTRNP  _NtVdmControl,2
        extrn   _KiExceptionExit:PROC

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        PAGE
        SUBTTL "Switch to V86 mode"
;++
;
;  VOID
;  Ki386SetupAndExitToV86Code (
;     VOID
;     )
;
;  Routine Description:
;
;     This function sets up return trap frame, switch stack and
;     calls VdmStartExecution routine to put vdm context to
;     base trap frame and causes the system to execute in v86 mode by
;     doing a KiExceptionExit.
;
;  Arguments:
;
;     BiosArguments - Supplies a pointer to a structure which contains
;                     the arguments for v86 int function.
;
;  Return Value:
;
;     None.
;
;--

cPublicProc _Ki386SetupAndExitToV86Code,1

NewTEB                  equ     [ecx+32] ; location of the parameter based on
                                         ; the ecx stack pointer.
KsaeInitialStack        equ     [ecx]
OriginalThTeb           equ     [ecx+4]
OriginalPcTeb           equ     [ecx+8]

;
; Allocate TRAP FRAME at the bottom of the stack.
;

        push    ebp
        push    ebx
        push    esi
        push    edi
        sub     esp, 12                 ; 12 bytes for local variable
        mov     ecx, esp                ; (ecx) = saved esp

        sub     esp, NPX_FRAME_LENGTH
        and     esp, 0fffffff0h         ; FXSAVE 16 byte alignment requirement
        sub     esp, KTRAP_FRAME_LENGTH ; (esp)-> new trap frame
        mov     eax, esp                ; (eax)->New base trap frame

;
; Initialize newly allocated trap frame to caller's nonvolatle context.
; Note that it is very important that the trap frame we are going to create
; is a USER mode frame.  The system expects the top trap frame for user
; mode thread is a user mode frame.  (Get/SetContext enforce the rule.)
;
; (eax)-> Base of trap frame.
;

        mov     dword ptr [eax].TsSegCs, KGDT_R0_CODE OR RPL_MASK
                                        ; an invalid cs to trap it back to kernel
        mov     dword ptr [eax].TsSegEs, 0
        mov     dword ptr [eax].TsSegDs, 0
        mov     dword ptr [eax].TsSegFs, 0
        mov     dword ptr [eax].TsSegGs, 0
        mov     dword ptr [eax].TsErrCode, 0
        mov     ebx, fs:PcSelfPcr       ; (ebx)->Pcr
        mov     edx, [ebx].PcInitialStack
        mov     KsaeInitialStack, edx  ; (edx)->Pcr InitialSack

        mov     edi, [ebx]+PcPrcbData+PbCurrentThread ; (edi)->CurrentThread
        mov     edx, [edi].ThTeb
        mov     OriginalThTeb, edx

        mov     edx, fs:[PcTeb]
        mov     OriginalPcTeb, edx

        mov     edi, offset Ki386BiosCallReturnAddress
        mov     [eax].TsEsi, ecx       ; Saved esp
        mov     [eax].TsEip, edi       ; set up return address
        pushfd
        pop     edi
        and     edi, 60dd7h
        or      edi, 200h              ; sanitize EFLAGS
        mov     dword ptr [eax].TsHardwareSegSs, KGDT_R3_DATA OR RPL_MASK
        mov     dword ptr [eax].TsHardwareEsp, V86_STACK_POINTER
        mov     [eax].TsEflags, edi
        mov     [eax].TsExceptionList, EXCEPTION_CHAIN_END
        mov     [eax].TsPreviousPreviousMode, 0ffffffffh ; No previous mode
if DBG
        mov     [eax].TsDbgArgMark, 0BADB0D00h ; set trap frame mark
endif

        add     eax, KTRAP_FRAME_LENGTH

;
; Disable interrupt and change the stack pointer to make the new
; trap frame be the current thread's base trap frame.
;
; (eax)->Npx save area
;

        cli

;
; Set up various stack pointers
;
;  Low  |           |
;       |-----------| <- New esp
;       |  New Base |
;       |Trap Frame |
;       |-----------| <- Tss.Esp0
;       |V86 segs   |
;       |-----------| <- Pcr.InitialStack
;       |Npx Area   |
;       |-----------| <- Old Esp =  Thread.InitialStack
;       |           |
;  High |           |
;
;
; Copy the FP state to the new FP state save area (NPX frame)
;

        push    ecx     ; save ecx (saved esp)
        mov     esi, [ebx].PcInitialStack
        mov     ecx, NPX_FRAME_LENGTH/4
        mov     edi, eax
        rep movsd
        pop     ecx     ; restore ecx

        mov     edi, [ebx]+PcPrcbData+PbCurrentThread ; (edi)->CurrentThread
        mov     [ebx].PcInitialStack, eax
        mov     esi,[ebx]+PcTss         ; (esi)->TSS
        sub     eax,TsV86Gs - TsHardwareSegSs ; bias for missing fields
        mov     [ebx].PcExceptionList, EXCEPTION_CHAIN_END
        mov     [esi]+TssEsp0,eax
        add     eax, NPX_FRAME_LENGTH + (TsV86Gs - TsHardwareSegSs)
        mov     [edi].ThInitialStack, eax

;
; Set up the pointers to the fake TEB so we can execute the int10
; call
;
        mov     eax, NewTeb
        mov     fs:[PcTeb], eax
        mov     [edi].ThTeb, eax

        mov     ebx, PCR[PcGdt]
        mov     [ebx]+(KGDT_R3_TEB+KgdtBaseLow), ax
        shr     eax, 16
        mov     [ebx]+(KGDT_R3_TEB+KgdtBaseMid), al
        mov     [ebx]+(KGDT_R3_TEB+KgdtBaseHi), ah

        sti

; Now call VdmControl to save return 32bit frame and put vdm context
; to new base trap frame

        stdCall _NtVdmControl, <VdmStartExecution, 0>

if 0
;
; Now call _VdmpStartExecution to save return 32bit frame and put vdm context
; to new base trap frame
;

        mov     eax, ExecAddr
        stdCall _VdmpStartExecution, <eax>
endif

;
; Call KiexceptionExit to 'exit' to v86 code.
;

        mov     ebp, esp                ; (ebp)->Exit trap frame
        jmp     _KiExceptionExit        ; go execute int 10

        public  Ki386BiosCallReturnAddress
Ki386BiosCallReturnAddress:

;
; After ROM BIOS int completes, the bop instruction gets executed.
; This results in a trap to kernel mode bop handler where the
; 16 bit Vdm context will be saved to VdmTib->VdmCOntext, and
; the faked 32 bit user mode context (i.e. the one we created earlier)
; be restored.  Since the faked user mode context does NOT have a valid
; iret address, the 'iret' instruction of the EXIT_ALL will be trapped to
; our GP fault handler which recognizes this and transfers control back to
; here.
;
; when we come back here, all the segment registers are set up properly
; and esp is restored.  Interrupts are disabled.
;

;
; restore all the pointers.
;

        mov     eax, fs:PcSelfPcr       ; (eax)->Pcr
        pop     edi                     ; (edi) = Pcr InitialStack

;
; Copy the FP state back down to the default stack
;

        mov     esi, [eax].PcInitialStack
        mov     ecx, NPX_FRAME_LENGTH/4
        mov     [eax].PcInitialStack, edi ; Restore Pcr InitialStack
        rep movsd                       ; copy FP state
                                        ; (n.b. edi+= NPX_FRAME_LENGTH)

        mov     ecx, [eax]+PcPrcbData+PbCurrentThread ; (ecx)->CurrentThread
        mov     [ecx].ThInitialStack, edi ; Restore Thread.InitialStack

        mov     eax,[eax]+PcTss         ; (eax)->TSS
        sub     edi, (TsV86Gs - TsHardwareSegSs) + NPX_FRAME_LENGTH
        mov     [eax]+TssEsp0,edi

;
; restore pointers to the original TEB
;
        pop     edx                     ; (edx) = OriginalThTeb
        mov     [ecx].ThTeb, edx
        pop     edx                     ; (edx) = OriginalPcTeb
        mov     fs:[PcTeb], edx

        mov     ebx, PCR[PcGdt]
        mov     [ebx]+(KGDT_R3_TEB+KgdtBaseLow), dx
        shr     edx, 16
        mov     [ebx]+(KGDT_R3_TEB+KgdtBaseMid), dl
        mov     [ebx]+(KGDT_R3_TEB+KgdtBaseHi), dh


        sti

        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        stdRET  _Ki386SetupAndExitToV86Code

stdENDP _Ki386SetupAndExitToV86Code

_TEXT   ends

        end
