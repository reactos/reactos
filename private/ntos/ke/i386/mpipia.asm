        title  "mpipia"
;++
;
; Copyright (c) 1989-1995  Microsoft Corporation
;
; Module Name:
;
;    mpipia.asm
;
; Abstract:
;
;    This module implements the x86 specific fucntions required to
;    support multiprocessor systems.
;
; Author:
;
;    David N. Cutler (davec) 5-Feb-1995
;
; Environment:
;
;    Krnel mode only.
;
; Revision History:
;
;--

.486p
        .xlist
include ks386.inc
include mac386.inc
include callconv.inc
        .list

        EXTRNP  HalRequestSoftwareInterrupt,1,IMPORT,FASTCALL
        EXTRNP  HalRequestSoftwareInterrupt,1,IMPORT,FASTCALL
        EXTRNP  _HalRequestIpi,1,IMPORT
        EXTRNP  _KiFreezeTargetExecution, 2
ifdef DBGMP
        EXTRNP  _KiPollDebugger
endif
        extrn   _KiProcessorBlock:DWORD

DELAYCOUNT  equ    2000h

_DATA   SEGMENT DWORD PUBLIC 'DATA'

public  _KiSynchPacket
_KiSynchPacket dd  0

_DATA   ENDS



_TEXT  SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; BOOLEAN
; KiIpiServiceRoutine (
;     IN PKTRAP_FRAME TrapFrame,
;     IN PKEXCEPTION_FRAME ExceptionFrame
;     )
;
; Routine Description:
;
;     This routine is called at IPI level to process any outstanding
;     interporcessor requests for the current processor.
;
; Arguments:
;
;     TrapFrame - Supplies a pointer to a trap frame.
;
;     ExceptionFrame - Not used.
;
; Return Value:
;
;     A value of TRUE is returned, if one of more requests were service.
;     Otherwise, FALSE is returned.
;
;--

cPublicProc _KiIpiServiceRoutine, 2

ifndef NT_UP
cPublicFpo 2, 1
        push    ebx

        mov     ecx, PCR[PcPrcb]        ; get current processor block address

        xor     ebx, ebx                ; get request summary flags
        cmp     [ecx].PbRequestSummary, ebx
        jz      short isr10

        xchg    [ecx].PbRequestSummary, ebx

;
; Check for freeze request or synchronous request.
;

        test    bl, IPI_FREEZE+IPI_SYNCH_REQUEST
        jnz     short isr50             ; if nz, freeze or synch request

;
; For RequestSummary's other then IPI_FREEZE set return to TRUE
;

        mov     bh, 1

;
; Check for Packet ready.
;
; If a packet is ready, then get the address of the requested function
; and call the function passing the address of the packet address as a
; parameter.
;

isr10:  mov     eax, [ecx].PbSignalDone ; get source processor block address
        or      eax, eax                ; check if packet ready
        jz      short isr20             ; if z set, no packet ready

        mov     edx, [esp + 8]          ; Current trap frame

        push    [eax].PbCurrentPacket + 8 ; push parameters on stack
        push    [eax].PbCurrentPacket + 4 ;
        push    [eax].PbCurrentPacket + 0 ;
        push    eax                     ; push source processor block address
        mov     eax, [eax]+PbWorkerRoutine ; get worker routine address
        mov     [ecx].PbSignalDone, 0   ; clear packet address
        mov     [ecx].PbIpiFrame, edx   ; Save frame address
        call    eax                     ; call worker routine
        mov     bh, 1                   ; return TRUE

;
; Check for APC interrupt request.
;

isr20:  test    bl, IPI_APC             ; check if APC interrupt requested
        jz      short isr30             ; if z, APC interrupt not requested

        mov     ecx, APC_LEVEL          ; request APC interrupt
        fstCall HalRequestSoftwareInterrupt

;
; Check for DPC interrupt request.
;

isr30:  test    bl, IPI_DPC             ; check if DPC interrupt requested
        jz      short isr40             ; if z, DPC interrupt not requested

        mov     ecx, DISPATCH_LEVEL     ; request DPC interrupt
        fstCall HalRequestSoftwareInterrupt ;

isr40:  mov     al, bh                  ; return status
        pop     ebx
        stdRET  _KiIpiServiceRoutine

;
; Freeze or synchronous request
;

isr50:  test    bl, IPI_FREEZE
        jz      short isr60             ; if z, no freeze request

;
; Freeze request is requested
;

        mov     ecx, [esp] + 12         ; get exception frame address
        mov     edx, [esp] + 8          ; get trap frame address
        stdCall _KiFreezeTargetExecution, <edx, ecx> ; freeze execution
        mov     ecx, PCR[PcPrcb]        ; get current processor block address
        test    bl, not IPI_FREEZE      ; Any other IPI RequestSummary?
        setnz   bh                      ; Set return code accordingly
        test    bl, IPI_SYNCH_REQUEST   ; if z, no synch request
        jz      isr10

;
; Synchronous packet request.   Pointer to requesting PRCB in KiSynchPacket.
;

isr60:  mov     eax, _KiSynchPacket     ; get PRCB of requesting processor
        mov     edx, [esp + 8]          ; Current trap frame
        push    [eax].PbCurrentPacket+8 ; push parameters on stack
        push    [eax].PbCurrentPacket+4 ;
        push    [eax].PbCurrentPacket+0 ;
        push    eax                     ; push source processor block address
        mov     eax, [eax]+PbWorkerRoutine ; get worker routine address
        mov     [ecx].PbIpiFrame, edx   ; Save frame address
        call    eax                     ; call worker routine
        mov     ecx, PCR[PcPrcb]        ; restore processor block address
        mov     bh, 1                   ; return TRUE

        jmp     isr10
else
        xor     eax, eax                ; return FALSE
        stdRET  _KiIpiServiceRoutine
endif

stdENDP _KiIpiServiceRoutine

;++
;
; VOID
; FASTCALL
; KiIpiSend (
;    IN KAFFINITY TargetProcessors,
;    IN KIPI_REQUEST Request
;    )
;
; Routine Description:
;
;    This function requests the specified operation on the targt set of
;    processors.
;
; Arguments:
;
;    TargetProcessors (ecx) - Supplies the set of processors on which the
;        specified operation is to be executed.
;
;    IpiRequest (edx) - Supplies the request operation code.
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KiIpiSend, 2

ifndef NT_UP

cPublicFpo 0, 2
        push    esi                     ; save registers
        push    edi                     ;
        mov     esi, ecx                ; save target processor set

        shr     ecx, 1                  ; shift out first bit
        lea     edi, _KiProcessorBlock  ; get processor block array address
        jnc     short is20              ; if nc, not in target set

is10:   mov     eax, [edi]              ; get processor block address
   lock or      [eax].PbRequestSummary, edx ; set request summary bit

is20:   shr     ecx, 1                  ; shift out next bit
        lea     edi, [edi+4]            ; advance to next processor
        jc      short is10              ; if target, go set summary bit
        jnz     short is20              ; if more, check next

        stdCall _HalRequestIpi, <esi>   ; request IPI interrupts on targets

        pop     edi                     ; restore registers
        pop     esi                     ;
endif
        fstRet  KiIpiSend

fstENDP KiIpiSend

;++
;
; VOID
; KiIpiSendPacket (
;     IN KAFFINITY TargetProcessors,
;     IN PKIPI_WORKER WorkerFunction,
;     IN PVOID Parameter1,
;     IN PVOID Parameter2,
;     IN PVOID Parameter3
;     )
;
; Routine Description:
;
;    This routine executes the specified worker function on the specified
;    set of processors.
;
; Arguments:
;
;   TargetProcessors [esp + 4] - Supplies the set of processors on which the
;       specfied operation is to be executed.
;
;   WorkerFunction [esp + 8] - Supplies the address of the worker function.
;
;   Parameter1 - Parameter3 [esp + 12] - Supplies worker function specific
;       paramters.
;
; Return Value:
;
;     None.
;
;--*/

cPublicProc _KiIpiSendPacket, 5

ifndef NT_UP

cPublicFpo 5, 2
        push    esi                     ; save registers
        push    edi                     ;

;
; Store function address and parameters in the packet area of the PRCB on
; the current processor.
;

        mov     edx, PCR[PcPrcb]        ; get current processor block address
        mov     ecx, [esp] + 12         ; set target processor set
        mov     eax, [esp] + 16         ; set worker function address
        mov     edi, [esp] + 20         ; store worker function parameters
        mov     esi, [esp] + 24         ;

        mov     [edx].PbTargetSet, ecx
        mov     [edx].PbWorkerRoutine, eax

        mov     eax, [esp] + 28
        mov     [edx].PbCurrentPacket, edi
        mov     [edx].PbCurrentPacket + 4, esi
        mov     [edx].PbCurrentPacket + 8, eax

;
; Loop through the target processors and send the packet to the specified
; recipients.
;

        shr     ecx, 1                  ; shift out first bit
        lea     edi, _KiProcessorBlock  ; get processor block array address
        jnc     short isp30             ; if nc, not in target set
isp10:  mov     esi, [edi]              ; get processor block address
isp20:  mov     eax, [esi].PbSignalDone ; check if packet being processed
        or      eax, eax                ;
        jne     short isp20             ; if ne, packet being processed

   lock cmpxchg [esi].PbSignalDone, edx ; compare and exchange

        jnz     short isp20             ; if nz, exchange failed

isp30:  shr     ecx, 1                  ; shift out next bit
        lea     edi, [edi+4]            ; advance to next processor
        jc      short isp10             ; if c, in target set
        jnz     short isp30             ; if nz, more target processors

        mov     ecx, [esp] + 12         ; set target processor set
        stdCall _HalRequestIpi, <ecx>   ; send IPI to targets

        pop     edi                     ; restore register
        pop     esi                     ;
endif

        stdRet  _KiIpiSendPacket

stdENDP _KiIpiSendPacket

;++
;
; VOID
; FASTCALL
; KiIpiSignalPacketDone (
;     IN PKIPI_CONTEXT Signaldone
;     )
;
; Routine Description:
;
;     This routine signals that a processor has completed a packet by
;     clearing the calling processor's set member of the requesting
;     processor's packet.
;
; Arguments:
;
;     SignalDone (ecx) - Supplies a pointer to the processor block of the
;         sending processor.
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KiIpiSignalPacketDone, 1

ifndef NT_UP

        mov     edx, PCR[PcPrcb]                ; get current processor block address
        mov     eax, [edx].PbSetMember          ; get processor bit

   lock xor     [ecx].PbTargetSet, eax          ; clear processor set member
endif
        fstRET  KiIpiSignalPacketDone

fstENDP KiIpiSignalPacketDone


;++
;
; VOID
; FASTCALL
; KiIpiSignalPacketDoneAndStall (
;     IN PKIPI_CONTEXT Signaldone
;     IN PULONG ReverseStall
;     )
;
; Routine Description:
;
;     This routine signals that a processor has completed a packet by
;     clearing the calling processor's set member of the requesting
;     processor's packet, and then stalls of the reverse stall value
;
; Arguments:
;
;     SignalDone (ecx) - Supplies a pointer to the processor block of the
;         sending processor.
;
;     ReverseStall (edx) - Supplies a pointer to the reverse stall barrier
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KiIpiSignalPacketDoneAndStall, 2
cPublicFpo 0, 2

ifndef NT_UP
        push    ebx
        push    esi

        mov     esi, PCR[PcPrcb]                ; get current processor block address
        mov     eax, [esi].PbSetMember          ; get processor bit
        mov     ebx, dword ptr [edx]            ; get current value of barrier

   lock xor     [ecx].PbTargetSet, eax          ; clear processor set member

sps10:  mov     eax, DELAYCOUNT
sps20:  cmp     ebx, dword ptr [edx]            ; barrier set?
        jne     short sps90                     ; yes, all done

        YIELD
        dec     eax                             ; P54C pre C2 workaround
        jnz     short sps20                     ; if eax = 0, generate bus cycle

ifdef DBGMP
        stdCall _KiPollDebugger                 ; Check for debugger ^C
endif

;
; There could be a freeze execution outstanding.  Check and clear
; freeze flag.
;

.errnz IPI_FREEZE - 4
   lock btr     [esi].PbRequestSummary, 2       ; Generate bus cycle
        jnc     short sps10                     ; Freeze pending?

cPublicFpo 0,4
        push    ecx                             ; save TargetSet address
        push    edx
        stdCall _KiFreezeTargetExecution, <[esi].PbIpiFrame, 0>
        pop     edx
        pop     ecx
        jmp     short sps10

sps90:  pop     esi
        pop     ebx
endif
        fstRET  KiIpiSignalPacketDoneAndStall

fstENDP KiIpiSignalPacketDoneAndStall

_TEXT   ends
        end
