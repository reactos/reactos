/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Architecture specific support routines shared between x86 and x64
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern ULONG64 KeFeatureBits;

static const ULONG KxFlushIndividualProcessPagesMaximum = 33; // Based on Linux tlb_single_page_flush_ceiling
static const ULONG KxFlushIndividualGlobalPagesMaximum = 100; // Just a guess

/*!
 * \brief Flushes the current processor's TLB for a single page.
 * \note This function flushes global pages.
 */
FORCEINLINE
VOID
KxFlushSingleCurrentTb(
    _In_ PVOID Address)
{
    /* Invalidate the TLB entry for this address */
    __invlpg(Address);
}

/*!
* \brief Flushes the current processor's TLB for a range of pages.
* \param Address The starting address of the range to flush.
* \param NumberOfPages The number of pages to flush.
* \note This function flushes global pages. It does not optimize for large ranges.
*/
FORCEINLINE
VOID
KxFlushRangeCurrentTb(
    _In_ PVOID Address,
    _In_ ULONG NumberOfPages)
{
    /* Invalidate the TLB entry for each page */
    for (ULONG i = 0; i < NumberOfPages; i++)
    {
        __invlpg((PVOID)((ULONG_PTR)Address + (i * PAGE_SIZE)));
    }
}

/*!
 * \brief Flushes the current processor's TLB, excluding global pages.
 */
FORCEINLINE
VOID
KxFlushProcessCurrentTb(
    VOID)
{
    /* Flush the TLB excluding global pages */
    __writecr3(__readcr3());
}

/*!
 * \brief Flushes the current processor's entire TLB, including global pages.
 */
FORCEINLINE
VOID
KxFlushEntireCurrentTb(
    VOID)
{
#ifdef _GLOBAL_PAGES_ARE_AWESOME_
    /* Check if global pages are enabled in CR4 */
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        /* Disable and restore PGE. This will flush the entire TLB */
        ULONG64 oldCr4 = __readcr4();
        __writecr4(oldCr4 & ~CR4_PGE);
        __writecr4(oldCr4);
    }
    else
#endif
    {
        /* We don't use global pages, reset CR3 instead */
        __writecr3(__readcr3());
    }
}

#ifdef __cplusplus
} // extern "C"
#endif
