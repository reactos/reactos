//      TITLE("TB Management")
//++
//
// Copyright (c) 1996  Microsoft Corporation
//
// Module Name:
//
//    tb.s
//
// Abstract:
//
//    This module implements the code necessary to fill and flush TB entries.
//
// Author:
//
//    David N. Cutler (davec) 4-Apr-1991
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

//
// Define external variables that can be addressed using GP.
//

        .extern KeNumberTbEntries 4

        SBTTL("Fill Translation Buffer Entry")
//++
//
// VOID
// KeFillEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN BOOLEAN Invalid
//    )
//
// Routine Description:
//
//    This function fills a translation buffer entry. If the entry is already
//    in the translation buffer, then the entry is overwritten. Otherwise, a
//    random entry is overwritten.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    Invalid (a2) - Supplies a boolean value that determines whether the
//       TB entry should be invalidated.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeFillEntryTb)

        and     a0,a0,~0x7              // clear low bits of PTE address
        lw      t0,0(a0)                // get first PTE value
        lw      t1,4(a0)                // get second PTE value

#if DBG

        xor     t2,t1,t0                // compare G-bits
        and     t2,t2,1 << ENTRYLO_G    // isolate comparison
        beq     zero,t2,5f              // if eq, G-bits match
        break   KERNEL_BREAKPOINT       // break into kernel debugger
5:                                      //

#endif

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t3,entryhi              // get current PID and VPN2
        srl     a1,a1,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     a1,a1,ENTRYHI_VPN2      //
        and     t3,t3,PID_MASK << ENTRYHI_PID // isolate current PID
        or      a1,t3,a1                // merge PID with VPN2 of virtual address
        mtc0    a1,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        mtc0    t0,entrylo0             // set first PTE value
        mtc0    t1,entrylo1             // set second PTE value
        bltz    t3,20f                  // if ltz, entry is not in TB
        nop                             // fill

#if DBG

        sltu    t4,t3,FIXED_ENTRIES     // check if fixed entry within range
        beq     zero,t4,10f             // if eq, index not in fixed region
        nop                             //
        break   KERNEL_BREAKPOINT       // break into debugger

#endif

10:     tlbwi                           // overwrite indexed entry
        nop                             // 3 cycle hazzard
        nop                             //
        b       30f                     //
        nop                             //

20:     tlbwr                           // overwrite random TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        .set    at
        .set    reorder

30:     ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KeFillEntryTb

        SBTTL("Fill Large Translation Buffer Entry")
//++
//
// VOID
// KeFillLargeEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG PageSize
//    )
//
// Routine Description:
//
//    This function fills a large translation buffer entry.
//
//    N.B. It is assumed that the large entry is not in the TB and therefore
//      the TB is not probed.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    PageSize (a2) - Supplies the size of the large page table entry.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeFillLargeEntryTb)

        and     a0,a0,~0x7              // clear low bits of PTE address
        lw      t0,0(a0)                // get first PTE value
        lw      t1,4(a0)                // get second PTE value
        subu    a2,a2,1                 // compute the page mask value
        srl     a2,a2,PAGE_SHIFT        //
        sll     a2,a2,PAGE_SHIFT + 1    //
        nor     a3,a2,zero              // compute virtual address mask

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t3,entryhi              // get current PID and VPN2
        srl     a1,a1,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     a1,a1,ENTRYHI_VPN2      //
        and     a1,a3,a1                // isolate large entry virtual address
        and     t3,t3,PID_MASK << ENTRYHI_PID // isolate current PID
        or      a1,t3,a1                // merge PID with VPN2 of virtual address
        li      a3,LARGE_ENTRY          // set large entry index
        mtc0    a1,entryhi              // set entry high value for large entry
        mtc0    a2,pagemask             // set page mask value
        mtc0    a3,index                //
        mtc0    t0,entrylo0             // set first PTE value
        mtc0    t1,entrylo1             // set second PTE value
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite large TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mtc0    zero,pagemask           // clear page mask value
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KeFillLargeEntryTb

        SBTTL("Fill Fixed Translation Buffer Entry")
//++
//
// VOID
// KeFillFixedEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG Index
//    )
//
// Routine Description:
//
//    This function fills a fixed translation buffer entry.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    Index (a2) - Supplies the index where the TB entry is to be written.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeFillFixedEntryTb)

        lw      t0,0(a0)                // get first PTE value
        lw      t1,4(a0)                // get second PTE value

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t3,entryhi              // get current PID and VPN2
        srl     a1,a1,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     a1,a1,ENTRYHI_VPN2      //
        and     t3,t3,PID_MASK << ENTRYHI_PID // isolate current PID
        or      a1,t3,a1                // merge PID with VPN2 of virtual address
        mtc0    a1,entryhi              // set VPN2 and PID for probe
        mtc0    t0,entrylo0             // set first PTE value
        mtc0    t1,entrylo1             // set second PTE value
        mtc0    a2,index                // set TB entry index
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite indexed TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KeFillFixedEntryTb

        SBTTL("Flush Entire Translation Buffer")
//++
//
// VOID
// KeFlushCurrentTb (
//    VOID
//    )
//
// Routine Description:
//
//    This function flushes the random part of the translation buffer.
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

        j       KiFlushRandomTb         // execute common code

        .end    KeFlushCurrentTb

        SBTTL("Flush Fixed Translation Buffer Entries")
//++
//
// VOID
// KiFlushFixedTb (
//    VOID
//    )
//
// Routine Description:
//
//    This function is called to flush all the fixed entries from the
//    translation buffer.
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

        LEAF_ENTRY(KiFlushFixedTb)

        .set    noreorder
        .set    noat
        move    t0,zero                 // set base index of fixed TB entries
        j       KiFlushTb               //
        mfc0    t3,wired                // set highest index number + 1
        .set    at
        .set    reorder

        .end    KiFlushFixedTb

        SBTTL("Flush Random Translation Buffer Entries")
//++
//
// VOID
// KiFlushRandomTb (
//    VOID
//    )
//
// Routine Description:
//
//    This function is called to flush all the random entries from the TB.
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

        LEAF_ENTRY(KiFlushRandomTb)

        .set    noreorder
        .set    noat
        mfc0    t0,wired                // set base index of random TB entries
        lw      t3,KeNumberTbEntries    // set number of entries
        .set    at
        .set    reorder

        ALTERNATE_ENTRY(KiFlushTb)

        li      t4,KSEG0_BASE           // set high part of TB entry

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        sll     t0,t0,INDEX_INDEX       // shift starting index into position
        sll     t3,t3,INDEX_INDEX       // shift ending index into position
        and     t1,t1,PID_MASK << ENTRYHI_PID // isolate current PID
        li      t4,KSEG0_BASE           // set invalidate address
        or      t4,t4,t1                // merge PID with VPN2 of virtual address
        mtc0    zero,entrylo0           // set low part of TB entry
        mtc0    zero,entrylo1           //
        mtc0    t4,entryhi              //
        mtc0    t0,index                // set TB entry index
10:     addu    t0,t0,1 << INDEX_INDEX  //
        tlbwi                           // write TB entry
        bne     t0,t3,10b               // if ne, more entries to flush
        mtc0    t0,index                // set TB entry index
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KiFlushRandomTb

        SBTTL("Flush Multiple TB Entry")
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

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        nop                             //
        and     a3,t1,PID_MASK << ENTRYHI_PID // isolate current PID
10:     lw      v0,0(a1)                // get virtual address
        addu    a1,a1,4                 // advance to next entry
        subu    a2,a2,1                 // reduce number of entries
        srl     t2,v0,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     t2,t2,ENTRYHI_VPN2      //
        or      t2,t2,a3                // merge PID with VPN2 of virtual address
        mtc0    t2,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe TB for entry
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        nop                             //
        bltz    t3,30f                  // if ltz, entry is not in TB
        lui     t2,KSEG0_BASE  >> 16    // set invalidate address

#if DBG

        sltu    t4,t3,FIXED_ENTRIES     // check if fixed entry region
        beq     zero,t4,20f             // if eq, index not in fixed region
        nop                             //
        break   KERNEL_BREAKPOINT       // break into debugger

#endif

20:     mtc0    zero,entrylo0           // set low part of TB entry
        mtc0    zero,entrylo1           //
        or      t2,t2,a3                // merge PID with VPN2 of invalid address
        mtc0    t2,entryhi              // set VPN2 and PID for TB write
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite index TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
30:     bgtz    a2,10b                  // if gtz, more entires to flush
        mtc0    zero,pagemask           // restore page mask register
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    KiFlushMultipleTb

        SBTTL("Flush Multiple TB Entry 64-bits")
//++
//
// VOID
// KiFlushMultipleTb64 (
//    IN BOOLEAN Invalid,
//    IN PULONG *Virtual,
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
//       of the entries that are flushed from the translation buffer.
//
//    Count (a2) - Supplies the number of TB entries to flush.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiFlushMultipleTb64)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        nop                             //
        and     a3,t1,PID_MASK << ENTRYHI_PID // isolate current PID
10:     lw      v0,0(a1)                // get virtual address
        addu    a1,a1,4                 // advance to next entry
        subu    a2,a2,1                 // reduce number of entries
        srl     t2,v0,1                 // convert from virtual page to VPN2
        sll     t2,t2,ENTRYHI_VPN2      // shift VPN2 into place
        or      t2,t2,a3                // merge PID with VPN2 of virtual address
        mtc0    t2,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe TB for entry
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        nop                             //
        bltz    t3,30f                  // if ltz, entry is not in TB
        lui     t2,KSEG0_BASE  >> 16    // set invalidate address

#if DBG

        sltu    t4,t3,FIXED_ENTRIES     // check if fixed entry region
        beq     zero,t4,20f             // if eq, index not in fixed region
        nop                             //
        break   KERNEL_BREAKPOINT       // break into debugger

#endif

20:     mtc0    zero,entrylo0           // set low part of TB entry
        mtc0    zero,entrylo1           //
        or      t2,t2,a3                // merge PID with VPN2 of invalid address
        mtc0    t2,entryhi              // set VPN2 and PID for TB write
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite index TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
30:     bgtz    a2,10b                  // if gtz, more entires to flush
        mtc0    zero,pagemask           // restore page mask register
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    KiFlushMultipleTb64

        SBTTL("Flush Single TB Entry")
//++
//
// VOID
// KiFlushSingleTb (
//    IN BOOLEAN Invalid,
//    IN PVOID Virtual
//    )
//
// Routine Description:
//
//    This function flushes a single entry from the translation buffer.
//
// Arguments:
//
//    Invalid (a0) - Supplies a boolean variable that determines the reason
//       that the TB entry is being flushed.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be flushed from the translation buffer.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiFlushSingleTb)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        srl     t2,a1,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     t2,t2,ENTRYHI_VPN2      //
        and     a2,t1,PID_MASK << ENTRYHI_PID // isolate current PID
        or      t2,t2,a2                // merge PID with VPN2 of virtual address
        mtc0    t2,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe TB for entry
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        nop                             //
        bltz    t3,20f                  // if ltz, entry is not in TB
        lui     t2,KSEG0_BASE >> 16     // set invalid address

#if DBG

        sltu    t4,t3,FIXED_ENTRIES     // check if fixed entry region
        beq     zero,t4,10f             // if eq, index not in fixed region
        nop                             //
        break   KERNEL_BREAKPOINT       // break into debugger

#endif

10:     mtc0    zero,entrylo0           // set low part of TB entry
        mtc0    zero,entrylo1           //
        or      t2,t2,a2                // merge PID with VPN2 of invalid address
        mtc0    t2,entryhi              // set VPN2 and PID for TB write
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite index TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mtc0    zero,pagemask           // restore page mask register
        .set    at
        .set    reorder

20:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    KiFlushSingleTb

        SBTTL("Flush Single TB Entry 64-bits")
//++
//
// VOID
// KiFlushSingleTb64 (
//    IN BOOLEAN Invalid,
//    IN ULONG Virtual
//    )
//
// Routine Description:
//
//    This function flushes a single entry from the translation buffer.
//
// Arguments:
//
//    Invalid (a0) - Supplies a boolean variable that determines the reason
//       that the TB entry is being flushed.
//
//    Virtual (a1) - Supplies the virtual page number of the entry that is
//       flushed from the translation buffer.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiFlushSingleTb64)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        srl     t2,a1,1                 // convert from virtual page to VPN2
        sll     t2,t2,ENTRYHI_VPN2      // shift VPN2 into place
        and     a2,t1,PID_MASK << ENTRYHI_PID // isolate current PID
        or      t2,t2,a2                // merge PID with VPN2 of virtual address
        mtc0    t2,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe TB for entry
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        nop                             //
        bltz    t3,20f                  // if ltz, entry is not in TB
        lui     t2,KSEG0_BASE >> 16     // set invalid address

#if DBG

        sltu    t4,t3,FIXED_ENTRIES     // check if fixed entry region
        beq     zero,t4,10f             // if eq, index not in fixed region
        nop                             //
        break   KERNEL_BREAKPOINT       // break into debugger

#endif

10:     mtc0    zero,entrylo0           // set low part of TB entry
        mtc0    zero,entrylo1           //
        or      t2,t2,a2                // merge PID with VPN2 of invalid address
        mtc0    t2,entryhi              // set VPN2 and PID for TB write
        nop                             // 1 cycle hazzard
        tlbwi                           // overwrite index TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mtc0    zero,pagemask           // restore page mask register
        .set    at
        .set    reorder

20:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    KiFlushSingleTb64

        SBTTL("Probe Tb Entry")
//++
//
// ULONG
// KiProbeEntryTb (
//     IN PVOID VirtualAddress
//     )
//
// Routine Description:
//
//    This function is called to determine if a specified entry is valid
///   and within the fixed portion of the TB.
//
// Arguments:
//
//    VirtualAddress - Supplies the virtual address to probe.
//
// Return Value:
//
//    A value of TRUE is returned if the specified entry is valid and within
//    the fixed part of the TB. Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY(KiProbeEntryTb)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,entryhi              // get current PID and VPN2
        srl     t2,a0,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        sll     t2,t2,ENTRYHI_VPN2      //
        and     t1,t1,PID_MASK << ENTRYHI_PID // isolate current PID
        or      t2,t2,t1                // merge PID with VPN2 of virtual address
        mtc0    t2,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t3,index                // read result of probe
        li      v0,FALSE                // set to return failure
        bltz    t3,20f                  // if ltz, entry is not in TB
        sll     a0,a0,0x1f - (ENTRYHI_VPN2 - 1) // shift VPN<12> into sign
        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        bltz    a0,10f                  // if ltz, check second PTE
        mfc0    t2,entrylo1             // get second PTE for probe
        mfc0    t2,entrylo0             // get first PTE for probe
10:     mtc0    t1,entryhi              // restore current PID
        mtc0    zero,pagemask           // restore page mask register
        sll     t2,t2,0x1f - ENTRYLO_V  // shift valid bit into sign position
        bgez    t2,20f                  // if geq, entry is not valid
        srl     t3,INDEX_INDEX          // isolate TB index
        and     t3,t3,0x3f              //
        mfc0    t4,wired                // get number of wired entries
        nop                             // fill
        sltu    v0,t3,t4                // check if entry in fixed part of TB
        .set    at
        .set    reorder

20:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

        .end    KiProbeEntryTb

        SBTTL("Read Tb Entry")
//++
//
// VOID
// KiReadEntryTb (
//     IN ULONG Index,
//     OUT PTB_ENTRY TbEntry
//     )
//
// Routine Description:
//
//    This function is called to read an entry from the TB.
//
// Arguments:
//
//    Index - Supplies the index of the entry to read.
//
//    TbEntry - Supplies a pointer to a TB entry structure that receives the
//       contents of the specified TB entry.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiReadEntryTb)

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        sll     a0,INDEX_INDEX          // shift index into position
        dmfc0   t1,entryhi              // save entry high register
        mtc0    a0,index                // set TB entry index
        nop                             //
        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t2,entrylo0             // save first PTE value
        mfc0    t3,entrylo1             // save second PTE value
        dmfc0   t4,entryhi              // save entry high value
        mfc0    t5,pagemask             // save page mask value
        dmtc0   t1,entryhi              // restore entry high register
        mtc0    zero,pagemask           // restore page mask register
        nop                             // 1 cycle hazzard
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        sw      t2,TbEntrylo0(a1)       // set first PTE value
        sw      t3,TbEntrylo1(a1)       // set second PTE value
        sw      t4,TbEntryhi(a1)        // set low part of entry high value
        dsrl    t4,t4,32                // isolate high bits of vpn2
        and     t4,t4,0xff              //
        or      t5,t5,t4                // merge with page mask value
        sw      t5,TbPagemask(a1)       // set page mask value
        j       ra                      // return

        .end    KiReadEntryTb
