//        TITLE("Interprocessor Interrupt support routines")
//++
//
// Copyright (c) 1993  Microsoft Corporation
// Copyright (c) 1994  Motorola
// Copyright (c) 1994  IBM Corporation
//
// Module Name:
//
//    mpipi.s
//
// Abstract:
//
//    This module implements the PPC specific functions required to
//    support multiprocessor systems.
//
// Author:
//
//    Pat Carr
//
// Based on:  ke\mips\x4mpipi.s, authored by David N. Cutler (davec)
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

//list(off)
#include "ksppc.h"
//list(on)

        .extern ..KiFreezeTargetExecution
        .extern __imp_HalRequestIpi

        .extern KiProcessorBlock


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
//    Its function is to process all interprocess immediate and packet
//    requests.
//
//    This routine is entered at IPI_LEVEL.
//
// Arguments:
//
//    TrapFrame (r.3) - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

                .struct 0
ipi_int_hdr:    .space  StackFrameHeaderLength
ipi_int_ex_frm: .space  ExceptionFrameLength
                .align  3
ipi_int_frm_len:


        SPECIAL_ENTRY_S(KeIpiInterrupt, _TEXT$01)

#if !defined(NT_UP)

        stw     r.31, ExGpr31+ipi_int_ex_frm-ipi_int_frm_len(r.sp)
        stw     r.30, ExGpr30+ipi_int_ex_frm-ipi_int_frm_len(r.sp)
        mflr    r.30
        stwu    r.sp, -ipi_int_frm_len(r.sp)

#endif

        PROLOGUE_END(KeIpiInterrupt)

#if !defined(NT_UP)

        ori     r.31, r.3, 0            // save address of trap frame

//
// Process all interprocessor requests.
//
// N.B. KiIpiProcessRequests returns condition register bit 29 set
//      if freeze requested.
//

        bl      ..KiIpiProcessRequests  // process requests
        bf      29, ipi_int_fini        // jif no freeze requested

//
// Save the volatile floating state.
//

        SAVE_VOLATILE_FLOAT_STATE(r.31)

//
// Save the nonvolatile state:  integer registers and floating registers
//

        la      r.4, ipi_int_ex_frm(r.sp) // address of exception frame

        stw     r.13, ExGpr13(r.4)      // save non-volatile GPRs
        stw     r.14, ExGpr14(r.4)
        stw     r.15, ExGpr15(r.4)
        stw     r.16, ExGpr16(r.4)
        stw     r.17, ExGpr17(r.4)
        stw     r.18, ExGpr18(r.4)
        stw     r.19, ExGpr19(r.4)
        stw     r.20, ExGpr20(r.4)
        stw     r.21, ExGpr21(r.4)
        stw     r.22, ExGpr22(r.4)
        stw     r.23, ExGpr23(r.4)
        stw     r.24, ExGpr24(r.4)
        stw     r.25, ExGpr25(r.4)
        stw     r.26, ExGpr26(r.4)
        stw     r.27, ExGpr27(r.4)
        stw     r.28, ExGpr28(r.4)
        stw     r.29, ExGpr29(r.4)

        stfd    f.14, ExFpr14(r.4)      // save non-volatile FPRs
        stfd    f.15, ExFpr15(r.4)
        stfd    f.16, ExFpr16(r.4)
        stfd    f.17, ExFpr17(r.4)
        stfd    f.18, ExFpr18(r.4)
        stfd    f.19, ExFpr19(r.4)
        stfd    f.20, ExFpr20(r.4)
        stfd    f.21, ExFpr21(r.4)
        stfd    f.22, ExFpr22(r.4)
        stfd    f.23, ExFpr23(r.4)
        stfd    f.24, ExFpr24(r.4)
        stfd    f.25, ExFpr25(r.4)
        stfd    f.26, ExFpr26(r.4)
        stfd    f.27, ExFpr27(r.4)
        stfd    f.28, ExFpr28(r.4)
        stfd    f.29, ExFpr29(r.4)
        stfd    f.30, ExFpr30(r.4)
        stfd    f.31, ExFpr31(r.4)

//
// Freeze the execution of the current processor.
//

        ori     r.3, r.31, 0              // address of trap frame
//      la      r.4, ipi_int_ex_frm(r.sp) // address of exception frame
        bl      ..KiFreezeTargetExecution // freeze current processor execution

//
// Restore the nonvolatile state:  floating registers and integer registers
//

        la      r.3, ipi_int_ex_frm(r.sp) // address of exception frame

        lfd     f.14, ExFpr14 (r.3)       // restore non-volatile FPRs
        lfd     f.15, ExFpr15 (r.3)
        lfd     f.16, ExFpr16 (r.3)
        lfd     f.17, ExFpr17 (r.3)
        lfd     f.18, ExFpr18 (r.3)
        lfd     f.19, ExFpr19 (r.3)
        lfd     f.20, ExFpr20 (r.3)
        lfd     f.21, ExFpr21 (r.3)
        lfd     f.22, ExFpr22 (r.3)
        lfd     f.23, ExFpr23 (r.3)
        lfd     f.24, ExFpr24 (r.3)
        lfd     f.25, ExFpr25 (r.3)
        lfd     f.26, ExFpr26 (r.3)
        lfd     f.27, ExFpr27 (r.3)
        lfd     f.28, ExFpr28 (r.3)
        lfd     f.29, ExFpr29 (r.3)
        lfd     f.30, ExFpr30 (r.3)
        lfd     f.31, ExFpr31 (r.3)

        lwz     r.14, ExGpr14 (r.3)       // restore non-volatile GPRs
        lwz     r.15, ExGpr15 (r.3)
        lwz     r.16, ExGpr16 (r.3)
        lwz     r.17, ExGpr17 (r.3)
        lwz     r.18, ExGpr18 (r.3)
        lwz     r.19, ExGpr19 (r.3)
        lwz     r.20, ExGpr20 (r.3)
        lwz     r.21, ExGpr21 (r.3)
        lwz     r.22, ExGpr22 (r.3)
        lwz     r.23, ExGpr23 (r.3)
        lwz     r.24, ExGpr24 (r.3)
        lwz     r.25, ExGpr25 (r.3)
        lwz     r.26, ExGpr26 (r.3)
        lwz     r.27, ExGpr27 (r.3)
        lwz     r.28, ExGpr28 (r.3)
        lwz     r.29, ExGpr29 (r.3)

//
// Restore the volatile floating state.
//

        RESTORE_VOLATILE_FLOAT_STATE(r.31)

ipi_int_fini:
        mtlr    r.30
        lwz     r.31, ExGpr31+ipi_int_ex_frm(r.sp)
        lwz     r.30, ExGpr30+ipi_int_ex_frm(r.sp)
        addi    r.sp, r.sp, ipi_int_frm_len

#endif
        SPECIAL_EXIT(KeIpiInterrupt)

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
//    CR.7 contains the 4 LSBs of request summary, specifically,
//    CR bit 29 is set if freeze is requested.
//
//--
                .struct 0
                .space  StackFrameHeaderLength
PrTocSave:      .space  4
PrLrSave:       .space  4
Pr30Save:       .space  4
                .align  3
PrFrameLength:

        SPECIAL_ENTRY_S(KiIpiProcessRequests, _TEXT$01)

#if !defined(NT_UP)

        mflr    r.0                             // get return address
        stwu    r.sp, -PrFrameLength(r.sp)      // buy stack frame
        stw     r.30,  Pr30Save(r.sp)           // save reg 30
        stw     r.toc, PrTocSave(r.sp)          // save our toc
        lwz     r.30,  KiPcr+PcPrcb(r.0)        // get processor control block
        stw     r.0,   PrLrSave(r.sp)           // save return address

#endif

        PROLOGUE_END(KiIpiProcessRequests)

#if !defined(NT_UP)

//
// Check for Packet ready.
//
// If a packet is ready, then get the address of the requested function
// and call the function passing the address of the packet as a parameter.
//
// N.B. We do not need to check/clear the SignalDone field using
//      atomic operations because this processor is the only processor
//      attempting to clear this field and only clears it when it takes
//      work from it.   Other processors will only write to this field
//      when it is zero (they must use atomic operations to update it).
//

kipr_10:
        lwz     r.3, PbSignalDone(r.30)  // get packet source prcb
        li      r.6, 0                   // either way we need zero
        cmpwi   r.3, 0                   // check for packet ready
        addi    r.30, r.30, PbRequestSummary
        beq     kipr_20                  // if eq, no packet ready

//
// Packet ready.  Clear SignalDone then call the requested function.
// r.3 now contains the address of the PRCB of the processor which
// made the request.  That PRCB contains the function address and
// parameters.
//

        stw     r.6, PbSignalDone-PbRequestSummary(r.30)
        lwz     r.6, PbWorkerRoutine(r.3)// get &worker function fn desc
        lwz     r.4, PbCurrentPacket(r.3)// get request parameters
        lwz     r.0, 0(r.6)              // get worker entry point
        lwz     r.5, PbCurrentPacket+4(r.3)
        mtlr    r.0                      // set entry address
        lwz     r.toc, 4(r.6)            // get worker's toc
        lwz     r.6, PbCurrentPacket+8(r.3)
        blrl                             // call worker routine
        lwz     r.0,  PrLrSave(r.sp)     // get return address
        lwz     r.toc, PrTocSave(r.sp)   // restore kernel toc
        li      r.6, 0                   // need zero again

#if NT_INST

//
// Increment number of packet requests
//

        lwz     r.3, PbIpiCounts-PbRequestSummary+IcPacket(r.30)
        addi    r.3, r.3, 1
        stw     r.3, PbIpiCounts-PbRequestSummary+IcPacket(r.30)

#endif

        mtlr    r.0                      // reset return address

//
// Read request summary and write a zero result interlocked.
//

kipr_20:
        lwarx   r.3, 0, r.30             // get request summary
        stwcx.  r.6, 0, r.30             // zero request summary
        bne-    kipr_20                  // if ne, store conditional failed

//
// WARNING: For speed we are just going to move the request summary
//          into the condition register field 7.  The following code
//          is dependent on the following values-
//
//          IPI_APC     1       (condition register bit 31)
//          IPI_DPC     2       (condition register bit 30)
//          IPI_FREEZE  4       (condition register bit 29)
//

        mtcrf   0x01, r.3               // set appropriate CR bits
        li      r.4, 1                  // will need 1 if apc or dpc

//
// Check for APC interrupt request.
//
// If an APC interrupt is requested, then request a software interrupt at
// APC level on the current processor.
//


        bf      31, kipr_25             // jif no APC requested
        stb     r.4, KiPcr+PcApcInterrupt(r.0) // set APC interrupt request

#if NT_INST

//
// Increment number of APC requests
//

        lwz     r.5, PbIpiCounts-PbRequestSummary+IcAPC(r.30)
        addi    r.5, r.5, 1
        stw     r.5, PbIpiCounts-PbRequestSummary+IcAPC(r.30)

#endif

//
// Check for DPC interrupt request.
//
// If an DPC interrupt is requested, then request a software interrupt at
// DPC level on the current processor.
//

kipr_25:
        bf      30, kipr_30             // jif no DPC requested
        stb     r.4, KiPcr+PcDispatchInterrupt(r.0) // set DPC interrupt request

#if NT_INST

//
// Increment number of DPC requests
//

        lwz     r.5, PbIpiCounts-PbRequestSummary+IcDPC(r.30)
        addi    r.5, r.5, 1
        stw     r.5, PbIpiCounts-PbRequestSummary+IcDPC(r.30)

#endif

//
// Set function return value, restores registers, and return.
//

kipr_30:

#if NT_INST

        bf      29, kipr_40             // jif no freeze requested

//
// Increment number of freeze requests
//

        lwz     r.5, PbIpiCounts-PbRequestSummary+IcFreeze(r.30)
        addi    r.5, r.5, 1
        stw     r.5, PbIpiCounts-PbRequestSummary+IcFreeze(r.30)

#endif

kipr_40:

//
// N.B. Returning RequestSummary in r.3 (history), CR bit 29 set
//      if freeze requested.
//

        lwz     r.30, Pr30Save(r.sp)    // restore reg 30
        addi    r.sp, r.sp, PrFrameLength

#endif

        SPECIAL_EXIT(KiIpiProcessRequests)

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
//    TargetProcessors (r.3) - Supplies the set of processors on which the
//                             specified operation is to be executed.
//
//    IpiRequest (r.4)       - Supplies the request operation mask.
//
// Return Value:
//
//    None.
//
//--

                .struct 0
                .space  StackFrameHeaderLength
SpTocSave:      .space  4
Sp31Save:       .space  4
                .align  3
SpFrameLength:

        SPECIAL_ENTRY_S(KiIpiSend, _TEXT$01)

#if !defined(NT_UP)

        lwz     r.9, [toc]__imp_HalRequestIpi(r.toc)
        stwu    r.sp, -SpFrameLength(r.sp) // buy stack frame
        stw     r.31, Sp31Save(r.sp)       // save r.31
        mflr    r.31                       // save return address in r.31
        lwz     r.9, 0(r.9)                // get HalRequestIpi fn descr
        rlwinm. r.5, r.3, 0, 1             // check if cpu 0 is a target
        lwz     r.7, [toc]KiProcessorBlock(r.toc)// get &processor block array
        stw     r.2, SpTocSave(r.sp)       // save kernel's TOC

#endif

        PROLOGUE_END(KiIpiSend)

#if !defined(NT_UP)

        ori     r.6, r.3, 0             // copy target processors
        lwz     r.0, 0(r.9)             // get HalRequestIpi entry
        li      r.10,PbRequestSummary   // offset to RequestSummary in PRCB

kis10:  beq     kis30                   // if eq, target not specified

        lwz     r.5, 0(r.7)             // get target processor block address

kis20:  lwarx   r.8, r.10, r.5          // get request summary of target
        or      r.8, r.8, r.4           // merge current request with summary
        stwcx.  r.8, r.10, r.5          // store request summary
        bne-    kis20                   // if ne, store conditional failed

kis30:  addi    r.7, r.7, 4             // advance to next array element
        srwi.   r.6, r.6, 1             // shift out target bit
        beq     kis40                   // if eq, no more targets requested
        rlwinm. r.5, r.6, 0, 1          // check if target bit set
        b       kis10

kis40:  mtlr    r.0                     // set HalRequestIpi entry
        lwz     r.2, 4(r.9)             // get HAL's toc
        blrl                            // request IPI interrupt on targets

        mtlr    r.31                    // set return address
        lwz     r.toc, SpTocSave(r.sp)  // restore kernel's toc
        lwz     r.31, Sp31Save(r.sp)    // restore r.31
        addi    r.sp, r.sp, SpFrameLength

#endif

        SPECIAL_EXIT(KiIpiSend)

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
//    TargetProcessors - Supplies the set of processors on which the
//        specified operation is to be executed.
//
//    WorkerFunction   - Supplies the address of the worker function.
//
//    Parameter1 - Parameter3 - Supplies worker function specific parameters.
//
// Return Value:
//
//    None.
//
//--

        SPECIAL_ENTRY_S(KiIpiSendPacket, _TEXT$01)

#if !defined(NT_UP)

        lwz     r.9, [toc]__imp_HalRequestIpi(r.toc)
        stwu    r.sp, -SpFrameLength(r.sp) // buy stack frame
        stw     r.31, Sp31Save(r.sp)       // save r.31
        mflr    r.31                       // save return address in r.31
        lwz     r.9, 0(r.9)                // get HalRequestIpi fn descr
        stw     r.2, SpTocSave(r.sp)       // save kernel's TOC

#endif

        PROLOGUE_END(KiIpiSendPacket)

#if !defined(NT_UP)

        lwz     r.12, KiPcr+PcPrcb(r.0)    // get this processor's PRCB
        ori     r.11, r.3, 0               // copy target processor set
        lwz     r.0,  0(r.9)               // get HalRequestIpi entry addr

//
// Store function address and parameters in the packet area of the PRCB on
// the current processor.
//

        stw     r.3,PbTargetSet(r.12)      // set target processor set
        stw     r.4,PbWorkerRoutine(r.12)  // set worker function address
        stw     r.5,PbCurrentPacket(r.12)  // store worker function parameters
        stw     r.6,PbCurrentPacket+4(r.12)//
        stw     r.7,PbCurrentPacket+8(r.12)//

// GPRs r.4, - r.7 now available ...

        lwz     r.4, [toc]KiProcessorBlock(r.toc)// get &processor block array
        mtlr    r.0                     // set addr of HalRequestIpi entry

//
// Ensure above stores complete w.r.t. memory prior to allowing any
// processor to begin this request.
//

        eieio

//
// Loop through the target processors and send the packet to the specified
// recipients.
//

kisp10:
        lwz     r.10, 0(r.4)            // get target processor block address
        rlwinm. r.8, r.11, 0, 1         // check if target bit set
        srwi    r.11, r.11, 1           // shift out target processor
        addi    r.10, r.10, PbSignalDone // get packet lock address
        beq     kisp30                  // if eq, target not specified

//
// PowerPC uses the SignalDone field in the PRCB to indicate packet
// status.  Non zero implies packet busy.  This saves us having to
// update both the RequestSummary and the SignalDOne fields in an
// atomic manner.
//
// N.B. We write this like it's a spin lock, even though it isn't, quite.
//

        ACQUIRE_SPIN_LOCK(r.10, r.12, r.6, kisp20, kisp40)

kisp30: cmpwi   r.11, 0
        addi    r.4, r.4, 4             // advance to get array element
        bne     kisp10                  // if ne, more targets requested

        lwz     r.2, 4(r.9)             // get HAL's toc
        blrl                            // call HalRequestIpi

        mtlr    r.31                    // set return address
        lwz     r.toc, SpTocSave(r.sp)  // restore kernel's toc
        lwz     r.31, Sp31Save(r.sp)    // restore r.31
        addi    r.sp, r.sp, SpFrameLength

        blr

        SPIN_ON_SPIN_LOCK(r.10, r.6, kisp20, kisp40)
#endif

        DUMMY_EXIT(KiIpiSendPacket)

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
//    SignalDone (r.3) - Supplies a pointer to the processor block of the
//        sending processor.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiIpiSignalPacketDone)

        li      r.9, PbTargetSet        // offset to target set in prcb
        lwz     r.4, KiPcr+PcNotMember(r.0) // get processor set member
sigdn:  lwarx   r.5, r.9, r.3           // get request target set
        and     r.5, r.5, r.4           // clear processor set member
        stwcx.  r.5, r.9, r.3           // store target set
        bne-    sigdn                   // if ne, store conditional failed

        LEAF_EXIT(KiIpiSignalPacketDone)
