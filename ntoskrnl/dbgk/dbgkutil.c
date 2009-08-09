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
#include <debug.h>

/* FUNCTIONS *****************************************************************/

HANDLE
NTAPI
DbgkpSectionToFileHandle(IN PVOID Section)
{
    NTSTATUS Status;
    POBJECT_NAME_INFORMATION FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    PAGED_CODE();

    /* Get the filename of the section */
    Status = MmGetFileNameForSection(Section, &FileName);
    if (!NT_SUCCESS(Status)) return NULL;

    /* Initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName->Name,
                               OBJ_CASE_INSENSITIVE |
                               OBJ_FORCE_ACCESS_CHECK |
                               OBJ_KERNEL_HANDLE,
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
    ExFreePool(FileName);
    if (!NT_SUCCESS(Status)) return NULL;
    return Handle;
}

BOOLEAN
NTAPI
DbgkpSuspendProcess(VOID)
{
    PAGED_CODE();

    /* Make sure this isn't a deleted process */
    if (!PsGetCurrentProcess()->ProcessDelete)
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
DbgkCreateThread(IN PETHREAD Thread,
                 IN PVOID StartAddress)
{
    PEPROCESS Process = PsGetCurrentProcess();
    ULONG ProcessFlags;
    IMAGE_INFO ImageInfo;
    PIMAGE_NT_HEADERS NtHeader;
    POBJECT_NAME_INFORMATION ModuleName;
    UNICODE_STRING NtDllName;
    NTSTATUS Status;
    PVOID DebugPort;
    DBGKM_MSG ApiMessage;
    PDBGKM_CREATE_THREAD CreateThread = &ApiMessage.CreateThread;
    PDBGKM_CREATE_PROCESS CreateProcess = &ApiMessage.CreateProcess;
    PDBGKM_LOAD_DLL LoadDll = &ApiMessage.LoadDll;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PTEB Teb;
    PAGED_CODE();

    /* Sanity check */
    ASSERT(Thread == PsGetCurrentThread());

    /* Try ORing in the create reported and image notify flags */
    ProcessFlags = PspSetProcessFlag(Process,
                                     PSF_CREATE_REPORTED_BIT |
                                     PSF_IMAGE_NOTIFY_DONE_BIT);

    /* Check if we were the first to set them or if another thread raced us */
    if (!(ProcessFlags & PSF_IMAGE_NOTIFY_DONE_BIT) && (PsImageNotifyEnabled))
    {
        /* It hasn't.. set up the image info for the process */
        ImageInfo.Properties = 0;
        ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
        ImageInfo.ImageBase = Process->SectionBaseAddress;
        ImageInfo.ImageSize = 0;
        ImageInfo.ImageSelector = 0;
        ImageInfo.ImageSectionNumber = 0;

        /* Get the NT Headers */
        NtHeader = RtlImageNtHeader(Process->SectionBaseAddress);
        if (NtHeader)
        {
            /* Set image size */
            ImageInfo.ImageSize = NtHeader->OptionalHeader.SizeOfImage;
        }

        /* Get the image name */
        Status = MmGetFileNameForSection(Process->SectionObject, &ModuleName);
        if (NT_SUCCESS(Status))
        {
            /* Call the notify routines and free the name */
            PspRunLoadImageNotifyRoutines(&ModuleName->Name,
                                          Process->UniqueProcessId,
                                          &ImageInfo);
            ExFreePool(ModuleName);
        }
        else
        {
            /* Call the notify routines */
            PspRunLoadImageNotifyRoutines(NULL,
                                          Process->UniqueProcessId,
                                          &ImageInfo);
        }

        /* Setup the info for ntdll.dll */
        ImageInfo.Properties = 0;
        ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
        ImageInfo.ImageBase = PspSystemDllBase;
        ImageInfo.ImageSize = 0;
        ImageInfo.ImageSelector = 0;
        ImageInfo.ImageSectionNumber = 0;

        /* Get the NT Headers */
        NtHeader = RtlImageNtHeader(PspSystemDllBase);
        if (NtHeader)
        {
            /* Set image size */
            ImageInfo.ImageSize = NtHeader->OptionalHeader.SizeOfImage;
        }

        /* Call the notify routines */
        RtlInitUnicodeString(&NtDllName,
                             L"\\SystemRoot\\System32\\ntdll.dll");
        PspRunLoadImageNotifyRoutines(&NtDllName,
                                      Process->UniqueProcessId,
                                      &ImageInfo);
    }

    /* Fail if we have no port */
    DebugPort = Process->DebugPort;
    if (!DebugPort) return;

    /* Check if create was not already reported */
    if (!(ProcessFlags & PSF_CREATE_REPORTED_BIT))
    {
        /* Setup the information structure for the new thread */
        CreateProcess->InitialThread.SubSystemKey = 0;
        CreateProcess->InitialThread.StartAddress = NULL;

        /* And for the new process */
        CreateProcess->SubSystemKey = 0;
        CreateProcess->FileHandle = DbgkpSectionToFileHandle(Process->
                                                             SectionObject);
        CreateProcess->BaseOfImage = Process->SectionBaseAddress;
        CreateProcess->DebugInfoFileOffset = 0;
        CreateProcess->DebugInfoSize = 0;

        /* Get the NT Header */
        NtHeader = RtlImageNtHeader(Process->SectionBaseAddress);
        if (NtHeader)
        {
            /* Fill out data from the header */
            CreateProcess->InitialThread.StartAddress =
                (PVOID)((ULONG_PTR)NtHeader->OptionalHeader.ImageBase +
                        NtHeader->OptionalHeader.AddressOfEntryPoint);
            CreateProcess->DebugInfoFileOffset = NtHeader->FileHeader.
                                                 PointerToSymbolTable;
            CreateProcess->DebugInfoSize = NtHeader->FileHeader.
                                           NumberOfSymbols;
        }

        /* Setup the API Message */
        ApiMessage.h.u1.Length = sizeof(DBGKM_MSG) << 16 |
                                 (8 + sizeof(DBGKM_CREATE_PROCESS));
        ApiMessage.h.u2.ZeroInit = LPC_DEBUG_EVENT;
        ApiMessage.ApiNumber = DbgKmCreateProcessApi;

        /* Send the message */
        DbgkpSendApiMessage(&ApiMessage, FALSE);

        /* Close the handle */
        ObCloseHandle(CreateProcess->FileHandle, KernelMode);

        /* Setup the parameters */
        LoadDll->BaseOfDll = PspSystemDllBase;
        LoadDll->DebugInfoFileOffset = 0;
        LoadDll->DebugInfoSize = 0;
        LoadDll->NamePointer = NULL;

        /* Get the NT Headers */
        NtHeader = RtlImageNtHeader(PspSystemDllBase);
        if (NtHeader)
        {
            /* Fill out debug information */
            LoadDll->DebugInfoFileOffset = NtHeader->
                                           FileHeader.PointerToSymbolTable;
            LoadDll->DebugInfoSize = NtHeader->FileHeader.NumberOfSymbols;
        }

        /* Get the TEB */
        Teb = Thread->Tcb.Teb;
        if (Teb)
        {
            /* Copy the system library name and link to it */
            wcsncpy(Teb->StaticUnicodeBuffer,
                    L"ntdll.dll",
                    sizeof(Teb->StaticUnicodeBuffer) / sizeof(WCHAR));
            Teb->Tib.ArbitraryUserPointer = Teb->StaticUnicodeBuffer;

            /* Return it in the debug event as well */
            LoadDll->NamePointer = &Teb->Tib.ArbitraryUserPointer;
        }

        /* Get a handle */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &PsNtDllPathName,
                                   OBJ_CASE_INSENSITIVE |
                                   OBJ_KERNEL_HANDLE |
                                   OBJ_FORCE_ACCESS_CHECK,
                                   NULL,
                                   NULL);
        Status = ZwOpenFile(&LoadDll->FileHandle,
                            GENERIC_READ | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_DELETE |
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_NONALERT);
        if (NT_SUCCESS(Status))
        {
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
    }
    else
    {
        /* Otherwise, do it just for the thread */
        CreateThread->SubSystemKey = 0;
        CreateThread->StartAddress = StartAddress;

        /* Setup the API Message */
        ApiMessage.h.u1.Length = sizeof(DBGKM_MSG) << 16 |
                                 (8 + sizeof(DBGKM_CREATE_THREAD));
        ApiMessage.h.u2.ZeroInit = LPC_DEBUG_EVENT;
        ApiMessage.ApiNumber = DbgKmCreateThreadApi;

        /* Send the message */
        DbgkpSendApiMessage(&ApiMessage, TRUE);
    }
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
DbgkMapViewOfSection(IN PVOID Section,
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
    DBGKTRACE(DBGK_PROCESS_DEBUG,
              "Section: %p. Base: %p\n", Section, BaseAddress);

    /* Check if this thread is kernel, hidden or doesn't have a debug port */
    if ((ExGetPreviousMode() == KernelMode) ||
        (Thread->HideFromDebugger) ||
        !(Process->DebugPort))
    {
        /* Don't notify the debugger */
        return;
    }

    /* Setup the parameters */
    LoadDll->FileHandle = DbgkpSectionToFileHandle(Section);
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

    /* Check if this thread is kernel, hidden or doesn't have a debug port */
    if ((ExGetPreviousMode() == KernelMode) ||
        (Thread->HideFromDebugger) ||
        !(Process->DebugPort))
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
