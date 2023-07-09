//      TITLE("Processor Control Registers")
//++
//
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    kdppcr.s
//
// Abstract:
//
//    This module implements the code necessary to access the
//    processor control registers (pcr) on an alpha processor and
//    the routines that request internal processor information via
//    call pals.
//
//    On mips processors the pcr (which contains processor-specific data)
//    was mapped in the virtual address space using a fixed tb entry.
//    For alpha, we don't have fixed tb entries so we will get pcr data
//    via routine interfaces that will vary depending upon whether we are
//    on a multi- or uni-processor system..
//
//
// N.B.
//
// *************************************************************************
//
//      Most of the functions in this file are cloned from ntos\ke\alpha\pcr.s.
//      Any changes to the common functions must be made in both places.
//
// *************************************************************************
//
// Author:
//
//    Joe Notarangelo 15-Apr-1992
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

//++
//
// KIRQL
// KdpGetCurrentIrql(
//      VOID
//      )
//
// Routine Description:
//
//    This function returns the current irql of the processor.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Current processor irql.
//
//--

        LEAF_ENTRY(KdpGetCurrentIrql)


        GET_CURRENT_IRQL                // v0 = current irql

        ret     zero, (ra)              // return


        .end KdpGetCurrentIrql



//++
//
// PPRCB
// KdpGetCurrentPrcb
//      VOID
//      )
//
// Routine Description:
//
//    This function returns the current processor control block for this
//      processor.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Pointer to current processor's prcb.
//
//--

        LEAF_ENTRY(KdpGetCurrentPrcb)


        GET_PROCESSOR_CONTROL_BLOCK_BASE // v0 = prcb base

        ret     zero, (ra)              // return


        .end KdpGetCurrentPrcb



//++
//
// PKTHREAD
// KdpGetCurrentThread
//      VOID
//      )
//
// Routine Description:
//
//    This function return the current thread running on this processor.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Pointer to current thread.
//
//--

        LEAF_ENTRY(KdpGetCurrentThread)


        GET_CURRENT_THREAD              // v0 = current thread address

        ret     zero, (ra)              // return


        .end KdpGetCurrentThread


//++
//
// PKPCR
// KdpGetPcr(
//      VOID
//      )
//
// Routine Description:
//
//    This function returns the base address of the processor control
//    region for the current processor.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Pointer to current thread executing on this processor.
//
//--

        LEAF_ENTRY(KdpGetPcr)


        GET_PROCESSOR_CONTROL_REGION_BASE // v0 = pcr base address

        ret     zero, (ra)              // return

        .end KdpGetPcr


//++
//
// ULONG
// KdpReadInternalProcessorState(
//     PVOID Buffer,
//     ULONG BufferLength
//     )
//
// Routine Description:
//
//    This function implements a call to the PALcode to read the
//    internal processor state.
//
// Arguments:
//
//    Buffer(a0) - Supplies a quadword aligned pointer to the buffer
//                 to receive the state data.
//
//    BufferLength(a1) - Supplies the size of the buffer in bytes.
//
//
// Return Value:
//
//    (v0) - The size of the state data written into the buffer is
//           returned.  If the buffer was not sufficiently large to
//           contain the state data then the size of the state data record
//           will be returned.
//
//--

        LEAF_ENTRY(KdpReadInternalProcessorState)


        call_pal rdstate                // read the internal processor state

        ret     zero, (ra)              // return

        .end    KdpReadInternalProcessorState


//++
//
// ULONG
// KdpReadInternalProcessorCounters(
//     PVOID Buffer,
//     ULONG BufferLength
//     )
//
// Routine Description:
//
//    This function implements a call to the PALcode to read the
//    internal processor counters.
//
// Arguments:
//
//    Buffer(a0) - Supplies a quadword aligned pointer to the buffer
//                 to receive the counter values.
//
//    BufferLength(a1) - Supplies the size of the buffer in bytes.
//
//
// Return Value:
//
//    (v0) - The size of the state data written into the buffer is
//           returned.  If the buffer was not sufficiently large to
//           contain the counter data then the size of the counter data record
//           will be returned.
//
//--

        LEAF_ENTRY(KdpReadInternalProcessorCounters)


        call_pal rdcounters             // read the internal processor state

        ret     zero, (ra)              // return

        .end    KdpReadInternalProcessorCounters

