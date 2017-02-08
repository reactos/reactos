/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv_new/console.c
 * PURPOSE:         Console Management Functions
 * PROGRAMMERS:     Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/conio2.h"
#include "handle.h"
#include "procinit.h"
#include "alias.h"
#include "coninput.h"
#include "conoutput.h"
#include "lineinput.h"
#include "include/settings.h"

#include "frontends/gui/guiterm.h"
#ifdef TUITERM_COMPILE
#include "frontends/tui/tuiterm.h"
#endif

#include "include/console.h"
#include "console.h"
#include "resource.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

/***************/
#ifdef TUITERM_COMPILE
NTSTATUS NTAPI
TuiLoadFrontEnd(IN OUT PFRONTEND FrontEnd,
                IN OUT PCONSOLE_INFO ConsoleInfo,
                IN OUT PVOID ExtraConsoleInfo,
                IN ULONG ProcessId);
NTSTATUS NTAPI
TuiUnloadFrontEnd(IN OUT PFRONTEND FrontEnd);
#endif

NTSTATUS NTAPI
GuiLoadFrontEnd(IN OUT PFRONTEND FrontEnd,
                IN OUT PCONSOLE_INFO ConsoleInfo,
                IN OUT PVOID ExtraConsoleInfo,
                IN ULONG ProcessId);
NTSTATUS NTAPI
GuiUnloadFrontEnd(IN OUT PFRONTEND FrontEnd);
/***************/

typedef
NTSTATUS (NTAPI *FRONTEND_LOAD)(IN OUT PFRONTEND FrontEnd,
                                IN OUT PCONSOLE_INFO ConsoleInfo,
                                IN OUT PVOID ExtraConsoleInfo,
                                IN ULONG ProcessId);

typedef
NTSTATUS (NTAPI *FRONTEND_UNLOAD)(IN OUT PFRONTEND FrontEnd);

/*
 * If we are not in GUI-mode, start the text-mode terminal emulator.
 * If we fail, try to start the GUI-mode terminal emulator.
 *
 * Try to open the GUI-mode terminal emulator. Two cases are possible:
 * - We are in GUI-mode, therefore GuiMode == TRUE, the previous test-case
 *   failed and we start GUI-mode terminal emulator.
 * - We are in text-mode, therefore GuiMode == FALSE, the previous test-case
 *   succeeded BUT we failed at starting text-mode terminal emulator.
 *   Then GuiMode was switched to TRUE in order to try to open the GUI-mode
 *   terminal emulator (Win32k will automatically switch to graphical mode,
 *   therefore no additional code is needed).
 */

/*
 * NOTE: Each entry of the table should be retrieved when loading a front-end
 *       (examples of the CSR servers which register some data for CSRSS).
 */
struct
{
    CHAR            FrontEndName[80];
    FRONTEND_LOAD   FrontEndLoad;
    FRONTEND_UNLOAD FrontEndUnload;
} FrontEndLoadingMethods[] =
{
#ifdef TUITERM_COMPILE
    {"TUI", TuiLoadFrontEnd,    TuiUnloadFrontEnd},
#endif
    {"GUI", GuiLoadFrontEnd,    GuiUnloadFrontEnd},

//  {"Not found", 0, NULL}
};


/* PRIVATE FUNCTIONS **********************************************************/

#if 0000
VOID FASTCALL
ConioPause(PCONSOLE Console, UINT Flags)
{
    Console->PauseFlags |= Flags;
    if (!Console->UnpauseEvent)
        Console->UnpauseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

VOID FASTCALL
ConioUnpause(PCONSOLE Console, UINT Flags)
{
    Console->PauseFlags &= ~Flags;

    // if ((Console->PauseFlags & (PAUSED_FROM_KEYBOARD | PAUSED_FROM_SCROLLBAR | PAUSED_FROM_SELECTION)) == 0)
    if (Console->PauseFlags == 0 && Console->UnpauseEvent)
    {
        SetEvent(Console->UnpauseEvent);
        CloseHandle(Console->UnpauseEvent);
        Console->UnpauseEvent = NULL;

        CsrNotifyWait(&Console->WriteWaitQueue,
                      WaitAll,
                      NULL,
                      NULL);
        if (!IsListEmpty(&Console->WriteWaitQueue))
        {
            CsrDereferenceWait(&Console->WriteWaitQueue);
        }
    }
}
#endif


NTSTATUS FASTCALL
ConSrvGetConsole(PCONSOLE_PROCESS_DATA ProcessData,
                 PCONSOLE* Console,
                 BOOL LockConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE ProcessConsole;

    ASSERT(Console);
    *Console = NULL;

    // RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    Status = ConDrvGetConsole(&ProcessConsole, ProcessData->ConsoleHandle, LockConsole);
    if (NT_SUCCESS(Status)) *Console = ProcessConsole;

    // RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return Status;
}

VOID FASTCALL
ConSrvReleaseConsole(PCONSOLE Console,
                     BOOL WasConsoleLocked)
{
    /* Just call the driver*/
    ConDrvReleaseConsole(Console, WasConsoleLocked);
}


NTSTATUS WINAPI
ConSrvInitConsole(OUT PHANDLE NewConsoleHandle,
                  OUT PCONSOLE* NewConsole,
                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                  IN ULONG ConsoleLeaderProcessId)
{
    NTSTATUS Status;
    HANDLE ConsoleHandle;
    PCONSOLE Console;
    CONSOLE_INFO ConsoleInfo;
    SIZE_T Length = 0;
    ULONG i = 0;
    FRONTEND FrontEnd;

    if (NewConsole == NULL || ConsoleStartInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    *NewConsole = NULL;

    /*
     * Load the console settings
     */

    /* 1. Load the default settings */
    ConSrvGetDefaultSettings(&ConsoleInfo, ConsoleLeaderProcessId);

    /* 2. Get the title of the console (initialize ConsoleInfo.ConsoleTitle) */
    Length = min(wcslen(ConsoleStartInfo->ConsoleTitle),
                 sizeof(ConsoleInfo.ConsoleTitle) / sizeof(ConsoleInfo.ConsoleTitle[0]) - 1);
    wcsncpy(ConsoleInfo.ConsoleTitle, ConsoleStartInfo->ConsoleTitle, Length);
    ConsoleInfo.ConsoleTitle[Length] = L'\0';


    /*
     * Choose an adequate terminal front-end to load, and load it
     */
    Status = STATUS_SUCCESS;
    for (i = 0; i < sizeof(FrontEndLoadingMethods) / sizeof(FrontEndLoadingMethods[0]); ++i)
    {
        DPRINT("CONSRV: Trying to load %s terminal emulator...\n", FrontEndLoadingMethods[i].FrontEndName);
        Status = FrontEndLoadingMethods[i].FrontEndLoad(&FrontEnd,
                                                        &ConsoleInfo,
                                                        ConsoleStartInfo,
                                                        ConsoleLeaderProcessId);
        if (NT_SUCCESS(Status))
        {
            DPRINT("CONSRV: %s terminal emulator loaded successfully\n", FrontEndLoadingMethods[i].FrontEndName);
            break;
        }
        else
        {
            DPRINT1("CONSRV: Loading %s terminal emulator failed, Status = 0x%08lx , continuing...\n", FrontEndLoadingMethods[i].FrontEndName, Status);
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CONSRV: Failed to initialize a frontend, Status = 0x%08lx\n", Status);
        return Status;
    }

    DPRINT("CONSRV: Frontend initialized\n");


/******************************************************************************/
    /*
     * 4. Load the remaining console settings via the registry.
     */
    if ((ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) == 0)
    {
        /*
         * Either we weren't created by an app launched via a shell-link,
         * or we failed to load shell-link console properties.
         * Therefore, load the console infos for the application from the registry.
         */
        ConSrvReadUserSettings(&ConsoleInfo, ConsoleLeaderProcessId);

        /*
         * Now, update them with the properties the user might gave to us
         * via the STARTUPINFO structure before calling CreateProcess
         * (and which was transmitted via the ConsoleStartInfo structure).
         * We therefore overwrite the values read in the registry.
         */
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USEFILLATTRIBUTE)
        {
            ConsoleInfo.ScreenAttrib = (USHORT)ConsoleStartInfo->FillAttribute;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USECOUNTCHARS)
        {
            ConsoleInfo.ScreenBufferSize = ConsoleStartInfo->ScreenBufferSize;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USESIZE)
        {
            // ConsoleInfo.ConsoleSize = ConsoleStartInfo->ConsoleWindowSize;
            ConsoleInfo.ConsoleSize.X = (SHORT)ConsoleStartInfo->ConsoleWindowSize.cx;
            ConsoleInfo.ConsoleSize.Y = (SHORT)ConsoleStartInfo->ConsoleWindowSize.cy;
        }
    }

    /* Set-up the code page */
    ConsoleInfo.CodePage = GetOEMCP();
/******************************************************************************/

    Status = ConDrvInitConsole(&ConsoleHandle,
                               &Console,
                               &ConsoleInfo,
                               ConsoleLeaderProcessId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Creating a new console failed, Status = 0x%08lx\n", Status);
        FrontEndLoadingMethods[i].FrontEndUnload(&FrontEnd);
        return Status;
    }

    ASSERT(Console);
    DPRINT("Console initialized\n");

    Status = ConDrvRegisterFrontEnd(Console, &FrontEnd);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register frontend to the given console, Status = 0x%08lx\n", Status);
        ConDrvDeleteConsole(Console);
        FrontEndLoadingMethods[i].FrontEndUnload(&FrontEnd);
        return Status;
    }
    DPRINT("FrontEnd registered\n");

    /* Return the newly created console to the caller and a success code too */
    *NewConsoleHandle = ConsoleHandle;
    *NewConsole       = Console;
    return STATUS_SUCCESS;
}

VOID WINAPI
ConSrvDeleteConsole(PCONSOLE Console)
{
    DPRINT("ConSrvDeleteConsole\n");

    /* Just call the driver. ConSrvDeregisterFrontEnd is called on-demand. */
    ConDrvDeleteConsole(Console);
}


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvAllocConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_ALLOCCONSOLE AllocConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.AllocConsoleRequest;
    PCSR_PROCESS CsrProcess = CsrGetClientThread()->Process;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrProcess);

    if (ProcessData->ConsoleHandle != NULL)
    {
        DPRINT1("Process already has a console\n");
        return STATUS_ACCESS_DENIED;
    }

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&AllocConsoleRequest->ConsoleStartInfo,
                                  1,
                                  sizeof(CONSOLE_START_INFO)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize a new Console owned by the Console Leader Process */
    Status = ConSrvAllocateConsole(ProcessData,
                                   &AllocConsoleRequest->InputHandle,
                                   &AllocConsoleRequest->OutputHandle,
                                   &AllocConsoleRequest->ErrorHandle,
                                   AllocConsoleRequest->ConsoleStartInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console allocation failed\n");
        return Status;
    }

    /* Return the console handle and the input wait handle to the caller */
    AllocConsoleRequest->ConsoleHandle   = ProcessData->ConsoleHandle;
    AllocConsoleRequest->InputWaitHandle = ProcessData->ConsoleEvent;

    /* Set the Property-Dialog and Control-Dispatcher handlers */
    ProcessData->PropDispatcher = AllocConsoleRequest->PropDispatcher;
    ProcessData->CtrlDispatcher = AllocConsoleRequest->CtrlDispatcher;

    return STATUS_SUCCESS;
}

CSR_API(SrvAttachConsole)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_ATTACHCONSOLE AttachConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.AttachConsoleRequest;
    PCSR_PROCESS SourceProcess = NULL;  // The parent process.
    PCSR_PROCESS TargetProcess = CsrGetClientThread()->Process; // Ourselves.
    HANDLE ProcessId = ULongToHandle(AttachConsoleRequest->ProcessId);
    PCONSOLE_PROCESS_DATA SourceProcessData, TargetProcessData;

    TargetProcessData = ConsoleGetPerProcessData(TargetProcess);

    if (TargetProcessData->ConsoleHandle != NULL)
    {
        DPRINT1("Process already has a console\n");
        return STATUS_ACCESS_DENIED;
    }

    /* Check whether we try to attach to the parent's console */
    if (ProcessId == ULongToHandle(ATTACH_PARENT_PROCESS))
    {
        PROCESS_BASIC_INFORMATION ProcessInfo;
        ULONG Length = sizeof(ProcessInfo);

        /* Get the real parent's ID */

        Status = NtQueryInformationProcess(TargetProcess->ProcessHandle,
                                           ProcessBasicInformation,
                                           &ProcessInfo,
                                           Length, &Length);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SrvAttachConsole - Cannot retrieve basic process info, Status = %lu\n", Status);
            return Status;
        }

        ProcessId = ULongToHandle(ProcessInfo.InheritedFromUniqueProcessId);
    }

    /* Lock the source process via its PID */
    Status = CsrLockProcessByClientId(ProcessId, &SourceProcess);
    if (!NT_SUCCESS(Status)) return Status;

    SourceProcessData = ConsoleGetPerProcessData(SourceProcess);

    if (SourceProcessData->ConsoleHandle == NULL)
    {
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }

    /*
     * Inherit the console from the parent,
     * if any, otherwise return an error.
     */
    Status = ConSrvInheritConsole(TargetProcessData,
                                  SourceProcessData->ConsoleHandle,
                                  TRUE,
                                  &AttachConsoleRequest->InputHandle,
                                  &AttachConsoleRequest->OutputHandle,
                                  &AttachConsoleRequest->ErrorHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Console inheritance failed\n");
        goto Quit;
    }

    /* Return the console handle and the input wait handle to the caller */
    AttachConsoleRequest->ConsoleHandle   = TargetProcessData->ConsoleHandle;
    AttachConsoleRequest->InputWaitHandle = TargetProcessData->ConsoleEvent;

    /* Set the Property-Dialog and Control-Dispatcher handlers */
    TargetProcessData->PropDispatcher = AttachConsoleRequest->PropDispatcher;
    TargetProcessData->CtrlDispatcher = AttachConsoleRequest->CtrlDispatcher;

    Status = STATUS_SUCCESS;

Quit:
    /* Unlock the "source" process and exit */
    CsrUnlockProcess(SourceProcess);
    return Status;
}

CSR_API(SrvFreeConsole)
{
    ConSrvRemoveConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process));
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvGetConsoleMode(IN PCONSOLE Console,
                     IN PCONSOLE_IO_OBJECT Object,
                     OUT PULONG ConsoleMode);
CSR_API(SrvGetConsoleMode)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCONSOLE_IO_OBJECT Object;

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                             ConsoleModeRequest->ConsoleHandle,
                             &Object, NULL, GENERIC_READ, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvGetConsoleMode(Object->Console, Object,
                                  &ConsoleModeRequest->ConsoleMode);

    ConSrvReleaseObject(Object, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleMode(IN PCONSOLE Console,
                     IN PCONSOLE_IO_OBJECT Object,
                     IN ULONG ConsoleMode);
CSR_API(SrvSetConsoleMode)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleModeRequest;
    PCONSOLE_IO_OBJECT Object;

    Status = ConSrvGetObject(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                             ConsoleModeRequest->ConsoleHandle,
                             &Object, NULL, GENERIC_WRITE, TRUE, 0);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleMode(Object->Console, Object,
                                  ConsoleModeRequest->ConsoleMode);

    ConSrvReleaseObject(Object, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleTitle(IN PCONSOLE Console,
                      IN OUT PWCHAR Title,
                      IN OUT PULONG BufLength);
CSR_API(SrvGetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    PCONSOLE Console;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console, status %lx\n", Status);
        return Status;
    }

    Status = ConDrvGetConsoleTitle(Console,
                                   TitleRequest->Title,
                                   &TitleRequest->Length);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleTitle(IN PCONSOLE Console,
                      IN PWCHAR Title,
                      IN ULONG BufLength);
CSR_API(SrvSetConsoleTitle)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.TitleRequest;
    PCONSOLE Console;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&TitleRequest->Title,
                                  TitleRequest->Length,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console, status %lx\n", Status);
        return Status;
    }

    Status = ConDrvSetConsoleTitle(Console,
                                   TitleRequest->Title,
                                   TitleRequest->Length);

    if (NT_SUCCESS(Status)) ConioChangeTitle(Console);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleCP(IN PCONSOLE Console,
                   OUT PUINT CodePage,
                   IN BOOLEAN InputCP);
CSR_API(SrvGetConsoleCP)
{
    NTSTATUS Status;
    PCONSOLE_GETSETINPUTOUTPUTCP ConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleCPRequest;
    PCONSOLE Console;

    DPRINT("SrvGetConsoleCP, getting %s Code Page\n",
            ConsoleCPRequest->InputCP ? "Input" : "Output");

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvGetConsoleCP(Console,
                                &ConsoleCPRequest->CodePage,
                                ConsoleCPRequest->InputCP);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleCP(IN PCONSOLE Console,
                   IN UINT CodePage,
                   IN BOOLEAN InputCP);
CSR_API(SrvSetConsoleCP)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PCONSOLE_GETSETINPUTOUTPUTCP ConsoleCPRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ConsoleCPRequest;
    PCONSOLE Console;

    DPRINT("SrvSetConsoleCP, setting %s Code Page\n",
            ConsoleCPRequest->InputCP ? "Input" : "Output");

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleCP(Console,
                                ConsoleCPRequest->CodePage,
                                ConsoleCPRequest->InputCP);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleProcessList(IN PCONSOLE Console,
                            IN OUT PULONG ProcessIdsList,
                            IN ULONG MaxIdListItems,
                            OUT PULONG ProcessIdsTotal);
CSR_API(SrvGetConsoleProcessList)
{
    NTSTATUS Status;
    PCONSOLE_GETPROCESSLIST GetProcessListRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetProcessListRequest;
    PCONSOLE Console;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&GetProcessListRequest->pProcessIds,
                                  GetProcessListRequest->nMaxIds,
                                  sizeof(DWORD)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvGetConsoleProcessList(Console,
                                         GetProcessListRequest->pProcessIds,
                                         GetProcessListRequest->nMaxIds,
                                         &GetProcessListRequest->nProcessIdsTotal);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGenerateConsoleCtrlEvent)
{
    NTSTATUS Status;
    PCONSOLE_GENERATECTRLEVENT GenerateCtrlEventRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GenerateCtrlEventRequest;
    PCONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvConsoleProcessCtrlEvent(Console,
                                           GenerateCtrlEventRequest->ProcessGroup,
                                           GenerateCtrlEventRequest->Event);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

/* EOF */
