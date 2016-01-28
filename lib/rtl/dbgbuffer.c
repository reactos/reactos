/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/dbgbuffer.c
 * PROGRAMER:       James Tabor
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
PRTL_DEBUG_INFORMATION
NTAPI
RtlCreateQueryDebugBuffer(IN ULONG Size,
                          IN BOOLEAN EventPair)
{
    NTSTATUS Status;
    PRTL_DEBUG_INFORMATION Buf = NULL;
    SIZE_T ViewSize = 100 * PAGE_SIZE;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&Buf,
                                     0,
                                     &ViewSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) return NULL;

    Buf->ViewBaseClient = Buf;
    Buf->ViewSize = (ULONG)ViewSize;

    DPRINT("RtlCQDB: BA: %p BS: 0x%lx\n", Buf->ViewBaseClient, Buf->ViewSize);

    return Buf;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(IN PRTL_DEBUG_INFORMATION Buf)
{
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T ViewSize = 0;

    if (NULL != Buf)
    {
        Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&Buf,
                                     &ViewSize,
                                     MEM_RELEASE);
    }
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlDQDB: Failed to free VM!\n");
    }
    return Status;
}

/*
 *  Based on lib/epsapi/enum/modules.c by KJK::Hyperion.
 */
NTSTATUS
NTAPI
RtlpQueryRemoteProcessModules(HANDLE ProcessHandle,
                              IN PRTL_PROCESS_MODULES Modules OPTIONAL,
                              IN ULONG Size OPTIONAL,
                              OUT PULONG ReturnedSize)
{
    PROCESS_BASIC_INFORMATION pbiInfo;
    PPEB_LDR_DATA ppldLdrData;
    LDR_DATA_TABLE_ENTRY lmModule;
    PLIST_ENTRY pleListHead;
    PLIST_ENTRY pleCurEntry;

    PRTL_PROCESS_MODULE_INFORMATION ModulePtr = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG UsedSize = sizeof(ULONG);
    ANSI_STRING AnsiString;
    PCHAR p;

    DPRINT("RtlpQueryRemoteProcessModules Start\n");

    /* query the process basic information (includes the PEB address) */
    Status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessBasicInformation,
                                       &pbiInfo,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL);

    if (!NT_SUCCESS(Status))
    {
        /* failure */
        DPRINT("NtQueryInformationProcess 1 0x%lx \n", Status);
        return Status;
    }

    if (Modules == NULL || Size == 0)
    {
        Status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else
    {
        Modules->NumberOfModules = 0;
        ModulePtr = &Modules->Modules[0];
        Status = STATUS_SUCCESS;
    }

    /* get the address of the PE Loader data */
    Status = NtReadVirtualMemory(ProcessHandle,
                                 &(pbiInfo.PebBaseAddress->Ldr),
                                 &ppldLdrData,
                                 sizeof(ppldLdrData),
                                 NULL);

    if (!NT_SUCCESS(Status))
    {
        /* failure */
        DPRINT("NtReadVirtualMemory 1 0x%lx \n", Status);
        return Status;
    }


    /* head of the module list: the last element in the list will point to this */
    pleListHead = &ppldLdrData->InLoadOrderModuleList;

    /* get the address of the first element in the list */
    Status = NtReadVirtualMemory(ProcessHandle,
                                 &(ppldLdrData->InLoadOrderModuleList.Flink),
                                 &pleCurEntry,
                                 sizeof(pleCurEntry),
                                 NULL);

    if (!NT_SUCCESS(Status))
    {
        /* failure */
        DPRINT("NtReadVirtualMemory 2 0x%lx \n", Status);
        return Status;
    }

    while(pleCurEntry != pleListHead)
    {
        UNICODE_STRING Unicode;
        WCHAR  Buffer[256 * sizeof(WCHAR)];

        /* read the current module */
        Status = NtReadVirtualMemory(ProcessHandle,
                                     CONTAINING_RECORD(pleCurEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks),
                                     &lmModule,
                                     sizeof(LDR_DATA_TABLE_ENTRY),
                                     NULL);

        if (!NT_SUCCESS(Status))
        {
            /* failure */
            DPRINT( "NtReadVirtualMemory 3 0x%lx \n", Status);
            return Status;
        }

        /* Import module name from remote Process user space. */
        Unicode.Length = lmModule.FullDllName.Length;
        Unicode.MaximumLength = lmModule.FullDllName.MaximumLength;
        Unicode.Buffer = Buffer;

        Status = NtReadVirtualMemory(ProcessHandle,
                                     lmModule.FullDllName.Buffer,
                                     Unicode.Buffer,
                                     Unicode.Length,
                                     NULL);

        if (!NT_SUCCESS(Status))
        {
            /* failure */
            DPRINT( "NtReadVirtualMemory 3 0x%lx \n", Status);
            return Status;
        }

        DPRINT("  Module %wZ\n", &Unicode);

        if (UsedSize > Size)
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else if (Modules != NULL)
        {
            ModulePtr->Section        = 0;
            ModulePtr->MappedBase     = NULL;      // FIXME: ??
            ModulePtr->ImageBase      = lmModule.DllBase;
            ModulePtr->ImageSize      = lmModule.SizeOfImage;
            ModulePtr->Flags          = lmModule.Flags;
            ModulePtr->LoadOrderIndex = 0;      // FIXME:  ??
            ModulePtr->InitOrderIndex = 0;      // FIXME: ??
            ModulePtr->LoadCount      = lmModule.LoadCount;

            AnsiString.Length        = 0;
            AnsiString.MaximumLength = 256;
            AnsiString.Buffer        = ModulePtr->FullPathName;
            RtlUnicodeStringToAnsiString(&AnsiString,
                                         &Unicode,
                                         FALSE);

            p = strrchr(ModulePtr->FullPathName, '\\');
            if (p != NULL)
                ModulePtr->OffsetToFileName = (USHORT)(p - ModulePtr->FullPathName + 1);
            else
                ModulePtr->OffsetToFileName = 0;

            ModulePtr++;
            Modules->NumberOfModules++;
        }
        UsedSize += sizeof(RTL_PROCESS_MODULE_INFORMATION);

        /* address of the next module in the list */
        pleCurEntry = lmModule.InLoadOrderLinks.Flink;
    }

    if (ReturnedSize != 0)
        *ReturnedSize = UsedSize;

    DPRINT("RtlpQueryRemoteProcessModules End\n");

    /* success */
    return (STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(IN ULONG ProcessId,
                                IN ULONG DebugInfoMask,
                                IN OUT PRTL_DEBUG_INFORMATION Buf)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Pid = (ULONG)(ULONG_PTR) NtCurrentTeb()->ClientId.UniqueProcess;

    Buf->Flags = DebugInfoMask;
    Buf->OffsetFree = sizeof(RTL_DEBUG_INFORMATION);

    DPRINT("QueryProcessDebugInformation Start\n");

    /*
    Currently ROS can not read-only from kenrel space, and doesn't
    check for boundaries inside kernel space that are page protected
    from every one but the kernel. aka page 0 - 2
    */
    if (ProcessId <= 1)
    {
        Status = STATUS_ACCESS_VIOLATION;
    }
    else
        if (Pid == ProcessId)
        {
            if (DebugInfoMask & RTL_DEBUG_QUERY_MODULES)
            {
                PRTL_PROCESS_MODULES Mp;
                ULONG ReturnSize = 0;
                ULONG MSize;

                Mp = (PRTL_PROCESS_MODULES)((PUCHAR)Buf + Buf->OffsetFree);

                /* I like this better than the do & while loop. */
                Status = LdrQueryProcessModuleInformation(NULL,
                                                          0,
                                                          &ReturnSize);
                Status = LdrQueryProcessModuleInformation(Mp,
                                                          ReturnSize ,
                                                          &ReturnSize);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }

                MSize = Mp->NumberOfModules * (sizeof(RTL_PROCESS_MODULES) + 8);
                Buf->Modules = Mp;
                Buf->OffsetFree = Buf->OffsetFree + MSize;
            }

            if (DebugInfoMask & RTL_DEBUG_QUERY_HEAPS)
            {
                PRTL_PROCESS_HEAPS Hp;
                ULONG HSize;

                Hp = (PRTL_PROCESS_HEAPS)((PUCHAR)Buf + Buf->OffsetFree);
                HSize = sizeof(RTL_PROCESS_HEAPS);
                if (DebugInfoMask & RTL_DEBUG_QUERY_HEAP_TAGS)
                {
                    // TODO
                }
                if (DebugInfoMask & RTL_DEBUG_QUERY_HEAP_BLOCKS)
                {
                    // TODO
                }
                Buf->Heaps = Hp;
                Buf->OffsetFree = Buf->OffsetFree + HSize;

            }

            if (DebugInfoMask & RTL_DEBUG_QUERY_LOCKS)
            {
                PRTL_PROCESS_LOCKS Lp;
                ULONG LSize;

                Lp = (PRTL_PROCESS_LOCKS)((PUCHAR)Buf + Buf->OffsetFree);
                LSize = sizeof(RTL_PROCESS_LOCKS);
                Buf->Locks = Lp;
                Buf->OffsetFree = Buf->OffsetFree + LSize;
            }

            DPRINT("QueryProcessDebugInformation end \n");
            DPRINT("QueryDebugInfo : 0x%lx\n", Buf->OffsetFree);
        }
        else
        {
            HANDLE hProcess;
            CLIENT_ID ClientId;
            OBJECT_ATTRIBUTES ObjectAttributes;

            Buf->TargetProcessHandle = NtCurrentProcess();

            ClientId.UniqueThread = 0;
            ClientId.UniqueProcess = (HANDLE)(ULONG_PTR)ProcessId;
            InitializeObjectAttributes(&ObjectAttributes,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL);

            Status = NtOpenProcess(&hProcess,
                                   (PROCESS_ALL_ACCESS),
                                   &ObjectAttributes,
                                   &ClientId );
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            if (DebugInfoMask & RTL_DEBUG_QUERY_MODULES)
            {
                PRTL_PROCESS_MODULES Mp;
                ULONG ReturnSize = 0;
                ULONG MSize;

                Mp = (PRTL_PROCESS_MODULES)((PUCHAR)Buf + Buf->OffsetFree);

                Status = RtlpQueryRemoteProcessModules(hProcess,
                                                       NULL,
                                                       0,
                                                       &ReturnSize);

                Status = RtlpQueryRemoteProcessModules(hProcess,
                                                       Mp,
                                                       ReturnSize ,
                                                       &ReturnSize);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }

                MSize = Mp->NumberOfModules * (sizeof(RTL_PROCESS_MODULES) + 8);
                Buf->Modules = Mp;
                Buf->OffsetFree = Buf->OffsetFree + MSize;
            }

            if (DebugInfoMask & RTL_DEBUG_QUERY_HEAPS)
            {
                PRTL_PROCESS_HEAPS Hp;
                ULONG HSize;

                Hp = (PRTL_PROCESS_HEAPS)((PUCHAR)Buf + Buf->OffsetFree);
                HSize = sizeof(RTL_PROCESS_HEAPS);
                if (DebugInfoMask & RTL_DEBUG_QUERY_HEAP_TAGS)
                {
                    // TODO
                }
                if (DebugInfoMask & RTL_DEBUG_QUERY_HEAP_BLOCKS)
                {
                    // TODO
                }
                Buf->Heaps = Hp;
                Buf->OffsetFree = Buf->OffsetFree + HSize;

            }

            if (DebugInfoMask & RTL_DEBUG_QUERY_LOCKS)
            {
                PRTL_PROCESS_LOCKS Lp;
                ULONG LSize;

                Lp = (PRTL_PROCESS_LOCKS)((PUCHAR)Buf + Buf->OffsetFree);
                LSize = sizeof(RTL_PROCESS_LOCKS);
                Buf->Locks = Lp;
                Buf->OffsetFree = Buf->OffsetFree + LSize;
            }

            DPRINT("QueryProcessDebugInformation end \n");
            DPRINT("QueryDebugInfo : 0x%lx\n", Buf->OffsetFree);
        }

    return Status;
}

/* EOL */
