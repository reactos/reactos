        title "Miscellaneous support routines"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    vdmmisc.asm
;
;Abstract:
;
;    This module contains miscellaneous support touines
;
;Author:
;
;    Dave Hastings (daveh) 23-Feb-1992
;
;Revision History:
;    18-Dec-1992 sudeepb wrote vdmdispatchbop in assembly for performance
;
;--
.386p

        .xlist
include ks386.inc
include callconv.inc
include mi386.inc
include vdm.inc
include vdmtib.inc

        page ,132

_PAGE   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:NOTHING, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING

        EXTRNP   _Ki386AdjustEsp0,1
        EXTRNP   _KeRaiseIrql,2
        EXTRNP   _KeLowerIrql,1
        EXTRNP   _KeGetCurrentIrql,0
        EXTRNP   _VdmEndExecution,2
        EXTRNP   _Ki386GetSelectorParameters,4
        EXTRNP   _NTFastDOSIO,2
        EXTRNP   _KiDispatchException,5
        EXTRNP   _ObWaitForSingleObject,3
        EXTRNP   _NtReleaseSemaphore,3
        EXTRNP   _VdmpIsThreadTerminating, 1
        EXTRNP   _NtSetEvent,2

        extrn    _KeI386EFlagsAndMaskV86:DWORD
        extrn    _KeI386EFlagsOrMaskV86:DWORD
        extrn    _VdmBopCount:DWORD



_PAGE   ENDS


_DATA   SEGMENT  DWORD PUBLIC 'DATA'
        public IcaLockTimeout
IcaLockTimeout DWORD 0ff676980h,0ffffffffh  ; 1 sec time out in - hundred nanosecs

_DATA   ENDS



_PAGE   SEGMENT
        ASSUME  DS:FLAT, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING


        page ,132
        subttl "Switch between two contexts"
;++
;
;   Routine Description:
;
;       This routine unloads a context from a kframe, and loads a different
;       context in it's place.
;
;       ASSUMES that Irql is APC level, if it is not this routine
;       may produce incorrect trapframes.
;
;   Arguments:
;
;       esp + 4 = PKTRAP_FRAME TrapFrame
;       esp + 8 = PCONTEXT OutGoing
;       esp + c = PCONTEXT InComming
;
;   Returns:
;
;       Nothing
;
;
cPublicProc _VdmSwapContexts,3

        push    ebp
        mov     ebp,esp
        push    esi
        push    edi
        push    ebx

if DBG
        EXTRNP  _DbgBreakPoint, 0

        stdcall _KeGetCurrentIrql
        cmp     al, APC_LEVEL
        je      vs05
        stdcall _DbgBreakPoint
vs05:
endif

;
;
; Move context from trap frame to outgoing context
;
        mov     esi,[ebp + 8]
        mov     edi,[ebp + 0ch]

if DBG
;
; Insure that we are really working on a stack frame
;
        cmp     [esi].TsDbgArgMark, 0BADB0D00h
        jne     vscfail
endif

        test    dword ptr [esi].TsEFlags,EFLAGS_V86_MASK
        jz      vs10
;
;
; Move segment registers
;
        mov     eax,[esi].TsV86Gs
        mov     [edi].CsSegGs,eax
        mov     eax,[esi].TsV86Fs
        mov     [edi].CsSegFs,eax
        mov     eax,[esi].TsV86Es
        mov     [edi].CsSegEs,eax
        mov     eax,[esi].TsV86Ds
        mov     [edi].CsSegDs,eax
        jmp     short vs20
vs10:
        cmp     word ptr [esi].TsSegCs,KGDT_R3_CODE OR RPL_MASK
        je      vs20                    ; Flat Mode

        mov     eax,[esi].TsSegGs
        mov     [edi].CsSegGs,eax
        mov     eax,[esi].TsSegFs
        mov     [edi].CsSegFs,eax
        mov     eax,[esi].TsSegEs
        mov     [edi].CsSegEs,eax
        mov     eax,[esi].TsSegDs
        mov     [edi].CsSegDs,eax
vs20:
        mov     eax,[esi].TsSegCs
        mov     [edi].CsSegCs,eax
        mov     eax,[esi].TsHardwareSegSs
        mov     [edi].CsSegSs,eax
;
; Move General Registers
;
        mov     eax,[esi].TsEax
        mov     [edi].CsEax,eax
        mov     eax,[esi].TsEbx
        mov     [edi].CsEbx,eax
        mov     eax,[esi].TsEcx
        mov     [edi].CsEcx,eax
        mov     eax,[esi].TsEdx
        mov     [edi].CsEdx,eax
        mov     eax,[esi].TsEsi
        mov     [edi].CsEsi,eax
        mov     eax,[esi].TsEdi
        mov     [edi].CsEdi,eax
;
; Move Pointer registers
;
        mov     eax,[esi].TsEbp
        mov     [edi].CsEbp,eax
        mov     eax,[esi].TsHardwareEsp
        mov     [edi].CsEsp,eax
        mov     eax,[esi].TsEip
        mov     [edi].CsEip,eax
;
; Move Flags
;
        mov     eax,[esi].TsEFlags
        mov     [edi].CsEFlags,eax
;
; Move incoming context to trap frame
;
        mov     edi,esi
        mov     esi,[ebp + 10h]
        mov     eax,[esi].CsSegCs
        mov     ebx,[esi].CsSegSs
        test    dword ptr [esi].CsEFlags, EFLAGS_V86_MASK
        jnz     vsc05                           ; don't worry about v86 segments

        or      ax, 3                           ; RPL 3 only
        or      bx, 3                           ; RPL 3 only
vsc05:  mov     [edi].TsSegCs,eax
        mov     [edi].TsHardwareSegSs,ebx
;
; Move General Registers
;
        mov     eax,[esi].CsEax
        mov     [edi].TsEax,eax
        mov     eax,[esi].CsEbx
        mov     [edi].TsEbx,eax
        mov     eax,[esi].CsEcx
        mov     [edi].TsEcx,eax
        mov     eax,[esi].CsEdx
        mov     [edi].TsEdx,eax
        mov     eax,[esi].CsEsi
        mov     [edi].TsEsi,eax
        mov     eax,[esi].CsEdi
        mov     [edi].TsEdi,eax
;
; Move Pointer registers
;
        mov     eax,[esi].CsEbp
        mov     [edi].TsEbp,eax
        mov     eax,[esi].CsEsp
        mov     [edi].TsHardwareEsp,eax
        mov     eax,[esi].CsEip
        mov     [edi].TsEip,eax
;
; Move Flags
;
        mov     eax,[esi].CsEFlags
        test    eax,EFLAGS_V86_MASK
        jne     vsc10

        and     eax,EFLAGS_USER_SANITIZE
        or      eax,EFLAGS_INTERRUPT_MASK
        jmp     vsc15

vsc10:  and     eax,_KeI386EFlagsAndMaskV86
        or      eax,_KeI386EFlagsOrMaskV86
vsc15:  mov     [edi].TsEFlags,eax

;
; Fix Esp 0 as necessary
;
        mov     esi,[ebp+0ch]
        xor     eax,[esi].CsEFlags
        mov     esi,[ebp + 10h]
        test    eax,EFLAGS_V86_MASK
        jz      vsc20

        stdCall _Ki386AdjustEsp0, <edi>
        test    dword ptr [edi].TsEFlags,EFLAGS_V86_MASK
        jz      vsc20
;
; Move segment registers for vdm
;
        mov     eax,[esi].CsSegGs
        mov     [edi].TsV86Gs,eax
        mov     eax,[esi].CsSegFs
        mov     [edi].TsV86Fs,eax
        mov     eax,[esi].CsSegEs
        mov     [edi].TsV86Es,eax
        mov     eax,[esi].CsSegDs
        mov     [edi].TsV86Ds,eax
        jmp     short vsc30
vsc20:
;
; Move segment registers for monitor
;
        mov     eax,[esi].CsSegGs
        mov     [edi].TsSegGs,eax
        mov     eax,[esi].CsSegFs
        mov     [edi].TsSegFs,eax
        mov     eax,[esi].CsSegEs
        mov     [edi].TsSegEs,eax
        mov     eax,[esi].CsSegDs
        mov     [edi].TsSegDs,eax
;
; We are going back to 32 bit monitor code.  Set Trapframe exception list
; to END_OF_CHAIN such that we won't bugcheck in KiExceptionExit.
;
        mov     eax, 0ffffffffh
        mov     [edi].TsExceptionList, eax
vsc30:
        pop     ebx
        pop     edi
        pop     esi
        mov     esp,ebp
        pop     ebp
        stdRET  _VdmSwapContexts

if DBG
vscfail: int 3
endif

_VdmSwapContexts endp


; Converted C code VDMDispatchBop to this assembly routine. This
; routine is performance critical.
;
; Input EBP -> Trap Frame

            public VdmDispatchBop
VdmDispatchBop proc

vdb05:
        mov     eax,PCR[PcPrcbData+PbCurrentThread]
        mov     esi,[eax]+ThApcState+AsProcess
        mov     esi,[esi].EpVdmObjects
        mov     esi,[esi].VpVdmTib             ; get pointer to VdmTib
        test    dword ptr [ebp].TsEFlags,EFLAGS_V86_MASK
        jz      vdb10
        movzx   ebx,word ptr [ebp].TsSegCs
        shl     ebx,4
        add     ebx,[ebp].TsEip
        jmp     short vdb20
vdb10:
        mov     edi,ebp
        push    ebp
        mov     ebp,esp
        sub     esp,12          ; 3 dwords for local parameters
        lea     eax,[ebp-4]
        push    eax             ; For Limit
        lea     eax,[ebp-8]
        push    eax             ; For Base
        lea     eax,[ebp-12]
        push    eax             ; For Flags
        push    dword ptr [edi].TsSegCs
        call    _Ki386GetSelectorParameters@16
        mov     ebx,dword ptr [ebp-8]
        mov     esp,ebp
        pop     ebp

        or      al,al
        jz      vdb80
        add     ebx,dword ptr [ebp].TsEip
vdb20:
        cmp     word ptr [ebx], 0c4c4h
        jnz     vdb85
        movzx   eax,byte ptr [ebx+2]            ; Get The Bop Number
        cmp     eax,DOS_BOP
        je      chk_rdwr
no_rdwr:
if DEVL
        inc     _VdmBopCount
endif
        mov     dword ptr [esi].VtEIEvent,VdmBop
        mov     dword ptr [esi].VtEIBopNumber,eax
        mov     dword ptr [esi].VtEIInstSize,3

        stdcall _VdmEndExecution <ebp,esi>       ; ebp - TrapFrame
                                                 ; esi - pTib
vdb80:
        mov     eax,1
        ret

vdb85:
        xor     eax,eax
        ret

chk_rdwr:
        cmp     byte ptr [ebx+3],SVC_DEMFASTREAD
        je      do_fast_io
        cmp     byte ptr [ebx+3],SVC_DEMFASTWRITE
        jne     no_rdwr
do_fast_io:
        movzx   eax,byte ptr [ebx+3]
        push    eax
        push    ebp
        mov     eax, KGDT_R3_DATA OR RPL_MASK
        mov     es, ax
        call    _NTFastDOSIO@8
        jmp     vdb80



VdmDispatchBop endp


CriticalSection equ     [esp+4]

        page , 132
        subttl  "VdmpEnterCriticalSection"

;++
;
; NTSTATUS
; VdmpEnterIcaLock(
;    IN PRTL_CRITICAL_SECTION pIcaLock
;    )
;
; Routine Description:
;
;    This function enters a UserMode critical section, with a fixed Timeout
;    of several minutes.
;
;    Touching the critical section may cause an exception to be raised which
;    the caller must handle, since the critical section may be in UserMode
;    memory.
;
;
; Arguments:
;
;    CriticalSection - supplies a pointer to a critical section.
;
; Return Value:
;
;   STATUS_SUCCESS - wait was satisfied and the thread owns the CS
;   STATUS_INVALID_HANDLE - no semaphore available to wait on.
;   STATUS_TIMEOUT
;
;

;--

        align   16
cPublicProc _VdmpEnterIcaLock,1
cPublicFpo 1,0

	 mov	 edx,CriticalSection	     ; interlocked inc of
         mov     ecx,PCR[PcTeb]
         mov     ecx,TbClientId+4[ecx]        ; NtCurrentTeb()->ClientId.UniqueThread

         mov     eax, STATUS_INVALID_HANDLE
         cmp     dword ptr CsLockSemaphore[edx],0 ; avoid lazy creates
         jz      short Eil20

         xor     eax,eax                     ; assume success
    lock inc     dword ptr CsLockCount[edx]  ; ... CriticalSection->LockCount
         jnz     short Eil30

         ;
         ; Set Curr thread as Owner of CS with recursion count of 1
         ; and return SUCCESS
         ;
Eil10:
         mov     CsOwningThread[edx],ecx
         mov     dword ptr CsRecursionCount[edx],1
Eil20:
         stdRET  _VdmpEnterIcaLock


         ;
         ; If curr thread already owns CS,
         ;    inc recusrion count and return SUCCESS
         ;
Eil30:
         cmp     CsOwningThread[edx],ecx
         jne     short Eil42
         inc     dword ptr CsRecursionCount[edx]
         stdRET  _VdmpEnterIcaLock


         ;
         ;  Another Thread owns the CS so Wait on the lock semaphore,
         ;
Eil40:
         mov     edx, CriticalSection
Eil42:
         xor     eax,eax
         lea     ecx, IcaLockTimeout
         stdCall _ObWaitForSingleObject <CsLockSemaphore[edx], eax, ecx>

         mov     ecx,PCR[PcTeb]
	 mov	 ecx,TbClientId+4[ecx]	     ; NtCurrentTeb()->ClientId.UniqueThread
         mov     edx,CriticalSection

         or      eax, eax
         jz      short Eil10             ; Take Ownership of CS

         ;
         ; If !NT_SUCCESS(Status) return with error. else some other
         ; less severe error occurred. In that case if thread terminating
         ; fail. Note: we may wake for user apc's even tho we are non
         ; alertable, because the vdm hw int dispatching code, and PsThread
         ; termination code forces these to occur.
         ;
         test    eax, 080000000h
         jnz     short Eil20             ; exit with Status in eax

Eil50:
         stdCall _VdmpIsThreadTerminating <ecx> ;    check for Term of self
         or      eax, eax
         jnz     short Eil20             ; exit with Status in eax

         mov     edx, CriticalSection
         mov     eax, CsOwningThread[edx]
         stdCall _VdmpIsThreadTerminating <eax> ;    check for Term of CSOwner
         or      eax, eax
         jz      short Eil40             ; retry
         jmp     short Eil20             ; exit with Status in eax


stdENDP _VdmpEnterIcaLock



        page , 132
        subttl  "VdmpLeaveIcaLock"
;++
;
; NTSTATUS
; VdmpLeaveIcaLock(
;    IN PRTL_CRITICAL_SECTION pIcaLock
;    )
;
; Routine Description:
;
;    This function leaves a critical section.
;
;    Touching the critical section may cause an exception to be raised which
;    the caller must handle, since the critical section may be in UserMode
;    memory.
;
; Arguments:
;
;    CriticalSection - supplies a pointer to a critical section.
;
; Return Value:
;
;   STATUS_SUCCESS
;   STATUS_INVALID_OWNER
;   or NTSTATUS code from NtReleaseSemaphore
;
;--

        align   16
cPublicProc _VdmpLeaveIcaLock,1
cPublicFpo 1,0

        mov     edx,CriticalSection
	mov	ecx,PCR[PcTeb]
	mov	ecx,TbClientId+4[ecx]	     ; NtCurrentTeb()->ClientId.UniqueThread

	mov	eax,STATUS_INVALID_OWNER    ; Verify Owner of CritSect
        cmp     ecx,CsOwningThread[edx]
        jne     short Lil10

        xor     eax,eax                     ; Assume STATUS_SUCCESS
        dec     dword ptr CsRecursionCount[edx]
        jnz     short Lil30                 ; leaving recursion

        mov     CsOwningThread[edx],eax     ; clear owning thread id
   lock dec     dword ptr CsLockCount[edx]  ; interlocked dec of LockCount
        jge     short Lil20                 ; Thread waiting on LockSemaphore ?

Lil10:
        stdRET  _VdmpLeaveIcaLock


        ;
        ; release another thread waiting on the LockSemaphore
        ; and exit
Lil20:
        stdCall _NtSetEvent, <CsLockSemaphore[edx], 0>
        stdRET  _VdmpLeaveIcaLock


        ;
        ; leaving recursion, just dec lock count
        ;
Lil30:

   lock dec     dword ptr CsLockCount[edx]  ; interlocked dec of LockCount
        stdRET  _VdmpLeaveIcaLock

_VdmpLeaveIcaLock    endp

_PAGE ends

        end
