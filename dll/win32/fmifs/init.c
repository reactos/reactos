/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/init.c
 * PURPOSE:         Initialisation
 *
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"

#include <winreg.h>

#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/obfuncs.h>

#define NDEBUG
#include <debug.h>

static ULONG FmIfsAttached = 0;
LIST_ENTRY ProviderListHead;

PIFS_PROVIDER
LoadProvider(
    IN PWCHAR FileSystem)
{
    PLIST_ENTRY ListEntry;
    PIFS_PROVIDER Provider;

    DPRINT("LoadProvider(%S)\n", FileSystem);

    ListEntry = ProviderListHead.Flink;
    while (ListEntry != &ProviderListHead)
    {
        Provider = CONTAINING_RECORD(ListEntry, IFS_PROVIDER, ListEntry);
        if (_wcsicmp(Provider->Name, FileSystem) == 0)
        {
            DPRINT("Found it!\n");
            if (!Provider->hModule)
            {
                Provider->hModule = LoadLibraryW(Provider->DllFile);
                if (!Provider->hModule)
                {
                    DPRINT("Dll Loading failed!\n");
                    return NULL;
                }

                /* Get function pointers */
                Provider->Chkdsk = (PULIB_CHKDSK)GetProcAddress(Provider->hModule, "Chkdsk");
                //Provider->ChkdskEx = (PULIB_CHKDSKEX)GetProcAddress(Provider->hModule, "ChkdskEx");
                //Provider->Extend = (PULIB_EXTEND)GetProcAddress(Provider->hModule, "Extend");
                Provider->Format = (PULIB_FORMAT)GetProcAddress(Provider->hModule, "Format");
                //Provider->FormatEx = (PULIB_FORMATEX)GetProcAddress(Provider->hModule, "FormatEx");
            }
            return Provider;
        }
        ListEntry = ListEntry->Flink;
    }

    /* Provider not found */
    return NULL;
}

BOOLEAN
UnloadProvider(
    PIFS_PROVIDER Provider)
{
    DPRINT("UnloadProvider(%S)\n", Provider->Name);

    if (Provider->hModule)
    {
        FreeLibrary(Provider->hModule);
        Provider->hModule = NULL;

        Provider->Chkdsk =  NULL;
        Provider->ChkdskEx = NULL;
        Provider->Extend =  NULL;
        Provider->Format =  NULL;
        Provider->FormatEx = NULL;
    }

    return TRUE;
}

static
BOOLEAN
AddProvider(
    IN PCUNICODE_STRING FileSystem,
    IN PWCHAR DllFile)
{
    PIFS_PROVIDER Provider = NULL;
    ULONG ProcCount = 0;
    HMODULE hMod = NULL;
    BOOLEAN ret = FALSE;

    DPRINT("AddProvider(%wZ %S)\n", FileSystem, DllFile);

    hMod = LoadLibraryW(DllFile);
    if (!hMod)
        goto cleanup;

    if (GetProcAddress(hMod, "Chkdsk"))
        ProcCount++;
    if (GetProcAddress(hMod, "ChkdskEx"))
        ProcCount++;
    if (GetProcAddress(hMod, "Extend"))
        ProcCount++;
    if (GetProcAddress(hMod, "Format"))
        ProcCount++;
    if (GetProcAddress(hMod, "FormatEx"))
        ProcCount++;

    DPRINT("ProcCount %lu\n", ProcCount);
    if (ProcCount == 0)
        goto cleanup;

    Provider = (PIFS_PROVIDER)RtlAllocateHeap(
        RtlGetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(IFS_PROVIDER));
    if (!Provider)
        goto cleanup;

    Provider->Name = (PWSTR)RtlAllocateHeap(RtlGetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            FileSystem->Length + sizeof(UNICODE_NULL));
    if (!Provider->Name)
        goto cleanup;

    RtlCopyMemory(Provider->Name, FileSystem->Buffer, FileSystem->Length);

    Provider->DllFile = (PWSTR)RtlAllocateHeap(RtlGetProcessHeap(),
                                               HEAP_ZERO_MEMORY,
                                               (wcslen(DllFile) + 1) * sizeof(WCHAR));
    if (!Provider->DllFile)
        goto cleanup;

    wcscpy(Provider->DllFile, DllFile);

    InsertTailList(&ProviderListHead, &Provider->ListEntry);
    ret = TRUE;

    DPRINT("AddProvider success\n");

cleanup:
    if (hMod)
        FreeLibrary(hMod);

    if (!ret)
    {
        if (Provider)
        {
            if (Provider->Name)
                RtlFreeHeap(RtlGetProcessHeap(), 0, Provider->Name);
            if (Provider->DllFile)
                RtlFreeHeap(RtlGetProcessHeap(), 0, Provider->DllFile);
            RtlFreeHeap(RtlGetProcessHeap(), 0, Provider);
        }
    }
    return ret;
}

static
BOOLEAN
InitializeFmIfsOnce(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING RegistryPath
        = RTL_CONSTANT_STRING(L"\\REGISTRY\\Machine\\SOFTWARE\\ReactOS\\ReactOS\\CurrentVersion\\IFS");
    HANDLE hKey = NULL;
    PKEY_VALUE_FULL_INFORMATION Buffer;
    ULONG BufferSize = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH;
    ULONG RequiredSize;
    ULONG i = 0;
    UNICODE_STRING Name;
    UNICODE_STRING Data;
    NTSTATUS Status;

    InitializeListHead(&ProviderListHead);

    /* Read IFS providers from HKLM\SOFTWARE\ReactOS\ReactOS\CurrentVersion\IFS */
    InitializeObjectAttributes(&ObjectAttributes, &RegistryPath, 0, NULL, NULL);
    Status = NtOpenKey(&hKey, KEY_QUERY_VALUE, &ObjectAttributes);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        return TRUE;
    else if (!NT_SUCCESS(Status))
        return FALSE;

    Buffer = (PKEY_VALUE_FULL_INFORMATION)RtlAllocateHeap(
        RtlGetProcessHeap(),
        0,
        BufferSize);
    if (!Buffer)
    {
        NtClose(hKey);
        return FALSE;
    }

    while (TRUE)
    {
        Status = NtEnumerateValueKey(
            hKey,
            i++,
            KeyValueFullInformation,
            Buffer,
            BufferSize,
            &RequiredSize);
        if (Status == STATUS_BUFFER_OVERFLOW)
            continue;
        else if (!NT_SUCCESS(Status))
            break;
        else if (Buffer->Type != REG_SZ)
            continue;

        Name.Length = Name.MaximumLength = Buffer->NameLength;
        Name.Buffer = Buffer->Name;
        Data.Length = Data.MaximumLength = Buffer->DataLength;
        Data.Buffer = (PWCHAR)((ULONG_PTR)Buffer + Buffer->DataOffset);
        if (Data.Length > sizeof(WCHAR) && Data.Buffer[Data.Length / sizeof(WCHAR) - 1] == UNICODE_NULL)
            Data.Length -= sizeof(WCHAR);

        AddProvider(&Name, Data.Buffer);
    }

    NtClose(hKey);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    return TRUE;
}

static
VOID
UninitializeFmIfsOnce(VOID)
{
    PLIST_ENTRY ListEntry;
    PIFS_PROVIDER Provider;

    while (!IsListEmpty(&ProviderListHead))
    {
        ListEntry = RemoveTailList(&ProviderListHead);
        Provider = CONTAINING_RECORD(ListEntry, IFS_PROVIDER, ListEntry);
        if (Provider->Name)
            RtlFreeHeap(RtlGetProcessHeap(), 0, Provider->Name);
        if (Provider->DllFile)
            RtlFreeHeap(RtlGetProcessHeap(), 0, Provider->DllFile);
        if (Provider->hModule)
            FreeLibrary(Provider->hModule);
        RtlFreeHeap(RtlGetProcessHeap(), 0, Provider);
    }
}

/* FMIFS.8 */
BOOLEAN
NTAPI
InitializeFmIfs(
    IN PVOID hinstDll,
    IN DWORD dwReason,
    IN PVOID reserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (FmIfsAttached == 0)
            {
                if (InitializeFmIfsOnce() == FALSE)
                    return FALSE;

                FmIfsAttached++;
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            FmIfsAttached--;
            if (FmIfsAttached == 0)
                UninitializeFmIfsOnce();
            break;
    }

    return TRUE;
}

/* EOF */
