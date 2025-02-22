/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     LOAD_CONFIG validation / registration
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "loadconfig.h"


// Tell the linker we want a LOAD_CONFIG
const IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used =
{
    sizeof(IMAGE_LOAD_CONFIG_DIRECTORY),
    0,                          // TimeDateStamp
    0,                          // MajorVersion
    0,                          // MinorVersion
    0,                          // GlobalFlagsClear
    FLG_USER_STACK_TRACE_DB,    // GlobalFlagsSet
    0,                          // CriticalSectionDefaultTimeout
    0,                          // DeCommitFreeBlockThreshold
    0,                          // DeCommitTotalFreeThreshold
    0,                          // LockPrefixTable
    0,                          // MaximumAllocationSize
    0,                          // VirtualMemoryThreshold
    0,                          // ProcessHeapFlags
    0,                          // ProcessAffinityMask
    0,                          // CSDVersion
    0,                          // Reserved1
    0,                          // EditList
    0,                          // SecurityCookie
};


BOOL check_loadconfig()
{
    BOOL Result;
    PPEB Peb = NtCurrentPeb();
    ULONG ConfigSize = 0;
    ULONG MinimalSize;
    PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig;

    // Validate the required flag for the 'stacktrace' test
    ok(Peb->NtGlobalFlag & FLG_USER_STACK_TRACE_DB, "NtGlobalFlag: 0x%lx\n", Peb->NtGlobalFlag);
    Result = (Peb->NtGlobalFlag & FLG_USER_STACK_TRACE_DB) != 0;

    // Now validate our LOAD_CONFIG entry
    LoadConfig = (PIMAGE_LOAD_CONFIG_DIRECTORY)RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                                                            TRUE,
                                                                            IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                                            &ConfigSize);

    MinimalSize = FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY, SecurityCookie) + sizeof(LoadConfig->SecurityCookie);
    if (!LoadConfig || ConfigSize < MinimalSize)
    {
        ok(0, "Invalid IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG: %p, %lu (%lu)\n",
           LoadConfig, ConfigSize, MinimalSize);
    }
    else
    {
        ok(LoadConfig->GlobalFlagsSet & FLG_USER_STACK_TRACE_DB,
           "Invalid GlobalFlagsSet: %lx\n", LoadConfig->GlobalFlagsSet);
        ok(!(LoadConfig->GlobalFlagsClear & FLG_USER_STACK_TRACE_DB),
           "Invalid GlobalFlagsClear: %lx\n", LoadConfig->GlobalFlagsClear);
        ok(LoadConfig->Size == sizeof(IMAGE_LOAD_CONFIG_DIRECTORY),
           "Unexpected size difference: %lu vs %u\n", LoadConfig->Size, sizeof(IMAGE_LOAD_CONFIG_DIRECTORY));
    }

    return Result;
}
