        title  "Abios Support Assembly Routines"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    abiosa.asm
;
; Abstract:
;
;    This module implements assembley code for ABIOS support.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 25-May-1991
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
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
        .list

extrn   _DbgPrint:proc

EXTRNP _KeRaiseIrql,2,IMPORT
EXTRNP _KeLowerIrql,1,IMPORT
EXTRNP _KeGetCurrentIrql,0,IMPORT
extrn _KiStack16GdtEntry:DWORD

;
; This should be either 0 or 1, if it's greater than 1, then we've re-entered the BIOS.
;
extrn _KiInBiosCall:DWORD
extrn _FlagState:DWORD
extrn _KiBiosFrame:DWORD

OPERAND_OVERRIDE        equ     66h
ADDRESS_OVERRIDE        equ     67h
KGDT_CDA16              equ     0E8h

LocalStack                              equ     16          ; 4 DWORDS of slop for PnPBioses.

if DBG
extrn  KiBiosReenteredAssert:DWORD
endif

; Macro change note:
;
;   This macro pair used to do an uncondtional sti coming back from the 16-bit
;   side, this potentially caused problems in APM. Now we save and restore the
;   flag state
;

;++
;
;   STACK32_TO_STACK16
;
;   Macro Description:
;
;       This macro remaps current 32bit stack to 16bit stack.
;
;   Arguments:
;
;       None.
;
;--

STACK32_TO_STACK16      macro

        pushfd
        mov     ecx,[esp]
        mov     _FlagState,ecx
        popfd
        mov     eax, fs:PcStackLimit    ; [eax] = 16-bit stack selector base
        mov     edx, eax
        mov     ecx, _KiStack16GdtEntry
        mov     word ptr [ecx].KgdtBaseLow, ax
        shr     eax, 16
        mov     byte ptr [ecx].KgdtBaseMid, al
        mov     byte ptr [ecx].KgdtBaseHi, ah
        mov     eax, esp
        sub     eax, edx
        cli
        mov     esp, eax
        mov     eax, KGDT_STACK16
        mov     ss, ax

;
; NOTE that we MUST leave interrupts remain off.
; We'll turn it back on after we switch to 16 bit code.
;

endm

;++
;
;   STACK16_TO_STACK32
;
;   Macro Description:
;
;       This macro remaps current 32bit stack to 16bit stack.
;
;   Arguments:
;
;       None.
;
;--

STACK16_TO_STACK32      macro   Stack32

        db      OPERAND_OVERRIDE
        mov     eax, esp
        db      OPERAND_OVERRIDE
        db      ADDRESS_OVERRIDE
        add     eax, fs:PcStackLimit
        cli
        db      OPERAND_OVERRIDE
        mov     esp, eax
        db      OPERAND_OVERRIDE
        mov     eax, KGDT_R0_DATA
        mov     ss, ax
        db      OPERAND_OVERRIDE
        db      ADDRESS_OVERRIDE
        push ds:_FlagState
        db      OPERAND_OVERRIDE
        popfd
endm

COPY_CALL_FRAME macro FramePtr

        mov     [FramePtr].TsEax,eax
        mov     [FramePtr].TsEbx,ebx
        mov     [FramePtr].TsEcx,ecx
        mov     [FramePtr].TsEdx,edx
        mov     [FramePtr].TsEsi,esi
        mov     [FramePtr].TsEdi,edi
        mov     [FramePtr].TsEbp,ebp
        mov     [FramePtr].TsHardwareEsp,esp
        mov     [FramePtr].TsSegFs,fs
        mov     [FramePtr].TsSegCs,cs
endm
        page ,132
        subttl  "Abios Support Code"
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;
; BBT cannot instrument code between this label and BBT_Exclude_Selector_Code_End
;
        public  _BBT_Exclude_Selector_Code_Begin
_BBT_Exclude_Selector_Code_Begin  equ     $
        int 3


;++
; ULONG
; KiAbiosGetGdt (
;     VOID
;     )
;
; Routine Description:
;
;     This routine returns the starting address of GDT of current processor.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     return Pcr->GDT
;
;--

cPublicProc _KiAbiosGetGdt,0

        mov     eax, fs:PcGdt
        stdRET    _KiAbiosGetGdt

stdENDP _KiAbiosGetGdt

;++
; VOID
; KiI386CallAbios(
;     IN KABIOS_POINTER AbiosFunction,
;     IN KABIOS_POINTER DeviceBlockPointer,
;     IN KABIOS_POINTER FunctionTransferTable,
;     IN KABIOS_POINTER RequestBlock
;     )
;
; Routine Description:
;
;     This function invokes ABIOS service function for device driver.  This
;     routine is executing at DIAPTCH_LEVEL to prevent context swapping.
;
;     N.B. We arrive here from the Ke386AbiosCall with a 32bit CS. That is,
;     we're executing the code with cs:eip where cs contains a selector for a
;     32bit flat segment. We want to get to a 16bit cs. That is, cs:ip.
;     The reason is that ABIOS is running at 16 bit segment.
;     Before we can call ABIOS service we must load ss and cs segment
;     registers with selectors for 16bit segments.  We start by pushing a far
;     pointer to a label in the macro and then doing a retf. This allows us
;     to fall through to the next instruction, but we're now executing
;     through cs:ip with a 16bit CS. Then, we remap our 32-bit stack to 16-bit
;     stack.
;
; Arguments:
;
;     AbiosFunction - a 16:16 pointer to the abios service function.
;
;     DeviceBlockPointer - a 16:16 pointer to Device Block.
;
;     FunctionTransferTable - a 16:16 pointer to Function Transfer Table.
;
;     RequestBlock - a 16:16 pointer to device driver's request block.
;
; Return Value:
;
;     None.
;--

KacAbiosFunction        equ     [ebp + 8]
KacDeviceBlock          equ     [ebp + 12]
KacFunctionTable        equ     [ebp + 16]
KacRequestBlock         equ     [ebp + 20]

cPublicProc _KiI386CallAbios,4

;
; We're using a 32bit CS:EIP - go to a 16bit CS:IP
; Note the base of KiAbiosCallSelector is the flat address of _KiI386AbiosCall
; routine.
;

        push    ebp
        mov     ebp, esp
        push    ebx

        COPY_CALL_FRAME _KiBiosFrame
        sub     esp,LocalStack          ; After C style frame
        stdCall _KeGetCurrentIrql
        push    eax                             ; Local Varible

        cmp     al, DISPATCH_LEVEL              ; Is irql > Dispatch_level?
        jae     short Kac00

; Raise to Dispatch Level
        mov     eax, esp
        stdCall   _KeRaiseIrql, <DISPATCH_LEVEL,eax>

Kac00:

;
; Set up parameters on stack before remapping stack.
;

        push    word ptr KGDT_CDA16             ; CDA anchor selector
        push    KacRequestBlock                 ; Request Block
        push    KacFunctionTable                ; Func transfer table
        push    KacDeviceBlock                  ; Device Block
        mov     ebx, KacAbiosFunction           ; (ebx)-> Abios Entry

;
; Remap current stack to 16:16 stack.  The base of the 16bit stack selector is
; the base of current kernel stack.
;

        inc     _KiInBiosCall                         ; Set the 'In Bios' flag
if DBG
        cmp   _KiInBiosCall,2
        jb  @F
        push    offset FLAT:KiBiosReenteredAssert
        call    _dbgPrint
        add     esp, 4
@@:
endif

        STACK32_TO_STACK16                      ; Switch to 16bit stack
        push    word ptr KGDT_CODE16
IFDEF STD_CALL
        push    word ptr (offset FLAT:Kac40 - offset FLAT:_KiI386CallAbios@16)
        push    KGDT_CODE16
        push    offset FLAT:Kac30 - offset FLAT:_KiI386CallAbios@16
ELSE
        push    word ptr (offset FLAT:Kac40 - offset FLAT:_KiI386CallAbios)
        push    KGDT_CODE16
        push    offset FLAT:Kac30 - offset FLAT:_KiI386CallAbios
ENDIF
        retf

Kac30:

;
; Stack switching (from 32 to 16) turns interrupt off.  We must turn it
; back on.
;

        sti
        push    bx                              ; Yes, BX not EBX!
        retf
Kac40:
        add     esp, 14                         ; pop out all the parameters

        STACK16_TO_STACK32                      ; switch back to 32 bit stack

;
; Pull callers flat return address off stack and push the
; flat code selector followed by the return offset, then
; execute a far return and we'll be back in the 32-bit code space.
;

        db      OPERAND_OVERRIDE
        push    KGDT_R0_CODE
        db      OPERAND_OVERRIDE
        push    offset FLAT:Kac50
        db      OPERAND_OVERRIDE
        retf
Kac50:
        pop     eax                             ; [eax] = OldIrql
        pop     ebx                             ; restore ebx
        cmp     al, DISPATCH_LEVEL
        jae     short Kac60

        stdCall   _KeLowerIrql, <eax>             ; Lower irql to original level
Kac60:

        dec     _KiInBiosCall                          ;Clear 'In Bios' Flag

        add     esp,LocalStack                           ; subtract off the scratch space
        pop     ebp
        stdRET    _KiI386CallAbios

stdENDP _KiI386CallAbios


;; ********************************************************
;;
;; BEGIN - power_management
;;
;;

;++
; VOID
; KeI386Call16BitFunction (
;     IN OUT PCONTEXT Regs
;     )
;
; Routine Description:
;
;     This function calls the 16 bit function specified in the Regs.
;
; Parameters:
;
;     Regs - supplies a pointer to register context to call 16 function.
;
;   NOTE: Caller must be at DPC_LEVEL
;
;--

cPublicProc _KeI386Call16BitFunction,1

    ;  verify CurrentIrql
    ;  verify context flags

        push    ebp                             ; save nonvolatile registers
        push    ebx
        push    esi
        push    edi
        mov     ebx, dword ptr [esp + 20]       ; (ebx)-> Context

        COPY_CALL_FRAME _KiBiosFrame

        sub     esp,LocalStack          ; After prolog

        inc    _KiInBiosCall                         ; Set the 'In Bios' flag
if DBG
        cmp   _KiInBiosCall,2
        jb  @F
        push    offset FLAT:KiBiosReenteredAssert
        call    _dbgPrint
        add     esp, 4
@@:
endif

;
; We're using a 32bit CS:EIP - go to a 16bit CS:IP
; Note the base of KiAbiosCallSelector is the flat address of _KiI386AbiosCall
; routine.
;

;
; Remap current stack to 16:16 stack.  The base of the 16bit stack selector is
; the base of current kernel stack.
;

        STACK32_TO_STACK16                      ; Switch to 16bit stack
    ;
    ; Push return address from 16 bit function call to kernel
    ;

        push    word ptr KGDT_CODE16
        push    word ptr (offset FLAT:Kbf40 - offset FLAT:_KiI386CallAbios@16)

        ;
        ; Load context to call with
        ;

        push    word ptr [ebx].CsEFlags
        push    word ptr [ebx].CsSegCs
        push    word ptr [ebx].CsEip

        mov     eax, [ebx].CsEax
        mov     ecx, [ebx].CsEcx
        mov     edx, [ebx].CsEdx
        mov     edi, [ebx].CsEdi
        mov     esi, [ebx].CsEsi
        mov     ebp, [ebx].CsEbp
        push    [ebx].CsSegGs
        push    [ebx].CsSegFs
        push    [ebx].CsSegEs
        push    [ebx].CsSegDs
        mov     ebx, [ebx].CsEbx
        pop     ds
        pop     es
        pop     fs
        pop     gs

    ;
    ; Switch to 16bit CS
    ;
        push    KGDT_CODE16
        push    offset FLAT:Kbf30 - offset FLAT:_KiI386CallAbios@16
        retf

Kbf30:
    ;
    ; "call" to 16 bit function
    ;
        iretd

Kbf40:
    ;
    ; Push some of the returned context which will be needed to
    ; switch back to the 32 bit SS & CS.
    ;
        db      OPERAND_OVERRIDE
        push    ds

        db      OPERAND_OVERRIDE
        push    es

        db      OPERAND_OVERRIDE
        push    fs

        db      OPERAND_OVERRIDE
        push    gs

        db      OPERAND_OVERRIDE
        push    eax

        db      OPERAND_OVERRIDE
        pushfd

        db      OPERAND_OVERRIDE
        mov     eax, KGDT_R0_PCR
        mov     fs, ax

        db      OPERAND_OVERRIDE
        mov     eax, KGDT_R3_DATA OR RPL_MASK
        mov     ds, ax
        mov     es, ax

        xor     eax, eax

    ;
    ; Switch back to 32 bit stack
    ;

        STACK16_TO_STACK32

;
; Push the flat code selector followed by the return offset, then
; execute a far return and we'll be back in the 32-bit code space.
;


        db      OPERAND_OVERRIDE
        push    KGDT_R0_CODE
        db      OPERAND_OVERRIDE
        push    offset FLAT:Kbf50
        db      OPERAND_OVERRIDE
        retf

Kbf50:
    ;
    ; Return resulting context
    ;

        mov     eax, dword ptr [esp+44+LocalStack]     ; (eax) = Context Record
        pop     [eax].CsEflags
        pop     [eax].CsEax
        pop     [eax].CsSegGs
        pop     [eax].CsSegFs
        pop     [eax].CsSegEs
        pop     [eax].CsSegDs

        mov     [eax].CsEbx, ebx
        mov     [eax].CsEcx, ecx
        mov     [eax].CsEdx, edx
        mov     [eax].CsEdi, edi
        mov     [eax].CsEsi, esi
        mov     [eax].CsEbp, ebp

;
; Restore regs & return
;
        dec     _KiInBiosCall                         ; Clear  the 'In Bios' flag

        add     esp,LocalStack                                          ;remove scratch space
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        stdRET    _KeI386Call16BitFunction

stdENDP _KeI386Call16BitFunction

;++
; USHORT
; KeI386Call16BitCStyleFunction (
;     IN ULONG EntryOffset,
;     IN ULONG EntrySelector,
;     IN PUCHAR Parameters,
;     IN ULONG Size
;     )
;
; Routine Description:
;
;     This function calls the 16 bit function which supports C style calling convension.
;
; Parameters:
;
;     EntryOffset and EntrySelector - specifies the entry point of the 16 bit function.
;
;     Parameters - supplies a pointer to a parameter block which will be
;         passed to 16 bit function as parameters.
;
;     Size - supplies the size of the parameter block.
;
;   NOTE: Caller must be at DPC_LEVEL
;
; Returned Value:
;
;     AX returned by 16 bit function.
;
;--

cPublicProc _KeI386Call16BitCStyleFunction,4

;
;  verify CurrentIrql
;  verify context flags
;

        push    ebp                             ; save nonvolatile registers
        push    ebx
        push    esi
        push    edi

        COPY_CALL_FRAME _KiBiosFrame

        inc     _KiInBiosCall                         ; Set the 'In Bios' flag
if DBG
        cmp   _KiInBiosCall,2
        jb  @F
        push    offset FLAT:KiBiosReenteredAssert
        call    _dbgPrint
        add     esp, 4
@@:
endif

        mov     edi, esp
        sub     esp,LocalStack          ;  now, add in some scratch space
        mov     esi, dword ptr [esp + LocalStack +28]       ; (esi)->BiosParameters
        or         esi, esi
        jz         short @f

        mov    ecx, [esp + LocalStack +32]                 ; (ecx) = parameter size
        sub    esp, ecx                        ; allocate space on TOS to copy parameters

        mov   edi, esp
        rep     movsb                           ; (edi)-> Top of nonvolatile reg save area
        add    edi, LocalStack           ; edi now points to original stack

@@:

;
; We're using a 32bit CS:EIP - go to a 16bit CS:IP
; Note the base of KiAbiosCallSelector is the flat address of _KiI386AbiosCall
; routine.
;

;
; Remap current stack to 16:16 stack.  The base of the 16bit stack selector is
; the base of current kernel stack.
;

        STACK32_TO_STACK16                      ; Switch to 16bit stack

;
; Push return address from 16 bit function call to kernel
;

        push    word ptr KGDT_CODE16
        push    word ptr (offset FLAT:Kbfex40 - offset FLAT:_KiI386CallAbios@16)

        push    word ptr 0200h                  ; flags
        push    word ptr [edi + 24 ]             ; entry selector
        push    word ptr [edi + 20 ]             ; entry offset

;
; Switch to 16bit CS
;
        push    KGDT_CODE16
        push    offset FLAT:Kbfex30 - offset FLAT:_KiI386CallAbios@16
        retf

Kbfex30:
;
; "call" to 16 bit function
;
        iretd

Kbfex40:
;
; Save return value.
;

        db      OPERAND_OVERRIDE
        push    eax

;
; Restore Flat mode segment registers.
;

        db      OPERAND_OVERRIDE
        mov     eax, KGDT_R0_PCR
        mov     fs, ax

        db      OPERAND_OVERRIDE
        mov     eax, KGDT_R3_DATA OR RPL_MASK
        mov     ds, ax
        mov     es, ax

        xor     eax, eax

;
; Switch back to 32 bit stack
;

        STACK16_TO_STACK32

;
; Push the flat code selector followed by the return offset, then
; execute a far return and we'll be back in the 32-bit code space.
;


        db      OPERAND_OVERRIDE
        push    KGDT_R0_CODE
        db      OPERAND_OVERRIDE
        push    offset FLAT:Kbfex50
        db      OPERAND_OVERRIDE
        retf

Kbfex50:
        pop     eax

;
; Restore regs & return
;
        dec    _KiInBiosCall                         ; Clear  the 'In Bios' flag

        mov     esp, edi                                 ; Also removes the scratch space!
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        stdRET    _KeI386Call16BitCStyleFunction

stdENDP _KeI386Call16BitCStyleFunction

;
; BBT cannot instrument code between BBT_Exclude_Selector_Code_Begin and this label
;

        public  _BBT_Exclude_Selector_Code_End
_BBT_Exclude_Selector_Code_End  equ     $
        int 3

;;
;; END - power_management
;;
;; ********************************************************


        public  _KiEndOfCode16
_KiEndOfCode16  equ     $



_TEXT   ends
        end
