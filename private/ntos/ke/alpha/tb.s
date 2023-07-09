//      TITLE("Flush Translation Buffers")
//++
//
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    tb.s
//
// Abstract:
//
//    This module implements the code to flush the tbs on the current
//      processor.
//
// Author:
//
//    Joe Notarangelo 21-apr-1992
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

        SBTTL("Flush All Translation Buffers")
//++
//
// VOID
// KiFlushEntireTb(
//    )
//
// Routine Description:
//
//     This function flushes the data and instruction tbs on the current
//     processor.  All entries are flushed with the exception of any entries
//     that are fixed in the tb (either in hdw or via sfw).
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiFlushEntireTb)

        TB_INVALIDATE_ALL               // invalidate all tb entries

        ret     zero, (ra)              // return

        .end    KiFlushEntireTb

        SBTTL("Flush All Translation Buffers")
//++
//
// VOID
// KeFlushCurrentTb(
//    )
//
// Routine Description:
//
//     This function flushes the data and instruction tbs on the current
//     processor.  All entries are flushed with the exception of any entries
//     that are fixed in the tb (either in hdw or via sfw).
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeFlushCurrentTb)

        TB_INVALIDATE_ALL               // invalidate all tb entries

        ret     zero, (ra)              // return

        .end    KeFlushCurrentTb

        SBTTL("Flush Single 32-bit Translation Buffer")
//++
//
// VOID
// KiFlushSingleTb
//    IN BOOLEAN Invalid,
//    IN PVOID Virtual
//
// Routine Description:
//
//     This function flushes a single entry from both the instruction
//     and data translation buffers.  Note: it may flush more that just
//     the single entry.
//
// Arguments:
//
//    Invalid (a0) - Supplies a boolean variable that determines the reason that
//      that the TB entry is being flushed.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to be
//      flushed from the buffers.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiFlushSingleTb)

        bis     a1, zero, a0            // set virtual address to flush

        TB_INVALIDATE_SINGLE            // flush va(a0) from tbs

        ret     zero, (ra)              // return

        .end    KiFlushSingleTb

        SBTTL("Flush Single 64-bit Translation Buffer")
//++
//
// VOID
// KiFlushSingleTb64
//    IN BOOLEAN Invalid,
//    IN PVOID Virtual
//
// Routine Description:
//
//     This function flushes a single entry from both the instruction
//     and data translation buffers.  Note: it may flush more that just
//     the single entry.
//
// Arguments:
//
//    Invalid (a0) - Supplies a boolean variable that determines the reason that
//      that the TB entry is being flushed.
//
//    Virtual (a1) - Supplies the virtual page number of the entry that is to be
//      flushed from the buffers.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiFlushSingleTb64)

        bis     a1, zero, a0            // a0 = va to flush

        TB_INVALIDATE_SINGLE64          // flush va(a0) from tbs

        ret     zero, (ra)              // return

        .end    KiFlushSingleTb64

        SBTTL("Flush Multiple 32-bit Translation Buffer")
//++
//
// VOID
// KiFlushMultipleTb (
//    IN BOOLEAN Invalid,
//    IN PVOID *Virtual,
//    IN ULONG Count
//    )
//
// Routine Description:
//
//    This function flushes multiple entries from the translation buffer.
//
// Arguments:
//
//    Invalid (a0) - Supplies a boolean variable that determines the reason
//       that the TB entry is being flushed.
//
//    Virtual (a1) - Supplies a pointer to an array of virtual addresses of
//       the entries that are flushed from the translation buffer.
//
//    Count (a2) - Supplies the number of TB entries to flush.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiFlushMultipleTb)

        bis     a1, zero, a0            // a0 = pointer to array
        bis     a2, zero, a1            // a1 = count

        TB_INVALIDATE_MULTIPLE          // invalidate the entries

        ret     zero, (ra)              // return

        .end    KiFlushMultipleTb

        SBTTL("Flush Multiple 64-bit Translation Buffer")
//++
//
// VOID
// KiFlushMultipleTb64 (
//    IN BOOLEAN Invalid,
//    IN PVOID *Virtual,
//    IN ULONG Count
//    )
//
// Routine Description:
//
//    This function flushes multiple entries from the translation buffer.
//
// Arguments:
//
//    Invalid (a0) - Supplies a boolean variable that determines the reason
//       that the TB entry is being flushed.
//
//    Virtual (a1) - Supplies a pointer to an array of virtual page numbers
//       for the entries that are flushed from the translation buffer.
//
//    Count (a2) - Supplies the number of TB entries to flush.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiFlushMultipleTb64)

        bis     a1, zero, a0            // a0 = pointer to array
        bis     a2, zero, a1            // a1 = count

        TB_INVALIDATE_MULTIPLE64        // invalidate the entries

        ret     zero, (ra)              // return

        .end    KiFlushMultipleTb64

        SBTTL("Flush Single Data Translation Buffer")
//++
//
// VOID
// KiFlushSingleDataTb(
//    )
//
// Routine Description:
//
//    This function flushes a single entry for the data tb only.
//
// Arguments:
//
//    VirtualAddress(a0) - Supplies the virtual address of the entry
//      that is to be flushed from the data translations.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiFlushSingleDataTb)

        DATA_TB_INVALIDATE_SINGLE       // flush va(a0) from data tbs

        ret     zero, (ra)              // return

        .end    KiFlushSingleDataTb
