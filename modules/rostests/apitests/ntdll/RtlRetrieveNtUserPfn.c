/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for RtlRetrieveNtUserPfn
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

typedef
NTSTATUS
WINAPI
FN_RtlRetrieveNtUserPfn(
    _Out_ const PVOID**,
    _Out_ const PVOID**,
    _Out_ const PVOID**);

FN_RtlRetrieveNtUserPfn* pRtlRetrieveNtUserPfn;

START_TEST(RtlRetrieveNtUserPfn)
{
    HINSTANCE hinstNtdll = GetModuleHandleA("ntdll.dll");
    pRtlRetrieveNtUserPfn = (void*)GetProcAddress(hinstNtdll, "RtlRetrieveNtUserPfn");
    if (!pRtlRetrieveNtUserPfn)
    {
        win_skip("RtlRetrieveNtUserPfn not supported\n");
        return;
    }

    // Get the start and end of the ".const" section in ntdll, and check that the retrieved pointers are within that range.
    PVOID NtDllTextStart = NULL, NtDllTextEnd = NULL;
    PVOID NtDllRdataStart = NULL, NtDllRdataEnd = NULL;
    PIMAGE_NT_HEADERS NtHeaders = RtlImageNtHeader(hinstNtdll);
    PIMAGE_SECTION_HEADER SectionHeaders = IMAGE_FIRST_SECTION(NtHeaders);

    for (int i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER Section = &SectionHeaders[i];
        if (memcmp(Section->Name, ".text", 5) == 0)
        {
            NtDllTextStart = (PBYTE)hinstNtdll + Section->VirtualAddress;
            NtDllTextEnd = (PBYTE)NtDllTextStart + Section->Misc.VirtualSize;
        }
        else if (memcmp(Section->Name, ".rdata", 6) == 0)
        {
            NtDllRdataStart = (PBYTE)hinstNtdll + Section->VirtualAddress;
            NtDllRdataEnd = (PBYTE)NtDllRdataStart + Section->Misc.VirtualSize;
        }
    }

    /* Some versions don't have a .rdata section */
    if (NtDllRdataStart == NULL)
    {
        /* Use .text instead */
        NtDllRdataStart = NtDllTextStart;
        NtDllRdataEnd = NtDllTextEnd;
    }

    ok(NtDllTextStart != NULL, "Failed to find .text section in ntdll.dll\n");
    ok(NtDllRdataStart != NULL, "Failed to find .rdata section in ntdll.dll\n");

    NTSTATUS Status;
    const PVOID *p1, *p2, *p3;

    /* We expect the user PFNs not to have been set up by now */
    Status = pRtlRetrieveNtUserPfn(&p1, &p2, &p3);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    if (GetNTVersion() >= _WIN32_WINNT_WIN7)
    {
        /* Load win32u.dll, which should NOT register the PFNs */
        HINSTANCE hinstWin32u = GetModuleHandleA("win32u.dll");
        ok(hinstWin32u == NULL, "win32u.dll should not be loaded yet\n");
        hinstWin32u = LoadLibraryA("win32u.dll");
        ok(hinstWin32u != NULL, "Failed to load win32u.dll\n");

        /* Try again (should still fail) */
        Status = pRtlRetrieveNtUserPfn(&p1, &p2, &p3);
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    }

    /* Load user32, which should register the PFNs */
    HINSTANCE hinstUser32 = GetModuleHandleA("user32.dll");
    ok(hinstUser32 == NULL, "user32.dll should not be loaded yet\n");
    hinstUser32 = LoadLibraryA("user32.dll");
    ok(hinstUser32 != NULL, "Failed to load user32.dll\n");

    /* Try again */
    Status = pRtlRetrieveNtUserPfn(&p1, &p2, &p3);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* The function retrieves three pointers to internal ntdll pointer arrays.
       All three arrays are located in the .rdata section of ntdll.dll,
       so check that the retrieved pointers are within that range. */
    ok((PVOID)p1 >= NtDllRdataStart && (PVOID)p1 < NtDllRdataEnd,
       "p1 (%p) is not within .rdata section (%p-%p)\n", p1, NtDllRdataStart, NtDllRdataEnd);
    ok((PVOID)p2 >= NtDllRdataStart && (PVOID)p2 < NtDllRdataEnd,
       "p2 (%p) is not within .rdata section (%p-%p)\n", p2, NtDllRdataStart, NtDllRdataEnd);
    ok((PVOID)p3 >= NtDllRdataStart && (PVOID)p3 < NtDllRdataEnd,
       "p3 (%p) is not within .rdata section (%p-%p)\n", p3, NtDllRdataStart, NtDllRdataEnd);

    // The arrays are expected to follow one after the other
    ok(p2 > p1, "p2 (%p) is not greater than p1 (%p)\n", p2, p1);
    ok(p3 > p2, "p3 (%p) is not greater than p2 (%p)\n", p3, p2);

    /* Get the field size */
    BOOL IsWow64;
    IsWow64Process(GetCurrentProcess(), &IsWow64);
    SIZE_T FieldSize = IsWow64 ? sizeof(ULONG64) : sizeof(PVOID);

    SIZE_T Count1 = ((PUCHAR)p2 - (PUCHAR)p1) / FieldSize;
    SIZE_T Count2 = ((PUCHAR)p3 - (PUCHAR)p2) / FieldSize;

    ok_eq_size(Count1, Count2);
    ok_eq_size(Count1, GetNTVersion() >= _WIN32_WINNT_WIN10 ? 24 : 23);

    /* Iterate over all function pointers from arrays 1 and 2 in one run */
    for (SIZE_T i = 0; i < (Count1 + Count2); i++)
    {
        PUCHAR pfn = *(PVOID*)((PUCHAR)p1 + i * FieldSize);
        ok((PVOID)pfn >= NtDllTextStart && (PVOID)pfn < NtDllTextEnd,
           "p1[%d] (%p) is not within .text section (%p-%p)\n", i, pfn, NtDllTextStart, NtDllTextEnd);

        /* Check the dispatch code */
        if (is_reactos() || GetNTVersion() >= _WIN32_WINNT_WIN10)
        {
#if defined(_M_IX86)
            ok_eq_size((ULONG_PTR)pfn & 15, 0); // The functions should be 16-byte aligned
            ok_eq_int(memcmp(&pfn[0], "\xFF\x25", 2), 0); // jmp dword ptr [x]
            ok_eq_int(memcmp(&pfn[6], "\x8D\xA4\x24\x00\x00\x00\x00", 7), 0); // lea esp, [esp]
            ok_eq_int(memcmp(&pfn[13], "\x8D\x49\x00", 3), 0); // lea ecx, [ecx]
#elif defined(_M_AMD64)
            ok_eq_size((ULONG_PTR)pfn & 15, 0); // The functions should be 16-byte aligned
            ok_eq_int(memcmp(&pfn[0], "\xFF\x25", 2), 0); // jmp qword ptr [rip + x]
            ok_eq_int(memcmp(&pfn[6], "\x66\x66\x0F\x1F\x84\x00\x00\x00\x00\x00", 10), 0); // nop word ptr [rax + rax]
#endif
        }
        else
        {
#if defined(_M_IX86)
            ok_eq_size((ULONG_PTR)pfn & 3, 0); // The functions should be 4-byte aligned
            ok_eq_int(memcmp(&pfn[0], "\xFF\x25", 2), 0); // jmp dword ptr [x]
            ok_eq_int(memcmp(&pfn[6], "\x90\x90\x90\x90\x90", 5), 0); // nop nop ...
#elif defined(_M_AMD64)
            ok_eq_size((ULONG_PTR)pfn & 3, 0); // The functions should be 4-byte aligned
            ok_eq_int(memcmp(&pfn[0], "\x48\xFF\x25", 3), 0); // jmp qword ptr [rip + x]
            //ok_eq_int(memcmp(&pfn[7], "\x90\x90\x90\x90\x90\x90\x90\x90\x90", 2), 0); // nop nop ...
#endif
        }
    }

    /* The 3rd array has only 11 entries, check those as well. */
    for (SIZE_T i = 0; i < 11; i++)
    {
        PUCHAR pfn = *(PVOID*)((PUCHAR)p1 + i * FieldSize);
        ok((PVOID)pfn >= NtDllTextStart && (PVOID)pfn < NtDllTextEnd,
           "p3[%d] (%p) is not within .text section (%p-%p)\n", i, pfn, NtDllTextStart, NtDllTextEnd);
    }

    StartSeh()
        Status = pRtlRetrieveNtUserPfn(NULL, &p2, &p3);
    EndSeh(STATUS_ACCESS_VIOLATION)

    StartSeh()
        Status = pRtlRetrieveNtUserPfn(&p1, NULL, &p3);
    EndSeh(STATUS_ACCESS_VIOLATION)

    StartSeh()
        Status = pRtlRetrieveNtUserPfn(&p1, &p2, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION)

    return;
}
