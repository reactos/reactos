        title  "Context Swap"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ctxswap.asm
;
; Abstract:
;
;    This module implements the code necessary to field the dispatch
;    interrupt and to perform kernel initiated context switching.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 14-Jan-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;   22-feb-90   bryanwi
;       write actual swap context procedure
;
;--

.486p
        .xlist
include ks386.inc
include i386\kimacro.inc
include mac386.inc
include callconv.inc
        .list

        EXTRNP  KiAcquireQueuedSpinLock,1,,FASTCALL
        EXTRNP  KiReleaseQueuedSpinLock,1,,FASTCALL
        EXTRNP  KiTryToAcquireQueuedSpinLock,1,,FASTCALL

        EXTRNP  KfAcquireSpinLock,1,IMPORT,FASTCALL

        EXTRNP  HalClearSoftwareInterrupt,1,IMPORT,FASTCALL
        EXTRNP  HalRequestSoftwareInterrupt,1,IMPORT,FASTCALL
        EXTRNP  KiActivateWaiterQueue,1,,FASTCALL
        EXTRNP  KiReadyThread,1,,FASTCALL
        EXTRNP  KiWaitTest,2,,FASTCALL
        EXTRNP  KfLowerIrql,1,IMPORT,FASTCALL
        EXTRNP  KfRaiseIrql,1,IMPORT,FASTCALL
        EXTRNP  _KeGetCurrentIrql,0,IMPORT
        EXTRNP  _KeGetCurrentThread,0
        EXTRNP  _KiDeliverApc,3
        EXTRNP  _KiQuantumEnd,0
        EXTRNP  _KeBugCheckEx,5
        EXTRNP  _KeBugCheck,1

        extrn   _KiTrap13:PROC
        extrn   KiRetireDpcList:PROC
        extrn   _KeI386FxsrPresent:BYTE
        extrn   _KiDispatcherLock:DWORD
        extrn   _KeFeatureBits:DWORD
        extrn   _KeThreadSwitchCounters:DWORD
        extrn   _KeTickCount:DWORD

        extrn   __imp_@KfLowerIrql@4:DWORD

        extrn   _KiWaitInListHead:DWORD
        extrn   _KiWaitOutListHead:DWORD
        extrn   _KiDispatcherReadyListHead:DWORD
        extrn   _KiIdleSummary:DWORD
        extrn   _KiReadySummary:DWORD
        extrn   _KiSwapContextNotifyRoutine:DWORD
        extrn   _KiThreadSelectNotifyRoutine:DWORD

if DBG
        extrn   _KdDebuggerEnabled:BYTE
        EXTRNP  _DbgBreakPoint,0
        extrn   _DbgPrint:near
        extrn   _MsgDpcTrashedEsp:BYTE
        extrn   _MsgDpcTimeout:BYTE
        extrn   _KiDPCTimeout:DWORD
endif


_TEXT$00   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Unlock Dispatcher Database"
;++
;
; VOID
; KiUnlockDispatcherDatabase (
;    IN KIRQL OldIrql
;    )
;
; Routine Description:
;
;    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
;    database locked. Its function is to either unlock the dispatcher
;    database and return or initiate a context switch if another thread
;    has been selected for execution.
;
; Arguments:
;
;    (TOS)   Return address
;
;    (ecx)   OldIrql - Supplies the IRQL when the dispatcher database
;        lock was acquired.
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall KiUnlockDispatcherDatabase, 1

;
; Check if a new thread is scheduled for execution.
;

        cmp     PCR[PcPrcbData+PbNextThread], 0 ; check if next thread
        jne     short Kiu20             ; if ne, new thread scheduled

;
; Release dispatcher database lock, lower IRQL to its previous level,
; and return.
;

Kiu00:                                  ;

ifndef NT_UP

        mov     eax, PCR[PcPrcb]                ; get address of PRCB
        push    ecx                             ; save IRQL for lower IRQL
        lea     ecx, [eax]+PbLockQueue+(8*LockQueueDispatcherLock)
        fstCall KiReleaseQueuedSpinLock
        pop     ecx                             ; get OldIrql

endif

;
; N.B. This exit jumps directly to the lower IRQL routine which has a
;      compatible fastcall interface.
;

        jmp     dword ptr [__imp_@KfLowerIrql@4] ; lower IRQL to previous level

;
; A new thread has been selected to run on the current processor, but
; the new IRQL is not below dispatch level. If the current processor is
; not executing a DPC, then request a dispatch interrupt on the current
; processor.
;

Kiu10:  cmp     dword ptr PCR[PcPrcbData.PbDpcRoutineActive],0  ; check if DPC routine active
        jne     short Kiu00             ; if ne, DPC routine is active

        push    ecx                     ; save new IRQL

ifndef NT_UP

        mov     eax, PCR[PcPrcb]                ; get address of PRCB
        lea     ecx, [eax]+PbLockQueue+(8*LockQueueDispatcherLock)
        fstCall KiReleaseQueuedSpinLock

endif

        mov     cl, DISPATCH_LEVEL      ; request dispatch interrupt
        fstCall HalRequestSoftwareInterrupt ;
        pop     ecx                     ; restore new IRQL

;
; N.B. This exit jumps directly to the lower IRQL routine which has a
;      compatible fastcall interface.
;

        jmp     dword ptr [__imp_@KfLowerIrql@4] ; lower IRQL to previous level

;
; Check if the previous IRQL is less than dispatch level.
;

Kiu20:  cmp     cl, DISPATCH_LEVEL      ; check if IRQL below dispatch level
        jge     short Kiu10             ; if ge, not below dispatch level

;
; There is a new thread scheduled for execution and the previous IRQL is
; less than dispatch level. Context switch to the new thread immediately.
;
;
; N.B. The following registers MUST be saved such that ebp is saved last.
;      This is done so the debugger can find the saved ebp for a thread
;      that is not currently in the running state.
;

.fpo (0, 0, 0, 4, 1, 0)
        sub     esp, 4*4
        mov     [esp+12], ebx           ; save registers
        mov     [esp+8], esi            ;
        mov     [esp+4], edi            ;
        mov     [esp+0], ebp            ;
        mov     ebx, PCR[PcSelfPcr]     ; get address of PCR
        mov     esi, [ebx].PcPrcbData.PbNextThread ; get next thread address
        mov     edi, [ebx].PcPrcbData.PbCurrentThread ; get current thread address
        mov     dword ptr [ebx].PcPrcbData.PbNextThread, 0 ; clear next thread address
        mov     [ebx].PcPrcbData.PbCurrentThread, esi ; set current thread address
        mov     [edi].ThWaitIrql, cl    ; save previous IRQL
        mov     ecx, edi                ; set address of current thread
        fstCall KiReadyThread           ; reready thread for execution
        mov     cl, [edi].ThWaitIrql    ; set APC interrupt bypass disable
        call    SwapContext             ; swap context
        or      al, al                  ; check if kernel APC pending
        mov     cl, [esi].ThWaitIrql    ; get original wait IRQL
        jnz     short Kiu50             ; if nz, kernel APC pending

Kiu30:  mov     ebp, [esp+0]            ; restore registers
        mov     edi, [esp+4]            ;
        mov     esi, [esp+8]            ;
        mov     ebx, [esp+12]           ;
        add     esp, 4*4

;
; N.B. This exit jumps directly to the lower IRQL routine which has a
;      compatible fastcall interface.
;

        jmp     dword ptr [__imp_@KfLowerIrql@4] ; lower IRQL to previous level

Kiu50:  mov     cl, APC_LEVEL           ; lower IRQL to APC level
        fstCall KfLowerIrql             ;
        xor     eax, eax                ; set previous mode to kernel
        stdCall _KiDeliverApc, <eax, eax, eax> ; deliver kernel mode APC
        inc     dword ptr [ebx].PcPrcbData.PbApcBypassCount ; increment count
        xor     ecx, ecx                ; set original wait IRQL
        jmp     short Kiu30

fstENDP KiUnlockDispatcherDatabase

        page ,132
        subttl  "Swap Thread"
;++
;
; VOID
; KiSwapThread (
;    VOID
;    )
;
; Routine Description:
;
;    This routine is called to select the next thread to run on the
;    current processor and to perform a context switch to the thread.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Wait completion status (eax).
;
;--

cPublicFastCall KiSwapThread, 0
.fpo (0, 0, 0, 4, 1, 0)

;
; N.B. The following registers MUST be saved such that ebp is saved last.
;      This is done so the debugger can find the saved ebp for a thread
;      that is not currently in the running state.
;

        sub     esp, 4*4
        mov     [esp+12], ebx           ; save registers
        mov     [esp+8], esi            ;
        mov     [esp+4], edi            ;
        mov     [esp+0], ebp            ;

        mov     ebx, PCR[PcSelfPcr]     ; get address of PCR
        mov     edx, [ebx].PcPrcbData.PbNextThread ; get next thread address
        or      edx, edx                ; check if next thread selected
        jnz     Swt140                  ; if nz, next thread selected

;
; Find the highest nibble in the ready summary that contains a set bit
; and left justify so the nibble is in bits <31:28>
;

        mov     ecx, 16                 ; set base bit number
        mov     edi, _KiReadySummary    ; get ready summary
        mov     esi, edi                ; copy ready summary
        shr     esi, 16                 ; isolate bits <31:16> of summary
        jnz     short Swt10             ; if nz, bits <31:16> are nonzero
        xor     ecx, ecx                ; set base bit number
        mov     esi, edi                ; set bits <15:0> of summary
Swt10:  shr     esi, 8                  ; isolate bits <15:8> of low bits
        jz      short Swt20             ; if z, bits <15:8> are zero
        add     ecx, 8                  ; add offset to nonzero byte
Swt20:  mov     esi, edi                ; isolate highest nonzero byte
        shr     esi, cl                 ;
        add     ecx, 3                  ; adjust to high bit of nibble
        cmp     esi, 10h                ; check if high nibble nonzero
        jb      short Swt30             ; if b, then high nibble is zero
        add     ecx, 4                  ; compute ready queue priority
Swt30:  mov     esi, ecx                ; left justify ready summary nibble
        not     ecx                     ;
        shl     edi, cl                 ;
        or      edi, edi                ;

;
; If the next bit is set in the ready summary, then scan the corresponding
; dispatcher ready queue.
;

Swt40:  js      short Swt60             ; if s, queue contains an entry
Swt50:  sub     esi, 1                  ; decrement ready queue priority
        shl     edi, 1                  ; position next ready summary bit
        jnz     short Swt40             ; if nz, more queues to scan

;
; All ready queues were scanned without finding a runnable thread so
; default to the idle thread and set the appropriate bit in idle summary.
;

ifdef _COLLECT_SWITCH_DATA_

        inc     _KeThreadSwitchCounters + TwSwitchToIdle ; increment counter

endif

ifdef NT_UP

        mov     _KiIdleSummary, 1       ; set idle summary bit

else

        mov     eax, [ebx].PcPrcbData.PbSetMember ; get processor set member
        or      _KiIdleSummary, eax     ; set idle summary bit

endif

        mov     edx, [ebx].PcPrcbData.PbIdleThread ; set idle thread address
        jmp     Swt140                  ;

;
; If the thread can execute on the current processor, then remove it from
; the dispatcher ready queue.
;

        align   4
swt60:  lea     ebp, [esi*8] + _KiDispatcherReadyListHead ; get ready queue address
        mov     ecx, [ebp].LsFlink      ; get address of first queue entry
Swt70:  mov     edx, ecx                ; compute address of thread object
        sub     edx, ThWaitListEntry    ;

ifndef NT_UP

        mov     eax, [edx].ThAffinity   ; get thread affinity
        test    eax, [ebx].PcPrcbData.PbSetMember ; test if compatible affinity
        jnz     short Swt80             ; if nz, thread affinity compatible
        mov     ecx, [ecx].LsFlink      ; get address of next entry
        cmp     ebp, ecx                ; check if end of list
        jnz     short Swt70             ; if nz, not end of list
        jmp     short Swt50             ;

;
; If the thread last ran on the current processor, has been waiting for
; longer than a quantum, or its priority is greater than low realtime
; plus 9, then select the thread. Otherwise, an attempt is made to find
; a more appropriate candidate.
;

        align   4
Swt80:  cmp     _KiThreadSelectNotifyRoutine, 0 ; check for callout routine
        je      short Swt85             ; if eq, no callout routine registered
        push    edx                     ; save volatile registers
        push    ecx                     ;
        mov     ecx, [edx].EtCid.CidUniqueThread ; set trial thread unique id
        call    [_KiThreadSelectNotifyRoutine] ; notify callout routine
        pop     ecx                     ; restore volatile registers
        pop     edx                     ;
        or      eax, eax                ; check if trial thread selectable
        jnz     Swt120                  ; if nz, trial thread selectable
        jmp     Swt87                   ;

        align   4
Swt85:  mov     al, [edx].ThNextProcessor ; get last processor number
        cmp     al, [ebx].PcPrcbData.PbNumber ; check if current processor
        jz      Swt120                  ; if z, same as current processor
        mov     al, [edx].ThIdealProcessor ; get ideal processor number
        cmp     al, [ebx].PcPrcbData.PbNumber ; check if current processor
        jz      short Swt120            ; if z, same as current processor
Swt87:  cmp     esi, LOW_REALTIME_PRIORITY + 9 ; check if priority in range
        jae     short Swt120            ; if ae, priority not in range
        mov     edi, _KeTickCount + 0   ; get low part of tick count
        sub     edi, [edx].ThWaitTime   ; compute length of wait
        cmp     edi, READY_SKIP_QUANTUM + 1 ; check if wait time exceeded
        jae     short Swt120            ; if ae, wait time exceeded
        mov     edi, edx                ; set address of thread

;
; Search forward in the ready queue until the end of the list is reached
; or a more appropriate thread is found.
;

Swt90:  mov     edi, [edi].ThWaitListEntry ; get address of next entry
        cmp     ebp, edi                ; check if end of list
        jz      short Swt120            ; if z, end of list
        sub     edi, ThWaitListEntry    ; compute address of thread
        mov     eax, [edi].ThAffinity   ; get thread affinity
        test    eax, [ebx].PcPrcbData.PbSetMember ; test if compatible infinity
        jz      short Swt100            ; if z, thread affinity not compatible
        cmp     _KiThreadSelectNotifyRoutine, 0 ; check for callout routine
        je      short Swt95             ; if eq, no callout routine registered
        push    edx                     ; save volatile registers
        push    ecx                     ;
        mov     ecx, [edi].EtCid.CidUniqueThread ; set trial thread unique id
        call    [_KiThreadSelectNotifyRoutine] ; notify callout routine
        pop     ecx                     ; restore volatile registers
        pop     edx                     ;
        or      eax, eax                ; check if trial thread selectable
        jnz     short Swt110            ; if nz, trial thread selectable
        jmp     short Swt100            ;

        align   4
Swt95:  mov     al, [edi].ThNextProcessor ; get last processor number
        cmp     al, [ebx].PcPrcbData.PbNumber ; check if current processor
        jz      short Swt110            ; if z, same as current processor
        mov     al, [edi].ThIdealProcessor ; get ideal processor number
        cmp     al, [ebx].PcPrcbData.PbNumber ; check if current processor
        jz      short Swt110            ; if z, same as current processor
Swt100: mov     eax, _KeTickCount + 0   ; get low part of tick count
        sub     eax, [edi].ThWaitTime   ; compute length of wait
        cmp     eax, READY_SKIP_QUANTUM + 1 ; check if wait time exceeded
        jb      short Swt90             ; if b, wait time not exceeded
        jmp     short Swt120            ;

        align   4
Swt110: mov     edx, edi                ; set address of thread
        mov     ecx, edi                ; compute address of list entry
        add     ecx, ThWaitListEntry    ;
Swt120: mov     al, [ebx].PcPrcbData.PbNumber ; get current processor number

ifdef _COLLECT_SWITCH_DATA_

        lea     ebp, _KeThreadSwitchCounters + TwFindIdeal ; get counter address
        cmp     al, [edx].ThIdealProcessor ; check if same as ideal processor
        jz      short Swt130            ; if z, same as ideal processor
        add     ebp, TwFindLast - TwFindIdeal ; compute address of last counter
        cmp     al, [edx].ThNextProcessor ; check if same as last processor
        jz      short Swt130            ; if z, same as last processor
        add     ebp,TwFindAny - TwFindLast ; compute address of correct counter
Swt130: inc     dword ptr [ebp]         ; increment appropriate switch counter

endif

        mov     [edx].ThNextProcessor, al ; set next processor number

endif

;
; Remove the selected thread from the ready queue.
;

        mov     eax, [ecx].LsFlink      ; get list entry forward link
        mov     ebp, [ecx].LsBlink      ; get list entry backward link
        mov     [ebp].LsFlink, eax      ; set forward link in previous entry
        mov     [eax].LsBlink, ebp      ; set backward link in next entry
        cmp     eax, ebp                ; check if list is empty
        jnz     short Swt140            ; if nz, list is not empty
        mov     ebp, 1                  ; clear ready summary bit
        mov     ecx, esi                ;
        shl     ebp, cl                 ;
        xor     _KiReadySummary, ebp    ;

;
; Swap context to the next thread.
;

Swt140: mov     esi, edx                ; set address of next thread
        mov     edi, [ebx].PcPrcbData.PbCurrentThread ; set current thread address
        mov     dword ptr [ebx].PcPrcbData.PbNextThread, 0 ; clear next thread address
        mov     [ebx].PcPrcbData.PbCurrentThread, esi ; set current thread address
        mov     cl, [edi].ThWaitIrql    ; set APC interrupt bypass disable
        call    SwapContext             ; swap context
        or      al, al                  ; check if kernel APC pending
        mov     edi, [esi].ThWaitStatus ; save wait completion status
        mov     cl, [esi].ThWaitIrql    ; get wait IRQL
        jnz     short Swt160            ; if nz, kernel APC pending

Swt150: fstCall KfLowerIrql             ; lower IRQL to previous value

        mov     eax, edi                ; set wait completion status
        mov     ebp, [esp+0]            ; restore registers
        mov     edi, [esp+4]            ;
        mov     esi, [esp+8]            ;
        mov     ebx, [esp+12]           ;
        add     esp, 4*4                ;
        fstRET  KiSwapThread            ;

Swt160: mov     cl, APC_LEVEL           ; lower IRQL to APC level
        fstCall KfLowerIrql             ;
        xor     eax, eax                ; set previous mode to kernel
        stdCall _KiDeliverApc, <eax, eax, eax> ; deliver kernel mode APC
        inc     dword ptr [ebx].PcPrcbData.PbApcBypassCount ; increment count
        xor     ecx, ecx                ; set original wait IRQL
        jmp     short Swt150

fstENDP KiSwapThread

        page ,132
        subttl  "Dispatch Interrupt"
;++
;
; Routine Description:
;
;    This routine is entered as the result of a software interrupt generated
;    at DISPATCH_LEVEL. Its function is to process the Deferred Procedure Call
;    (DPC) list, and then perform a context switch if a new thread has been
;    selected for execution on the processor.
;
;    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
;    database unlocked. When a return to the caller finally occurs, the
;    IRQL remains at DISPATCH_LEVEL, and the dispatcher database is still
;    unlocked.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    None.
;
;--

        align 16
cPublicProc _KiDispatchInterrupt ,0
cPublicFpo 0, 0

        mov     ebx, PCR[PcSelfPcr]     ; get address of PCR
kdi00:  lea     eax, [ebx].PcPrcbData.PbDpcListHead ; get DPC listhead address

;
; Disable interrupts and check if there is any work in the DPC list
; of the current processor.
;

kdi10:  cli                             ; disable interrupts
        cmp     eax, [eax].LsFlink      ; check if DPC List is empty
        je      short kdi40             ; if eq, list is empty
        push    ebp                     ; save register

;
; Exceptions occuring in DPCs are unrelated to any exception handlers
; in the interrupted thread.  Terminate the exception list.
;

        push    [ebx].PcExceptionList
        mov     [ebx].PcExceptionList, EXCEPTION_CHAIN_END

;
; Switch to the DPC stack for this processor.
;

        mov     edx, esp
        mov     esp, [ebx].PcPrcbData.PbDpcStack
        push    edx

.fpo (0, 0, 0, 1, 1, 0)

        mov     ebp, eax                ; set address of DPC listhead
        call    KiRetireDpcList         ; process the current DPC list

;
; Switch back to the current thread stack, restore the exception list
; and saved EBP.
;

        pop     esp
	pop     [ebx].PcExceptionList
        pop     ebp

.fpo (0, 0, 0, 0, 0, 0)

;
; Check to determine if quantum end is requested.
;
; N.B. If a new thread is selected as a result of processing the quantum
;      end request, then the new thread is returned with the dispatcher
;      database locked. Otherwise, NULL is returned with the dispatcher
;      database unlocked.
;

kdi40:  sti                             ; enable interrupts
        cmp     dword ptr [ebx].PcPrcbData.PbQuantumEnd, 0 ; quantum end requested
        jne     kdi90                   ; if neq, quantum end request

;
; Check to determine if a new thread has been selected for execution on this
; processor.
;

        cmp     dword ptr [ebx].PcPrcbData.PbNextThread, 0 ; check addr of next thread object
        je      short kdi70             ; if eq, then no new thread

;
; Disable interrupts and attempt to acquire the dispatcher database lock.
;

ifndef NT_UP

        cli
        cmp     dword ptr _KiDispatcherLock, 0
        jnz     short kdi80
        lea     ecx, [ebx]+PcPrcbData+PbLockQueue+(8*LockQueueDispatcherLock)
        fstCall KiTryToAcquireQueuedSpinLock
        jz      short kdi80             ; jif not acquired
        mov ecx, SYNCH_LEVEL            ; raise IRQL to synchronization level
        fstCall KfRaiseIrql             ;
        sti                             ; enable interrupts

endif

        mov     eax, [ebx].PcPrcbData.PbNextThread ; get next thread address

;
; N.B. The following registers MUST be saved such that ebp is saved last.
;      This is done so the debugger can find the saved ebp for a thread
;      that is not currently in the running state.
;

.fpo (0, 0, 0, 3, 1, 0)

kdi60:  sub     esp, 3*4
        mov     [esp+8], esi            ; save registers
        mov     [esp+4], edi            ;
        mov     [esp+0], ebp            ;
        mov     esi, eax                ; set next thread address
        mov     edi, [ebx].PcPrcbData.PbCurrentThread ; get current thread address
        mov     dword ptr [ebx].PcPrcbData.PbNextThread, 0 ; clear next thread address
        mov     [ebx].PcPrcbData.PbCurrentThread, esi ; set current thread address
        mov     ecx, edi                ; set address of current thread
        fstCall KiReadyThread           ; ready thread (ecx) for execution
        mov     cl, 1                   ; set APC interrupt bypass disable
        call    SwapContext             ; call context swap routine
        mov     ebp, [esp+0]            ; restore registers
        mov     edi, [esp+4]            ;
        mov     esi, [esp+8]            ;
        add     esp, 3*4
kdi70:  stdRET  _KiDispatchInterrupt    ; return

;
; Enable interrupts and check DPC queue.
;

ifndef NT_UP

kdi80: sti                              ; enable interrupts
       YIELD                            ; rest
       jmp     kdi00                    ;

endif

;
; Process quantum end event.
;
; N.B. If the quantum end code returns a NULL value, then no next thread
;      has been selected for execution. Otherwise, a next thread has been
;      selected and the dispatcher databased is locked.
;

kdi90:  mov     dword ptr [ebx].PcPrcbData.PbQuantumEnd, 0 ; clear quantum end indicator
        stdCall _KiQuantumEnd           ; process quantum end
        or      eax, eax                ; check if new thread selected
        jne     short kdi60             ; if ne, new thread selected
        stdRET  _KiDispatchInterrupt    ; return

stdENDP _KiDispatchInterrupt

        page ,132
        subttl  "Swap Context to Next Thread"
;++
;
; Routine Description:
;
;    This routine is called to swap context from one thread to the next.
;    It swaps context, flushes the data, instruction, and translation
;    buffer caches, restores nonvolatile integer registers, and returns
;    to its caller.
;
;    N.B. It is assumed that the caller (only callers are within this
;         module) saved the nonvolatile registers, ebx, esi, edi, and
;         ebp. This enables the caller to have more registers available.
;
; Arguments:
;
;    cl - APC interrupt bypass disable (zero enable, nonzero disable).
;    edi - Address of previous thread.
;    esi - Address of next thread.
;    ebx - Address of PCR.
;
; Return value:
;
;    al - Kernel APC pending.
;    ebx - Address of PCR.
;    esi - Address of current thread object.
;
;--

        align   16
        public  SwapContext

ifndef NT_UP
        public  _ScPatchFxb
        public  _ScPatchFxe
endif

SwapContext     proc


;
;   NOTE:   The ES: override on the move to ThState is part of the
;           lazy-segment load system.  It assures that ES has a valid
;           selector in it, thus preventing us from propagating a bad
;           ES accross a context switch.
;
;           Note that if segments, other than the standard flat segments,
;           with limits above 2 gig exist, neither this nor the rest of
;           lazy segment loads are reliable.
;
; Note that ThState must be set before the dispatcher lock is released
; to prevent KiSetPriorityThread from seeing a stale value.
;

;
; Save the APC disable flag and set new thread state to running.
;

        or      cl, cl                  ; set zf in flags
        mov     byte ptr es:[esi]+ThState, Running ; set thread state to running
        pushfd
cPublicFpo 0, 1


;
; Acquire the context swap lock so the address space of the old process
; cannot be deleted and then release the dispatcher database lock.
;
; N.B. This lock is used to protect the address space until the context
;    switch has sufficiently progressed to the point where the address
;    space is no longer needed. This lock is also acquired by the reaper
;    thread before it finishes thread termination.
;

ifndef NT_UP

        lea     ecx, [ebx]+PcPrcbData+PbLockQueue+(8*LockQueueContextSwapLock)
        fstCall KiAcquireQueuedSpinLock ; Acquire ContextSwap lock

        lea     ecx, [ebx]+PcPrcbData+PbLockQueue+(8*LockQueueDispatcherLock)
        fstCall KiReleaseQueuedSpinLock ; release Dispatcher Lock

endif

;
; Save the APC disable flag and the exception listhead.
; (also, check for DPC running which is illegal right now).
;

        mov     ecx, [ebx]+PcExceptionList ; save exception list
        cmp     [ebx]+PcPrcbData+PbDpcRoutineActive, 0
        push    ecx
cPublicFpo 0, 2
        jne     sc91                    ; bugcheck if DPC active.

;
; Notify registered callout routine of swap context.
;

ifndef NT_UP

        cmp     _KiSwapContextNotifyRoutine, 0 ; check for callout routine
        jne     sc92                    ; if ne, callout routine registered
sc03:

endif

;
; Accumulate the total time spent in a thread.
;

ifdef PERF_DATA

        test    _KeFeatureBits, KF_RDTSC ; feature supported?
        jz      short @f                 ; if z, feature not present

.586p
        rdtsc                            ; read cycle counter
.486p

        sub     eax, [ebx].PcPrcbData.PbThreadStartCount.LiLowPart ; sub off thread
        sbb     edx, [ebx].PcPrcbData.PbThreadStartCount.LiHighPart ; starting time
        add     [edi].EtPerformanceCountLow, eax ; accumlate thread run time
        adc     [edi].EtPerformanceCountHigh, edx ;
        add     [ebx].PcPrcbData.PbThreadStartCount.LiLowPart, eax ; set new thread
        adc     [ebx].PcPrcbData.PbThreadStartCount.LiHighPart, edx ; starting time
@@:                                     ;

endif

;
; On a uniprocessor system the NPX state is swapped in a lazy manner.
; If a thread whose state is not in the coprocessor attempts to perform
; a coprocessor operation, the current NPX state is swapped out (if needed),
; and the new state is swapped in durning the fault.  (KiTrap07)
;
; On a multiprocessor system we still fault in the NPX state on demand, but
; we save the state when the thread switches out (assuming the NPX state
; was loaded).  This is because it could be difficult to obtain the thread's
; NPX in the trap handler if it was loaded into a different processor's
; coprocessor.
;
        mov     ebp, cr0                ; get current CR0
        mov     edx, ebp

ifndef NT_UP
        cmp     byte ptr [edi]+ThNpxState, NPX_STATE_LOADED ; check if NPX state
        je      sc_save_npx_state
endif


sc05:   mov     cl, [esi]+ThDebugActive ; get debugger active state
        mov     [ebx]+PcDebugActive, cl ; set new debugger active state

;
; Switch stacks:
;
;   1.  Save old esp in old thread object.
;   2.  Copy stack base and stack limit into TSS AND PCR
;   3.  Load esp from new thread object
;
; Keep interrupts off so we don't confuse the trap handler into thinking
; we've overrun the kernel stack.
;

        cli                             ; disable interrupts
        mov     [edi]+ThKernelStack, esp ; save old kernel stack pointer
        mov     eax, [esi]+ThInitialStack ; get new initial stack pointer
        mov     ecx, [esi]+ThStackLimit ; get stack limit
        sub     eax, NPX_FRAME_LENGTH   ; space for NPX_FRAME & NPX CR0 flags
        mov     [ebx]+PcStackLimit, ecx ; set new stack limit
        mov     [ebx]+PcInitialStack, eax ; set new stack base

.errnz (NPX_STATE_NOT_LOADED - CR0_TS - CR0_MP)
.errnz (NPX_STATE_LOADED - 0)

; (eax) = Initial Stack
; (ebx) = Prcb
; (edi) = OldThread
; (esi) = NewThread
; (ebp) = Current CR0
; (edx) = Current CR0

        xor     ecx, ecx
        mov     cl, [esi]+ThNpxState            ; New NPX state is (or is not) loaded

        and     edx, NOT (CR0_MP+CR0_EM+CR0_TS) ; clear thread settable NPX bits
        or      ecx, edx                        ; or in new thread's cr0
        or      ecx, [eax]+FpCr0NpxState        ; merge new thread settable state
        cmp     ebp, ecx                ; check if old and new CR0 match
        jne     sc_reload_cr0           ; if ne, no change in CR0

;
; N.B. It is important that the following adjustment NOT be applied to
;      the initial stack value in the PCR. If it is, it will cause the
;      location in memory that the processor pushes the V86 mode segment
;      registers and the first 4 ULONGs in the FLOATING_SAVE_AREA to
;      occupy the same memory locations, which could result in either
;      trashed segment registers in V86 mode, or a trashed NPX state.
;
;      Adjust ESP0 so that V86 mode threads and 32 bit threads can share
;      a trapframe structure, and the NPX save area will be accessible
;      in the same manner on all threads
;
;      This test will check the user mode flags. On threads with no user
;      mode context, the value of esp0 does not matter (we will never run
;      in user mode without a usermode context, and if we don't run in user
;      mode the processor will never use the esp0 value.
;

        align   4
sc06:   test    dword ptr [eax] - KTRAP_FRAME_LENGTH + TsEFlags, EFLAGS_V86_MASK
        jnz     short sc07              ; if nz, V86 frame, no adjustment
        sub     eax, TsV86Gs - TsHardwareSegSs ; bias for missing fields
sc07:   mov     ecx, [ebx]+PcTss        ;
        mov     [ecx]+TssEsp0, eax      ;
        mov     esp, [esi]+ThKernelStack ; set new stack pointer
        mov     eax, [esi]+ThTeb        ; get user TEB address
        mov     [ebx]+PcTeb, eax        ; set user TEB address

;
; Edit the TEB descriptor to point to the TEB
;

        sti                             ; enable interrupts
        mov     ecx, [ebx]+PcGdt        ;
        mov     [ecx]+(KGDT_R3_TEB+KgdtBaseLow), ax  ;
        shr     eax, 16                 ;
        mov     [ecx]+(KGDT_R3_TEB+KgdtBaseMid), al  ;
        shr     eax, 8
        mov     [ecx]+(KGDT_R3_TEB+KgdtBaseHi), al

;
; NOTE: Keep KiSwapProcess (below) in sync with this code!
;
; If the new process is not the same as the old process, then switch the
; address space to the new process.
;

        mov     eax, [edi].ThApcState.AsProcess ; get old process address
        cmp     eax, [esi].ThApcState.AsProcess ; check if process match
        jz      short sc22                      ; if z, old and new process match
        mov     edi, [esi].ThApcState.AsProcess ; get new process address

;
; Update the processor set masks.
;

ifndef NT_UP

if DBG

        mov     cl, [esi]+ThNextProcessor ; get current processor number
        cmp     cl, [ebx]+PcPrcbData+PbNumber ; same as running processor?
        jne     sc_error2               ; if ne, processor number mismatch

endif

        mov     ecx, [ebx]+PcSetMember  ; get processor set member
        xor     [eax]+PrActiveProcessors, ecx ; clear bit in old processor set
        xor     [edi]+PrActiveProcessors, ecx ; set bit in new processor set

if DBG
        test    [eax]+PrActiveProcessors, ecx ; test if bit clear in old set
        jnz     sc_error4               ; if nz, bit not clear in old set
        test    [edi]+PrActiveProcessors, ecx ; test if bit set in new set
        jz      sc_error5               ; if z, bit not set in new set

endif
endif

;
; New CR3, flush tb, sync tss, set IOPM
; CS, SS, DS, ES all have flat (GDT) selectors in them.
; FS has the pcr selector.
; Therefore, GS is only selector we need to flush.  We null it out,
; it will be reloaded from a stack frame somewhere above us.
; Note: this load of GS before CR3 works around P6 step B0 errata 11
;

        xor     eax, eax                ; assume null ldt
        mov     gs, ax                  ;
        mov     eax, [edi]+PrDirectoryTableBase ; get new directory base
        mov     ebp, [ebx]+PcTss        ; get new TSS
        mov     ecx, [edi]+PrIopmOffset ; get IOPM offset
        mov     [ebp]+TssCR3, eax       ; make TSS be in sync with hardware
        mov     cr3, eax                ; flush TLB and set new directory base
        mov     [ebp]+TssIoMapBase, cx  ;

;
; LDT switch
;

        xor     eax, eax                ; check if null LDT limit
        cmp     word ptr [edi]+PrLdtDescriptor, ax
        jnz     short sc_load_ldt       ; if nz, LDT limit

        lldt    ax                      ; set LDT

;
; Release the context swap lock.
;

        align   4
sc22:                                   ;

ifndef NT_UP

        lea     ecx, [ebx]+PcPrcbData+PbLockQueue+(8*LockQueueContextSwapLock)
        fstCall KiReleaseQueuedSpinLock

endif

;
; Update context switch counters.
;

        inc     dword ptr [esi]+ThContextSwitches ; thread count
        inc     dword ptr [ebx]+PcPrcbData+PbContextSwitches ; processor count
        pop     ecx                     ; restore exception list
        mov     [ebx].PcExceptionList, ecx ;

;
; If the new thread has a kernel mode APC pending, then request an APC
; interrupt.
;

        cmp     byte ptr [esi].ThApcState.AsKernelApcPending, 0 ; APC pending?
        jne     short sc80              ; if ne, kernel APC pending
        popfd                           ; restore flags
        xor     eax, eax                ; clear kernel APC pending
        ret                             ; return

;
; The new thread has an APC interrupt pending. If APC interrupt bypass is
; enable, then return kernel APC pending. Otherwise, request a software
; interrupt at APC_LEVEL and return no kernel APC pending.
;

sc80:   popfd                           ; restore flags
        jnz     short sc90              ; if nz, APC interupt bypass disabled
        mov     al, 1                   ; set kernel APC pending
        ret                             ;

sc90:   mov     cl, APC_LEVEL           ; request software interrupt level
        fstCall HalRequestSoftwareInterrupt ;
        xor     eax, eax                ; clear kernel APC pending
        ret                             ;

;
; Set for new LDT value
;

sc_load_ldt:
        mov     ebp, [ebx]+PcGdt        ;
        mov     eax, [edi+PrLdtDescriptor] ;
        mov     [ebp+KGDT_LDT], eax     ;
        mov     eax, [edi+PrLdtDescriptor+4] ;
        mov     [ebp+KGDT_LDT+4], eax   ;
        mov     eax, KGDT_LDT           ;

;
; Set up int 21 descriptor of IDT.  If the process does not have an Ldt, it
; should never make any int 21 calls.  If it does, an exception is generated. If
; the process has an Ldt, we need to update int21 entry of LDT for the process.
; Note the Int21Descriptor of the process may simply indicate an invalid
; entry.  In which case, the int 21 will be trapped to the kernel.
;

        mov     ebp, [ebx]+PcIdt        ;
        mov     ecx, [edi+PrInt21Descriptor] ;
        mov     [ebp+21h*8], ecx        ;
        mov     ecx, [edi+PrInt21Descriptor+4] ;
        mov     [ebp+21h*8+4], ecx      ;
        lldt    ax                      ; set LDT
        jmp     short sc22

;
; Cr0 has changed (ie, floating point processor present), load the new value.
;

sc_reload_cr0:
if DBG

        test    byte ptr [esi]+ThNpxState, NOT (CR0_TS+CR0_MP)
        jnz     sc_error                ;
        test    dword ptr [eax]+FpCr0NpxState, NOT (CR0_PE+CR0_MP+CR0_EM+CR0_TS)
        jnz     sc_error3               ;

endif
        mov     cr0,ecx                 ; set new CR0 NPX state
        jmp     sc06


ifndef NT_UP


; Save coprocessor's current context.  FpCr0NpxState is the current thread's
; CR0 state.  The following bits are valid: CR0_MP, CR0_EM, CR0_TS.  MVDMs
; may set and clear MP & EM as they please and the settings will be reloaded
; on a context switch (but they will not be saved from CR0 to Cr0NpxState).
; The kernel sets and clears TS as required.
;
; (ebp) = Current CR0
; (edx) = Current CR0

sc_save_npx_state:
        and     edx, NOT (CR0_MP+CR0_EM+CR0_TS) ; we need access to the NPX state

        mov     ecx,[ebx]+PcInitialStack        ; get NPX save save area address

        cmp     ebp, edx                        ; Does CR0 need reloading?
        je      short sc_npx10

        mov     cr0, edx                        ; set new cr0
        mov     ebp, edx                        ; (ebp) = (edx) = current cr0 state

sc_npx10:
;
; The fwait following the fnsave is to make sure that the fnsave has stored the
; data into the save area before this coprocessor state could possibly be
; context switched in and used on a different (co)processor.  I've added the
; clocks from when the dispatcher lock is released and don't believe it's a
; possibility.  I've also timed the impact this fwait seems to have on a 486
; when performing lots of numeric calculations.  It appears as if there is
; nothing to wait for after the fnsave (although the 486 manual says there is)
; and therefore the calculation time far outweighed the 3clk fwait and it
; didn't make a noticable difference.
;

;
; If FXSR feature is NOT present on the processor, the fxsave instruction is
; patched at boot time to start using fnsave instead
;

_ScPatchFxb:
;       fxsave  [ecx]                   ; save NPX state
        db      0FH, 0AEH, 01
_ScPatchFxe:

        mov     byte ptr [edi]+ThNpxState, NPX_STATE_NOT_LOADED ; set no NPX state
        mov     dword ptr [ebx].PcPrcbData+PbNpxThread, 0  ; clear npx owner
        jmp     sc05

;
; Notify context swap callout routine.  This code is out of line to
; optimize the normal case (which is expected to be the case where
; there is no callout routine.
;

sc92:   mov     edx, [esi].EtCid.CidUniqueThread ; set new thread unique id
        mov     ecx, [edi].EtCid.CidUniqueThread ; set old thread unique id
        call    [_KiSwapContextNotifyRoutine] ; notify callout routine
        jmp     sc03

endif

.fpo (2, 0, 0, 0, 0, 0)
sc91:   stdCall _KeBugCheck <ATTEMPTED_SWITCH_FROM_DPC>
        ret                             ; return

if DBG
sc_error5:  int 3
sc_error4:  int 3
sc_error3:  int 3
sc_error2:  int 3
sc_error:   int 3
endif

SwapContext     endp

        page , 132
        subttl "Interlocked Swap PTE"

;++
;
; HARDWARE_PTE
; KeInterlockedSwapPte (
;     IN PKI_INTERNAL_PTE PtePointer,
;     IN PKI_INTERNAL_PTE NewPteContents
;     )
;
; Routine Description:
;
;     This function performs an interlocked swap of a PTE.  This is only needed
;     for the PAE architecture where the PTE width is larger than the register
;     width.
;
;     Both PTEs must be valid or a careful write would have been done instead.
;
; Arguments:
;
;     PtePointer - Address of Pte to update with new value.
;
;     NewPteContents - Pointer to the new value to put in the Pte.  Will simply
;         be assigned to *PtePointer, in a fashion correct for the hardware.
;
; Return Value:
;
;     Returns the contents of the PtePointer before the new value
;     is stored.
;
;--

cPublicProc _KeInterlockedSwapPte ,2

    push    ebx
    push    esi

    mov     ebx, [esp] + 16         ; ebx = NewPteContents
    mov     esi, [esp] + 12         ; esi = PtePointer

    mov     ecx, [ebx] + 4
    mov     ebx, [ebx]              ; ecx:ebx = source pte contents

    mov     edx, [esi] + 4
    mov     eax, [esi]              ; edx:eax = target pte contents

swapagain:

    ;
    ; cmpxchg loads edx:eax with the new contents of the target quadword
    ; in the event of failure
    ;

.586
ifndef NT_UP
    lock cmpxchg8b qword ptr [esi]  ; compare and exchange
else
    cmpxchg8b qword ptr [esi]       ; compare and exchange
endif
.386

    jnz     short swapagain         ; if z clear, exchange failed

    pop     esi
    pop     ebx

    stdRET   _KeInterlockedSwapPte
stdENDP _KeInterlockedSwapPte

        page , 132
        subttl "Flush EntireTranslation Buffer"
;++
;
; VOID
; KeFlushCurrentTb (
;     )
;
; Routine Description:
;
;     This function flushes the entire translation buffer (TB) on the current
;     processor and also flushes the data cache if an entry in the translation
;     buffer has become invalid.
;
; Arguments:
;
; Return Value:
;
;     None.
;
;--

cPublicProc _KeFlushCurrentTb ,0

ktb00:  mov     eax, cr3                ; (eax) = directory table base
        mov     cr3, eax                ; flush TLB
        stdRET    _KeFlushCurrentTb

.586p
ktb_gb: mov     eax, cr4                ; *** see Ki386EnableGlobalPage ***
        and     eax, not CR4_PGE        ; This FlushCurrentTb version gets copied into
        mov     cr4, eax                ; ktb00 at initialization time if needed.
        or      eax, CR4_PGE
        mov     cr4, eax
ktb_eb: stdRET    _KeFlushCurrentTb
.486p

stdENDP _KeFlushCurrentTb
        ;;
        ;; moved KiFlushDcache below KeFlushCurrentTb for BBT purposes.  BBT
        ;; needs an end label to treat KeFlushCurrentTb as data and to keep together.
        ;;
        page , 132
        subttl "Flush Data Cache"
;++
;
; VOID
; KiFlushDcache (
;     )
;
; VOID
; KiFlushIcache (
;     )
;
; Routine Description:
;
;   This routine does nothing on i386 and i486 systems.   Why?  Because
;   (a) their caches are completely transparent,  (b) they don't have
;   instructions to flush their caches.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     None.
;
;--

cPublicProc _KiFlushDcache  ,0
cPublicProc _KiFlushIcache  ,0

        stdRET    _KiFlushIcache

stdENDP _KiFlushIcache
stdENDP _KiFlushDcache


_TEXT$00   ends

INIT    SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; Ki386EnableGlobalPage (
;     IN volatile PLONG Number
;     )
;
; /*++
;
; Routine Description:
;
;     This routine enables the global page PDE/PTE support in the system,
;     and stalls until complete and them sets the current processor's cr4
;     register to enable global page support.
;
; Arguments:
;
;     Number - Supplies a pointer to the count of the number of processors in
;     the configuration.
;
; Return Value:
;
;     None.
;--

cPublicProc _Ki386EnableGlobalPage,1
        push    esi
        push    edi
        push    ebx

        mov     edx, [esp+16]           ; pointer to Number
        pushfd
        cli

;
; Wait for all processors
;
        lock dec dword ptr [edx]        ; count down
egp10:  YIELD
        cmp     dword ptr [edx], 0      ; wait for all processors to signal
        jnz     short egp10

        cmp     PCR[PcNumber], 0        ; processor 0?
        jne     short egp20

;
; Install proper KeFlushCurrentTb function.
;

        mov     edi, ktb00
        mov     esi, ktb_gb
        mov     ecx, ktb_eb - ktb_gb + 1
        rep movsb

        mov     byte ptr [ktb_eb], 0

;
; Wait for P0 to signal that proper flush TB handlers have been installed
;
egp20:  cmp     byte ptr [ktb_eb], 0
        jnz     short egp20

;
; Flush TB, and enable global page support
; (note load of CR4 is explicitly done before the load of CR3
; to work around P6 step B0 errata 11)
;
.586p
        mov     eax, cr4
        and     eax, not CR4_PGE        ; should not be set, but let's be safe
        mov     ecx, cr3
        mov     cr4, eax

        mov     cr3, ecx                ; Flush TB

        or      eax, CR4_PGE            ; enable global TBs
        mov     cr4, eax
.486p
        popfd
        pop     ebx
        pop     edi
        pop     esi

        stdRET  _Ki386EnableGlobalPage
stdENDP _Ki386EnableGlobalPage

;++
;
; VOID
; Ki386EnableFxsr (
;     IN volatile PLONG Number
;     )
;
; /*++
;
; Routine Description:
;
;     This routine sets OSFXSR bit in CR4 to indicate that OS supports
;     FXSAVE/FXRSTOR for use during context switches
;
; Arguments:
;
;     Number - Supplies a pointer to the count of the number of processors in
;     the configuration.
;
; Return Value:
;
;     None.
;--

cPublicProc _Ki386EnableFxsr,1

.586p
        mov     eax, cr4
        or      eax, CR4_FXSR
        mov     cr4, eax
.386p

        stdRET  _Ki386EnableFxsr
stdENDP _Ki386EnableFxsr


;++
;
; VOID
; Ki386EnableXMMIExceptions (
;     IN volatile PLONG Number
;     )
;
; /*++
;
; Routine Description:
;
;     This routine installs int 19 XMMI unmasked Numeric Exception handler
;     and sets OSXMMEXCPT bit in CR4 to indicate that OS supports
;     unmasked Katmai New Instruction technology exceptions.
;
; Arguments:
;
;     Number - Supplies a pointer to count of the number of processors in
;     the configuration.
;
; Return Value:
;
;     None.
;--

cPublicProc _Ki386EnableXMMIExceptions,1


        ;Set up IDT for INT19
        mov     ecx,PCR[PcIdt]              ;Get IDT address
        lea     eax, [ecx] + 098h           ;XMMI exception is int 19
        mov     byte ptr [eax + 5], 08eh    ;P=1,DPL=0,Type=e
        mov     word ptr [eax + 2], KGDT_R0_CODE ;Kernel code selector
        mov     edx, offset FLAT:_KiTrap13  ;Address of int 19 handler
        mov     ecx,edx
        mov     word ptr [eax],cx           ;addr moves into low byte
        shr     ecx,16
        mov     word ptr [eax + 6],cx       ;addr moves into high byte
.586p
        ;Enable XMMI exception handling
        mov     eax, cr4
        or      eax, CR4_XMMEXCPT
        mov     cr4, eax
.386p

        stdRET  _Ki386EnableXMMIExceptions
stdENDP _Ki386EnableXMMIExceptions


;++
;
; VOID
; Ki386EnableCurrentLargePage (
;     IN ULONG IdentityAddr,
;     IN ULONG IdentityCr3
;     )
;
; /*++
;
; Routine Description:
;
;     This routine enables the large page PDE support in the processor.
;
; Arguments:
;
;     IdentityAddr - Supplies the linear address of the beginning of this
;     function where (linear == physical).
;
;     IdentityCr3 - Supplies a pointer to the temporary page directory and
;     page tables that provide both the kernel (virtual ->physical) and
;     identity (linear->physical) mappings needed for this function.
;
; Return Value:
;
;     None.
;--

public _Ki386EnableCurrentLargePageEnd
cPublicProc _Ki386EnableCurrentLargePage,2
        mov     ecx,[esp]+4             ; (ecx)-> IdentityAddr
        mov     edx,[esp]+8             ; (edx)-> IdentityCr3
        pushfd                          ; save current IF state
        cli                             ; disable interrupts

        mov     eax, cr3                ; (eax)-> original Cr3
        mov     cr3, edx                ; load Cr3 with Identity mapping

        sub     ecx, offset _Ki386EnableCurrentLargePage
        add     ecx, offset _Ki386LargePageIdentityLabel
        jmp     ecx                     ; jump to (linear == physical)

_Ki386LargePageIdentityLabel:
        mov    ecx, cr0
        and    ecx, NOT CR0_PG          ; clear PG bit to disable paging
        mov    cr0, ecx                 ; disable paging
        jmp    $+2

.586p
        mov     edx, cr4
        or      edx, CR4_PSE            ; enable Page Size Extensions
        mov     cr4, edx

.486p
        mov     edx, offset OriginalMapping
        or      ecx, CR0_PG             ; set PG bit to enable paging
        mov     cr0, ecx                ; enable paging
        jmp     edx                     ; Return to original mapping.

OriginalMapping:
        mov     cr3, eax                ; restore original Cr3
        popfd                           ; restore interrupts to previous

        stdRET  _Ki386EnableCurrentLargePage

_Ki386EnableCurrentLargePageEnd:

stdENDP _Ki386EnableCurrentLargePage

INIT    ends

_TEXT$00   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page , 132
        subttl "Flush Single Translation Buffer"
;++
;
; VOID
; FASTCALL
; KiFlushSingleTb (
;     IN BOOLEAN Invalid,
;     IN PVOID Virtual
;     )
;
; Routine Description:
;
;     This function flushes a single TB entry.
;
;     It only works on a 486 or greater.
;
; Arguments:
;
;     Invalid - Supplies a boolean value that specifies the reason for
;               flushing the translation buffer.
;
;     Virtual - Supplies the virtual address of the single entry that is
;               to be flushed from the translation buffer.
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KiFlushSingleTb ,2

;
; 486 or above code
;
        invlpg  [edx]
        fstRET  KiFlushSingleTb

fstENDP KiFlushSingleTb

        page , 132
        subttl "Swap Process"
;++
;
; VOID
; KiSwapProcess (
;     IN PKPROCESS NewProcess,
;     IN PKPROCESS OldProcess
;     )
;
; Routine Description:
;
;     This function swaps the address space to another process by flushing
;     the data cache, the instruction cache, the translation buffer, and
;     establishes a new directory table base.
;
;     It also swaps in the LDT and IOPM of the new process.  This is necessary
;     to avoid bogus mismatches in SwapContext.
;
;     NOTE: keep in sync with process switch part of SwapContext
;
; Arguments:
;
;     Process - Supplies a pointer to a control object of type process.
;
; Return Value:
;
;     None.
;
;--

cPublicProc _KiSwapProcess  ,2
cPublicFpo 2, 0

ifndef NT_UP

;
; Acquire the context swap lock.
;

        mov     ecx, PCR[PcPrcb]
        lea     ecx, [ecx]+PbLockQueue+(8*LockQueueContextSwapLock)
        fstCall KiAcquireQueuedSpinLock

        mov     edx,[esp]+4             ; (edx)-> New process
        mov     eax,[esp]+8             ; (eax)-> Old Process

;
; Clear the processor set member in the old process, set the processor
; member in the new process, and release the context swap lock.
;

        mov     ecx, PCR[PcSetMember]
        xor     [eax]+PrActiveProcessors,ecx ; clear bit in old processor set
        xor     [edx]+PrActiveProcessors,ecx ; set bit in new processor set

if DBG

        test    [eax]+PrActiveProcessors,ecx ; test if bit clear in old set
        jnz     kisp_error              ; if nz, bit not clear in old set
        test    [edx]+PrActiveProcessors,ecx ; test if bit set in new set
        jz      kisp_error1             ; if z, bit not set in new set

endif

        mov     ecx, PCR[PcPrcb]
        lea     ecx, [ecx]+PbLockQueue+(8*LockQueueContextSwapLock)
        fstCall KiReleaseQueuedSpinLock

endif

        mov     ecx,PCR[PcTss]          ; (ecx)-> TSS
        mov     edx,[esp]+4             ; (edx)-> New process

;
;   Change address space
;

        xor     eax,eax                         ; assume ldtr is to be NULL
        mov     gs,ax                           ; Clear gs.  (also workarounds
                                                ; P6 step B0 errata 11)
        mov     eax,[edx]+PrDirectoryTableBase
        mov     [ecx]+TssCR3,eax        ; be sure TSS in sync with processor
        mov     cr3,eax

;
;   Change IOPM
;

        mov     ax,[edx]+PrIopmOffset
        mov     [ecx]+TssIoMapBase,ax

;
;   Change LDT
;

        xor     eax, eax
        cmp     word ptr [edx]+PrLdtDescriptor,ax ; limit 0?
        jz      short kisp10                    ; null LDT, go load NULL ldtr

;
;   Edit LDT descriptor
;

        mov     ecx,PCR[PcGdt]
        mov     eax,[edx+PrLdtDescriptor]
        mov     [ecx+KGDT_LDT],eax
        mov     eax,[edx+PrLdtDescriptor+4]
        mov     [ecx+KGDT_LDT+4],eax

;
;   Set up int 21 descriptor of IDT.  If the process does not have Ldt, it
;   should never make any int 21 call.  If it does, an exception is generated.
;   If the process has Ldt, we need to update int21 entry of LDT for the process.
;   Note the Int21Descriptor of the process may simply indicate an invalid
;   entry.  In which case, the int 21 will be trapped to the kernel.
;

        mov     ecx, PCR[PcIdt]
        mov     eax, [edx+PrInt21Descriptor]
        mov     [ecx+21h*8], eax
        mov     eax, [edx+PrInt21Descriptor+4]
        mov     [ecx+21h*8+4], eax

        mov     eax,KGDT_LDT                    ;@@32-bit op to avoid prefix

;
;   Load LDTR
;

kisp10: lldt    ax
        stdRET    _KiSwapProcess

if DBG
kisp_error1: int 3
kisp_error:  int 3
endif

stdENDP _KiSwapProcess

        page , 132
        subttl "Adjust TSS ESP0 value"
;++
;
; VOID
; KiAdjustEsp0 (
;     IN PKTRAP_FRAME TrapFrame
;     )
;
; Routine Description:
;
;     This routine puts the apropriate ESP0 value in the esp0 field of the
;     TSS.  This allows protect mode and V86 mode to use the same stack
;     frame.  The ESP0 value for protected mode is 16 bytes lower than
;     for V86 mode to compensate for the missing segment registers.
;
; Arguments:
;
;     TrapFrame - Supplies a pointer to the TrapFrame.
;
; Return Value:
;
;     None.
;
;--
cPublicProc _Ki386AdjustEsp0 ,1

        stdCall _KeGetCurrentThread

        mov     edx,[esp + 4]                   ; edx -> trap frame
        mov     eax,[eax]+thInitialStack        ; eax = base of stack
        test    dword ptr [edx]+TsEFlags,EFLAGS_V86_MASK  ; is this a V86 frame?
        jnz     short ae10

        sub     eax,TsV86Gs - TsHardwareSegSS   ; compensate for missing regs
ae10:   sub     eax,NPX_FRAME_LENGTH
        pushfd                                  ; Make sure we don't move
        cli                                     ; processors while we do this
        mov     edx,PCR[PcTss]
        mov     [edx]+TssEsp0,eax               ; set Esp0 value
        popfd
        stdRET    _Ki386AdjustEsp0

stdENDP _Ki386AdjustEsp0

_TEXT$00   ends
        end
