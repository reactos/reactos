/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/vdm.c
 * PURPOSE:         Virtual DOS Machines (VDM) Support
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *                  Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "basesrv.h"
#include "vdm.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

BOOLEAN FirstVDM = TRUE;
LIST_ENTRY VDMConsoleListHead;
RTL_CRITICAL_SECTION DosCriticalSection;

/* FUNCTIONS ******************************************************************/

NTSTATUS NTAPI BaseSrvGetConsoleRecord(HANDLE ConsoleHandle, PVDM_CONSOLE_RECORD *Record)
{
    PLIST_ENTRY i;
    PVDM_CONSOLE_RECORD CurrentRecord = NULL;

    /* Search for a record that has the same console handle */
    for (i = VDMConsoleListHead.Flink; i != &VDMConsoleListHead; i = i->Flink)
    {
        CurrentRecord = CONTAINING_RECORD(i, VDM_CONSOLE_RECORD, Entry);
        if (CurrentRecord->ConsoleHandle == ConsoleHandle) break;
    }

    *Record = CurrentRecord;
    return CurrentRecord ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

VOID NTAPI BaseInitializeVDM(VOID)
{
    /* Initialize the list head */
    InitializeListHead(&VDMConsoleListHead);

    /* Initialize the critical section */
    RtlInitializeCriticalSection(&DosCriticalSection);
}

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(BaseSrvCheckVDM)
{
    PBASE_CHECK_VDM CheckVdmRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.CheckVDMRequest;

    /* Validate the message buffers */
    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&CheckVdmRequest->CmdLine,
                                  CheckVdmRequest->CmdLen,
                                  sizeof(*CheckVdmRequest->CmdLine))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->AppName,
                                     CheckVdmRequest->AppLen,
                                     sizeof(*CheckVdmRequest->AppName))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->PifFile,
                                     CheckVdmRequest->PifLen,
                                     sizeof(*CheckVdmRequest->PifFile))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->CurDirectory,
                                     CheckVdmRequest->CurDirectoryLen,
                                     sizeof(*CheckVdmRequest->CurDirectory))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->Desktop,
                                     CheckVdmRequest->DesktopLen,
                                     sizeof(*CheckVdmRequest->Desktop))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->Title,
                                     CheckVdmRequest->TitleLen,
                                     sizeof(*CheckVdmRequest->Title))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&CheckVdmRequest->Reserved,
                                     CheckVdmRequest->ReservedLen,
                                     sizeof(*CheckVdmRequest->Reserved)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // TODO: NOT IMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvUpdateVDMEntry)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvGetNextVDMCommand)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvExitVDM)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvIsFirstVDM)
{
    PBASE_IS_FIRST_VDM IsFirstVDMRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.IsFirstVDMRequest;

    /* Return the result */
    IsFirstVDMRequest->FirstVDM = FirstVDM;

    /* Clear the first VDM flag */
    FirstVDM = FALSE;

    return STATUS_SUCCESS;
}

CSR_API(BaseSrvGetVDMExitCode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvSetReenterCount)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvSetVDMCurDirs)
{
    NTSTATUS Status;
    PBASE_GETSET_VDM_CURDIRS VDMCurrentDirsRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.VDMCurrentDirsRequest;
    PVDM_CONSOLE_RECORD ConsoleRecord;
    PCHAR Buffer = NULL;

    /* Validate the input buffer */
    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&VDMCurrentDirsRequest->lpszzCurDirs,
                                  VDMCurrentDirsRequest->cchCurDirs,
                                  sizeof(*VDMCurrentDirsRequest->lpszzCurDirs)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Enter the critical section */
    RtlEnterCriticalSection(&DosCriticalSection);

    /* Find the console record */
    Status = BaseSrvGetConsoleRecord(VDMCurrentDirsRequest->ConsoleHandle, &ConsoleRecord);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    if (ConsoleRecord->CurrentDirs == NULL)
    {
        /* Allocate memory for the current directory information */
        Buffer = RtlAllocateHeap(BaseSrvHeap,
                                 HEAP_ZERO_MEMORY,
                                 VDMCurrentDirsRequest->cchCurDirs);
    }
    else
    {
        /* Resize the amount of allocated memory */
        Buffer = RtlReAllocateHeap(BaseSrvHeap,
                                   HEAP_ZERO_MEMORY,
                                   ConsoleRecord->CurrentDirs,
                                   VDMCurrentDirsRequest->cchCurDirs);
    }

    if (Buffer == NULL)
    {
        /* Allocation failed */
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    /* Update the console record */
    ConsoleRecord->CurrentDirs = Buffer;
    ConsoleRecord->CurDirsLength = VDMCurrentDirsRequest->cchCurDirs;

    /* Copy the data */
    RtlMoveMemory(ConsoleRecord->CurrentDirs,
                  VDMCurrentDirsRequest->lpszzCurDirs,
                  VDMCurrentDirsRequest->cchCurDirs);

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(&DosCriticalSection);

    return Status;
}

CSR_API(BaseSrvGetVDMCurDirs)
{
    NTSTATUS Status;
    PBASE_GETSET_VDM_CURDIRS VDMCurrentDirsRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.VDMCurrentDirsRequest;
    PVDM_CONSOLE_RECORD ConsoleRecord;

    /* Validate the output buffer */
    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&VDMCurrentDirsRequest->lpszzCurDirs,
                                  VDMCurrentDirsRequest->cchCurDirs,
                                  sizeof(*VDMCurrentDirsRequest->lpszzCurDirs)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Enter the critical section */
    RtlEnterCriticalSection(&DosCriticalSection);

    /* Find the console record */
    Status = BaseSrvGetConsoleRecord(VDMCurrentDirsRequest->ConsoleHandle, &ConsoleRecord);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Check if the buffer is large enough */
    if (VDMCurrentDirsRequest->cchCurDirs < ConsoleRecord->CurDirsLength)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Cleanup;
    }

    /* Copy the data */
    RtlMoveMemory(VDMCurrentDirsRequest->lpszzCurDirs,
                  ConsoleRecord->CurrentDirs,
                  ConsoleRecord->CurDirsLength);

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(&DosCriticalSection);

    return Status;
}

CSR_API(BaseSrvBatNotification)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvRegisterWowExec)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvRefreshIniFileMapping)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
