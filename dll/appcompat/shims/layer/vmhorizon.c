/*
 * PROJECT:     ReactOS 'Layers' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shim for VMWare Horizon setup
 * COPYRIGHT:   Copyright 2017 Thomas Faber (thomas.faber@reactos.org)
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include "ntndk.h"

static BOOL Write(PBYTE Address, PBYTE Data, SIZE_T Size)
{
    PVOID BaseAddress = Address;
    SIZE_T RegionSize = Size;
    ULONG OldProtection;
    NTSTATUS Status = NtProtectVirtualMemory(NtCurrentProcess(), &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtection);
    if (NT_SUCCESS(Status))
    {
        SIZE_T Bytes;
        Status = NtWriteVirtualMemory(NtCurrentProcess(), Address, Data, Size, &Bytes);
        if (NT_SUCCESS(Status) && Bytes != Size)
            Status = STATUS_MEMORY_NOT_ALLOCATED;
        NtProtectVirtualMemory(NtCurrentProcess(), &BaseAddress, &RegionSize, OldProtection, &OldProtection);
    }
    return NT_SUCCESS(Status);
}

static void FixupDll(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    static const UCHAR Match1[5] = { 0x0C, 0x8B, 0xFC, 0xF3, 0xA5 };
    static const UCHAR Match2[5] = { 0x0C, 0x8B, 0xFC, 0xF3, 0xA5 };
    static const UCHAR Match3[5] = { 0xB0, 0x8B, 0xFC, 0xF3, 0xA5 };
    UCHAR Replacement1[5] = { 0x10, 0x89, 0x34, 0x24, 0x90 };
    UCHAR Replacement2[5] = { 0x10, 0x89, 0x34, 0x24, 0x90 };
    UCHAR Replacement3[5] = { 0xB4, 0x89, 0x34, 0x24, 0x90 };
#define OFFSET_1    0x21A6E
#define OFFSET_2    0x21B04
#define OFFSET_3    0x21C3C


    UCHAR Buffer[5];
    PBYTE Base = LdrEntry->DllBase;
    SIZE_T Bytes;

    /*
    00020E6E: 0C 8B FC F3 A5 --> 10 89 34 24 90     F11A6E - ef0000 = 21A6E
    00020F04: 0C 8B FC F3 A5 --> 10 89 34 24 90     F11B04 - ef0000 = 21B04
    00021C3C: B0 8B FC F3 A5 --> B4 89 34 24 90     F11C3C - ef0000 = 21C3C
    */
    do {
        DbgPrint("Module %wZ Loaded at 0x%p, we should patch!\n", &LdrEntry->BaseDllName, LdrEntry->DllBase);
        if (!NT_SUCCESS(NtReadVirtualMemory(NtCurrentProcess(), Base + OFFSET_1, Buffer, 5, &Bytes)) || Bytes != 5)
            break;
        if (memcmp(Buffer, Match1, sizeof(Match1)))
            break;

        if (!NT_SUCCESS(NtReadVirtualMemory(NtCurrentProcess(), Base + OFFSET_2, Buffer, 5, &Bytes)) || Bytes != 5)
            break;
        if (memcmp(Buffer, Match2, sizeof(Match2)))
            break;

        if (!NT_SUCCESS(NtReadVirtualMemory(NtCurrentProcess(), Base + OFFSET_3, Buffer, 5, &Bytes)) || Bytes != 5)
            break;
        if (memcmp(Buffer, Match3, sizeof(Match3)))
            break;

        DbgPrint("Module %wZ Loaded at 0x%p, OK to patch!\n", &LdrEntry->BaseDllName, LdrEntry->DllBase);
        if (!Write(Base + OFFSET_1, Replacement1, sizeof(Replacement1)))
            break;
        if (!Write(Base + OFFSET_2, Replacement2, sizeof(Replacement2)))
            break;
        if (!Write(Base + OFFSET_3, Replacement3, sizeof(Replacement3)))
            break;

        NtFlushInstructionCache(NtCurrentProcess(), Base, 0x22000);

        DbgPrint("Module %wZ Loaded at 0x%p, patched!\n", &LdrEntry->BaseDllName, LdrEntry->DllBase);
    } while (0);
}

static BOOLEAN PostfixUnicodeString(const UNICODE_STRING* String1, const UNICODE_STRING* String2)
{
    PWCHAR pc1;
    PWCHAR pc2;
    ULONG  NumChars;

    if (String2->Length < String1->Length)
        return FALSE;

    if (!String1->Buffer || !String2->Buffer)
        return FALSE;

    NumChars = String1->Length / sizeof(WCHAR);
    pc1 = String1->Buffer;
    pc2 = String2->Buffer + (String2->Length / sizeof(WCHAR)) - NumChars;

    while (NumChars--)
    {
        if (RtlUpcaseUnicodeChar(*pc1++) != RtlUpcaseUnicodeChar(*pc2++))
            return FALSE;
    }

    return TRUE;
}

#define SHIM_NS         VMHorizonSetup
#include <setup_shim.inl>

#define SHIM_NUM_HOOKS  0
#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)

BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason, PVOID ptr)
{
    if (fdwReason == SHIM_REASON_DLL_LOAD)
    {
        static const UNICODE_STRING DllPrefix = RTL_CONSTANT_STRING(L"msi");
        static const UNICODE_STRING DllPostfix = RTL_CONSTANT_STRING(L".tmp");
        PLDR_DATA_TABLE_ENTRY LdrEntry = ptr;

        BOOLEAN Prefix = RtlPrefixUnicodeString(&DllPrefix, &LdrEntry->BaseDllName, TRUE);
        BOOLEAN Postfix = PostfixUnicodeString(&DllPostfix, &LdrEntry->BaseDllName);
        ULONG ExtraChars = (LdrEntry->BaseDllName.Length - DllPrefix.Length - DllPostfix.Length) / sizeof(WCHAR);

        /* msiN[N].tmp */
        if (Prefix && Postfix && ExtraChars <= 2)
        {
            PIMAGE_NT_HEADERS ImageNtHeader = RtlImageNtHeader(LdrEntry->DllBase);
            if (ImageNtHeader && ImageNtHeader->OptionalHeader.CheckSum == 0x176241)
            {
                SHIM_MSG("Module %wZ is a match, applying fixups\n", &LdrEntry->BaseDllName);
                FixupDll(LdrEntry);
            }
        }
    }
    return TRUE;
}

#include <implement_shim.inl>
