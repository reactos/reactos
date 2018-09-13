        title "Operand Decoding"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    vdmoprnd.asm
;
;Abstract:
;
;    This module contains support for decoding 386/486 instruction operands.
;    This is used by the opcode 0f emulation.
;
;
;Author:
;
;    Dave Hastings (daveh) 20-Feb-1992
;
;Notes:
;
;    The only instruction which uses the operand decodeing (3/10/92) is
;    LMSW.  This instruction only has 16 bit operands, so only the 16 bit
;    operand decode has been tested.  The 32 bit decode will be tested
;    (or removed?) during clean up, after code freeze.
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
include vdmtib.inc

        page ,132

_PAGE   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING

        EXTRNP  _Ki386GetSelectorParameters,4
        extrn   CheckEip:proc

_PAGE   ENDS

_DATA	SEGMENT  DWORD PUBLIC 'DATA'
;
; This table is used to dispatch base on the mod code for 16 bit address size.
; When these locations are dispatched to,
;       edi = linear address of next byte of instruction
;       esi = pointer to register info
;       ecx = R/M value for the instruction
modtab16 dd offset FLAT:do20            ; no displacement
         dd offset FLAT:do40            ; 8 bit displacement
         dd offset FLAT:do50            ; 16 bit displacement
         dd offset FLAT:do60            ; Register operand

;
; This table is used to dispatch based on the RM code for 16 bit address size.
; When these locations are dispatched to,
;       edi = pointer to trap frame
;       esi = pointer to register info
;       ebx = partial linear address of operand
rmtab16 dd offset FLAT:do70             ; [bx + si]
        dd offset FLAT:do80             ; [bx + di]
        dd offset FLAT:do90             ; [bp + si]
        dd offset FLAT:do100            ; [bp + di]
        dd offset FLAT:do95             ; [si]
        dd offset FLAT:do85             ; [di]
        dd offset FLAT:do105            ; [bp]
        dd offset FLAT:do75             ; [bx]

;
; This table is used to dispatch base on the mod code for 32 bit address size.
; When these locations are dispatched to,
;       edi = linear address of next byte of instruction
;       esi = pointer to register info
;       ecx = R/M value for the instruction
modtab32 dd offset FLAT:do220           ; no displacement
         dd offset FLAT:do240           ; 8 bit displacement
         dd offset FLAT:do250           ; 32 bit displacement
         dd offset FLAT:do260           ; Register operand

;
; This table is used to pick up register offsets in the trap frame.
; N.B.  This table cannot be used to find byte registers!!
;
        public RegTab
RegTab  dd TsEax
        dd TsEcx
        dd TsEdx
        dd TsEbx
        dd TsHardwareEsp
        dd TsEbp
        dd TsEsi
        dd TsEdi


_DATA   ENDS


_PAGE   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME DS:FLAT, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

        page ,132
        subttl "Decode Operands"
;++
;
;   Routine Description:
;
;       This routine decodes the operands for 386/486 instructions.  It returns
;       the linear address of the operand.  For register operands, this is
;       an address in the stack frame.  The read/write flag is used for
;       operand verification.
;
;   Arguments:
;
;       esi = address of reg info
;       eax = 1 -- Read of operand
;             0 -- Write of operand
;
;   Returns:
;
;       eax = True -- operand ok, and reg info operand field filled in
;       eax = False -- operand not ok.
;       reg info eip updated
;
;   Notes:
;
;       This routine DOES NOT decode the reg field of the mod r/m byte of the
;       opcode.  This is not a problem because it will only return one operand
;       address anyway.  It does not decode byte registers correctly!!.
;
;       check grow down ss handling

        public VdmDecodeOperand
VdmDecodeOperand proc

SegBase equ     [ebp] - 04h
SegLimit equ    [ebp] - 08h
SegFlags equ    [ebp] - 0ch
SelLookupResult equ [ebp] - 010h
ReadWrite equ   [ebp] - 014h
ModRm   equ     [ebp] - 018h
SIB     equ     [ebp] - 01ch

        push    ebp
        mov     ebp,esp
        sub     esp,01ch
        push    edi
        push    ecx
        push    ebx

        mov     ReadWrite,eax

;
; Get the info on DS (assumed default selector)
;
        lea     edx,SegLimit
        push    edx
        lea     edx,SegBase
        push    edx
        lea     edx,SegFlags
        push    edx
        mov     edi,[esi].RiTrapFrame
        push    [edi].TsSegDs
        call    VdmSegParams
        add     esp,010h
        mov     SelLookupResult,eax     ; check result after override check

        mov     edi,[esi].RiEip
        add     edi,[esi].RiCsBase
        call    CheckEip
        test    al,0fh
        jz      do200                   ; Gp fault, report error

        movzx   edx,byte ptr [edi]      ; get mod r/m byte
        inc     [esi].RiEip
        inc     edi
        mov     ecx,edx
        mov     ModRm,edx
        and     edx,MI_MODMASK
        shr     edx,MI_MODSHIFT         ; form jump table index from mod
        and     ecx,MI_RMMASK           ; form index for RM jump table
        test    [esi].RiPrefixFlags,PREFIX_ADDR32
        ; 32 bit segments.
        jnz     do210                   ; 32 bit instructions have diff form

        jmp     modtab16[edx * 4]

do20:
;
; These forms have no displacement, except for the "bp" form, which
; is just a 16 bit immediate displacement
;
        mov     ebx,0                   ; assume no displacement
        cmp     ecx,MI_RMBP
        jne     do30                    ; dispatch through jmp table

        call    CheckEip
        test    al,0fh
        jz      do200                   ; Gp fault, report error

        movzx   ebx,word ptr [edi]      ; get displacement
        inc     [esi].RiEip             ; update eip
        inc     [esi].RiEip
        jmp     do120                   ; go add in seg

do30:   mov     edi,[esi].RiTrapFrame
        jmp     rmtab16[ecx * 4]        ; go get register info.

do40:
;
; These forms have an 8 bit displacement
;
        call    CheckEip
        test    al,0fh
        jz      do200                   ; Gp fault, report error

        movsx   ebx,byte ptr [edi]
        inc     [esi].RiEip
        mov     edi,[esi].RiTrapFrame
        jmp     rmtab16[ecx * 4]

do50:
;
; These forms have an 16 bit displacement
;
        call    CheckEip
        test    al,0fh
        jz      do200                   ; Gp fault, report error

        movzx   ebx,word ptr [edi]
        inc     [esi].RiEip
        inc     [esi].RiEip
        mov     edi,[esi].RiTrapFrame
        jmp     rmtab16[ecx * 4]

do60:
;
; These forms are register operands
;
        mov     ebx,RegTab[ecx * 4]     ; get offset into stackframe
        add     ebx,[esi].RiTrapFrame   ; form linear address
        jmp     do194                   ; return success

do70:
;
; This is the [bx + si] operand
;
        movzx   edx,word ptr [edi].TsEsi
        add     ebx,edx

do75:
;
; This is the [bx] operand, and a fall through to finish forming [bx + si]
;
        movzx   edx,word ptr [edi].TsEbx
        add     ebx,edx
        jmp     do120                   ; go add seg info

do80:
;
; This is the [bx + di] operand
;
        movzx   edx,word ptr [edi].TsEbx
        add     ebx,edx

do85:
;
; This is the [di] operand, and the fall through to finish [bx + di]
;
        movzx   edx,word ptr [edi].TsEdi
        add     ebx,edx
        jmp     do120                   ; go add seg info

do90:
;
; This is the [bp + si] operand
;
        movzx   edx,word ptr [edi].TsEbp
        add     ebx,edx
;
; Change default segment to be ss
;
        lea     edx,SegLimit
        push    edx
        lea     edx,SegBase
        push    edx
        lea     edx,SegFlags
        push    edx
        mov     edi,[esi].RiTrapFrame
        push    [edi].TsHardwareSegSs
        call    VdmSegParams
        add     esp,010h
        mov     SelLookupResult,eax

do95:
;
; This is the [si] operand, and the fall through for forming [bp + si]
;
        movzx   edx,word ptr [edi].TsEsi
        add     ebx,edx
        jmp     do120                   ; go add seg info

do100:
;
; This is the [bp + di] operand
;
        movzx   edx,word ptr [edi].TsEdi
        add     ebx,edx

do105:
;
; This is the [bp] operand, and the fall through for forming [bp + di]
;
        movzx   edx,word ptr [edi].TsEbp
        add     ebx,edx
;
; Change default segment to be SS
;
        lea     edx,SegLimit
        push    edx
        lea     edx,SegBase
        push    edx
        lea     edx,SegFlags
        push    edx
        mov     edi,[esi].RiTrapFrame
        push    [edi].TsHardwareSegSs
        call    VdmSegParams
        add     esp,010h
        mov     SelLookupResult,eax

do120:  test    [esi].RiPrefixFlags,PREFIX_SEG_ALL  ; check for seg prefixes
        jz      do190                   ; no prefixes, use default.

        ; Note: we could use a bsr instruction here, but it has a high
        ;       overhead relative to a test and a jump, and I expect that
        ;       es overrides will be by far the most common
        mov     edi,[esi].RiTrapFrame
        test    [esi].RiPrefixFlags,PREFIX_ES
        jz      do130

        movzx   edx,word ptr [edi].TsSegEs
        jmp     do180

do130:  test    [esi].RiPrefixFlags,PREFIX_CS
        jz      do140

        movzx   edx,word ptr [edi].TsSegCs
        jmp     do180

do140:  test    [esi].RiPrefixFlags,PREFIX_SS
        jz      do150

        movzx   edx,word ptr [edi].TsHardwareSegSs
        jmp     do180

do150:  test    [esi].RiPrefixFlags,PREFIX_DS
        jz      do160

        movzx   edx,word ptr [edi].TsSegDs
        jmp     do180

do160:  test    [esi].RiPrefixFlags,PREFIX_FS
        jz      do170

        movzx   edx,word ptr [edi].TsSegFs
        jmp     do180

do170:  ; assert that seg gs bit is set
        movzx   edx,word ptr [edi].TsSegGs

;
; Get information on new default segment
;
do180:  lea     ecx,SegLimit
        push    ecx
        lea     ecx,SegBase
        push    ecx
        lea     ecx,SegFlags
        push    ecx
        push    edx
        call    VdmSegParams
        add     esp,010h
        mov     SelLookupResult,eax

        test    byte ptr SelLookupResult,0fh
        jz      do200                                   ; return error

        cmp     dword ptr ReadWrite,0
        jnz     do190                                   ; we can read all sels

        test    dword ptr SegFlags,SEL_TYPE_WRITE
        jz      do200                                   ; return error.

        cmp     ebx,SegLimit
        jae     do200                                   ; gp fault

do190:  add     ebx,SegBase
do194:  mov     [esi].RiOperand,ebx                     ; update op pointer
        mov     eax,1
do195:  pop     ebx
        pop     ecx
        pop     edi
        mov     esp,ebp
        pop     ebp
        ret

do200:  xor     eax,eax
        jmp     do195

;
; Get the SIB if there is one, and save it for later.
;
do210:  cmp     ecx,MI_RMSIB
        jne     do215                   ; no Sib, dispatch for displacement

        call    CheckEip
        test    al,0fh
        jz      do200                   ; report GP fault

        movzx   eax,byte ptr [edi]
        mov     Sib,eax
        inc     edi
        inc     [esi].RiEip
do215:  jmp     modtab32[edx * 4]

do220:
;
; These forms have no displacement, except for the "bp" form, which
; is just a 32 bit immediate displacement
;
        mov     ebx,0                   ; assume no displacement
        cmp     ecx,MI_RMBP
        jne     do270

        call    CheckEip
        test    al,0fh
        jz      do200                   ; Gp fault, report error

        mov     ebx,[edi]               ; get displacement
        add     [esi].RiEip,4           ; update eip
        jmp     do120                   ; go add in seg

do240:
;
; These forms have an 8 bit displacement
;
        call    CheckEip
        test    al,0fh
        jz      do200                   ; Gp fault, report error

        movsx   ebx,byte ptr [edi]
        inc     [esi].RiEip
        jmp     do270

do250:
;
; These forms have an 32 bit displacement
;
        call    CheckEip
        test    al,0fh
        jz      do200                   ; Gp fault, report error

        mov     ebx, [edi]
        add     [esi].RiEip,4
        jmp     do270

do260:
;
; These forms are register operands
;
        mov     ebx,RegTab[ecx * 4]     ; get offset into stackframe
        add     ebx,[esi].RiTrapFrame   ; form linear address
        jmp     do195                   ; return success

do270:
;
; Add in the RM portion of the effective address.
;
        cmp     ecx,MI_RMSIB
        je      do290                   ; handle SIB specially

        mov     edi,[esi].RiTrapFrame
        mov     edx,RegTab[ecx * 4]    ; get offset of register
        add     ebx,[edx+edi]           ; add register to displacement
        cmp     ecx,MI_RMBP             ; bp is base?
        je      do280                   ; set up ss as default

        jmp     do120                   ; get segment info.

do280:
;
; Change default segment to be SS
;
        lea     edx,SegLimit
        push    edx
        lea     edx,SegBase
        push    edx
        lea     edx,SegFlags
        push    edx
        mov     edi,[esi].RiTrapFrame
        push    [edi].TsHardwareSegSs
        call    VdmSegParams
        add     esp,010h
        mov     SelLookupResult,eax
        jmp     do120
do290:
;
;  Decode the Sib
;
        mov     edx,Sib
        mov     edi,[esi].RiTrapFrame
        and     edx,MI_SIB_BASEMASK     ; isolate base
        cmp     edx,MI_SIB_BASENONE     ; no base
        je      do300

        mov     eax,RegTab[edx * 4]
        add     ebx,[edi+eax]           ; get register contents, and add

do300:  mov     edx,Sib
        and     ecx,MI_SIB_INDEXMASK
        shr     ecx,MI_SIB_INDEXSHIFT   ; make index out of "index" field
        cmp     ecx,MI_SIB_INDEXNONE
        je      do310                   ; no index

        mov     eax,RegTab[ecx * 4]
        mov     eax,[eax+edi]           ; get reg contents for multiply.
        mov     ecx,Sib
        and     ecx,MI_SIB_SSMASK
        shr     ecx,MI_SIB_SSSHIFT      ; for shift count
        shl     eax,cl
        add     ebx,eax

do310:  cmp     edx,MI_SIB_BASENONE
        jne     do120

;
; If mod != 0, then we have to add in EBP, and make ss the default seg
;
        mov     edx,ModRm
        and     edx,MI_MODMASK
        shr     edx,MI_MODSHIFT
        cmp     edx,MI_MODNONE
        jne     do120
;
; Add in Ebp, and change default segment to ss
;
        add     ebx,[edi].TsEbp

        lea     edx,SegLimit
        push    edx
        lea     edx,SegBase
        push    edx
        lea     edx,SegFlags
        push    edx
        mov     edi,[esi].RiTrapFrame
        push    [edi].TsHardwareSegSs
        call    VdmSegParams
        add     esp,010h
        mov     SelLookupResult,eax
        jmp     do120                   ; add in segment info

VdmDecodeOperand endp

        public VdmSegParams
VdmSegParams proc

        push    edi
        mov     edi,[esi].RiTrapFrame
        test    dword ptr [edi].TsEFlags,EFLAGS_V86_MASK
        jz      vsp20

Segmt   equ     word ptr [ebp + 8]
SegFlags equ    [ebp + 0Ch]
SegBase equ     [ebp + 010h]
SegLimit equ    [ebp + 014h]

        pop     edi
        push    ebp
        mov     ebp,esp
        push    edi

        movzx   eax,Segmt
        shl     eax,4
        mov     edi,SegBase
        mov     [edi],eax
        mov     edi,SegLimit
        mov     dword ptr [edi],0FFFFh
        mov     edi,SegFlags
        mov     [edi],dword ptr SEL_TYPE_WRITE
        mov     eax,1

        pop     edi
        mov     esp,ebp
        pop     ebp
        ret

vsp20:  pop     edi
IFDEF STD_CALL
        jmp     _Ki386GetSelectorParameters@16
ELSE
        jmp     _Ki386GetSelectorParameters
ENDIF

VdmSegParams endp
_PAGE   ENDS
        end
