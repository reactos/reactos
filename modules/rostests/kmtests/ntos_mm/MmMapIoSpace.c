/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel-Mode Test Suite MmMapIoSpace test
 * COPYRIGHT:   Ankit Kumar <nsg650@outlook.com>
 */

#include <kmt_test.h>

static PHYSICAL_ADDRESS GetProcessDirectory(PEPROCESS Process) 
{
    PHYSICAL_ADDRESS PageDirectory;
    KAPC_STATE ApcState;

    if (Process != NULL) 
    {
        /* KeStackAttachProcess sets the address space to that of the process it attaches to. */
        KeStackAttachProcess(&Process->Pcb, &ApcState);
    }
#if  defined(_M_IX86) || defined(_M_AMD64)
    PageDirectory.QuadPart = __readcr3();
#else
    PageDirectory.QuadPart = 0LL;
#endif
    if (Process != NULL) 
    {
        KeUnstackDetachProcess(&ApcState);
    }

    return PageDirectory;
}

START_TEST(MmMapIoSpace)
{
    PHYSICAL_ADDRESS CurrentPageDirectory;
    PVOID Mapping;

    CurrentPageDirectory = GetProcessDirectory(PsInitialSystemProcess);
    if (!skip(CurrentPageDirectory.QuadPart != 0, "Cannot locate paging structures for this architecture\n"))
    {
        Mapping = MmMapIoSpace(CurrentPageDirectory, PAGE_SIZE, MmNonCached);
        ok(Mapping == NULL, "MmMapIoSpace on system process directory unexpectedly succeeded, expected NULL got 0x%p. This test is expected to fail on Windows 10 1803 or below\n", Mapping);
        if (Mapping) 
            MmUnmapIoSpace(Mapping, PAGE_SIZE);
    }

    CurrentPageDirectory = GetProcessDirectory(NULL);
    if (!skip(CurrentPageDirectory.QuadPart != 0, "Cannot locate paging structures for this architecture\n"))
    {
        Mapping = MmMapIoSpace(CurrentPageDirectory, PAGE_SIZE, MmNonCached);
        ok(Mapping != NULL, "MmMapIoSpace on running process directory failed, expected mapping got NULL.\n");
        if (Mapping) 
            MmUnmapIoSpace(Mapping, PAGE_SIZE);
    }
}
