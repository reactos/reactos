/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Engine floating point functions
 * FILE:              win32ss/gdi/eng/float.c
 * PROGRAMER:         David Welch
 */

/* INCLUDES *****************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

typedef struct _WIN32K_FLOATING_SAVE
{
    KFLOATING_SAVE FloatState;
    BOOLEAN IsFloatingPointSaved;
} WIN32K_FLOATING_SAVE, *PWIN32K_FLOATING_SAVE;

/* FUNCTIONS *****************************************************************/

#ifdef _PREFAST_
#pragma warning(disable:__WARNING_WRONG_KIND)
#endif

_Check_return_
_Success_(return)
_Kernel_float_restored_
_At_(*pBuffer, _Kernel_requires_resource_held_(EngFloatState)
               _Kernel_releases_resource_(EngFloatState))
ENGAPI
BOOL
APIENTRY
EngRestoreFloatingPointState(
    _In_reads_(_Inexpressible_(statesize)) PVOID pBuffer)
{
    NTSTATUS Status;
    PWIN32K_FLOATING_SAVE State = (PWIN32K_FLOATING_SAVE)pBuffer;

    if (!State->IsFloatingPointSaved)
    {
        DPRINT1("The driver has attempted to restore floating point state after already restoring it.\n");
        DPRINT1("This (probably ICafe AMD) driver has done an incorrect behavior.\n");
        return FALSE;
    }

    State->IsFloatingPointSaved = FALSE;

    Status = KeRestoreFloatingPointState(&State->FloatState);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

_Check_return_
_Success_(((pBuffer != NULL && cjBufferSize != 0) && return == 1) ||
          ((pBuffer == NULL || cjBufferSize == 0) && return > 0))
_When_(pBuffer != NULL && cjBufferSize != 0 && return == 1, _Kernel_float_saved_
    _At_(*pBuffer, _Post_valid_ _Kernel_acquires_resource_(EngFloatState)))
_On_failure_(_Post_satisfies_(return == 0))
ENGAPI
ULONG
APIENTRY
EngSaveFloatingPointState(
    _At_(*pBuffer, _Kernel_requires_resource_not_held_(EngFloatState))
    _Out_writes_bytes_opt_(cjBufferSize) PVOID pBuffer,
    _Inout_ ULONG cjBufferSize)
{
    PWIN32K_FLOATING_SAVE State;
    NTSTATUS Status;

    if ((pBuffer == NULL) || (cjBufferSize == 0))
    {
        KFLOATING_SAVE TempBuffer;

        /* Check for floating point support. */
        Status = KeSaveFloatingPointState(&TempBuffer);
        if (Status != STATUS_SUCCESS)
        {
            return(0);
        }
        KeRestoreFloatingPointState(&TempBuffer);
        return sizeof(WIN32K_FLOATING_SAVE);
    }

    if (cjBufferSize < sizeof(WIN32K_FLOATING_SAVE))
    {
        return(0);
    }

    /* Per MSDN, "This buffer must be zero-initialized, and must be in nonpaged memory." */
    State = (PWIN32K_FLOATING_SAVE)pBuffer;

    if (State->IsFloatingPointSaved)
    {
        DPRINT1("The driver has attempted to save floating point state after already saving it.\n");
        DPRINT1("This (probably ICafe AMD) driver has done an incorrect behavior.\n");
    }

    Status = KeSaveFloatingPointState(&State->FloatState);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    State->IsFloatingPointSaved = TRUE;
    return TRUE;
}

