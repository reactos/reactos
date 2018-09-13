        title  "Processor type and stepping detection"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    cpu.asm
;
; Abstract:
;
;    This module implements the assembley code necessary to determine
;    cpu type and stepping information.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 28-Oct-1991.
;        Some of the code is extracted from Cruiser (mainly,
;        the code to determine 386 stepping.)
;
; Environment:
;
;    80x86
;
; Revision History:
;
;--

        .xlist
include i386\cpu.inc
include ks386.inc
include callconv.inc
include mac386.inc
        .list

;
; constant for i386 32-bit multiplication test
;

MULTIPLIER            equ     00000081h
MULTIPLICAND          equ     0417a000h
RESULT_HIGH           equ     00000002h
RESULT_LOW            equ     0fe7a000h

;
; Constants for Floating Point test
;

REALLONG_LOW          equ     00000000
REALLONG_HIGH         equ     3FE00000h
PSEUDO_DENORMAL_LOW   equ     00000000h
PSEUDO_DENORMAL_MID   equ     80000000h
PSEUDO_DENORMAL_HIGH  equ     0000h

.586p

INIT    SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


;++
;
; USHORT
; KiSetProcessorType (
;    VOID
;    )
;
; Routine Description:
;
;    This function determines type of processor (80486, 80386),
;    and it's corrisponding stepping.  The results are saved in
;    the current processor's PRCB.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Prcb->CpuType
;       3, 4, 5, ...    3 = 386, 4 = 486, etc..
;
;    Prcb->CpuStep is encoded as follows:
;       lower byte as stepping #
;       upper byte as stepping letter (0=a, 1=b, 2=c, ...)
;
;    (ax) = x86h or 0 if unrecongnized processor.
;
;--
cPublicProc _KiSetProcessorType,0

        mov     byte ptr fs:PcPrcbData.PbCpuID, 0

        push    edi
        push    esi
        push    ebx                     ; Save C registers
        mov     eax, cr0
        push    eax
        pushfd                          ; save Cr0 & flags

        pop     ebx                     ; Get flags into eax
        push    ebx                     ; Save original flags

        mov     ecx, ebx
        xor     ecx, EFLAGS_ID          ; flip ID bit
        push    ecx
        popfd                           ; load it into flags
        pushfd                          ; re-save flags
        pop     ecx                     ; get flags into eax
        cmp     ebx, ecx                ; did bit stay flipped?
        jne     short cpu_has_cpuid     ; Yes, go use CPUID

cpuid_unsupported:
        pop     ebx                     ; Get flags into eax
        push    ebx                     ; Save original flags

        mov     ecx, ebx
        xor     ecx, EFLAGS_AC          ; flip AC bit
        push    ecx
        popfd                           ; load it into flags
        pushfd                          ; re-save flags
        pop     ecx                     ; get flags into eax
        cmp     ebx, ecx                ; did bit stay flipped?
        je      short cpu_is_386        ; No, then this is a 386

cpu_is_486:
        mov     byte ptr fs:PcPrcbData.PbCpuType, 4h    ; Save CPU Type
        call    Get486Stepping
        jmp     cpu_save_stepping

cpu_is_386:
        mov     byte ptr fs:PcPrcbData.PbCpuType, 3h    ; Save CPU Type
        call    Get386Stepping
        jmp     cpu_save_stepping

cpu_has_cpuid:
        or      ebx, EFLAGS_ID
        push    ebx
        popfd                           ; Make sure ID bit is set

        mov     ecx, fs:PcIdt           ; Address of IDT
        push    dword ptr [ecx+30h]     ; Save Trap06 handler incase
        push    dword ptr [ecx+34h]     ; the CPUID instruction faults

        mov     eax, offset CpuIdTrap6Handler
        mov     word ptr [ecx+30h], ax  ; Set LowWord
        shr     eax, 16
        mov     word ptr [ecx+36h], ax  ; Set HighWord

        mov     eax, 0                  ; argument to CPUID
        cpuid                           ; Uses eax, ebx, ecx, edx

        mov     ecx, fs:PcIdt           ; Address of IDT
        pop     dword ptr [ecx+34h]     ; restore trap6 handler
        pop     dword ptr [ecx+30h]

        cmp     eax, 1                  ; make sure level 1 is supported
        jc      short cpuid_unsupported ; no, then punt


        ; Get the family and stepping (cpuid fn=1).  Format returned is
        ; 3          2         1          
        ; 10987654321098765432109876543210
        ; --------------------------------
        ;                   ppffffmmmmssss
        ; where
        ;    pp = Processor Type 
        ;  ffff = Family
        ;  mmmm = Model
        ;  ssss = Stepping
        ;
        ; This is transformed and saved in the PRCB as
        ; 
        ; PRCB->CpuStep = 0000mmmm0000ssss                v v
        ;     ->CpuID   = 00000001                        | | v
        ;     ->CpuType = 0000ffff                        | | | v
        ;                                                 | | | |
        ; ie the dword that contains all this looks like 0M0S010F
        ;

        mov     eax, 1                  ; get the family and stepping
        cpuid

        mov     ebx, eax

        and     eax, 0F0h               ; (eax) = Model
        shl     eax, 4
        mov     al, bl
        and     eax, 0F0Fh              ; (eax) = Model[15:8] | Step[7:0]

        and     ebx, 0F00h              ; (bh) = CpuType

        mov     byte ptr fs:PcPrcbData.PbCpuID, 1       ; Has ID support
        mov     byte ptr fs:PcPrcbData.PbCpuType, bh    ; Save CPU Type

cpu_save_stepping:
        mov     word ptr fs:PcPrcbData.PbCpuStep, ax    ; Save CPU Stepping
        popfd                                   ; Restore flags
        pop     eax
        mov     cr0, eax
        pop     ebx
        pop     esi
        pop     edi
        stdRET  _KiSetProcessorType

cpuid_trap:
        mov     ecx, fs:PcIdt           ; Address of IDT
        pop     dword ptr [ecx+34h]     ; restore trap6 handler
        pop     dword ptr [ecx+30h]
        jmp     cpuid_unsupported       ; Go get processor information

stdENDP _KiSetProcessorType

;++
;
; BOOLEAN
; CpuIdTrap6 (
;    VOID
;    )
;
; Routine Description:
;
;    Temporary int 6 handler - assumes the cause of the exception was the
;    attempted CPUID instruction.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    none.
;
;--

CpuIdTrap6Handler   proc

        mov     [esp].IretEip,offset cpuid_trap
        iretd

CpuIdTrap6Handler  endp


;++
;
; USHORT
; Get386Stepping (
;    VOID
;    )
;
; Routine Description:
;
;    This function determines cpu stepping for i386 CPU stepping.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    [ax] - Cpu stepping.
;           0 = A, 1 = B, 2 = C, ...
;
;--

        public  Get386Stepping
Get386Stepping  proc

        call    MultiplyTest            ; Perform mutiplication test
        jnc     short G3s00             ; if nc, muttest is ok
        mov     ax, 0
        ret
G3s00:
        call    Check386B0              ; Check for B0 stepping
        jnc     short G3s05             ; if nc, it's B1/later
        mov     ax, 100h                ; It is B0/earlier stepping
        ret

G3s05:
        call    Check386D1              ; Check for D1 stepping
        jc      short G3s10             ; if c, it is NOT D1
        mov     ax, 301h                ; It is D1/later stepping
        ret

G3s10:
        mov     ax, 101h                ; assume it is B1 stepping
        ret

Get386Stepping  endp

;++
;
; USHORT
; Get486Stepping (
;    VOID
;    )
;
; Routine Description:
;
;    This function determines cpu stepping for i486 CPU type.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    [ax] - Cpu stepping.  For example, [ax] = D0h for D0 stepping.
;
;--

        public  Get486Stepping
Get486Stepping          proc

        call    Check486AStepping       ; Check for A stepping
        jnc     short G4s00             ; if nc, it is NOT A stepping

        mov     ax, 0                   ; set to A stepping
        ret

G4s00:  call    Check486BStepping       ; Check for B stepping
        jnc     short G4s10             ; if nc, it is NOT a B stepping

        mov     ax, 100h                ; set to B stepping
        ret

;
; Before we test for 486 C/D step, we need to make sure NPX is present.
; Because the test uses FP instruction to do the detection.
;
G4s10:
        call    _KiIsNpxPresent         ; Check if cpu has coprocessor support?
        or      ax, ax
        jz      short G4s15             ; it is actually 486sx

        call    Check486CStepping       ; Check for C stepping
        jnc     short G4s20             ; if nc, it is NOT a C stepping
G4s15:
        mov     ax, 200h                ; set to C stepping
        ret

G4s20:  mov     ax, 300h                ; Set to D stepping
        ret

Get486Stepping          endp

;++
;
; BOOLEAN
; Check486AStepping (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 486 A Stepping.
;
;    It takes advantage of the fact that on the A-step of the i486
;    processor, the ET bit in CR0 could be set or cleared by software,
;    but was not used by the hardware.  On B or C -step, ET bit in CR0
;    is now hardwired to a "1" to force usage of the 386 math coprocessor
;    protocol.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if B or later stepping.
;    Carry Flag set if A or earlier stepping.
;
;--
        public  Check486AStepping
Check486AStepping       proc    near
        mov     eax, cr0                ; reset ET bit in cr0
        and     eax, NOT CR0_ET
        mov     cr0, eax

        mov     eax, cr0                ; get cr0 back
        test    eax, CR0_ET             ; if ET bit still set?
        jnz     short cas10             ; if nz, yes, still set, it's NOT A step
        stc
        ret

cas10:  clc
        ret
Check486AStepping       endp

;++
;
; BOOLEAN
; Check486BStepping (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 486 B Stepping.
;
;    On the i486 processor, the "mov to/from DR4/5" instructions were
;    aliased to "mov to/from DR6/7" instructions.  However, the i486
;    B or earlier steps generate an Invalid opcode exception when DR4/5
;    are used with "mov to/from special register" instruction.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if C or later stepping.
;    Carry Flag set if B stepping.
;
;--
        public  Check486BStepping
Check486BStepping       proc

        push    ebx

        mov     ebx, fs:PcIdt           ; Address of IDT
        push    dword ptr [ebx+30h]
        push    dword ptr [ebx+34h]     ; Save Trap06 handler

        mov     eax, offset Temporary486Int6
        mov     word ptr [ebx+30h], ax  ; Set LowWord
        shr     eax, 16
        mov     word ptr [ebx+36h], ax  ; Set HighWord

c4bs50: db      0fh, 21h, 0e0h          ; mov eax, DR4
        nop
        nop
        nop
        nop
        nop
        clc                             ; it is C step
        jmp     short c4bs70
c4bs60: stc                             ; it's B step
c4bs70: pop     dword ptr [ebx+34h]     ; restore old int 6 vector
        pop     dword ptr [ebx+30h]

        pop     ebx
        ret

        ret

Check486BStepping       endp

;++
;
; BOOLEAN
; Temporary486Int6 (
;    VOID
;    )
;
; Routine Description:
;
;    Temporary int 6 handler - assumes the cause of the exception was the
;    attempted execution of an mov to/from DR4/5 instruction.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    none.
;
;--

Temporary486Int6        proc

        mov     [esp].IretEIp,offset c4bs60 ; set EIP to stc instruction
        iretd

Temporary486Int6        endp

;++
;
; BOOLEAN
; Check486CStepping (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 486 C Stepping.
;
;    This routine takes advantage of the fact that FSCALE produces
;    wrong result with Denormal or Pseudo-denormal operand on 486
;    C and earlier steps.
;
;    If the value contained in ST(1), second location in the floating
;    point stack, is between 1 and 11, and the value in ST, top of the
;    floating point stack, is either a pseudo-denormal number or a
;    denormal number with the underflow exception unmasked, the FSCALE
;    instruction produces an incorrect result.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if D or later stepping.
;    Carry Flag set if C stepping.
;
;--

FpControl       equ     [ebp - 2]
RealLongSt1     equ     [ebp - 10]
PseudoDenormal  equ     [ebp - 20]
FscaleResult    equ     [ebp - 30]

        public  Check486CStepping
Check486CStepping       proc

        push    ebp
        mov     ebp, esp
        sub     esp, 30                 ; Allocate space for temp real variables

        mov     eax, cr0                ; Don't trap while doing math
        and     eax, NOT (CR0_ET+CR0_MP+CR0_TS+CR0_EM)
        mov     cr0, eax

;
; Initialize the local FP variables to predefined values.
; RealLongSt1 = 1.0 * (2 ** -1) = 0.5 in normalized double precision FP form
; PseudoDenormal =  a unsupported format by IEEE.
;                   Sign bit = 0
;                   Exponent = 000000000000000B
;                   Significand = 100000...0B
; FscaleResult = The result of FSCALE instruction.  Depending on 486 step,
;                the value will be different:
;                Under C and earlier steps, 486 returns the original value
;                in ST as the result.  The correct returned value should be
;                original significand and an exponent of 0...01.
;

        mov     dword ptr RealLongSt1, REALLONG_LOW
        mov     dword ptr RealLongSt1 + 4, REALLONG_HIGH
        mov     dword ptr PseudoDenormal, PSEUDO_DENORMAL_LOW
        mov     dword ptr PseudoDenormal + 4, PSEUDO_DENORMAL_MID
        mov     word ptr PseudoDenormal + 8, PSEUDO_DENORMAL_HIGH

.387
        fnstcw  FpControl               ; Get FP control word
        fwait
        or      word ptr FpControl, 0FFh ; Mask all the FP exceptions
        fldcw   FpControl               ; Set FP control

        fld     qword ptr RealLongSt1   ; 0 < ST(1) = RealLongSt1 < 1
        fld     tbyte ptr PseudoDenormal; Denormalized operand. Note, i486
                                        ; won't report denormal exception
                                        ; on 'FLD' instruction.
                                        ; ST(0) = Extended Denormalized operand
        fscale                          ; try to trigger 486Cx errata
        fstp    tbyte ptr FscaleResult  ; Store ST(0) in FscaleResult
        cmp     word ptr FscaleResult + 8, PSEUDO_DENORMAL_HIGH
                                        ; Is Exponent changed?
        jz      short c4ds00            ; if z, no, it is C step
        clc
        jmp     short c4ds10
c4ds00: stc
c4ds10: mov     esp, ebp
        pop     ebp
        ret

Check486CStepping       endp

;++
;
; BOOLEAN
; Check386B0 (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 386 B0 or earlier stepping.
;
;    It takes advantage of the fact that the bit INSERT and
;    EXTRACT instructions that existed in B0 and earlier versions of the
;    386 were removed in the B1 stepping.  When executed on the B1, INSERT
;    and EXTRACT cause an int 6 (invalid opcode) exception.  This routine
;    can therefore discriminate between B1/later 386s and B0/earlier 386s.
;    It is intended to be used in sequence with other checks to determine
;    processor stepping by exercising specific bugs found in specific
;    steppings of the 386.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if B1 or later stepping
;    Carry Flag set if B0 or prior
;
;--

Check386B0      proc

        push    ebx

        mov     ebx, fs:PcIdt           ; Address of IDT
        push    dword ptr [ebx+30h]
        push    dword ptr [ebx+34h]     ; Save Trap06 handler

        mov     eax, offset TemporaryInt6
        mov     word ptr [ebx+30h], ax  ; Set LowWord
        shr     eax, 16
        mov     word ptr [ebx+36h], ax  ; Set HighWord


;
; Attempt execution of Extract Bit String instruction.  Execution on
; B0 or earlier with length (CL) = 0 will return 0 into the destination
; (CX in this case).  Execution on B1 or later will fail either due to
; taking the invalid opcode trap, or if the opcode is valid, we don't
; expect CX will be zeroed by any new instruction supported by newer
; steppings.  The dummy int 6 handler will clears the Carry Flag and
; returns execution to the appropriate label.  If the instruction
; actually executes, CX will *probably* remain unchanged in any new
; stepping that uses the opcode for something else.  The nops are meant
; to handle newer steppings with an unknown instruction length.
;

        xor     eax,eax
        mov     edx,eax
        mov     ecx,0ff00h              ; Extract length (CL) == 0, (CX) != 0

b1c50:  db      0fh, 0a6h, 0cah         ; xbts cx,dx,ax,cl
        nop
        nop
        nop
        nop
        nop
        stc                             ; assume B0
        jecxz    short b1c70            ; jmp if B0
b1c60:  clc
b1c70:  pop     dword ptr [ebx+34h]     ; restore old int 6 vector
        pop     dword ptr [ebx+30h]

        pop     ebx
        ret

Check386B0      endp

;++
;
; BOOLEAN
; TemporaryInt6 (
;    VOID
;    )
;
; Routine Description:
;
;    Temporary int 6 handler - assumes the cause of the exception was the
;    attempted execution of an XTBS instruction.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    none.
;
;--

TemporaryInt6    proc

        mov     [esp].IretEip,offset b1c60 ; set IP to clc instruction
        iretd

TemporaryInt6   endp

;++
;
; BOOLEAN
; Check386D1 (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 386 D1 Stepping.
;
;    It takes advantage of the fact that on pre-D1 386, if a REPeated
;    MOVS instruction is executed when single-stepping is enabled,
;    a single step trap is taken every TWO moves steps, but should
;    occuu each move step.
;
;    NOTE: This routine cannot distinguish between a D0 stepping and a D1
;    stepping.  If a need arises to make this distinction, this routine
;    will need modification.  D0 steppings will be recognized as D1.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if D1 or later stepping
;    Carry Flag set if B1 or prior
;
;--

Check386D1      proc
        push    ebx

        mov     ebx, fs:PcIdt           ; Address of IDT
        push    dword ptr [ebx+08h]
        push    dword ptr [ebx+0ch]     ; Save Trap01 handler

        mov     eax, offset TemporaryInt1
        mov     word ptr [ebx+08h], ax  ; Set LowWord
        shr     eax, 16
        mov     word ptr [ebx+0eh], ax  ; Set HighWord

;
; Attempt execution of rep movsb instruction with the Trace Flag set.
; Execution on B1 or earlier with length (CX) > 1 will trace over two
; iterations before accepting the trace trap.  Execution on D1 or later
; will accept the trace trap after a single iteration.  The dummy int 1
; handler will return execution to the instruction following the movsb
; instruction.  Examination of (CX) will reveal the stepping.
;

        sub     esp,4                   ; make room for target of movsb
        mov     esi, offset TemporaryInt1 ; (ds:esi) -> some present data
        mov     edi,esp
        mov     ecx,2                   ; 2 iterations
        pushfd
        or      dword ptr [esp], EFLAGS_TF
        popfd                           ; cause a single step trap
        rep movsb

d1c60:  add     esp,4                   ; clean off stack
        pop     dword ptr [ebx+0ch]     ; restore old int 1 vector
        pop     dword ptr [ebx+08h]
        stc                             ; assume B1
        jecxz   short d1cx              ; jmp if <= B1
        clc                             ; else clear carry to indicate >= D1
d1cx:
        pop     ebx
        ret

Check386D1      endp

;++
;
; BOOLEAN
; TemporaryInt1 (
;    VOID
;    )
;
; Routine Description:
;
;    Temporary int 1 handler - assumes the cause of the exception was
;    trace trap at the above rep movs instruction.
;
; Arguments:
;
;    (esp)->eip of trapped instruction
;           cs  of trapped instruction
;           eflags of trapped instruction
;
;--

TemporaryInt1   proc

        and     [esp].IretEFlags,not EFLAGS_TF ; clear caller's Trace Flag
        mov     [esp].IretEip,offset d1c60     ; set IP to next instruction
        iretd

TemporaryInt1   endp

;++
;
; BOOLEAN
; MultiplyTest (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks the 386 32-bit multiply instruction.
;    The reason for this check is because some of the i386 fail to
;    perform this instruction.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear on success
;    Carry Flag set on failure
;
;--
;

MultiplyTest    proc

        xor     cx,cx                   ; 64K times is a nice round number
mlt00:  push    cx
        call    Multiply                ; does this chip's multiply work?
        pop     cx
        jc      short mltx              ; if c, No, exit
        loop    mlt00                   ; if nc, YEs, loop to try again
        clc
mltx:
        ret

MultiplyTest    endp

;++
;
; BOOLEAN
; Multiply (
;    VOID
;    )
;
; Routine Description:
;
;    This routine performs 32-bit multiplication test which is known to
;    fail on bad 386s.
;
;    Note, the supplied pattern values must be used for consistent results.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear on success.
;    Carry Flag set on failure.
;
;--

Multiply        proc

        mov     ecx, MULTIPLIER
        mov     eax, MULTIPLICAND
        mul     ecx

        cmp     edx, RESULT_HIGH        ; Q: high order answer OK ?
        stc                             ; assume failure
        jnz     short mlpx              ;   N: exit with error

        cmp     eax, RESULT_LOW         ; Q: low order answer OK ?
        stc                             ; assume failure
        jnz     short mlpx              ;   N: exit with error

        clc                             ; indicate success
mlpx:
        ret

Multiply        endp

;++
;
; BOOLEAN
; KiIsNpxPresent(
;     VOID
;     );
;
; Routine Description:
;
;     This routine determines if there is any Numeric coprocessor
;     present.
;
;     Note that we do NOT determine its type (287, 387).
;     This code is extracted from Intel book.
;
; Arguments:
;
;     None.
;
; Return:
;
;     TRUE - If NPX is present.  Else a value of FALSE is returned.
;     Sets CR0 NPX bits accordingly.
;
;--

cPublicProc _KiIsNpxPresent,0

        push    ebp                     ; Save caller's bp
        mov     eax, cr0
        and     eax, NOT (CR0_ET+CR0_MP+CR0_TS+CR0_EM)
        mov     cr0, eax
        xor     edx, edx
.287
        fninit                          ; Initialize NPX
        mov     ecx, 5A5A5A5Ah          ; Put non-zero value
        push    ecx                     ;   into the memory we are going to use
        mov     ebp, esp
        fnstsw  word ptr [ebp]          ; Retrieve status - must use non-wait
        cmp     byte ptr [ebp], 0       ; All bits cleared by fninit?
        jne     Inp10

        or      eax, CR0_ET
        mov     edx, 1

        cmp     fs:PcPrcbData.PbCpuType, 3h
        jbe     Inp10

        or      eax, CR0_NE

Inp10:
        or      eax, CR0_EM+CR0_TS      ; During Kernel Initialization set
                                        ; the EM bit
        mov     cr0, eax
        pop     eax                     ; clear scratch value
        pop     ebp                     ; Restore caller's bp
        mov     eax, edx
        stdRet  _KiIsNpxPresent


stdENDP _KiIsNpxPresent


;++
;
; VOID
; CPUID (
;     ULONG   InEax,
;     PULONG  OutEax,
;     PULONG  OutEbx,
;     PULONG  OutEcx,
;     PULONG  OutEdx
;     );
;
; Routine Description:
;
;   Executes the CPUID instruction and returns the registers from it
;
;   Only available at INIT time
;
; Arguments:
;
; Return Value:
;
;--
cPublicProc _CPUID,5

    push    ebx
    push    esi

    mov     eax, [esp+12]

    cpuid

    mov     esi, [esp+16]   ; return EAX
    mov     [esi], eax

    mov     esi, [esp+20]   ; return EBX
    mov     [esi], ebx

    mov     esi, [esp+24]   ; return ECX
    mov     [esi], ecx

    mov     esi, [esp+28]   ; return EDX
    mov     [esi], edx

    pop     esi
    pop     ebx

    stdRET  _CPUID

stdENDP _CPUID

;++
;
; LONGLONG
; RDTSC (
;       VOID
;     );
;
; Routine Description:
;
; Arguments:
;
; Return Value:
;
;--
cPublicProc _RDTSC
    rdtsc
    stdRET  _RDTSC

stdENDP _RDTSC

INIT    ENDS

_TEXT   SEGMENT DWORD PUBLIC 'CODE'      ; Put IdleLoop in text section
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; ULONGLONG
; FASTCALL
; RDMSR (
;   IN ULONG MsrRegister
;   );
;
; Routine Description:
;
; Arguments:
;
; Return Value:
;
;--
cPublicFastCall RDMSR, 1
    rdmsr
    fstRET  RDMSR
fstENDP RDMSR


;++
;
; VOID
; WRMSR (
;   IN ULONG MsrRegister
;   IN LONGLONG MsrValue
;   );
;
; Routine Description:
;
; Arguments:
;
; Return Value:
;
;--
cPublicProc _WRMSR, 3
    mov     ecx, [esp+4]
    mov     eax, [esp+8]
    mov     edx, [esp+12]
    wrmsr
    stdRET  _WRMSR
stdENDP _WRMSR

;++
;
; VOID
; KeYieldProcessor (
;   VOID
;   );
;
; Routine Description:
;
;   Yields a thread of the processor
;
; Arguments:
;
; Return Value:
;
;--
cPublicProc _KeYieldProcessor
    YIELD
    stdRET _KeYieldProcessor
stdENDP _KeYieldProcessor

_TEXT   ENDS
        END
