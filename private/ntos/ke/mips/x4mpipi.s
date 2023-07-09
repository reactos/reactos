//        TITLE("Interprocessor Interrupt support routines")
//++
//
// Copyright (c) 1993  Microsoft Corporation
//
// Module Name:
//
//    x4mpipi.s
//
// Abstract:
//
//    This module implements the MIPS specific functions required to
//    support multiprocessor systems.
//
// Author:
//
//    David N. Cutler (davec) 22-Apr-1993
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Interprocess Interrupt Processing")
//++
//
// VOID
// KeIpiInterrupt (
//    IN PKTRAP_FRAME TrapFrame
//    );
//
// Routine Description:
//
//    This routine is entered as the result of an interprocessor interrupt.
//    It's function is to process all interprocess immediate and packet
//    requests.
//
// Arguments:
//
//    TrapFrame (s8) - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KeIpiInterrupt, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate exception frame
        sw      ra,ExIntRa(sp)          // save return address

        PROLOGUE_END

//
// Process all interprocessor requests.
//

        jal     KiIpiProcessRequests    // process requests
        andi    v1,v0,IPI_FREEZE        // check if freeze is requested
        beq     zero,v1,10f             // if eq, no freeze requested

//
// Save the floating state.
//

        SAVE_VOLATILE_FLOAT_STATE       // save volatile floating state

        sdc1    f20,ExFltF20(sp)        // save floating registers f20 - f31
        sdc1    f22,ExFltF22(sp)        //
        sdc1    f24,ExFltF24(sp)        //
        sdc1    f26,ExFltF26(sp)        //
        sdc1    f28,ExFltF28(sp)        //
        sdc1    f30,ExFltF30(sp)        //

//
// Freeze the execution of the current processor.
//

        move    a0,s8                   // set address of trap frame
        move    a1,sp                   // set address of exception frame
        jal     KiFreezeTargetExecution // freeze current processor execution

//
// Restore the volatile floating state.
//

        RESTORE_VOLATILE_FLOAT_STATE    // restore volatile floating state

        ldc1    f20,ExFltF20(sp)        // restore floating registers f20 - f31
        ldc1    f22,ExFltF22(sp)        //
        ldc1    f24,ExFltF24(sp)        //
        ldc1    f26,ExFltF26(sp)        //
        ldc1    f28,ExFltF28(sp)        //
        ldc1    f30,ExFltF30(sp)        //

10:     lw      ra,ExIntRa(sp)          // restore return address
        addu    sp,sp,ExceptionFrameLength // deallocate exception frame
        j       ra                      // return

        .end    KeIpiInterrupt

        SBTTL("Processor Request")
//++
//
// ULONG
// KiIpiProcessRequests (
//    VOID
//    );
//
// Routine Description:
//
//    This routine processes interprocessor requests and returns a summary
//    of the requests that were processed.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    The request summary is returned as the function value.
//
//--

        .struct 0
        .space  4 * 4                   // argument save area
PrS0:   .space  4                       // saved integer register s0
PrS1:   .space  4                       // saved integer register s1
        .space  4                       // fill
PrRa:   .space  4                       // saved return address
PrFrameLength:                          // frame length

        NESTED_ENTRY(KiIpiProcessRequests, PrFrameLength, zero)

        subu    sp,sp,PrFrameLength     // allocate exception frame
        sw      s0,PrS0(sp)             // save integer register s0

#if NT_INST

        sw      s1,PrS1(sp)             // save integer register s1

#endif

        sw      ra,PrRa(sp)             // save return address

        PROLOGUE_END

//
// Read request summary and write a zero result interlocked.
//

        lw      t0,KiPcr + PcPrcb(zero) // get current processor block address
10:     lld     t1,PbRequestSummary(t0) // get request summary and entry address
        move    t2,zero                 // set zero value for store
        scd     t2,PbRequestSummary(t0) // zero request summary
        beq     zero,t2,10b             // if eq, store conditional failed
        dsra    a0,t1,32                // shift entry address to low 32-bits
        move    s0,t1                   // copy request summary

//
// Check for Packet ready.
//
// If a packet is ready, then get the address of the requested function
// and call the function passing the address of the packet address as a
// parameter.
//

        and     t1,s0,IPI_PACKET_READY  // check for packet ready
        beq     zero,t1,20f             // if eq, packet not ready
        lw      t2,PbWorkerRoutine(a0)  // get address of worker function
        lw      a1,PbCurrentPacket(a0)  // get request parameters
        lw      a2,PbCurrentPacket + 4(a0) //
        lw      a3,PbCurrentPacket + 8(a0) //
        jal     t2                      // call work routine

#if NT_INST

        lw      s1,PbIpiCounts(t0)      // get interrupt count structure
        lw      t1,IcPacket(s1)         // increment number of packet requests
        addu    t1,t1,1                 //
        sw      t1,IcPacket(s1)         //

#endif

//
// Check for APC interrupt request.
//
// If an APC interrupt is requested, then request a software interrupt at
// APC level on the current processor.
//

20:     and     t1,s0,IPI_APC           // check for APC interrupt request
        beq     zero,t1,25f             // if eq, no APC interrupt requested

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,cause                // get exception cause register
        or      t1,t1,APC_INTERRUPT     // set dispatch interrupt request
        mtc0    t1,cause                // set exception cause register
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

#if NT_INST

        lw      t1,IcAPC(s1)            // increment number of APC requests
        addu    t1,t1,1                 //
        sw      t1,IcAPC(s1)            //

#endif

//
// Check for DPC interrupt request.
//
// If an DPC interrupt is requested, then request a software interrupt at
// DPC level on the current processor.
//

25:     and     t1,s0,IPI_DPC           // check for DPC interrupt request
        beq     zero,t1,30f             // if eq, no DPC interrupt requested

        DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,cause                // get exception cause register
        or      t1,t1,DISPATCH_INTERRUPT // set dispatch interrupt request
        mtc0    t1,cause                // set exception cause register
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

#if NT_INST

        lw      t1,IcDPC(s1)            // increment number of DPC requests
        addu    t1,t1,1                 //
        sw      t1,IcDPC(s1)            //

#endif

//
// Set function return value, restores registers, and return.
//

30:     move    v0,s0                   // set funtion return value
        lw      s0,PrS0(sp)             // restore integer register s0

#if NT_INST

        and     t1,v0,IPI_FREEZE        // check if freeze requested
        beq     zero,t1,40f             // if eq, no freeze request
        lw      t1,IcFreeze(s1)         // increment number of freeze requests
        addu    t1,t1,1                 //
        sw      t1,IcFreeze(s1)         //
40:     lw      s1,PrS1(sp)             // restore integer register s1

#endif

        lw      ra,PrRa(sp)             // restore return address
        addu    sp,sp,PrFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    KiIpiProcessRequests

        SBTTL("Send Interprocess Request")
//++
//
// VOID
// KiIpiSend (
//    IN KAFINITY TargetProcessors,
//    IN KIPI_REQUEST IpiRequest
//    );
//
// Routine Description:
//
//    This routine requests the specified operation on the target set of
//    processors.
//
// Arguments:
//
//    TargetProcessors (a0) - Supplies the set of processors on which the
//        specified operation is to be executed.
//
//    IpiRequest (a1) - Supplies the request operation mask.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiIpiSend)

#if !defined(NT_UP)

        move    v0,a0                   // copy target processor set
        la      v1,KiProcessorBlock     // get processor block array address

//
// Loop through the target processors and send the request to the specified
// recipients.
//

10:     and     t1,v0,1                 // check if target bit set
        srl     v0,v0,1                 // shift out target processor
        beq     zero,t1,30f             // if eq, target not specified
        lw      t1,0(v1)                // get target processor block address
20:     lld     t3,PbRequestSummary(t1) // get request summary of target
        or      t3,t3,a1                // merge current request with summary
        scd     t3,PbRequestSummary(t1) // store request summary and entry address
        beq     zero,t3,20b             // if eq, store conditional failed
30:     add     v1,v1,4                 // advance to next array element
        bne     zero,v0,10b             // if ne, more targets requested
        lw      t0,__imp_HalRequestIpi  // request IPI interrupt on targets
        j       t0                      //
#else

        j       ra                      // return

#endif

        .end    KiIpiSend

        SBTTL("Send Interprocess Request Packet")
//++
//
// VOID
// KiIpiSendPacket (
//    IN KAFINITY TargetProcessors,
//    IN PKIPI_WORKER WorkerFunction,
//    IN PVOID Parameter1,
//    IN PVOID Parameter2,
//    IN PVOID Parameter3
//    );
//
// Routine Description:
//
//    This routine executes the specified worker function on the specified
//    set of processors.
//
// Arguments:
//
//    TargetProcessors (a0) - Supplies the set of processors on which the
//        specified operation is to be executed.
//
//    WorkerFunction (a1) - Supplies the address of the worker function.
//
//    Parameter1 - Parameter3 (a2, a3, 4 * 4(sp)) - Supplies worker
//        function specific parameters.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiIpiSendPacket)

#if !defined(NT_UP)

        lw      t0,KiPcr + PcPrcb(zero) // get current processor block address
        move    v0,a0                   // copy target processor set
        la      v1,KiProcessorBlock     // get processor block array address

//
// Store function address and parameters in the packet area of the PRCB on
// the current processor.
//

        lw      t9,4 * 4(sp)            // get parameter3 value
        sw      a0,PbTargetSet(t0)      // set target processor set
        sw      a1,PbWorkerRoutine(t0)  // set worker function address
        sw      a2,PbCurrentPacket(t0)  // store worker function parameters
        sw      a3,PbCurrentPacket + 4(t0) //
        sw      t9,PbCurrentPacket + 8(t0) //

//
// Loop through the target processors and send the packet to the specified
// recipients.
//

10:     and     t1,v0,1                 // check if target bit set
        srl     v0,v0,1                 // shift out target processor
        beq     zero,t1,30f             // if eq, target not specified
        lw      t1,0(v1)                // get target processor block address
        dsll    t3,t0,32                // shift entry address to upper 32-bits
        or      t3,t3,IPI_PACKET_READY  // set packet ready in lower 32-bits
20:     lld     t4,PbRequestSummary(t1) // get request summary of target
        and     t5,t4,IPI_PACKET_READY  // check if target packet busy
        or      t4,t4,t3                // set entry address in request summary
        bne     zero,t5,20b             // if ne, target packet busy
        scd     t4,PbRequestSummary(t1) // store request summary and entry address
        beq     zero,t4,20b             // if eq, store conditional failed
30:     addu    v1,v1,4                 // advance to get array element
        bne     zero,v0,10b             // if ne, more targets requested
        lw      t0,__imp_HalRequestIpi  // request IPI interrupt on targets
        j       t0                      //

#else

        j       ra                      // return

#endif

        .end    KiIpiSendPacket

        SBTTL("Signal Packet Done")
//++
//
// VOID
// KeIpiSignalPacketDone (
//    IN PVOID SignalDone
//    );
//
// Routine Description:
//
//    This routine signals that a processor has completed a packet by
//    clearing the calling processor's set member of the requesting
//    processor's packet.
//
// Arguments:
//
//    SignalDone (a0) - Supplies a pointer to the processor block of the
//        sending processor.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiIpiSignalPacketDone)

        lw      a1,KiPcr + PcNotMember(zero) // get processor set member
10:     ll      a2,PbTargetSet(a0)      // get request target set
        and     a2,a2,a1                // clear processor set member
        sc      a2,PbTargetSet(a0)      // store target set
        beq     zero,a2,10b             // if eq, store conditional failed
        j       ra                      // return

        .end    KiIpiSignalPacketDone
