/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for RtlImageRvaToVa
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

static
PVOID
AllocateGuarded(
    _In_ SIZE_T SizeRequested)
{
    NTSTATUS Status;
    SIZE_T Size = PAGE_ROUND_UP(SizeRequested + PAGE_SIZE);
    PVOID VirtualMemory = NULL;
    PCHAR StartOfBuffer;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_RESERVE, PAGE_NOACCESS);

    if (!NT_SUCCESS(Status))
        return NULL;

    Size -= PAGE_SIZE;
    if (Size)
    {
        Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            Size = 0;
            Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
            ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
            return NULL;
        }
    }

    StartOfBuffer = VirtualMemory;
    StartOfBuffer += Size - SizeRequested;

    return StartOfBuffer;
}

static
VOID
FreeGuarded(
    _In_ PVOID Pointer)
{
    NTSTATUS Status;
    PVOID VirtualMemory = (PVOID)PAGE_ROUND_DOWN((SIZE_T)Pointer);
    SIZE_T Size = 0;

    Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
}

START_TEST(RtlImageRvaToVa)
{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER Section;
    ULONG NumberOfSections;
    ULONG ImageSize;
    PUCHAR BaseAddress;
    PVOID Va;
    PIMAGE_SECTION_HEADER OutSection;
    IMAGE_SECTION_HEADER DummySection;
    struct
    {
        ULONG Rva;
        ULONG VaOffset;
        ULONG SectionIndex;
    } Tests[] =
    {
        {      0,      0, 0 },
        {  0xfff,      0, 0 },
        { 0x1000, 0x3000, 0 },
        { 0x1001, 0x3001, 0 },
        { 0x1fff, 0x3fff, 0 },
        { 0x2000,      0, 0 },
        { 0x2fff,      0, 0 },
        { 0x3000, 0x4000, 3 },
        { 0x3fff, 0x4fff, 3 },
        { 0x4000, 0x5000, 3 },
        { 0x4fff, 0x5fff, 3 },
        { 0x5000,      0, 0 },
        { 0x5fff,      0, 0 },
        { 0x6000,      0, 0 },
        { 0x6fff,      0, 0 },
        { 0x7000, 0x7000, 5 },
        { 0x7fff, 0x7fff, 5 },
        { 0x8000, 0x9000, 7 },
        { 0x8fff, 0x9fff, 7 },
        { 0x9000, 0x8000, 6 },
        { 0x9fff, 0x8fff, 6 },
    };
    ULONG i;

    NumberOfSections = 8;
    ImageSize = FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader.DataDirectory) +
                NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
    NtHeader = AllocateGuarded(ImageSize);
    if (!NtHeader)
    {
        skip("Could not allocate %lu bytes\n", ImageSize);
        return;
    }

    RtlFillMemory(NtHeader, ImageSize, 0xDD);
    NtHeader->FileHeader.NumberOfSections = NumberOfSections;
    NtHeader->FileHeader.SizeOfOptionalHeader = FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, DataDirectory);
    Section = (PIMAGE_SECTION_HEADER)((PUCHAR)&NtHeader->OptionalHeader +
                                      NtHeader->FileHeader.SizeOfOptionalHeader);
    Section[0].VirtualAddress = 0x1000;
    Section[0].Misc.VirtualSize = 0x1000;
    Section[0].SizeOfRawData = 0x1000;
    Section[0].PointerToRawData = 0x3000;
    Section[1].VirtualAddress = 0x2000;
    Section[1].Misc.VirtualSize = 0;
    Section[1].SizeOfRawData = 0;
    Section[1].PointerToRawData = 0x4000;
    Section[2].VirtualAddress = 0x2000;
    Section[2].Misc.VirtualSize = 0x1000;
    Section[2].SizeOfRawData = 0;
    Section[2].PointerToRawData = 0x4000;
    Section[3].VirtualAddress = 0x3000;
    Section[3].Misc.VirtualSize = 0x1000;
    Section[3].SizeOfRawData = 0x2000;
    Section[3].PointerToRawData = 0x4000;
    Section[4].VirtualAddress = 0x4000;
    Section[4].Misc.VirtualSize = 0x2000;
    Section[4].SizeOfRawData = 0x1000;
    Section[4].PointerToRawData = 0x6000;
    Section[5].VirtualAddress = 0x7000;
    Section[5].Misc.VirtualSize = 0x1000;
    Section[5].SizeOfRawData = 0x1000;
    Section[5].PointerToRawData = 0x7000;
    Section[6].VirtualAddress = 0x9000;
    Section[6].Misc.VirtualSize = 0x1000;
    Section[6].SizeOfRawData = 0x1000;
    Section[6].PointerToRawData = 0x8000;
    Section[7].VirtualAddress = 0x8000;
    Section[7].Misc.VirtualSize = 0x1000;
    Section[7].SizeOfRawData = 0x1000;
    Section[7].PointerToRawData = 0x9000;
    DummySection.VirtualAddress = 0xf000;
    DummySection.Misc.VirtualSize = 0xf000;
    DummySection.SizeOfRawData = 0xf000;
    DummySection.PointerToRawData = 0xf000;

    BaseAddress = (PUCHAR)0x2000000;

    StartSeh()
        RtlImageRvaToVa(NULL, NULL, 0, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    Va = RtlImageRvaToVa(NtHeader, NULL, 0, NULL);
    ok(Va == NULL, "Va = %p\n", Va);

    Va = RtlImageRvaToVa(NtHeader, BaseAddress, 0, NULL);
    ok(Va == NULL, "Va = %p\n", Va);

    OutSection = NULL;
    Va = RtlImageRvaToVa(NtHeader, BaseAddress, 0, &OutSection);
    ok(Va == NULL, "Va = %p\n", Va);
    ok(OutSection == NULL, "OutSection = %p\n", OutSection);

    OutSection = (PVOID)1;
    StartSeh()
        RtlImageRvaToVa(NtHeader, BaseAddress, 0, &OutSection);
    EndSeh(STATUS_ACCESS_VIOLATION);

    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        /* Section = not specified */
        StartSeh()
            Va = RtlImageRvaToVa(NtHeader, BaseAddress, Tests[i].Rva, NULL);
        EndSeh(STATUS_SUCCESS);
        if (Tests[i].VaOffset == 0)
            ok(Va == NULL, "[0x%lx] Va = %p\n", Tests[i].Rva, Va);
        else
            ok(Va == BaseAddress + Tests[i].VaOffset, "[0x%lx] Va = %p\n", Tests[i].Rva, Va);

        /* Section = NULL */
        OutSection = NULL;
        StartSeh()
            Va = RtlImageRvaToVa(NtHeader, BaseAddress, Tests[i].Rva, &OutSection);
        EndSeh(STATUS_SUCCESS);
        if (Tests[i].VaOffset == 0)
        {
            ok(Va == NULL, "[0x%lx] Va = %p\n", Tests[i].Rva, Va);
            ok(OutSection == NULL, "[0x%lx] OutSection = %p (%Id)\n", Tests[i].Rva, OutSection, OutSection - Section);
        }
        else
        {
            ok(Va == BaseAddress + Tests[i].VaOffset, "[0x%lx] Va = %p\n", Tests[i].Rva, Va);
            ok(OutSection == &Section[Tests[i].SectionIndex], "[0x%lx] OutSection = %p (%Id)\n", Tests[i].Rva, OutSection, OutSection - Section);
        }

        /* Section = first section */
        OutSection = Section;
        StartSeh()
            Va = RtlImageRvaToVa(NtHeader, BaseAddress, Tests[i].Rva, &OutSection);
        EndSeh(STATUS_SUCCESS);
        if (Tests[i].VaOffset == 0)
        {
            ok(Va == NULL, "[0x%lx] Va = %p\n", Tests[i].Rva, Va);
            ok(OutSection == Section, "[0x%lx] OutSection = %p (%Id)\n", Tests[i].Rva, OutSection, OutSection - Section);
        }
        else
        {
            ok(Va == BaseAddress + Tests[i].VaOffset, "[0x%lx] Va = %p\n", Tests[i].Rva, Va);
            ok(OutSection == &Section[Tests[i].SectionIndex], "[0x%lx] OutSection = %p (%Id)\n", Tests[i].Rva, OutSection, OutSection - Section);
        }

        /* Section = dummy section */
        OutSection = &DummySection;
        StartSeh()
            Va = RtlImageRvaToVa(NtHeader, BaseAddress, Tests[i].Rva, &OutSection);
        EndSeh(STATUS_SUCCESS);
        if (Tests[i].VaOffset == 0)
        {
            ok(Va == NULL, "[0x%lx] Va = %p\n", Tests[i].Rva, Va);
            ok(OutSection == &DummySection, "[0x%lx] OutSection = %p (%Id)\n", Tests[i].Rva, OutSection, OutSection - Section);
        }
        else
        {
            ok(Va == BaseAddress + Tests[i].VaOffset, "[0x%lx] Va = %p\n", Tests[i].Rva, Va);
            ok(OutSection == &Section[Tests[i].SectionIndex], "[0x%lx] OutSection = %p (%Id)\n", Tests[i].Rva, OutSection, OutSection - Section);
        }
    }

    FreeGuarded(NtHeader);
}
