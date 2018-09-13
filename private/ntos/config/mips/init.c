/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    init.c

Abstract:

    This module is responsible to build any mips specific entries in
    the hardware tree of registry which the arc environment doesn't
    normally provide for.

Author:

    Ken Reneris (kenr) 04-Aug-1992


Environment:

    Kernel mode.

Revision History:

--*/

#include "cmp.h"


NTSTATUS
CmpInitializeMachineDependentConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This routine creates mips specific entries in the registry.

Arguments:

    LoaderBlock - supplies a pointer to the LoaderBlock passed in from the
                  OS Loader.

Returns:

    NTSTATUS code for sucess or reason of failure.

--*/
{
    return STATUS_SUCCESS;
}
