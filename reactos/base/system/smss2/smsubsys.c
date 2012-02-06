/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

RTL_CRITICAL_SECTION SmpKnownSubSysLock;
LIST_ENTRY SmpKnownSubSysHead;
HANDLE SmpWindowsSubSysProcess;
HANDLE SmpWindowsSubSysProcessId;
BOOLEAN RegPosixSingleInstance;
WCHAR InitialCommandBuffer[256];

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpLoadSubSystem(IN PUNICODE_STRING FileName,
                 IN PUNICODE_STRING Directory,
                 IN PUNICODE_STRING CommandLine,
                 IN ULONG MuSessionId,
                 OUT PHANDLE ProcessId)
{
    DPRINT1("Should start subsystem %wZ for Session: %lx\n", FileName, MuSessionId);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpLoadSubSystemsForMuSession(IN PULONG MuSessionId,
                              OUT PHANDLE ProcessId,
                              IN PUNICODE_STRING InitialCommand)
{
    NTSTATUS Status = STATUS_SUCCESS, Status2;
    PSMP_REGISTRY_VALUE RegEntry;
    UNICODE_STRING DestinationString, NtPath;
    PLIST_ENTRY NextEntry;
    LARGE_INTEGER Timeout;
    PVOID State;

    /* Write a few last registry keys with the boot partition information */
    SmpTranslateSystemPartitionInformation();

    /* Process "SetupExecute" values */
    NextEntry = SmpSetupExecuteList.Flink;
    while (NextEntry != &SmpSetupExecuteList)
    {
        /* Execute each one and move on */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        DPRINT1("SetupExecute entry: %wZ\n", &RegEntry->Name);
        SmpExecuteCommand(&RegEntry->Name, 0, NULL, 0);
        NextEntry = NextEntry->Flink;
    }

    /* Now process the subsystems */
    NextEntry = SmpSubSystemList.Flink;
    while (NextEntry != &SmpSubSystemList)
    {
        /* Get the entry and check if this is the special Win32k entry */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        DPRINT1("Subsystem: %wZ\n", &RegEntry->Name);
        if (!_wcsicmp(RegEntry->Name.Buffer, L"Kmode"))
        {
            /* Translate it */
            if (!RtlDosPathNameToNtPathName_U(RegEntry->Value.Buffer,
                                              &NtPath,
                                              NULL,
                                              NULL))
            {
                Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
                DPRINT1("Failed: %lx\n", Status);
            }
            else
            {
                /* Get the driver privilege */
                Status = SmpAcquirePrivilege(SE_LOAD_DRIVER_PRIVILEGE, &State);
                if (NT_SUCCESS(Status))
                {
                    /* Create the new session */
                    ASSERT(AttachedSessionId == -1);
                    Status = NtSetSystemInformation(SystemSessionCreate,
                                                    MuSessionId,
                                                    sizeof(*MuSessionId));
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("SMSS: Session space creation failed\n");
                        SmpReleasePrivilege(State);
                        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPath.Buffer);
                        return Status;
                    }
                    AttachedSessionId = *MuSessionId;

                    /* Start Win32k.sys on this session */
                    DPRINT1("Starting win32k.sys...\n");
                    RtlInitUnicodeString(&DestinationString,
                                         L"\\SystemRoot\\System32\\win32k.sys");
                    Status = NtSetSystemInformation(SystemExtendServiceTableInformation,
                                                    &DestinationString,
                                                    sizeof(DestinationString));
                    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPath.Buffer);
                    SmpReleasePrivilege(State);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("SMSS: Load of WIN32K failed.\n");
                        return Status;
                    }
                }
            }
        }

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Now parse the required subsystem list */
    NextEntry = SmpSubSystemsToLoad.Flink;
    while (NextEntry != &SmpSubSystemsToLoad)
    {
        /* Get each entry and check if it's the internal debug or not */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        DPRINT1("Subsystem (required): %wZ\n", &RegEntry->Name);
        if (_wcsicmp(RegEntry->Name.Buffer, L"debug"))
        {
            /* Load the required subsystem */
            Status = SmpExecuteCommand(&RegEntry->Value,
                                       *MuSessionId,
                                       ProcessId,
                                       SMP_SUBSYSTEM_FLAG);
        }
        else
        {
            /* Load the internal debug system */
            Status = SmpExecuteCommand(&RegEntry->Value,
                                       *MuSessionId,
                                       ProcessId,
                                       SMP_DEBUG_FLAG | SMP_SUBSYSTEM_FLAG);
        }
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("SMSS: Subsystem execute failed (%WZ)\n", &RegEntry->Value);
            return Status;
        }

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Process the "Execute" list now */
    NextEntry = SmpExecuteList.Blink;
    if (NextEntry != &SmpExecuteList)
    {
        /* Get the custom initial command */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        DPRINT1("Initial command found: %wZ\n", &RegEntry->Name);

        /* Write the initial command and wait for 5 seconds (why??!) */
        *InitialCommand = RegEntry->Name;
        Timeout.QuadPart = -50000000;
        NtDelayExecution(FALSE, &Timeout);
    }
    else
    {
        /* Use the default Winlogon initial command */
        RtlInitUnicodeString(InitialCommand, L"winlogon.exe");
        InitialCommandBuffer[0] = UNICODE_NULL;
        DPRINT1("Initial command found: %wZ\n", InitialCommand);

        /* Check if there's a debugger for Winlogon */
        Status2 = LdrQueryImageFileExecutionOptions(InitialCommand,
                                                    L"Debugger",
                                                    REG_SZ,
                                                    InitialCommandBuffer,
                                                    sizeof(InitialCommandBuffer) -
                                                    InitialCommand->Length,
                                                    NULL);
        if ((NT_SUCCESS(Status2)) && (InitialCommandBuffer[0]))
        {
            /* Put the debugger string with the Winlogon string */
            wcscat(InitialCommandBuffer, L" ");
            wcscat(InitialCommandBuffer, InitialCommand->Buffer);
            RtlInitUnicodeString(InitialCommand, InitialCommandBuffer);
        }
    }

    /* Finally check if there was a custom initial command */
    NextEntry = SmpExecuteList.Flink;
    while (NextEntry != &SmpExecuteList)
    {
        /* Execute each one */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        DPRINT1("Initial command (custom): %wZ\n", &RegEntry->Name);
        SmpExecuteCommand(&RegEntry->Name, *MuSessionId, NULL, 0);
        NextEntry = NextEntry->Flink;
    }

    /* Return status */
    return Status;
}

