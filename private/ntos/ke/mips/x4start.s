//      TITLE("System Initialization")
//++
//
// Copyright (c) 1991  Microsoft Corporation
//
// Module Name:
//
//    x4start.s
//
// Abstract:
//
//    This module implements the code necessary to initially startup the
//    NT system.
//
// Author:
//
//    David N. Cutler (davec) 5-Apr-1991
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

        .extern KdDebuggerEnabled  1
        .extern KeNumberProcessIds 4
        .extern KeNumberProcessors 1
        .extern KeNumberTbEntries  4
        .extern KiBarrierWait      4
        .extern KiContextSwapLock  4
        .extern KiDispatcherLock   4
        .extern KiSynchIrql        4

        SBTTL("System Initialization")
//++
//
// Routine Description:
//
//    This routine is called when the NT system begins execution.
//    Its function is to initialize system hardware state, call the
//    kernel initialization routine, and then fall into code that
//    represents the idle thread for all processors.
//
// Arguments:
//
//    a0 - Supplies a pointer to the loader parameter block.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
SsArgA0:.space  4                       // process address argument (a0)
SsArgA1:.space  4                       // thread address argument (a1)
SsArgA2:.space  4                       // idle stack argument (a2)
SsArgA3:.space  4                       // processor block address argument (a3)
SsPrNum:.space  4                       // processor number argument
SsLdPrm:.space  4                       // loader parameter block address
SsPte:  .space  2 * 4                   // Pte values
        .space  4                       // fill
SsRa:   .space  4                       // saved return address

SsFrameLength:                          // length of stack frame

        NESTED_ENTRY_S(KiSystemBegin, SsFrameLength, zero, INIT)

        subu    sp,sp,SsFrameLength     // allocate stack frame
        sw      ra,SsRa(sp)             // save return address

        PROLOGUE_END

        ALTERNATE_ENTRY(KiInitializeSystem)

        lw      sp,LpbKernelStack(a0)   // get address of idle thread stack
        subu    sp,sp,SsFrameLength     // allocate stack frame
        lw      gp,LpbGpBase(a0)        // get global pointer base address
        sw      zero,SsRa(sp)           // zero return address

        DISABLE_INTERRUPTS(t0)          // disable interrupts

//
// Get page frame numbers for the PCR and PDR pages that were allocated by
// the OS loader.
//

        lw      s0,LpbPdrPage(a0)       // set PDR page number
        lw      s1,LpbPcrPage(a0)       // set PCR page number
        move    s2,a0                   // save loader parameter block address
        lw      s3,LpbPrcb(s2)          // get processor block address
        lbu     s3,PbNumber(s3)         // get processor number
        lw      s6,LpbPcrPage2(a0)      // set second PCR page

//
// Initialize the configuration, context, page mask, watch, and wired
// registers.
//
// N.B. The base virtual address of the page table pages is left shift by
//      one because of the way VPN2 in inserted into the context register
//      when a TB miss occurs. The TB miss routine right arithmetic shifts
//      the address by one to obtain the real virtual address. Note that it
//      is assumed that bits <31:30> of PTE_BASE are set.
//

        li      t0,PTE_BASE << 1        // set base virtual page table address
        li      t1,FIXED_ENTRIES        // set number of fixed TB entries
        li      t2,0xf000               // set frame mask register value

        .set    noreorder
        .set    noat
        mfc0    s7,config               // get processor configuration
        mfc0    s8,prid                 // get processor id
        mtc0    t0,context              // initialize the context register
        mtc0    zero,pagemask           // initialize the page mask register
        mtc0    zero,taglo              // initialize the tag low register
        mtc0    zero,watchlo            // initialize the watch address register
        mtc0    zero,watchhi            //
        mtc0    t1,wired                // initialize the wired register
        and     s4,s7,0x7               // isolate KSEG0 cache policy
        and     t3,s8,0xff00            // isolate processor id
        xor     t3,t3,0x900             // check if r10000 processor
        bne     zero,t3,5f              // if ne, not r10000 processor
        sll     s5,s4,ENTRYLO_C         // shift cache policy into position
        mtc0    t2,framemask            // set frame mask register
        .set    at
        .set    reorder

//
// Clear the translation buffer.
//

5:      bne     zero,s3,20f             // if ne, not processor zero
        li      t0,48                   // set number of TB entries for r4x00
        and     t1,s8,0xff00            // isolate processor id
        xor     t1,t1,0x900             // check if r10000 processor
        bne     zero,t1,10f             // if ne, not r10000 processor
        li      t0,64                   // set number of TB entries for r10000
10:     sw      t0,KeNumberTbEntries    // store number of TB entries
        li      t0,256                  // set number of process id's
        sw      t0,KeNumberProcessIds   //
20:     jal     KiFlushFixedTb          // flush fixed TB entries
        jal     KiFlushRandomTb         // flush random TB entries

//
// Initialize fixed entries that map the PCR into system and user space.
//

        sll     t0,s6,ENTRYLO_PFN       // shift PFN into position
        or      t0,t0,1 << ENTRYLO_G    // Set G, V, D, and the cache policy
        or      t0,t0,1 << ENTRYLO_V    //
        or      t0,t0,1 << ENTRYLO_D    //
        or      t0,t0,s5                //
        sll     t1,s1,ENTRYLO_PFN       // shift PFN into position
        or      t1,t1,1 << ENTRYLO_G    // Set G, V, D, and the cache policy
        or      t1,t1,1 << ENTRYLO_V    //
        or      t1,t1,1 << ENTRYLO_D    //
        or      t1,t1,s5                //
        sw      t0,SsPte(sp)            // set first PTE value
        sw      t1,SsPte + 4(sp)        // set second PTE value
        addu    a0,sp,SsPte             // compute address of PTE values
        li      a1,KiPcr & ~(1 << PAGE_SHIFT) // set virtual address/2 of PCR
        li      a2,PCR_ENTRY            // set index of system PCR entry
        jal     KeFillFixedEntryTb      // fill fixed TB entry

        sll     t0,s6,ENTRYLO_PFN       // shift PFN into position
        or      t0,t0,1 << ENTRYLO_G    // Set G, V, D, and the cache policy
        or      t0,t0,1 << ENTRYLO_V    //
        or      t0,t0,s5                //
        sll     t1,s1,ENTRYLO_PFN       // shift PFN into position
        or      t1,t1,1 << ENTRYLO_G    // set G, V, and cache policy
        or      t1,t1,1 << ENTRYLO_V    //
        or      t1,t1,s5                //
        sw      t0,SsPte(sp)            // set first PTE value
        sw      t1,SsPte + 4(sp)        // set second PTE value
        addu    a0,sp,SsPte             // compute address of PTE values
        li      a1,UsPcr & ~(1 << PAGE_SHIFT) // set virtual address/2 of PCR
        li      a2,PCR_ENTRY + 1        // set index of user PCR entry
        jal     KeFillFixedEntryTb      // fill fixed TB entry

//
// Set the cache policy for cached memory.
//

        li      t1,KiPcr                // get PCR address
        sw      s4,PcCachePolicy(t1)    // set cache policy for cached memory
        sw      s5,PcAlignedCachePolicy(t1) //

//
// Set the first level data and instruction cache fill size and size.
//

        lw      t2,LpbFirstLevelDcacheSize(s2) //
        sw      t2,PcFirstLevelDcacheSize(t1) //
        lw      t2,LpbFirstLevelDcacheFillSize(s2) //
        sw      t2,PcFirstLevelDcacheFillSize(t1) //
        lw      t2,LpbFirstLevelIcacheSize(s2) //
        sw      t2,PcFirstLevelIcacheSize(t1) //
        lw      t2,LpbFirstLevelIcacheFillSize(s2) //
        sw      t2,PcFirstLevelIcacheFillSize(t1) //

//
// Set the second level data and instruction cache fill size and size.
//

        lw      t2,LpbSecondLevelDcacheSize(s2) //
        sw      t2,PcSecondLevelDcacheSize(t1) //
        lw      t2,LpbSecondLevelDcacheFillSize(s2) //
        sw      t2,PcSecondLevelDcacheFillSize(t1) //
        lw      t2,LpbSecondLevelIcacheSize(s2) //
        sw      t2,PcSecondLevelIcacheSize(t1) //
        lw      t2,LpbSecondLevelIcacheFillSize(s2) //
        sw      t2,PcSecondLevelIcacheFillSize(t1) //

//
// Set the data cache fill size and alignment values.
//

        lw      t2,PcSecondLevelDcacheSize(t1) // get second level dcache size
        lw      t3,PcSecondLevelDcacheFillSize(t1) // get second level fill size
        bne     zero,t2,30f             // if ne, second level cache present
        lw      t3,PcFirstLevelDcacheFillSize(t1) // get first level fill size
30:     subu    t4,t3,1                 // compute dcache alignment value
        sw      t3,PcDcacheFillSize(t1) // set dcache fill size
        sw      t4,PcDcacheAlignment(t1) // set dcache alignment value

//
// Set the instruction cache fill size and alignment values.
//

        lw      t2,PcSecondLevelIcacheSize(t1) // get second level icache size
        lw      t3,PcSecondLevelIcacheFillSize(t1) // get second level fill size
        bne     zero,t2,40f             // if ne, second level cache present
        lw      t3,PcFirstLevelIcacheFillSize(t1) // get first level fill size
40:     subu    t4,t3,1                 // compute icache alignment value
        sw      t3,PcIcacheFillSize(t1) // set icache fill size
        sw      t4,PcIcacheAlignment(t1) // set icache alignment value

//
// Sweep the data and instruction caches.
//

        lw      t0,__imp_HalSweepIcache // sweep the instruction cache
        jal     t0                      //
        lw      t0,__imp_HalSweepDcache // sweep the data cache
        jal     t0                      //

//
// Initialize the fixed entries that map the PDR pages.
//

        sll     t0,s0,ENTRYLO_PFN       // shift PFN into position
        or      t0,t0,1 << ENTRYLO_V    // set V, D, and cache policy
        or      t0,t0,1 << ENTRYLO_D    //
        or      t0,t0,s5                //
        addu    t1,t0,1 << ENTRYLO_PFN  // compute PTE for second PDR page
        sw      t0,SsPte(sp)            // set first PTE value
        sw      t1,SsPte + 4(sp)        // set second PTE value
        addu    a0,sp,SsPte             // compute address of PTE values
        li      a1,PDE_BASE             // set system virtual address/2 of PDR
        li      a2,PDR_ENTRY            // set index of system PCR entry
        jal     KeFillFixedEntryTb      // fill fixed TB entry
        li      t2,PDE_BASE             // set virtual address of PDR
        lw      t0,SsPte(sp)            // get first PTE value
        lw      t1,SsPte + 4(sp)        // get second PTE value
        sw      t0,((PDE_BASE >> (PDI_SHIFT - 2)) & 0xffc)(t2) // set recursive PDE
        sw      t1,((PDE_BASE >> (PDI_SHIFT - 2)) & 0xffc) + 4(t2) // set hyper PDE

//
// Initialize the Processor Control Registers (PCR).
//

        li      t1,KiPcr                // get PCR address

//
// Initialize the minor and major version numbers.
//

        li      t2,PCR_MINOR_VERSION    // set minor version number
        sh      t2,PcMinorVersion(t1)   //
        li      t2,PCR_MAJOR_VERSION    // set major version number
        sh      t2,PcMajorVersion(t1)   //

//
// Set address of processor block.
//

        lw      t2,LpbPrcb(s2)          // set processor block address
        sw      t2,PcPrcb(t1)           //

//
// Initialize the routine addresses in the exception dispatch table.
//

        la      t2,KiInvalidException   // set address of invalid exception
        li      t3,XCODE_VECTOR_LENGTH  // set length of dispatch vector
        la      t4,PcXcodeDispatch(t1)  // compute address of dispatch vector
50:     sw      t2,0(t4)                // fill dispatch vector
        subu    t3,t3,1                 // decrement number of entries
        addu    t4,t4,4                 // advance to next vector entry
        bgtz    t3,50b                  // if gtz, more to fill

        la      t2,KiInterruptException // Initialize exception dispatch table
        sw      t2,PcXcodeDispatch + XCODE_INTERRUPT(t1) //
        la      t2,KiModifyException    //
        sw      t2,PcXcodeDispatch + XCODE_MODIFY(t1) //
        la      t2,KiReadMissException  // set read miss address for r4x00
        and     t3,s8,0xff00            // isolate processor id
        xor     t3,t3,0x900             // check if r10000 processor
        bne     zero,t3,55f             // if ne, not r10000 processor
        la      t2,KiReadMissException9.x // set read miss address for r10000
55:     sw      t2,PcXcodeDispatch + XCODE_READ_MISS(t1) //
        la      t2,KiWriteMissException //
        sw      t2,PcXcodeDispatch + XCODE_WRITE_MISS(t1) //
        la      t2,KiReadAddressErrorException //
        sw      t2,PcXcodeDispatch + XCODE_READ_ADDRESS_ERROR(t1) //
        la      t2,KiWriteAddressErrorException //
        sw      t2,PcXcodeDispatch + XCODE_WRITE_ADDRESS_ERROR(t1) //
        la      t2,KiInstructionBusErrorException //
        sw      t2,PcXcodeDispatch + XCODE_INSTRUCTION_BUS_ERROR(t1) //
        la      t2,KiDataBusErrorException //
        sw      t2,PcXcodeDispatch + XCODE_DATA_BUS_ERROR(t1) //
        la      t2,KiSystemServiceException //
        sw      t2,PcXcodeDispatch + XCODE_SYSTEM_CALL(t1) //
        la      t2,KiBreakpointException //
        sw      t2,PcXcodeDispatch + XCODE_BREAKPOINT(t1) //
        la      t2,KiIllegalInstructionException //
        sw      t2,PcXcodeDispatch + XCODE_ILLEGAL_INSTRUCTION(t1) //
        la      t2,KiCoprocessorUnusableException //
        sw      t2,PcXcodeDispatch + XCODE_COPROCESSOR_UNUSABLE(t1) //
        la      t2,KiIntegerOverflowException //
        sw      t2,PcXcodeDispatch + XCODE_INTEGER_OVERFLOW(t1) //
        la      t2,KiTrapException      //
        sw      t2,PcXcodeDispatch + XCODE_TRAP(t1) //
        la      t2,KiInstructionCoherencyException //
        sw      t2,PcXcodeDispatch + XCODE_VIRTUAL_INSTRUCTION(t1) //
        la      t2,KiFloatingException //
        sw      t2,PcXcodeDispatch + XCODE_FLOATING_EXCEPTION(t1) //
        la      t2,KiUserAddressErrorException //
        sw      t2,PcXcodeDispatch + XCODE_INVALID_USER_ADDRESS(t1)
        la      t2,KiPanicException     //
        sw      t2,PcXcodeDispatch + XCODE_PANIC(t1) //
        la      t2,KiDataCoherencyException //
        sw      t2,PcXcodeDispatch + XCODE_VIRTUAL_DATA(t1) //

//
// Initialize the addresses of various data structures that are referenced
// from the exception and interrupt handling code.
//
// N.B. The panic stack is a separate stack that is used when the current
//      kernel stack overlfows.
//
// N.B. The interrupt stack is a separate stack and is used to process all
//      interrupts that run at IRQL 3 and above.
//

        lw      t2,LpbKernelStack(s2)   // set initial stack address
        sw      t2,PcInitialStack(t1)   //
        lw      t2,LpbPanicStack(s2)    // set panic stack address
        sw      t2,PcPanicStack(t1)     //
        lw      t2,LpbInterruptStack(s2) // set interrupt stack address
        sw      t2,PcInterruptStack(t1) //
        sw      gp,PcSystemGp(t1)       // set system global pointer address
        lw      t2,LpbThread(s2)        // set current thread address
        sw      t2,PcCurrentThread(t1)  //

//
// Set current IRQL to highest value.
//

        li      t2,HIGH_LEVEL           // set current IRQL
        sb      t2,PcCurrentIrql(t1)    //

//
// Set processor id and configuration.
//

        sw      s7,PcSystemReserved(t1) // save processor configuration
        sw      s8,PcProcessorId(t1)    // save processor id

//
// Clear floating status and zero the count and compare registers.
//

        .set    noreorder
        .set    noat
        ctc1    zero,fsr                // clear floating status
        mtc0    zero,count              // initialize the count register
        mtc0    zero,compare            // initialize the compare register
        .set    at
        .set    reorder

//
// Set system dispatch address limits used by get and set context.
//

        la      t2,KiSystemServiceDispatchStart // set starting address of range
        sw      t2,PcSystemServiceDispatchStart(t1) //
        la      t2,KiSystemServiceDispatchEnd // set ending address of range
        sw      t2,PcSystemServiceDispatchEnd(t1) //

//
// Copy the TB miss, XTB miss, cache parity, and general exception handlers to
// low memory.
//

        bne     zero,s3,100f            // if ne, not processor zero

//
// Copy TB Miss Handler.
//

        la      t2,KiTbMissStartAddress2.x // get user TB miss start address
        la      t3,KiTbMissEndAddress3.x // get user TB miss end address
        and     a0,s8,0xfff0            // isolate id and major chip version
        xor     a0,a0,0x420             // test if id 4 and version 2.0 chip
        beq     zero,a0,60f             // if eq, version 2.0 chip
        la      t2,KiTbMissStartAddress3.x // get user TB miss start address
        and     a0,s8,0xff00            // isolate processor id
        xor     a0,a0,0x900             // check if r10000 processor
        bne     zero,a0,60f             // if ne, not r10000 processor
        la      t2,KiTbMissStartAddress9.x // get user TB miss start address
        la      t3,KiTbMissEndAddress9.x // get user TB miss end address
60:     li      t4,KSEG0_BASE           // get copy address
70:     lw      t5,0(t2)                // copy code to low memory
        sw      t5,0(t4)                //
        addu    t2,t2,4                 // advance copy pointers
        addu    t4,t4,4                 //
        bne     t2,t3,70b               // if ne, more to copy

//
// Copy XTB Miss Handler.
//

        la      t2,KiXTbMissStartAddress2.x // get user TB miss start address
        la      t3,KiXTbMissEndAddress3.x // get user TB miss end address
        and     a0,s8,0xfff0            // isolate id and major chip version
        xor     a0,a0,0x420             // test if id 4 and version 2.0 chip
        beq     zero,a0,73f             // if eq, version 2.0 chip
        la      t2,KiXTbMissStartAddress3.x // get user TB miss start address
        and     a0,s8,0xff00            // isolate processor id
        xor     a0,a0,0x900             // check if r10000 processor
        bne     zero,a0,73f             // if ne, not r10000 processor
        la      t2,KiXTbMissStartAddress9.x // get user TB miss start address
        la      t3,KiXTbMissEndAddress9.x // get user TB miss end address
73:     li      t4,KSEG0_BASE + 0x80    // get copy address
77:     lw      t5,0(t2)                // copy code to low memory
        sw      t5,0(t4)                //
        addu    t2,t2,4                 // advance copy pointers
        addu    t4,t4,4                 //
        bne     t2,t3,77b               // if ne, more to copy

//
// Copy Cache Error Handler.
//

        la      t2,KiCacheErrorStartAddress // get cache error start address
        la      t3,KiCacheErrorEndAddress // get cache error end address
        li      t4,KSEG1_BASE + 0x100   // get copy address
80:     lw      t5,0(t2)                // copy code to low memory
        sw      t5,0(t4)                //
        addu    t2,t2,4                 // advance copy pointers
        addu    t4,t4,4                 //
        bne     t2,t3,80b               // if ne, more to copy

//
// Copy General Exception Handler.
//

        la      t2,KiGeneralExceptionStartAddress // get general exception start address
        la      t3,KiGeneralExceptionEndAddress // get general exception end address
        li      t4,KSEG0_BASE + 0x180   // get copy address
90:     lw      t5,0(t2)                // copy code to low memory
        sw      t5,0(t4)                //
        addu    t2,t2,4                 // advance copy pointers
        addu    t4,t4,4                 //
        bne     t2,t3,90b               // if ne, more to copy

//
// Set the default cache error routine address.
//

        la      t0,SOFT_RESET_VECTOR    // get soft reset vector address
        la      t1,CACHE_ERROR_VECTOR   // get cache error vector address
        sw      t0,0(t1)                // set default cache error routine

//
// Sweep the data and instruction caches.
//

100:    lw      t0,__imp_HalSweepIcache // sweep the instruction cache
        jal     t0                      //
        lw      t0,__imp_HalSweepDcache // sweep the data cache
        jal     t0                      //

// ****** temp ******
//
// Setup watch registers to catch write to location 0.
//
// ****** temp ******

//        .set    noreorder
//        .set    noat
//        li      t0,1                    // set to watch writes to location 0
//        mtc0    t0,watchlo              //
//        mtc0    zero,watchhi            //
//        .set    at
//        .set    reorder

//
// Setup arguments and call kernel initialization routine.
//

        lw      s0,LpbProcess(s2)       // get idle process address
        lw      s1,LpbThread(s2)        // get idle thread address
        move    a0,s0                   // set idle process address
        move    a1,s1                   // set idle thread address
        lw      a2,LpbKernelStack(s2)   // set idle thread stack address
        lw      a3,LpbPrcb(s2)          // get processor block address
        sw      s3,SsPrNum(sp)          // set processor number
        sw      s2,SsLdPrm(sp)          // set loader parameter block address
        jal     KiInitializeKernel      // initialize system data structures

//
// Control is returned to the idle thread with IRQL at HIGH_LEVEL. Lower IRQL
// to DISPATCH_LEVEL, set wait IRQL of idle thread, load global register values,
// and enter idle loop.
//

        move    s7,s3                   // set processor number
        lw      s0,KiPcr + PcPrcb(zero) // get processor control block address
        addu    s3,s0,PbDpcListHead     // compute DPC listhead address
        li      a0,DISPATCH_LEVEL       // get dispatch level IRQL
        sb      a0,ThWaitIrql(s1)       // set wait IRQL of idle thread
        jal     KeLowerIrql             // lower IRQL

        DISABLE_INTERRUPTS(s8)          // disable interrupts

        or      s8,s8,1 << PSR_IE       // set interrupt enable bit set
        subu    s6,s8,1 << PSR_IE       // clear interrupt enable bit

        ENABLE_INTERRUPTS(s8)           // enable interrupts

        move    s4,zero                 // clear breakin loop counter
        lbu     a0,KiSynchIrql          // get new IRQL value
        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        and     s5,s8,t1                // clear current interrupt enables
        or      s5,s5,t0                // set new interrupt enables

//
// In a multiprocessor system the boot processor proceeds directly into
// the idle loop. As other processors start executing, however, they do
// not directly enter the idle loop and spin until all processors have
// been started and the boot master allows them to proceed.
//

#if !defined(NT_UP)

110:    lw      t0,KiBarrierWait        // get the current barrier wait value
        bne     zero,t0,110b            // if ne, spin until allowed to proceed
        lbu     t1,KiPcr + PcNumber(zero) // get current processor number
        beq     zero,t1,120f            // if eq, processor zero
        jal     HalAllProcessorsStarted // perform platform specific operations
        bne     zero,v0,120f            // if ne, initialization succeeded
        li      a0,HAL1_INITIALIZATION_FAILED // set bug check reason
        jal     KeBugCheck              // bug check

#endif

//
// Allocate an exception frame and store the nonvolatile register and
// return address in the frame so when a context switch from the idle
// thread to another thread occurs, context does not have to be saved
// and the special swtich from idle entry pointer in the context swap
// code can be called.
//
// Registers s0 - s8 have the following contents:
//
//    s0 - Address of the current processor block.
//    s1 - Not used.
//    s2 - Not used.
//    s3 - Address of DPC listhead for current processor.
//    s4 - Debugger breakin poll counter.
//    s5 - Saved PSR with interrupt enabled and IRQL of synchronization level.
//    s6 - Saved PSR with interrupts disabled and an IRQL of DISPATCH_LEVEL.
//    s7 - Number of the current processor.
//    s8 - Saved PSR with interrupt enabled and IRQL of DISPATCH_LEVEL.
//

120:    subu    sp,sp,ExceptionFrameLength // allocate exception frame
        sw      s3,ExIntS3(sp)          // save register s3 - s8
        sw      s4,ExIntS4(sp)          //
        sw      s5,ExIntS5(sp)          //
        sw      s6,ExIntS6(sp)          //
        sw      s7,ExIntS7(sp)          //
        sw      s8,ExIntS8(sp)          //
        la      ra,KiIdleLoop           // set address of swap return
        sw      ra,ExSwapReturn(sp)     //
        j       KiIdleLoop              //

        .end    KiSystemBegin

//
// The following code represents the idle thread for a processor. The idle
// thread executes at IRQL DISPATCH_LEVEL and continually polls for work to
// do. Control may be given to this loop either as a result of a return from
// the system initialize routine or as the result of starting up another
// processor in a multiprocessor configuration.
//

        LEAF_ENTRY(KiIdleLoop)

#if DBG

        move    s4,zero                 // clear breakin loop counter

#endif

//
// Lower IRQL to DISPATCH_LEVEL and enable interrupts.
//

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        li      a0,DISPATCH_LEVEL       // get new IRQL value
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(s8)           // enable interrupts

//
// Check if the debugger is enabled, the current processor is zero, and
// whether it is time to poll for a debugger breakin.
//

KiIdleTop:                              //

#if DBG

#if !defined(NT_UP)

        bne     zero,s7,CheckDpcList    // if ne, not processor zero

#endif

        subu    s4,s4,1                 // decrement poll counter
        bgtz    s4,CheckDpcList         // if gtz, then not time to poll
        lbu     t0,KdDebuggerEnabled    // check if debugger is enabled
        li      s4,200 * 1000           // set breakin loop counter
        beq     zero,t0,CheckDpcList    // if eq, debugger not enabled
        jal     KdPollBreakIn           // check if breakin is requested
        beq     zero,v0,CheckDpcList    // if eq, no breakin requested
        li      a0,DBG_STATUS_CONTROL_C // break in and send
        jal     DbgBreakPointWithStatus //  status to the debugger

#endif

//
// Enable interrupts to allow any outstanding interrupts to occur, then
// disable interrupts and check if there is any work in the DPC list of
// the current processor.
//

CheckDpcList:                           //

//
// N.B. The following code enables interrupts for a few cycles, then
//      disables them again for the subsequent DPC and next thread
//      checks.
//

        .set    noreorder
        .set    noat
        mtc0    s8,psr                  // enable interrupts
        nop                             //
        nop                             //
        nop                             //
        nop                             // allow interrupts to occur
        nop                             //
        mtc0    s6,psr                  // disable interrupts
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        .set    at
        .set    reorder

//
// Process the deferred procedure call list for the current processor.
//

        lw      a0,LsFlink(s3)          // get address of next entry
        beq     a0,s3,CheckNextThread   // if eq, DPC list is empty

        .set    noreorder
        .set    noat
        mfc0    t0,cause                // get exception cause register
        and     t0,t0,APC_INTERRUPT     // clear dispatch interrupt pending
        mtc0    t0,cause                // set exception cause register
        .set    at
        .set    reorder

        move    v0,s8                   // set previous PSR value
        jal     KiRetireDpcList         // process the DPC list

#if DBG

        move    s4,zero                 // clear breakin loop counter

#endif

//
// Check if a thread has been selected to run on the current processor.
//

CheckNextThread:                        //
        lw      s2,PbNextThread(s0)     // get address of next thread object
        beq     zero,s2,20f             // if eq, no thread selected

//
// A thread has been selected for execution on this processor. Acquire
// dispatcher database lock, get the thread address again (it may have
// changed), clear the address of the next thread in the processor block,
// and call swap context to start execution of the selected thread.
//
// N.B. If the dispatcher database lock cannot be obtained immediately,
//      then attempt to process another DPC rather than spinning on the
//      dispatcher database lock.
//

        lbu     a0,KiSynchIrql          // get new IRQL value

#if !defined(NT_UP)

10:     ll      t0,KiDispatcherLock     // get current lock value
        move    t1,s2                   // set lock ownership value
        bne     zero,t0,CheckDpcList    // if ne, spin lock owned
        sc      t1,KiDispatcherLock     // set spin lock owned
        beq     zero,t1,10b             // if eq, store conditional failed

#endif

//
// Raise IRQL to synchronization level and enable interrupts.
//

        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(s5)           // enable interrupts

        lw      s1,PbCurrentThread(s0)  // get address of current thread
        lw      s2,PbNextThread(s0)     // get address of next thread object
        sw      zero,PbNextThread(s0)   // clear next thread address
        sw      s2,PbCurrentThread(s0)  // set address of current thread object

//
// Set the thread state to running.
//

        li      t0,Running              // set thread state to running
        sb      t0,ThState(s2)          //

//
// Acquire the context swap lock so the address space of the old process
// cannot be deleted and then release the dispatcher database lock. In
// this case the old process is the system process, but the context swap
// code releases the context swap lock so it must be acquired.
//
// N.B. This lock is used to protect the address space until the context
//    switch has sufficiently progressed to the point where the address
//    space is no longer needed. This lock is also acquired by the reaper
//    thread before it finishes thread termination.
//

#if !defined(NT_UP)

15:     ll      t0,KiContextSwapLock    // get current lock value
        move    t1,s2                   // set ownership value
        bne     zero,t0,15b             // if ne, lock already owned
        sc      t1,KiContextSwapLock    // set lock ownership value
        beq     zero,t1,15b             // if eq, store conditional failed
        sw      zero,KiDispatcherLock   // set lock not owned

#endif

        j       SwapFromIdle            // swap context to new thread

//
// There are no entries in the DPC list and a thread has not been selected
// for excuttion on this processor. Call the HAL so power managment can be
// performed.
//
// N.B. The HAL is called with interrupts disabled. The HAL will return
//      with interrupts enabled.
//

20:     la      ra,KiIdleTop            // set return address
        lw      t0,__imp_HalProcessorIdle // notify HAL of idle state
        j       t0                      //

        .end    KiIdleLoop

        SBTTL("Retire Deferred Procedure Call List")
//++
//
// Routine Description:
//
//    This routine is called to retire the specified deferred procedure
//    call list. DPC routines are called using the idle thread (current)
//    stack.
//
//    N.B. Interrupts must be disabled on entry to this routine. Control
//         is returned to the caller with the same conditions true.
//
// Arguments:
//
//    v0 - Previous PSR value.
//    s0 - Address of the current PRCB.
//
// Return value:
//
//    None.
//
//--

        .struct 0
        .space  4 * 4                   // argument save area
DpRa:   .space  4                       // return address
        .space  4                       // fill

#if DBG

DpStart:.space  4                       // DPC start time in ticks
DpFunct:.space  4                       // DPC function address
DpCount:.space  4                       // interrupt count at start of DPC
DpTime: .space  4                       // interrupt time at start of DPC

#endif

DpcFrameLength:                         // DPC frame length

        NESTED_ENTRY(KiRetireDpcList, DpcFrameLength, zero)

        subu    sp,sp,DpcFrameLength    // allocate stack frame
        sw      ra,DpRa(sp)             // save return address

        PROLOGUE_END

5:      sw      sp,PbDpcRoutineActive(s0) // set DPC routine active
        sw      sp,KiPcr + PcDpcRoutineActive(zero) //

//
// Process the DPC list.
//

10:     addu    a1,s0,PbDpcListHead     // compute DPC listhead address
        lw      a0,LsFlink(a1)          // get address of next entry
        beq     a0,a1,60f               // if eq, DPC list is empty

#if !defined(NT_UP)

20:     ll      t1,PbDpcLock(s0)        // get current lock value
        move    t2,s0                   // set lock ownership value
        bne     zero,t1,20b             // if ne, spin lock owned
        sc      t2,PbDpcLock(s0)        // set spin lock owned
        beq     zero,t2,20b             // if eq, store conditional failed
        lw      a0,LsFlink(a1)          // get address of next entry
        beq     a0,a1,50f               // if eq, DPC list is empty

#endif

        lw      t1,LsFlink(a0)          // get address of next entry
        subu    a0,a0,DpDpcListEntry    // compute address of DPC Object
        sw      t1,LsFlink(a1)          // set address of next in header
        sw      a1,LsBlink(t1)          // set address of previous in next
        lw      a1,DpDeferredContext(a0) // get deferred context argument
        lw      a2,DpSystemArgument1(a0) // get first system argument
        lw      a3,DpSystemArgument2(a0) // get second system argument
        lw      t1,DpDeferredRoutine(a0) // get deferred routine address
        sw      zero,DpLock(a0)         // clear DPC inserted state
        lw      t2,PbDpcQueueDepth(s0)  // decrement the DPC queue depth
        subu    t2,t2,1                 //
        sw      t2,PbDpcQueueDepth(s0)  //

#if !defined(NT_UP)

        sw      zero,PbDpcLock(s0)      // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(v0)           // enable interrupts

#if DBG

        sw      t1,DpFunct(sp)          // save DPC function address
        lw      t2,KeTickCount          // save current tick count
        sw      t2,DpStart(sp)          //
        lw      t3,PbInterruptCount(s0) // get current interrupt count
        lw      t4,PbInterruptTime(s0)  // get current interrupt time
        sw      t3,DpCount(sp)          // save interrupt count at start of DPC
        sw      t4,DpTime(sp)           // save interrupt time at start of DPC

#endif

        jal     t1                      // call DPC routine

#if DBG

        lbu     t0,KiPcr + PcCurrentIrql(zero) // get current IRQL
        sltu    t1,t0,DISPATCH_LEVEL    // check if less than dispatch level
        beq     zero,t1,30f             // if eq, not less than dispatch level
        lw      t1,DpFunct(sp)          // get DPC function address
        jal     DbgBreakPoint           // execute debug breakpoint
30:     lw      t0,KeTickCount          // get current tick count
        lw      t1,DpStart(sp)          // get starting tick count
        lw      t2,DpFunct(sp)          // get DPC function address
        subu    t3,t0,t1                // compute time in DPC function
        sltu    t3,t3,100               // check if less than one second
        bne     zero,t3,40f             // if ne, less than one second
        lw      t3,PbInterruptCount(s0) // get current interrupt count
        lw      t4,PbInterruptTime(s0)  // get current interrupt time
        lw      t5,DpCount(sp)          // get starting interrupt count
        lw      t6,DpTime(sp)           // get starting interrupt time
        subu    t3,t3,t5                // compute number of interrupts
        subu    t4,t4,t6                // compute time of interrupts
        jal     DbgBreakPoint           // execute debug breakpoint

#endif

40:     DISABLE_INTERRUPTS(v0)          // disable interrupts

        b       10b                     //

//
// Unlock DPC list and clear DPC active.
//

50:

#if !defined(NT_UP)

        sw      zero,PbDpcLock(s0)      // set spin lock not owned

#endif

60:     sw      zero,PbDpcRoutineActive(s0) // clear DPC routine active
        sw      zero,KiPcr + PcDpcRoutineActive(zero) //
        sw      zero,PbDpcInterruptRequested(s0) // clear DPC interrupt requested

//
// Check one last time that the DPC list is empty. This is required to
// close a race condition with the DPC queuing code where it appears that
// a DPC routine is active (and thus an interrupt is not requested), but
// this code has decided the DPC list is empty and is clearing the DPC
// active flag.
//

        addu    a1,s0,PbDpcListHead     // compute DPC listhead address
        lw      a0,LsFlink(a1)          // get address of next entry
        bne     a0,a1,5b                // if ne, DPC list is not empty
        lw      ra,DpRa(sp)             // restore return address
        addu    sp,sp,DpcFrameLength    // deallocate stack frame
        j       ra                      // return

        .end    KiRetireDpcList
