/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ldrctx.c

Abstract:

    This module contains support for relocating executables.

Author:

    Landy Wang (landyw) 8-Jul-1998

Environment:

    User Mode only

Revision History:

--*/

#include <ntos.h>

VOID
LdrpRelocateStartContext (
    IN PCONTEXT Context,
    IN LONG_PTR Diff
    )
/*++

Routine Description:

   This routine relocates the start context to mesh with the
   executable that has just been relocated.

Arguments:

   Context - Supplies a context that needs editing.

   Diff - Supplies the difference from the based address to the relocated
          address.

Return Value:

   None.

--*/
{
    Context->IntS1 += (ULONGLONG)Diff;
}
