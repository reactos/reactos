//++
//
// Copyright (c) 1989  Microsoft Corporation
//
// Module Name:
//
//    procstat.asm
//
// Abstract:
//
//    This module implements procedures for saving and restoring
//    processor control state.
//
//    These procedures support debugging of UP and MP systems.
//
// Author:
//
//    Chuck Bauman (v-cbaum@microsoft.com) 7-Nov-1994
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksppc.h"

// Supported PPC versions

	.set	PV601,   1
	.set	PV603,   3
	.set	PV604,   4
	.set	PV603p,  6
	.set	PV603pp, 7
	.set	PV613,   8
	.set	PV604p,  9
	.set	PV620,  20


// 601 special purpose register names
        .set    hid1,  1009

// special purpose register names (601, 603 and 604)
        .set    iabr,  1010

// special purpose register names (601, 604, 620)
        .set    dabr,  1013

        .extern KiBreakPoints

//++
//
// KiSaveProcessorControlState(
//       PKPROCESSOR_STATE   ProcessorState
//       );
//
// Routine Description:
//
//    This routine saves the control subset of the processor state.
//    Called by the debug subsystem, and KiSaveProcessorState()
//
//   N.B.  This procedure will save the debug registers and then turn off
//         the appropriate debug registers at the hardware.  This prevents
//         recursive hardware trace breakpoints and allows debuggers
//         to work.
//
// Arguments:
//
//   ProcessorState (r.3)
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiSaveProcessorControlState)

        lwz     r.5, [toc]KiBreakPoints(r.2)    // Available Breakpoints Addr
        addi    r.3, r.3, PsSpecialRegisters
        lwz     r.5, 0(r.5)                     // # available breakpoints
        lwz     r.7, SrKernelDr7(r.3)           // Get Dr state
        lwz     r.6, SrKernelDr6(r.3)
        rlwinm. r.7, r.7, 0, 0xff               // KD set Drs?
        or      r.5, r.5, r.6                   // Return # DRs in Dr6
        stw     r.5, SrKernelDr6(r.3)           // return allowed Drs
        beq     getsregs                        // Leave if no DR set

        mfpvr   r.4
        li      r.9, 0                          // Initialize Dr7
        li      r.8, 0                          // Turn off Drs
        rlwinm  r.4, r.4, 16, 0xffff            // isolate processor type
        cmpwi   r.4, PV604
        beq     ss.604                          // jif 604
        cmpwi   r.4, PV603p
        beq     ss.603                          // jif Stretch (603+)
        cmpwi   r.4, PV604p
        beq     ss.604                          // jif Sirocco (604+)
        cmpwi   r.4, PV603
        beq     ss.603                          // jif 603
        cmpwi   r.4, PV603pp
        beq     ss.603                          // jif 603++
        cmpwi   r.4, PV613
        beq     ss.604                          // jif 613
        cmpwi   r.4, PV620
        beq     ss.620                          // jif 620
        cmpwi   r.4, PV601
        li      r.10, 0x0080                    // Normal, run mode (601)
        beq     ss.601                          // jif 601
        stw     r.9, SrKernelDr7(r.3)           // Drs not supported
        b       getsregs                        // return
                                                // No DRs supported

ss.601:                                         // 601 SPECIFIC
        mtspr   hid1, r.10                      // turn off trace mode

ss.604:                                         // 601/604 SPECIFIC
        mfspr   r.4, iabr                       // Load the IABR (Dr0)
        rlwinm. r.4, r.4, 0, 0xfffffffc         // IABR(DR0) set?
        stw     r.4, SrKernelDr0(r.3)
        mfspr   r.4, dabr                       // Load the DABR (Dr1)
        beq     ssnoiabr.1                      // jiff Dr0 not set
        li      r.9, 0x2                        // Set GE0 in Dr7

ssnoiabr.1:
        rlwimi  r.9, r.4, 19, 11, 11            // Interchange R/W1 bits
        rlwimi  r.9, r.4, 21, 10, 10            // and move to Dr7
        rlwinm. r.4, r.4, 0, 0xfffffff8         // Sanitize Dr1
        stw     r.4, SrKernelDr1(r.3)           // Store Dr1 in trap frame
        beq     ssnodabr.1                      // jiff Dr1 not set
        ori     r.9, r.9, 0x8                   // Set GE1 in Dr7

ssnodabr.1:
        stw     r.9, SrKernelDr7(r.3)           // Initialize if no DRs set
        rlwinm. r.5, r.9, 0, 0xf                // Any DRs set?
        mtspr   dabr, r.8
        mtspr   iabr, r.8                       // Turn off DRs
        isync
        ori     r.9, r.9, 0x200                 // Set GE bit in Dr7
        beq     getsregs                        // exit if not set
        stw     r.9, SrKernelDr7(r.3)
        b       getsregs

ss.620:						// 620 SPECIFIC
        mfspr   r.4, dabr                       // Load the DABR (Dr1)
        rlwimi  r.9, r.4, 19, 11, 11            // Interchange R/W1 bits
        rlwimi  r.9, r.4, 21, 10, 10            // and move to Dr7
        rlwinm. r.4, r.4, 0, 0xfffffff8         // Sanitize Dr1
        stw     r.4, SrKernelDr1(r.3)           // Store Dr1 in trap frame
        beq     ssno620dabr.1                   // jif Dr1 not set
        ori     r.9, r.9, 0x8                   // Set GE1 in Dr7

ssno620dabr.1:
        stw     r.9, SrKernelDr7(r.3)           // Initialize if no DRs set
        rlwinm. r.5, r.9, 0, 0xf                // Any DRs set?
        mtspr   dabr, r.8
        isync
        ori     r.9, r.9, 0x200                 // Set GE bit in Dr7
        beq     getsregs                        // exit if not set
        stw     r.9, SrKernelDr7(r.3)
        b       getsregs

ss.603:                                         // 603 SPECIFIC
        mfspr   r.4, iabr                       // Load the IABR (Dr0)
        rlwinm. r.4, r.4, 0, 0xfffffffc         // Sanitize Dr0
        beq     ssnoiabr.3                      // jiff Dr0 not set
        li      r.9, 0x202                      // Initialize Dr7

ssnoiabr.3:
        stw     r.4, SrKernelDr0(r.3)           // Store Dr0
        stw     r.9, SrKernelDr7(r.3)
        mtspr   iabr, r.8                       // Turn off DRs

getsregs:
        mfsr    r.4, 0
        mfsr    r.5, 1
        mfsr    r.6, 2
        mfsr    r.7, 3
        stw     r.4, SrSr0(r.3)
        mfsr    r.4, 4
        stw     r.5, SrSr1(r.3)
        mfsr    r.5, 5
        stw     r.6, SrSr2(r.3)
        mfsr    r.6, 6
        stw     r.7, SrSr3(r.3)
        mfsr    r.7, 7
        stw     r.4, SrSr4(r.3)
        mfsr    r.4, 8
        stw     r.5, SrSr5(r.3)
        mfsr    r.5, 9
        stw     r.6, SrSr6(r.3)
        mfsr    r.6, 10
        stw     r.7, SrSr7(r.3)
        mfsr    r.7, 11
        stw     r.4, SrSr8(r.3)
        mfsr    r.4, 12
        stw     r.5, SrSr9(r.3)
        mfsr    r.5, 13
        stw     r.6, SrSr10(r.3)
        mfsr    r.6, 14
        stw     r.7, SrSr11(r.3)
        mfsr    r.7, 15
        stw     r.4, SrSr12(r.3)
        stw     r.5, SrSr13(r.3)
        stw     r.6, SrSr14(r.3)
        stw     r.7, SrSr15(r.3)

        mfsdr1  r.0
        stw     r.0, SrSdr1(r.3)

        mfibatl r.4, 0
        mfibatu r.5, 0
        mfibatl r.6, 1
        mfibatu r.7, 1
        stw     r.4, SrIBAT0L(r.3)
        mfibatl r.4, 2
        stw     r.5, SrIBAT0U(r.3)
        mfibatu r.5, 2
        stw     r.6, SrIBAT1L(r.3)
        mfibatl r.6, 3
        stw     r.7, SrIBAT1U(r.3)
        mfibatu r.7, 3
        stw     r.4, SrIBAT2L(r.3)
        stw     r.5, SrIBAT2U(r.3)
        stw     r.6, SrIBAT3L(r.3)
        stw     r.7, SrIBAT3U(r.3)

        mfpvr   r.4
        cmpwi   r.4, PV601
        beqlr                                   // exit if 601 (no DBATs)

        mfdbatl r.4, 0
        mfdbatu r.5, 0
        mfdbatl r.6, 1
        mfdbatu r.7, 1
        stw     r.4, SrDBAT0L(r.3)
        mfdbatl r.4, 2
        stw     r.5, SrDBAT0U(r.3)
        mfdbatu r.5, 2
        stw     r.6, SrDBAT1L(r.3)
        mfdbatl r.6, 3
        stw     r.7, SrDBAT1U(r.3)
        mfdbatu r.7, 3
        stw     r.4, SrDBAT2L(r.3)
        stw     r.5, SrDBAT2U(r.3)
        stw     r.6, SrDBAT3L(r.3)
        stw     r.7, SrDBAT3U(r.3)

        LEAF_EXIT(KiSaveProcessorControlState)


//++
//
// KiRestoreProcessorControlState(
//       );
//
// Routine Description:
//
//    This routine restores the control subset of the processor state.
//    (Restores the same information as KiRestoreProcessorState EXCEPT that
//     data in TrapFrame/ExceptionFrame=Context record is NOT restored.)
//    Called by the debug subsystem, and KiRestoreProcessorState()
//
// Arguments:
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRestoreProcessorControlState)

        addi    r.3, r.3, PsSpecialRegisters
        lwz     r.4, SrKernelDr7(r.3)           // Active DRs
        mfpvr   r.7
        rlwinm. r.6, r.4, 0, 0xff               // Drs Set?
        bne     DRon                            // jif DRs active
        blr                                     // exit No DRs set

DRon:
        rlwinm  r.7, r.7, 16, 0xffff            // isolate processor type
        lwz     r.5, SrKernelDr1 (r.3)          // Get kernel DABR (Dr1)
        lwz     r.6, SrKernelDr0 (r.3)          // Get kernel IABR (Dr0)
        ori     r.5, r.5, 0x4                   // Sanitize DABR (Dr1) 604
        ori     r.6, r.6, 0x3                   // Sanitize IABR (Dr0) 604
        cmpwi   r.7, PV604
        beq     rs.604                          // jif 604
        cmpwi   r.7, PV603p
        beq     rs.603                          // jif 603+
        cmpwi   r.7, PV604p
        beq     rs.604                          // jif 604+
        cmpwi   r.7, PV603
        beq     rs.603                          // jif 603
        cmpwi   r.7, PV603pp
        beq     rs.603                          // jif 603++
        cmpwi   r.7, PV613
        beq     rs.604                          // jif 613
        cmpwi   r.7, PV620
        beq     rs.604                          // jif 620
        cmpwi   r.7, PV601
        lis     r.10, 0x6080                    // Full cmp. trace mode (601)
        beq     rs.601                          // jif 601
        blr                                     // return
                                                // No DRs supported

rs.601:                                         // 601 SPECIFIC
        rlwinm  r.6, r.6, 0, 0xfffffffc         // Sanitze IABR (Dr0) undo 604
        rlwinm  r.5, r.5, 0, 0xfffffff8         // Sanitze DABR (Dr0) undo 604
        mtspr   hid1, r.10                      // turn on full cmp

rs.604:
        rlwinm. r.9, r.4, 0, 0x0000000c         // LE1/GE1 set?
        beq     rsnodabr.1                      // jiff Dr1 not set
        rlwimi  r.5, r.4, 13, 30, 30            // Interchange R/W1 bits
        rlwimi  r.5, r.4, 11, 31, 31
        mtspr   dabr, r.5

rsnodabr.1:
        rlwinm. r.4, r.4, 0, 0x00000003         // LE0/GE0 set?
        beqlr
        mtspr   iabr, r.6
        isync
        blr

rs.603:                                         // 603 SPECIFIC
        rlwinm  r.6, r.6, 0, 0xfffffffc         // Sanitize IABR
        ori     r.6, r.6, 0x2
        mtspr   iabr, r.6

        LEAF_EXIT(KiRestoreProcessorControlState)
