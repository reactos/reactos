/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SfcGetFiles
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include <apitest.h>
#include <strsafe.h>
#include <ndk/umtypes.h>

typedef struct _PROTECT_FILE_ENTRY {
    PWSTR SourceFileName;
    PWSTR FileName;
    PWSTR InfName;
} PROTECT_FILE_ENTRY, *PPROTECT_FILE_ENTRY;

NTSTATUS (WINAPI *SfcGetFiles)(PPROTECT_FILE_ENTRY* ProtFileData, PULONG FileCount);

PCWSTR wszEnvVars[] =
{
    L"%systemroot%",
    L"%commonprogramfiles%",
    L"%ProgramFiles%",
    L"%systemdrive%"
};

static void Test_GetFiles()
{
    PPROTECT_FILE_ENTRY FileData;
    PCWSTR Ptr;
    ULONG FileCount, n, j;
    NTSTATUS Status;

    Status = SfcGetFiles(&FileData, &FileCount);
    ok(NT_SUCCESS(Status), "SfcGetFiles failed: 0x%lx\n", Status);

    if (!NT_SUCCESS(Status))
        return;

    for (n = 0; n < FileCount; ++n)
    {
        PPROTECT_FILE_ENTRY Entry = FileData + n;

        ok(Entry->FileName != NULL, "Entry %lu without FileName!", n);
        if (Entry->FileName)
        {
            Ptr = NULL;
            for (j = 0; j < _countof(wszEnvVars); ++j)
            {
                Ptr = wcsstr(Entry->FileName, wszEnvVars[j]);
                if (Ptr)
                    break;
            }
            ok(Ptr != NULL, "Expected to find one match from wszEnvVars in %s\n", wine_dbgstr_w(Entry->FileName));
        }
        if (Entry->InfName)
        {
            Ptr = wcsstr(Entry->InfName, L".inf");
            ok(Ptr == (Entry->InfName + wcslen(Entry->InfName) - 4),
               ".inf not found in %s\n", wine_dbgstr_w(Entry->InfName));
        }
    }
}

START_TEST(SfcGetFiles)
{
    HMODULE mod;

    mod = LoadLibraryA("sfcfiles.dll");
    if (!mod)
    {
        skip("sfcfiles.dll not found\n");
        return;
    }

    SfcGetFiles = (void*)GetProcAddress(mod, "SfcGetFiles");
    ok(SfcGetFiles != NULL, "Function not exported!\n");
    if (!SfcGetFiles)
        return;

    Test_GetFiles();
}
