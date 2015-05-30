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
RTL_CRITICAL_SECTION WowCriticalSection;

/* FUNCTIONS ******************************************************************/

NTSTATUS NTAPI BaseSrvGetConsoleRecord(HANDLE ConsoleHandle, PVDM_CONSOLE_RECORD *Record)
{
    PLIST_ENTRY i;
    PVDM_CONSOLE_RECORD CurrentRecord = NULL;

    /* NULL is not a valid console handle */
    if (ConsoleHandle == NULL) return STATUS_INVALID_PARAMETER;

    /* Search for a record that has the same console handle */
    for (i = VDMConsoleListHead.Flink; i != &VDMConsoleListHead; i = i->Flink)
    {
        CurrentRecord = CONTAINING_RECORD(i, VDM_CONSOLE_RECORD, Entry);
        if (CurrentRecord->ConsoleHandle == ConsoleHandle) break;
    }

    /* Check if nothing was found */
    if (i == &VDMConsoleListHead) CurrentRecord = NULL;

    *Record = CurrentRecord;
    return CurrentRecord ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

NTSTATUS NTAPI GetConsoleRecordBySessionId(ULONG TaskId, PVDM_CONSOLE_RECORD *Record)
{
    PLIST_ENTRY i;
    PVDM_CONSOLE_RECORD CurrentRecord = NULL;

    /* Search for a record that has the same console handle */
    for (i = VDMConsoleListHead.Flink; i != &VDMConsoleListHead; i = i->Flink)
    {
        CurrentRecord = CONTAINING_RECORD(i, VDM_CONSOLE_RECORD, Entry);
        if (CurrentRecord->SessionId == TaskId) break;
    }

    /* Check if nothing was found */
    if (i == &VDMConsoleListHead) CurrentRecord = NULL;

    *Record = CurrentRecord;
    return CurrentRecord ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

ULONG NTAPI GetNextDosSesId(VOID)
{
    ULONG SessionId;
    PLIST_ENTRY i;
    PVDM_CONSOLE_RECORD CurrentRecord = NULL;
    BOOLEAN Found;

    /* Search for an available session ID */
    for (SessionId = 1; SessionId != 0; SessionId++)
    {
        Found = FALSE;

        /* Check if the ID is already in use */
        for (i = VDMConsoleListHead.Flink; i != &VDMConsoleListHead; i = i->Flink)
        {
            CurrentRecord = CONTAINING_RECORD(i, VDM_CONSOLE_RECORD, Entry);
            if (CurrentRecord->SessionId == SessionId) Found = TRUE;
        }

        /* If not, we found one */
        if (!Found) break;
    }

    ASSERT(SessionId != 0);

    /* Return the session ID */
    return SessionId;
}

BOOLEAN NTAPI BaseSrvIsVdmAllowed(VOID)
{
    NTSTATUS Status;
    BOOLEAN VdmAllowed = TRUE;
    HANDLE RootKey, KeyHandle;
    UNICODE_STRING KeyName, ValueName, MachineKeyName;
    OBJECT_ATTRIBUTES Attributes;
    UCHAR ValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    ULONG ActualSize;

    /* Initialize the unicode strings */
    RtlInitUnicodeString(&MachineKeyName, L"\\Registry\\Machine");
    RtlInitUnicodeString(&KeyName, VDM_POLICY_KEY_NAME);
    RtlInitUnicodeString(&ValueName, VDM_DISALLOWED_VALUE_NAME);

    InitializeObjectAttributes(&Attributes,
                               &MachineKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the local machine key */
    Status = NtOpenKey(&RootKey, KEY_READ, &Attributes);
    if (!NT_SUCCESS(Status)) return FALSE;

    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    /* Open the policy key in the local machine hive, if it exists */
    if (NT_SUCCESS(NtOpenKey(&KeyHandle, KEY_READ, &Attributes)))
    {
        /* Read the value, if it's set */
        if (NT_SUCCESS(NtQueryValueKey(KeyHandle,
                                       &ValueName,
                                       KeyValuePartialInformation,
                                       ValueInfo,
                                       sizeof(ValueBuffer),
                                       &ActualSize)))
        {
            if (*((PULONG)ValueInfo->Data))
            {
                /* The VDM has been disabled in the registry */
                VdmAllowed = FALSE;
            }
        }

        NtClose(KeyHandle);
    }

    /* Close the local machine key */
    NtClose(RootKey);

    /* If it's disabled system-wide, there's no need to check the user key */
    if (!VdmAllowed) return FALSE;

    /* Open the current user key of the client */
    if (!CsrImpersonateClient(NULL)) return VdmAllowed;
    Status = RtlOpenCurrentUser(KEY_READ, &RootKey);
    CsrRevertToSelf();

    /* If that fails, return the system-wide setting */
    if (!NT_SUCCESS(Status)) return VdmAllowed;

    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    /* Open the policy key in the current user hive, if it exists */
    if (NT_SUCCESS(NtOpenKey(&KeyHandle, KEY_READ, &Attributes)))
    {
        /* Read the value, if it's set */
        if (NT_SUCCESS(NtQueryValueKey(KeyHandle,
                                       &ValueName,
                                       KeyValuePartialInformation,
                                       ValueInfo,
                                       sizeof(ValueBuffer),
                                       &ActualSize)))
        {
            if (*((PULONG)ValueInfo->Data))
            {
                /* The VDM has been disabled in the registry */
                VdmAllowed = FALSE;
            }
        }

        NtClose(KeyHandle);
    }

    return VdmAllowed;
}

NTSTATUS NTAPI BaseSrvCreatePairWaitHandles(PHANDLE ServerEvent, PHANDLE ClientEvent)
{
    NTSTATUS Status;

    /* Create the event */
    Status = NtCreateEvent(ServerEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Duplicate the event into the client process */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               *ServerEvent,
                               CsrGetClientThread()->Process->ProcessHandle,
                               ClientEvent,
                               0,
                               0,
                               DUPLICATE_SAME_ATTRIBUTES | DUPLICATE_SAME_ACCESS);

    if (!NT_SUCCESS(Status)) NtClose(*ServerEvent);
    return Status;
}

VOID NTAPI BaseSrvFreeVDMInfo(PVDM_COMMAND_INFO CommandInfo)
{
    /* Free the allocated structure members */
    if (CommandInfo->CmdLine != NULL) RtlFreeHeap(BaseSrvHeap, 0, CommandInfo->CmdLine);
    if (CommandInfo->AppName != NULL) RtlFreeHeap(BaseSrvHeap, 0, CommandInfo->AppName);
    if (CommandInfo->PifFile != NULL) RtlFreeHeap(BaseSrvHeap, 0, CommandInfo->PifFile);
    if (CommandInfo->CurDirectory != NULL) RtlFreeHeap(BaseSrvHeap, 0, CommandInfo->CurDirectory);
    if (CommandInfo->Env != NULL) RtlFreeHeap(BaseSrvHeap, 0, CommandInfo->Env);
    if (CommandInfo->Desktop != NULL) RtlFreeHeap(BaseSrvHeap, 0, CommandInfo->Desktop);
    if (CommandInfo->Title != NULL) RtlFreeHeap(BaseSrvHeap, 0, CommandInfo->Title);
    if (CommandInfo->Reserved != NULL) RtlFreeHeap(BaseSrvHeap, 0, CommandInfo->Reserved);

    /* Free the structure itself */
    RtlFreeHeap(BaseSrvHeap, 0, CommandInfo);
}

VOID
NTAPI
BaseSrvCleanupVDMResources(IN PCSR_PROCESS CsrProcess)
{
    ULONG ProcessId = HandleToUlong(CsrProcess->ClientId.UniqueProcess);
    PVDM_CONSOLE_RECORD ConsoleRecord = NULL;
    PVDM_DOS_RECORD DosRecord;
    PLIST_ENTRY i;

    /* Enter the critical section */
    RtlEnterCriticalSection(&DosCriticalSection);

    /* Search for a record that has the same process handle */
    for (i = VDMConsoleListHead.Flink; i != &VDMConsoleListHead; i = i->Flink)
    {
        ConsoleRecord = CONTAINING_RECORD(i, VDM_CONSOLE_RECORD, Entry);

        if (ConsoleRecord->ProcessId == ProcessId)
        {
            /* Cleanup the DOS records */
            while (ConsoleRecord->DosListHead.Flink != &ConsoleRecord->DosListHead)
            {
                DosRecord = CONTAINING_RECORD(ConsoleRecord->DosListHead.Flink,
                                              VDM_DOS_RECORD,
                                              Entry);

                /* Set the event and close it */
                NtSetEvent(DosRecord->ServerEvent, NULL);
                NtClose(DosRecord->ServerEvent);

                /* Remove the DOS entry */
                if (DosRecord->CommandInfo) BaseSrvFreeVDMInfo(DosRecord->CommandInfo);
                RemoveEntryList(&DosRecord->Entry);
                RtlFreeHeap(BaseSrvHeap, 0, DosRecord);
            }

            if (ConsoleRecord->CurrentDirs != NULL)
            {
                /* Free the current directories */
                RtlFreeHeap(BaseSrvHeap, 0, ConsoleRecord->CurrentDirs);
                ConsoleRecord->CurrentDirs = NULL;
                ConsoleRecord->CurDirsLength = 0;
            }

            /* Close the process handle */
            if (ConsoleRecord->ProcessHandle) NtClose(ConsoleRecord->ProcessHandle);

            /* Close the event handle */
            if (ConsoleRecord->ServerEvent) NtClose(ConsoleRecord->ServerEvent);

            /* Remove the console record */
            i = i->Blink;
            RemoveEntryList(&ConsoleRecord->Entry);
            RtlFreeHeap(BaseSrvHeap, 0, ConsoleRecord);
        }
    }

    /* Leave the critical section */
    RtlLeaveCriticalSection(&DosCriticalSection);
}

BOOLEAN NTAPI BaseSrvCopyCommand(PBASE_CHECK_VDM CheckVdmRequest, PVDM_DOS_RECORD DosRecord)
{
    BOOLEAN Success = FALSE;
    PVDM_COMMAND_INFO CommandInfo = NULL;

    /* Allocate the command information structure */
    CommandInfo = (PVDM_COMMAND_INFO)RtlAllocateHeap(BaseSrvHeap,
                                                     HEAP_ZERO_MEMORY,
                                                     sizeof(VDM_COMMAND_INFO));
    if (CommandInfo == NULL) return FALSE;

    /* Fill the structure */
    CommandInfo->TaskId = CheckVdmRequest->iTask;
    CommandInfo->ExitCode = DosRecord->ExitCode;
    CommandInfo->CodePage = CheckVdmRequest->CodePage;
    CommandInfo->StdIn = CheckVdmRequest->StdIn;
    CommandInfo->StdOut = CheckVdmRequest->StdOut;
    CommandInfo->StdErr = CheckVdmRequest->StdErr;

    /* Allocate memory for the command line */
    CommandInfo->CmdLine = RtlAllocateHeap(BaseSrvHeap,
                                           HEAP_ZERO_MEMORY,
                                           CheckVdmRequest->CmdLen);
    if (CommandInfo->CmdLine == NULL) goto Cleanup;

    /* Copy the command line */
    RtlMoveMemory(CommandInfo->CmdLine, CheckVdmRequest->CmdLine, CheckVdmRequest->CmdLen);

    /* Allocate memory for the application name */
    CommandInfo->AppName = RtlAllocateHeap(BaseSrvHeap,
                                           HEAP_ZERO_MEMORY,
                                           CheckVdmRequest->AppLen);
    if (CommandInfo->AppName == NULL) goto Cleanup;

    /* Copy the application name */
    RtlMoveMemory(CommandInfo->AppName, CheckVdmRequest->AppName, CheckVdmRequest->AppLen);

    /* Allocate memory for the PIF file name */
    if (CheckVdmRequest->PifLen != 0)
    {
        CommandInfo->PifFile = RtlAllocateHeap(BaseSrvHeap,
                                               HEAP_ZERO_MEMORY,
                                               CheckVdmRequest->PifLen);
        if (CommandInfo->PifFile == NULL) goto Cleanup;

        /* Copy the PIF file name */
        RtlMoveMemory(CommandInfo->PifFile, CheckVdmRequest->PifFile, CheckVdmRequest->PifLen);
    }
    else CommandInfo->PifFile = NULL;

    /* Allocate memory for the current directory */
    if (CheckVdmRequest->CurDirectoryLen != 0)
    {
        CommandInfo->CurDirectory = RtlAllocateHeap(BaseSrvHeap,
                                                    HEAP_ZERO_MEMORY,
                                                    CheckVdmRequest->CurDirectoryLen);
        if (CommandInfo->CurDirectory == NULL) goto Cleanup;

        /* Copy the current directory */
        RtlMoveMemory(CommandInfo->CurDirectory,
                      CheckVdmRequest->CurDirectory,
                      CheckVdmRequest->CurDirectoryLen);
    }
    else CommandInfo->CurDirectory = NULL;

    /* Allocate memory for the environment block */
    CommandInfo->Env = RtlAllocateHeap(BaseSrvHeap,
                                       HEAP_ZERO_MEMORY,
                                       CheckVdmRequest->EnvLen);
    if (CommandInfo->Env == NULL) goto Cleanup;

    /* Copy the environment block */
    RtlMoveMemory(CommandInfo->Env, CheckVdmRequest->Env, CheckVdmRequest->EnvLen);

    CommandInfo->EnvLen = CheckVdmRequest->EnvLen;
    RtlMoveMemory(&CommandInfo->StartupInfo,
                  CheckVdmRequest->StartupInfo,
                  sizeof(STARTUPINFOA));

    /* Allocate memory for the desktop */
    if (CheckVdmRequest->DesktopLen != 0)
    {
        CommandInfo->Desktop = RtlAllocateHeap(BaseSrvHeap,
                                               HEAP_ZERO_MEMORY,
                                               CheckVdmRequest->DesktopLen);
        if (CommandInfo->Desktop == NULL) goto Cleanup;

        /* Copy the desktop name */
        RtlMoveMemory(CommandInfo->Desktop, CheckVdmRequest->Desktop, CheckVdmRequest->DesktopLen);
    }
    else CommandInfo->Desktop = NULL;

    CommandInfo->DesktopLen = CheckVdmRequest->DesktopLen;

    /* Allocate memory for the title */
    if (CheckVdmRequest->TitleLen != 0)
    {
        CommandInfo->Title = RtlAllocateHeap(BaseSrvHeap,
                                             HEAP_ZERO_MEMORY,
                                             CheckVdmRequest->TitleLen);
        if (CommandInfo->Title == NULL) goto Cleanup;

        /* Copy the title */
        RtlMoveMemory(CommandInfo->Title, CheckVdmRequest->Title, CheckVdmRequest->TitleLen);
    }
    else CommandInfo->Title = NULL;

    CommandInfo->TitleLen = CheckVdmRequest->TitleLen;

    /* Allocate memory for the reserved field */
    if (CheckVdmRequest->ReservedLen != 0)
    {
        CommandInfo->Reserved = RtlAllocateHeap(BaseSrvHeap,
                                                HEAP_ZERO_MEMORY,
                                                CheckVdmRequest->ReservedLen);
        if (CommandInfo->Reserved == NULL) goto Cleanup;

        /* Copy the reserved field */
        RtlMoveMemory(CommandInfo->Reserved,
                      CheckVdmRequest->Reserved,
                      CheckVdmRequest->ReservedLen);
    }
    else CommandInfo->Reserved = NULL;

    CommandInfo->ReservedLen = CheckVdmRequest->ReservedLen;

    CommandInfo->CmdLen = CheckVdmRequest->CmdLen;
    CommandInfo->AppLen = CheckVdmRequest->AppLen;
    CommandInfo->PifLen = CheckVdmRequest->PifLen;
    CommandInfo->CurDirectoryLen = CheckVdmRequest->CurDirectoryLen;
    CommandInfo->VDMState = DosRecord->State;
    // TODO: Set CommandInfo->CurrentDrive
    // TODO: Set CommandInfo->ComingFromBat

    /* Set the DOS record's command structure */
    DosRecord->CommandInfo = CommandInfo;

    /* The operation was successful */
    Success = TRUE;

Cleanup:
    /* If it wasn't successful, free the memory */
    if (!Success) BaseSrvFreeVDMInfo(CommandInfo);

    return Success;
}

NTSTATUS NTAPI BaseSrvFillCommandInfo(PVDM_COMMAND_INFO CommandInfo,
                                      PBASE_GET_NEXT_VDM_COMMAND Message)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Copy the data */
    Message->iTask = CommandInfo->TaskId;
    Message->StdIn = CommandInfo->StdIn;
    Message->StdOut = CommandInfo->StdOut;
    Message->StdErr = CommandInfo->StdErr;
    Message->CodePage = CommandInfo->CodePage;
    Message->dwCreationFlags = CommandInfo->CreationFlags;
    Message->ExitCode = CommandInfo->ExitCode;
    Message->CurrentDrive = CommandInfo->CurrentDrive;
    Message->VDMState = CommandInfo->VDMState;
    Message->fComingFromBat = CommandInfo->ComingFromBat;

    if (CommandInfo->CmdLen && Message->CmdLen)
    {
        if (Message->CmdLen >= CommandInfo->CmdLen)
        {
            /* Copy the command line */
            RtlMoveMemory(Message->CmdLine, CommandInfo->CmdLine, CommandInfo->CmdLen);
        }
        else Status = STATUS_BUFFER_TOO_SMALL;

        Message->CmdLen = CommandInfo->CmdLen;
    }

    if (CommandInfo->AppLen && Message->AppLen)
    {
        if (Message->AppLen >= CommandInfo->AppLen)
        {
            /* Copy the application name */
            RtlMoveMemory(Message->AppName, CommandInfo->AppName, CommandInfo->AppLen);
        }
        else Status = STATUS_BUFFER_TOO_SMALL;

        Message->AppLen = CommandInfo->AppLen;
    }

    if (CommandInfo->PifLen && Message->PifLen)
    {
        if (Message->PifLen >= CommandInfo->PifLen)
        {
            /* Copy the PIF file name */
            RtlMoveMemory(Message->PifFile, CommandInfo->PifFile, CommandInfo->PifLen);
        }
        else Status = STATUS_BUFFER_TOO_SMALL;

        Message->PifLen = CommandInfo->PifLen;
    }

    if (CommandInfo->CurDirectoryLen && Message->CurDirectoryLen)
    {
        if (Message->CurDirectoryLen >= CommandInfo->CurDirectoryLen)
        {
            /* Copy the current directory */
            RtlMoveMemory(Message->CurDirectory, CommandInfo->CurDirectory, CommandInfo->CurDirectoryLen);
        }
        else Status = STATUS_BUFFER_TOO_SMALL;

        Message->CurDirectoryLen = CommandInfo->CurDirectoryLen;
    }

    if (CommandInfo->EnvLen && Message->EnvLen)
    {
        if (Message->EnvLen >= CommandInfo->EnvLen)
        {
            /* Copy the environment */
            RtlMoveMemory(Message->Env, CommandInfo->Env, CommandInfo->EnvLen);
        }
        else Status = STATUS_BUFFER_TOO_SMALL;

        Message->EnvLen = CommandInfo->EnvLen;
    }

    /* Copy the startup info */
    RtlMoveMemory(Message->StartupInfo,
                  &CommandInfo->StartupInfo,
                  sizeof(STARTUPINFOA));

    if (CommandInfo->DesktopLen && Message->DesktopLen)
    {
        if (Message->DesktopLen >= CommandInfo->DesktopLen)
        {
            /* Copy the desktop name */
            RtlMoveMemory(Message->Desktop, CommandInfo->Desktop, CommandInfo->DesktopLen);
        }
        else Status = STATUS_BUFFER_TOO_SMALL;

        Message->DesktopLen = CommandInfo->DesktopLen;
    }

    if (CommandInfo->TitleLen && Message->TitleLen)
    {
        if (Message->TitleLen >= CommandInfo->TitleLen)
        {
            /* Copy the title */
            RtlMoveMemory(Message->Title, CommandInfo->Title, CommandInfo->TitleLen);
        }
        else Status = STATUS_BUFFER_TOO_SMALL;

        Message->TitleLen = CommandInfo->TitleLen;
    }

    if (CommandInfo->ReservedLen && Message->ReservedLen)
    {
        if (Message->ReservedLen >= CommandInfo->ReservedLen)
        {
            /* Copy the reserved parameter */
            RtlMoveMemory(Message->Reserved, CommandInfo->Reserved, CommandInfo->ReservedLen);
        }
        else Status = STATUS_BUFFER_TOO_SMALL;

        Message->ReservedLen = CommandInfo->ReservedLen;
    }

    return Status;
}

VOID NTAPI BaseInitializeVDM(VOID)
{
    /* Initialize the list head */
    InitializeListHead(&VDMConsoleListHead);

    /* Initialize the critical section */
    RtlInitializeCriticalSection(&DosCriticalSection);
    RtlInitializeCriticalSection(&WowCriticalSection);
}

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(BaseSrvCheckVDM)
{
    NTSTATUS Status;
    PBASE_CHECK_VDM CheckVdmRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.CheckVDMRequest;
    PRTL_CRITICAL_SECTION CriticalSection = NULL;
    PVDM_CONSOLE_RECORD ConsoleRecord = NULL;
    PVDM_DOS_RECORD DosRecord = NULL;
    BOOLEAN NewConsoleRecord = FALSE;

    /* Don't do anything if the VDM has been disabled in the registry */
    if (!BaseSrvIsVdmAllowed()) return STATUS_VDM_DISALLOWED;

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

    CriticalSection = (CheckVdmRequest->BinaryType != BINARY_TYPE_SEPARATE_WOW)
                      ? &DosCriticalSection
                      : &WowCriticalSection;

    /* Enter the critical section */
    RtlEnterCriticalSection(CriticalSection);

    /* Check if this is a DOS or WOW VDM */
    if (CheckVdmRequest->BinaryType != BINARY_TYPE_SEPARATE_WOW)
    {
        /* Get the console record */
        Status = BaseSrvGetConsoleRecord(CheckVdmRequest->ConsoleHandle,
                                         &ConsoleRecord);

        if (!NT_SUCCESS(Status))
        {
            /* Allocate a new console record */
            ConsoleRecord = (PVDM_CONSOLE_RECORD)RtlAllocateHeap(BaseSrvHeap,
                                                                 HEAP_ZERO_MEMORY,
                                                                 sizeof(VDM_CONSOLE_RECORD));
            if (ConsoleRecord == NULL)
            {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            /* Remember that the console record was allocated here */
            NewConsoleRecord = TRUE;

            /* Initialize the console record */
            ConsoleRecord->ConsoleHandle = CheckVdmRequest->ConsoleHandle;
            ConsoleRecord->ProcessHandle = NULL;
            ConsoleRecord->ServerEvent = ConsoleRecord->ClientEvent = NULL;
            ConsoleRecord->ReenterCount = 0;
            ConsoleRecord->CurrentDirs = NULL;
            ConsoleRecord->CurDirsLength = 0;
            ConsoleRecord->SessionId = GetNextDosSesId();
            InitializeListHead(&ConsoleRecord->DosListHead);
        }

        /* Allocate a new DOS record */
        DosRecord = (PVDM_DOS_RECORD)RtlAllocateHeap(BaseSrvHeap,
                                                     HEAP_ZERO_MEMORY,
                                                     sizeof(VDM_DOS_RECORD));
        if (DosRecord == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        /* Initialize the DOS record */
        DosRecord->State = VDM_NOT_LOADED;
        DosRecord->ExitCode = 0;

        Status = BaseSrvCreatePairWaitHandles(&DosRecord->ServerEvent, &DosRecord->ClientEvent);
        if (!NT_SUCCESS(Status)) goto Cleanup;

        /* Return the client event handle */
        CheckVdmRequest->WaitObjectForParent = DosRecord->ClientEvent;

        /* Translate the input structure into a VDM command structure and set it in the DOS record */
        if (!BaseSrvCopyCommand(CheckVdmRequest, DosRecord))
        {
            /* The only possibility is that an allocation failure occurred */
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        /* Add the DOS record */
        InsertHeadList(&ConsoleRecord->DosListHead, &DosRecord->Entry);

        if (ConsoleRecord->ServerEvent)
        {
            /* Signal the session event */
            NtSetEvent(ConsoleRecord->ServerEvent, NULL);
        }

        if (NewConsoleRecord)
        {
            /* Add the console record */
            InsertTailList(&VDMConsoleListHead, &ConsoleRecord->Entry);
        }

        if (ConsoleRecord->ConsoleHandle == NULL)
        {
            /* The parent doesn't have a console, so return the session ID */
            CheckVdmRequest->iTask = ConsoleRecord->SessionId;
        }
        else CheckVdmRequest->iTask = 0;

        CheckVdmRequest->VDMState = NewConsoleRecord ? VDM_NOT_LOADED : VDM_READY;
        Status = STATUS_SUCCESS;
    }
    else
    {
        // TODO: NOT IMPLEMENTED
        UNIMPLEMENTED;
        Status = STATUS_NOT_IMPLEMENTED;
    }

Cleanup:
    /* Check if it failed */
    if (!NT_SUCCESS(Status))
    {
        /* Free the DOS record */
        if (DosRecord != NULL)
        {
            if (DosRecord->ServerEvent) NtClose(DosRecord->ServerEvent);
            if (DosRecord->ClientEvent)
            {
                /* Close the remote handle */
                NtDuplicateObject(CsrGetClientThread()->Process->ProcessHandle,
                                  DosRecord->ClientEvent,
                                  NULL,
                                  NULL,
                                  0,
                                  0,
                                  DUPLICATE_CLOSE_SOURCE);
            }

            RtlFreeHeap(BaseSrvHeap, 0, DosRecord);
            DosRecord = NULL;
        }

        /* Free the console record if it was allocated here */
        if (NewConsoleRecord)
        {
            RtlFreeHeap(BaseSrvHeap, 0, ConsoleRecord);
            ConsoleRecord = NULL;
        }
    }

    /* Leave the critical section */
    RtlLeaveCriticalSection(CriticalSection);

    return Status;
}

CSR_API(BaseSrvUpdateVDMEntry)
{
    NTSTATUS Status;
    PBASE_UPDATE_VDM_ENTRY UpdateVdmEntryRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.UpdateVDMEntryRequest;
    PRTL_CRITICAL_SECTION CriticalSection = NULL;
    PVDM_CONSOLE_RECORD ConsoleRecord = NULL;
    PVDM_DOS_RECORD DosRecord = NULL;

    CriticalSection = (UpdateVdmEntryRequest->BinaryType != BINARY_TYPE_SEPARATE_WOW)
                      ? &DosCriticalSection
                      : &WowCriticalSection;

    /* Enter the critical section */
    RtlEnterCriticalSection(CriticalSection);

    /* Check if this is a DOS or WOW VDM */
    if (UpdateVdmEntryRequest->BinaryType != BINARY_TYPE_SEPARATE_WOW)
    {
        if (UpdateVdmEntryRequest->iTask != 0)
        {
            /* Get the console record using the task ID */
            Status = GetConsoleRecordBySessionId(UpdateVdmEntryRequest->iTask,
                                                 &ConsoleRecord);
        }
        else
        {
            /* Get the console record using the console handle */
            Status = BaseSrvGetConsoleRecord(UpdateVdmEntryRequest->ConsoleHandle,
                                             &ConsoleRecord);
        }

        if (!NT_SUCCESS(Status)) goto Cleanup;

        /* Get the primary DOS record */
        DosRecord = (PVDM_DOS_RECORD)CONTAINING_RECORD(ConsoleRecord->DosListHead.Flink,
                                                       VDM_DOS_RECORD,
                                                       Entry);

        switch (UpdateVdmEntryRequest->EntryIndex)
        {
            case VdmEntryUndo:
            {
                /* Close the server event handle, the client will close the client handle */
                NtClose(DosRecord->ServerEvent);
                DosRecord->ServerEvent = DosRecord->ClientEvent = NULL;

                if (UpdateVdmEntryRequest->VDMCreationState & (VDM_UNDO_PARTIAL | VDM_UNDO_FULL))
                {
                    /* Remove the DOS record */
                    if (DosRecord->CommandInfo) BaseSrvFreeVDMInfo(DosRecord->CommandInfo);
                    RemoveEntryList(&DosRecord->Entry);
                    RtlFreeHeap(BaseSrvHeap, 0, DosRecord);

                    /*
                     * Since this is an undo, if that was the only DOS record the VDM
                     * won't even start, so the console record should be removed too.
                     */
                    if (ConsoleRecord->DosListHead.Flink == &ConsoleRecord->DosListHead)
                    {
                        if (ConsoleRecord->ProcessHandle) NtClose(ConsoleRecord->ProcessHandle);
                        if (ConsoleRecord->ServerEvent) NtClose(ConsoleRecord->ServerEvent);
                        RemoveEntryList(&ConsoleRecord->Entry);
                        RtlFreeHeap(BaseSrvHeap, 0, ConsoleRecord);
                    }
                }

                /* It was successful */
                Status = STATUS_SUCCESS;

                break;
            }

            case VdmEntryUpdateProcess:
            {
                /* Duplicate the VDM process handle */
                Status = NtDuplicateObject(CsrGetClientThread()->Process->ProcessHandle,
                                           UpdateVdmEntryRequest->VDMProcessHandle,
                                           NtCurrentProcess(),
                                           &ConsoleRecord->ProcessHandle,
                                           0,
                                           0,
                                           DUPLICATE_SAME_ATTRIBUTES | DUPLICATE_SAME_ACCESS);
                if (!NT_SUCCESS(Status)) goto Cleanup;

                /* Create a pair of handles to one event object */
                Status = BaseSrvCreatePairWaitHandles(&DosRecord->ServerEvent,
                                                      &DosRecord->ClientEvent);
                if (!NT_SUCCESS(Status)) goto Cleanup;

                /* Return the client event handle */
                UpdateVdmEntryRequest->WaitObjectForParent = DosRecord->ClientEvent;

                break;
            }

            case VdmEntryUpdateControlCHandler:
            {
                // TODO: NOT IMPLEMENTED
                DPRINT1("BaseSrvUpdateVDMEntry: VdmEntryUpdateControlCHandler not implemented!");
                Status = STATUS_NOT_IMPLEMENTED;

                break;
            }

            default:
            {
                /* Invalid */
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }
    else
    {
        // TODO: NOT IMPLEMENTED
        UNIMPLEMENTED;
        Status = STATUS_NOT_IMPLEMENTED;
    }

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(CriticalSection);

    return Status;
}

CSR_API(BaseSrvGetNextVDMCommand)
{
    NTSTATUS Status;
    PBASE_GET_NEXT_VDM_COMMAND GetNextVdmCommandRequest =
    &((PBASE_API_MESSAGE)ApiMessage)->Data.GetNextVDMCommandRequest;
    PRTL_CRITICAL_SECTION CriticalSection;
    PLIST_ENTRY i = NULL;
    PVDM_CONSOLE_RECORD ConsoleRecord = NULL;
    PVDM_DOS_RECORD DosRecord = NULL;

    /* Validate the message buffers */
    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&GetNextVdmCommandRequest->CmdLine,
                                  GetNextVdmCommandRequest->CmdLen,
                                  sizeof(*GetNextVdmCommandRequest->CmdLine))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&GetNextVdmCommandRequest->AppName,
                                     GetNextVdmCommandRequest->AppLen,
                                     sizeof(*GetNextVdmCommandRequest->AppName))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&GetNextVdmCommandRequest->PifFile,
                                     GetNextVdmCommandRequest->PifLen,
                                     sizeof(*GetNextVdmCommandRequest->PifFile))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&GetNextVdmCommandRequest->CurDirectory,
                                     GetNextVdmCommandRequest->CurDirectoryLen,
                                     sizeof(*GetNextVdmCommandRequest->CurDirectory))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&GetNextVdmCommandRequest->Env,
                                     GetNextVdmCommandRequest->EnvLen,
                                     sizeof(*GetNextVdmCommandRequest->Env))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&GetNextVdmCommandRequest->Desktop,
                                     GetNextVdmCommandRequest->DesktopLen,
                                     sizeof(*GetNextVdmCommandRequest->Desktop))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&GetNextVdmCommandRequest->Title,
                                     GetNextVdmCommandRequest->TitleLen,
                                     sizeof(*GetNextVdmCommandRequest->Title))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&GetNextVdmCommandRequest->Reserved,
                                     GetNextVdmCommandRequest->ReservedLen,
                                     sizeof(*GetNextVdmCommandRequest->Reserved))
        || !CsrValidateMessageBuffer(ApiMessage,
                                     (PVOID*)&GetNextVdmCommandRequest->StartupInfo,
                                     1,
                                     sizeof(STARTUPINFOA)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    CriticalSection = (GetNextVdmCommandRequest->VDMState & VDM_FLAG_WOW)
                      ? &WowCriticalSection
                      : &DosCriticalSection;

    /* Enter the critical section */
    RtlEnterCriticalSection(CriticalSection);

    if (!(GetNextVdmCommandRequest->VDMState & VDM_FLAG_WOW))
    {
        if (GetNextVdmCommandRequest->iTask != 0)
        {
            /* Get the console record using the task ID */
            Status = GetConsoleRecordBySessionId(GetNextVdmCommandRequest->iTask,
                                                 &ConsoleRecord);
        }
        else
        {
            /* Get the console record using the console handle */
            Status = BaseSrvGetConsoleRecord(GetNextVdmCommandRequest->ConsoleHandle,
                                             &ConsoleRecord);
        }

        /* Make sure we found the console record */
        if (!NT_SUCCESS(Status)) goto Cleanup;

        /* Return the session ID */
        GetNextVdmCommandRequest->iTask = ConsoleRecord->SessionId;
        GetNextVdmCommandRequest->WaitObjectForVDM = NULL;

        if (GetNextVdmCommandRequest->VDMState & VDM_GET_FIRST_COMMAND)
        {
            /* Check if the DOS record list is empty */
            if (ConsoleRecord->DosListHead.Flink == &ConsoleRecord->DosListHead)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            /* Get the first DOS record */
            DosRecord = CONTAINING_RECORD(ConsoleRecord->DosListHead.Flink, VDM_DOS_RECORD, Entry);

            /* Make sure its command information is still there */
            if (DosRecord->CommandInfo == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            /* Check if the console handle hasn't been set yet */
            if (ConsoleRecord->ConsoleHandle == NULL)
            {
                /* Set it now */
                ConsoleRecord->ConsoleHandle = GetNextVdmCommandRequest->ConsoleHandle;
            }

            /* Fill the command information */
            Status = BaseSrvFillCommandInfo(DosRecord->CommandInfo, GetNextVdmCommandRequest);
            goto Cleanup;
        }

        /* Check if we should set the state of a running DOS record to ready */
        if (!(GetNextVdmCommandRequest->VDMState
            & (VDM_FLAG_FIRST_TASK | VDM_FLAG_RETRY | VDM_FLAG_NESTED_TASK)))
        {
            /* Search for a DOS record that is currently running */
            for (i = ConsoleRecord->DosListHead.Flink; i != &ConsoleRecord->DosListHead; i = i->Flink)
            {
                DosRecord = CONTAINING_RECORD(i, VDM_DOS_RECORD, Entry);
                if (DosRecord->State == VDM_NOT_READY) break;
            }

            /* Check if we found any */
            if (i == &ConsoleRecord->DosListHead)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            /* Set the exit code */
            DosRecord->ExitCode = GetNextVdmCommandRequest->ExitCode;

            /* Update the VDM state */
            DosRecord->State = VDM_READY;

            /* Notify all waiting threads that the task is finished */
            NtSetEvent(DosRecord->ServerEvent, NULL);
            NtClose(DosRecord->ServerEvent);
            DosRecord->ServerEvent = NULL;
        }

        /* Search for a DOS record that isn't loaded yet */
        for (i = ConsoleRecord->DosListHead.Flink; i != &ConsoleRecord->DosListHead; i = i->Flink)
        {
            DosRecord = CONTAINING_RECORD(i, VDM_DOS_RECORD, Entry);
            if (DosRecord->State == VDM_NOT_LOADED) break;
        }

        if (i != &ConsoleRecord->DosListHead)
        {
            /* DOS tasks which haven't been loaded yet should have a command info structure */
            ASSERT(DosRecord->CommandInfo != NULL);

            /* Check if the caller only wants environment data */
            if (GetNextVdmCommandRequest->VDMState & VDM_GET_ENVIRONMENT)
            {
                if (GetNextVdmCommandRequest->EnvLen < DosRecord->CommandInfo->EnvLen)
                {
                    /* Not enough space was reserved */
                    GetNextVdmCommandRequest->EnvLen = DosRecord->CommandInfo->EnvLen;
                    Status = STATUS_BUFFER_OVERFLOW;
                    goto Cleanup;
                }

                /* Copy the environment data */
                RtlMoveMemory(GetNextVdmCommandRequest->Env,
                              DosRecord->CommandInfo->Env,
                              DosRecord->CommandInfo->EnvLen);

                /* Return the actual size to the caller */
                GetNextVdmCommandRequest->EnvLen = DosRecord->CommandInfo->EnvLen;
            }
            else
            {
                /* Fill the command information */
                Status = BaseSrvFillCommandInfo(DosRecord->CommandInfo, GetNextVdmCommandRequest);
                if (!NT_SUCCESS(Status)) goto Cleanup;

                /* Free the command information, it's no longer needed */
                BaseSrvFreeVDMInfo(DosRecord->CommandInfo);
                DosRecord->CommandInfo = NULL;

                /* Update the VDM state */
                GetNextVdmCommandRequest->VDMState = DosRecord->State = VDM_NOT_READY;
            }

            Status = STATUS_SUCCESS;
            goto Cleanup;
        }
    }
    else
    {
        // TODO: WOW SUPPORT NOT IMPLEMENTED
        Status = STATUS_NOT_IMPLEMENTED;
        goto Cleanup;
    }

    /* There is no command yet */
    if (!(GetNextVdmCommandRequest->VDMState & VDM_FLAG_DONT_WAIT))
    {
        if (ConsoleRecord->ServerEvent)
        {
            /* Reset the event */
            NtResetEvent(ConsoleRecord->ServerEvent, NULL);
        }
        else
        {
            /* Create a pair of wait handles */
            Status = BaseSrvCreatePairWaitHandles(&ConsoleRecord->ServerEvent,
                                                  &ConsoleRecord->ClientEvent);
            if (!NT_SUCCESS(Status)) goto Cleanup;
        }

        /* Return the client event handle */
        GetNextVdmCommandRequest->WaitObjectForVDM = ConsoleRecord->ClientEvent;
    }

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(CriticalSection);

    return Status;
}

CSR_API(BaseSrvExitVDM)
{
    NTSTATUS Status;
    PBASE_EXIT_VDM ExitVdmRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.ExitVDMRequest;
    PRTL_CRITICAL_SECTION CriticalSection = NULL;
    PVDM_CONSOLE_RECORD ConsoleRecord = NULL;
    PVDM_DOS_RECORD DosRecord;

    CriticalSection = (ExitVdmRequest->iWowTask == 0)
                      ? &DosCriticalSection
                      : &WowCriticalSection;

    /* Enter the critical section */
    RtlEnterCriticalSection(CriticalSection);

    if (ExitVdmRequest->iWowTask == 0)
    {
        /* Get the console record */
        Status = BaseSrvGetConsoleRecord(ExitVdmRequest->ConsoleHandle, &ConsoleRecord);
        if (!NT_SUCCESS(Status)) goto Cleanup;

        /* Cleanup the DOS records */
        while (ConsoleRecord->DosListHead.Flink != &ConsoleRecord->DosListHead)
        {
            DosRecord = CONTAINING_RECORD(ConsoleRecord->DosListHead.Flink,
                                          VDM_DOS_RECORD,
                                          Entry);

            /* Set the event and close it */
            NtSetEvent(DosRecord->ServerEvent, NULL);
            NtClose(DosRecord->ServerEvent);

            /* Remove the DOS entry */
            if (DosRecord->CommandInfo) BaseSrvFreeVDMInfo(DosRecord->CommandInfo);
            RemoveEntryList(&DosRecord->Entry);
            RtlFreeHeap(BaseSrvHeap, 0, DosRecord);
        }

        if (ConsoleRecord->CurrentDirs != NULL)
        {
            /* Free the current directories */
            RtlFreeHeap(BaseSrvHeap, 0, ConsoleRecord->CurrentDirs);
            ConsoleRecord->CurrentDirs = NULL;
            ConsoleRecord->CurDirsLength = 0;
        }

        /* Close the event handle */
        if (ConsoleRecord->ServerEvent) NtClose(ConsoleRecord->ServerEvent);

        /* Remove the console record */
        RemoveEntryList(&ConsoleRecord->Entry);
        RtlFreeHeap(BaseSrvHeap, 0, ConsoleRecord);
    }
    else
    {
        // TODO: NOT IMPLEMENTED
        UNIMPLEMENTED;
        Status = STATUS_NOT_IMPLEMENTED;
    }

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(CriticalSection);

    return Status;
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
    NTSTATUS Status;
    PBASE_GET_VDM_EXIT_CODE GetVDMExitCodeRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.GetVDMExitCodeRequest;
    PLIST_ENTRY i = NULL;
    PVDM_CONSOLE_RECORD ConsoleRecord = NULL;
    PVDM_DOS_RECORD DosRecord = NULL;

    /* Enter the critical section */
    RtlEnterCriticalSection(&DosCriticalSection);

    /* Get the console record */
    Status = BaseSrvGetConsoleRecord(GetVDMExitCodeRequest->ConsoleHandle, &ConsoleRecord);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Search for a DOS record that has the same parent process handle */
    for (i = ConsoleRecord->DosListHead.Flink; i != &ConsoleRecord->DosListHead; i = i->Flink)
    {
        DosRecord = CONTAINING_RECORD(i, VDM_DOS_RECORD, Entry);
        if (DosRecord->ClientEvent == GetVDMExitCodeRequest->hParent) break;
    }

    /* Check if no DOS record was found */
    if (i == &ConsoleRecord->DosListHead)
    {
        Status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    /* Check if this task is still running */
    if (DosRecord->State != VDM_READY)
    {
        GetVDMExitCodeRequest->ExitCode = STATUS_PENDING;
        goto Cleanup;
    }

    /* Return the exit code */
    GetVDMExitCodeRequest->ExitCode = DosRecord->ExitCode;

    /* Since this is a zombie task record, remove it */
    if (DosRecord->CommandInfo) BaseSrvFreeVDMInfo(DosRecord->CommandInfo);
    RemoveEntryList(&DosRecord->Entry);
    RtlFreeHeap(BaseSrvHeap, 0, DosRecord);

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(&DosCriticalSection);

    return Status;
}

CSR_API(BaseSrvSetReenterCount)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PBASE_SET_REENTER_COUNT SetReenterCountRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.SetReenterCountRequest;
    PVDM_CONSOLE_RECORD ConsoleRecord;

    /* Enter the critical section */
    RtlEnterCriticalSection(&DosCriticalSection);

    /* Get the console record */
    Status = BaseSrvGetConsoleRecord(SetReenterCountRequest->ConsoleHandle, &ConsoleRecord);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    if (SetReenterCountRequest->fIncDec == VDM_INC_REENTER_COUNT) ConsoleRecord->ReenterCount++;
    else if (SetReenterCountRequest->fIncDec == VDM_DEC_REENTER_COUNT)
    {
        ConsoleRecord->ReenterCount--;
        if (ConsoleRecord->ServerEvent != NULL) NtSetEvent(ConsoleRecord->ServerEvent, NULL);
    }
    else Status = STATUS_INVALID_PARAMETER;

Cleanup:
    /* Leave the critical section */
    RtlLeaveCriticalSection(&DosCriticalSection);

    return Status;
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

    /* Return the actual size of the current directory information */
    VDMCurrentDirsRequest->cchCurDirs = ConsoleRecord->CurDirsLength;

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
