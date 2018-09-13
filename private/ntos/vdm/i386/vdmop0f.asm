        title "Opcode 0f Instruction Emulation"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    vdmop0f.asm
;
;Abstract:
;
;    This module contains the support for the 0f opcodes that are emulated
;    (such as LMSW, mov to/from CR0, mov to/from DR?).
;
;
;Author:
;
;    Dave Hastings (daveh) 23-Feb-1992
;
;Notes:
;
;    This file needs to be modified for Kenr's NPX changes, i.e. what to
;    do with CR0 values
;
;
;Revision History:
;
;--
.386p

        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
include mi386.inc
include vdm.inc

        page ,132
;   Force assume into place

_PAGE   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING
_PAGE   ENDS

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING
_TEXT$00   ENDS



_PAGE   SEGMENT DWORD USE32 PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING

        extrn OpcodeInvalid:proc
VdmOpcodeInvalid equ OpcodeInvalid

        extrn CheckEip:proc
        extrn VdmDecodeOperand:proc
        extrn RegTab:dword
        EXTRNP _KeGetCurrentThread,0
        extrn  KiVdmSetUserCR0:proc
        extrn _VdmUserCr0MapOut:byte
_PAGE   ENDS

_DATA	SEGMENT  DWORD PUBLIC 'DATA'

;
; This table is used to dispatch the special register moves
;
; WARNING Trx not implemented.

SpecialRegTab dd offset VdmOpcodeGetCrx
        dd offset VdmOpcodeGetDrx
        dd offset VdmOpcodeSetCrx
        dd offset VdmOpcodeSetDrx
;        dd offset VdmOpcodeGetTrx
        dd offset VdmOpcodeInvalid
        dd offset VdmOpcodeInvalid
;        dd offset VdmOpcodeSetTrx
        dd offset VdmOpcodeInvalid

;
; This table is used to locate the debug register contents in the trap frame
; The values for DR4, DR5 should never be used, since these registers are
; reserved by intel
;
DrTab   dd TsDr0
        dd TsDr1
        dd TsDr2
        dd TsDr3
        dd 0BADF00Dh            ; hopefully this will fault if used (dr4)
        dd 0BADF00Dh            ; hopefully this will fault if used (dr5)
        dd TsDr6
        dd TsDr7

_DATA   ENDS


_PAGE   SEGMENT DWORD USE32 PUBLIC 'CODE'
        ASSUME DS:FLAT, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

        page ,132
        subttl "Emulate 0f opcodes"
;++
;
;   Routine Description:
;
;       This routine dispatches for the opcode 0f emulation.  Currently,
;       only LMSW, MOV to/from CR0, and MOV to /from DRx are emulated.
;       The others cause access violations in user mode.
;
;       Interrupts are disabled upon entry, and enabled upon exit
;
;   Arguments:
;
;       esi = address of reg info
;
;   Returns:
;
;       EAX = true if the opcode was emulated.
;               reginfo updated
;       EAX = false if the opcode was not emulated
;
;
        public VdmOpcode0f
VdmOpcode0f proc


        push    ebp
        mov     ebp,esp
        push    edi

;
; Construct address of next opcode byte, and check for gp fault
;
        inc     [esi].RiEip
        call    CheckEip
        test    al,0fh
        jz      of40                    ; Gp fault, report error

        mov     edi,[esi].RiCsBase
        add     edi,[esi].RiEip
        movzx   edx,byte ptr [edi]
        cmp     edx,MI_LMSW_OPCODE
        jne     of10

        call    VdmOpcodeLmsw
        jmp     of30                    ; return

of10:   cmp     edx,MI_CLTS_OPCODE
        jne     of20

        call    VdmOpcodeClts
        jmp     of30                    ; return

of20:   cmp     edx,MI_GET_CRx_OPCODE
        jb      of40                    ; return error

        cmp     edx,MI_SET_TRx_OPCODE
        ja      of40                    ; return error

        sub     edx,MI_GET_CRx_OPCODE
        call    SpecialRegTab[edx * 4]

of30:
        pop     edi
        mov     esp,ebp
        pop     ebp

        ret

of40:   xor     eax,eax
        jmp     of30

VdmOpcode0f endp

        page ,132
        subttl "Emulate LMSW"
;++
;
;    Routine Desription:
;
;       This routine emulates the LMSW instruction.  It allows the dos
;       application to set or clear EM and MP.  It forces MP to be set
;       when EM is clear.  It ignores to change any of the other bits.
;
;    Arguments:
;
;       esi = pointer to reg info
;
;    Returns:
;
;       EAX = true if the opcode was emulated.
;               reginfo updated
;       EAX = false if the opcode was not emulated.
;
        public VdmOpcodeLmsw
VdmOpcodeLmsw proc

        push    ebp
        mov     ebp,esp
        push    edi
        push    ebx
;
; Check reg field of mod r/m byte to see if this is really an Lmsw
;

        inc     [esi].RiEip
        call    CheckEip
        test    al,0fh
        jz      ol30                    ; gp fault

        mov     edi,[esi].RiEip
        add     edi,[esi].RiCsBase
        movzx   edx,byte ptr [edi]
        and     edx,MI_REGMASK
        cmp     edx,MI_REGLMSW          ; check for the additional opcode bits
        jne     ol40                    ; not lmsw, return error.

;
; Turn off the 32 bit operand prefix (lmsw always uses 16 bit operands)
;
        and     [esi].RiPrefixFlags,NOT PREFIX_OPER32
;
; Get the linear address of the value to put into the MSW.
;
        mov     eax,1                   ; read operand
        call    VdmDecodeOperand

        test    al,0fh
        jz      ol30                    ; error reading operand

        mov     edi,[esi].RiOperand
        mov     eax,[edi]

        call    KiVdmSetUserCR0

        mov     eax,1                   ; indicate success
ol30:   pop     ebx
        pop     edi
        mov     esp,ebp
        pop     ebp
        ret

ol40:   xor     eax,eax
        jmp     ol30

VdmOpcodeLmsw endp

        page ,132
        subttl "Emulate CLTS"
;++
;
;    Routine Description:
;
;       This routine emulates CLTS by ignoring it.
;
;    Arguments:
;
;       esi = pointer to reg info
;
;    Returns:
;
;       eax = 1
;
        public VdmOpcodeClts
VdmOpcodeClts proc
        inc     [esi].RiEip             ; skip second byte
        mov     eax,1
        ret
VdmOpcodeClts endp

        page ,132
        subttl "Emulate GetCRx"
;++
;
;    Routine Description:
;
;       This routine emulates the GetCRx opcodes.  For now, it returns the
;       contents of CR0 unmodified, and 0 for all other CR?
;
;    Arguments:
;
;       esi = pointer to reg info
;
;    Returns:
;
;       eax = true if the opcode was emulated
;               reginfo updated.
;       eax = false if the opcode was not emulated.
;
        public VdmOpcodeGetCrx
VdmOpcodeGetCrx proc

        push    ebp
        mov     ebp,esp
        push    edi

        inc     [esi].RiEip
        call    CheckEip
        test    al,0fh
        jz      ogc40                   ; Gp fault, report error

        mov     edi,[esi].RiEip
        add     edi,[esi].RiCsBase
        inc     [esi].RiEip
        movzx   edx,byte ptr [edi]
;
; Verify the Mod field for mov special
;
        and     edx,MI_MODMASK
        cmp     edx,MI_MODMOVSPEC       ; require for mov to/from special
        jne     short ogc50             ; return error

        movzx   edx,byte ptr [edi]
        test    edx,MI_REGMASK          ; mov from CR0?
        mov     eax,0                   ; assume 0
        jne     short ogc30

        mov     edi,PCR[PcInitialStack]         ; (edi) = fp save area
        mov     edi,[edi].FpCr0NpxState         ; get users bits
        and     edi, CR0_MP+CR0_EM+CR0_PE       ; mask
        movzx   edi,_VdmUserCr0MapOut[edi]      ; map to real settings

        mov     eax, cr0                        ; read CR0
        and     eax, not (CR0_MP+CR0_EM+CR0_TS) ; clear npx bits
        or      eax, edi                        ; set npx bits

;
; Pull the destination register from the R/M field
;
ogc30:  and     edx,MI_RMMASK
        mov     edi,[esi].RiTrapFrame
        mov     edx,RegTab[edx * 4]     ; get register offset
        mov     [edx + edi],eax         ; store CR? contents into register
        mov     eax,1
ogc40:  pop     edi
        mov     esp,ebp
        pop     ebp
        ret

ogc50:  xor     eax,eax
        jmp     ogc40

VdmOpcodeGetCrx endp


        page ,132
        subttl "Emulate SetCRx"
;++
;
;    Routine Description:
;
;       This routine emulates the SetCRx opcodes.  For now, it only emulates
;       set CR0, and only for the same conditions as LMSW.  It causes a fault
;       for all other CR?
;
;    Arguments:
;
;       esi = pointer to reg info
;
;    Returns:
;
;       eax = true if the opcode was emulated
;               reginfo updated.
;       eax = false if the opcode was not emulated.
;
        public VdmOpcodeSetCrx
VdmOpcodeSetCrx proc

        push    ebp
        mov     ebp,esp
        push    edi
        push    ebx

        inc     [esi].RiEip
        call    CheckEip
        test    al,0fh
        jz      osc40                   ; Gp fault, report error

        mov     edi,[esi].RiEip
        add     edi,[esi].RiCsBase
        inc     [esi].RiEip
        movzx   edx,byte ptr [edi]
;
; Verify the Mod field for mov special
;
        and     edx,MI_MODMASK
        cmp     edx,MI_MODMOVSPEC       ; require for mov to/from special
        jne     osc50                   ; return error

        movzx   edx,byte ptr [edi]
        test    edx,MI_REGMASK          ; mov to CR0?
        jne     osc50                   ; no, return error

;
; Get the source register from the R/M field
;
        and     edx,MI_RMMASK
        mov     edi,[esi].RiTrapFrame
        mov     edx,RegTab[edx * 4]     ; get register offset
        mov     eax,[edx + edi]         ; get CR? contents from register

        call    KiVdmSetUserCR0

        mov     eax,1
osc40:  pop     edi
        mov     esp,ebp
        pop     ebp
        ret

osc50:  xor     eax,eax
        jmp     osc40

VdmOpcodeSetCrx endp

        page ,132
        subttl "Emulate GetDRx"
;++
;
;    Routine Description:
;
;       This routine emulates the GetDRx opcodes.  For DR0-DR3, it returns the
;       values from the user mode trap frame.  For DR4-DR5, it returns 0.  For
;       DR6 and DR7, it returns the bits from the user mode trap frame that are
;       settable from user mode.
;
;    Arguments:
;
;       esi = pointer to reg info
;
;    Returns:
;
;       eax = true if the opcode was emulated
;               reginfo updated.
;       eax = false if the opcode was not emulated.
;
        public VdmOpcodeGetDrx
VdmOpcodeGetDrx proc

ModRm   equ     [ebp - 4]

        push    ebp
        mov     ebp,esp
        sub     esp,4
        push    edi
        push    ecx

        inc     [esi].RiEip
        call    CheckEip
        test    al,0fh
        jz      ogd50                   ; Gp fault, report error

        mov     edi,[esi].RiEip
        add     edi,[esi].RiCsBase
        inc     [esi].RiEip
        movzx   edx,byte ptr [edi]
        mov     ModRm,edx
;
; Verify the Mod field
;
        and     edx,MI_MODMASK
        cmp     edx,MI_MODMOVSPEC       ; require for mov special
        jne     ogd60                   ; return error

        mov     edi,[esi].RiTrapFrame
;
; Get the Dr number
;
        mov     edx,ModRm
        and     edx,MI_REGMASK
        shr     edx,MI_REGSHIFT
;
; If it is DR4 or DR5, set the destination register to zero
;
        cmp     edx,4
        jb      ogd20

        cmp     edx,5
        ja      ogd20

        mov     eax,0                   ; set the destination to zero
        jmp     ogd40
;
; Otherwise, use the actual user mode register contents
;
ogd20:  mov     ecx,DrTab[edx * 4]
        mov     eax,[edi + ecx]         ; get register value
        cmp     edx,6                   ; do we need to sanitize?
        jb      ogd40                   ; no.

        cmp     edx,7
        je      ogd30

        and     eax,DR6_LEGAL
        jmp     ogd40

ogd30:  and     eax,DR7_LEGAL
;
; Get destination register from opcode, and store Dr contents
;
ogd40:  mov     edx,ModRm
        and     edx,MI_RMMASK
        mov     ecx,RegTab[edx * 4]
        mov     [ecx + edi],eax         ; set user mode register

        mov     eax,1                   ; indicate success
ogd50:  pop     ecx
        pop     edi
        mov     esp,ebp
        pop     ebp
        ret

ogd60:  xor     eax,eax                 ; indicate error
        jmp     ogd50

VdmOpcodeGetDrx endp

        page ,132
        subttl "Emulate SetDRx"
;++
;
;    Routine Description:
;
;       This routine emulates the SetDRx opcodes.  For DR0-DR3, it will set
;       the user mode register to any linear address up to 16MB.  For DR4-DR5,
;       it will ignore the instruction.  For DR6 and DR7, it will set the bits
;       that are user mode setable.  For DR7, it also translates the global
;       bits to local bits.
;
;    Arguments:
;
;       esi = pointer to reg info
;
;    Returns:
;
;       eax = true if the opcode was emulated
;               reginfo updated.
;       eax = false if the opcode was not emulated.
;
        public VdmOpcodeSetDrx
VdmOpcodeSetDrx proc

ModRm   equ     [ebp - 4]

        push    ebp
        mov     ebp,esp
        sub     esp,4
        push    edi
        push    ecx

        inc     [esi].RiEip
        call    CheckEip
        test    al,0fh
        jz      osd80                   ; Gp fault, report error

        mov     edi,[esi].RiEip
        add     edi,[esi].RiCsBase
        inc     [esi].RiEip
        movzx   edx,byte ptr [edi]
        mov     ModRm,edx
;
; Verify the Mod field
;
        and     edx,MI_MODMASK
        cmp     edx,MI_MODMOVSPEC       ; require for mov special
        jne     osd90                   ; return error
;
; Get value to put into debug register
;
        mov     edi,[esi].RiTrapFrame
        mov     edx,ModRm
        and     edx,MI_RMMASK
        mov     edx,RegTab[edx * 4]
        mov     eax,[edi + edx]
;
; Determine which debug register it goes into, and sanitize appropriately
;
        mov     edx,ModRm
        and     edx,MI_REGMASK
        shr     edx,MI_REGSHIFT
        cmp     edx,4
        jnb     osd20
;
; DR0-DR4, make linear address < 16MB
;
        and     eax,MAX_VDM_ADDR
        jmp     osd60

osd20:  cmp     edx,6
        jnb     osd30
;
; DR4-DR5, just return
;
        jmp     osd70                   ; return success

osd30:  cmp     edx,6
        jne     osd40

;
; DR6, mask for legal bits
;
        and     eax,DR6_LEGAL
        jmp     osd60

;
; DR7, mask for legal bits, and translate g to l, and activate debug
;
osd40:  mov     ecx,eax
        and     ecx,DR7_GLOBAL
        shr     ecx,1                   ; translate the G to L by shifting
        or      eax,ecx                 ; put in any new bits
        and     eax,DR7_LEGAL           ; sanitize the value

;
; Put the value into the user mode register
;
osd60:  mov     ecx,DrTab[edx * 4]
        mov     [edi + ecx],eax
        cmp     edx,7
        jne     osd70

        mov     ecx,eax
        stdCall _KeGetCurrentThread
        test    ecx,DR7_ACTIVE
        jz      osd65

        mov     byte ptr [eax].ThDebugActive,1   ; set debugging active for this thread
        jmp     osd70

osd65:  mov     byte ptr [eax].ThDebugActive,0
osd70:  mov     eax,1
osd80:  pop     ecx
        pop     edi
        mov     esp,ebp
        pop     ebp
        ret

osd90:  xor     eax,eax
        jmp     osd80

VdmOpcodeSetDrx endp

_PAGE ends

      end
