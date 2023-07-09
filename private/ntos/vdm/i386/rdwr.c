/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    rdwr.c

Abstract:

    This module contains routines for read and write for NTDOS. These
    routines saves the switch to user mode. The BOP is handled in the
    kernel for performance reasons. These routines are called only for
    files. Local DOS devices and named pipe operations never come here.

Author:

    Sudeep Bharati (Sudeepb) 04-Mar-1993

Revision History:

    04-Mar-1993 sudeepb Created
--*/
#include "vdmp.h"

VOID
NTFastDOSIO (
    PKTRAP_FRAME TrapFrame,
    ULONG IoType
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NTFastDOSIO)
#endif

#define EFLAGS_CF               0x1
#define EFLAGS_ZF               0x40
#define GETFILEPOINTER(hi,lo)   (((ULONG)hi << 16) + (ULONG)lo)
#define GETHANDLE(hi,lo)        (HANDLE)(((ULONG)hi << 16) + (ULONG)lo)
#define GETBUFFER(hi,lo)        (((ULONG)hi << 4) + lo)
#define SVC_DEMFASTREAD         0x42
#define SVC_DEMFASTWRITE        0x43
#define CONSOLE_HANDLE_SIGNATURE 0x00000003
#define CONSOLE_HANDLE(HANDLE) (((ULONG)(HANDLE) & CONSOLE_HANDLE_SIGNATURE) == CONSOLE_HANDLE_SIGNATURE)
#define STD_INPUT_HANDLE       (ULONG)-10
#define STD_OUTPUT_HANDLE      (ULONG)-11
#define STD_ERROR_HANDLE       (ULONG)-12

VOID
NTFastDOSIO (
    PKTRAP_FRAME TrapFrame,
    ULONG IoType
    )
{
    HANDLE hFile;
    PVOID  lpBuf;
    ULONG  ulBX,ulSI;
    LARGE_INTEGER Large;
    PIO_STATUS_BLOCK IoStatusBlock;
    PFILE_POSITION_INFORMATION CurrentPosition;
    NTSTATUS Status;
    ULONG CountToIO;
    PFILE_END_OF_FILE_INFORMATION EndOfFile;
    PVDM_TIB VdmTib;
    KIRQL OldIrql;

    PAGED_CODE();

    // signal softpc that we are doing disk io for idle detection
    *pNtVDMState |= VDM_IDLEACTIVITY;

    Status = VdmpGetVdmTib(&VdmTib, VDMTIB_KMODE); // no probe, valid user-mode
                                                   // address there
    if (!NT_SUCCESS(Status)) { // vdmtib is bad
       TrapFrame->EFlags |= EFLAGS_CF;
       return;
    }

    IoStatusBlock = (PIO_STATUS_BLOCK) &VdmTib->TempArea1;
    CurrentPosition = (PFILE_POSITION_INFORMATION) &VdmTib->TempArea2;
    EndOfFile = (PFILE_END_OF_FILE_INFORMATION) CurrentPosition;

    // Get the NT handle
    hFile = GETHANDLE((TrapFrame->Eax & 0x0000ffff),(TrapFrame->Ebp & 0x0000ffff));

    // advance ip past the bop instruction
    // clear carry flag, assuming success
    TrapFrame->Eip += 4;
    TrapFrame->EFlags &= ~EFLAGS_CF;

    if (CONSOLE_HANDLE(hFile) ||
        hFile == (HANDLE) STD_INPUT_HANDLE ||
        hFile == (HANDLE) STD_OUTPUT_HANDLE ||
        hFile == (HANDLE) STD_ERROR_HANDLE )
      {
        TrapFrame->EFlags |= EFLAGS_CF;
        return;
    }

    // Get the IO buffer
    lpBuf = (PVOID) GETBUFFER(TrapFrame->V86Ds, (TrapFrame->Edx & 0x0000ffff));

    // Get the Count
    CountToIO = TrapFrame->Ecx & 0x0000ffff;

    // Get Seek Parameters
    ulBX = TrapFrame->Ebx & 0x0000ffff;
    ulSI = TrapFrame->Esi & 0x0000ffff;


    // Lower Irql to passive level for io system
    OldIrql = KeGetCurrentIrql();
    KeLowerIrql(PASSIVE_LEVEL);

    try {

    // Check if we need to seek
    if (!(TrapFrame->EFlags & EFLAGS_ZF)) {
        Large = RtlConvertUlongToLargeInteger(GETFILEPOINTER(ulBX,ulSI));
        CurrentPosition->CurrentByteOffset = Large;

        Status = NtSetInformationFile(
                hFile,
                IoStatusBlock,
                CurrentPosition,
                sizeof(FILE_POSITION_INFORMATION),
                FilePositionInformation
                );
        if (!NT_SUCCESS(Status) ||
            CurrentPosition->CurrentByteOffset.LowPart == -1 )
          {
            goto ErrorExit;
          }
    }

    if (IoType == SVC_DEMFASTREAD){
        Status = NtReadFile(
                hFile,
                NULL,
                NULL,
                NULL,
                IoStatusBlock,
                (PVOID)lpBuf,
                CountToIO,
                NULL,
                NULL
                );
    }
    else{
        if (CountToIO == 0) {
            Status = NtQueryInformationFile(
                        hFile,
                        IoStatusBlock,
                        CurrentPosition,
                        sizeof(FILE_POSITION_INFORMATION),
                        FilePositionInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                goto ErrorExit;
            }

            EndOfFile->EndOfFile = CurrentPosition->CurrentByteOffset;

            Status = NtSetInformationFile(
                        hFile,
                        IoStatusBlock,
                        EndOfFile,
                        sizeof(FILE_END_OF_FILE_INFORMATION),
                        FileEndOfFileInformation
                        );
            if ( NT_SUCCESS(Status) ){
                KeRaiseIrql(OldIrql, &OldIrql);
                return;
            }

            goto ErrorExit;
        }
        else {
            Status = NtWriteFile(
                    hFile,
                    NULL,
                    NULL,
                    NULL,
                    IoStatusBlock,
                    (PVOID)lpBuf,
                    CountToIO,
                    NULL,
                    NULL
                    );
        }
    }

    if ( Status == STATUS_PENDING) {
        // Operation must complete before return & IoStatusBlock destroyed
        Status = NtWaitForSingleObject( hFile, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {
            Status = IoStatusBlock->Status;
        }
    }

    }
    except(ExSystemExceptionFilter()) {
       goto ErrorExit; // we have caught an exception, error exit
    }


    KeRaiseIrql(OldIrql, &OldIrql);

    if ( NT_SUCCESS(Status) ) {
        TrapFrame->Eax &= 0xffff0000;
        TrapFrame->Eax |= (USHORT) IoStatusBlock->Information;
    }
    else if (IoType == SVC_DEMFASTREAD && Status == STATUS_END_OF_FILE) {
        TrapFrame->Eax &= 0xffff0000;
    }
    else {
        TrapFrame->EFlags |= EFLAGS_CF;
    }

    return;


ErrorExit:
    KeRaiseIrql(OldIrql, &OldIrql);
    TrapFrame->EFlags |= EFLAGS_CF;

    return;
}
