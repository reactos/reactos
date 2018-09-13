/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    VDM.C

Abstract:
    
    This routine has a stub for the x86 only api NtStartVdmExecution.

Author:

    Dave Hastings (daveh) 2 Apr 1991


Revision History:

--*/

#include "ki.h"



NTSTATUS
NtInitializeVDM(
    VOID
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
NtVdmStartExecution (
    )

/*++

Routine Description:
    
    This routine returns STATUS_NOT_IMPLEMENTED

Arguments:
    
Return Value:

    STATUS_NOT_IMPLEMENTED
--*/
{

    return STATUS_NOT_IMPLEMENTED;
    
}
