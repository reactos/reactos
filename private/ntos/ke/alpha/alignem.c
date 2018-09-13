/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

    alignem.c

Abstract:

    This module implements the code necessary to emulate unaligned data
    references.

Author:

    David N. Cutler (davec) 17-Jun-1991
    Joe Notarangelo         14-May-1992

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Function prototypes for emulation routines
//
ULONGLONG
KiEmulateLoadLong(
   IN PULONG UnalignedAddress
   );

ULONGLONG
KiEmulateLoadQuad(
   IN PUQUAD UnalignedAddress
   );

ULONGLONG
KiEmulateLoadFloatIEEESingle(
   IN PULONG UnalignedAddress
   );

ULONGLONG
KiEmulateLoadFloatIEEEDouble(
   IN PUQUAD UnalignedAddress
   );

VOID
KiEmulateStoreLong(
   IN PULONG UnalignedAddress,
   IN ULONGLONG  Data
   );

VOID
KiEmulateStoreQuad(
   IN PUQUAD UnalignedAddress,
   IN ULONGLONG Data
   );

VOID
KiEmulateStoreFloatIEEESingle(
   IN PULONG UnalignedAddress,
   IN ULONGLONG  Data
   );

VOID
KiEmulateStoreFloatIEEEDouble(
   IN PUQUAD UnalignedAddress,
   IN ULONGLONG  Data
   );

VOID
KiEnablePALAlignmentFixups(
    VOID
    );

VOID
KiDisablePALAlignmentFixups(
    VOID
    );

VOID
KiProfileInterrupt(
   IN KPROFILE_SOURCE ProfileSource,
   IN PKTRAP_FRAME TrapFrame
   );


VOID
KiEnableAlignmentExceptions(
    VOID
    )
/*++

Routine Description:

    Enables alignment exceptions on the current processor by
    disabling automatic PAL code alignment fixups. PAL is
    called to turn off automatic fixups only if CPU is
    21164 or greater.

Arguments:

    None

Return Value:

    None

--*/

{
    if (KeProcessorLevel >= PROCESSOR_ALPHA_21164) {
        KiDisablePALAlignmentFixups();
    }
}


VOID
KiDisableAlignmentExceptions(
    VOID
    )
/*++

Routine Description:

    Disables alignment exceptions on the current processor by
    enabling automatic PAL code alignment fixups. PAL is
    called to turn on automatic fixups only if CPU is
    21164 or greater and KiEnableAlignmentFaultExceptions==0

    If KiEnableAlignmentFaultExceptions is either 1 or 2, then
    the kernel always needs to see alignment faults, so PAL
    automatic alignment fixups should not be enabled

Arguments:

    None

Return Value:

    None

--*/

{
    if ((KeProcessorLevel >= PROCESSOR_ALPHA_21164) &&
        (KiEnableAlignmentFaultExceptions == 0)) {
        KiEnablePALAlignmentFixups();
    }
}


BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME  ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame,
    IN BOOLEAN QuadwordOnly
    )

/*++

Routine Description:

    Routine emulates an unaligned data reference from user part
    of the address space.

Arguments:

    ExceptionRecord - Supplies a pointer to the exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame

    QuadwordOnly - Supplies a boolean which controls whether both longword
        and quadword references are to be emulated or quadword references
        only.

Return Value:

    True is returned if reference is successfully emulated,
    otherwise False is returned.

--*/

{

    ULONGLONG Data;
    PVOID  EffectiveAddress;
    PVOID  ExceptionAddress;
    ULONG  Fa;
    ULONG  Opcode;
    KPROCESSOR_MODE PreviousMode;
    ULONG  Ra;
    KIRQL  OldIrql;

    //
    // Call out to profile interrupt if alignment profiling is active
    //
    if (KiProfileAlignmentFixup) {

        if (++KiProfileAlignmentFixupCount >= KiProfileAlignmentFixupInterval) {

            KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
            KiProfileAlignmentFixupCount = 0;
            KiProfileInterrupt(ProfileAlignmentFixup, TrapFrame);
            KeLowerIrql(OldIrql);

        }
    }

    //
    // Save original exception address in case another exception occurs
    //

    ExceptionAddress = ExceptionRecord->ExceptionAddress;

    //
    // The ExceptionInformation in the ExceptionRecord has already
    // recorded information we need to emulate the access.
    //
    // ExceptionInformation:
    //                [0]   =  opcode
    //                [1]   =  destination register
    //                [2]   =  effective address of access

    Opcode = (ULONG)ExceptionRecord->ExceptionInformation[0];
    Ra = (ULONG)ExceptionRecord->ExceptionInformation[1];
    Fa = Ra + 32;     // convert to floating register name for floating opcodes
    EffectiveAddress = (PVOID)ExceptionRecord->ExceptionInformation[2];

    //
    // Capture previous mode from trap frame not current thread.
    //

    PreviousMode = (KPROCESSOR_MODE)(((PSR *)(&TrapFrame->Psr))->MODE);

    //
    // Any exception that occurs during the attempted emulation will cause
    // the emulation to be aborted.  The new exception code and information
    // will be copied to the original exception record and FALSE will be
    // returned.  If the unaligned access was not from kernel mode then
    // probe the effective address before performing the emulation.
    //

    try {

        switch (Opcode) {

        //
        // load longword
        //

        case LDL_OP:
            if (QuadwordOnly != FALSE) {
                return FALSE;
            }
            if( PreviousMode != KernelMode ){
                ProbeForRead( EffectiveAddress,
                              sizeof(LONG),
                              sizeof(UCHAR) );
            }
            Data = KiEmulateLoadLong( EffectiveAddress );
            KiSetRegisterValue( Ra,
                                Data,
                                ExceptionFrame,
                                TrapFrame );

            break;

        //
        // load quadword
        //

        case LDQ_OP:
            if( PreviousMode != KernelMode ){
                ProbeForRead( EffectiveAddress,
                              sizeof(LONGLONG),
                              sizeof(UCHAR) );
            }
            Data = KiEmulateLoadQuad( EffectiveAddress );
            KiSetRegisterValue( Ra,
                                Data,
                                ExceptionFrame,
                                TrapFrame );

            break;

        //
        // load IEEE single float
        //

        case LDS_OP:
            if (QuadwordOnly != FALSE) {
                return FALSE;
            }
            if( PreviousMode != KernelMode ){
                ProbeForRead( EffectiveAddress,
                              sizeof(float),
                              sizeof(UCHAR) );
            }
            Data = KiEmulateLoadFloatIEEESingle( EffectiveAddress );
            KiSetRegisterValue( Fa,
                                Data,
                                ExceptionFrame,
                                TrapFrame );

            break;

        //
        // load IEEE double float
        //

        case LDT_OP:
            if( PreviousMode != KernelMode ){
                ProbeForRead( EffectiveAddress,
                              sizeof(DOUBLE),
                              sizeof(UCHAR) );
            }
            Data = KiEmulateLoadFloatIEEEDouble( EffectiveAddress );
            KiSetRegisterValue( Fa,
                                Data,
                                ExceptionFrame,
                                TrapFrame );

            break;

        //
        // Load word unsigned.
        //

        case LDWU_OP :
            if (QuadwordOnly != FALSE) {
                return FALSE;
            }
            if (PreviousMode != KernelMode) {
                ProbeForRead(EffectiveAddress,
                             sizeof(SHORT),
                             sizeof(UCHAR));
            }
            Data = (ULONGLONG)*(UNALIGNED USHORT *)EffectiveAddress;
            KiSetRegisterValue(Ra,
                               Data,
                               ExceptionFrame,
                               TrapFrame);

            break;

        //
        // store longword
        //

        case STL_OP:
            if (QuadwordOnly != FALSE) {
                return FALSE;
            }
            if( PreviousMode != KernelMode ){
                ProbeForWrite( EffectiveAddress,
                               sizeof(LONG),
                               sizeof(UCHAR) );
            }
            Data = KiGetRegisterValue( Ra,
                                       ExceptionFrame,
                                       TrapFrame );
            KiEmulateStoreLong( EffectiveAddress, (ULONG)Data );

            break;

        //
        // store quadword
        //

        case STQ_OP:
            if( PreviousMode != KernelMode ){
                ProbeForWrite( EffectiveAddress,
                               sizeof(LONGLONG),
                               sizeof(UCHAR) );
            }
            Data = KiGetRegisterValue( Ra,
                                       ExceptionFrame,
                                       TrapFrame );
            KiEmulateStoreQuad( EffectiveAddress, Data );

            break;

        //
        // store IEEE float single
        //

        case STS_OP:
            if (QuadwordOnly != FALSE) {
                return FALSE;
            }
            if( PreviousMode != KernelMode ){
                ProbeForWrite( EffectiveAddress,
                               sizeof(float),
                               sizeof(UCHAR) );
            }
            Data = KiGetRegisterValue( Fa,
                                       ExceptionFrame,
                                       TrapFrame );
            KiEmulateStoreFloatIEEESingle( EffectiveAddress, Data );

            break;

        //
        // store IEEE float double
        //

        case STT_OP:
            if( PreviousMode != KernelMode ){
                ProbeForWrite( EffectiveAddress,
                               sizeof(DOUBLE),
                               sizeof(UCHAR) );
            }
            Data = KiGetRegisterValue( Fa,
                                       ExceptionFrame,
                                       TrapFrame );
            KiEmulateStoreFloatIEEEDouble( EffectiveAddress, Data );

            break;

        //
        // Store word.
        //

        case STW_OP :
            if (QuadwordOnly != FALSE) {
                return FALSE;
            }
            if (PreviousMode != KernelMode) {
                ProbeForWrite(EffectiveAddress,
                              sizeof(SHORT),
                              sizeof(UCHAR));
            }
            Data = KiGetRegisterValue(Ra,
                                      ExceptionFrame,
                                      TrapFrame);
            *(UNALIGNED USHORT *)EffectiveAddress = (USHORT)Data;

            break;

        //
        // all other instructions are not emulated
        //

        default:
            return FALSE;
        }

        TrapFrame->Fir += 4;
        return TRUE;

    } except (KiCopyInformation(ExceptionRecord,
                                (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Preserve the original exception address
        //

        ExceptionRecord->ExceptionAddress = ExceptionAddress;

        return FALSE;
    }
}
