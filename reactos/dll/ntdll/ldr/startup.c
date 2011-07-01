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
//#define NDEBUG
#include <debug.h>
#include <win32k/callback.h>

VOID RtlInitializeHeapManager(VOID);
extern PTEB LdrpTopLevelDllBeingLoadedTeb;
VOID NTAPI RtlpInitDeferedCriticalSection(VOID);
VOID RtlpInitializeVectoredExceptionHandling(VOID);

/* GLOBALS *******************************************************************/

extern PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
extern RTL_CRITICAL_SECTION FastPebLock;
extern RTL_BITMAP TlsBitMap;
extern RTL_BITMAP TlsExpansionBitMap;

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

/* EOF */
