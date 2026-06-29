
;++
;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Module:
;
;   kxamd64.w
;
; Astract:
;
;   Contains AMD64 architecture constants and assembly macros.
;
;
;--

include macamd64.inc

;
; If the actual CFG dispatch code is not being assembled, then declare the
; guard dispatch function as external.
;

ifdef CAPXCALL_VIA_CFG
ifndef CFG_ASM

        extern  _guard_dispatch_icall:proc

endif ; CFG_ASM
endif

;
;

;
; Retpoline macros
;

RETPOLINEDIRECTIVE macro RetpolineType

ifdef _RETPOLINE
    ifidn <RetpolineType>,<RetpolineRax>
    .retpolinerax
    elseifidn <RetpolineType>,<RetpolineSwitchTable>
    .retpolineswitchtable
    elseifidn <RetpolineType>,<RetpolineImport>
    .retpolineimport
    elseifidn <RetpolineType>,<RetpolineCfgDispatch>
    .retpolineguardicall
    elseifidn <RetpolineType>,<RetpolineIgnore>
    .retpolineignore
    elseifnb <RetpolineType>
    .err @catstr(<Invalid retpoline type >, <RetpolineType>)
    endif
endif ; _RETPOLINE

    ENDM

;
; IceCAP macros
;

ifdef _CAPKERN

;
; Define kernel icecap macros for tracing assembly routines.
;

        extern  __CAP_Start_DirectCall_Profiling_ASM:proc
        extern  __CAP_Start_IndirectCall_Profiling_ASM:proc
        extern  __CAP_Start_DirectCall_Tail_Profiling_ASM:proc
        extern  __CAP_Start_IndirectCall_Tail_Profiling_ASM:proc
        extern  __CAP_Start_DirectJmp_Profiling_ASM:proc
        extern  __CAP_Start_IndirectJmp_Profiling_ASM:proc
        extern  __CAP_End_Profiling:proc

endif ; _CAPKERN

;
; CAPDCALL - Perform an IceCAP instrumented direct function call.
;
;            Do NOT use this macro directly. Use CAPCALL instead.
;
;            Instrumenation is disabled on non-IceCAP builds.
;
; Arguments:
;
;   Callee - A function label.
;
;   NoRet - If set, the function does not return to the caller.
;
; Return value:
;
;   Returns the value of the function specified by Callee.
;

CAPDCALL macro Callee, NoRet

ifdef _CAPKERN

ifnb <NoRet>

        call    __CAP_Start_DirectCall_Tail_Profiling_ASM ; record profiling information

else

        call    __CAP_Start_DirectCall_Profiling_ASM ; record profiling information

endif

endif ; _CAPKERN

        call    Callee

ifdef _CAPKERN

ifb <NoRet>

        call    __CAP_End_Profiling       ; record profiling information

endif

endif ; _CAPKERN

        endm

;
; CAPICALL - Perform an IceCAP instrumented indirect function call.
;
;            Do NOT use this macro directly. Use CAPCALL instead.
;
;            Instrumenation is disabled on non-IceCAP builds.
;
; Arguments:
;
;   Callee - A register or a memory location.
;
;   NoRet - If set, the function does not return to the caller.
;
; Return value:
;
;   Returns the value of the function specified by Callee.
;

CAPICALL macro Callee, NoRet, RetpolineType

ifdef _CAPKERN

ifnb <NoRet>

        call    __CAP_Start_IndirectCall_Tail_Profiling_ASM ; record profiling information

else

        call    __CAP_Start_IndirectCall_Profiling_ASM ; record profiling information

endif

endif ; _CAPKERN

        RETPOLINEDIRECTIVE RetpolineType

        call    Callee


ifdef _CAPKERN

ifb <NoRet>

        call    __CAP_End_Profiling       ; record profiling information

endif

endif ; _CAPKERN

        endm

;
; CAPXCALL - Perform a CFG IceCAP instrumented indirect function call.
;
;            This macro MUST be used to call CFG checked functions.
;
;            Instrumenation is disabled on non-IceCAP builds.
;
; Arguments:
;
;   Callee - A register or a memory location.
;
;   NoRet - If set, the function does not return to the caller.
;
; Return value:
;
;   Returns the value of the function specified by Callee.
;

CAPXCALL macro Callee, NoRet

ifdifi  <Callee>, <rax>

        mov     rax, Callee

endif

ifdef CAPXCALL_VIA_CFG

        CAPDCALL _guard_dispatch_icall, <NoRet>

else

        CAPICALL     rax, <NoRet>, RetpolineRax
endif

        endm

;
; CAPCALL - Perform an IceCAP instrumented function call.
;
;           Instrumenation is disabled on non-IceCAP builds.
;
; Arguments:
;
;   Callee - A function label, a register, or a memory location.
;
;   NoRet - If set, the function does not return to the caller.
;
; Return value:
;
;   Returns the value of the function specified by Callee.
;

CAPCALL macro Callee, NoRet, RetpolineType

;
; The documentation surrounding the .type operator is very poor.
; Here is the significance of each of the bits.
;
; 0 - References a label in the code segment if set.
; 1 - References a memory variable or relocatable data object if set.
; 2 - Is an immediate (absolute/constant) value if set.
; 3 - Uses direct memory addressing if set.
; 4 - Is a register name, if set.
; 5 - References no undefined symbols and there is no error, if set.
; 6 - Is an SS: relative reference, if set.
; 7 - References an external name.
;

if (.type(Callee)) eq 000h ; direct via local label

        CAPDCALL Callee, NoRet

elseif (.type(Callee)) eq 022h ; register-direct call

        CAPICALL Callee, NoRet, RetpolineType

elseif (.type(Callee)) eq 025h ; direct via extern proc

        CAPDCALL Callee, NoRet

elseif (.type(Callee)) eq 030h ; register-indirect call

        CAPICALL Callee, NoRet, RetpolineType

elseif (.type(Callee)) eq 062h ; indirect via offset from register

        CAPICALL Callee, NoRet, RetpolineType

elseif (.type(Callee)) eq 0A5h ; direct via extern proc

        CAPDCALL Callee, NoRet

elseif (.type(Callee)) eq 0AAh ; indirect via extern qword

        CAPICALL Callee, NoRet, RetpolineType

else

        .err @catstr(<unknown expression type >, %(.type(Callee)))

endif

        endm

;
; CAPJMP - Perform an IceCAP instrumented tail jump.
;
;          Instrumenation is disabled on non-IceCAP builds.
;
; Arguments:
;
;   Callee - A function label, a memory address, or a register.
;
; Return value:
;
;   Never returns.
;

CAPJMP macro Callee, RetpolineType

;
; Begin jump instrumentation
;

ifdef _CAPKERN

;
; The documentation surrounding the .type operator is very poor.
; See above for syntax explanation.
;

if (.type(Callee)) eq 000h ; direct via local label

        call    __CAP_Start_DirectJmp_Profiling_ASM ; record profiling information

elseif (.type(Callee)) eq 022h ; register-direct jump

        call    __CAP_Start_IndirectJmp_Profiling_ASM  ; record profiling information

elseif (.type(Callee)) eq 025h ; direct via extern proc

        call    __CAP_Start_DirectJmp_Profiling_ASM ; record profiling information

elseif (.type(Callee)) eq 030h ; register-indirect

        call    __CAP_Start_IndirectJmp_Profiling_ASM  ; record profiling information

elseif (.type(Callee)) eq 062h ; indirect via offset from register

        call    __CAP_Start_IndirectJmp_Profiling_ASM  ; record profiling information

elseif (.type(Callee)) eq 0A5h ; direct via extern proc

        call    __CAP_Start_DirectJmp_Profiling_ASM ; record profiling information

elseif (.type(Callee)) eq 0AAh ; indirect via extern qword

        call    __CAP_Start_IndirectJmp_Profiling_ASM  ; record profiling information

else

        .err @catstr(<unknown expression type >, %(.type(Callee)))

endif

endif ; _CAPKERN

;
; End jump instrumentation. Begin common code.
;

        RETPOLINEDIRECTIVE <RetpolineType>

        jmp     Callee

        endm

;
; CAPRETPOLJMP - Perform an IceCAP instrumented tail jump using retpoline.
;
;          Instrumentation is disabled on non-IceCAP builds.
;
; Arguments:
;
;   Callee - A function label, a memory address or a register.
;
; Return value:
;
;   Never returns.
;

CAPRETPOLJMP macro Callee, Alignment

        LOCAL Jump

;
; Begin jump instrumentation
;

ifdef _CAPKERN

;
; The documentation surrounding the .type operator is very poor.
; See above for syntax explanation.
;

if (.type(Callee)) eq 000h ; direct via local label

        call    __CAP_Start_DirectJmp_Profiling_ASM ; record profiling information

elseif (.type(Callee)) eq 022h ; register-direct jump

        call    __CAP_Start_IndirectJmp_Profiling_ASM  ; record profiling information

elseif (.type(Callee)) eq 025h ; direct via extern proc

        call    __CAP_Start_DirectJmp_Profiling_ASM ; record profiling information

elseif (.type(Callee)) eq 030h ; register-indirect

        call    __CAP_Start_IndirectJmp_Profiling_ASM  ; record profiling information

elseif (.type(Callee)) eq 062h ; indirect via offset from register

        call    __CAP_Start_IndirectJmp_Profiling_ASM  ; record profiling information

elseif (.type(Callee)) eq 0A5h ; direct via extern proc

        call    __CAP_Start_DirectJmp_Profiling_ASM ; record profiling information

elseif (.type(Callee)) eq 0AAh ; indirect via extern qword

        call    __CAP_Start_IndirectJmp_Profiling_ASM  ; record profiling information

else

        .err @catstr(<unknown expression type >, %(.type(Callee)))

endif

endif ; _CAPKERN

;
; End jump instrumentation. Begin common code.
;

        call Jump
        int  3

ifnb <Alignment>
        ALIGN Alignment
else
        ALIGN 16
endif

Jump:
        mov [rsp], Callee
        ret

        endm

;
; CAPEPILOGJMP - Logs a tail jump from an epilog.
;
;                Instrumenation is disabled on non-IceCAP builds.
;
; Arguments:
;
;   Callee - A function label, a memory address, or a register.
;
; Return value:
;
;   None
;

CAPEPILOGJMP macro Callee

        LOCAL Exit

ifdef _CAPKERN

        jmp Exit
Exit:
        CAPJMP Callee

else

        jmp Callee

endif ; _CAPKERN

        endm

;
; CAPREX_JMP_REG  - Perform an IceCAP instrumented indirect tail jump.
;                   Use this when a function does not return to the caller
;                   and when a rex_jmp_reg would normally be specified.
;
;                   Instrumentation is disabled on non-IceCAP builds.
;
; Arguments:
;
;   Callee is a register or memory location.
;
; Return value:
;
;   None
;

CAPREX_JMP_REG macro Callee, RetpolineType

ifdef _CAPKERN

        call    __CAP_Start_IndirectJmp_Profiling_ASM ; record profiling information

endif ; _CAPKERN

        RETPOLINEDIRECTIVE <RetpolineType>
        rex_jmp_reg Callee                    ; make the jump
        endm

;
; Define macro to clear legacy floating exceptions.
;

clfpex  macro

        db      0dbh, 0e2h

        endm

;
; Define macro to perform an enlightened yield.
;
; Arguments:
;
;   None.
;
; N.B. This macro is restricted to only freely using the register specified by
;      the 'Register' parameter and rcx. 'Register' should be nonvolatile.
;

EnlightenedYield macro Register

        local skip

;
;;

skip:   Yield

        endm

;
; Define macro to acquire spin lock.
;
; Arguments:
;
;   None.
;
; N.B. This macro is restricted to only freely using the register specified by
;      the 'Register' parameter and rcx. 'Register' should be nonvolatile.
;
; N.B. If 'Register' is specified, 'Address' must be nonvolatile or global.
;

AcquireSpinLock macro Address, Register

        local exit, spin

ifndef NT_UP

   lock bts     qword ptr Address, 0    ; attempt to acquire spin lock
        jnc     short exit              ; if nc, spin lock acquired

ifndef XBOX_SYSTEMOS

ifnb <Register>

        xor      Register, Register      ; initialize spin count

endif

endif

spin:   EnlightenedYield <Register>     ; yield execution
        test    qword ptr Address, 1    ; check if lock currently owned
        jnz     short spin              ; if nz, spin lock owned
   lock bts     qword ptr Address, 0    ; attempt to acquire spin lock
        jc      short spin              ; if c, spin lock owned

exit:                                   ; continue

endif

        endm

;
; Define macro to acquire spin lock and mask interrupts.
;
; Arguments:
;
;   None.
;
; Note:
;
;   rsp is assumed to point to pushed EFLAGS
;
; N.B. This macro uses no registers.
;

AcquireSpinLockDisable macro Address

        local exit, spin, spin1

        cli                             ; disable interrupts

ifndef NT_UP

   lock bts     qword ptr Address, 0    ; attempt to acquire spin lock
        jnc     short exit              ; if nc, spin lock acquired
spin:   test    dword ptr [rsp], EFLAGS_IF_MASK ; test if interrupts enabled
        jz      short spin1             ; if z, interrupts disabled
        sti                             ; enable interrupts

spin1:  Yield                           ; yield execution

        test    qword ptr Address, 1    ; check if lock currently owned
        jnz     short spin1             ; if nz, spin lock owned
        cli                             ; lock is (was) clear, disable ints
   lock bts     qword ptr Address, 0    ; attempt to acquire spin lock
        jc      short spin              ; if c, spin lock owned
exit:                                   ; continue

endif

        endm

;
; Define macro to release spin lock.
;
; Arguments:
;
;   None.
;
; N.B. This macro uses no registers.
;

ReleaseSpinLock macro Address

ifndef NT_UP

   lock and     qword ptr Address, 0    ; release spin lock

endif

        endm

;
; Define macro to release spin lock and restore the interrupt flag.
;
; Arguments:
;
;   None.
;
; Note:
;
;   rsp is assumed to point to pushd EFLAGS
;
; N.B. This macro uses no registers.
;

ReleaseSpinLockEnable macro Address

        local exit

ifndef NT_UP

   lock and     qword ptr Address, 0    ; release spin lock

endif

        test    dword ptr [rsp], EFLAGS_IF_MASK ; test if interrupts enabled
        jz      short exit              ; if z, interrupts not enabled
        sti                             ; enable interrupts
exit:                                   ; continue

        endm

;
; Define macro to try to acquire spin lock.
;
; Arguments:
;
;   None.
;
; N.B. This macro uses no registers.
;

TryToAcquireSpinLock macro Address

ifndef NT_UP

        lock bts qword ptr Address, 0   ; attempt to acquire spin lock

endif

        endm

;
; Define macro to perform the equivalent of reading cr8.
;
; Arguments:
;
;   None
;
; The equivalent of the contents of cr8 is returned in rax
;
; N.B. This macro is restricted to using only rax.
;

ReadCr8 macro

        mov     rax, cr8                ; read IRQL

        endm

;
; Define macro to perform the equivalent of writing cr8.
;
; Arguments:
;
;   rcx - The desired value of cr8.
;

WriteCr8 macro

        mov     cr8, rcx                ; write IRQL

        endm

;
; Define macro to get current IRQL.
;
; Arguments:
;
;   None.
;
; The previous IRQL is returned in rax.
;

CurrentIrql macro

        ReadCr8                         ; get current IRQL

        endm

;
; Define macro to lower IRQL.
;
; Arguments:
;
;   rcx - Supplies the new IRQL.
;
; N.B. The register rax is destroyed.
;
; N.B. This macro is restricted to using only rcx and rdx.
;

LowerIrql macro

        local   exit

if DBG

        mov     rdx, rax                ; preserve rax
        ReadCr8                         ; get current IRQL
        cmp     eax, ecx                ; check new IRQL
        jge     short exit              ; if ge, new IRQL okay
        int     3                       ; break into debugger
exit:   mov     rax, rdx

endif

        WriteCr8                        ; set new IRQL

        endm

;
; Define macro to raise IRQL.
;
; Arguments:
;
;   rcx - Supplies the new IRQL.
;
; The previous IRQL is returned in rax.
;
; N.B. This macro is restricted to using only rax and rcx.
;

RaiseIrql macro

        local   exit

        ReadCr8                         ; get current IRQL

if DBG

        cmp     eax, ecx                ; check new IRQL
        jle     short exit              ; if le, new IRQL okay
        int     3                       ; break into debugger

endif

exit:   WriteCr8                        ; set new IRQL

        endm

;
; Define macro to set IRQL.
;
; Arguments:
;
;   rcx - Supplies the new IRQL.
;
; N.B. This macro is restricted to using only rcx.
;

SetIrql macro

        WriteCr8                        ; set new IRQL

        endm

;
; Define macro to swap IRQL.
;
; Arguments:
;
;   rcx - Supplies the new IRQL.
;
; The previous IRQL is returned in rax.
;
; N.B. This macro is restricted to using only rax and rcx.
;

SwapIrql macro

        ReadCr8                         ; get current IRQL
        WriteCr8                        ; set new IRQL

        endm

;
;

;
; Define restore exception state macro.
;
;   This macro restores the nonvolatile state.
;
; Arguments:
;
;   Flag - If blank, then nonvolatile floating and integer registers are
;       restored. If nonblank and identical to "Rbp", then rbp is restored
;       in addition to the nonvolatile floating and integer registers. If
;       nonblank and identical to "NoFp", then only the nonvolatile integer
;       registers are restored.
;
; Implicit arguments:
;
;   rsp - Supplies the address of the exception frame.
;

RESTORE_EXCEPTION_STATE macro Flag

        lea     rcx, 100h[rsp]          ; set frame display pointer

ifdif <Flag>, <NoFp>

        movaps  xmm6, ExXmm6[rsp]       ; restore nonvolatile xmm registers
        movaps  xmm7, ExXmm7[rsp]       ;
        movaps  xmm8, ExXmm8[rsp]       ;
        movaps  xmm9, ExXmm9[rsp]       ;
        movaps  xmm10, ExXmm10[rsp]     ;
        movaps  xmm11, (ExXmm11 - 100h)[rcx] ;
        movaps  xmm12, (ExXmm12 - 100h)[rcx] ;
        movaps  xmm13, (ExXmm13 - 100h)[rcx] ;
        movaps  xmm14, (ExXmm14 - 100h)[rcx] ;
        movaps  xmm15, (ExXmm15 - 100h)[rcx] ;

endif

        mov     rbx, (ExRbx - 100h)[rcx] ; restore nonvolatile integer registers
        mov     rdi, (ExRdi - 100h)[rcx] ;
        mov     rsi, (ExRsi - 100h)[rcx] ;
        mov     r12, (ExR12 - 100h)[rcx] ;
        mov     r13, (ExR13 - 100h)[rcx] ;
        mov     r14, (ExR14 - 100h)[rcx] ;
        mov     r15, (ExR15 - 100h)[rcx] ;

ifdif <Flag>, <NoPop>

ifidn <Flag>, <Rbp>

        mov     rbp, (ExRbp - 100h)[rcx]  ; restore nonvolatile integer register

endif

        add     rsp, KEXCEPTION_FRAME_LENGTH - (1 * 8) ; deallocate frame

ifdif <Flag>, <Rbp>

        BEGIN_EPILOGUE

endif

endif

        endm

;
; Define generate exception frame macro.
;
;   This macro allocates an exception frame and saves the nonvolatile state.
;
; Arguments:
;
;   Flag - If blank, then nonvolatile floating and integer registers are
;       saved. If nonblank and identical to "Rbp", then rbp is saved in
;       addition to the nonvolatile floating and integer registers. If
;       nonblank and identical to "NoFp", then only the nonvolatile integer
;       registers are saved. If nonblank and identical to "NoPop", then
;       allocate an exception record in addition to an exception frame. If
;       nonblank and identical to "NoFrame", then rbp is saved in addition to
;       the other registers but is not established as a frame pointer.
;
; Implicit arguments:
;
;   The top of the stack is assumed to contain a return address.
;

GENERATE_EXCEPTION_FRAME macro Flag


ifidn <Flag>, <NoPop>

        alloc_stack (EXCEPTION_RECORD_LENGTH + KEXCEPTION_FRAME_LENGTH - (1 * 8)) ; allocate frame

else

        alloc_stack (KEXCEPTION_FRAME_LENGTH - (1 * 8)) ; allocate frame

endif

        lea     rax, 100h[rsp]          ; set frame display pointer

ifdif <Flag>, <NoFp>

        save_xmm128 xmm6, ExXmm6        ; save xmm nonvolatile registers
        save_xmm128 xmm7, ExXmm7        ;
        save_xmm128 xmm8, ExXmm8        ;
        save_xmm128 xmm9, ExXmm9        ;
        save_xmm128 xmm10, ExXmm10      ;

        movaps  (ExXmm11 - 100h)[rax], xmm11 ;
        .savexmm128 xmm11, ExXmm11      ;

        movaps  (ExXmm12 - 100h)[rax], xmm12 ;
        .savexmm128 xmm12, ExXmm12      ;

        movaps  (ExXmm13 - 100h)[rax], xmm13 ;
        .savexmm128 xmm13, ExXmm13      ;

        movaps  (ExXmm14 - 100h)[rax], xmm14 ;
        .savexmm128 xmm14, ExXmm14      ;

        movaps  (ExXmm15 - 100h)[rax], xmm15 ;
        .savexmm128 xmm15, ExXmm15      ;

endif

ifidn <Flag>, <Rbp>

        mov     (ExRbp - 100h)[rax], rbp  ; save nonvolatile integer register
        .savereg rbp, ExRbp             ;
        set_frame rbp, 0                ; set frame pointer

endif

ifidn <Flag>, <NoFrame>

        mov     (ExRbp - 100h)[rax], rbp  ; save nonvolatile integer register
        .savereg rbp, ExRbp             ;

endif

        mov     (ExRbx - 100h)[rax], rbx  ;
        .savereg rbx, ExRbx             ;

        mov     (ExRdi - 100h)[rax], rdi  ;
        .savereg rdi, ExRdi             ;

        mov     (ExRsi - 100h)[rax], rsi  ;
        .savereg rsi, ExRsi             ;

        mov     (ExR12 - 100h)[rax], r12  ;
        .savereg r12, ExR12             ;

        mov     (ExR13 - 100h)[rax], r13  ;
        .savereg r13, ExR13             ;

        mov     (ExR14 - 100h)[rax], r14  ;
        .savereg r14, ExR14             ;

        mov     (ExR15 - 100h)[rax], r15  ;
        .savereg r15, ExR15             ;

        END_PROLOGUE

        endm

;
