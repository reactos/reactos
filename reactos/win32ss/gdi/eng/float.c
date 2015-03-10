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

    Status = KeRestoreFloatingPointState((PKFLOATING_SAVE)pBuffer);
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
    KFLOATING_SAVE TempBuffer;
    NTSTATUS Status;

    if ((pBuffer == NULL) || (cjBufferSize == 0))
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

    if (cjBufferSize < sizeof(KFLOATING_SAVE))
    {
        return(0);
    }

    Status = KeSaveFloatingPointState((PKFLOATING_SAVE)pBuffer);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

