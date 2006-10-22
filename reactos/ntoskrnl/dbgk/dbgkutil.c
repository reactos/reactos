/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/dbgk/dbgkutil.c
 * PURPOSE:         User-Mode Debugging Support, Internal Debug Functions.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

HANDLE
NTAPI
DbgkpSectionToFileHandle(IN PVOID Section)
{
    NTSTATUS Status;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    PAGED_CODE();

    /* Get the filename of the section */
    Status = MmGetFileNameForSection(Section, &FileName);
    if (!NT_SUCCESS(Status)) return NULL;

    /* Initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the file */
    Status = ZwOpenFile(&Handle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    /* Free the name and return the handle if we succeeded */
    ExFreePool(FileName.Buffer);
    if (!NT_SUCCESS(Status)) return NULL;
    return Handle;
}

BOOLEAN
NTAPI
DbgkpSuspendProcess(VOID)
{
    PAGED_CODE();

    /* Make sure this isn't a deleted process */
    if (PsGetCurrentProcess()->ProcessDelete)
    {
        /* Freeze all the threads */
        KeFreezeAllThreads();
        return TRUE;
    }
    else
    {
        /* No suspend was done */
        return FALSE;
    }
}

VOID
NTAPI
DbgkpResumeProcess(VOID)
{
    PAGED_CODE();

    /* Thaw all the threads */
    KeThawAllThreads();
}

VOID
NTAPI
DbgkCreateThread(PVOID StartAddress)
{
    /* FIXME */
}

VOID
NTAPI
DbgkExitProcess(IN NTSTATUS ExitStatus)
{
    DBGKM_MSG ApiMessage;
    PDBGKM_EXIT_PROCESS ExitProcess = &ApiMessage.ExitProcess;
    PEPROCESS Process = PsGetCurrentProcess();
    PETHREAD Thread = PsGetCurrentThread();
    PAGED_CODE();

    /* Check if this thread is hidden, doesn't have a debug port, or died */
    if ((Thread->HideFromDebugger) ||
        !(Process->DebugPort) ||
        (Thread->DeadThread))
    {
        /* Don't notify the debugger */
        return;
    }

    /* Set the exit status */
    ExitProcess->ExitStatus = ExitStatus;

    /* Setup the API Message */
    ApiMessage.h.u1.Length = sizeof(DBGKM_MSG) << 16 |
                             (8 + sizeof(DBGKM_EXIT_PROCESS));
    ApiMessage.h.u2.ZeroInit = LPC_DEBUG_EVENT;
    ApiMessage.ApiNumber = DbgKmExitProcessApi;

    /* Set the current exit time */
    KeQuerySystemTime(&Process->ExitTime);

    /* Send the message */
    DbgkpSendApiMessage(&ApiMessage, FALSE);
}

VOID
NTAPI
DbgkExitThread(IN NTSTATUS ExitStatus)
{
    DBGKM_MSG ApiMessage;
    PDBGKM_EXIT_THREAD ExitThread = &ApiMessage.ExitThread;
    PEPROCESS Process = PsGetCurrentProcess();
    PETHREAD Thread = PsGetCurrentThread();
    BOOLEAN Suspended;
    PAGED_CODE();

    /* Check if this thread is hidden, doesn't have a debug port, or died */
    if ((Thread->HideFromDebugger) ||
        !(Process->DebugPort) ||
        (Thread->DeadThread))
    {
        /* Don't notify the debugger */
        return;
    }

    /* Set the exit status */
    ExitThread->ExitStatus = ExitStatus;

    /* Setup the API Message */
    ApiMessage.h.u1.Length = sizeof(DBGKM_MSG) << 16 |
                             (8 + sizeof(DBGKM_EXIT_THREAD));
    ApiMessage.h.u2.ZeroInit = LPC_DEBUG_EVENT;
    ApiMessage.ApiNumber = DbgKmExitThreadApi;

    /* Suspend the process */
    Suspended = DbgkpSuspendProcess();

    /* Send the message */
    DbgkpSendApiMessage(&ApiMessage, FALSE);

    /* Resume the process if needed */
    if (Suspended) DbgkpResumeProcess();
}

VOID
NTAPI
DbgkMapViewOfSection(IN HANDLE SectionHandle,
                     IN PVOID BaseAddress,
                     IN ULONG SectionOffset,
                     IN ULONG_PTR ViewSize)
{
    DBGKM_MSG ApiMessage;
    PDBGKM_LOAD_DLL LoadDll = &ApiMessage.LoadDll;
    PEPROCESS Process = PsGetCurrentProcess();
    PETHREAD Thread = PsGetCurrentThread();
    PIMAGE_NT_HEADERS NtHeader;
    PAGED_CODE();

    /* Check if this thread is hidden, doesn't have a debug port, or died */
    if ((Thread->HideFromDebugger) ||
        !(Process->DebugPort) ||
        (Thread->DeadThread) ||
        (KeGetPreviousMode() == KernelMode))
    {
        /* Don't notify the debugger */
        return;
    }

    /* Setup the parameters */
    LoadDll->FileHandle = DbgkpSectionToFileHandle(SectionHandle);
    LoadDll->BaseOfDll = BaseAddress;
    LoadDll->DebugInfoFileOffset = 0;
    LoadDll->DebugInfoSize = 0;

    /* Get the NT Headers */
    NtHeader = RtlImageNtHeader(BaseAddress);
    if (NtHeader)
    {
        /* Fill out debug information */
        LoadDll->DebugInfoFileOffset = NtHeader->FileHeader.
                                       PointerToSymbolTable;
        LoadDll->DebugInfoSize = NtHeader->FileHeader.NumberOfSymbols;
    }

    /* Setup the API Message */
    ApiMessage.h.u1.Length = sizeof(DBGKM_MSG) << 16 |
                             (8 + sizeof(DBGKM_LOAD_DLL));
    ApiMessage.h.u2.ZeroInit = LPC_DEBUG_EVENT;
    ApiMessage.ApiNumber = DbgKmLoadDllApi;

    /* Send the message */
    DbgkpSendApiMessage(&ApiMessage, TRUE);

    /* Close the handle */
    ObCloseHandle(LoadDll->FileHandle, KernelMode);
}

VOID
NTAPI
DbgkUnMapViewOfSection(IN PVOID BaseAddress)
{
    DBGKM_MSG ApiMessage;
    PDBGKM_UNLOAD_DLL UnloadDll = &ApiMessage.UnloadDll;
    PEPROCESS Process = PsGetCurrentProcess();
    PETHREAD Thread = PsGetCurrentThread();
    PAGED_CODE();

    /* Check if this thread is hidden, doesn't have a debug port, or died */
    if ((Thread->HideFromDebugger) ||
        !(Process->DebugPort) ||
        (Thread->DeadThread) ||
        (KeGetPreviousMode() == KernelMode))
    {
        /* Don't notify the debugger */
        return;
    }

    /* Set the DLL Base */
    UnloadDll->BaseAddress = BaseAddress;

    /* Setup the API Message */
    ApiMessage.h.u1.Length = sizeof(DBGKM_MSG) << 16 |
                             (8 + sizeof(DBGKM_UNLOAD_DLL));
    ApiMessage.h.u2.ZeroInit = LPC_DEBUG_EVENT;
    ApiMessage.ApiNumber = DbgKmUnloadDllApi;

    /* Send the message */
    DbgkpSendApiMessage(&ApiMessage, TRUE);
}

/* EOF */
