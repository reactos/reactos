/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 interlocked S-List routines
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

//
// The 16 byte header holds the depth in the low 16 bits of its first
// quadword and the first entry in the second one. Exclusive pair loads and
// stores update both halves at once, so no sequence number is needed to
// catch a list that changed in between.
//

    IMPORT RtlRaiseStatus

    TEXTAREA

//
// VOID
// RtlInitializeSListHead(
//     _Out_ PSLIST_HEADER SListHead);
//
    LEAF_ENTRY RtlInitializeSListHead
    tst     x0, #15
    b.ne    RtlpSListHeadMisaligned
    stp     xzr, xzr, [x0]
    ret
LABEL(RtlpSListHeadMisaligned)
    ldr     w0, =STATUS_DATATYPE_MISALIGNMENT
    b       RtlRaiseStatus
    LEAF_END RtlInitializeSListHead

//
// PSLIST_ENTRY
// RtlFirstEntrySList(
//     _In_ const SLIST_HEADER *SListHead);
//
    LEAF_ENTRY RtlFirstEntrySList
    ldr     x0, [x0, #8]
    ret
    LEAF_END RtlFirstEntrySList

//
// USHORT
// RtlQueryDepthSList(
//     _In_ PSLIST_HEADER SListHead);
//
    LEAF_ENTRY RtlQueryDepthSList
    ldrh    w0, [x0]
    ret
    LEAF_END RtlQueryDepthSList

//
// PSLIST_ENTRY
// RtlInterlockedPopEntrySList(
//     _Inout_ PSLIST_HEADER SListHead);
//
// The entry can be freed by another thread before its link is read. The
// kernel recognizes a fault on ExpInterlockedPopEntrySListFault and restarts
// at ExpInterlockedPopEntrySListResume.
//
    LEAF_ENTRY RtlInterlockedPopEntrySList
    ALTERNATE_ENTRY ExpInterlockedPopEntrySList
    ALTERNATE_ENTRY ExpInterlockedPopEntrySListResume
    ldxp    x16, x17, [x0]
    cbz     x17, RtlpPopEntrySListEmpty
    ALTERNATE_ENTRY ExpInterlockedPopEntrySListFault
    ldr     x1, [x17]
    sub     x16, x16, #1
    ALTERNATE_ENTRY ExpInterlockedPopEntrySListEnd
    stxp    w2, x16, x1, [x0]
    cbnz    w2, ExpInterlockedPopEntrySListResume
    mov     x0, x17
    ret
LABEL(RtlpPopEntrySListEmpty)
    clrex
    mov     x0, xzr
    ret
    LEAF_END RtlInterlockedPopEntrySList

//
// PSLIST_ENTRY
// RtlInterlockedPushEntrySList(
//     _Inout_ PSLIST_HEADER SListHead,
//     _Inout_ PSLIST_ENTRY ListEntry);
//
    LEAF_ENTRY RtlInterlockedPushEntrySList
    ALTERNATE_ENTRY ExpInterlockedPushEntrySList
    mov     x2, x1
    mov     w3, #1
    b       RtlpPushSList
    LEAF_END RtlInterlockedPushEntrySList

//
// PSLIST_ENTRY
// RtlInterlockedPushListSList(
//     _Inout_ PSLIST_HEADER SListHead,
//     _Inout_ PSLIST_ENTRY List,
//     _Inout_ PSLIST_ENTRY ListEnd,
//     _In_ ULONG Count);
//
// The new entries get linked in front of the current first entry before
// they are published, a header that moved meanwhile means linking again.
//
    LEAF_ENTRY RtlInterlockedPushListSList
    ALTERNATE_ENTRY RtlInterlockedPushListSListEx
LABEL(RtlpPushSList)
    ldr     x17, [x0, #8]
    str     x17, [x2]
    dmb     ish
    ldxp    x16, x4, [x0]
    cmp     x4, x17
    b.ne    RtlpPushSList
    add     x16, x16, w3, uxtw
    stxp    w5, x16, x1, [x0]
    cbnz    w5, RtlpPushSList
    mov     x0, x17
    ret
    LEAF_END RtlInterlockedPushListSList

//
// PSLIST_ENTRY
// RtlInterlockedFlushSList(
//     _Inout_ PSLIST_HEADER SListHead);
//
    LEAF_ENTRY RtlInterlockedFlushSList
    ALTERNATE_ENTRY ExpInterlockedFlushSList
    ldxp    x16, x17, [x0]
    stxp    w2, xzr, xzr, [x0]
    cbnz    w2, RtlInterlockedFlushSList
    mov     x0, x17
    ret
    LEAF_END RtlInterlockedFlushSList

    END
