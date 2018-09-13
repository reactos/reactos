        title  "Interlocked Support"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    intrlock.asm
;
; Abstract:
;
;    This module implements functions to support interlocked operations.
;    Interlocked operations can only operate on nonpaged data.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 12-Feb-1990
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;   bryanwi 1-aug-90    Clean up and fix stuff.
;   bryanwi 3-aug-90    Add ExInterlockedIncrementLlong,...
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
        .list


_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
;   General Notes on Interlocked Procedures:
;
;       These procedures assume that neither their code, nor any of
;       the data they touch, will cause a page fault.
;
;       They use spinlocks to achieve MP atomicity, iff it's an MP machine.
;       (The spinlock macros generate zilch if NT_UP = 1, and
;        we if out some aux code here as well.)
;
;       They turn off interrupts so that they can be used for synchronization
;       between ISRs and driver code.  Flags are preserved so they can
;       be called in special code (Like IPC interrupt handlers) that
;       may have interrupts off.
;
;--


;;      align  512

        page ,132
        subttl  "ExInterlockedAddLargeInteger"
;++
;
; LARGE_INTEGER
; ExInterlockedAddLargeInteger (
;    IN PLARGE_INTEGER Addend,
;    IN LARGE_INTEGER Increment,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of an increment value to an
;    addend variable of type unsigned large integer. The initial value of
;    the addend variable is returned as the function value.
;
; Arguments:
;
;    (TOS+4) = Addend - a pointer to the addend value
;    (TOS+8) = Increment - the increment value
;    (TOS+16) = Lock - a pointer to a pointer to a spin lock
;
; Return Value:
;
;    The initial value of the addend variable is stored in eax:edx
;
;--

EiulAddend      equ     [ebp + 8]
EiulIncrement   equ     [ebp + 12]
EiulLock        equ     [ebp + 20]
EiulRetval      equ     [ebp - 8]

cPublicProc _ExInterlockedAddLargeInteger, 4

        push    ebp
        mov     ebp,esp
        sub     esp, 8

ifndef NT_UP
        mov     eax,EiulLock            ; (eax) -> KSPIN_LOCK
endif

eiul10: pushfd
        cli                             ; disable interrupts
        ACQUIRE_SPINLOCK eax,<short eiul20>

        mov     eax,EiulAddend          ; (eax)-> addend variable
        mov     ecx,[eax]               ; (ecx)= low part of addend value
        mov     edx,[eax]+4             ; (edx)= high part of addend value
        mov     EiulRetVal,ecx               ; set low part of return value
        mov     EiulRetVal+4,edx             ; set high part of return value
        add     ecx,EiulIncrement       ; add low parts of large integer
        adc     edx,EiulIncrement+4     ; add high parts of large integer and carry
        mov     eax,EiulAddend          ; RELOAD (eax)-> addend variable
        mov     [eax],ecx               ; store low part of result
        mov     [eax]+4,edx             ; store high part of result

ifndef NT_UP
        mov     eax,EiulLock
        RELEASE_SPINLOCK   eax          ; NOTHING if NT_UP = 1
endif
        popfd                           ; restore flags including interrupts
        mov     eax, EiulRetval         ; calling convention
        mov     edx, EiulRetval+4       ; calling convention
        mov     esp, ebp
        pop     ebp
        stdRET    _ExInterlockedAddLargeInteger

ifndef NT_UP
eiul20: popfd
        SPIN_ON_SPINLOCK   eax, eiul10
endif

stdENDP _ExInterlockedAddLargeInteger

        page ,132
        subttl  "ExInterlocked Exchange Add Large Integer"
;++
;
; LARGE_INTEGER
; ExInterlockedExchangeAddLargeInteger (
;    IN PLARGE_INTEGER Addend,
;    IN LARGE_INTEGER Increment,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of an increment value to an
;    addend variable of type unsigned large integer. The initial value of
;    the addend variable is returned as the function value.
;
;    N.B. The cmpxchg8b instruction is only supported on some processors.
;         If the host processor does not support this instruction, then
;         then following code is patched to contain a jump to the normal
;         add large integer code which has a compatible calling sequence
;         and data structure.
;
; Arguments:
;
;    (TOS + 4) = Addend - Supplies a pointer to the addend variable.
;
;    (TOS + 8) = Increment - Supplies the increment value.
;
;    (TOS + 16) = Lock - Supplies a pointer a spin lock.
;
;    N.B. This routine does not use the spin lock.
;
; Return Value:
;
;    The initial value of the addend variable is stored in eax:edx.
;
;--

XaAddend      equ     [esp + 12]
XaIncrement   equ     [esp + 16]
XaLock        equ     [esp + 24]

cPublicProc _ExInterlockedExchangeAddLargeInteger, 4

cPublicFpo 0,2

;
; Save nonvolatile registers and get the addend value.
;

        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        mov     ebp, XaAddend           ; get the addend variable address
        mov     eax,[ebp] + 0           ; get low part of addend value
        mov     edx,[ebp] + 4           ; get high part of addend value

;
; Add the increment value to the addend value.
;

Xa10:   mov     ebx, eax                ; copy low part of addend value
        mov     ecx, edx                ; copy high part of addend value
        add     ebx, XaIncrement        ; add low part of increment value
        adc     ecx, XaIncrement + 4    ; add high part of increment value

;
; Exchange the updated addend value with the previous addend value.
;

.586
ifndef NT_UP

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

else

        cmpxchg8b qword ptr [ebp]       ; compare and exchange

endif
.386

        jnz     short Xa10              ; if z clear, exchange failed

;
; Restore nonvolatile registers and return result.
;

cPublicFpo 0,0

        pop     ebp                     ;
        pop     ebx                     ;

        stdRET    _ExInterlockedExchangeAddLargeInteger

stdENDP _ExInterlockedExchangeAddLargeInteger

        page , 132
        subttl  "Interlocked Add Unsigned Long"
;++
;
; ULONG
; ExInterlockedAddUlong (
;    IN PULONG Addend,
;    IN ULONG Increment,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of an increment value to an
;    addend variable of type unsinged long. The initial value of the addend
;    variable is returned as the function value.
;
;       It is NOT possible to mix ExInterlockedDecrementLong and
;       ExInterlockedIncrementong with ExInterlockedAddUlong.
;
;
; Arguments:
;
;    Addend - Supplies a pointer to a variable whose value is to be
;       adjusted by the increment value.
;
;    Increment - Supplies the increment value to be added to the
;       addend variable.
;
;    Lock - Supplies a pointer to a spin lock to be used to synchronize
;       access to the addend variable.
;
; Return Value:
;
;    The initial value of the addend variable.
;
;--

EialAddend      equ     [esp + 8]
EialIncrement   equ     [esp + 12]
EialLock        equ     [esp + 16]

; end of arguments

cPublicProc _ExInterlockedAddUlong  , 3
cPublicFpo 3, 1

ifdef NT_UP
;
; UP version of ExInterlockedAddUlong
;

        pushfd
        cli                             ; disable interrupts

        mov     ecx, EialAddend         ; (ecx)->initial addend value
        mov     edx, [ecx]              ; (edx)= initial addend value
        mov     eax, edx                ; (eax)= initial addend value
        add     edx, EialIncrement      ; (edx)=adjusted value
        mov     [ecx], edx              ; [ecx]=adjusted value

        popfd                           ; restore flags including ints
        stdRET    _ExInterlockedAddUlong                             ; cRetURN

else

;
; MP version of ExInterlockedAddUlong
;
        pushfd
        mov     edx,EialLock            ; (edx)-> KSPIN_LOCK
Eial10: cli                             ; disable interrupts
        ACQUIRE_SPINLOCK edx, <short Eial20>

        mov     ecx, EialAddend         ; (ecx)->initial addend value
        mov     eax, [ecx]              ; (eax)=initial addend value
        add     eax, EialIncrement      ; (eax)=adjusted value
        mov     [ecx], eax              ; [ecx]=adjusted value
        sub     eax, EialIncrement      ; (eax)=initial addend value

        RELEASE_SPINLOCK edx
        popfd
        stdRET    _ExInterlockedAddUlong                             ; cRetURN

Eial20: popfd
        pushfd
        SPIN_ON_SPINLOCK edx, <short Eial10>
endif

stdENDP _ExInterlockedAddUlong


        page , 132
        subttl  "Interlocked Insert Head List"
;++
;
; PLIST_ENTRY
; ExInterlockedInsertHeadList (
;    IN PLIST_ENTRY ListHead,
;    IN PLIST_ENTRY ListEntry,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function inserts an entry at the head of a doubly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;
;    N.B. The pages of data which this routine operates on MUST be
;         present.  No page fault is allowed in this routine.
;
; Arguments:
;
;    ListHead - Supplies a pointer to the head of the doubly linked
;       list into which an entry is to be inserted.
;
;    ListEntry - Supplies a pointer to the entry to be inserted at the
;       head of the list.
;
;    Lock - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;    Pointer to entry that was at the head of the list or NULL if the list
;    was empty.
;
;--

EiihListHead    equ     [esp + 8]
EiihListEntry   equ     [esp + 12]
EiihLock        equ     [esp + 16]

; end arguments

cPublicProc _ExInterlockedInsertHeadList    , 3
cPublicFpo 3, 1

ifndef NT_UP
        mov     edx, EiihLock - 4       ; (edx)->KSPIN_LOCK
endif
Eiih10: pushfd
        cli
        ACQUIRE_SPINLOCK    edx,<short Eiih20>

        mov     eax, EiihListHead       ; (eax)->head of linked list
        mov     ecx, EiihListEntry      ; (ecx)->entry to be inserted
        mov     edx, LsFlink[eax]       ; (edx)->next entry in the list
        mov     [ecx]+LsFlink, edx      ; store next link in entry
        mov     [ecx]+LsBlink, eax      ; store previous link in entry
        mov     [eax]+LsFlink, ecx      ; store next link in head
        mov     [edx]+LsBlink, ecx      ; store previous link in next

ifndef NT_UP
        mov     ecx, EiihLock           ; (ecx)->KSPIN_LOCK
        RELEASE_SPINLOCK ecx
endif
cPublicFpo 3, 0
        popfd                           ; restore flags including interrupts

        xor     eax,edx                 ; return null if list was empty
        jz      short Eiih15
        mov     eax,edx                 ; otherwise return prev. entry at head
Eiih15:
        stdRET    _ExInterlockedInsertHeadList

ifndef NT_UP
align 4
Eiih20: popfd
        SPIN_ON_SPINLOCK edx, <short Eiih10>
endif

stdENDP _ExInterlockedInsertHeadList


        page , 132
        subttl  "Interlocked Insert Tail List"
;++
;
; PLIST_ENTRY
; ExInterlockedInsertTailList (
;    IN PLIST_ENTRY ListHead,
;    IN PLIST_ENTRY ListEntry,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function inserts an entry at the tail of a doubly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;
;    N.B. The pages of data which this routine operates on MUST be
;         present.  No page fault is allowed in this routine.
;
; Arguments:
;
;    ListHead - Supplies a pointer to the head of the doubly linked
;       list into which an entry is to be inserted.
;
;    ListEntry - Supplies a pointer to the entry to be inserted at the
;       tail of the list.
;
;    Lock - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;    Pointer to entry that was at the tail of the list or NULL if the list
;    was empty.
;
;--

EiitListHead    equ     [esp + 8]
EiitListEntry   equ     [esp + 12]
EiitLock        equ     [esp + 16]

; end arguments

cPublicProc _ExInterlockedInsertTailList    , 3
cPublicFpo 3, 1


ifndef NT_UP
        mov     edx,EiitLock - 4        ; (edx)->KSPIN_LOCK
endif

Eiit10: pushfd
        cli                             ; disable interrupts
        ACQUIRE_SPINLOCK edx, <short Eiit20>

        mov     eax, EiihListHead       ; (eax)->head of linked list
        mov     ecx, EiihListEntry      ; (ecx)->entry to be inserted
        mov     edx, LsBlink[eax]       ; (edx)->previous entry in the list
        mov     [ecx]+LsFlink, eax      ; store next link in entry
        mov     [ecx]+LsBlink, edx      ; store previous link in entry
        mov     [eax]+LsBlink, ecx      ; store previous link in head
        mov     [edx]+LsFlink, ecx      ; store next link in next

ifndef NT_UP
        mov     ecx,EiitLock            ; (ecx)->KSPIN_LOCK
        RELEASE_SPINLOCK ecx
endif
cPublicFpo 3,0
        popfd                           ; restore flags including interrupts

        xor     eax,edx                 ; return null if list was empty
        jz      short Eiit15
        mov     eax,edx                 ; otherwise return prev. entry at tail
Eiit15:
        stdRET    _ExInterlockedInsertTailList

ifndef NT_UP
align 4
Eiit20: popfd
        SPIN_ON_SPINLOCK edx, <short Eiit10>
endif

stdENDP _ExInterlockedInsertTailList

        page , 132
        subttl  "Interlocked Remove Head List"
;++
;
; PLIST_ENTRY
; ExInterlockedRemoveHeadList (
;    IN PLIST_ENTRY ListHead,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function removes an entry from the head of a doubly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;    If there are no entries in the list, then a value of NULL is returned.
;    Otherwise, the address of the entry that is removed is returned as the
;    function value.
;
;    N.B. The pages of data which this routine operates on MUST be
;         present.  No page fault is allowed in this routine.
;
; Arguments:
;
;    ListHead - Supplies a pointer to the head of the doubly linked
;       list from which an entry is to be removed.
;
;    Lock - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;    The address of the entry removed from the list, or NULL if the list is
;    empty.
;
;--

EirhListHead    equ     [esp + 8]
EirhLock        equ     [esp + 12]

; end of arguments

cPublicProc _ExInterlockedRemoveHeadList    , 2
cPublicFpo 2, 1

ifndef NT_UP
        mov     edx, EirhLock - 4       ; (edx)-> KSPIN_LOCK
endif

Eirh10: pushfd
        cli

        ACQUIRE_SPINLOCK edx, <Eirh30>

        mov     edx, EirhListHead       ; (edx)-> head of list
        mov     eax, [edx]+LsFlink      ; (eax)-> next entry
        cmp     eax, edx                ; Is list empty?
        je      short Eirh20            ; if e, list is empty, go Eirh20
        mov     ecx, [eax]+LsFlink      ; (ecx)-> next entry(after deletion)
        mov     [edx]+LsFlink, ecx      ; store address of next in head
        mov     [ecx]+LsBlink, edx      ; store address of previous in next
if DBG
        mov     [eax]+LsFlink, 0baddd0ffh
        mov     [eax]+LsBlink, 0baddd0ffh
endif
ifndef NT_UP
        mov     edx, EirhLock           ; (edx)-> KSPIN_LOCK
        RELEASE_SPINLOCK  edx
endif
cPublicFpo 2, 0
        popfd                           ; restore flags including interrupts
        stdRET    _ExInterlockedRemoveHeadList                             ; cReturn entry

align 4
Eirh20:
ifndef NT_UP
        mov     edx, EirhLock           ; (edx)-> KSPIN_LOCK
        RELEASE_SPINLOCK edx
endif
        popfd
        xor     eax,eax                 ; (eax) = null for empty list
        stdRET    _ExInterlockedRemoveHeadList                             ; cReturn NULL

ifndef NT_UP
align 4
Eirh30: popfd
        SPIN_ON_SPINLOCK edx, Eirh10
endif

stdENDP _ExInterlockedRemoveHeadList

        page , 132
        subttl  "Interlocked Pop Entry List"
;++
;
; PSINGLE_LIST_ENTRY
; ExInterlockedPopEntryList (
;    IN PSINGLE_LIST_ENTRY ListHead,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function removes an entry from the front of a singly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;    If there are no entries in the list, then a value of NULL is returned.
;    Otherwise, the address of the entry that is removed is returned as the
;    function value.
;
; Arguments:
;
;    ListHead - Supplies a pointer to the head of the singly linked
;       list from which an entry is to be removed.
;
;    Lock - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;    The address of the entry removed from the list, or NULL if the list is
;    empty.
;
;--

; end of arguments

cPublicProc _ExInterlockedPopEntryList      , 2
cPublicFpo 2,1

ifndef NT_UP
        mov     edx, [esp+8]            ; (edx)-> KSPIN_LOCK
endif

Eipe10: pushfd
        cli                             ; disable interrupts

        ACQUIRE_SPINLOCK edx, <short Eipe30>

        mov     ecx, [esp+8]            ; (ecx)-> head of list
        mov     eax, [ecx]              ; (eax)-> next entry
        or      eax, eax                ; Is it empty?
        je      short Eipe20            ; if e, empty list, go Eipe20
        mov     edx, [eax]              ; (edx)->next entry (after deletion)
        mov     [ecx], edx              ; store address of next in head
if DBG
        mov     [eax], 0baddd0ffh
endif

ifndef NT_UP
        mov     edx, [esp+12]           ; (edx)-> KSPIN_LOCK
endif

Eipe15: RELEASE_SPINLOCK edx

cPublicFpo 2,0
        popfd                           ; restore flags including interrupts
        stdRET    _ExInterlockedPopEntryList    ; cReturn (eax)->removed entry

Eipe20: xor     eax, eax                ; return NULL for empty list
        jmp     short Eipe15            ; continue in common exit

ifndef NT_UP
Eipe30: popfd
        SPIN_ON_SPINLOCK edx, Eipe10
endif

stdENDP _ExInterlockedPopEntryList

        page , 132
        subttl  "Interlocked Push Entry List"
;++
;
; PSINGLE_LIST_ENTRY
; ExInterlockedPushEntryList (
;    IN PSINGLE_LIST_ENTRY ListHead,
;    IN PSINGLE_LIST_ENTRY ListEntry,
;    IN PKSPIN_LOCK Lock
;    )
;
; Routine Description:
;
;    This function inserts an entry at the head of a singly linked list
;    so that access to the list is synchronized in a multiprocessor system.
;
; Arguments:
;
;    ListHead - Supplies a pointer to the head of the singly linked
;       list into which an entry is to be inserted.
;
;    ListEntry - Supplies a pointer to the entry to be inserted at the
;       head of the list.
;
;    Lock - Supplies a pointer to a spin lock to be used to synchronize
;       access to the list.
;
; Return Value:
;
;    Previous contents of ListHead.  NULL implies list went from empty
;       to not empty.
;
;--

; end of arguments

cPublicProc _ExInterlockedPushEntryList     , 3
cPublicFpo 3,1

ifndef NT_UP
        mov     edx, [esp+12]           ; (edx)->KSPIN_LOCK
endif

Eipl10: pushfd
        cli

        ACQUIRE_SPINLOCK edx, <short Eipl20>

        mov     edx, [esp+8]            ; (edx)-> Head of list
        mov     eax, [edx]              ; (eax)-> next entry (return value also)
        mov     ecx, [esp+12]           ; (ecx)-> Entry to be pushed
        mov     [ecx], eax              ; store address of next in new entry
        mov     [edx], ecx              ; set address of next in head

ifndef NT_UP
        mov     edx, [esp+16]           ; (edx)->KSPIN_LOCK
        RELEASE_SPINLOCK edx
endif
cPublicFpo 3,0
        popfd                           ; restore flags including interrupts
        stdRET    _ExInterlockedPushEntryList

ifndef NT_UP
align 4
Eipl20: popfd
        SPIN_ON_SPINLOCK edx, <short Eipl10>
endif

stdENDP _ExInterlockedPushEntryList

        page , 132
        subttl  "Interlocked Increment Long"
;++
;
;   INTERLOCKED_RESULT
;   ExInterlockedIncrementLong (
;       IN PLONG Addend,
;       IN PKSPIN_LOCK Lock
;       )
;
;   Routine Description:
;
;       This function atomically increments Addend, returning an ennumerated
;       type which indicates what interesting transitions in the value of
;       Addend occurred due the operation.
;
;       It is NOT possible to mix ExInterlockedDecrementLong and
;       ExInterlockedIncrementong with ExInterlockedAddUlong.
;
;
;   Arguments:
;
;       Addend (esp+4) - Pointer to variable to increment.
;
;       Lock (esp+8) - Spinlock used to implement atomicity.
;                      (not actually used on x86)
;
;   Return Value:
;
;       An ennumerated type:
;
;       ResultNegative if Addend is < 0 after increment.
;       ResultZero     if Addend is = 0 after increment.
;       ResultPositive if Addend is > 0 after increment.
;
;--

cPublicProc _ExInterlockedIncrementLong , 2
cPublicFpo 2, 0

        mov     eax, [esp+4]            ; (eax) -> addend
ifdef NT_UP
        add dword ptr [eax],1
else
        lock add dword ptr [eax],1
endif
        lahf                            ; (ah) = flags
        and     eax,EFLAG_SELECT        ; clear all but sign and zero flags
        stdRET    _ExInterlockedIncrementLong

stdENDP _ExInterlockedIncrementLong


        page , 132
        subttl  "Interlocked Decrement Long"
;++
;
;   INTERLOCKED_RESULT
;   ExInterlockedDecrementLong (
;       IN PLONG Addend,
;       IN PKSPIN_LOCK Lock
;       )
;
;   Routine Description:
;
;       This function atomically decrements Addend, returning an ennumerated
;       type which indicates what interesting transitions in the value of
;       Addend occurred due the operation.
;
;       It is NOT possible to mix ExInterlockedDecrementLong and
;       ExInterlockedIncrementong with ExInterlockedAddUlong.
;
;   Arguments:
;
;       Addend (esp+4) - Pointer to variable to decrement.
;
;       Lock (esp+8) - Spinlock used to implement atomicity.
;                      (not actually used on x86)
;
;   Return Value:
;
;       An ennumerated type:
;
;       ResultNegative if Addend is < 0 after decrement.
;       ResultZero     if Addend is = 0 after decrement.
;       ResultPositive if Addend is > 0 after decrement.
;
;--

cPublicProc _ExInterlockedDecrementLong , 2
cPublicFpo 2, 0

        mov     eax, [esp+4]            ; (eax) -> addend
ifdef NT_UP
        sub dword ptr [eax], 1
else
        lock sub dword ptr [eax], 1
endif
        lahf                            ; (ah) = flags
        and     eax, EFLAG_SELECT       ; clear all but sign and zero flags
        stdRET    _ExInterlockedDecrementLong

stdENDP _ExInterlockedDecrementLong

        page , 132
        subttl  "Interlocked Exchange Ulong"
;++
;
;   ULONG
;   ExInterlockedExchangeUlong (
;       IN PULONG Target,
;       IN ULONG Value,
;       IN PKSPIN_LOCK Lock
;       )
;
;   Routine Description:
;
;       This function atomically exchanges the Target and Value, returning
;       the prior contents of Target
;
;       This function does not necessarily synchronize with the Lock.
;
;   Arguments:
;
;       Target - Address of ULONG to exchange
;       Value  - New value of ULONG
;       Lock   - SpinLock used to implement atomicity.
;
;   Return Value:
;
;       The prior value of Source
;--

cPublicProc _ExInterlockedExchangeUlong, 3
cPublicFpo 3,0

ifndef NT_UP
        mov     edx, [esp+4]                ; (edx) = Target
        mov     eax, [esp+8]                ; (eax) = Value

        xchg    [edx], eax                  ; make the exchange
else
        mov     edx, [esp+4]                ; (edx) = Target
        mov     ecx, [esp+8]                ; (eax) = Value

        pushfd
        cli
        mov     eax, [edx]                  ; get current value
        mov     [edx], ecx                  ; store new value
        popfd
endif

        stdRET  _ExInterlockedExchangeUlong

stdENDP _ExInterlockedExchangeUlong

        page , 132
        subttl  "Interlocked i386 Increment Long"
;++
;
;   INTERLOCKED_RESULT
;   Exi386InterlockedIncrementLong (
;       IN PLONG Addend
;       )
;
;   Routine Description:
;
;       This function atomically increments Addend, returning an ennumerated
;       type which indicates what interesting transitions in the value of
;       Addend occurred due the operation.
;
;       See ExInterlockedIncrementLong.  This function is the i386
;       architectural specific version of ExInterlockedIncrementLong.
;       No source directly calls this function, instead
;       ExInterlockedIncrementLong is called and when built on x86 these
;       calls are macroed to the i386 optimized version.
;
;   Arguments:
;
;       Addend (esp+4) - Pointer to variable to increment.
;
;       Lock (esp+8) - Spinlock used to implement atomicity.
;                      (not actually used on x86)
;
;   Return Value:
;
;       An ennumerated type:
;
;       ResultNegative if Addend is < 0 after increment.
;       ResultZero     if Addend is = 0 after increment.
;       ResultPositive if Addend is > 0 after increment.
;
;--

cPublicProc _Exi386InterlockedIncrementLong , 1
cPublicFpo 1, 0

        mov     eax, [esp+4]            ; (eax) -> addend
ifdef NT_UP
        add dword ptr [eax],1
else
        lock add dword ptr [eax],1
endif
        lahf                            ; (ah) = flags
        and     eax,EFLAG_SELECT        ; clear all but sign and zero flags
        stdRET    _Exi386InterlockedIncrementLong

stdENDP _Exi386InterlockedIncrementLong


        page , 132
        subttl  "Interlocked i386 Decrement Long"
;++
;
;   INTERLOCKED_RESULT
;   ExInterlockedDecrementLong (
;       IN PLONG Addend,
;       IN PKSPIN_LOCK Lock
;       )
;
;   Routine Description:
;
;       This function atomically decrements Addend, returning an ennumerated
;       type which indicates what interesting transitions in the value of
;       Addend occurred due the operation.
;
;       See Exi386InterlockedDecrementLong.  This function is the i386
;       architectural specific version of ExInterlockedDecrementLong.
;       No source directly calls this function, instead
;       ExInterlockedDecrementLong is called and when built on x86 these
;       calls are macroed to the i386 optimized version.
;
;   Arguments:
;
;       Addend (esp+4) - Pointer to variable to decrement.
;
;       Lock (esp+8) - Spinlock used to implement atomicity.
;                      (not actually used on x86)
;
;   Return Value:
;
;       An ennumerated type:
;
;       ResultNegative if Addend is < 0 after decrement.
;       ResultZero     if Addend is = 0 after decrement.
;       ResultPositive if Addend is > 0 after decrement.
;
;--

cPublicProc _Exi386InterlockedDecrementLong , 1
cPublicFpo 1, 0

        mov     eax, [esp+4]            ; (eax) -> addend
ifdef NT_UP
        sub dword ptr [eax], 1
else
        lock sub dword ptr [eax], 1
endif
        lahf                            ; (ah) = flags
        and     eax, EFLAG_SELECT       ; clear all but sign and zero flags
        stdRET    _Exi386InterlockedDecrementLong

stdENDP _Exi386InterlockedDecrementLong

        page , 132
        subttl  "Interlocked i386 Exchange Ulong"

;++
;
;   ULONG
;   Exi386InterlockedExchangeUlong (
;       IN PULONG Target,
;       IN ULONG Value,
;       IN PKSPIN_LOCK Lock
;       )
;
;   Routine Description:
;
;       This function atomically exchanges the Target and Value, returning
;       the prior contents of Target
;
;       See Exi386InterlockedExchangeUlong.  This function is the i386
;       architectural specific version of ExInterlockedDecrementLong.
;       No source directly calls this function, instead
;       ExInterlockedDecrementLong is called and when built on x86 these
;       calls are macroed to the i386 optimized version.
;
;   Arguments:
;
;       Source - Address of ULONG to exchange
;       Value  - New value of ULONG
;       Lock   - SpinLock used to implement atomicity.
;
;   Return Value:
;
;       The prior value of Source
;--

cPublicProc _Exi386InterlockedExchangeUlong, 2
cPublicFpo 2,0

ifndef NT_UP
        mov     edx, [esp+4]                ; (edx) = Target
        mov     eax, [esp+8]                ; (eax) = Value

        xchg    [edx], eax                  ; make the exchange
else
        mov     edx, [esp+4]                ; (edx) = Target
        mov     ecx, [esp+8]                ; (eax) = Value

        pushfd
        cli
        mov     eax, [edx]                  ; get current value
        mov     [edx], ecx                  ; store new value
        popfd
endif

        stdRET  _Exi386InterlockedExchangeUlong

stdENDP _Exi386InterlockedExchangeUlong

_TEXT$00   ends
        end
