/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Engine floating point functions
 * FILE:              subsys/win32k/eng/float.c
 * PROGRAMER:         David Welch
 */

/* INCLUDES *****************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL
APIENTRY
EngRestoreFloatingPointState(
    PVOID Buffer)
{
    NTSTATUS Status;

    Status = KeRestoreFloatingPointState((PKFLOATING_SAVE)Buffer);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

ULONG
APIENTRY
EngSaveFloatingPointState(
    PVOID Buffer,
    ULONG BufferSize)
{
    KFLOATING_SAVE TempBuffer;
    NTSTATUS Status;

    if ((Buffer == NULL) || (BufferSize == 0))
    {
        /* Check for floating point support. */
        Status = KeSaveFloatingPointState(&TempBuffer);
        if (Status != STATUS_SUCCESS)
        {
            return(0);
        }

        KeRestoreFloatingPointState(&TempBuffer);
        return(sizeof(KFLOATING_SAVE));
    }

    if (BufferSize < sizeof(KFLOATING_SAVE))
    {
        return(0);
    }

    Status = KeSaveFloatingPointState((PKFLOATING_SAVE)Buffer);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

