        TITLE   "Fast Mutex Support"
;++
;
;  Copyright (c) 1994  Microsoft Corporation
;
;  Module Name:
;
;     spinlock.asm
;
;  Abstract:
;
;     This module implements teh code necessary to acquire and release fast
;     mutexs without raising or lowering IRQL.
;
;  Author:
;
;     David N. Cutler (davec) 26-May-1994
;
;  Environment:
;
;     Kernel mode only.
;
;  Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
        .list

        EXTRNP _KeSetEventBoostPriority, 2
        EXTRNP _KeWaitForSingleObject, 5

ifdef NT_UP
    LOCK_ADD  equ   add
    LOCK_DEC  equ   dec
else
    LOCK_ADD  equ   lock add
    LOCK_DEC  equ   lock dec
endif

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
;  VOID
;  FASTCALL
;  ExAcquireFastMutexUnsafe (
;     IN PFAST_MUTEX FastMutex
;     )
;
;  Routine description:
;
;   This function acquires ownership of a fast mutex, but does not raise
;   IRQL to APC level.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to a fast mutex.
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExAcquireFastMutexUnsafe,1
cPublicFpo 0,0

   LOCK_DEC     dword ptr [ecx].FmCount ; decrement lock count
        jnz     short afm10             ; if nz, ownership not obtained
        fstRet  ExAcquireFastMutexUnsafe ; return

afm10:  inc     dword ptr [ecx].FmContention ; increment contention count
        add     ecx, FmEvent            ; wait for ownership event
        stdCall _KeWaitForSingleObject,<ecx,WrExecutive,0,0,0> ;
        fstRet  ExAcquireFastMutexUnsafe ; return

fstENDP ExAcquireFastMutexUnsafe

;++
;
;  VOID
;  FASTCALL
;  ExReleaseFastMutexUnsafe (
;     IN PFAST_MUTEX FastMutex
;     )
;
;  Routine description:
;
;   This function releases ownership of a fast mutex, and does nt
;   restore IRQL to its previous value.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to a fast mutex.
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExReleaseFastMutexUnsafe,1
cPublicFpo 0,0

   LOCK_ADD     dword ptr [ecx].FmCount, 1 ; increment ownership count
        jng     short rfm10             ; if ng, waiter present
        fstRet  ExReleaseFastMutexUnsafe ; return

rfm10:  add     ecx, FmEvent            ; compute event address
        stdCall _KeSetEventBoostPriority,<ecx, 0> ; set ownerhsip event
        fstRet  ExReleaseFastMutexUnsafe ; return

fstENDP ExReleaseFastMutexUnsafe


;++
;
;  BOOLEAN
;  FASTCALL
;  ExTryToAcquireFastMutexUnsafe (
;     IN PFAST_MUTEX FastMutex
;     )
;
;  Routine description:
;
;   This function attempts to acquire ownership of a fast mutex, and if
;   successful, does not raise IRQL to APC level.
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to a fast mutex.
;
;  Return Value:
;
;     Returns TRUE if the FAST_MUTEX was acquired; otherwise false
;
;--

if 0

cPublicFastCall ExTryToAcquireFastMutexUnsafe,1
cPublicFpo 0,0

ifdef NT_UP

        cli                             ; disable interrupts

endif

        cmp     dword ptr [ecx].FmCount, 1 ; check if mutex already owned
        jne     short tam10             ; if ne, mutex already owned

ifndef NT_UP

        mov     eax, 1                  ; set value to compare against
        mov     edx, 0                  ; set value to set
   lock cmpxchg dword ptr [ecx].FmCount, edx ; attempt to acquire fast mutex
        jne     short tam10             ; if ne, mutex already owned

else

        mov     dword ptr [ecx].FmCount, 0 ; set mutex owned
        sti                             ; enable interrupts

endif

        mov     eax, 1                  ; set return value
        fstRet  ExTryToAcquireFastMutexUnsafe ; return

tam10:  xor     eax, eax                ; set return value
        YIELD
        fstRet  ExTryToAcquireFastMutexUnsafe ; return

fstENDP ExTryToAcquireFastMutexUnsafe

endif


_TEXT$00   ends

        end
