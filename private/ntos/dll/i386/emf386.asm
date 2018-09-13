        subttl  emf386.asm - 32 bit Emulator Interrupt Handler
        page
;***
;emf386.asm - 32 bit Emulator Interrupt Handler
;
;        IBM/Microsoft Confidential
;
;        Copyright (c) IBM Corporation 1987, 1989
;        Copyright (c) Microsoft Corporation 1987, 1989
;
;        All Rights Reserved
;
;Purpose:
;       32 bit Emulator Interrupt Handler
;
;Revision History:  (also see emulator.hst)
;
;    1/21/92  JWM   Minor modifications for DOSX32 emulator
;    8/23/91  TP    Reduce to only two decoding steps
;
;*******************************************************************************


;*********************************************************************;
;                                                                     ;
;         Main Entry Point and Address Calculation Procedure          ;
;                                                                     ;
;               80386 version                                         ;
;                                                                     ;
;*********************************************************************;
;
; This routine fetches the 8087 instruction, calculates memory address
; if necessary into ES:ESI and calls a routine to emulate the instruction.
; Most of the dispatching is done through tables. (see comments in CONST)
;
; The instruction dispatching is designed to favor the 386 addressing modes


ifdef _DOS32EXT                 ; JWM
public __astart
__astart:
        mov     eax, 1
        ret

public _Ms32KrnlHandler
_Ms32KrnlHandler:
endif

ifdef   NT386

;
; NPXEmulatorTable is a table read by the Windows/NT kernel in
; order to support the R3 emulator
;
public _NPXEMULATORTABLE
_NPXEMULATORTABLE   label   dword
        dd      offset NpxNpHandler     ; Address of Ring3 Trap7 handler
        dd      offset tRoundMode       ; Address of rounding vector table
endif

public NPXNPHandler
NPXNPHandler:

ifdef  DEBUG
        int     3
endif
        cld                             ; clear direction flag forever

ifdef NT386


;-- BUGBUG - bryanwi - 16Oct91 - Hack FP fix, not pointing IDT:7 at this
;   routine for 16bit code is the right thing to do.
;
;   Check to see if we are running on flat SS.  If so, assume things
;   are OK and proceed.  (If a 16bit app loads the flat SS and then
;   does an FP instruction, they're hosed, no skin off our nose.)
;
;   If SS not what we expect, then either (a) a flat apps is *very*
;   confused, or (b) a 16 bit app has hit an FP instuction.  In either
;   case, this emulator is not going to work.  Therefore, raise an exception.
;

        push    ax                      ; use form that will word with any SS
        mov     ax,ss
        or      ax,RPL_MASK
        cmp     ax,(KGDT_R3_DATA OR RPL_MASK)
        pop     ax
        jz      OK_Segment              ; Segments are OK, proceed normally.

        jmp     Around

_DATA   SEGMENT  DWORD USE32 PUBLIC 'DATA'

    align 4

EmerStk         db      1024 dup (?)                    ; *** SaveContext is assumed to be
SaveContext     db  size ContextFrameLength  dup (?)    ; *** at the top of the EmerStk by
SaveException   db  size ExceptionRecordLength dup (?)  ; *** the function @ 13f:0

_DATA   ENDS

Around:
;
;   Trap occured in 16bit code, get to flat environment and raise exception
;

        push    eax                             ; save EAX on old stack
        mov     ax, ds
        push    eax                             ; Save DS on old stack

        mov     ax,(KGDT_R3_DATA OR RPL_MASK)
        mov     ds,ax
    ASSUME  DS:FLAT

        pop     dword ptr [SaveContext.CsSegDs] ; remove ds  from old stack
        pop     dword ptr [SaveContext.CsEax]   ; remove eax from old stack
        pop     dword ptr [SaveContext.CsEip]   ; copy eip   from old stack
        pop     dword ptr [SaveContext.CsSegCs] ; copy cs    from old stack
        pop     dword ptr [SaveContext.CsEflags] ; copy eflag from old stack

        push    dword ptr [SaveContext.CsEFlags] ; restore eflag to old stack
        push    dword ptr [SaveContext.CsSegCs] ; restore cs    to old stack
        push    dword ptr [SaveContext.CsEip]   ; restore eip   to old stack
        mov     dword ptr [SaveContext.CsEsp], esp

;
; Build rest of context frame
;

        mov     dword ptr [SaveContext.CsContextFlags],CONTEXT_CONTROL OR CONTEXT_SEGMENTS OR CONTEXT_INTEGER
        mov     dword ptr [SaveContext.CsEbx], ebx
        mov     dword ptr [SaveContext.CsEcx], ecx
        mov     dword ptr [SaveContext.CsEdx], edx
        mov     dword ptr [SaveContext.CsEsi], esi
        mov     dword ptr [SaveContext.CsEdi], edi
        mov     dword ptr [SaveContext.CsEbp], ebp
        mov     dword ptr [SaveContext.CsSegEs], es
        mov     dword ptr [SaveContext.CsSegFs], fs
        mov     dword ptr [SaveContext.CsSegGs], gs
        mov     dword ptr [SaveContext.CsSegSs], ss

        mov     ss,ax                   ; Switch to new stack
        mov     esp,(OFFSET FLAT:EmerStk) + 1024
    ASSUME  SS:FLAT

;
;   ss: flat, esp -> EmerStk
;

        mov     ax,KGDT_R3_TEB OR RPL_MASK
        mov     fs, ax
        mov     ecx, fs:[TbVdm]
        or      ecx, ecx
        jne     short DoVdmFault

        mov     ecx, offset SaveContext         ; (ecx) -> context record
        mov     edx, offset SaveException       ; (edx) -> exception record

        mov     dword ptr [edx.ErExceptionCode],STATUS_ILLEGAL_FLOAT_CONTEXT
        mov     dword ptr [edx.ErExceptionFlags],0
        mov     dword ptr [edx.ErExceptionRecord],0
        mov     ebx, [ecx.CsEip]
        mov     [edx.ErExceptionAddress],ebx
        mov     [edx.ErNumberParameters],0

;
;   ZwRaiseException(edx=ExceptionRecord, ecx=ContextRecord, TRUE=FirstChance)
;

        stdCall _ZwRaiseException, <edx, ecx, 1>

;
;   If we come back HERE, things are hosed.  We cannot bugcheck because
;   we are in user space, so int-3 and loop forever instead.
;

Forever:
        int     3
        jmp     short Forever

DoVdmFault:
;
; Does the VDM want the fault, or should the instruction be skipped
;
        test    ds:[ecx].VtVdmContext.CsFloatSave.FpCr0NpxState, CR0_EM
        jz      short SkipNpxInstruction

        add     dword ptr [SaveContext.CsEsp], 12   ; remove from old stack

; jump to the dos extender NPX exception handler

;       jmp     far ptr 013fh:0
        db      0eah
        dd      0
        dw      013fh

SkipNpxInstruction:
        mov     ax,(KGDT_R3_DATA OR RPL_MASK)
        mov     es,ax

        stdCall _NpxNpSkipInstruction, <offset SaveContext>

        mov     ebx, dword ptr [SaveContext.CsEbx]
        mov     ecx, dword ptr [SaveContext.CsEcx]
        mov     edx, dword ptr [SaveContext.CsEdx]
        mov     edi, dword ptr [SaveContext.CsEdi]
        mov     esi, dword ptr [SaveContext.CsEsi]
        mov     ebp, dword ptr [SaveContext.CsEbp]
        mov     gs,  dword ptr [SaveContext.CsSegGs]
        mov     fs,  dword ptr [SaveContext.CsSegFs]
        mov     es,  dword ptr [SaveContext.CsSegEs]

        mov     eax, dword ptr [SaveContext.CsEsp]
        mov     ss,  dword ptr [SaveContext.CsSegSs]  ; switch to original stack
        mov     esp, eax

        add     esp, 12                     ; remove eflag, cs, eip
        push    dword ptr [SaveContext.CsEflags]
        push    dword ptr [SaveContext.CsSegCs]
        push    dword ptr [SaveContext.CsEip]
        mov     eax, dword ptr [SaveContext.CsEax]
        mov     ds,  dword ptr [SaveContext.CsSegDs]

        iretd                               ; restore eflag, cs, eip

OK_Segment:
endif


        push    ds                      ; save segment registers

        GetEmData   ds

        push    EMSEG:[LongStatusWord]  ;In case we're saving status
        push    EMSEG:[PrevCodeOff]     ;In case we save environment
;Save registers in order of their index number
        push    edi
        push    esi
        push    ebp
        push    esp
        add     dword ptr [esp],regFlg-regESP   ; adjust to original esp
        push    ebx
        push    edx
        push    ecx
        push    eax

        cmp     EMSEG:[Einstall], 0     ; Make sure emulator is initialized.
        je      InstalEm

EmInstalled:
        mov     edi,[esp].regEIP            ;edi = 387 instruction address
        movzx   edx, word ptr cseg:[edi]    ;dx = esc and opcode

; Check for unmasked errors
        mov     al, EMSEG:[CURerr]      ; fetch errors
        and     al, EMSEG:[ErrMask]
        jnz     short PossibleException

; UNDONE: rip test for FWAIT in final version
        cmp     dl, 9bh                 ;FWAIT?
        je      sawFWAIT

NoException:
Execute387inst:
;Enter here if look-ahead found another 387 instruction
        mov     EMSEG:[PrevCodeOff],edi
        mov     EMSEG:[CurErrCond],0    ;clear error and cond. codes, show busy
        add     edi, 2                  ; point past opcode
        
;CONSIDER:  remove the two instruction below and muck with EA386Tab
;CONSIDER:  to optimize for mem ops instead of reg ops.
        add     dh,40h                  ; No effective address?
        jc      NoEffectiveAddress0     ;  yes, go do instruction
        rol     dh,2                    ; rotate MOD field next to r/m field
        mov     bl,dh
        and     ebx,1FH                 ; Mask to MOD and r/m fields
MemModeDispatch:                        ;Label for debugging
        jmp     EA386Tab[4*ebx]


InstalEm:
        call    EmulatorInit
        mov     edi,DefaultControlWord  ; Default mode to start in
        mov     eax, edi
        call    SetControlWord          ; Set it
        mov     EMSEG:[LongControlWord], edi    ; reset reserved bits
        jmp     EmInstalled

; ************************

;
; We are about to execute a new FP instruction and there is an
; unmasked expcetion.  Check to see if the new FP instruction is
; a "no wait" instruction.   If so, let it proceede; otherwise, raise
; the exception.
;

PossibleException:
        cmp     edx, 0E3DBh             ; if fninit, no exception
        je      short NoException

        cmp     edx, 0E2DBh             ; if fnclex, no exception
        je      short NoException

        cmp     edx, 0E0DFh             ; if "fnstsw ax", no exception
        je      short NoException

        cmp     dl, 0D9h                ; possible encoding for fnstenv or fnstcw?
        je      short pe20              ; yes, check mod r/m
        cmp     dl, 0DDh                ; possible encoding for fnsave or fnstsw?
        jne     short pe30

pe20:   mov     bl, dh                  ; bl = op2
        shr     bl, 3
        and     bl, 7                   ; bl = mod r/m
        cmp     bl, 6                   ; is it a 6 or 7?
        jnc     short NoException       ; yes, no exception

pe30:
        jmp     CommonExceptions        ; unmasked exception is pending, raise it

; ************************



;       386 address modes

;       SIB does not handle SS overrides for ebp

SIB     macro   modval
        local   SIBindex,SIBbase

        movzx   ebx,byte ptr cseg:[edi] ; ebx = SIB field
        inc     edi                     ; bump past SIB field
        mov     eax,ebx
        and     al,7                    ; mask down to base register

if      modval eq 0
        cmp     al,5                    ; base = ebp
        jne     short SIBbase           ;   yes - get base register value
        mov     eax,cseg:[edi]          ; eax = disp32
        add     edi,4                   ; bump past displacement
        jmp     SIBindex                ; Added to support bbt, 
                                        ;   replacing    SKIP    3,SIBindex
endif

SIBbase:
        mov     eax,[esp+4*eax]         ; eax = base register value

SIBindex:
        mov     [esp].regESP,0          ; no esp indexing allowed
        mov     cl,bl
        shr     cl,6                    ; cl = scale factor
        and     bl,7 shl 3              ; ebx = 8 * index register
        shr     bl,1
        mov     esi,[esp+1*ebx]         ; esi = index register value
        shl     esi,cl                  ; esi = scaled index register value
        add     esi,eax                 ; esi = SIB address value
        endm


        ALIGN   4

SIB00:
        SIB     00                      ; decode SIB field
        jmp     CommonMemory

        ALIGN   4

SIB01:
        SIB     01                      ; decode SIB field
        movsx   eax,byte ptr cseg:[edi]
        inc     edi
        add     esi,eax
        jmp     short CommonMemory

        ALIGN   4

SIB10:
        SIB     10                      ; decode SIB field
        mov     eax,cseg:[edi]
        add     edi,4
        add     esi,eax
        jmp     short CommonMemory


;       386 single register addressing

        ALIGN   4

Exx00:
        and     bl,7 shl 2              ; mask off mod bits
        mov     esi,[esp+1*ebx]
        jmp     short CommonMemory

        ALIGN   4

Exx01:
        and     bl,7 shl 2              ; mask off mod bits
        mov     esi,[esp+1*ebx]
        movsx   eax,byte ptr cseg:[edi]
        inc     edi
        add     esi,eax
        jmp     short CommonMemory

        ALIGN   4

Exx10:
        and     bl,7 shl 2              ; mask off mod bits
        mov     esi,[esp+1*ebx]
        add     esi,cseg:[edi]
        add     edi,4
        jmp     short CommonMemory


;       386 direct addressing

        ALIGN   4

Direct386:
        mov     esi,cseg:[edi]
        add     edi,4

CommonMemory:
        MOV     [esp].regEIP,edi        ; final return offset


; At this point ESI = memory address, dx = |Op|r/m|MOD|escape|MF|Arith|
; Current format of opcode and address mode bytes (after rol dh,2)
;
;  7 6 5 4 3 2 1 0
; |1 1 0 1 1| op1 |   dl
;
;  7 6 5 4 3 2 1 0
; | op2 | r/m |mod|   dh
;
;op1 and op2 fields together make the FP opcode

        rol     dx,5                    ; dl = | op1 | op2 |? ?|
        and     edx,0FCH                ;Keep only op1 & op2 bits
        push    offset EMLFINISH
        mov     edi,EMSEG:[CURstk]
MemOpDisp:                              ;Debugging label
;edi = [CURstk]
        jmp     tOpMemDisp[edx]


        ALIGN   4


NoEffectiveAddress0:
        rol     dh,2
NoEffectiveAddress:                     ; Either Register op or Miscellaneous
        mov     [esp].regEIP,edi        ; final return offset

;Current format of opcode and address mode bytes (after rol dh,2)
;
;  7 6 5 4 3 2 1 0
; |1 1 0 1 1| op1 |   dl
;
;  7 6 5 4 3 2 1 0
; | op2 | r/m |mod|   dh
;
;op1 and op2 fields together make the FP opcode

        mov     al,dh                   ;Save r/m bits (contains reg. no.)
        rol     dx,5                    ; dl = | op1 | op2 |? ?|
        and     edx,0FCH                ;Keep only op1 & op2 bits
        push    offset EMLFINISH
        and     eax,7 shl 2             ;Mask to register number * 4
        mov     edi,EMSEG:[CURstk]
        lea     esi,[2*eax+eax]         ;Register no. * 12
        add     esi,edi
        cmp     esi,ENDstk              ;Run past end?
        jae     RegWrap
RegOpDisp:                              ;Debugging label
;eax = r/m bits * 4
;esi = FP register address
;edi = [CURstk]
        jmp     tOpRegDisp[edx]

        ALIGN   4
RegWrap:
        sub     esi,ENDstk - BEGstk     ;Wrap around    JWM
RegOpDispWrap:                          ;Debugging label
        jmp     tOpRegDisp[edx]


SawFwait:
        inc     edi                     ; bump past FWAIT
        mov     [esp].regEIP,edi        ; final return offset
        mov     EMSEG:[CURErr],0        ; clear current error and cond. codes

; return from routine;  restore registers and return

        align   4
EMLFINISH:
; check for errors
        mov     al, EMSEG:[CURerr]      ; fetch errors
        or      al, EMSEG:[SWerr]
        mov     EMSEG:[SWerr],al        ; set errors in sticky error flag
        and     al,EMSEG:[ErrMask]
        jnz     CommonExceptions

ifdef TRACENPX
        jmp     CommonExceptions
endif

if DBG eq 0

;
; On a free build, look ahead to next instruction
;

;09BH is FWAIT - just skip it
;0D8H - 0DFH is 387 instruction, emulate it
        mov     edi,[esp].regEIP        ;edi = 387 instruction address
        mov     dx,cseg:[edi]
        cmp     dl,09BH                 ;FWAIT?
        jz      short SawFwait
        sub     dl,0D8H
        cmp     dl,8
        jb      ReExecute
endif
        mov     ebx,[esp].[OldLongStatus]
        and		ebx,LongSavedFlags		;preserve condition codes, error flags
        or		EMSEG:[LongStatusWord],ebx	;merge saved status word, condition codes
        
        pop     eax
        pop     ecx
        pop     edx
        pop     ebx
        add     esp,4                   ; toss esp value
        pop     ebp
        pop     esi
        pop     edi
        add     esp,8                   ;toss old PrevCodeOff and StatusWord
        mov     EMSEG:[CURerr],Summary  ;Indicate we are not busy
        pop     ds
        error_return                    ; common exit sequence

ReExecute:
        mov     eax,EMSEG:[LongStatusWord]
        mov     ebx,[esp].[OldLongStatus]
        and		ebx,LongSavedFlags		;preserve condition codes, error flags
        or		eax,ebx					;merge saved status word, condition codes
        mov     [esp].OldLongStatus,eax
        mov     eax,EMSEG:[PrevCodeOff]
        mov     [esp].OldCodeOff,eax
        lea		eax,[esp+regFlg+4]		;must restore "saved" esp
        mov		[esp].RegEsp,eax
        jmp     Execute387inst
