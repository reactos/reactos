/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            dll/ntdll/ldr/startup.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>
#include <win32k/callback.h>

VOID RtlInitializeHeapManager(VOID);
VOID LdrpInitLoader(VOID);
extern PTEB LdrpTopLevelDllBeingLoadedTeb;

/* GLOBALS *******************************************************************/

extern PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
static RTL_CRITICAL_SECTION PebLock;
static RTL_BITMAP TlsBitMap;
static RTL_BITMAP TlsExpansionBitMap;

#define VALUE_BUFFER_SIZE 256

/* FUNCTIONS *****************************************************************/

BOOLEAN
FASTCALL
ReadCompatibilitySetting(HANDLE Key,
                         LPWSTR Value,
                         PKEY_VALUE_PARTIAL_INFORMATION ValueInfo,
                         DWORD * Buffer)
{
    UNICODE_STRING ValueName;
    NTSTATUS Status;
    ULONG Length;

    RtlInitUnicodeString(&ValueName, Value);
    Status = NtQueryValueKey(Key,
                             &ValueName,
                             KeyValuePartialInformation,
                             ValueInfo,
                             VALUE_BUFFER_SIZE,
                             &Length);

    if (!NT_SUCCESS(Status) || (ValueInfo->Type != REG_DWORD))
    {
        RtlFreeUnicodeString(&ValueName);
        return FALSE;
    }

    RtlCopyMemory(Buffer, &ValueInfo->Data[0], sizeof(DWORD));
    RtlFreeUnicodeString(&ValueName);
    return TRUE;
}

VOID
FASTCALL
LoadImageFileExecutionOptions(PPEB Peb)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Value = 0;
    UNICODE_STRING ImageName;
    UNICODE_STRING ImagePathName;
    ULONG ValueSize;
    extern ULONG RtlpDphGlobalFlags, RtlpPageHeapSizeRangeStart, RtlpPageHeapSizeRangeEnd;
    extern ULONG RtlpPageHeapDllRangeStart, RtlpPageHeapDllRangeEnd;
    extern WCHAR RtlpDphTargetDlls[512];
    extern BOOLEAN RtlpPageHeapEnabled;

    if (Peb->ProcessParameters &&
        Peb->ProcessParameters->ImagePathName.Length > 0)
    {
        DPRINT("%wZ\n", &Peb->ProcessParameters->ImagePathName);

        ImagePathName = Peb->ProcessParameters->ImagePathName;
        ImageName.Buffer = ImagePathName.Buffer + ImagePathName.Length / sizeof(WCHAR);
        ImageName.Length = 0;

        while (ImagePathName.Buffer < ImageName.Buffer)
        {
            ImageName.Buffer--;
            if (*ImageName.Buffer == L'\\')
            {
                ImageName.Buffer++;
                break;
            }
        }

        ImageName.Length = ImagePathName.Length -
            (ImageName.Buffer - ImagePathName.Buffer) * sizeof(WCHAR);
        ImageName.MaximumLength = ImageName.Length +
            ImagePathName.MaximumLength - ImagePathName.Length;

        DPRINT("%wZ\n", &ImageName);

        /* global flag */
        Status = LdrQueryImageFileExecutionOptions(&ImageName,
                                                   L"GlobalFlag",
                                                   REG_DWORD,
                                                   (PVOID)&Value,
                                                   sizeof(Value),
                                                   &ValueSize);
        if (NT_SUCCESS(Status))
        {
            Peb->NtGlobalFlag = Value;
            DPRINT("GlobalFlag: Value=0x%lx\n", Value);
        }
        else
        {
            /* Add debugging flags if there is no GlobalFlags override */
            if (Peb->BeingDebugged)
            {
                Peb->NtGlobalFlag |= FLG_HEAP_VALIDATE_PARAMETERS |
                                     FLG_HEAP_ENABLE_FREE_CHECK |
                                     FLG_HEAP_ENABLE_TAIL_CHECK;
            }
        }

        /* Handle the case when page heap is enabled */
        if (Peb->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS)
        {
            /* Disable all heap debugging flags so that no heap call goes via page heap branch */
            Peb->NtGlobalFlag &= ~(FLG_HEAP_VALIDATE_PARAMETERS |
                                   FLG_HEAP_VALIDATE_ALL |
                                   FLG_HEAP_ENABLE_FREE_CHECK |
                                   FLG_HEAP_ENABLE_TAIL_CHECK |
                                   FLG_USER_STACK_TRACE_DB |
                                   FLG_HEAP_ENABLE_TAGGING |
                                   FLG_HEAP_ENABLE_TAG_BY_DLL);

            /* Get page heap flags without checking return value */
            LdrQueryImageFileExecutionOptions(&ImageName,
                                              L"PageHeapFlags",
                                              REG_DWORD,
                                              (PVOID)&RtlpDphGlobalFlags,
                                              sizeof(RtlpDphGlobalFlags),
                                              &ValueSize);

            LdrQueryImageFileExecutionOptions(&ImageName,
                                              L"PageHeapSizeRangeStart",
                                              REG_DWORD,
                                              (PVOID)&RtlpPageHeapSizeRangeStart,
                                              sizeof(RtlpPageHeapSizeRangeStart),
                                              &ValueSize);

            LdrQueryImageFileExecutionOptions(&ImageName,
                                              L"PageHeapSizeRangeEnd",
                                              REG_DWORD,
                                              (PVOID)&RtlpPageHeapSizeRangeEnd,
                                              sizeof(RtlpPageHeapSizeRangeEnd),
                                              &ValueSize);

            LdrQueryImageFileExecutionOptions(&ImageName,
                                              L"PageHeapDllRangeStart",
                                              REG_DWORD,
                                              (PVOID)&RtlpPageHeapDllRangeStart,
                                              sizeof(RtlpPageHeapDllRangeStart),
                                              &ValueSize);

            LdrQueryImageFileExecutionOptions(&ImageName,
                                              L"PageHeapDllRangeEnd",
                                              REG_DWORD,
                                              (PVOID)&RtlpPageHeapDllRangeEnd,
                                              sizeof(RtlpPageHeapDllRangeEnd),
                                              &ValueSize);

            LdrQueryImageFileExecutionOptions(&ImageName,
                                              L"PageHeapTargetDlls",
                                              REG_SZ,
                                              (PVOID)RtlpDphTargetDlls,
                                              sizeof(RtlpDphTargetDlls),
                                              &ValueSize);

            /* Now when all parameters are read, enable page heap */
            RtlpPageHeapEnabled = TRUE;
        }
    }
}

BOOLEAN
FASTCALL
LoadCompatibilitySettings(PPEB Peb)
{
    NTSTATUS Status;
    HANDLE UserKey = NULL;
    HANDLE KeyHandle;
    HANDLE SubKeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");
    UNICODE_STRING ValueName;
    UCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    ULONG Length;
    DWORD MajorVersion, MinorVersion, BuildNumber, PlatformId,
          SPMajorVersion, SPMinorVersion = 0;

    if (Peb->ProcessParameters &&
        (Peb->ProcessParameters->ImagePathName.Length > 0))
    {
        Status = RtlOpenCurrentUser(KEY_READ, &UserKey);
        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   UserKey,
                                   NULL);

        Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);

        if (!NT_SUCCESS(Status))
        {
            if (UserKey)
                NtClose(UserKey);
            return FALSE;
        }

        /* query version name for application */
        ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) ValueBuffer;
        Status = NtQueryValueKey(KeyHandle,
                                 &Peb->ProcessParameters->ImagePathName,
                                 KeyValuePartialInformation,
                                 ValueBuffer,
                                 VALUE_BUFFER_SIZE,
                                 &Length);

        if (!NT_SUCCESS(Status) || (ValueInfo->Type != REG_SZ))
        {
            NtClose(KeyHandle);
            if (UserKey)
                NtClose(UserKey);
            return FALSE;
        }

        ValueName.Length = ValueInfo->DataLength;
        ValueName.MaximumLength = ValueInfo->DataLength;
        ValueName.Buffer = (PWSTR) ValueInfo->Data;

        /* load version info */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &ValueName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);

        Status = NtOpenKey(&SubKeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);

        if (!NT_SUCCESS(Status))
        {
            NtClose(KeyHandle);
            if (UserKey)
                NtClose(UserKey);
            return FALSE;
        }

        DPRINT("Loading version information for: %wZ\n", &ValueName);

        /* read settings from registry */
        if (!ReadCompatibilitySetting(SubKeyHandle, L"MajorVersion", ValueInfo, &MajorVersion))
            goto finish;
        if (!ReadCompatibilitySetting(SubKeyHandle, L"MinorVersion", ValueInfo, &MinorVersion))
            goto finish;
        if (!ReadCompatibilitySetting(SubKeyHandle, L"BuildNumber", ValueInfo, &BuildNumber))
            goto finish;
        if (!ReadCompatibilitySetting(SubKeyHandle, L"PlatformId", ValueInfo, &PlatformId))
            goto finish;

        /* now assign the settings */
        Peb->OSMajorVersion =  (ULONG) MajorVersion;
        Peb->OSMinorVersion =  (ULONG) MinorVersion;
        Peb->OSBuildNumber  = (USHORT) BuildNumber;
        Peb->OSPlatformId   =  (ULONG) PlatformId;

        /* optional service pack version numbers */
        if (ReadCompatibilitySetting(SubKeyHandle,
                                     L"SPMajorVersion",
                                     ValueInfo,
                                     &SPMajorVersion) &&
            ReadCompatibilitySetting(SubKeyHandle,
                                     L"SPMinorVersion",
                                     ValueInfo,
                                     &SPMinorVersion))
        {
            Peb->OSCSDVersion = ((SPMajorVersion & 0xFF) << 8) |
                                (SPMinorVersion & 0xFF);
        }

finish:
        /* we're finished */
        NtClose(SubKeyHandle);
        NtClose(KeyHandle);
        if (UserKey)
            NtClose(UserKey);
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
NTAPI
LdrpInitializeProcess(PCONTEXT Context,
                      PVOID SystemArgument1)
{
    PIMAGE_NT_HEADERS NTHeaders;
    PEPFUNC EntryPoint;
    PIMAGE_DOS_HEADER PEDosHeader;
    PVOID ImageBase;
    PPEB Peb = NtCurrentPeb();
    PLDR_DATA_TABLE_ENTRY NtModule;     // ntdll
    NLSTABLEINFO NlsTable;
    WCHAR FullNtDllPath[MAX_PATH];
    SYSTEM_BASIC_INFORMATION SystemInformation;
    NTSTATUS Status;
    PVOID BaseAddress = SystemArgument1;

    DPRINT("LdrpInit()\n");
    DPRINT("Peb %p\n", Peb);
    ImageBase = Peb->ImageBaseAddress;
    DPRINT("ImageBase %p\n", ImageBase);

    if (ImageBase <= (PVOID) 0x1000)
    {
        DPRINT("ImageBase is null\n");
        ZwTerminateProcess(NtCurrentProcess(), STATUS_INVALID_IMAGE_FORMAT);
    }

    /*  If MZ header exists  */
    PEDosHeader = (PIMAGE_DOS_HEADER) ImageBase;
    NTHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)ImageBase + PEDosHeader->e_lfanew);
    DPRINT("PEDosHeader %p\n", PEDosHeader);

    if (PEDosHeader->e_magic != IMAGE_DOS_SIGNATURE ||
        PEDosHeader->e_lfanew == 0L ||
        NTHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        DPRINT1("Image has bad header\n");
        ZwTerminateProcess(NtCurrentProcess(), STATUS_INVALID_IMAGE_FORMAT);
    }

    /* normalize process parameters */
    RtlNormalizeProcessParams(Peb->ProcessParameters);

    /* Initialize NLS data */
    RtlInitNlsTables(Peb->AnsiCodePageData,
                     Peb->OemCodePageData,
                     Peb->UnicodeCaseTableData,
                     &NlsTable);
    RtlResetRtlTranslations(&NlsTable);

    /* Get number of processors */
    DPRINT("Here\n");
    Status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &SystemInformation,
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL);
    DPRINT("Here2\n");
    if (!NT_SUCCESS(Status))
    {
        ZwTerminateProcess(NtCurrentProcess(), Status);
    }

    Peb->NumberOfProcessors = SystemInformation.NumberOfProcessors;

    /* Load execution options */
    LoadImageFileExecutionOptions(Peb);

    /* create process heap */
    RtlInitializeHeapManager();
    Peb->ProcessHeap = RtlCreateHeap(HEAP_GROWABLE,
                                     NULL,
                                     NTHeaders->OptionalHeader.SizeOfHeapReserve,
                                     NTHeaders->OptionalHeader.SizeOfHeapCommit,
                                     NULL,
                                     NULL);
    if (Peb->ProcessHeap == 0)
    {
        DPRINT1("Failed to create process heap\n");
        ZwTerminateProcess(NtCurrentProcess(), STATUS_INSUFFICIENT_RESOURCES);
    }

    /* Check for correct machine type */
    if (NTHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_NATIVE)
    {
        ULONG_PTR HardErrorParameters[1];
        UNICODE_STRING ImageNameU;
        ANSI_STRING ImageNameA;
        WCHAR *Ptr;
        ULONG ErrorResponse;

        DPRINT1("Image %wZ is for a foreign architecture (0x%x).\n",
                &Peb->ProcessParameters->ImagePathName, NTHeaders->FileHeader.Machine);

        /* Get the full image path name */
        ImageNameU = Peb->ProcessParameters->ImagePathName;

        /* Get the file name */
        Ptr = Peb->ProcessParameters->ImagePathName.Buffer +
              (Peb->ProcessParameters->ImagePathName.Length / sizeof(WCHAR)) -1;
        while ((Ptr >= Peb->ProcessParameters->ImagePathName.Buffer) &&
               (*Ptr != L'\\')) Ptr--;
        ImageNameU.Buffer = Ptr + 1;
        ImageNameU.Length = Peb->ProcessParameters->ImagePathName.Length -
            (ImageNameU.Buffer - Peb->ProcessParameters->ImagePathName.Buffer) * sizeof(WCHAR);
        ImageNameU.MaximumLength = ImageNameU.Length;

        /*`Convert to ANSI, harderror message needs that */
        RtlUnicodeStringToAnsiString(&ImageNameA, &ImageNameU, TRUE);

        /* Raise harderror */
        HardErrorParameters[0] = (ULONG_PTR)&ImageNameA;
        NtRaiseHardError(STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE,
                         1,
                         1,
                         HardErrorParameters,
                         OptionOk,
                         &ErrorResponse);

        RtlFreeAnsiString(&ImageNameA);
        ZwTerminateProcess(NtCurrentProcess(), STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE);
    }

    /* initalize peb lock support */
    RtlInitializeCriticalSection(&PebLock);
    Peb->FastPebLock = &PebLock;

    /* initialize tls bitmaps */
    RtlInitializeBitMap(&TlsBitMap, Peb->TlsBitmapBits, TLS_MINIMUM_AVAILABLE);
    RtlInitializeBitMap(&TlsExpansionBitMap, Peb->TlsExpansionBitmapBits, TLS_EXPANSION_SLOTS);

    Peb->TlsBitmap = &TlsBitMap;
    Peb->TlsExpansionBitmap = &TlsExpansionBitMap;
    Peb->TlsExpansionCounter = TLS_MINIMUM_AVAILABLE;

    /* Initialize table of callbacks for the kernel. */
    Peb->KernelCallbackTable = RtlAllocateHeap(RtlGetProcessHeap(),
                                               0,
                                               sizeof(PVOID) *
                                                (USER32_CALLBACK_MAXIMUM + 1));
    if (Peb->KernelCallbackTable == NULL)
    {
        DPRINT1("Failed to create callback table\n");
        ZwTerminateProcess(NtCurrentProcess(), STATUS_INSUFFICIENT_RESOURCES);
    }

    /* initalize loader lock */
    RtlInitializeCriticalSection(&LdrpLoaderLock);
    Peb->LoaderLock = &LdrpLoaderLock;

    /* create loader information */
    Peb->Ldr = (PPEB_LDR_DATA) RtlAllocateHeap(Peb->ProcessHeap,
                                               0,
                                               sizeof(PEB_LDR_DATA));
    if (Peb->Ldr == NULL)
    {
        DPRINT1("Failed to create loader data\n");
        ZwTerminateProcess(NtCurrentProcess(), STATUS_INSUFFICIENT_RESOURCES);
    }

    Peb->Ldr->Length = sizeof(PEB_LDR_DATA);
    Peb->Ldr->Initialized = FALSE;
    Peb->Ldr->SsHandle = NULL;
    InitializeListHead(&Peb->Ldr->InLoadOrderModuleList);
    InitializeListHead(&Peb->Ldr->InMemoryOrderModuleList);
    InitializeListHead(&Peb->Ldr->InInitializationOrderModuleList);

    /* Load compatibility settings */
    LoadCompatibilitySettings(Peb);

    /* build full ntdll path */
    wcscpy(FullNtDllPath, SharedUserData->NtSystemRoot);
    wcscat(FullNtDllPath, L"\\system32\\ntdll.dll");

    /* add entry for ntdll */
    NtModule = (PLDR_DATA_TABLE_ENTRY)
                RtlAllocateHeap(Peb->ProcessHeap,
                                0,
                                sizeof(LDR_DATA_TABLE_ENTRY));
    if (NtModule == NULL)
    {
        DPRINT1("Failed to create loader module entry (NTDLL)\n");
        ZwTerminateProcess(NtCurrentProcess(), STATUS_INSUFFICIENT_RESOURCES);
    }
    memset(NtModule, 0, sizeof(LDR_DATA_TABLE_ENTRY));

    NtModule->DllBase = BaseAddress;
    NtModule->EntryPoint = 0;       /* no entry point */
    RtlCreateUnicodeString(&NtModule->FullDllName, FullNtDllPath);
    RtlCreateUnicodeString(&NtModule->BaseDllName, L"ntdll.dll");
    NtModule->Flags = LDRP_IMAGE_DLL | LDRP_ENTRY_PROCESSED;

    NtModule->LoadCount = -1;       /* don't unload */
    NtModule->TlsIndex = -1;
    NtModule->SectionPointer = NULL;
    NtModule->CheckSum = 0;

    NTHeaders = RtlImageNtHeader(NtModule->DllBase);
    NtModule->SizeOfImage = LdrpGetResidentSize(NTHeaders);
    NtModule->TimeDateStamp = NTHeaders->FileHeader.TimeDateStamp;

    InsertTailList(&Peb->Ldr->InLoadOrderModuleList,
                   &NtModule->InLoadOrderLinks);
    InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                   &NtModule->InInitializationOrderModuleList);

    /* add entry for executable (becomes first list entry) */
    LdrpImageEntry = (PLDR_DATA_TABLE_ENTRY)
                 RtlAllocateHeap(Peb->ProcessHeap,
                                 HEAP_ZERO_MEMORY,
                                 sizeof(LDR_DATA_TABLE_ENTRY));
    if (LdrpImageEntry == NULL)
    {
        DPRINT1("Failed to create loader module infomation\n");
        ZwTerminateProcess(NtCurrentProcess(), STATUS_INSUFFICIENT_RESOURCES);
    }

    LdrpImageEntry->DllBase = Peb->ImageBaseAddress;

    if ((Peb->ProcessParameters == NULL) ||
        (Peb->ProcessParameters->ImagePathName.Length == 0))
    {
        DPRINT1("Failed to access the process parameter block\n");
        ZwTerminateProcess(NtCurrentProcess(), STATUS_UNSUCCESSFUL);
    }

    RtlCreateUnicodeString(&LdrpImageEntry->FullDllName,
                           Peb->ProcessParameters->ImagePathName.Buffer);
    RtlCreateUnicodeString(&LdrpImageEntry->BaseDllName,
                           wcsrchr(LdrpImageEntry->FullDllName.Buffer, L'\\') + 1);

    DPRINT("BaseDllName '%wZ'  FullDllName '%wZ'\n", &LdrpImageEntry->BaseDllName, &LdrpImageEntry->FullDllName);

    LdrpImageEntry->Flags = LDRP_ENTRY_PROCESSED;
    LdrpImageEntry->LoadCount = -1;      /* don't unload */
    LdrpImageEntry->TlsIndex = -1;
    LdrpImageEntry->SectionPointer = NULL;
    LdrpImageEntry->CheckSum = 0;

    NTHeaders = RtlImageNtHeader(LdrpImageEntry->DllBase);
    LdrpImageEntry->SizeOfImage = LdrpGetResidentSize(NTHeaders);
    LdrpImageEntry->TimeDateStamp = NTHeaders->FileHeader.TimeDateStamp;

    LdrpTopLevelDllBeingLoadedTeb = NtCurrentTeb();

    InsertHeadList(&Peb->Ldr->InLoadOrderModuleList,
                   &LdrpImageEntry->InLoadOrderLinks);

    LdrpInitLoader();

    EntryPoint = LdrPEStartup((PVOID)ImageBase, NULL, NULL, NULL);
    LdrpImageEntry->EntryPoint = EntryPoint;

    /* all required dlls are loaded now */
    Peb->Ldr->Initialized = TRUE;

    /* Check before returning that we can run the image safely. */
    if (EntryPoint == NULL)
    {
        DPRINT1("Failed to initialize image\n");
        ZwTerminateProcess(NtCurrentProcess(), STATUS_INVALID_IMAGE_FORMAT);
    }

    /* Break into debugger */
    if (Peb->BeingDebugged)
        DbgBreakPoint();

    return STATUS_SUCCESS;
}

/* EOF */
