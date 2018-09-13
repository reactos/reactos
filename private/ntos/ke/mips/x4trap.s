//      TITLE("Interrupt and Exception Processing")
//++
//
// Copyright (c) 1991  Microsoft Corporation
//
// Module Name:
//
//    x4trap.s
//
// Abstract:
//
//    This module implements the code necessary to field and process MIPS
//    interrupt and exception conditions.
//
//    N.B. This module executes in KSEG0 or KSEG1 and, in general, cannot
//       tolerate a TB Miss. Registers k0 and k1 are used for argument
//       passing during the initial stage of interrupt and exception
//       processing, and therefore, extreme care must be exercised when
//       modifying this module.
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

        SBTTL("Constant Value Definitions")
//++
//
// The following are definitions of constants used in this module.
//
//--

#define PSR_ENABLE_MASK ((0xff << PSR_INTMASK) | (0x3 << PSR_KSU) | (1 << PSR_EXL))

#define PSR_MASK (~((0x3 << PSR_KSU) | (1 << PSR_EXL))) // PSR exception mask

//
// Define exception handler frame structure.
//

        .struct 0
        .space  4 * 4                   // argument save area
HdRa:   .space  4                       // return address
        .space  3 * 4                   //
HandlerFrameLength:                     // handler frame length

//
// Define external variables that can be addressed using GP.
//

        .extern KdpOweBreakpoint    1
        .extern KeGdiFlushUserBatch 4
        .extern PsWatchEnabled      1

//
// Define set of load/store instructions.
//
// This set has a one bit for each of the possible load/store instructions.
//
// These include: ldl, ldr, lb, lh, lwl, lw, lbu, lhu, lwr, lwu, sb, sh, swl,
//                sw, sdl. sdr. swr, ll, lwc1, lwc2, lld, ldc1, ldc2, ld, sc,
//                swc1, swc2, sdc, sdc1, sdc2, sd.
//
// N.B. The set is biased by a base of 0x20 which is the opcode for lb.
//

        .sdata
        .align  3
        .globl  KiLoadInstructionSet
KiLoadInstructionSet:                   // load instruction set
        .word   0x0c000000              //
        .word   0xf7f77fff              //

//
// Define count of bad virtual address register cases.
//

#if DBG

        .globl  KiBadVaddrCount
KiBadVaddrCount:                        // count of bad virtual
        .word   0                       //

        .globl  KiMismatchCount
KiMismatchCount:                        // count of read miss address mismatches
        .word   0                       //

#endif


        SBTTL("System Startup")
//++
//
// Routine Description:
//
//    Control is transfered to this routine when the system is booted. Its
//    function is to transfer control to the real system startup routine.
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

        LEAF_ENTRY(KiSystemStartup)

        j       KiInitializeSystem      // initialize system

        .end    KiSystemStartup

        SBTTL("TB Miss Vector Routine")
//++
//
// Routine Description:
//
//    This routine is entered as the result of a TB miss on a reference
//    to any part of the 32-bit address space from kernel mode. Interrupts
//    are disabled when this routine is entered.
//
//    The function of this routine is to load a pair of second level PTEs
//    from the current page table into the TB. The context register is
//    loaded by hardware with the virtual address of the PTE * 2. In addition,
//    the entryhi register is loaded with the virtual tag, such that the PTEs
//    can be loaded directly into the TB. The badvaddr register is loaded by
//    hardware with the virtual address of the fault and is saved in case the
//    page table page is not currently mapped by the TB.
//
//    If a fault occurs when attempting to load the specified PTEs from the
//    current page table, then it is vectored through the general exception
//    vector at KSEG0_BASE + 0x180.
//
//    This routine is copied to address KSEG0_BASE at system startup.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//    N.B. This routine saves the contents of the badvaddr register in k1
//       so that it can be used by the general exception vector routine
//       if an exception occurs while trying to load the first PTE from
//       memory.
//
//--

        LEAF_ENTRY(KiTbMiss)

        START_REGION(KiTbMissStartAddress2.x)

        START_REGION(KiTbMissStartAddress3.x)

        .set    noreorder
        .set    noat

//
// The following code is required on all MP systems to work around a problem
// where the hardware reports a TB miss even when the entry is really in the
// TB.
//

#if defined(NT_UP)

        mfc0    k0,context              // get virtual address * 2 of PTE
        dmfc0   k1,badvaddr             // get bad virtual address
        sra     k0,k0,1                 // compute virtual address of PTE

#else

        tlbp                            // ****** r4400 errata ******
        mfc0    k0,context              // ****** r4400 errata ******
        nop                             // ****** r4400 errata ******
        mfc0    k1,index                // ****** r4400 errata ******
        sra     k0,k0,1                 // compute virtual address of PTE
        bgez    k1,20f                  // ****** r4400 errata ******
        dmfc0   k1,badvaddr             // get bad virtual address

#endif

        mtc0    k0,taglo                // set first level active flag
        lw      k1,0(k0)                // get first PTE - may fault
        lw      k0,4(k0)                // get second PTE - no fault
        mtc0    k1,entrylo0             // set first PTE value
        mtc0    k0,entrylo1             // set second PTE value

#if DBG

        xor     k1,k1,k0                // compare G-bits
        and     k1,k1,1 << ENTRYLO_G    // isolate G-bit
        beq     zero,k1,10f             // if eq, G-bits match
        nop                             // fill
        mtc0    zero,entrylo0           // reset first PTE value
        mtc0    zero,entrylo1           // reset second PTE value

#endif

10:     nop                             //
        tlbwr                           // write entry randomly into TB
        nop                             // 3 cycle hazzard
        nop                             //
        mtc0    zero,taglo              // 1 cycle hazzard - clear active flag
20:     eret                            //
        .set    at
        .set    reorder

        END_REGION(KiTbMissEndAddress3.x)

//
// The r10000 TB miss routine is different since the fine designers of the
// chip didn't understand what the frame mask register was really for and
// only masked PFN bits. Unfortunately they didn't mask the UC bits which
// require the bits to be masked manually.
//

        START_REGION(KiTbMissStartAddress9.x)

        .set    noreorder
        .set    noat
        mfc0    k0,context              // get virtual address * 2 of PTE
        dmfc0   k1,badvaddr             // get bad virtual address
        sra     k0,k0,1                 // compute virtual address of PTE
        mtc0    k0,taglo                // set first level active flag
        lwu     k1,0(k0)                // get first PTE - may fault
        lwu     k0,4(k0)                // get second PTE - no fault
        mtc0    k1,entrylo0             // set first PTE value
        mtc0    k0,entrylo1             // set second PTE value

#if DBG

        xor     k1,k1,k0                // compare G-bits
        and     k1,k1,1 << ENTRYLO_G    // isolate G-bit
        beq     zero,k1,10f             // if eq, G-bits match
        nop                             // fill
        mtc0    zero,entrylo0           // reset first PTE value
        mtc0    zero,entrylo1           // reset second PTE value

#endif

10:     nop                             //
        tlbwr                           // write entry randomly into TB
        nop                             // 3 cycle hazzard
        nop                             //
        mtc0    zero,taglo              // 1 cycle hazzard - clear active flag
20:     eret                            //
        .set    at
        .set    reorder

        END_REGION(KiTbMissEndAddress9.x)

        .end    KiTbMiss

        SBTTL("XTB Miss Vector Routine")
//++
//
// Routine Description:
//
//    This routine is entered as the result of a TB miss on a reference
//    to any part of the 64-bit address space from user mode. Interrupts
//    are disabled when this routine is entered.
//
//    The function of this routine is to load a pair of second level PTEs
//    from the current page table into the TB. The context register is
//    loaded by hardware with the virtual address of the PTE * 2. In addition,
//    the entryhi register is loaded with the virtual tag, such that the PTEs
//    can be loaded directly into the TB. The badvaddr register is loaded by
//    hardware with the virtual address of the fault and is saved in case the
//    page table page is not currently mapped by the TB.
//
//    If a fault occurs when attempting to load the specified PTEs from the
//    current page table, then it is vectored through the general exception
//    vector at KSEG0_BASE + 0x180.
//
//    This routine is copied to address KSEG0_BASE + 0x80 at system startup.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//    N.B. This routine saves the contents of the badvaddr register in k1
//       so that it can be used by the general exception vector routine
//       if an exception occurs while trying to load the first PTE from
//       memory.
//
//--

        LEAF_ENTRY(KiXTbMiss)

        START_REGION(KiXTbMissStartAddress2.x)

        START_REGION(KiXTbMissStartAddress3.x)

        .set    noreorder
        .set    noat

//
// The following code is required on all MP systems to work around a problem
// where the hardware reports a TB miss even when the entry is really in the
// TB.
//

#if defined(NT_UP)

        dmfc0   k1,xcontext             // get extended context register
        mfc0    k0,context              // get virtual address * 2 of PTE
        dsrl    k1,k1,22                // isolate bits 63:62 and 39:31
        daddu   k1,k1,1                 //
        and     k1,k1,0x7ff             // check if valid 32-bit address
        sltu    k1,k1,2                 //
        beq     zero,k1,30f             // if eq, not valid 32-bit address
        dmfc0   k1,xcontext             // get extended context register
        dmfc0   k1,badvaddr             // get bad virtual address
10:     sra     k0,k0,1                 // compute virtual address of PTE

#else

//
// ****** r4400 errata ******
//

        dmfc0   k1,xcontext             // get extended context register
        tlbp                            // probe TB for miss address
        mfc0    k0,context              // get virtual address * 2 of PTE
        dsrl    k1,k1,22                // isolate bits 63:62 and 39:31
        daddu   k1,k1,1                 //
        and     k1,k1,0x7ff             // check if valid 32-bit address
        sltu    k1,k1,2                 //
        beq     zero,k1,30f             // if eq, not valid 32-bit address
        dmfc0   k1,xcontext             // get extended context register
        mfc0    k1,index                // get index register
10:     sra     k0,k0,1                 // compute virtual address of PTE
        bgez    k1,20f                  // if gez, address already in TB
        dmfc0   k1,badvaddr             // get bad virtual address

#endif

        mtc0    k0,taglo                // set first level active flag
        lw      k1,0(k0)                // get first PTE - may fault
        lw      k0,4(k0)                // get second PTE - no fault
        mtc0    k1,entrylo0             // set first PTE value
        mtc0    k0,entrylo1             // set second PTE value
        nop                             //
        tlbwr                           // write entry randomly into TB
        nop                             // 3 cycle hazzard
        nop                             //
        mtc0    zero,taglo              // 1 cycle hazzard - clear active flag
20:     eret                            //

//
// The bad virtual address is a 64-bit address. Check to ensure that it is a
// user address and it is within the virtual address range supported by the
// system.
//

30:     lui     k0,(PTE64_BASE >> 15) & 0xffff // get PTE base address
        addu    k0,k0,k1                // compute address of PTE
        srl     k1,k1,26                // isolate upper bits of address
        beq     zero,k1,10b             // if eq, valid 64-bit user address

#if defined(NT_UP)

        dmfc0   k1,badvaddr             // get bad virtual address

#else

        mfc0    k1,index                // get index register

#endif

        j       KiInvalidUserAddress    //
        nop                             //
        .set    at
        .set    reorder

        END_REGION(KiXTbMissEndAddress3.x)

//
// The r10000 TB miss routine is different since the fine designers of the
// chip didn't understand what the frame mask register was really for and
// only masked PFN bits. Unfortunately they didn't mask the UC bits which
// require the bits to be masked manually.
//

        START_REGION(KiXTbMissStartAddress9.x)

        .set    noreorder
        .set    noat
        dmfc0   k1,xcontext             // get extended context register
        mfc0    k0,context              // get virtual address * 2 of PTE
        dsrl    k1,k1,22                // isolate bits 63:62 and 43:31
        daddu   k1,k1,1                 //
        and     k1,k1,0x7ff             // check if valid 32-bit address
        sltu    k1,k1,2                 //
        beq     zero,k1,30f             // if eq, not valid 32-bit address
        dmfc0   k1,xcontext             // get extended context register
        dmfc0   k1,badvaddr             // get bad virtual address
10:     sra     k0,k0,1                 // compute virtual address of PTE
        mtc0    k0,taglo                // set first level active flag
        lwu     k1,0(k0)                // get first PTE - may fault
        lwu     k0,4(k0)                // get second PTE - no fault
        mtc0    k1,entrylo0             // set first PTE value
        mtc0    k0,entrylo1             // set second PTE value
        nop                             //
        tlbwr                           // write entry randomly into TB
        nop                             // 3 cycle hazzard
        nop                             //
        mtc0    zero,taglo              // 1 cycle hazzard - clear active flag
        eret                            //

//
// The bad virtual address is a 64-bit address. Check to ensure that it is a
// user address and it is within the virtual address range supported by the
// system.
//

30:     lui     k0,(PTE64_BASE >> 15) & 0xffff // get PTE base address
        addu    k0,k0,k1                // compute address of PTE
        srl     k1,k1,26                // isolate upper bits of address
        beq     zero,k1,10b             // if eq, valid 64-bit user address
        dmfc0   k1,badvaddr             // get bad virtual address
35:     j       KiInvalidUserAddress    //
        nop                             //
        .set    at
        .set    reorder

        END_REGION(KiXTbMissEndAddress9.x)

        .end    KiXTbMiss

        SBTTL("Cache Parity Error Vector Routine")
//++
//
// Routine Description:
//
//    This routine is entered as the result of a cache parity error and runs
//    uncached. Its function is to remap the PCR uncached and call the cache
//    parity routine to save all pertinent cache error information, establish
//    an error stack frame, and call the system cache parity error routine.
//
//    N.B. The cache parity error routine runs uncached and must be
//         extremely careful not access any cached addresses.
//
//    N.B. If a second exception occurs while cache error handling is in
//         progress, then a soft reset is performed by the hardware.
//
//    N.B. While ERL is set in the PSR, the user address space is replaced
//         by an uncached, unmapped, address that corresponds to physical
//         memory.
//
//    N.B. There is room for up to 32 instructions in the vectored cache
//         parity error routine.
//
//    This routine is copied to address KSEG1_BASE + 0x100 at system startup.
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

        LEAF_ENTRY(KiCacheError)

        START_REGION(KiCacheErrorStartAddress)

        .set    noreorder
        .set    noat
        nop                             // fill
        nop                             // fill
        la      k0,CACHE_ERROR_VECTOR   // get cache error vector address
        lw      k0,0(k0)                // get cache error routine address
        nop                             // fill
        j       k0                      // dispatch to cache error routine
        nop                             // fill
        .set    at
        .set    reorder

        END_REGION(KiCacheErrorEndAddress)

        .end    KiCacheError

        SBTTL("General Exception Vector Routine")
//++
//
// Routine Description:
//
//    This routine is entered as the result of a general exception. The reason
//    for the exception is contained in the cause register. When this routine
//    is entered, interrupts are disabled.
//
//    The primary function of this routine is to route the exception to the
//    appropriate exception handling routine. If the cause of the exception
//    is a read or write TB miss and the access can be resolved, then this
//    routine performs the necessary processing and returns from the exception.
//    If the exception cannot be resolved, then it is dispatched to the proper
//    routine.
//
//    This routine is copied to address KSEG0_BASE + 0x180 at system startup.
//
//    N.B. This routine is very carefully written to not destroy k1 until
//       it has been determined that the exception did not occur in the
//       user TB miss vector routine.
//
// Arguments:
//
//    k1 - Supplies the bad virtual address if the exception occurred from
//       the TB miss vector routine while attempting to load a PTE into the
//       TB.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiGeneralException)

        START_REGION(KiGeneralExceptionStartAddress)

        .set    noreorder
        .set    noat
        mfc0    k0,cause                // get cause of exception
        sd      k1,KiPcr + PcTmpVaddr(zero) // save bad virtual address
        li      k1,XCODE_READ_MISS      // get exception code for read miss
        and     k0,k0,R4000_MISS_MASK   // isolate exception code

//
// The read and write miss codes differ by exactly one bit such that they
// can be tested for by a single mask operation followed by a test for the
// read miss code.
//

        bne     k0,k1,20f               // if ne, not read or write miss
        dmfc0   k1,badvaddr             // get the bad virtual address

//
// The exception is either a read or a write to an address that is not mapped
// by the TB, or a reference to an invalid entry that is in the TB. Attempt to
// resolve the reference by loading a pair of a PDEs from the page directory
// page.
//
// There are four cases to be considered:
//
//    1. The address specified by the badvaddr register is not in the TB.
//
//       For this case, a pair of PDEs are loaded into the TB from the
//       page directory page and execution is resumed if the badvaddr is
//       a valid 32-bit or 64-bit address. If the address is an invalid
//       64-bit address, then report the TB miss as an exception.
//
//    2. The address specified by the badvaddr register is in the TB and the
//       address is not the address of a page table page.
//
//       For this case an invalid translation has occured, but since it is
//       not the address of a page table page, then it could not have come
//       from the TB Miss handler. The badvaddr register contains the virtual
//       address of the exception and is passed to the appropriate exception
//       routine.
//
//    3. The address specified by the badvaddr register is in the TB, the
//       address is the address of a page table page, and the first level
//       TB miss routine was active when the current TB miss occurred.
//
//       For this case, an invalid translation has occured, but since it is
//       a page table page and the first level TB miss routine active flag
//       is set, then the exception occured in the TB Miss handler. The
//       integer register k1 contains the virtual address of the exception
//       as saved by the first level TB fill handler and is passed to the
//       appropriate exception routine.
//
//       N.B. The virtual address that is passed to the exception routine is
//            the exact virtual address that caused the fault and is obtained
//            from integer register k1.
//
//    4. The address specified by the badvaddr register is in the TB, the
//       address is the address of a page table page, and the first level
//       TB miss routine was not active when the current TB miss occurred.
//
//       For this case, an invalid translation has occured, but since it is
//       a page table page and the first level TB miss routine active flag
//       is clear, then the exception must have occured as part of a probe
//       operation or is a page fault to an invalid page.
//
//       N.B. The virtual address that is passed to the exception routine is
//            the exact virtual address that caused the fault and is obtained
//            from the badvaddr register.
//

        tlbp                            // probe TB for the faulting address
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    k1,index                // read result of probe
        mfc0    k0,context              // get virtual address * 2 of PDE
        bgez    k1,10f                  // if gez, entry is in TB
        sra     k0,k0,ENTRYHI_VPN2 + 1  // isolate VPN2 of PDE address

//
// Case 1 - The entry is not in the TB.
//
// The TB miss is a reference to a page table page and a pair of PDEs are
// loaded into the TB from the page directory page and execution is continued.
//
// Probe the TB to determine if the PTEs that map the PDE page are in the TB.
// This will always succeed for 32-bit translations since the mapping for the
// PDE page is locked in the TB. For 64-bit addresses this may or may not
// succeed since the mapping of the PDE page for the 64-bit page tables is not
// locked in the TB.
//

        mfc0    k1,entryhi              // get current VPN2 and PID
        sll     k0,k0,ENTRYHI_VPN2      // shift VPN2 into place
        and     k1,k1,PID_MASK << ENTRYHI_PID // isolate current PID
        or      k0,k0,k1                // merge PID with VPN2 of address
        mfc0    k1,entryhi              // save current VPN2 and PID
        mtc0    k0,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // two cycle hazzard
        nop                             //
        mfc0    k0,index                // read result of probe
        mtc0    k1,entryhi              // restore current VPN2 and PID
        bltz    k0,2f                   // if ltz, entry not in TB
        mfc0    k0,context              // get virtual address * 2 of PDE
        sra     k0,k0,1                 // compute virtual address of PTE

//
// The address of the page that contains the PDEs is currently mapped by
// TB entry. Load a pair of enries in the TB and continue execution.
//

        lw      k1,4(k0)                // get second PDE value
        lw      k0,0(k0)                // get first PDE value
        mtc0    k1,entrylo1             // set second PTE value
        mtc0    k0,entrylo0             // set first PTE value

#if DBG

        xor     k1,k1,k0                // compare G-bits
        and     k1,k1,1 << ENTRYLO_G    // isolate G-bit
        beq     zero,k1,1f              // if eq, G-bits match
        nop                             // fill
        mtc0    zero,entrylo0           // reset first PTE value
        mtc0    zero,entrylo1           // reset second PTE value
1:                                      //

#endif

        nop                             //
        tlbwr                           // write entry randomly into TB
        nop                             // 3 cycle hazzard
        nop                             //
        mtc0    zero,taglo              // 1 cycle hazzard - clear active flag

#if DBG

        lw      k0,KiPcr + PcPrcb(zero) // get processor block address
        nop                             // fill
        lw      k1,PbSecondLevelTbFills(k0) // increment number of second level
        nop                             // fill
        addu    k1,k1,1                 // TB fills
        sw      k1,PbSecondLevelTbFills(k0) //

#endif

        eret                            //

//
// The TB does not contain an entry that maps the PDE page required to map
// the PTEs of the reference. This can only happen on a 64-bit reference
// since the PDE pages for 64-bit addresses are not locked in the TB. Load
// the PDE pair that maps the PDE page into the TB and continue execution.
//
// N.B. Both PDEs that are loaded into the TB MUST be valid.
//

2:      mfc0    k1,entryhi              // get current VPN2 and PID
        sra     k0,k0,ENTRYHI_VPN2 + 1  // isolate VPN2 of PDE address
        sll     k0,k0,ENTRYHI_VPN2      //
        and     k1,k1,PID_MASK << ENTRYHI_PID // isolate current PID
        or      k0,k0,k1                // merge PID with VPN2 of address
        mtc0    k0,entryhi              // set VPN2 and PID for TB write
        sd      k0,KiPcr + PcSystemReserved + 16(zero) // *******
        lui     k1,PTE_BASE >> 16       // get base virtual page table address
        srl     k0,k0,ENTRYHI_VPN2 - 3  // compute address of PDE pair
        addu    k0,k0,k1                //
        sd      k0,KiPcr + PcSystemReserved + 24(zero) // *******
        lw      k1,4(k0)                // get second PDE value
        lw      k0,0(k0)                // get first PDE value
        mtc0    k1,entrylo1             // set second PTE value
        mtc0    k0,entrylo0             // set first PTE value
        or      k0,k0,k1                // check if both PDEs are valid
        and     k0,k0,1 << ENTRYLO_V    //
        beq     zero,k0,15f             // if eq, both entries not valid
        nop                             //
        tlbwr                           // write entry randomly into TB
        nop                             // 3 cycle hazzard
        nop                             //
        mtc0    zero,taglo              // 1 cycle hazzard - clear active flag
        eret                            //

//
// Case 2, 3, or 4 - The entry is in the TB.
//
// Check for one of the three remaining cases.
//

10:     dmfc0   k1,badvaddr             // get bad virtual address
        srl     k1,k1,PDI_SHIFT         // isolate page directory index
        xor     k1,k1,PDE_BASE >> PDI_SHIFT // check if page table reference
        bne     zero,k1,20f             // if ne, not a page table page
        dmfc0   k1,badvaddr             // get bad virtual address

//
// Case 2 or 3 - The bad virtual address is the address of a page table page.
//
// Check for one of the two remaining cases.
//

15:     mfc0    k0,taglo                // get first level flag
        beq     zero,k0,20f             // if eq, not first level miss
        dmfc0   k1,badvaddr             // get bad virtual address
        ld      k1,KiPcr + PcTmpVaddr(zero) // get bad virtual address

//
// Save bad virtual address in case it is needed by the exception handling
// routine.
//

20:     mfc0    k0,epc                  // get exception PC
        mtc0    zero,taglo              // clear first level miss flag
        sd      t7,KiPcr + PcSavedT7(zero) // save integer registers t7 - t9
        sd      t8,KiPcr + PcSavedT8(zero) //
        sd      t9,KiPcr + PcSavedT9(zero) //
        sw      k0,KiPcr + PcSavedEpc(zero) // save exception PC
        sd      k1,KiPcr + PcBadVaddr(zero) // save bad virtual address

//
// The bad virtual address is saved in the PCR in case it is needed by the
// respective dispatch routine.
//
// N.B. EXL must be cleared in the current PSR so switching the stack
//      can occur with TB Misses enabled.
//

        mfc0    t9,psr                  // get current processor status
        li      t8,CU1_ENABLE           // set coprocessor 1 enable bits
        mfc0    t7,cause                // get cause of exception
        mtc0    t8,psr                  // clear EXL and disable interrupts
        lw      k1,KiPcr + PcInitialStack(zero) // get initial kernel stack
        and     t8,t9,1 << PSR_PMODE    // isolate previous processor mode
        bnel    zero,t8,30f             // if ne, previous mode was user
        subu    t8,k1,TrapFrameLength   // allocate trap frame

//
// If the kernel stack has overflowed, then a switch to the panic stack is
// performed and the exception/ code is set to cause a bug check.
//

        lw      k1,KiPcr + PcStackLimit(zero) // get current stack limit
        subu    t8,sp,TrapFrameLength   // allocate trap frame
        sltu    k1,t8,k1                // check for stack overflow
        beql    zero,k1,30f             // if eq, no stack overflow
        nop                             // fill

//
// The kernel stack has either overflowed. Switch to the panic stack and
// cause a bug check to occur by setting the exception cause value to the
// panic code.
//

        lw      t7,KiPcr + PcInitialStack(zero) // ***** temp ****
        lw      t8,KiPcr + PcStackLimit(zero) // ***** temp ****
        sw      t7,KiPcr + PcSystemReserved(zero) // **** temp ****
        sw      t8,KiPcr + PcSystemReserved + 4(zero) // **** temp ****
        lw      k1,KiPcr + PcPanicStack(zero) // get address of panic stack
        li      t7,XCODE_PANIC          // set cause of exception to panic
        sw      k1,KiPcr + PcInitialStack(zero) // reset initial stack pointer
        subu    t8,k1,KERNEL_STACK_SIZE // compute and set stack limit
        sw      t8,KiPcr + PcStackLimit(zero) //
        subu    t8,k1,TrapFrameLength   // allocate trap frame

//
// Allocate a trap frame, save parital context, and dispatch to the appropriate
// exception handling routine.
//
// N.B. At this point:
//
//          t7 contains the cause of the exception,
//          t8 contains the new stack pointer, and
//          t9 contains the previous processor state.
//
//      Since the kernel stack is not wired into the TB, a TB miss can occur
//      during the switch of the stack and the subsequent storing of context.
//
//

30:     sd      sp,TrXIntSp(t8)         // save integer register sp
        move    sp,t8                   // set new stack pointer
        cfc1    t8,fsr                  // get floating status register
        sd      gp,TrXIntGp(sp)         // save integer register gp
        sd      s8,TrXIntS8(sp)         // save integer register s8
        sw      t8,TrFsr(sp)            // save current FSR
        sw      t9,TrPsr(sp)            // save processor state
        sd      ra,TrXIntRa(sp)         // save integer register ra
        lw      gp,KiPcr + PcSystemGp(zero) // set system general pointer
        and     t8,t7,R4000_XCODE_MASK  // isolate exception code

//
// Check for system call exception.
//
// N.B. While k1 is being used a TB miss cannot be tolerated.
//

        xor     k1,t8,XCODE_SYSTEM_CALL // check for system call exception
        bne     zero,k1,40f             // if ne, not system call exception
        move    s8,sp                   // set address of trap frame

//
// Get the address of the current thread and form the next PSR value.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        li      t8,PSR_MASK             // get the PSR mask
        and     t8,t9,t8                // clear EXL and mode in PSR
        sw      ra,TrFir(s8)            // set real continuation address
        sb      zero,TrSavedFlag(s8)    // clear s-registers saved flag
        j       KiSystemServiceNormal   // execute normal system service
        mtc0    t8,psr                  // enable interrupts

//
// Save the volatile integer register state.
//

40:     sd      AT,TrXIntAt(s8)         // save assembler temporary register
        sd      v0,TrXIntV0(s8)         // save integer register v0
        sd      v1,TrXIntV1(s8)         // save integer register v1
        sd      a0,TrXIntA0(s8)         // save integer registers a0 - a3
        sd      a1,TrXIntA1(s8)         //
        sd      a2,TrXIntA2(s8)         //
        sd      a3,TrXIntA3(s8)         //
        sd      t0,TrXIntT0(s8)         // save integer registers t0 - t2
        sd      t1,TrXIntT1(s8)         //
        sd      t2,TrXIntT2(s8)         //
        ld      t0,KiPcr + PcSavedT7(zero) // get saved register t8 - t9
        ld      t1,KiPcr + PcSavedT8(zero) //
        ld      t2,KiPcr + PcSavedT9(zero) //
        sd      t3,TrXIntT3(s8)         // save integer register t3 - t7
        sd      t4,TrXIntT4(s8)         //
        sd      t5,TrXIntT5(s8)         //
        sd      t6,TrXIntT6(s8)         //
        sd      t0,TrXIntT7(s8)         //
        sd      s0,TrXIntS0(s8)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(s8)         //
        sd      s2,TrXIntS2(s8)         //
        sd      s3,TrXIntS3(s8)         //
        sd      s4,TrXIntS4(s8)         //
        sd      s5,TrXIntS5(s8)         //
        sd      s6,TrXIntS6(s8)         //
        sd      s7,TrXIntS7(s8)         //
        sd      t1,TrXIntT8(s8)         // save integer registers t8 - t9
        sd      t2,TrXIntT9(s8)         //
        mflo    t3                      // get multiplier/quotient lo and hi
        mfhi    t4                      //
        lw      t5,KiPcr + PcXcodeDispatch(t8) // get exception routine address
        xor     t6,t8,XCODE_INTERRUPT   // check for interrupt exception
        lw      t8,KiPcr + PcSavedEpc(zero) // get exception PC
        sd      t3,TrXIntLo(s8)         // save multiplier/quotient lo and hi
        sd      t4,TrXIntHi(s8)         //
        beq     zero,t6,50f             // if eq, interrupt exception
        sw      t8,TrFir(s8)            // save exception PC

//
// Save the volatile floating register state.
//

        sdc1    f0,TrFltF0(s8)          // save floating register f0 - f19
        sdc1    f2,TrFltF2(s8)          //
        sdc1    f4,TrFltF4(s8)          //
        sdc1    f6,TrFltF6(s8)          //
        sdc1    f8,TrFltF8(s8)          //
        sdc1    f10,TrFltF10(s8)        //
        sdc1    f12,TrFltF12(s8)        //
        sdc1    f14,TrFltF14(s8)        //
        sdc1    f16,TrFltF16(s8)        //
        sdc1    f18,TrFltF18(s8)        //
        srl     t6,t9,PSR_PMODE         // isolate previous mode
        and     t6,t6,1                 //
        li      t0,PSR_MASK             // clear EXL amd mode is PSR
        and     t9,t9,t0                //

//
// Dispatch to exception handing routine with:
//
//    t5 - Address of the exception handling routine.
//    t6 - If not an interrupt, then the previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - If not an interrupt, then the new PSR with EXL and mode clear.
//         Otherwise the previous PSR with EXL and mode set.
//

50:     li      t4,TRUE                 // get saved s-registers flag
        bltzl   t7,60f                  // if ltz, exception in delay slot
        addu    t8,t8,4                 // compute address of exception
60:     j       t5                      // dispatch to exception routine
        sb      t4,TrSavedFlag(s8)      // set s-registers saved flag
        .set    at
        .set    reorder

        END_REGION(KiGeneralExceptionEndAddress)

        .end    KiGeneralException

        SBTTL("Invalid User Address")
//++
//
// Routine Description:
//
//    This routine is entered when an invalid user address is encountered
//    in the XTB Miss handler. When this routine is entered, interrupts
//    are disabled.
//
//    The primary function of this routine is to route the exception to the
//    invalid user 64-bit address exception handling routine.
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

        LEAF_ENTRY(KiInvalidUserAddress)

        .set    noreorder
        .set    noat
        dmfc0   k1,badvaddr             // get the bad virtual address
        dmfc0   k0,epc                  // get exception PC
        sd      k1,KiPcr + PcBadVaddr(zero) // save bad virtual address
        sd      k1,KiPcr + PcSystemReserved(zero) // **** temp ****
        dmfc0   k1,xcontext             // **** temp ****
        sd      k0,KiPcr + PcSystemReserved + 8(zero) // **** temp ****
        sd      k1,KiPcr + PcSystemReserved + 16(zero) // **** temp ****
        sd      t7,KiPcr + PcSavedT7(zero) // save integer registers t7 - t9
        sd      t8,KiPcr + PcSavedT8(zero) //
        sd      t9,KiPcr + PcSavedT9(zero) //
        sw      k0,KiPcr + PcSavedEpc(zero) // save exception PC

//
// The bad virtual address is saved in the PCR in case it is needed by the
// respective dispatch routine.
//
// N.B. EXL must be cleared in the current PSR so switching the stack
//      can occur with TB Misses enabled.
//

        mfc0    t9,psr                  // get current processor status
        li      t8,CU1_ENABLE           // set coprocessor 1 enable bits
        mfc0    t7,cause                // get cause of exception
        mtc0    t8,psr                  // clear EXL and disable interrupts
        lw      k1,KiPcr + PcInitialStack(zero) // get initial kernel stack
        and     t8,t9,1 << PSR_PMODE    // isolate previous processor mode
        bnel    zero,t8,10f             // if ne, previous mode was user
        subu    t8,k1,TrapFrameLength   // allocate trap frame

//
// If the kernel stack has overflowed, then a switch to the panic stack is
// performed and the exception/ code is set to cause a bug check.
//

        lw      k1,KiPcr + PcStackLimit(zero) // get current stack limit
        subu    t8,sp,TrapFrameLength   // allocate trap frame
        sltu    k1,t8,k1                // check for stack overflow
        beql    zero,k1,10f             // if eq, no stack overflow
        nop                             // fill

//
// The kernel stack has either overflowed. Switch to the panic stack and
// cause a bug check to occur by setting the exception cause value to the
// panic code.
//

        lw      k1,KiPcr + PcPanicStack(zero) // get address of panic stack
        li      t7,XCODE_PANIC          // set cause of exception to panic
        sw      k1,KiPcr + PcInitialStack(zero) // reset initial stack pointer
        subu    t8,k1,KERNEL_STACK_SIZE // compute and set stack limit
        sw      t8,KiPcr + PcStackLimit(zero) //
        subu    t8,k1,TrapFrameLength   // allocate trap frame

//
// Allocate a trap frame, save parital context, and dispatch to the appropriate
// exception handling routine.
//
// N.B. At this point:
//
//          t7 contains the cause of the exception,
//          t8 contains the new stack pointer, and
//          t9 contains the previous processor state.
//
//      Since the kernel stack is not wired into the TB, a TB miss can occur
//      during the switch of the stack and the subsequent storing of context.
//
//

10:     sd      sp,TrXIntSp(t8)         // save integer register sp
        move    sp,t8                   // set new stack pointer
        cfc1    t8,fsr                  // get floating status register
        sd      gp,TrXIntGp(sp)         // save integer register gp
        sd      s8,TrXIntS8(sp)         // save integer register s8
        sw      t8,TrFsr(sp)            // save current FSR
        sw      t9,TrPsr(sp)            // save processor state
        sd      ra,TrXIntRa(sp)         // save integer register ra
        lw      gp,KiPcr + PcSystemGp(zero) // set system general pointer
        and     t8,t7,R4000_XCODE_MASK  // isolate exception code

//
// Check for panic stack switch.
//
// N.B. While k1 is being used a TB miss cannot be tolerated.
//

        xor     k1,t8,XCODE_PANIC       // check for panic stack switch
        bnel    zero,k1,20f             // if ne, invalid user address
        li      t8,XCODE_INVALID_USER_ADDRESS // set exception dispatch code

//
// Save the volatile integer register state.
//

20:     move    s8,sp                   // set address of trap frame
        sd      AT,TrXIntAt(s8)         // save assembler temporary register
        sd      v0,TrXIntV0(s8)         // save integer register v0
        sd      v1,TrXIntV1(s8)         // save integer register v1
        sd      a0,TrXIntA0(s8)         // save integer registers a0 - a3
        sd      a1,TrXIntA1(s8)         //
        sd      a2,TrXIntA2(s8)         //
        sd      a3,TrXIntA3(s8)         //
        sd      t0,TrXIntT0(s8)         // save integer registers t0 - t2
        sd      t1,TrXIntT1(s8)         //
        sd      t2,TrXIntT2(s8)         //
        ld      t0,KiPcr + PcSavedT7(zero) // get saved register t8 - t9
        ld      t1,KiPcr + PcSavedT8(zero) //
        ld      t2,KiPcr + PcSavedT9(zero) //
        sd      t3,TrXIntT3(s8)         // save integer register t3 - t7
        sd      t4,TrXIntT4(s8)         //
        sd      t5,TrXIntT5(s8)         //
        sd      t6,TrXIntT6(s8)         //
        sd      t0,TrXIntT7(s8)         //
        sd      s0,TrXIntS0(s8)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(s8)         //
        sd      s2,TrXIntS2(s8)         //
        sd      s3,TrXIntS3(s8)         //
        sd      s4,TrXIntS4(s8)         //
        sd      s5,TrXIntS5(s8)         //
        sd      s6,TrXIntS6(s8)         //
        sd      s7,TrXIntS7(s8)         //
        sd      t1,TrXIntT8(s8)         // save integer registers t8 - t9
        sd      t2,TrXIntT9(s8)         //
        mflo    t3                      // get multiplier/quotient lo and hi
        mfhi    t4                      //
        lw      t5,KiPcr + PcXcodeDispatch(t8) // get exception routine address
        lw      t8,KiPcr + PcSavedEpc(zero) // get exception PC
        sd      t3,TrXIntLo(s8)         // save multiplier/quotient lo and hi
        sd      t4,TrXIntHi(s8)         //
        sw      t8,TrFir(s8)            // save exception PC

//
// Save the volatile floating register state.
//

        sdc1    f0,TrFltF0(s8)          // save floating register f0 - f19
        sdc1    f2,TrFltF2(s8)          //
        sdc1    f4,TrFltF4(s8)          //
        sdc1    f6,TrFltF6(s8)          //
        sdc1    f8,TrFltF8(s8)          //
        sdc1    f10,TrFltF10(s8)        //
        sdc1    f12,TrFltF12(s8)        //
        sdc1    f14,TrFltF14(s8)        //
        sdc1    f16,TrFltF16(s8)        //
        sdc1    f18,TrFltF18(s8)        //
        srl     t6,t9,PSR_PMODE         // isolate previous mode
        and     t6,t6,1                 //
        li      t0,PSR_MASK             // clear EXL amd mode is PSR
        and     t9,t9,t0                //

//
// Dispatch to exception handing routine with:
//
//    t5 - Address of the exception handling routine.
//    t6 - Previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//

        li      t4,TRUE                 // get saved s-registers flag
        bltzl   t7,30f                  // if ltz, exception in delay slot
        addu    t8,t8,4                 // compute address of exception
30:     j       t5                      // dispatch to exception routine
        sb      t4,TrSavedFlag(s8)      // set s-registers saved flag
        .set    at
        .set    reorder

        .end    KiInvalidUserAddress

        SBTTL("Address Error Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiAddressErrorDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a read or write address error exception
//    code is read from the cause register. When this routine is entered,
//    interrupts are disabled.
//
//    The function of this routine is to raise an data misalignment exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiReadAddressErrorException)

        li      t0,0                    // set read indicator
        b       10f                     // join common code

        ALTERNATE_ENTRY(KiWriteAddressErrorException)

        li      t0,1                    // set write indicator

//
// Common code for read and write address error exceptions.
//

10:     addu    a0,s8,TrExceptionRecord // compute exception record address
        lw      t1,KiPcr + PcBadVaddr(zero) // get bad virtual address

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        sw      t0,ErExceptionInformation(a0) // save load/store indicator
        sw      t1,ErExceptionInformation + 4(a0) // save bad virtual address
        sw      t8,ErExceptionAddress(a0) // set exception address

//
// If the faulting instruction address is the same as the faulting virtual
// address, then the fault is an instruction misalignment exception. Otherwise,
// the exception is a data misalignment.
//

        li      t3,STATUS_INSTRUCTION_MISALIGNMENT // set exception code
        beq     t1,t8,20f               // if eq, instruction misalignment
        li      t3,STATUS_DATATYPE_MISALIGNMENT // set exception code

//
// If the faulting address is a kernel address and the previous mode was
// user, then the address error is really an access violation since an
// attempt was made to access kernel memory from user mode.
//

20:     bgez    t1,30f                  // if gez, KUSEG address
        beq     zero,a3,30f             // if eq, previous mode was kernel
        li      t3,STATUS_ACCESS_VIOLATION // set exception code
30:     sw      t3,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        li      t0,2                    // set number of exception parameters
        sw      t0,ErNumberParameters(a0) //
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiAddressErrorDispatch

        SBTTL("Breakpoint Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiBreakpointDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a breakpoint exception code is read from the
//    cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to raise a breakpoint exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiBreakpointException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception
        lw      t0,0(t8)                // get breakpoint instruction

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        sw      t0,ErExceptionInformation(a0) // save breakpoint instruction
        li      t1,STATUS_BREAKPOINT    // set exception code
        sw      t1,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code

        ALTERNATE_ENTRY(KiKernelBreakpoint)

        break   KERNEL_BREAKPOINT       // kernel breakpoint instruction

        .end    KiBreakpointDispatch

        SBTTL("Bug Check Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiBugCheckDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when the following codes are read from the cause
//    register:
//
//      Data coherency,
//      Instruction coherency,
//      Invlid exception, and
//      Panic exception.
//
//    The function of this routine is to cause a bug check with the appropriate
//    code.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiDataCoherencyException)

        li      a0,DATA_COHERENCY_EXCEPTION // set bug check code
        b       10f                     // finish in common code

        ALTERNATE_ENTRY(KiInstructionCoherencyException)

        li      a0,INSTRUCTION_COHERENCY_EXCEPTION // set bug check code
         b      10f                     // finish in common code

        ALTERNATE_ENTRY(KiInvalidException)

        li      a0,TRAP_CAUSE_UNKNOWN   // set bug check code
        b       10f                     // finish in common code

        ALTERNATE_ENTRY(KiPanicException)

        li      a0,PANIC_STACK_SWITCH   // set bug check code
10:     lw      a1,KiPcr + PcBadVaddr(zero) // get bad virtual address

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a2,t8                   // set address of faulting instruction
        .set    at
        .set    reorder

        move    a3,t6                   // set previous mode
        jal     KeBugCheckEx            // call bug check routine
        j       KiExceptionExit         // dummy jump for filler

        .end    KiBugCheckDispatch

        SBTTL("Coprocessor Unusable Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiCoprocessorUnusableDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a coprocessor unusable exception code is read
//    from the cause register. When this routine is entered, interrupts are
//    disabled.
//
//    The function of this routine is to raise an illegal instruction exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiCoprocessorUnusableException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,STATUS_ILLEGAL_INSTRUCTION // set exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiCoprocessorUnusableDispatch

        SBTTL("Data Bus Error Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiDataBusErrorDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a data bus error exception code is read from
//    the cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to capture the current machine state and
//    call the exception dispatcher which will provide specical case processing
//    of this exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiDataBusErrorException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,DATA_BUS_ERROR | 0xdfff0000 // set special exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiDataBusErrorDispatch

        SBTTL("Floating Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiFloatDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a floating exception code is read from the
//    cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to raise a floating exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiFloatingException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        cfc1    t0,fsr                  // get current floating status
        li      t1,~(0x3f << FSR_XI)    // get exception mask value
        and     t1,t0,t1                // clear exception bits
        ctc1    t1,fsr                  // set new floating status
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,STATUS_FLOAT_STACK_CHECK // set floating escape code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiFloatDispatch

        SBTTL("Illegal Instruction Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiIllegalInstructionDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when an illegal instruction exception code is read
//    from the cause register. When this routine is entered, interrupts are
//    disabled.
//
//    The function of this routine is to raise an illegal instruction exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiIllegalInstructionException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,STATUS_ILLEGAL_INSTRUCTION // set exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiIllegalInstructionDispatch

        SBTTL("Instruction Bus Error Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiInstructionBusErrorDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when an instruction bus error exception code is read
//    from the cause register. When this routine is entered, interrupts are
//    disabled.
//
//    The function of this routine is to capture the current machine state and
//    call the exception dispatcher which will provide specical case processing
//    of this exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiInstructionBusErrorException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,INSTRUCTION_BUS_ERROR | 0xdfff0000 // set special exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiInstructionBusErrorDispatch

        SBTTL("Integer Overflow Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiIntegerOverflowDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when an integer overflow exception code is read
//    from the cause register. When this routine is entered, interrupts are
//    disabled.
//
//    The function of this routine is to raise an integer overflow exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiIntegerOverflowException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t0,STATUS_INTEGER_OVERFLOW // set exception code
        sw      t0,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiIntegerOverflowDispatch

        SBTTL("Interrupt Exception")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        EXCEPTION_HANDLER(KiInterruptHandler)

        NESTED_ENTRY(KiInterruptDistribution, TrapFrameLength, zero);

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when an interrupt exception code is read from the
//    cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to determine the highest priority pending
//    interrupt, raise the IRQL to the level of the highest interrupt, and then
//    dispatch the interrupt to the proper service routine.
//
// Arguments:
//
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The old PSR with EXL and mode set.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiInterruptException)

        .set    noreorder
        .set    noat
        lbu     t1,KiPcr + PcCurrentIrql(zero) // get current IRQL
        srl     t2,t7,CAUSE_INTPEND + 4  // isolate high interrupt pending bits
        and     t2,t2,0xf               //
        bne     zero,t2,10f             // if ne, use high bits as index
        sb      t1,TrOldIrql(s8)        // save old IRQL
        srl     t2,t7,CAUSE_INTPEND     // isolate low interrupt pending bits
        and     t2,t2,0xf               //
        addu    t2,t2,16                // bias low bits index by 16
10:     lbu     t0,KiPcr + PcIrqlMask(t2) // get new IRQL from mask table
        li      t2,PSR_ENABLE_MASK      // get PSR enable mask
        nor     t2,t2,zero              // complement interrupt enable mask
        lbu     t3,KiPcr + PcIrqlTable(t0) // get new mask from IRQL table

//
// It is possible that the interrupt was asserted and then deasserted before
// the interrupt dispatch code executed. Therefore, there may be an interrupt
// pending at the current or a lower level. This interrupt is not yet valid
// and cannot be processed until the IRQL is lowered.
//

        sltu    t4,t1,t0                // check if old IRQL less than new
        beq     zero,t4,40f             // if eq, no valid interrupt pending
        subu    t4,t0,DISPATCH_LEVEL + 1 // check if above dispatch level

//
// If the interrupt level is above dispatch level, then execute the service
// routine on the interrupt stack. Otherwise, execute the service on the
// current stack.
//

        bgezal  t4,60f                  // if gez, above dispatch level
        sll     t3,t3,PSR_INTMASK       // shift table entry into position

//
// N.B. The following code is duplicated on the control path where the stack
//      is switched to the interrupt stack. This is done to avoid branching
//      logic.
//

        and     t9,t9,t2                // clear interrupt mask, EXL, and KSU
        or      t9,t9,t3                // merge new interrupt enable mask
        or      t9,t9,1 << PSR_IE       // set interrupt enable
        mtc0    t9,psr                  // enable interrupts
        sb      t0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        .set    at
        .set    reorder

        sll     t0,t0,2                 // compute offset in vector table
        lw      a0,KiPcr + PcInterruptRoutine(t0) // get service routine address

#if DBG

        sw      a0,TrExceptionRecord(s8) // save service routine address

#endif

//
// Increment interrupt count and call interrupt service routine.
//
// N.B. It is known that the interrupt is either an APC interrupt or
//      a dispatch interrupt, and therefore, the volatile floating
//      state is saved and restored to avoid saves and restores in
//      both interrupt dispatchers.
//

        SAVE_VOLATILE_FLOAT_STATE       // save volatile floating state

        lw      t2,KiPcr + PcPrcb(zero) // get current processor block address
        lw      t3,PbInterruptCount(t2) // increment the count of interrupts
        addu    t3,t3,1                 //
        sw      t3,PbInterruptCount(t2) // store result
        jal     a0                      // call interrupt service routine

        RESTORE_VOLATILE_FLOAT_STATE    // restore volatile floating state

//
// Common exit point for special dispatch and APC interrupt bypass.
//
// Restore state and exit interrupt.
//

        ALTERNATE_ENTRY(KiInterruptExit)

40:     lw      t1,TrFsr(s8)            // get previous floating status
        li      t0,CU1_ENABLE           // set coprocessor 1 enable bits

        .set    noreorder
        .set    noat
        mtc0    t0,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t1,fsr                  // restore floating status
        lw      t0,TrPsr(s8)            // get previous processor status
        lw      t1,TrFir(s8)            // get continuation address
        lw      t2,KiPcr + PcCurrentThread(zero) // get current thread address
        lbu     t3,TrOldIrql(s8)        // get old IRQL
        and     t4,t0,1 << PSR_PMODE    // check if previous mode was user
        beq     zero,t4,50f             // if eq, previous mode was kernel
        sb      t3,KiPcr + PcCurrentIrql(zero) // restore old IRQL

//
// If a user mode APC is pending, then request an APV interrupt.
//

        lbu     t3,ThApcState + AsUserApcPending(t2) // get user APC pending
        sb      zero,ThAlerted(t2)      // clear kernel mode alerted
        mfc0    t4,cause                // get exception cause register
        sll     t3,t3,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t4,t4,t3                // merge possible APC interrupt request
        mtc0    t4,cause                // set exception cause register

//
// Save the new processor status and continuation PC in the PCR so a TB
// is not possible, then restore the volatile register state.
//

50:     sw      t0,KiPcr + PcSavedT7(zero) // save processor status
        j       KiTrapExit              // join common code
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        .set    at
        .set    reorder

//
// Switch to interrupt stack.
//

60:     j       KiSwitchStacks          //

//
// Increment number of bypassed dispatch interrupts and check if an APC
// interrupt is pending and the old IRQL is zero.
//

        ALTERNATE_ENTRY(KiContinueInterrupt)

        .set    noreorder
        .set    noat
        lw      t7,KiPcr + PcPrcb(zero) // get current PRCB
        li      t1,CU1_ENABLE           // get coprocessor 1 enable bits
        mfc0    t9,psr                  // get current PSR
        mtc0    t1,psr                  // disable interrupts - 3 cycle hazzard
        lw      t1,PbDpcBypassCount(t7) // increment the DPC bypass count
        li      t2,PSR_ENABLE_MASK      // get PSR enable mask
        lbu     t8,TrOldIrql(s8)        // get old IRQL
        mfc0    t6,cause                // get exception cause register
        addu    t1,t1,1                 //
        sw      t1,PbDpcBypassCount(t7) // store result
        and     t5,t6,APC_INTERRUPT     // check for an APC interrupt
        beq     zero,t5,70f             // if eq, no APC interrupt
        li      t0,APC_LEVEL            // set new IRQL to APC_LEVEL
        bne     zero,t8,70f             // if ne, APC interrupts blocked
        move    a0,zero                 // set previous mode to kernel

//
// An APC interrupt is pending.
//

        lbu     t3,KiPcr + PcIrqlTable(t0) // get new mask from IRQL table
        nor     t2,t2,zero              // complement interrupt enable mask
        and     t9,t9,t2                // clear interrupt mask, EXL, and KSU
        sll     t3,t3,PSR_INTMASK       // shift table entry into position
        or      t9,t9,t3                // merge new interrupt enable mask
        sb      t0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        and     t6,t6,DISPATCH_INTERRUPT // clear APC interrupt pending
        mtc0    t6,cause                //
        mtc0    t9,psr                  // enable interrupts
        .set    at
        .set    reorder

        lw      t1,PbApcBypassCount(t7) // increment the APC bypass count
        addu    t1,t1,1                 //
        sw      t1,PbApcBypassCount(t7) //
        move    a1,zero                 // set exception frame address
        move    a2,zero                 // set trap frame address
        jal     KiDeliverApc            // deliver kernel mode APC

70:     RESTORE_VOLATILE_FLOAT_STATE    // restore volatile floating state

        j       KiInterruptExit         //

        .end    KiInterruptDistribution

        SBTTL("Interrupt Stack Switch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        .struct 0
        .space  4 * 4                   // argument register area
        .space  2 * 4                   // fill
SwSp:   .space  4                       // saved stack pointer
SwRa:   .space  4                       // saved return address
SwFrameLength:                          // length of stack frame

        EXCEPTION_HANDLER(KiInterruptHandler)

        NESTED_ENTRY(KiInterruptStackSwitch, SwFrameLength, zero);

        .set    noreorder
        .set    noat
        sw      sp,SwSp(sp)             // save stack pointer
        sw      ra,SwRa(sp)             // save return address
        .set    at
        .set    reorder

        PROLOGUE_END

//
// The interrupt level is above dispatch level. Execute the interrupt
// service routine on the interrupt stack.
//
// N.B. The following code is duplicated on the control path where the stack
//      is not switched to the interrupt stack. This is done to avoid branching
//      logic.
//


        ALTERNATE_ENTRY(KiSwitchStacks)

        .set    noreorder
        .set    noat
        lw      t4,KiPcr + PcOnInterruptStack(zero) // get stack indicator
        sw      sp,KiPcr + PcOnInterruptStack(zero) // set new stack indicator
        sw      t4,TrOnInterruptStack(s8) // save previous stack indicator
        move    t5,sp                   // save current stack pointer
        bne     zero,t4,10f             // if ne, aleady on interrupt stack
        and     t9,t9,t2                // clear interrupt mask, EXL, and KSU

//
// Switch to the interrupt stack.
//

        lw      t6,KiPcr + PcInitialStack(zero) // get old initial stack address
        lw      t7,KiPcr + PcStackLimit(zero) // and stack limit
        lw      sp,KiPcr + PcInterruptStack(zero) // set interrupt stack address
        sw      t6,KiPcr + PcSavedInitialStack(zero) // save old stack address
        sw      t7,KiPcr + PcSavedStackLimit(zero) // and stack limit
        sw      sp,KiPcr + PcInitialStack(zero) // set new initial stack address
        subu    t4,sp,KERNEL_STACK_SIZE // and stack limit
        sw      t4,KiPcr + PcStackLimit(zero) //
10:     subu    sp,sp,SwFrameLength     // allocate stack frame
        sw      t5,SwSp(sp)             // save previous stack pointer
        sw      ra,SwRa(sp)             // save return address
        or      t9,t9,t3                // merge new interrupt enable mask
        or      t9,t9,1 << PSR_IE       // set interrupt enable
        mtc0    t9,psr                  // enable interrupts
        sb      t0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        .set    at
        .set    reorder

        sll     t0,t0,2                 // compute offset in vector table
        lw      a0,KiPcr + PcInterruptRoutine(t0) // get service routine address

#if DBG

        sw      a0,TrExceptionRecord(s8) // save service routine address

#endif

//
// Increment interrupt count and call interrupt service routine.
//

        lw      t2,KiPcr + PcPrcb(zero) // get current processor block address
        lw      t3,PbInterruptCount(t2) // increment the count of interrupts
        addu    t3,t3,1                 //
        sw      t3,PbInterruptCount(t2) // store result
        jal     a0                      // call interrupt service routine

//
// Restore state, and exit interrupt.
//

        lw      t1,TrFsr(s8)            // get previous floating status
        li      t0,CU1_ENABLE           // set coprocessor 1 enable bits

        .set    noreorder
        .set    noat
        mtc0    t0,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t1,fsr                  // restore floating status
        lbu     t8,TrOldIrql(s8)        // get old IRQL
        lw      t9,TrPsr(s8)            // get previous processor status
        lw      t1,TrFir(s8)            // get continuation address

//
// Save the new processor status and continuation PC in the PCR so a TB
// is not possible later, then restore the volatile register state.
//

        lw      t2,TrOnInterruptStack(s8) // get saved stack indicator
        sb      t8,KiPcr + PcCurrentIrql(zero) // restore old IRQL
        sw      t9,KiPcr + PcSavedT7(zero) // save processor status
        bne     zero,t2,KiTrapExit      // if ne, stay on interrupt stack
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        lw      t3,KiPcr + PcSavedInitialStack(zero) // get old initial stack
        lw      t4,KiPcr + PcSavedStackLimit(zero) // get old stack limit
        sltu    t8,t8,DISPATCH_LEVEL    // check if IRQL less than dispatch
        sw      t3,KiPcr + PcInitialStack(zero) // restore old initial stack
        sw      t4,KiPcr + PcStackLimit(zero) // restore old stack limit
        mfc0    t6,cause                // get exception cause register
        beq     zero,t8,KiTrapExit      // if eq, old IRQL dispatch or above
        sw      t2,KiPcr + PcOnInterruptStack(zero) // restore stack indicator

//
// Check if a DPC interrupt is pending since the old IRQL is less than
// DISPATCH_LEVEL and it is more efficient to directly dispatch than
// let the interrupt logic request the interrupt.
//

        and     t8,t6,DISPATCH_INTERRUPT // check for dispatch interrupt
        beql    zero,t8,40f             // if eq, no dispatch interrupt
        lw      t7,KiPcr + PcCurrentThread(zero) // get current thread address

//
// A dispatch interrupt is pending.
//

        move    sp,s8                   // set correct stack pointer
        li      t0,DISPATCH_LEVEL       // set new IRQL to DISPATCH_LEVEL
        lbu     t3,KiPcr + PcIrqlTable(t0) // get new mask from IRQL table
        li      t2,PSR_ENABLE_MASK      // get PSR enable mask
        nor     t2,t2,zero              // complement interrupt enable mask
        sll     t3,t3,PSR_INTMASK       // shift table entry into position
        and     t9,t9,t2                // clear interrupt mask, EXL, and KSU
        or      t9,t9,t3                // merge new interrupt enable mask
        or      t9,t9,1 << PSR_IE       // set interrupt enable
        sb      t0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        mtc0    t9,psr                  // enable interrupts
        .set    at
        .set    reorder

        SAVE_VOLATILE_FLOAT_STATE       // save volatile floating state

//
// N.B. The following code returns to the main interrupt dispatch so
//      get and set context APCs can virtually unwind the stack properly.
//

        la      ra,KiContinueInterrupt  // set return address
        j       KiDispatchInterrupt     // process dispatch interrupt

//
// If the previous mode is user and a user mode APC is pending, then
// request an APC interrupt.
//

        .set    noreorder
        .set    noat
40:     and     t4,t9,1 << PSR_PMODE    // check if previous mode was user
        beq     zero,t4,50f             // if eq, previous mode was kernel
        ld      AT,TrXIntAt(s8)         // restore integer register AT
        lbu     t3,ThApcState + AsUserApcPending(t7) // get user APC pending
        sb      zero,ThAlerted(t7)      // clear kernel mode alerted
        sll     t3,t3,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t6,t6,t3                // merge possible APC interrupt request
        mtc0    t6,cause                // set exception cause register
        .set    at
        .set    reorder

//
// Common trap exit sequence for all traps.
//

        ALTERNATE_ENTRY(KiTrapExit)

        .set    noreorder
        .set    noat
        ld      AT,TrXIntAt(s8)         // restore integer register AT
50:     ld      v0,TrXIntV0(s8)         // restore integer register v0
        ld      v1,TrXIntV1(s8)         // restore integer register v1
        ld      a0,TrXIntA0(s8)         // restore integer registers a0 - a3
        ld      a1,TrXIntA1(s8)         //
        ld      a2,TrXIntA2(s8)         //
        ld      t0,TrXIntLo(s8)         // restore lo and hi integer registers
        ld      t1,TrXIntHi(s8)         //
        ld      a3,TrXIntA3(s8)         //
        mtlo    t0                      //
        mthi    t1                      //
        ld      t0,TrXIntT0(s8)         // restore integer registers t0 - t7
        ld      t1,TrXIntT1(s8)         //
        ld      t2,TrXIntT2(s8)         //
        ld      t3,TrXIntT3(s8)         //
        ld      t4,TrXIntT4(s8)         //
        ld      t5,TrXIntT5(s8)         //
        ld      t6,TrXIntT6(s8)         //
        ld      t7,TrXIntT7(s8)         //
        ld      s0,TrXIntS0(s8)         // restore integer registers s0 - s7
        ld      s1,TrXIntS1(s8)         //
        ld      s2,TrXIntS2(s8)         //
        ld      s3,TrXIntS3(s8)         //
        ld      s4,TrXIntS4(s8)         //
        ld      s5,TrXIntS5(s8)         //
        ld      s6,TrXIntS6(s8)         //
        ld      s7,TrXIntS7(s8)         //
        ld      t8,TrXIntT8(s8)         // restore integer registers t8 - t9
        ld      t9,TrXIntT9(s8)         //

//
// Common exit sequence for system services.
//

        ALTERNATE_ENTRY(KiServiceExit)

        ld      gp,TrXIntGp(s8)         // restore integer register gp
        ld      sp,TrXIntSp(s8)         // restore stack pointer
        ld      ra,TrXIntRa(s8)         // restore return address
        ld      s8,TrXIntS8(s8)         // restore integer register s8

//
// WARNING: From this point on no TB Misses can be tolerated.
//

        li      k0,1 << PSR_EXL         // set EXL bit in temporary PSR
        mtc0    k0,psr                  // set new PSR value - 3 cycle hazzard
        lw      k0,KiPcr + PcSavedT7(zero) // get previous processor status
        lw      k1,KiPcr + PcSavedEpc(zero) // get continuation address
        nop                             //
        mtc0    k0,psr                  // set new PSR value - 3 cycle hazzard
        mtc0    k1,epc                  // set continuation PC
        nop                             //
        nop                             //
        eret                            //
        nop                             // errata
        nop                             //
        nop                             //
        eret                            //
        .set    at
        .set    reorder

        .end    KiInterruptStackSwitch

        SBTTL("Interrupt Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// KiInterruptHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//
// Routine Description:
//
//    Control reaches here when an exception is not handled by an interrupt
//    service routine or an unwind is initiated in an interrupt service
//    routine that would result in an unwind through the interrupt dispatcher.
//    This is considered to be a fatal system error and bug check is called.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//       N.B. This is not actually the frame pointer of the establisher of
//            this handler. It is actually the stack pointer of the caller
//            of the system service. Therefore, the establisher frame pointer
//            is not used and the address of the trap frame is determined by
//            examining the saved s8 register in the context record.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to  the dispatcher context
//       record.
//
// Return Value:
//
//    There is no return from this routine.
//
//--

        NESTED_ENTRY(KiInterruptHandler, HandlerFrameLength, zero)

        subu    sp,sp,HandlerFrameLength // allocate stack frame
        sw      ra,HdRa(sp)             // save return address

        PROLOGUE_END

        lw      t0,ErExceptionFlags(a0) // get exception flags
        li      a0,INTERRUPT_UNWIND_ATTEMPTED // assume unwind in progress
        and     t1,t0,EXCEPTION_UNWIND  // check if unwind in progress
        bne     zero,t1,10f             // if ne, unwind in progress
        li      a0,INTERRUPT_EXCEPTION_NOT_HANDLED // set bug check code
10:     jal     KeBugCheck              // call bug check routine
        j       KiExceptionExit         // dummy jump for filler

        .end    KiInterruptHandler

        SBTTL("Memory Management Exceptions")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiVirtualMemoryDispatch, TrapFrameLength, zero);

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a modify, read miss, or write miss exception
//    code is read from the cause register. When this routine is entered,
//    interrupts are disabled.
//
//    The function of this routine is to call memory management in an attempt
//    to resolve the problem. If memory management can resolve the problem,
//    then execution is continued. Otherwise an exception record is constructed
//    and an exception is raised.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiReadMissException)

        li      a0,0                    // set read indicator
        ld      a1,KiPcr + PcBadVaddr(zero) // get the bad virtual address

//
// N.B. The following code is a work around for a chip bug where the bad
//      virtual address is not correct on an instruction stream TB miss.
//
//      If the exception PC is equal to the bad virtual address, then the
//      bad virtual address is correct.
//
//      If the instruction at the exception PC is not in the TB or the
//      TB entry is invalid, then the bad virtual address is incorrect
//      and the instruction is repeated.
//
//      If the instruction at the exception PC is valid and is a load or
//      a store instruction, then the effective address is computed and
//      compared with the bad virtual address. If the comparison is equal,
//      then the bad virtual address is correct. Otherwise, the address is
//      incorrect and the instruction is repeated.
//
//      If the instruction at the exception PC is valid, is not a load or
//      store instruction, and is not the last instruction in the page,
//      the bad virtual address is correct.
//
//      If the instruction at the exception PC is valid, is not a load or
//      a store instruction, and is the last instruction in the page, then
//
//          If the exception PC + 4 is equal to the bad virtual address,
//          then the bad virtual address is correct.
//
//          If the instruction at the exception PC + 4 is not in the TB
//          or the TB entry is invalid, then the bad virtual address is
//          incorrect and the instruction is repeated.
//
//          If the instruction at the exception PC + 4 is valid and is a
//          load or a store instruction, then the effective address is
//          computed and compared with the bad virtual address. If the
//          comparison is equal, the the bad virtual address is correct.
//          Otherwise, the address is incorrect and the instruction is
//          repeated.
//

#if !defined(NT_UP)

        lw      t7,TrFir(s8)            // get exception PC

        .set    noreorder
        .set    noat
        dsra    t0,t7,30                // isolate high bits of exception PC
        beq     a1,t7,30f               // if eq, addresses match
        daddu   a2,t0,2                 // check for kseg0 or kseg1 address

//
// If the instruction at the exception PC is not in the TB or the TB entry
// invalid, then the bad virtual address is not valid and the instruction is
// repeated.
//

        beq     zero,a2,4f              // if eq, kseg0 or kseg1 address
        dsra    t1,t7,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        dmfc0   v0,entryhi              // get current VPN2 and PID
        dsll    t1,t1,ENTRYHI_VPN2      //
        and     v1,v0,PID_MASK << ENTRYHI_PID // isolate current PID
        or      t1,t1,v1                // merge PID with VPN2 of address
        dmtc0   t1,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t2,index                // read result of probe
        nop                             // 1 cycle hazzard
        bltzl   t2,20f                  // if ltz, entry not in TB
        dmtc0   v0,entryhi              // restore VPN2 and PID
        sll     t3,t7,31 - 12           // shift page bit into sign
        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t5,entrylo1             // read low part of TB entry
        mfc0    t4,entrylo0             //
        bltzl   t3,3f                   // if ltz, check second PTE
        and     t5,t5,1 << ENTRYLO_V    // check if second PTE valid
        and     t5,t4,1 << ENTRYLO_V    // check if first PTE valid
3:      mtc0    zero,pagemask           // restore page mask register
        beq     zero,t5,20f             // if eq, PTE not valid but in TB
        dmtc0   v0,entryhi              // restore VPN2 and PID
        nop                             // 2 cycle hazzard
        nop                             //

//
// If the instruction at the exception PC is a load or a store instruction,
// then compute its effective virtual address. Otherwise, check to determine
// if the instruction is at the end of the page.
//

4:      lw      t0,0(t7)                // get instruction value
        ld      t1,KiLoadInstructionSet // get load/store instruction set
        li      t2,1                    // compute opcode set member
        srl     t3,t0,32 - 6            // right justify opcode value
        dsll    t2,t2,t3                // shift opcode member into position
        and     t2,t2,t1                // check if load/store instruction
        bne     zero,t2,10f             // if ne, load/store instruction
        srl     t1,t0,21 - 3            // extract base register number

//
// If the instruction at the exception PC + 4 is not the first instruction in
// next page, then the bad virtual address is correct.
//

5:      daddu   t0,t7,4                 // compute next instruction address
        and     t1,t0,0xfff             // isolate offset in page
        bne     zero,t1,30f             // if ne, not in next page
        dsra    t1,t0,ENTRYHI_VPN2      // isolate VPN2 of virtual address

//
// If the exception PC + 4 is equal to the bad virtual address, then the
// bad virtual address is correct.
//

        beq     a1,t0,30f               // if eq, address match
        dsll    t1,t1,ENTRYHI_VPN2      //

//
// If the instruction at the exception PC + 4 is not in the TB or the TB entry
// invalid, then the bad virtual address is not valid and the instruction is
// repeated. Otherwise, the bad virtual address is correct.
//

        beq     zero,a2,8f              // if eq, kseg0 or kseg1 address
        or      t1,t1,v1                // merge PID with VPN2 of address
        dmtc0   t1,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t2,index                // read result of probe
        nop                             // 1 cycle hazzard
        bltzl   t2,20f                  // if ltz, entry not in TB
        dmtc0   v0,entryhi              // restore VPN2 and PID
        sll     t3,t0,31 - 12           // shift page bit into sign
        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t5,entrylo1             // read low part of TB entry
        mfc0    t4,entrylo0             //
        bltzl   t3,7f                   // if ltz, check second PTE
        and     t5,t5,1 << ENTRYLO_V    // check if second PTE valid
        and     t5,t4,1 << ENTRYLO_V    // check if first PTE valid
7:      mtc0    zero,pagemask           // restore page mask register
        beq     zero,t5,20f             // if eq, PTE is invalid
        dmtc0   v0,entryhi              // restore VPN2 and PID
        nop                             // 2 cycle hazzard
        nop                             //

//
// If the first instruction in the next page is a load/store, then compute
// its effective virtual address.  Otherwise, the bad virtual address is not
// valid and the instruction at the exception PC should be repeated.
//

8:      lw      t0,0(t0)                // get instruction value
        ld      t1,KiLoadInstructionSet // get load/store instruction set
        li      t2,1                    // compute opcode set member
        srl     t3,t0,32 - 6            // right justify opcode value
        dsll    t2,t2,t3                // shift opcode member into position
        and     t2,t2,t1                // check if load/store instruction
        beq     zero,t2,20f             // if eq, not load/store instruction
        srl     t1,t0,21 - 3            // extract base register number

//
// The faulting instruction was a load/store instruction.
//
// Compute the effect virtual address and check to detemrine if it is equal
// to the bad virtual address.
//

10:     and     t1,t1,0x1f << 3         // isolate base register number
        la      t2,12f                  // get base address of load table
        addu    t2,t2,t1                // compute address of register load
        j       t2                      // dispath to register load routine
        dsll    t1,t0,48                // shift displacement into position

12:     b       14f                     // zero
        move    t2,zero                 //

        b       14f                     // at
        ld      t2,TrXIntAt(s8)         //

        b       14f                     // v0
        ld      t2,TrXIntV0(s8)         //

        b       14f                     // v1
        ld      t2,TrXIntV1(s8)         //

        b       14f                     // a0
        ld      t2,TrXIntA0(s8)         //

        b       14f                     // a1
        ld      t2,TrXIntA1(s8)         //

        b       14f                     // a2
        ld      t2,TrXIntA2(s8)         //

        b       14f                     // a3
        ld      t2,TrXIntA3(s8)         //

        b       14f                     // t0
        ld      t2,TrXIntT0(s8)         //

        b       14f                     // t1
        ld      t2,TrXIntT1(s8)         //

        b       14f                     // t2
        ld      t2,TrXIntT2(s8)         //

        b       14f                     // t3
        ld      t2,TrXIntT3(s8)         //

        b       14f                     // t4
        ld      t2,TrXIntT4(s8)         //

        b       14f                     // t5
        ld      t2,TrXIntT5(s8)         //

        b       14f                     // t6
        ld      t2,TrXIntT6(s8)         //

        b       14f                     // t7
        ld      t2,TrXIntT7(s8)         //

        b       14f                     // s0
        move    t2,s0                   //

        b       14f                     // s1
        move    t2,s1                   //

        b       14f                     // s2
        move    t2,s2                   //

        b       14f                     // s3
        move    t2,s3                   //

        b       14f                     // s4
        move    t2,s4                   //

        b       14f                     // s5
        move    t2,s5                   //

        b       14f                     // s6
        move    t2,s6                   //

        b       14f                     // s7
        move    t2,s7                   //

        b       14f                     // t8
        ld      t2,TrXIntT8(s8)         //

        b       14f                     // t9
        ld      t2,TrXIntT9(s8)         //

        b       14f                     // k0
        move    t2,zero                 //

        b       14f                     // k1
        move    t2,zero                 //

        b       14f                     // gp
        ld      t2,TrXIntGp(s8)         //

        b       14f                     // sp
        ld      t2,TrXIntSp(s8)         //

        b       14f                     // s8
        ld      t2,TrXIntS8(s8)         //

        ld      t2,TrXIntRa(s8)         // ra

//
// If the effective virtual address matches the bad virtual address, then
// the bad virtual address is correct. Otherwise, repeat the instruction.
//

14:     dsra    t1,t1,48                // sign extend displacement value
        daddu   t3,t2,t1                // compute effective load address
        beq     a1,t3,30f               // if eq, bad virtual address is okay
        nop                             // fill

#if DBG

        lw      ra,KiMismatchCount      // increment address mismatch count
        nop                             // TB fills
        addu    ra,ra,1                 //
        sw      ra,KiMismatchCount      // store result

#endif


//
// N.B. PSR and EPC may have changed because of TB miss and need to be
//      reloaded.
//

20:     nop                             // 2 cycle hazzard
        nop                             //
        lw      t0,TrPsr(s8)            // get previous processor state
        lw      t1,TrFir(s8)            // get continuation address

#if DBG

        lw      ra,KiBadVaddrCount      // increment number of second level
        nop                             // TB fills
        addu    ra,ra,1                 //
        sw      ra,KiBadVaddrCount      // store result

#endif

        sw      t0,KiPcr + PcSavedT7(zero) // save processor status
        j       KiTrapExit              // join common code
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        .set    at
        .set    reorder

#else

        b       30f                     // join common code

#endif

        ALTERNATE_ENTRY(KiReadMissException9.x)

        li      a0,0                    // set read indicator
        ld      a1,KiPcr + PcBadVaddr(zero) // get the bad virtual address
        b       30f                     // join common code

        ALTERNATE_ENTRY(KiModifyException)

        ALTERNATE_ENTRY(KiWriteMissException)

        li      a0,1                    // set write indicator
        ld      a1,KiPcr + PcBadVaddr(zero) // get bad virtual address

//
// Common code for modify, read miss, and write miss exceptions.
//

30:     sw      t8,TrExceptionRecord + ErExceptionAddress(s8) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a2,t6                   // set previous mode
        .set    at
        .set    reorder

        sw      a0,TrExceptionRecord + ErExceptionInformation(s8) // save load/store indicator
        sw      a1,TrExceptionRecord + ErExceptionInformation + 4(s8) // save bad virtual address
        sw      a2,TrExceptionRecord + ErExceptionCode(s8) // save previous mode

//
// Check to determine if the bad virtual address is a 32 or 64-bit address.
//

        dsra    t0,a1,31                // isolate bits 63:31 of virtual address
        daddu   t1,t0,1                 //
        sltu    t1,t1,2                 // check if 32- or 64-bit address
        beq     zero,t1,38f             // if eq, 64-bit address

//
// Attempt to resolve 32-bit access fault.
//

        jal     MmAccessFault           // call memory management fault routine

//
// Check if working set watch is enabled.
//

        lbu     t0,PsWatchEnabled       // get working set watch enable flag
        lw      t1,TrExceptionRecord + ErExceptionCode(s8) // get previous mode
        move    a0,v0                   // set status of fault resolution
        bltz    v0,40f                  // if ltz, unsuccessful resolution
        beq     zero,t0,35f             // if eq, watch not enabled
        lw      a1,TrExceptionRecord + ErExceptionAddress(s8) // get exception address
        lw      a2,TrExceptionRecord + ErExceptionInformation + 4(s8) // set bad address
        jal     PsWatchWorkingSet       // record working set information

//
// Check if the debugger has any owed breakpoints.
//

35:     lbu     t0,KdpOweBreakpoint     // get owned breakpoint flag
        beq     zero,t0,37f             // if eq, no owed breakpoints
        jal     KdSetOwedBreakpoints    // insert breakpoints if necessary
37:     j       KiAlternateExit         //

//
// Attempt to resolve 64-bit access fault.
//

38:     sw      t0,TrExceptionRecord + ErExceptionInformation + 8(s8) // set upper address bits
        jal     MmAccessFault64         // call memory management fault routine
        bltz    v0,39f                  // if ltz, unsuccessful resolution
        j       KiAlternateExit         //

//
// The bad virtual address is not a valid 64-bit address. Set the number
// of parameters and attempt to dispatch the exception.
//

39:     addu    a0,s8,TrExceptionRecord // compute exception record address
        lw      a3,ErExceptionCode(a0)  // restore previous mode
        li      t0,3                    // set number of parameters
        b       50f                     //

//
// Check to determine if the fault occured in the interlocked pop entry slist
// code. There is a case where a fault may occur in this code when the right
// set of circumstances occurs. The fault can be ignored by simply skipping
// the faulting instruction.
//

40:     lw      t0,TrFir(s8)            // get address of faulting instruction
        la      t1,ExpInterlockedPopEntrySListFault // get address of pop code
        beq     t0,t1,70f               // if eq, skip faulting instruction

//
// The exception was not resolved. Fill in the remainder of the exception
// record and attempt to dispatch the exception.
//

        addu    a0,s8,TrExceptionRecord // compute exception record address
        lw      a3,ErExceptionCode(a0)  // restore previous mode
        li      t1,STATUS_IN_PAGE_ERROR | 0x10000000 // get special code
        beq     v0,t1,60f               // if eq, special bug check code
        li      t0,2                    // set number of parameters
        li      t1,STATUS_ACCESS_VIOLATION // get access violation code
        beq     v0,t1,50f               // if eq, access violation
        li      t1,STATUS_GUARD_PAGE_VIOLATION // get guard page violation code
        beq     v0,t1,50f               // if eq, guard page violation
        li      t1,STATUS_STACK_OVERFLOW // get stack overflow code
        beq     v0,t1,50f               // if eq, stack overflow
        li      t0,3                    // set number of parameters
        sw      v0,ErExceptionInformation + 8(a0) // save real status value
        li      v0,STATUS_IN_PAGE_ERROR // set in page error status
50:     sw      v0,ErExceptionCode(a0)  // save exception code
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      t0,ErNumberParameters(a0) //
        jal     KiExceptionDispatch     // join common code

//
// Generate a bug check - A page fault has occured at an IRQL that is greater
// than APC_LEVEL.
//

60:     li      a0,IRQL_NOT_LESS_OR_EQUAL // set bug check code
        lw      a1,TrExceptionRecord + ErExceptionInformation + 4(s8) // set bad virtual address
        lbu     a2,KiPcr + PcCurrentIrql(zero) // set current IRQL
        lw      a3,TrExceptionRecord + ErExceptionInformation(s8) // set load/store indicator
        lw      t1,TrFir(s8)            // set exception PC
        sw      t1,4 * 4(sp)            //
        jal     KeBugCheckEx            // call bug check routine
        j       KiExceptionExit         // dummy jump for filler

//
// The fault occured in the interlocked pop slist function and the faulting
// instruction should be skipped.
//

70:     la      t0,ExpInterlockedPopEntrySListResume // get resumption address
        sw      t0,TrFir(s8)            // set continuation address
        j       KiAlternateExit         //

        .end    KiVirtualMemoryDispatch

        SBTTL("System Service Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        EXCEPTION_HANDLER(KiSystemServiceHandler)

        NESTED_ENTRY(KiSystemServiceDispatch, TrapFrameLength, zero);

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp - TrapFrameLength(sp) // save stack pointer
        subu    sp,sp,TrapFrameLength   // allocate trap frame
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a system call exception code is read from
//    the cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to call the specified system service.
//
// N.B. The exception dispatcher jumps to the correct entry point depending
//      on whether the system service is a fast path event pair servive or
//      a normal service. The new PSR has been loaded before the respective
//      routines are entered.
//
// Arguments:
//
//    v0 - Supplies the system service code.
//    t0 - Supplies the address of the current thread object.
//    t9 - Supplies the previous PSR with the EXL and mode set.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiSystemServiceException)

        START_REGION(KiSystemServiceDispatchStart)

        ALTERNATE_ENTRY(KiSystemServiceNormal)

        srl     t9,t9,PSR_PMODE         // isolate previous processor mode
        lbu     t3,ThPreviousMode(t0)   // get old previous mode from thread object
        lw      t4,ThTrapFrame(t0)      // get current trap frame address
        and     t9,t9,0x1               // isolate previous mode
        sb      t9,ThPreviousMode(t0)   // set new previous mode in thread object
        sb      t3,TrPreviousMode(s8)   // save old previous mode of thread object
        sw      t4,TrTrapFrame(s8)      // save current trap frame address

#if DBG

        lw      t7,ThKernelApcDisable(t0) // get current APC disable count
        lbu     t8,ThApcStateIndex(t0)  // get current APC state index
        sw      t7,TrExceptionRecord(s8) // save APC disable count
        sb      t8,TrExceptionRecord + 4(s8) // save APC state index

#endif

//
// If the specified system service number is not within range, then
// attempt to convert the thread to a GUI thread and retry the service
// dispatch.
//
// N.B. The argument registers a0-a3, the system service number in v0,
//      and the thread address in t0 must be preserved while attempting
//      to convert the thread to a GUI thread.
//

        ALTERNATE_ENTRY(KiSystemServiceRepeat)

        sw      s8,ThTrapFrame(t0)      // save address of trap frame
        lw      t6,ThServiceTable(t0)   // get service descriptor table address
        srl     t1,v0,SERVICE_TABLE_SHIFT // isolate service descriptor offset
        and     t1,t1,SERVICE_TABLE_MASK //
        add     t6,t6,t1                // compute service descriptor address
        lw      t4,SdLimit(t6)          // get service number limit
        lw      t5,SdBase(t6)           // get service table address
        and     t7,v0,SERVICE_NUMBER_MASK // isolate service table offset
        sll     v1,t7,2                 // compute system service offset value
        sltu    t4,t7,t4                // check if invalid service number
        addu    v1,v1,t5                // compute address of service entry
        beq     zero,t4,50f             // if eq, invalid service number
        lw      v1,0(v1)                // get address of service routine

#if DBG

        lw      t6,SdCount(t6)          // get service count table address
        sll     t5,t7,2                 // compute system service offset value
        beq     zero,t6,12f             // if eq, table not defined
        addu    t6,t6,t5                // compute address of service entry
        lw      t7,0(t6)                // increment system service count
        addu    t7,t7,1                 //
        sw      t7,0(t6)                // store result
12:                                     //

#endif

//
// If the system service is a GUI service and the GDI user batch queue is
// not empty, then call the appropriate service to flush the user batch.
//

        xor     t2,t1,SERVICE_TABLE_TEST // check if GUI system service
        bne     zero,t2,15f             // if ne, not GUI system service
        lw      t3,KiPcr + PcTeb(zero)  // get current thread TEB address
        sw      v1,TrXIntV1(s8)         // save service routine address
        sw      a0,TrXIntA0(s8)         // save possible arguments 1 and 2
        lw      t4,TeGdiBatchCount(t3)  // get number of batched GDI calls
        sw      a1,TrXIntA1(s8)         //
        sw      a2,TrXIntA2(s8)         // save possible third argument
        lw      t5,KeGdiFlushUserBatch  // get address of flush routine
        beq     zero,t4,15f             // if eq, no batched calls
        sw      a3,TrXIntA3(s8)         // save possible fourth argument
        jal     t5                      // flush GDI user batch
        lw      v1,TrXIntV1(s8)         // restore service routine address
        lw      a0,TrXIntA0(s8)         // restore possible arguments
        lw      a1,TrXIntA1(s8)         //
        lw      a2,TrXIntA2(s8)         //
        lw      a3,TrXIntA3(s8)         //
15:     addu    a0,a0,zero              // make sure of sign extension
        addu    a1,a1,zero              // N.B. needed for 64-bit addressing
        and     t1,v1,1                 // check if any in-memory arguments
        beq     zero,t1,30f             // if eq, no in-memory arguments

//
// The following code captures arguments that were passed in memory on the
// callers stack. This is necessary to ensure that the caller does not modify
// the arguments after they have been probed and is also necessary in kernel
// mode because a trap frame has been allocated on the stack.
//
// If the previous mode is user, then the user stack is probed for readability.
//
// N.B. The maximum possible number of parameters are copied to avoid loop
//      and computational overhead.
//

        START_REGION(KiSystemServiceStartAddress)

        subu    sp,sp,TrapFrameArguments // allocate argument list space
        lw      t0,TrXIntSp(s8)         // get previous stack pointer
        beq     zero,t9,20f             // if eq, previous mode was kernel
        li      t1,MM_USER_PROBE_ADDRESS // get user probe address
        sltu    t2,t0,t1                // check if stack in user region
        bne     zero,t2,20f             // if ne, stack in user region
        move    t0,t1                   // set invalid user stack address
20:     ld      t1,16(t0)               // get twelve argument values from
        ld      t2,24(t0)               // callers stack
        ld      t3,32(t0)               //
        ld      t4,40(t0)               //
        ld      t5,48(t0)               //
        ld      t6,56(t0)               //
        sd      t1,16(sp)               // stores arguments on kernel stack
        sd      t2,24(sp)               //
        sd      t3,32(sp)               //
        sd      t4,40(sp)               //
        sd      t5,48(sp)               //
        sd      t6,56(sp)               //

        END_REGION(KiSystemServiceEndAddress)

        subu    v1,v1,1                 // clear low bit of service address

//
// Call system service.
//

30:     addu    a2,a2,zero              // make sure of sign extension
        addu    a3,a3,zero              // needed for 64-bit addressing
        jal     v1                      // call system service

//
// Restore old trap frame address from the current trap frame.
//

        ALTERNATE_ENTRY(KiSystemServiceExit)

        lw      a0,KiPcr + PcPrcb(zero) // get processor block address
        lw      t2,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      t3,TrTrapFrame(s8)      // get old trap frame address
        lw      t0,PbSystemCalls(a0)    // increment number of system calls
        addu    t0,t0,1                 //
        sw      t0,PbSystemCalls(a0)    //
        sw      t3,ThTrapFrame(t2)      // restore old trap frame address

//
// Restore state and exit system service.
//

        lw      t1,TrFsr(s8)            // get previous floating status
        li      t0,CU1_ENABLE           // set coprocessor 1 enable bits

        .set    noreorder
        .set    noat
        mtc0    t0,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t1,fsr                  // restore floating status
        lw      t0,TrPsr(s8)            // get previous processor status
        lw      t1,TrFir(s8)            // get continuation address
        lbu     t3,TrPreviousMode(s8)   // get old previous mode

#if DBG

        lw      a2,ThKernelApcDisable(t2) // get current APC disable count
        lbu     a3,ThApcStateIndex(t2)  // get current APC state index
        lw      t5,TrExceptionRecord(s8) // get previous APC disable count
        lbu     t6,TrExceptionRecord + 4(s8) // get previous APC state index
        xor     t7,t5,a2                // compare APC disable count
        xor     t8,t6,a3                // compare APC state index
        or      t9,t8,t7                // merge comparison value
        bne     zero,t9,60f             // if ne, invalid state or count
        nop                             // fill

#endif

        and     t4,t0,1 << PSR_PMODE    // check if previous mode was user
        beq     zero,t4,40f             // if eq, previous mode was kernel
        sb      t3,ThPreviousMode(t2)   // restore old previous mode

//
// If a user mode APC is pending, then request an APV interrupt.
//

        lbu     t3,ThApcState + AsUserApcPending(t2) // get user APC pending
        sb      zero,ThAlerted(t2)      // clear kernel mode alerted
        mfc0    t4,cause                // get exception cause register
        sll     t3,t3,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t4,t4,t3                // merge possilbe APC interrupt request
        mtc0    t4,cause                // set exception cause register

//
// Save the new processor status and continuation PC in the PCR so a TB
// is not possible, then restore the volatile register state.
//

40:     sw      t0,KiPcr + PcSavedT7(zero) // save processor status
        j       KiServiceExit           // join common code
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        .set    at
        .set    reorder

//
// The specified system service number is not within range. Attempt to
// convert the thread to a GUI thread if specified system service is
// not a base service and the thread has not already been converted to
// a GUI thread.
//
// N.B. The argument register a0-a3, the system service number in v0,
//      and the thread address in t0 must be preserved if an attempt
//      is made to convert the thread to a GUI thread.
//

50:     xor     t2,t1,SERVICE_TABLE_TEST // check if GUI system service
        sw      v0,TrXIntV0(s8)         // save system service number
        bne     zero,t2,55f             // if ne, not GUI system service
        sw      a0,TrXIntA0(s8)         // save argument register a0
        sw      a1,TrXIntA1(s8)         // save argument registers a1-a3
        sw      a2,TrXIntA2(s8)         //
        sw      a3,TrXIntA3(s8)         //
        jal     PsConvertToGuiThread    // attempt to convert to GUI thread
        move    v1,v0                   // save completion status
        move    s8,sp                   // reset trap frame address
        lw      v0,TrXIntV0(s8)         // restore system service number
        lw      a0,TrXIntA0(s8)         // restore argument registers a0-a3
        lw      a1,TrXIntA1(s8)         //
        lw      a2,TrXIntA2(s8)         //
        lw      a3,TrXIntA3(s8)         //
        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        beq     zero,v1,KiSystemServiceRepeat // if eq, successful conversion

//
// Return invalid system service status for invalid service code.
//

55:     li      v0,STATUS_INVALID_SYSTEM_SERVICE // set completion status
        b       KiSystemServiceExit      //

//
// An attempt is being made to exit a system service while kernel APCs are
// disabled, or while attached to another process and the previous mode is
// not kernel.
//
//    a2 - Supplies the APC disable count.
//    a3 - Supplies the APC state index.
//

#if DBG

60:     li      a0,APC_INDEX_MISMATCH   // set bug check code
        move    a1,t5                   // set previous APC disable
        sw      t6,4 * 4(sp)            // set previous state index
        jal     KeBugCheckEx            // call bug check routine
        j       KiExceptionExit         // dummy jump for filler

#endif

        START_REGION(KiSystemServiceDispatchEnd)

        .end    KiSystemServiceDispatch

        SBTTL("System Service Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// KiSystemServiceHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//    )
//
// Routine Description:
//
//    Control reaches here when a exception is raised in a system service
//    or the system service dispatcher, and for an unwind during a kernel
//    exception.
//
//    If an unwind is being performed and the system service dispatcher is
//    the target of the unwind, then an exception occured while attempting
//    to copy the user's in-memory argument list. Control is transfered to
//    the system service exit by return a continue execution disposition
//    value.
//
//    If an unwind is being performed and the previous mode is user, then
//    bug check is called to crash the system. It is not valid to unwind
//    out of a system service into user mode.
//
//    If an unwind is being performed, the previous mode is kernel, the
//    system service dispatcher is not the target of the unwind, and the
//    thread does not own any mutexes, then the previous mode field from
//    the trap frame is restored to the thread object. Otherwise, bug
//    check is called to crash the system. It is invalid to unwind out of
//    a system service while owning a mutex.
//
//    If an exception is being raised and the exception PC is within the
//    range of the system service dispatcher in-memory argument copy code,
//    then an unwind to the system service exit code is initiated.
//
//    If an exception is being raised and the exception PC is not within
//    the range of the system service dispatcher, and the previous mode is
//    not user, then a continue searh disposition value is returned. Otherwise,
//    a system service has failed to handle an exception and bug check is
//    called. It is invalid for a system service not to handle all exceptions
//    that can be raised in the service.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//       N.B. This is not actually the frame pointer of the establisher of
//            this handler. It is actually the stack pointer of the caller
//            of the system service. Therefore, the establisher frame pointer
//            is not used and the address of the trap frame is determined by
//            examining the saved s8 register in the context record.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to  the dispatcher context
//       record.
//
// Return Value:
//
//    If bug check is called, there is no return from this routine and the
//    system is crashed. If an exception occured while attempting to copy
//    the user in-memory argument list, then there is no return from this
//    routine, and unwind is called. Otherwise, ExceptionContinueSearch is
//    returned as the function value.
//
//--

        LEAF_ENTRY(KiSystemServiceHandler)

        subu    sp,sp,HandlerFrameLength // allocate stack frame
        sw      ra,HdRa(sp)             // save return address

        PROLOGUE_END

        lw      t0,ErExceptionFlags(a0) // get exception flags
        and     t1,t0,EXCEPTION_UNWIND  // check if unwind in progress
        bne     zero,t1,40f             // if ne, unwind in progress

//
// An exception is in progress.
//
// If the exception PC is within the in-memory argument copy code of the
// system service dispatcher, then call unwind to transfer control to the
// system service exit code. Otherwise, check if the previous mode is user
// or kernel mode.
//
//

        lw      t0,ErExceptionAddress(a0) // get address of exception
        la      t1,KiSystemServiceStartAddress // get start address of range
        sltu    t3,t0,t1                // check if before start of range
        la      t2,KiSystemServiceEndAddress // get end address of range
        bne     zero,t3,10f             // if ne, before start of range
        sltu    t3,t0,t2                // check if before end of range
        bne     zero,t3,30f             // if ne, before end of range

//
// If the previous mode was kernel mode, then a continue search disposition
// value is returned. Otherwise, the exception was raised in a system service
// and was not handled by that service. Call bug check to crash the system.
//

10:     lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        lbu     t1,ThPreviousMode(t0)   // get previous mode from thread object
        bne     zero,t1,20f             // if ne, previous mode was user

//
// Previous mode is kernel mode.
//

        li      v0,ExceptionContinueSearch // set disposition code
        addu    sp,sp,HandlerFrameLength // deallocate stack frame
        j       ra                      // return

//
// Previous mode is user mode. Call bug check to crash the system.
//

20:     li      a0,SYSTEM_SERVICE_EXCEPTION // set bug check code
        jal     KeBugCheck              // call bug check routine

//
// The exception was raised in the system service dispatcher. Unwind to the
// the system service exit code.
//

30:     lw      a3,ErExceptionCode(a0)  // set return value
        move    a2,zero                 // set exception record address
        move    a0,a1                   // set target frame address
        la      a1,KiSystemServiceExit  // set target PC address
        jal     RtlUnwind               // unwind to system service exit

//
// An unwind is in progress.
//
// If a target unwind is being performed, then continue execution is returned
// to transfer control to the system service exit code. Otherwise, restore the
// previous mode if the previous mode is not user and there are no mutexes owned
// by the current thread.
//

40:     and     t1,t0,EXCEPTION_TARGET_UNWIND // check if target unwind in progress
        bne     zero,t1,60f             // if ne, target unwind in progress

//
// An unwind is being performed through the system service dispatcher. If the
// previous mode is not kernel or the current thread owns one or more mutexes,
// then call bug check and crash the system. Otherwise, restore the previous
// mode in the current thread object.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      t1,CxXIntS8(a2)         // get address of trap frame
        lbu     t3,ThPreviousMode(t0)   // get previous mode from thread object
        lbu     t4,TrPreviousMode(t1)   // get previous mode from trap frame
        bne     zero,t3,50f             // if ne, previous mode was user

//
// Restore previous from trap frame to thread object and continue the unwind
// operation.
//

        sb      t4,ThPreviousMode(t0)   // restore previous mode from trap frame
        li      v0,ExceptionContinueSearch // set disposition value
        addu    sp,sp,HandlerFrameLength // deallocate stack frame
        j       ra                      // return

//
// An attempt is being made to unwind into user mode. Call bug check to crash
// the system.
//

50:     li      a0,SYSTEM_UNWIND_PREVIOUS_USER // set bug check code
        jal     KeBugCheck              // call bug check

//
// A target unwind is being performed. Return a continue execution disposition
// value.
//

60:     li      v0,ExceptionContinueSearch // set disposition value
        addu    sp,sp,HandlerFrameLength // deallocate stack frame
        j       ra                      // return

        .end    KiSystemServiceHandler

        SBTTL("Trap Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiTrapDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a trap exception code is read from the
//    cause register. When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to raise an array bounds exceeded
//    exception.
//
//    N.B. Integer register v1 is not usuable in the first instuction of the
//       routine.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiTrapException)

        addu    a0,s8,TrExceptionRecord // compute exception record address
        sw      t8,ErExceptionAddress(a0) // save address of exception

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        li      t1,STATUS_ARRAY_BOUNDS_EXCEEDED // set exception code
        sw      t1,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        sw      zero,ErNumberParameters(a0) // set number of parameters
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiTrapDispatch

        SBTTL("User Address Error Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--

        NESTED_ENTRY(KiUserAddressErrorDispatch, TrapFrameLength, zero)

        .set    noreorder
        .set    noat
        sd      sp,TrXIntSp(sp)         // save stack pointer
        sd      ra,TrXIntRa(sp)         // save return address
        sw      ra,TrFir(sp)            // save return address
        sd      s8,TrXIntS8(sp)         // save frame pointer
        sd      gp,TrXIntGp(sp)         // save general pointer
        sd      s0,TrXIntS0(sp)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(sp)         //
        sd      s2,TrXIntS2(sp)         //
        sd      s3,TrXIntS3(sp)         //
        sd      s4,TrXIntS4(sp)         //
        sd      s5,TrXIntS5(sp)         //
        sd      s6,TrXIntS6(sp)         //
        sd      s7,TrXIntS7(sp)         //
        move    s8,sp                   // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when a read or write user address error exception
//    is generated from the XTB miss handler. A user address error exception
//    occurs when an invalid 64-bit user address is generated. Interrupts are
//    disabled when this routine is entered.
//
//    The function of this routine is to raise an access violation exception.
//
// Arguments:
//
//    t6 - The previous mode.
//    t7 - The cause register with the BD bit set.
//    t8 - The address of the faulting instruction.
//    t9 - The new PSR with EXL and mode clear.
//    gp - Supplies a pointer to the system short data area.
//    s8 - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiUserAddressErrorException)

        ld      a1,KiPcr + PcBadVaddr(zero) // get the bad virtual address

//
// N.B. The following code is a work around for a chip bug where the bad
//      virtual address is not correct on an instruction stream TB miss.
//
//      If the exception PC is equal to the bad virtual address, then the
//      bad virtual address is correct.
//
//      If the instruction at the exception PC is not in the TB or the
//      TB entry is invalid, then the bad virtual address is incorrect
//      and the instruction is repeated.
//
//      Otherwise, the bad virtual address is correct.
//

#if !defined(NT_UP)

        move    t7,t8                   // get address of faulting instruction

        .set    noreorder
        .set    noat
        dsra    t0,t7,30                // isolate high bits of exception PC
        beq     a1,t7,30f               // if eq,  addresses match
        daddu   a2,t0,2                 // check for kseg0 or kseg1 address

//
// If the instruction at the exception PC is not in the TB or the TB entry
// invalid, then the bad virtual address is not valid and the instruction is
// repeated.
//

        beq     zero,a2,30f             // if eq, kseg0 or kseg1 address
        dsra    t1,t7,ENTRYHI_VPN2      // isolate VPN2 of virtual address
        dmfc0   v0,entryhi              // get current VPN2 and PID
        dsll    t1,t1,ENTRYHI_VPN2      //
        and     v1,v0,PID_MASK << ENTRYHI_PID // isolate current PID
        or      t1,t1,v1                // merge PID with VPN2 of address
        dmtc0   t1,entryhi              // set VPN2 and PID for probe
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        tlbp                            // probe for entry in TB
        nop                             // 2 cycle hazzard
        nop                             //
        mfc0    t2,index                // read result of probe
        nop                             // 1 cycle hazzard
        bltzl   t2,20f                  // if ltz, entry not in TB
        dmtc0   v0,entryhi              // restore VPN2 and PID
        sll     t3,t7,31 - 12           // shift page bit into sign
        tlbr                            // read entry from TB
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        mfc0    t5,entrylo1             // read low part of TB entry
        mfc0    t4,entrylo0             //
        bltzl   t3,10f                  // if ltz, check second PTE
        and     t5,t5,1 << ENTRYLO_V    // check if second PTE valid
        and     t5,t4,1 << ENTRYLO_V    // check if first PTE valid
10:     mtc0    zero,pagemask           // restore page mask register
        dmtc0   v0,entryhi              // restore VPN2 and PID
        bne     zero,t5,30f             // if ne, PTE valid

//
// N.B. PSR and EPC may have changed because of TB miss and need to be
//      reloaded.
//

20:     nop                             // 2 cycle hazzard
        nop                             //
        lw      t0,TrPsr(s8)            // get previous processor state
        lw      t1,TrFir(s8)            // get continuation address
        sw      t0,KiPcr + PcSavedT7(zero) // save processor status
        j       KiTrapExit              // join common code
        sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        .set    at
        .set    reorder

#endif

30:     addu    a0,s8,TrExceptionRecord // compute exception record address

        .set    noreorder
        .set    noat
        mtc0    t9,psr                  // set new PSR
        move    a3,t6                   // set previous mode
        .set    at
        .set    reorder

        sw      zero,ErExceptionInformation(a0) // save load/store indicator
        sw      a1,ErExceptionInformation + 4(a0) // save bad virtual address
        dsrl    t0,a1,32                // isolate upper bits of 64-bit address
        sw      t0,ErExceptionInformation + 8(a0) // set upper address bits
        sw      t8,ErExceptionAddress(a0) // set exception address

//
// If the address is a reference to the last 64k of user address space, then
// treat the error as an address error. Otherwise, treat the error as an
// access violation.
//

        li      t3,STATUS_ACCESS_VIOLATION // set exception code
        li      t4,0x7fff0000           // get address mask value
        dsrl    t0,a1,32                // isolate high bits of bad address
        bne     zero,t0,40f             // if ne, invalid user address
        and     t5,t4,a1                // isolate low bits of bad address
        bne     t4,t5,40f               // if ne, invalid user address
        li      t3,STATUS_DATATYPE_MISALIGNMENT // set exception code
40:     sw      t3,ErExceptionCode(a0)  //
        sw      zero,ErExceptionFlags(a0) // set exception flags
        sw      zero,ErExceptionRecord(a0) // set associated record
        li      t0,3                    // set number of exception parameters
        sw      t0,ErNumberParameters(a0) //
        jal     KiExceptionDispatch     // join common code
        j       KiExceptionExit         // dummy jump for filler

        .end    KiUserAddressErrorDispatch

        SBTTL("Exception Dispatch")
//++
//
// Routine Desription:
//
//    Control is transfered to this routine to call the exception
//    dispatcher to resolve an exception.
//
// Arguments:
//
//    a0 - Supplies a pointer to an exception record.
//
//    a3 - Supplies the previous processor mode.
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    There is no return from this routine.
//
//--

        NESTED_ENTRY(KiExceptionDispatch, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate exception frame
        sw      ra,ExIntRa(sp)          // save return address
        sdc1    f20,ExFltF20(sp)        // save floating registers f20 - f31
        sdc1    f22,ExFltF22(sp)        //
        sdc1    f24,ExFltF24(sp)        //
        sdc1    f26,ExFltF26(sp)        //
        sdc1    f28,ExFltF28(sp)        //
        sdc1    f30,ExFltF30(sp)        //

        PROLOGUE_END

        move    a1,sp                   // set exception frame address
        move    a2,s8                   // set trap frame address
        li      t0,TRUE                 // set first chance TRUE
        sw      t0,ExArgs + (4 * 4)(sp) //
        jal     KiDispatchException     // call exception dispatcher

        SBTTL("Exception Exit")
//++
//
// Routine Desription:
//
//    Control is transfered to this routine to exit from an exception.
//
//    N.B. This transfer of control occurs from:
//
//       1. a fall through from the above code.
//       2. an exit from the continue system service.
//       3. an exit from the raise exception system service.
//       4. an exit into user mode from thread startup.
//
//    N.B. The alternate exit point is used by memory management which does
//       generate an exception frame.
//
// Arguments:
//
//    s8 - Supplies a pointer to a trap frame.
//    sp - Supplies a pointer to an exception frame.
//
// Return Value:
//
//    There is no return from this routine.
//
//--

        ALTERNATE_ENTRY(KiExceptionExit)

        ldc1    f20,ExFltF20(sp)        // restore floating registers f20 - f31
        ldc1    f22,ExFltF22(sp)        //
        ldc1    f24,ExFltF24(sp)        //
        ldc1    f26,ExFltF26(sp)        //
        ldc1    f28,ExFltF28(sp)        //
        ldc1    f30,ExFltF30(sp)        //

        ALTERNATE_ENTRY(KiAlternateExit)

        lw      t1,TrFsr(s8)            // get previous floating status
        li      t0,CU1_ENABLE           // set coprocessor 1 enable bits

        .set    noreorder
        .set    noat
        mtc0    t0,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t1,fsr                  // restore floating status
        lw      t0,TrPsr(s8)            // get previous processor status
        lw      t1,TrFir(s8)            // get continuation address
        lw      t2,KiPcr + PcCurrentThread(zero) // get current thread address
        and     t3,t0,1 << PSR_PMODE    // check if previous mode was user
        beq     zero,t3,10f             // if eq, previous mode was kernel
        sw      t0,KiPcr + PcSavedT7(zero) // save processor status

//
// If a user mode APC is pending, then request an APV interrupt.
//

        lbu     t3,ThApcState + AsUserApcPending(t2) // get user APC pending
        sb      zero,ThAlerted(t2)      // clear kernel mode alerted
        mfc0    t4,cause                // get exception cause register
        sll     t3,t3,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t4,t4,t3                // merge possible APC interrupt request
        mtc0    t4,cause                // set exception cause register

//
// Save the new processor status and continuation PC in the PCR so a TB
// is not possible, then restore the volatile register state.
//

10:     sw      t1,KiPcr + PcSavedEpc(zero) // save continuation address
        ldc1    f0,TrFltF0(s8)          // restore floating register f0
        ldc1    f2,TrFltF2(s8)          // restore floating registers f2 - f19
        ldc1    f4,TrFltF4(s8)          //
        ldc1    f6,TrFltF6(s8)          //
        ldc1    f8,TrFltF8(s8)          //
        ldc1    f10,TrFltF10(s8)        //
        ldc1    f12,TrFltF12(s8)        //
        ldc1    f14,TrFltF14(s8)        //
        ldc1    f16,TrFltF16(s8)        //
        j       KiTrapExit              //
        ldc1    f18,TrFltF18(s8)        //
        .set    at
        .set    reorder

        .end    KiExceptionDispatch

        SBTTL("Disable Interrupts")
//++
//
// BOOLEAN
// KiDisableInterrupts (
//    VOID
//    )
//
// Routine Description:
//
//    This function disables interrupts and returns whether interrupts
//    were previously enabled.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    A boolean value that determines whether interrupts were previously
//    enabled (TRUE) or disabled(FALSE).
//
//--

        LEAF_ENTRY(KiDisableInterrupts)

        .set    noreorder
        .set    noat
        mfc0    t0,psr                  // get current processor status
        li      t1,~(1 << PSR_IE)       // set interrupt enable mask
        and     t2,t1,t0                // clear interrupt enable
        mtc0    t2,psr                  // disable interrupts
        and     v0,t0,1 << PSR_IE       // iosolate current interrupt enable
        srl     v0,v0,PSR_IE            //
        .set    at
        .set    reorder

        j       ra                      // return

        .end    KiDisableInterrupts

        SBTTL("Restore Interrupts")
//++
//
// VOID
// KiRestoreInterrupts (
//    IN BOOLEAN Enable
//    )
//
// Routine Description:
//
//    This function restores the interrupt enable that was returned by
//    the disable interrupts function.
//
// Arguments:
//
//    Enable (a0) - Supplies the interrupt enable value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRestoreInterrupts)

        .set    noreorder
        .set    noat
        mfc0    t0,psr                  // get current processor status
        and     a0,a0,0xff              // isolate interrupt enable
        sll     t1,a0,PSR_IE            // shift interrupt enable into position
        or      t1,t1,t0                // merge interrupt enable with PSR
        mtc0    t1,psr                  // restore previous interrupt enable
        nop                             //
        .set    at
        .set    reorder

        j       ra                      // return

        .end    KiRestoreInterrupts

        SBTTL("Passive Release")
//++
//
// VOID
// KiPassiveRelease (
//    VOID
//    )
//
// Routine Description:
//
//    This function is called when an interrupt has been passively released.
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

        LEAF_ENTRY(KiPassiveRelease)

        j       ra                      // return

        .end    KiPassiveRelease
