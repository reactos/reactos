/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    intapi.h

Abstract:

    This header defines the function prototypes for the interrupt
    handler support routines in the 486 emulator.

Author:

    Neil Sandlin (neilsa)

Notes:

    
Revision History:


--*/

NTSTATUS
VdmInstallHardwareIntHandler(
    PVOID HwIntHandler
    );

NTSTATUS
VdmInstallSoftwareIntHandler(
    PVOID SwIntHandler
    );

NTSTATUS
VdmInstallFaultHandler(
    PVOID FaultHandler
    );

