        page    78,132
;*******************************************************************************
;        Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;   ke\i386\emxcptn.asm
;
;       Module to support getting/setting context to and from the R3
;       emulator.
;
;Revision History:
;
;
;*******************************************************************************

        .386p
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


;;*******************************************************************************
;;
;;   Include some more macros and constants.
;;
;;*******************************************************************************
;

NT386   equ     1

        include ks386.inc
        include em387.inc               ; Emulator TEB data layout
        include callconv.inc

        EXTRNP  _KeGetCurrentIrql,0
        EXTRNP  _KeBugCheck,1
        EXTRNP  _ExRaiseStatus,1
        extrn   _Ki387RoundModeTable:dword


        subttl  _KiEm87StateToNpxFrame
        page

;*** _KiEm87StateToNpxFrames
;
;  Translates the R3 emulators state to the NpxFrame
;
;  Returns TRUE if NpxFrame sucessfully completed.
;   else FALSE
;
;  Warning: This function can only be called at Irql 0 with interrupts
;  enabled.  It is intended to be called only to deal with R3 exceptions
;  when the emulator is being used.
;
;  Revision History:
;
;
;*******************************************************************************

cPublicProc _KiEm87StateToNpxFrame, 1
        push    ebp
        mov     ebp, esp
        push    ebx                     ; Save C runtime varibles
        push    edi
        push    esi

        push    esp                     ; Pass current Esp to handler
        push    offset stnpx_30         ; Set Handler address
        push    PCR[PcExceptionList]    ; Set next pointer
        mov     PCR[PcExceptionList],esp ; Link us on

if DBG
        pushfd                              ; Sanity check
        pop     ecx                         ; make sure interrupts are enabled
        test    ecx, EFLAGS_INTERRUPT_MASK
        jz      short stnpx_err

        stdCall _KeGetCurrentIrql           ; Sanity check
        cmp     al, DISPATCH_LEVEL          ; make sure Irql is below DPC level
        jnc     short stnpx_err
endif

        xor     eax, eax                ; set FALSE

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jnz     short stnpx_10               ; Yes, then not supported

        mov     ebx, PCR[PcTeb]         ; R3 Teb
        cmp     [ebx].Einstall, 0       ; Initialized?
        je      short stnpx_10          ; No, then don't return NpxFrame

        test    [ebx].CURErr, Summary   ; Completed?
        jz      short stnpx_10          ; No, then don't return NpxFrame

        mov     esi, [ebp+8]            ; (esi) = NpxFrame
        call    SaveState

        mov     eax, 1                  ; Return TRUE
stnpx_10:
        pop     PCR[PcExceptionList]    ; Remove our exception handle
        add     esp, 8                  ; clear stack
        pop     esi
        pop     edi
        pop     ebx
        pop     ebp
        stdRET  _KiEm87StateToNpxFrame

stnpx_30:
;
; WARNING: Here we directly unlink the exception handler from the
; exception registration chain.  NO unwind is performed.  We can take
; this short cut because we know that our handler is a leaf-node.
;

        mov     esp, [esp+8]            ; (esp)-> ExceptionList
        xor     eax, eax                ; Return FALSE
        jmp     short stnpx_10

if DBG
stnpx_err:
        stdCall _KeBugCheck <IRQL_NOT_LESS_OR_EQUAL>
endif
_KiEm87StateToNpxFrame ENDP


;*** SaveEnv
;
;
;   ARGUMENTS
;
;       (esi) = NpxFrame
;       (ebx) = PcTeb
;
;
;   DESCRIPTION
;

SaveEnv:
        xor     ax,ax
        mov     [esi].reserved1,ax
        mov     [esi].reserved2,ax
        mov     [esi].reserved3,ax
        mov     [esi].reserved4,ax
        mov     [esi].reserved5,ax
        mov     ax,[ebx].ControlWord
        mov     [esi].E32_ControlWord,ax
        call    GetEMSEGStatusWord
        mov     [esi].E32_StatusWord,ax
        call    GetTagWord
        mov     [esi].E32_TagWord,ax
        mov     ax,cs
        mov     [esi].E32_CodeSeg,ax    ; NOTE: Not R0 code & stack
        mov     ax,ss
        mov     [esi].E32_DataSeg,ax
        mov     eax,[ebx].PrevCodeOff
        mov     [esi].E32_CodeOff,eax
        mov     eax,[ebx].PrevDataOff
        mov     [esi].E32_DataOff,eax
        ret


;*** SaveState -
;
;   ARGUMENTS
;       (esi) = where to store environment
;       (ebx) = PcTeb
;
;   DESCRIPTION
;
;   REGISTERS
;       Destroys ALL, but EBX
;

SaveState:                              ; Enter here for debugger save state
        mov     dword ptr [esi].FpCr0NpxState, CR0_EM

        call    SaveEnv
        add     esi,size Env80x87_32    ;Skip over environment
        mov     ebp,NumLev              ;Save entire stack
        mov     edi,[ebx].CURstk
ss_loop:
        mov     eax,[ebx+edi].ExpSgn
        call    StoreTempReal           ;in emstore.asm
        add     esi,10

        mov     edi,[ebx].CURstk
;;;     NextStackElem   edi,SaveState
        cmp     edi,INITstk
        jae     short ss_wrap
        add     edi,Reg87Len
ss_continue:
        mov     [ebx].CURstk,edi
        dec     ebp
        jnz     short ss_loop
        ret
ss_wrap:
        mov     edi, BEGstk
        jmp     short ss_continue


;***    GetTagWord - figures out what the tag word is from the numeric stack
;                  and returns the value of the tag word in ax.
;
;   ARGUMENTS
;       (ebx) = PcTeb
;

GetTagWord:
        push    esi
        xor     eax, eax
        mov     ecx, NumLev             ; get tags for regs. 0, 7 - 1
        mov     esi, INITstk
GetTagLoop:
        mov     dh, [ebx+esi].bTag      ; The top 2 bits of Tag are the X87 tag bits.
        shld    ax, dx, 2
        sub     esi, Reg87Len
        loop    GetTagLoop
        rol     ax, 2                   ; This moves Tag(0) into the low 2 bits
        pop     esi
        ret


;***    GetEMSEGStatusWord
;
; User status word returned in ax.
; Uses status word in per-thread data area, otherwise
;   identical to GetStatusWord
;
;   ARGUMENTS
;       (ebx) = PcTeb

GetEMSEGStatusWord:
        mov     eax, [ebx].CURstk
        sub     eax, BEGstk

        ;
        ; Make sure the 'div' won't overflowed.
        ;

        cmp     eax, Reg87Len * (NumLev + 2)
        ja      short @f

        mov     dl,Reg87Len
        div     dl
        inc     eax
        and     eax, 7                  ; eax is now the stack number
        shl     ax, 11
        or      ax, [ebx].StatusWord    ; or in the rest of the status word.
        ret
@@:
        mov     eax, STATUS_INTEGER_OVERFLOW
        stdCall _ExRaiseStatus, <eax>
        ret                             ; Should never come here ...

;***  StoreTempReal
;
;
;   ARGUMENTS
;       ??
;       (ebx) = PcTeb
;

StoreTempReal:
        mov     edx,[ebx+edi].lManHi
        mov     edi,[ebx+edi].lManLo
;mantissa in edx:edi, exponent in high eax, sign in ah bit 7, tag in al
;memory destination is esi
        mov     ecx,eax                 ;get copy of sign and tag
        shr     ecx,16                  ;Bring exponent down
        cmp     al,bTAG_ZERO
        jz      short StoreIEEE80       ;Skip bias if zero
        add     ecx,IexpBias-TexpBias   ;Correct bias
        cmp     al,bTAG_DEN
        jz      short Denorm80
StoreIEEE80:
        and     eax,bSign shl 8
        or      ecx,eax                 ;Combine sign with exponent
        mov     [esi],edi
        mov     [esi+4],edx
        mov     [esi+8],cx
        ret

Denorm80:
;Must change it to a denormal
        dec     ecx
        neg     ecx                     ;Use as shift count
        cmp     cl,32                   ;Long shift?
        jae     LongDenorm
        shrd    edi,edx,cl
        shr     edx,cl
        xor     ecx,ecx                 ;Exponent is zero
        jmp     short StoreIEEE80

LongDenorm:
;edi must be zero if we have 32 bits to shift
        xchg    edx,edi                 ;32-bit right shift
        shr     edi,cl                  ;shift count is modulo-32
        xor     ecx,ecx                 ;Exponent is zero
        jmp     short StoreIEEE80


;****************************************************
;****************************************************
;****************************************************
;****************************************************


;*** _KiNpxFrameToEm87State
;
;  Translates the NpxFrame to the R3 emulators state
;
;  Returns TRUE if NpxFrame state sucessfully transfered.
;   else FALSE
;
;  Warning: This function can only be called at Irql 0 with interrupts
;  enabled.  It is intended to be called only to deal with R3 exceptions
;  when the emulator is being used.
;
;  Revision History:
;
;
;*******************************************************************************

cPublicProc _KiNpxFrameToEm87State, 1
        push    ebp
        mov     ebp, esp
        push    ebx                     ; Save C runtime varibles
        push    edi
        push    esi

        push    esp                     ; Pass current Esp to handler
        push    offset npxts_30         ; Set Handler address
        push    PCR[PcExceptionList]    ; Set next pointer
        mov     PCR[PcExceptionList],esp  ; Link us on

if DBG
        pushfd                              ; Sanity check
        pop     ecx                         ; make sure interrupts are enabled
        test    ecx, EFLAGS_INTERRUPT_MASK
        jz      short npxts_err

        stdCall _KeGetCurrentIrql           ; Sanity check
        cmp     al, DISPATCH_LEVEL          ; make sure Irql is below DPC level
        jnc     short npxts_err
endif

        xor     eax, eax                ; set FALSE

        mov     ebx,PCR[PcPrcbData+PbCurrentThread]
        mov     ebx,[ebx]+ThApcState+AsProcess
        test    byte ptr [ebx]+PrVdmFlag,0fh ; is this a vdm process?
        jnz     short npxts_10               ; Yes, then not supported

        mov     ebx, PCR[PcTeb]         ; R3 Teb
        cmp     [ebx].Einstall, 0       ; Initialized?
        je      short npxts_10          ; No, then don't set NpxFrame

        mov     esi, [ebp+8]            ; (esi) = NpxFrame
        call    StorState
        or      [ebx].CURErr, Summary   ; Set completed

        mov     eax, 1                  ; Return TRUE
npxts_10:
        pop     PCR[PcExceptionList]    ; Remove our exception handle
        add     esp, 8                  ; clear stack
        pop     esi
        pop     edi
        pop     ebx
        pop     ebp
        stdRet  _KiNpxFrameToEm87State

npxts_30:
;
; WARNING: Here we directly unlink the exception handler from the
; exception registration chain.  NO unwind is performed.  We can take
; this short cut because we know that our handler is a leaf-node.
;

        mov     esp, [esp+8]            ; (esp)-> ExceptionList
        xor     eax, eax                ; Return FALSE
        jmp     short npxts_10
        ret

if DBG
npxts_err:
        stdCall _KeBugCheck <IRQL_NOT_LESS_OR_EQUAL>
endif
_KiNpxFrameToEm87State ENDP



;*** StorState - emulate FRSTOR  [address]
;
;   ARGUMENTS
;       (esi)  = where to get the environment
;       (ebx)  = PcTeb
;
;
;   DESCRIPTION
;           This routine emulates an 80387 FRSTOR (restore state)

StorState:
;First we set up the status word so that [CURstk] is initialized.
;The floating-point registers are stored in logical ST(0) - ST(7) order,
;not physical register order.  We don't do a full load of the environment
;because we're not ready to use the tag word yet.

        mov     ax, [esi].E32_StatusWord
        call    SetEmStatusWord         ;Initialize [CURstk]
        add     esi,size Env80x87_32    ;Skip over environment

;Load of temp real has one difference from real math chip: it is an invalid
;operation to load an unsupported format.  By ensuring the exception is
;masked, we will convert unsupported format to Indefinite.  Note that the
;mask and [CURerr] will be completely restored by the FLDENV at the end.

        mov     [ebx].CWmask,3FH        ;Mask off invalid operation exception
        mov     edi,[ebx].CURstk
        mov     ebp,NumLev
FrstorLoadLoop:
        push    esi
        call    LoadTempReal            ;In emload.asm
        pop     esi
        add     esi,10          ;Point to next temp real
;;;     NextStackElem   edi,Frstor
        cmp     edi,INITstk
        jae     short fr_wrap
        add     edi,Reg87Len
fr_continue:
        dec     ebp
        jnz     short FrstorLoadLoop
        sub     esi,NumLev*10+size Env80x87_32  ;Point to start of env.
;
; Stor Enviroment
; (esi) = where to get enviroment
; (ebx) = PcTeb
;

        mov     ax, [esi].E32_StatusWord
        call    SetEmStatusWord                 ; set up status word
        mov     ax, [esi].E32_ControlWord
        call    SetControlWord
        mov     ax, [esi].E32_TagWord
        call    UseTagWord

        mov     eax, [esi].E32_CodeOff
        mov     [ebx].PrevCodeOff, eax
        mov     eax, [esi].E32_DataOff
        mov     [ebx].PrevDataOff, eax
        ret

fr_wrap:
        mov     edi, BEGstk
        jmp     short fr_continue


;***    SetEmStatusWord -
;
; Given user status word in ax, set into emulator.
; Destroys ebx only.


SetEmStatusWord:
        and     ax,7F7FH
        mov     cx,ax
        and     cx,3FH                  ; set up CURerr in case user
        mov     [ebx].CURerr,cl         ; wants to force an exception
        mov     ecx, eax
        and     ecx, not (7 shl 11)     ; remove stack field.
        mov     [ebx].StatusWord, cx

        sub     ah, 8                   ; adjust for emulator's stack layout
        and     ah, 7 shl 3
        mov     al, ah
        shr     ah, 1
        add     al, ah                  ; stack field * 3 * 4
.erre   Reg87Len eq 12
        and     eax, 255                ; eax is now 12*stack number
        add     eax, BEGstk
        mov     [ebx].CURstk, eax
        ret

SetControlWord:
        and     ax,0F3FH                ; Limit to valid values
        mov     [ebx].ControlWord, ax   ; Store in the emulated control word
        not     al                      ;Flip mask bits for fast compare
        and     al,3FH                  ;Limit to valid mask bits
        mov     [ebx].ErrMask,al
        and     eax,(RoundControl + PrecisionControl) shl 8
.erre   RoundControl eq 1100B
.erre   PrecisionControl eq 0011B
        shr     eax,6                   ;Put PC and RC in bits 2-5
        mov     ecx,_Ki387RoundModeTable
        mov     ecx,[ecx+eax]           ;Get correct RoundMode vector
        mov     [ebx].RoundMode,ecx
        mov     [ebx].SavedRoundMode,ecx
        and     eax,RoundControl shl (8-6)      ;Mask off precision control
        mov     ecx,_Ki387RoundModeTable
        mov     ecx,[ecx+(eax+PC64 shl (8-6))];Get correct RoundMode vector
        mov     [ebx].TransRound,ecx    ;Round mode w/o precision
        ret


;***    UseTagWord - Set up tags using tag word from environment
;
;       ARGUMENTS
;              ax - should contain the tag word
;
;       Destroys ax,bx,cx,dx,di

UseTagWord:
        ror     ax, 2                   ; mov Tag(0) into top bits of ax
        mov     edi,INITstk
        mov     ecx, NumLev
UseTagLoop:
        mov     dl,bTAG_EMPTY
        cmp     ah, 0c0h                ;Is register to be tagged Empty?
        jae     short SetTag            ;Yes, go mark it
        mov     dl,[ebx+edi].bTag       ;Get current tag
        cmp     dl,bTAG_EMPTY           ;Is register currently Empty?
        je      short SetTagNotEmpty    ;If so, go figure out tag for it
SetTag:
        mov     [ebx+edi].bTag,dl
UseTagLoopCheck:
        sub     edi, Reg87Len
        shl     eax, 2
        loop    UseTagLoop
        ret

SetTagEmpty:
        mov     [ebx+edi].bTag, bTAG_EMPTY
        jmp     short UseTagLoopCheck

SetTagNotEmpty:
;Register is currently tagged empty, but new tag word says it is not empty.
;Figure out a new tag for it.  The rules are:
;
;1. Everything is either normalized or zero--unnormalized formats cannot
;get in.  So if the high half mantissa is zero, the number is zero.
;
;2. Although the exponent bias is different, NANs and Infinities are in
;standard IEEE format - exponent is TexpMax, mantissa indicates NAN vs.
;infinity (mantissa for infinity is 800..000H).
;
;3. Denormals have an exponent less than TexpMin.
;
;4. If the low half of the mantissa is zero, it is tagged bTAG_SNGL
;
;5. Everything else is bTAG_VALID

        cmp     [ebx+edi].lManHi, 0
        mov     dl,bTAG_ZERO            ;Try zero first
        jz      short SetTag            ;Is mantissa zero?
        mov     edx,[ebx+edi].ExpSgn
        mov     dl,bTAG_DEN
        cmp     edx,TexpMin shl 16      ;Is it denormal?
        jl      short SetTag
        cmp     [ebx+edi].lManLo,0      ;Is low half zero?
.erre   bTAG_VALID eq 1
.erre   bTAG_SNGL eq 0
        setnz   dl                      ;if low half==0 then dl=0 else dl=1
        cmp     edx,TexpMax shl 16      ;Is it NAN or Infinity?
        jl      short SetTag            ;If not, it's valid
.erre   (bTAG_VALID - bTAG_SNGL) shl TAG_SHIFT eq (bTAG_NAN - bTAG_INF)
        shl     dl,TAG_SHIFT
        add     dl,bTAG_INF - bTAG_SNGL
;If the low bits were zero we have just changed bTAG_SNGL to bTAG_INF
;If the low bits weren't zero, we changed bTAG_VALID to bTAG_NAN
;See if infinity is really possible: is high half 80..00H?
        cmp     [ebx+edi].lManHi,1 shl 31   ;Is it infinity?
        jz      short SetTag            ;Store tag for infinity or NAN
        mov     dl,bTAG_NAN
        jmp     short SetTag


;***    LoadTempReal
;
;
;

LoadTempReal:
        mov     ebx,[esi+4]             ;Get high half of mantissa
        mov     cx,[esi+8]              ;Get exponent and sign
        mov     esi,[esi]               ;Get low half of mantissa
        mov     eax,ecx
        and     ch,7FH                  ;Mask off sign bit
        shl     ecx,16                  ;Move exponent to high end
        mov     ch,ah                   ;Restore sign
        jz      short ZeroOrDenorm80
;Check for unsupported format: unnormals (MSB not set)
        or      ebx,ebx
        jns     short Unsupported
        sub     ecx,(IexpBias-TexpBias) shl 16  ;Correct the bias
        cmp     ecx,TexpMax shl 16
        jge     short NANorInf80
SetupTag:
        or      esi,esi                 ;Any bits in low half?
.erre   bTAG_VALID eq 1
.erre   bTAG_SNGL eq 0
        setnz   cl                      ;if low half==0 then cl=0 else cl=1
        jmp     short SaveStack

NANorInf80:
        mov     cl,bTAG_NAN
        cmp     ebx,1 shl 31            ;Only 1 bit set means infinity
        jnz     short SaveStack
        or      esi,esi
        jnz     short SaveStack
        mov     cl,bTAG_INF
        jmp     short SaveStack

ZeroOrDenorm80:
;Exponent is zero. Number is either zero or denormalized
        or      ebx,ebx
        jnz     short ShortNorm80       ;Are top 32 bits zero?
        or      esi,esi                 ;Are low 32 bits zero too?
        jnz     LongNorm80
        mov     cl,bTAG_ZERO
        jmp     short SaveStack

;This code accepts and works correctly with pseudo-denormals (MSB already set)
LongNorm80:
        xchg    ebx,esi                 ;Shift up 32 bits
        sub     ecx,32 shl 16           ;Correct exponent
ShortNorm80:
        add     ecx,(TexpBias-IexpBias+1-31) shl 16     ;Fix up bias
        bsr     edx,ebx                 ;Scan for MSB
;Bit number in edx ranges from 0 to 31
        mov     cl,dl
        not     cl                      ;Convert bit number to shift count
        shld    ebx,esi,cl
        shl     esi,cl
        shl     edx,16                  ;Move exp. adjustment to high end
        add     ecx,edx                 ;Adjust exponent
        jmp     short SetUpTag

SaveStack:
        mov     eax, PCR[PcTeb]
        mov     [eax].CURstk,edi
        mov     [eax+edi].lManLo,esi
        mov     [eax+edi].lManHi,ebx
        mov     [eax+edi].ExpSgn,ecx
        mov     ebx, eax                ; (ebx) = PcTeb
        ret

Unsupported:
        mov     ebx, PCR[PcTeb]
        or      [ebx].CURerr,Invalid    ; (assume it's masked?)
        mov     [ebx+edi].lManLo,0
        mov     [ebx+edi].lManHi,0C0000000H
        mov     [ebx+edi].ExpSgn,TexpMax shl 16 + bSign shl 8 + bTAG_NAN
        mov     [ebx].CURstk,edi        ;Update top of stack
        ret

_TEXT   ENDS
END
