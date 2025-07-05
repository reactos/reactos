/*
 * Unit test suite for the PE loader.
 *
 * Copyright 2006,2011 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winuser.h"
#include "wine/test.h"
#include "delayloadhandler.h"

/* PROCESS_ALL_ACCESS in Vista+ PSDKs is incompatible with older Windows versions */
#define PROCESS_ALL_ACCESS_NT4 (PROCESS_ALL_ACCESS & ~0xf000)

#define ALIGN_SIZE(size, alignment) (((size) + (alignment - 1)) & ~((alignment - 1)))

struct PROCESS_BASIC_INFORMATION_PRIVATE
{
    DWORD_PTR ExitStatus;
    PPEB      PebBaseAddress;
    DWORD_PTR AffinityMask;
    DWORD_PTR BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
};

static LONG *child_failures;
static WORD cb_count;
static DWORD page_size;
static BOOL is_win64 = sizeof(void *) > sizeof(int);
static BOOL is_wow64;

static NTSTATUS (WINAPI *pNtCreateSection)(HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES *,
                                           const LARGE_INTEGER *, ULONG, ULONG, HANDLE );
static NTSTATUS (WINAPI *pNtQuerySection)(HANDLE, SECTION_INFORMATION_CLASS, void *, SIZE_T, SIZE_T *);
static NTSTATUS (WINAPI *pNtMapViewOfSection)(HANDLE, HANDLE, PVOID *, ULONG, SIZE_T, const LARGE_INTEGER *, SIZE_T *, ULONG, ULONG, ULONG);
static NTSTATUS (WINAPI *pNtUnmapViewOfSection)(HANDLE, PVOID);
static NTSTATUS (WINAPI *pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtSetInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);
static NTSTATUS (WINAPI *pNtTerminateProcess)(HANDLE, DWORD);
static void (WINAPI *pLdrShutdownProcess)(void);
static BOOLEAN (WINAPI *pRtlDllShutdownInProgress)(void);
static NTSTATUS (WINAPI *pNtAllocateVirtualMemory)(HANDLE, PVOID *, ULONG, SIZE_T *, ULONG, ULONG);
static NTSTATUS (WINAPI *pNtFreeVirtualMemory)(HANDLE, PVOID *, SIZE_T *, ULONG);
static NTSTATUS (WINAPI *pLdrLockLoaderLock)(ULONG, ULONG *, ULONG_PTR *);
static NTSTATUS (WINAPI *pLdrUnlockLoaderLock)(ULONG, ULONG_PTR);
static void (WINAPI *pRtlAcquirePebLock)(void);
static void (WINAPI *pRtlReleasePebLock)(void);
static PVOID    (WINAPI *pResolveDelayLoadedAPI)(PVOID, PCIMAGE_DELAYLOAD_DESCRIPTOR,
                                                 PDELAYLOAD_FAILURE_DLL_CALLBACK, PVOID,
                                                 PIMAGE_THUNK_DATA ThunkAddress,ULONG);
static PVOID (WINAPI *pRtlImageDirectoryEntryToData)(HMODULE,BOOL,WORD,ULONG *);
static DWORD (WINAPI *pFlsAlloc)(PFLS_CALLBACK_FUNCTION);
static BOOL (WINAPI *pFlsSetValue)(DWORD, PVOID);
static PVOID (WINAPI *pFlsGetValue)(DWORD);
static BOOL (WINAPI *pFlsFree)(DWORD);
static BOOL (WINAPI *pIsWow64Process)(HANDLE,PBOOL);

static PVOID RVAToAddr(DWORD_PTR rva, HMODULE module)
{
    if (rva == 0)
        return NULL;
    return ((char*) module) + rva;
}

static IMAGE_DOS_HEADER dos_header;

static const IMAGE_NT_HEADERS nt_header_template =
{
    IMAGE_NT_SIGNATURE, /* Signature */
    {
#if defined __i386__
      IMAGE_FILE_MACHINE_I386, /* Machine */
#elif defined __x86_64__
      IMAGE_FILE_MACHINE_AMD64, /* Machine */
#elif defined __powerpc__
      IMAGE_FILE_MACHINE_POWERPC, /* Machine */
#elif defined __arm__
      IMAGE_FILE_MACHINE_ARMNT, /* Machine */
#elif defined __aarch64__
      IMAGE_FILE_MACHINE_ARM64, /* Machine */
#else
# error You must specify the machine type
#endif
      1, /* NumberOfSections */
      0, /* TimeDateStamp */
      0, /* PointerToSymbolTable */
      0, /* NumberOfSymbols */
      sizeof(IMAGE_OPTIONAL_HEADER), /* SizeOfOptionalHeader */
      IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL /* Characteristics */
    },
    { IMAGE_NT_OPTIONAL_HDR_MAGIC, /* Magic */
      1, /* MajorLinkerVersion */
      0, /* MinorLinkerVersion */
      0, /* SizeOfCode */
      0, /* SizeOfInitializedData */
      0, /* SizeOfUninitializedData */
      0, /* AddressOfEntryPoint */
      0x10, /* BaseOfCode, also serves as e_lfanew in the truncated MZ header */
#ifndef _WIN64
      0, /* BaseOfData */
#endif
      0x10000000, /* ImageBase */
      0, /* SectionAlignment */
      0, /* FileAlignment */
      4, /* MajorOperatingSystemVersion */
      0, /* MinorOperatingSystemVersion */
      1, /* MajorImageVersion */
      0, /* MinorImageVersion */
      4, /* MajorSubsystemVersion */
      0, /* MinorSubsystemVersion */
      0, /* Win32VersionValue */
      sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER) + 0x1000, /* SizeOfImage */
      sizeof(dos_header) + sizeof(nt_header_template), /* SizeOfHeaders */
      0, /* CheckSum */
      IMAGE_SUBSYSTEM_WINDOWS_CUI, /* Subsystem */
      0, /* DllCharacteristics */
      0, /* SizeOfStackReserve */
      0, /* SizeOfStackCommit */
      0, /* SizeOfHeapReserve */
      0, /* SizeOfHeapCommit */
      0, /* LoaderFlags */
      0, /* NumberOfRvaAndSizes */
      { { 0 } } /* DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] */
    }
};

static IMAGE_SECTION_HEADER section =
{
    ".rodata", /* Name */
    { 0 }, /* Misc */
    0, /* VirtualAddress */
    0, /* SizeOfRawData */
    0, /* PointerToRawData */
    0, /* PointerToRelocations */
    0, /* PointerToLinenumbers */
    0, /* NumberOfRelocations */
    0, /* NumberOfLinenumbers */
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ, /* Characteristics */
};


static const char filler[0x1000];
static const char section_data[0x10] = "section data";

static DWORD create_test_dll( const IMAGE_DOS_HEADER *dos_header, UINT dos_size,
                              const IMAGE_NT_HEADERS *nt_header, char dll_name[MAX_PATH] )
{
    char temp_path[MAX_PATH];
    DWORD dummy, size, file_align;
    HANDLE hfile;
    BOOL ret;

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "ldr", 0, dll_name);

    hfile = CreateFileA(dll_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
    ok( hfile != INVALID_HANDLE_VALUE, "failed to create %s err %u\n", dll_name, GetLastError() );
    if (hfile == INVALID_HANDLE_VALUE) return 0;

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, dos_header, dos_size, &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    if (nt_header->FileHeader.SizeOfOptionalHeader)
    {
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &nt_header->OptionalHeader,
                        sizeof(IMAGE_OPTIONAL_HEADER),
                        &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());
        if (nt_header->FileHeader.SizeOfOptionalHeader > sizeof(IMAGE_OPTIONAL_HEADER))
        {
            file_align = nt_header->FileHeader.SizeOfOptionalHeader - sizeof(IMAGE_OPTIONAL_HEADER);
            assert(file_align < sizeof(filler));
            SetLastError(0xdeadbeef);
            ret = WriteFile(hfile, filler, file_align, &dummy, NULL);
            ok(ret, "WriteFile error %d\n", GetLastError());
        }
    }

    assert(nt_header->FileHeader.NumberOfSections <= 1);
    if (nt_header->FileHeader.NumberOfSections)
    {
        SetFilePointer(hfile, dos_size + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + nt_header->FileHeader.SizeOfOptionalHeader, NULL, FILE_BEGIN);

        section.SizeOfRawData = 10;

        if (nt_header->OptionalHeader.SectionAlignment >= page_size)
        {
            section.PointerToRawData = dos_size;
            section.VirtualAddress = nt_header->OptionalHeader.SectionAlignment;
            section.Misc.VirtualSize = section.SizeOfRawData * 10;
        }
        else
        {
            section.PointerToRawData = nt_header->OptionalHeader.SizeOfHeaders;
            section.VirtualAddress = nt_header->OptionalHeader.SizeOfHeaders;
            section.Misc.VirtualSize = 5;
        }

        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &section, sizeof(section), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());

        /* section data */
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, section_data, sizeof(section_data), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());
    }

    /* Minimal PE image that Windows7+ is able to load: 268 bytes */
    size = GetFileSize(hfile, NULL);
    if (size < 268)
    {
        file_align = 268 - size;
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, filler, file_align, &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());
    }

    size = GetFileSize(hfile, NULL);
    CloseHandle(hfile);
    return size;
}

static DWORD create_test_dll_sections( const IMAGE_DOS_HEADER *dos_header, const IMAGE_NT_HEADERS *nt_header,
                                       const IMAGE_SECTION_HEADER *sections, const void *section_data,
                                       char dll_name[MAX_PATH] )
{
    char temp_path[MAX_PATH];
    DWORD dummy, i, size;
    HANDLE hfile;
    BOOL ret;

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "ldr", 0, dll_name);

    hfile = CreateFileA(dll_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
    ok( hfile != INVALID_HANDLE_VALUE, "failed to create %s err %u\n", dll_name, GetLastError() );
    if (hfile == INVALID_HANDLE_VALUE) return 0;

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, dos_header, sizeof(*dos_header), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, nt_header, offsetof(IMAGE_NT_HEADERS, OptionalHeader) + nt_header->FileHeader.SizeOfOptionalHeader, &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, sections, sizeof(*sections) * nt_header->FileHeader.NumberOfSections,
                    &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    for (i = 0; i < nt_header->FileHeader.NumberOfSections; i++)
    {
        SetFilePointer(hfile, sections[i].PointerToRawData, NULL, FILE_BEGIN);
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, section_data, sections[i].SizeOfRawData, &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());
    }
    size = GetFileSize(hfile, NULL);
    CloseHandle(hfile);
    return size;
}

static BOOL query_image_section( int id, const char *dll_name, const IMAGE_NT_HEADERS *nt_header,
                                 const void *section_data )
{
    static BOOL is_winxp;
    SECTION_BASIC_INFORMATION info;
    SECTION_IMAGE_INFORMATION image;
    const IMAGE_COR20_HEADER *cor_header = NULL;
    SIZE_T info_size = (SIZE_T)0xdeadbeef << 16;
    NTSTATUS status;
    HANDLE file, mapping;
    ULONG file_size;
    LARGE_INTEGER map_size;
    SIZE_T max_stack, commit_stack;
    void *entry_point;

    /* truncated header is not handled correctly in windows <= w2k3 */
    BOOL truncated;

    file = CreateFileA( dll_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "%u: CreateFile error %d\n", id, GetLastError() );
    file_size = GetFileSize( file, NULL );

    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, NULL, PAGE_READONLY, SEC_IMAGE, file );
    ok( !status, "%u: NtCreateSection failed err %x\n", id, status );
    if (status)
    {
        CloseHandle( file );
        return FALSE;
    }
    status = pNtQuerySection( mapping, SectionImageInformation, &image, sizeof(image), &info_size );
    ok( !status, "%u: NtQuerySection failed err %x\n", id, status );
    ok( info_size == sizeof(image), "%u: NtQuerySection wrong size %lu\n", id, info_size );
    if (nt_header->OptionalHeader.Magic == (is_win64 ? IMAGE_NT_OPTIONAL_HDR64_MAGIC
                                                     : IMAGE_NT_OPTIONAL_HDR32_MAGIC))
    {
        max_stack = nt_header->OptionalHeader.SizeOfStackReserve;
        commit_stack = nt_header->OptionalHeader.SizeOfStackCommit;
        entry_point = (char *)nt_header->OptionalHeader.ImageBase + nt_header->OptionalHeader.AddressOfEntryPoint;
        truncated = nt_header->FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER);
        if (!truncated &&
            nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress &&
            nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size)
            cor_header = section_data;
    }
    else if (nt_header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_NT_HEADERS64 *nt64 = (const IMAGE_NT_HEADERS64 *)nt_header;
        max_stack = 0x100000;
        commit_stack = 0x10000;
        entry_point = (void *)0x81231234;
        truncated = nt_header->FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER64);
        if (!truncated &&
            nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress &&
            nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size)
            cor_header = section_data;
    }
    else
    {
        const IMAGE_NT_HEADERS32 *nt32 = (const IMAGE_NT_HEADERS32 *)nt_header;
        max_stack = nt32->OptionalHeader.SizeOfStackReserve;
        commit_stack = nt32->OptionalHeader.SizeOfStackCommit;
        entry_point = (char *)(ULONG_PTR)nt32->OptionalHeader.ImageBase + nt32->OptionalHeader.AddressOfEntryPoint;
        truncated = nt_header->FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER32);
        if (!truncated &&
            nt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress &&
            nt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size)
            cor_header = section_data;
    }
    ok( (char *)image.TransferAddress == (char *)entry_point ||
        (S(U(image)).ImageDynamicallyRelocated && LOWORD(image.TransferAddress) == LOWORD(entry_point)),
        "%u: TransferAddress wrong %p / %p (%08x)\n", id,
        image.TransferAddress, entry_point, nt_header->OptionalHeader.AddressOfEntryPoint );
    ok( image.ZeroBits == 0, "%u: ZeroBits wrong %08x\n", id, image.ZeroBits );
    ok( image.MaximumStackSize == max_stack || broken(truncated),
        "%u: MaximumStackSize wrong %lx / %lx\n", id, image.MaximumStackSize, max_stack );
    ok( image.CommittedStackSize == commit_stack || broken(truncated),
        "%u: CommittedStackSize wrong %lx / %lx\n", id, image.CommittedStackSize, commit_stack );
    if (truncated)
        ok( !image.SubSystemType || broken(truncated),
            "%u: SubSystemType wrong %08x / 00000000\n", id, image.SubSystemType );
    else
        ok( image.SubSystemType == nt_header->OptionalHeader.Subsystem,
            "%u: SubSystemType wrong %08x / %08x\n", id,
            image.SubSystemType, nt_header->OptionalHeader.Subsystem );
    ok( image.MinorSubsystemVersion == nt_header->OptionalHeader.MinorSubsystemVersion,
        "%u: SubsystemVersionLow wrong %04x / %04x\n", id,
        image.MinorSubsystemVersion, nt_header->OptionalHeader.MinorSubsystemVersion );
    ok( image.MajorSubsystemVersion == nt_header->OptionalHeader.MajorSubsystemVersion,
        "%u: SubsystemVersionHigh wrong %04x / %04x\n", id,
        image.MajorSubsystemVersion, nt_header->OptionalHeader.MajorSubsystemVersion );
    ok( image.ImageCharacteristics == nt_header->FileHeader.Characteristics,
        "%u: ImageCharacteristics wrong %04x / %04x\n", id,
        image.ImageCharacteristics, nt_header->FileHeader.Characteristics );
    ok( image.DllCharacteristics == nt_header->OptionalHeader.DllCharacteristics || broken(truncated),
        "%u: DllCharacteristics wrong %04x / %04x\n", id,
        image.DllCharacteristics, nt_header->OptionalHeader.DllCharacteristics );
    ok( image.Machine == nt_header->FileHeader.Machine, "%u: Machine wrong %04x / %04x\n", id,
        image.Machine, nt_header->FileHeader.Machine );
    ok( image.LoaderFlags == (cor_header != NULL), "%u: LoaderFlags wrong %08x\n", id, image.LoaderFlags );
    ok( image.ImageFileSize == file_size || broken(!image.ImageFileSize), /* winxpsp1 */
        "%u: ImageFileSize wrong %08x / %08x\n", id, image.ImageFileSize, file_size );
    ok( image.CheckSum == nt_header->OptionalHeader.CheckSum || broken(truncated),
        "%u: CheckSum wrong %08x / %08x\n", id,
        image.CheckSum, nt_header->OptionalHeader.CheckSum );

    if (nt_header->OptionalHeader.SizeOfCode || nt_header->OptionalHeader.AddressOfEntryPoint)
        ok( image.ImageContainsCode == TRUE, "%u: ImageContainsCode wrong %u\n", id,
            image.ImageContainsCode );
    else if ((nt_header->OptionalHeader.SectionAlignment % page_size) ||
             (nt_header->FileHeader.NumberOfSections == 1 &&
              (section.Characteristics & IMAGE_SCN_MEM_EXECUTE)))
        ok( image.ImageContainsCode == TRUE || broken(!image.ImageContainsCode), /* <= win8 */
            "%u: ImageContainsCode wrong %u\n", id, image.ImageContainsCode );
    else
        ok( !image.ImageContainsCode, "%u: ImageContainsCode wrong %u\n", id, image.ImageContainsCode );

    if (cor_header &&
        (cor_header->Flags & COMIMAGE_FLAGS_ILONLY) &&
        (cor_header->MajorRuntimeVersion > 2 ||
         (cor_header->MajorRuntimeVersion == 2 && cor_header->MinorRuntimeVersion >= 5)))
    {
        ok( S(U(image)).ComPlusILOnly || broken(is_winxp),
            "%u: wrong ComPlusILOnly flags %02x\n", id, U(image).ImageFlags );
        if (nt_header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
            !(cor_header->Flags & COMIMAGE_FLAGS_32BITREQUIRED))
            ok( S(U(image)).ComPlusNativeReady || broken(is_winxp),
                "%u: wrong ComPlusNativeReady flags %02x\n", id, U(image).ImageFlags );
        else
            ok( !S(U(image)).ComPlusNativeReady,
                "%u: wrong ComPlusNativeReady flags %02x\n", id, U(image).ImageFlags );
    }
    else
    {
        ok( !S(U(image)).ComPlusILOnly, "%u: wrong ComPlusILOnly flags %02x\n", id, U(image).ImageFlags );
        ok( !S(U(image)).ComPlusNativeReady, "%u: wrong ComPlusNativeReady flags %02x\n", id, U(image).ImageFlags );
    }
    if (!(nt_header->OptionalHeader.SectionAlignment % page_size))
        ok( !S(U(image)).ImageMappedFlat, "%u: wrong ImageMappedFlat flags %02x\n", id, U(image).ImageFlags );
    else
    {
        /* winxp doesn't support any of the loader flags */
        if (!S(U(image)).ImageMappedFlat) is_winxp = TRUE;
        ok( S(U(image)).ImageMappedFlat || broken(is_winxp),
        "%u: wrong ImageMappedFlat flags %02x\n", id, U(image).ImageFlags );
    }
    if (!(nt_header->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE))
        ok( !S(U(image)).ImageDynamicallyRelocated || broken( S(U(image)).ComPlusILOnly ), /* <= win7 */
            "%u: wrong ImageDynamicallyRelocated flags %02x\n", id, U(image).ImageFlags );
    else if (image.ImageContainsCode && !cor_header)
        ok( S(U(image)).ImageDynamicallyRelocated || broken(is_winxp),
            "%u: wrong ImageDynamicallyRelocated flags %02x\n", id, U(image).ImageFlags );
    else
        ok( !S(U(image)).ImageDynamicallyRelocated || broken(TRUE), /* <= win8 */
            "%u: wrong ImageDynamicallyRelocated flags %02x\n", id, U(image).ImageFlags );
    ok( !S(U(image)).BaseBelow4gb, "%u: wrong BaseBelow4gb flags %02x\n", id, U(image).ImageFlags );

    /* FIXME: needs more work: */
    /* image.GpValue */

    map_size.QuadPart = (nt_header->OptionalHeader.SizeOfImage + page_size - 1) & ~(page_size - 1);
    status = pNtQuerySection( mapping, SectionBasicInformation, &info, sizeof(info), NULL );
    ok( !status, "NtQuerySection failed err %x\n", status );
    ok( info.Size.QuadPart == map_size.QuadPart, "NtQuerySection wrong size %x%08x / %x%08x\n",
        info.Size.u.HighPart, info.Size.u.LowPart, map_size.u.HighPart, map_size.u.LowPart );
    CloseHandle( mapping );

    map_size.QuadPart = (nt_header->OptionalHeader.SizeOfImage + page_size - 1) & ~(page_size - 1);
    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, &map_size, PAGE_READONLY, SEC_IMAGE, file );
    ok( !status, "%u: NtCreateSection failed err %x\n", id, status );
    status = pNtQuerySection( mapping, SectionBasicInformation, &info, sizeof(info), NULL );
    ok( !status, "NtQuerySection failed err %x\n", status );
    ok( info.Size.QuadPart == map_size.QuadPart, "NtQuerySection wrong size %x%08x / %x%08x\n",
        info.Size.u.HighPart, info.Size.u.LowPart, map_size.u.HighPart, map_size.u.LowPart );
    CloseHandle( mapping );

    map_size.QuadPart++;
    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, &map_size, PAGE_READONLY, SEC_IMAGE, file );
    ok( status == STATUS_SECTION_TOO_BIG, "%u: NtCreateSection failed err %x\n", id, status );

    SetFilePointerEx( file, map_size, NULL, FILE_BEGIN );
    SetEndOfFile( file );
    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, &map_size, PAGE_READONLY, SEC_IMAGE, file );
    ok( status == STATUS_SECTION_TOO_BIG, "%u: NtCreateSection failed err %x\n", id, status );

    map_size.QuadPart = 1;
    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, &map_size, PAGE_READONLY, SEC_IMAGE, file );
    ok( !status, "%u: NtCreateSection failed err %x\n", id, status );
    status = pNtQuerySection( mapping, SectionBasicInformation, &info, sizeof(info), NULL );
    ok( !status, "NtQuerySection failed err %x\n", status );
    ok( info.Size.QuadPart == map_size.QuadPart, "NtQuerySection wrong size %x%08x / %x%08x\n",
        info.Size.u.HighPart, info.Size.u.LowPart, map_size.u.HighPart, map_size.u.LowPart );
    CloseHandle( mapping );

    CloseHandle( file );
    return image.ImageContainsCode && (!cor_header || !(cor_header->Flags & COMIMAGE_FLAGS_ILONLY));
}

/* helper to test image section mapping */
static NTSTATUS map_image_section( const IMAGE_NT_HEADERS *nt_header, const IMAGE_SECTION_HEADER *sections,
                                   const void *section_data, int line )
{
    char dll_name[MAX_PATH];
    LARGE_INTEGER size;
    HANDLE file, map;
    NTSTATUS status;
    ULONG file_size;
    BOOL has_code;
    HMODULE mod;

    file_size = create_test_dll_sections( &dos_header, nt_header, sections, section_data, dll_name );

    file = CreateFileA(dll_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    size.QuadPart = file_size;
    status = pNtCreateSection(&map, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                              NULL, &size, PAGE_READONLY, SEC_IMAGE, file );
    if (!status)
    {
        SECTION_BASIC_INFORMATION info;
        SIZE_T info_size = 0xdeadbeef;
        NTSTATUS ret = pNtQuerySection( map, SectionBasicInformation, &info, sizeof(info), &info_size );
        ok( !ret, "NtQuerySection failed err %x\n", ret );
        ok( info_size == sizeof(info), "NtQuerySection wrong size %lu\n", info_size );
        ok( info.Attributes == (SEC_IMAGE | SEC_FILE), "NtQuerySection wrong attr %x\n", info.Attributes );
        ok( info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", info.BaseAddress );
        ok( info.Size.QuadPart == file_size, "NtQuerySection wrong size %x%08x / %08x\n",
            info.Size.u.HighPart, info.Size.u.LowPart, file_size );
        has_code = query_image_section( line, dll_name, nt_header, section_data );
        /* test loading dll of wrong 32/64 bitness */
        if (nt_header->OptionalHeader.Magic == (is_win64 ? IMAGE_NT_OPTIONAL_HDR32_MAGIC
                                                         : IMAGE_NT_OPTIONAL_HDR64_MAGIC))
        {
            SetLastError( 0xdeadbeef );
            mod = LoadLibraryExA( dll_name, 0, DONT_RESOLVE_DLL_REFERENCES );
            if (!has_code && nt_header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                BOOL il_only = FALSE;
                if (((const IMAGE_NT_HEADERS32 *)nt_header)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress)
                {
                    const IMAGE_COR20_HEADER *cor_header = section_data;
                    il_only = (cor_header->Flags & COMIMAGE_FLAGS_ILONLY) != 0;
                }
                ok( mod != NULL || broken(il_only), /* <= win7 */
                    "%u: loading failed err %u\n", line, GetLastError() );
            }
            else
            {
                ok( !mod, "%u: loading succeeded\n", line );
                ok( GetLastError() == ERROR_BAD_EXE_FORMAT, "%u: wrong error %u\n", line, GetLastError() );
            }
            if (mod) FreeLibrary( mod );
        }
    }
    if (map) CloseHandle( map );
    CloseHandle( file );
    DeleteFileA( dll_name );
    return status;
}


static void test_Loader(void)
{
    static const struct test_data
    {
        DWORD size_of_dos_header;
        WORD number_of_sections, size_of_optional_header;
        DWORD section_alignment, file_alignment;
        DWORD size_of_image, size_of_headers;
        DWORD errors[4]; /* 0 means LoadLibrary should succeed */
    } td[] =
    {
        { sizeof(dos_header),
          1, 0, 0, 0, 0, 0,
          { ERROR_BAD_EXE_FORMAT }
        },
        { sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER) + 0xe00,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_BAD_EXE_FORMAT } /* XP doesn't like too small image size */
        },
        { sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_SUCCESS }
        },
        { sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          0x1f00,
          0x1000,
          { ERROR_SUCCESS }
        },
        { sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x200, 0x200,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER) + 0x200,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_SUCCESS, ERROR_INVALID_ADDRESS } /* vista is more strict */
        },
        { sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x200, 0x1000,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_BAD_EXE_FORMAT } /* XP doesn't like alignments */
        },
        { sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_SUCCESS }
        },
        { sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          0x200,
          { ERROR_SUCCESS }
        },
        /* Mandatory are all fields up to SizeOfHeaders, everything else
         * is really optional (at least that's true for XP).
         */
#if 0 /* 32-bit Windows 8 crashes inside of LoadLibrary */
        { sizeof(dos_header),
          1, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          sizeof(dos_header) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(IMAGE_SECTION_HEADER) + 0x10,
          sizeof(dos_header) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT, ERROR_INVALID_ADDRESS,
            ERROR_NOACCESS }
        },
#endif
        { sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0xd0, /* beyond of the end of file */
          0xc0, /* beyond of the end of file */
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
        { sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0x1000,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
        { sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          1,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
#if 0 /* not power of 2 alignments need more test cases */
        { sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x300, 0x300,
          1,
          0,
          { ERROR_BAD_EXE_FORMAT } /* alignment is not power of 2 */
        },
#endif
        { sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 4, 4,
          1,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
        { sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 1, 1,
          1,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
        { sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0,
          0,
          { ERROR_BAD_EXE_FORMAT } /* image size == 0 -> failure */
        },
        /* the following data mimics the PE image which upack creates */
        { 0x10,
          1, 0x148, 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header_template) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          0x200,
          { ERROR_SUCCESS }
        },
        /* Minimal PE image that XP is able to load: 92 bytes */
        { 0x04,
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum),
          0x04 /* also serves as e_lfanew in the truncated MZ header */, 0x04,
          1,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
        /* Minimal PE image that Windows7 is able to load: 268 bytes */
        { 0x04,
          0, 0xf0, /* optional header size just forces 0xf0 bytes to be written,
                      0 or another number don't change the behaviour, what really
                      matters is file size regardless of values in the headers */
          0x04 /* also serves as e_lfanew in the truncated MZ header */, 0x04,
          0x40, /* minimal image size that Windows7 accepts */
          0,
          { ERROR_SUCCESS }
        },
        /* the following data mimics the PE image which 8k demos have */
        { 0x04,
          0, 0x08,
          0x04 /* also serves as e_lfanew in the truncated MZ header */, 0x04,
          0x200000,
          0x40,
          { ERROR_SUCCESS }
        }
    };
    int i;
    DWORD file_size;
    HANDLE h;
    HMODULE hlib, hlib_as_data_file;
    char dll_name[MAX_PATH];
    SIZE_T size;
    BOOL ret;
    NTSTATUS status;
    WORD alt_machine, orig_machine = nt_header_template.FileHeader.Machine;
    IMAGE_NT_HEADERS nt_header;
    IMAGE_COR20_HEADER cor_header;

    /* prevent displaying of the "Unable to load this DLL" message box */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        nt_header = nt_header_template;
        nt_header.FileHeader.NumberOfSections = td[i].number_of_sections;
        nt_header.FileHeader.SizeOfOptionalHeader = td[i].size_of_optional_header;

        nt_header.OptionalHeader.SectionAlignment = td[i].section_alignment;
        nt_header.OptionalHeader.FileAlignment = td[i].file_alignment;
        nt_header.OptionalHeader.SizeOfImage = td[i].size_of_image;
        nt_header.OptionalHeader.SizeOfHeaders = td[i].size_of_headers;

        file_size = create_test_dll( &dos_header, td[i].size_of_dos_header, &nt_header, dll_name );

        SetLastError(0xdeadbeef);
        hlib = LoadLibraryA(dll_name);
        if (hlib)
        {
            MEMORY_BASIC_INFORMATION info;
            void *ptr;

            ok( td[i].errors[0] == ERROR_SUCCESS, "%d: should have failed\n", i );

            SetLastError(0xdeadbeef);
            size = VirtualQuery(hlib, &info, sizeof(info));
            ok(size == sizeof(info),
                "%d: VirtualQuery error %d\n", i, GetLastError());
            ok(info.BaseAddress == hlib, "%d: %p != %p\n", i, info.BaseAddress, hlib);
            ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
            ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
            ok(info.RegionSize == ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size), "%d: got %lx != expected %x\n",
               i, info.RegionSize, ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size));
            ok(info.State == MEM_COMMIT, "%d: %x != MEM_COMMIT\n", i, info.State);
            if (nt_header.OptionalHeader.SectionAlignment < page_size)
                ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.Protect);
            else
                ok(info.Protect == PAGE_READONLY, "%d: %x != PAGE_READONLY\n", i, info.Protect);
            ok(info.Type == SEC_IMAGE, "%d: %x != SEC_IMAGE\n", i, info.Type);

            SetLastError(0xdeadbeef);
            ptr = VirtualAlloc(hlib, page_size, MEM_COMMIT, info.Protect);
            ok(!ptr, "%d: VirtualAlloc should fail\n", i);
            ok(GetLastError() == ERROR_ACCESS_DENIED, "%d: expected ERROR_ACCESS_DENIED, got %d\n", i, GetLastError());

            SetLastError(0xdeadbeef);
            size = VirtualQuery((char *)hlib + info.RegionSize, &info, sizeof(info));
            ok(size == sizeof(info),
                "%d: VirtualQuery error %d\n", i, GetLastError());
            if (nt_header.OptionalHeader.SectionAlignment == page_size ||
                nt_header.OptionalHeader.SectionAlignment == nt_header.OptionalHeader.FileAlignment)
            {
                ok(info.BaseAddress == (char *)hlib + ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size), "%d: got %p != expected %p\n",
                   i, info.BaseAddress, (char *)hlib + ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size));
                ok(info.AllocationBase == 0, "%d: %p != 0\n", i, info.AllocationBase);
                ok(info.AllocationProtect == 0, "%d: %x != 0\n", i, info.AllocationProtect);
                /*ok(info.RegionSize == not_practical_value, "%d: %lx != not_practical_value\n", i, info.RegionSize);*/
                ok(info.State == MEM_FREE, "%d: %x != MEM_FREE\n", i, info.State);
                ok(info.Type == 0, "%d: %x != 0\n", i, info.Type);
                ok(info.Protect == PAGE_NOACCESS, "%d: %x != PAGE_NOACCESS\n", i, info.Protect);
            }
            else
            {
                ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.Protect);
                ok(info.BaseAddress == hlib, "%d: got %p != expected %p\n", i, info.BaseAddress, hlib);
                ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
                ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
                ok(info.RegionSize == ALIGN_SIZE(file_size, page_size), "%d: got %lx != expected %x\n",
                   i, info.RegionSize, ALIGN_SIZE(file_size, page_size));
                ok(info.State == MEM_COMMIT, "%d: %x != MEM_COMMIT\n", i, info.State);
                ok(info.Protect == PAGE_READONLY, "%d: %x != PAGE_READONLY\n", i, info.Protect);
                ok(info.Type == SEC_IMAGE, "%d: %x != SEC_IMAGE\n", i, info.Type);
            }

            /* header: check the zeroing of alignment */
            if (nt_header.OptionalHeader.SectionAlignment >= page_size)
            {
                const char *start;

                start = (const char *)hlib + nt_header.OptionalHeader.SizeOfHeaders;
                size = ALIGN_SIZE((ULONG_PTR)start, page_size) - (ULONG_PTR)start;
                ok(!memcmp(start, filler, size), "%d: header alignment is not cleared\n", i);
            }

            if (nt_header.FileHeader.NumberOfSections)
            {
                SetLastError(0xdeadbeef);
                size = VirtualQuery((char *)hlib + section.VirtualAddress, &info, sizeof(info));
                ok(size == sizeof(info),
                    "%d: VirtualQuery error %d\n", i, GetLastError());
                if (nt_header.OptionalHeader.SectionAlignment < page_size)
                {
                    ok(info.BaseAddress == hlib, "%d: got %p != expected %p\n", i, info.BaseAddress, hlib);
                    ok(info.RegionSize == ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size), "%d: got %lx != expected %x\n",
                       i, info.RegionSize, ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size));
                    ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.Protect);
                }
                else
                {
                    ok(info.BaseAddress == (char *)hlib + section.VirtualAddress, "%d: got %p != expected %p\n", i, info.BaseAddress, (char *)hlib + section.VirtualAddress);
                    ok(info.RegionSize == ALIGN_SIZE(section.Misc.VirtualSize, page_size), "%d: got %lx != expected %x\n",
                       i, info.RegionSize, ALIGN_SIZE(section.Misc.VirtualSize, page_size));
                    ok(info.Protect == PAGE_READONLY, "%d: %x != PAGE_READONLY\n", i, info.Protect);
                }
                ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
                ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
                ok(info.State == MEM_COMMIT, "%d: %x != MEM_COMMIT\n", i, info.State);
                ok(info.Type == SEC_IMAGE, "%d: %x != SEC_IMAGE\n", i, info.Type);

                if (nt_header.OptionalHeader.SectionAlignment >= page_size)
                    ok(!memcmp((const char *)hlib + section.VirtualAddress + section.PointerToRawData, &nt_header, section.SizeOfRawData), "wrong section data\n");
                else
                    ok(!memcmp((const char *)hlib + section.PointerToRawData, section_data, section.SizeOfRawData), "wrong section data\n");

                /* check the zeroing of alignment */
                if (nt_header.OptionalHeader.SectionAlignment >= page_size)
                {
                    const char *start;

                    start = (const char *)hlib + section.VirtualAddress + section.PointerToRawData + section.SizeOfRawData;
                    size = ALIGN_SIZE((ULONG_PTR)start, page_size) - (ULONG_PTR)start;
                    ok(memcmp(start, filler, size), "%d: alignment should not be cleared\n", i);
                }

                SetLastError(0xdeadbeef);
                ptr = VirtualAlloc((char *)hlib + section.VirtualAddress, page_size, MEM_COMMIT, info.Protect);
                ok(!ptr, "%d: VirtualAlloc should fail\n", i);
                ok(GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_INVALID_ADDRESS,
                   "%d: expected ERROR_ACCESS_DENIED, got %d\n", i, GetLastError());
            }

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryExA(dll_name, 0, LOAD_LIBRARY_AS_DATAFILE);
            ok(hlib_as_data_file != 0, "LoadLibraryEx error %u\n", GetLastError());
            ok(hlib_as_data_file == hlib, "hlib_as_file and hlib are different\n");

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib);
            ok(ret, "FreeLibrary error %d\n", GetLastError());

            SetLastError(0xdeadbeef);
            hlib = GetModuleHandleA(dll_name);
            ok(hlib != 0, "GetModuleHandle error %u\n", GetLastError());

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib_as_data_file);
            ok(ret, "FreeLibrary error %d\n", GetLastError());

            hlib = GetModuleHandleA(dll_name);
            ok(!hlib, "GetModuleHandle should fail\n");

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryExA(dll_name, 0, LOAD_LIBRARY_AS_DATAFILE);
            ok(hlib_as_data_file != 0, "LoadLibraryEx error %u\n", GetLastError());
            ok(((ULONG_PTR)hlib_as_data_file & 3) == 1, "hlib_as_data_file got %p\n", hlib_as_data_file);

            hlib = GetModuleHandleA(dll_name);
            ok(!hlib, "GetModuleHandle should fail\n");

            SetLastError(0xdeadbeef);
            h = CreateFileA( dll_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
            ok( h != INVALID_HANDLE_VALUE, "open failed err %u\n", GetLastError() );
            CloseHandle( h );

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib_as_data_file);
            ok(ret, "FreeLibrary error %d\n", GetLastError());

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryExA(dll_name, 0, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);
            if (!((ULONG_PTR)hlib_as_data_file & 3) ||  /* winxp */
                (!hlib_as_data_file && GetLastError() == ERROR_INVALID_PARAMETER))  /* w2k3 */
            {
                win_skip( "LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE not supported\n" );
                FreeLibrary(hlib_as_data_file);
            }
            else
            {
                ok(hlib_as_data_file != 0, "LoadLibraryEx error %u\n", GetLastError());

                SetLastError(0xdeadbeef);
                h = CreateFileA( dll_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
                ok( h == INVALID_HANDLE_VALUE, "open succeeded\n" );
                ok( GetLastError() == ERROR_SHARING_VIOLATION, "wrong error %u\n", GetLastError() );
                CloseHandle( h );

                SetLastError(0xdeadbeef);
                h = CreateFileA( dll_name, GENERIC_READ | DELETE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
                ok( h != INVALID_HANDLE_VALUE, "open failed err %u\n", GetLastError() );
                CloseHandle( h );

                SetLastError(0xdeadbeef);
                ret = FreeLibrary(hlib_as_data_file);
                ok(ret, "FreeLibrary error %d\n", GetLastError());
            }

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryExA(dll_name, 0, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
            if (!((ULONG_PTR)hlib_as_data_file & 3) ||  /* winxp */
                (!hlib_as_data_file && GetLastError() == ERROR_INVALID_PARAMETER))  /* w2k3 */
            {
                win_skip( "LOAD_LIBRARY_AS_IMAGE_RESOURCE not supported\n" );
                FreeLibrary(hlib_as_data_file);
            }
            else
            {
                ok(hlib_as_data_file != 0, "LoadLibraryEx error %u\n", GetLastError());
                ok(((ULONG_PTR)hlib_as_data_file & 3) == 2, "hlib_as_data_file got %p\n",
                   hlib_as_data_file);

                hlib = GetModuleHandleA(dll_name);
                ok(!hlib, "GetModuleHandle should fail\n");

                SetLastError(0xdeadbeef);
                ret = FreeLibrary(hlib_as_data_file);
                ok(ret, "FreeLibrary error %d\n", GetLastError());
            }

            SetLastError(0xdeadbeef);
            ret = DeleteFileA(dll_name);
            ok(ret, "DeleteFile error %d\n", GetLastError());

            nt_header.OptionalHeader.AddressOfEntryPoint = 0x12345678;
            file_size = create_test_dll( &dos_header, td[i].size_of_dos_header, &nt_header, dll_name );
            if (!file_size)
            {
                ok(0, "could not create %s\n", dll_name);
                break;
            }

            query_image_section( i, dll_name, &nt_header, NULL );
        }
        else
        {
            BOOL error_match;
            int error_index;

            error_match = FALSE;
            for (error_index = 0;
                 ! error_match && error_index < sizeof(td[i].errors) / sizeof(DWORD);
                 error_index++)
            {
                error_match = td[i].errors[error_index] == GetLastError();
            }
            ok(error_match, "%d: unexpected error %d\n", i, GetLastError());
        }

        SetLastError(0xdeadbeef);
        ret = DeleteFileA(dll_name);
        ok(ret, "DeleteFile error %d\n", GetLastError());
    }

    nt_header = nt_header_template;
    nt_header.FileHeader.NumberOfSections = 1;
    nt_header.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);

    nt_header.OptionalHeader.SectionAlignment = page_size;
    nt_header.OptionalHeader.AddressOfEntryPoint = 0x1234;
    nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
    nt_header.OptionalHeader.FileAlignment = page_size;
    nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER);
    nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + page_size;

    section.SizeOfRawData = sizeof(section_data);
    section.PointerToRawData = page_size;
    section.VirtualAddress = page_size;
    section.Misc.VirtualSize = page_size;

    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

    nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

    nt_header.OptionalHeader.SizeOfCode = 0x1000;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );
    nt_header.OptionalHeader.SizeOfCode = 0;
    nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT;

    dos_header.e_magic = 0;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_NOT_MZ, "NtCreateSection error %08x\n", status );

    dos_header.e_magic = IMAGE_DOS_SIGNATURE;
    nt_header.Signature = IMAGE_OS2_SIGNATURE;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_NE_FORMAT, "NtCreateSection error %08x\n", status );

    nt_header.Signature = 0xdeadbeef;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_PROTECT, "NtCreateSection error %08x\n", status );

    nt_header.Signature = IMAGE_NT_SIGNATURE;
    nt_header.OptionalHeader.Magic = 0xdead;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_FORMAT, "NtCreateSection error %08x\n", status );

    nt_header.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
    nt_header.FileHeader.Machine = 0xdead;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_FORMAT || broken(status == STATUS_SUCCESS), /* win2k */
        "NtCreateSection error %08x\n", status );

    nt_header.FileHeader.Machine = IMAGE_FILE_MACHINE_UNKNOWN;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_FORMAT || broken(status == STATUS_SUCCESS), /* win2k */
        "NtCreateSection error %08x\n", status );

    switch (orig_machine)
    {
    case IMAGE_FILE_MACHINE_I386:  alt_machine = IMAGE_FILE_MACHINE_ARMNT; break;
    case IMAGE_FILE_MACHINE_AMD64: alt_machine = IMAGE_FILE_MACHINE_ARM64; break;
    case IMAGE_FILE_MACHINE_ARMNT: alt_machine = IMAGE_FILE_MACHINE_I386; break;
    case IMAGE_FILE_MACHINE_ARM64: alt_machine = IMAGE_FILE_MACHINE_AMD64; break;
    }
    nt_header.FileHeader.Machine = alt_machine;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_FORMAT || broken(status == STATUS_SUCCESS), /* win2k */
        "NtCreateSection error %08x\n", status );

    switch (orig_machine)
    {
    case IMAGE_FILE_MACHINE_I386:  alt_machine = IMAGE_FILE_MACHINE_AMD64; break;
    case IMAGE_FILE_MACHINE_AMD64: alt_machine = IMAGE_FILE_MACHINE_I386; break;
    case IMAGE_FILE_MACHINE_ARMNT: alt_machine = IMAGE_FILE_MACHINE_ARM64; break;
    case IMAGE_FILE_MACHINE_ARM64: alt_machine = IMAGE_FILE_MACHINE_ARMNT; break;
    }
    nt_header.FileHeader.Machine = alt_machine;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_FORMAT || broken(status == STATUS_SUCCESS), /* win2k */
                  "NtCreateSection error %08x\n", status );

    nt_header.FileHeader.Machine = orig_machine;
    nt_header.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = page_size;
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = sizeof(cor_header);
    section.SizeOfRawData = sizeof(cor_header);

    memset( &cor_header, 0, sizeof(cor_header) );
    cor_header.cb = sizeof(cor_header);
    cor_header.MajorRuntimeVersion = 2;
    cor_header.MinorRuntimeVersion = 4;
    cor_header.Flags = COMIMAGE_FLAGS_ILONLY;
    U(cor_header).EntryPointToken = 0xbeef;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

    cor_header.MinorRuntimeVersion = 5;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

    cor_header.MajorRuntimeVersion = 3;
    cor_header.MinorRuntimeVersion = 0;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

    cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITREQUIRED;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

    cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITPREFERRED;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

    cor_header.Flags = 0;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = 1;
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = 1;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

    if (nt_header.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        IMAGE_NT_HEADERS64 nt64;

        memset( &nt64, 0, sizeof(nt64) );
        nt64.Signature = IMAGE_NT_SIGNATURE;
        nt64.FileHeader.Machine = orig_machine;
        nt64.FileHeader.NumberOfSections = 1;
        nt64.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
        nt64.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL;
        nt64.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
        nt64.OptionalHeader.MajorLinkerVersion = 1;
        nt64.OptionalHeader.SizeOfCode = 0x1000;
        nt64.OptionalHeader.AddressOfEntryPoint = 0x1000;
        nt64.OptionalHeader.ImageBase = 0x10000000;
        nt64.OptionalHeader.SectionAlignment = 0x1000;
        nt64.OptionalHeader.FileAlignment = 0x1000;
        nt64.OptionalHeader.MajorOperatingSystemVersion = 4;
        nt64.OptionalHeader.MajorImageVersion = 1;
        nt64.OptionalHeader.MajorSubsystemVersion = 4;
        nt64.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt64) + sizeof(IMAGE_SECTION_HEADER);
        nt64.OptionalHeader.SizeOfImage = nt64.OptionalHeader.SizeOfHeaders + 0x1000;
        nt64.OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
        nt64.OptionalHeader.SizeOfStackReserve = 0x321000;
        nt64.OptionalHeader.SizeOfStackCommit = 0x123000;
        section.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE;

        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_INVALID_IMAGE_FORMAT : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        switch (orig_machine)
        {
        case IMAGE_FILE_MACHINE_I386: nt64.FileHeader.Machine = IMAGE_FILE_MACHINE_ARM64; break;
        case IMAGE_FILE_MACHINE_ARMNT: nt64.FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64; break;
        }
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_INVALID_IMAGE_FORMAT : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        nt64.FileHeader.Machine = alt_machine;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        nt64.OptionalHeader.SizeOfCode = 0;
        nt64.OptionalHeader.AddressOfEntryPoint = 0x1000;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        nt64.OptionalHeader.SizeOfCode = 0;
        nt64.OptionalHeader.AddressOfEntryPoint = 0;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        nt64.OptionalHeader.SizeOfCode = 0x1000;
        nt64.OptionalHeader.AddressOfEntryPoint = 0;
        nt64.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        nt64.OptionalHeader.SizeOfCode = 0;
        nt64.OptionalHeader.AddressOfEntryPoint = 0;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        nt64.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        nt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = page_size;
        nt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = sizeof(cor_header);
        cor_header.MajorRuntimeVersion = 2;
        cor_header.MinorRuntimeVersion = 4;
        cor_header.Flags = COMIMAGE_FLAGS_ILONLY;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        nt64.OptionalHeader.SizeOfCode = 0x1000;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        cor_header.MinorRuntimeVersion = 5;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITREQUIRED;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITPREFERRED;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        cor_header.Flags = 0;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );

        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = 1;
        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = 1;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08x\n", status );
    }
    else
    {
        IMAGE_NT_HEADERS32 nt32;

        memset( &nt32, 0, sizeof(nt32) );
        nt32.Signature = IMAGE_NT_SIGNATURE;
        nt32.FileHeader.Machine = orig_machine;
        nt32.FileHeader.NumberOfSections = 1;
        nt32.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
        nt32.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL;
        nt32.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
        nt32.OptionalHeader.MajorLinkerVersion = 1;
        nt32.OptionalHeader.SizeOfCode = 0x1000;
        nt32.OptionalHeader.AddressOfEntryPoint = 0x1000;
        nt32.OptionalHeader.ImageBase = 0x10000000;
        nt32.OptionalHeader.SectionAlignment = 0x1000;
        nt32.OptionalHeader.FileAlignment = 0x1000;
        nt32.OptionalHeader.MajorOperatingSystemVersion = 4;
        nt32.OptionalHeader.MajorImageVersion = 1;
        nt32.OptionalHeader.MajorSubsystemVersion = 4;
        nt32.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt32) + sizeof(IMAGE_SECTION_HEADER);
        nt32.OptionalHeader.SizeOfImage = nt32.OptionalHeader.SizeOfHeaders + 0x1000;
        nt32.OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
        nt32.OptionalHeader.SizeOfStackReserve = 0x321000;
        nt32.OptionalHeader.SizeOfStackCommit = 0x123000;
        section.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE;

        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_INVALID_IMAGE_FORMAT, "NtCreateSection error %08x\n", status );

        switch (orig_machine)
        {
        case IMAGE_FILE_MACHINE_AMD64: nt32.FileHeader.Machine = IMAGE_FILE_MACHINE_ARMNT; break;
        case IMAGE_FILE_MACHINE_ARM64: nt32.FileHeader.Machine = IMAGE_FILE_MACHINE_I386; break;
        }
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_INVALID_IMAGE_FORMAT || broken(!status) /* win8 */,
            "NtCreateSection error %08x\n", status );

        nt32.FileHeader.Machine = alt_machine;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        nt32.OptionalHeader.SizeOfCode = 0;
        nt32.OptionalHeader.AddressOfEntryPoint = 0x1000;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        nt32.OptionalHeader.SizeOfCode = 0;
        nt32.OptionalHeader.AddressOfEntryPoint = 0;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        nt32.OptionalHeader.SizeOfCode = 0x1000;
        nt32.OptionalHeader.AddressOfEntryPoint = 0;
        nt32.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        nt32.OptionalHeader.SizeOfCode = 0;
        nt32.OptionalHeader.AddressOfEntryPoint = 0;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        nt32.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        nt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = page_size;
        nt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = sizeof(cor_header);
        cor_header.MajorRuntimeVersion = 2;
        cor_header.MinorRuntimeVersion = 4;
        cor_header.Flags = COMIMAGE_FLAGS_ILONLY;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        nt32.OptionalHeader.SizeOfCode = 0x1000;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        cor_header.MinorRuntimeVersion = 5;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITREQUIRED;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITPREFERRED;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        cor_header.Flags = 0;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );

        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = 1;
        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = 1;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08x\n", status );
    }

    section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
}

static void test_filenames(void)
{
    IMAGE_NT_HEADERS nt_header = nt_header_template;
    char dll_name[MAX_PATH], long_path[MAX_PATH], short_path[MAX_PATH], buffer[MAX_PATH];
    HMODULE mod, mod2;
    BOOL ret;

    nt_header.FileHeader.NumberOfSections = 1;
    nt_header.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);

    nt_header.OptionalHeader.SectionAlignment = page_size;
    nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
    nt_header.OptionalHeader.FileAlignment = page_size;
    nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER);
    nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + page_size;

    create_test_dll( &dos_header, sizeof(dos_header), &nt_header, dll_name );
    strcpy( long_path, dll_name );
    strcpy( strrchr( long_path, '\\' ), "\\this-is-a-long-name.dll" );
    ret = MoveFileA( dll_name, long_path );
    ok( ret, "MoveFileA failed err %u\n", GetLastError() );
    GetShortPathNameA( long_path, short_path, MAX_PATH );

    mod = LoadLibraryA( short_path );
    ok( mod != NULL, "loading failed err %u\n", GetLastError() );
    GetModuleFileNameA( mod, buffer, MAX_PATH );
    ok( !lstrcmpiA( buffer, short_path ), "got wrong path %s / %s\n", buffer, short_path );
    mod2 = GetModuleHandleA( short_path );
    ok( mod == mod2, "wrong module %p for %s\n", mod2, short_path );
    mod2 = GetModuleHandleA( long_path );
    ok( mod == mod2, "wrong module %p for %s\n", mod2, long_path );
    mod2 = LoadLibraryA( long_path );
    ok( mod2 != NULL, "loading failed err %u\n", GetLastError() );
    ok( mod == mod2, "library loaded twice\n" );
    GetModuleFileNameA( mod2, buffer, MAX_PATH );
    ok( !lstrcmpiA( buffer, short_path ), "got wrong path %s / %s\n", buffer, short_path );
    FreeLibrary( mod2 );
    FreeLibrary( mod );

    mod = LoadLibraryA( long_path );
    ok( mod != NULL, "loading failed err %u\n", GetLastError() );
    GetModuleFileNameA( mod, buffer, MAX_PATH );
    ok( !lstrcmpiA( buffer, long_path ), "got wrong path %s / %s\n", buffer, long_path );
    mod2 = GetModuleHandleA( short_path );
    ok( mod == mod2, "wrong module %p for %s\n", mod2, short_path );
    mod2 = GetModuleHandleA( long_path );
    ok( mod == mod2, "wrong module %p for %s\n", mod2, long_path );
    mod2 = LoadLibraryA( short_path );
    ok( mod2 != NULL, "loading failed err %u\n", GetLastError() );
    ok( mod == mod2, "library loaded twice\n" );
    GetModuleFileNameA( mod2, buffer, MAX_PATH );
    ok( !lstrcmpiA( buffer, long_path ), "got wrong path %s / %s\n", buffer, long_path );
    FreeLibrary( mod2 );
    FreeLibrary( mod );

    strcpy( dll_name, long_path );
    strcpy( strrchr( dll_name, '\\' ), "\\this-is-another-name.dll" );
    ret = CreateHardLinkA( dll_name, long_path, NULL );
    ok( ret, "CreateHardLinkA failed err %u\n", GetLastError() );
    if (ret)
    {
        mod = LoadLibraryA( dll_name );
        ok( mod != NULL, "loading failed err %u\n", GetLastError() );
        GetModuleFileNameA( mod, buffer, MAX_PATH );
        ok( !lstrcmpiA( buffer, dll_name ), "got wrong path %s / %s\n", buffer, dll_name );
        mod2 = GetModuleHandleA( long_path );
        ok( mod == mod2, "wrong module %p for %s\n", mod2, long_path );
        mod2 = LoadLibraryA( long_path );
        ok( mod2 != NULL, "loading failed err %u\n", GetLastError() );
        ok( mod == mod2, "library loaded twice\n" );
        GetModuleFileNameA( mod2, buffer, MAX_PATH );
        ok( !lstrcmpiA( buffer, dll_name ), "got wrong path %s / %s\n", buffer, short_path );
        FreeLibrary( mod2 );
        FreeLibrary( mod );
        DeleteFileA( dll_name );
    }
    DeleteFileA( long_path );
}

static void test_FakeDLL(void)
{
#if defined(__i386__) || defined(__x86_64__)
    NTSTATUS (WINAPI *pNtSetEvent)(HANDLE, ULONG *) = NULL;
    IMAGE_EXPORT_DIRECTORY *dir;
    HMODULE module = GetModuleHandleA("ntdll.dll");
    HANDLE file, map, event;
    WCHAR path[MAX_PATH];
    DWORD *names, *funcs;
    WORD *ordinals;
    ULONG size;
    void *ptr;
    int i;

    GetModuleFileNameW(module, path, MAX_PATH);

    file = CreateFileW(path, GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to open %s (error %u)\n", wine_dbgstr_w(path), GetLastError());

    map = CreateFileMappingW(file, NULL, PAGE_EXECUTE_READ | SEC_IMAGE, 0, 0, NULL);
    ok(map != NULL, "CreateFileMapping failed with error %u\n", GetLastError());
    ptr = MapViewOfFile(map, FILE_MAP_READ | FILE_MAP_EXECUTE, 0, 0, 0);
    ok(ptr != NULL, "MapViewOfFile failed with error %u\n", GetLastError());

    dir = RtlImageDirectoryEntryToData(ptr, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size);
    ok(dir != NULL, "RtlImageDirectoryEntryToData failed\n");

    names    = RVAToAddr(dir->AddressOfNames, ptr);
    ordinals = RVAToAddr(dir->AddressOfNameOrdinals, ptr);
    funcs    = RVAToAddr(dir->AddressOfFunctions, ptr);
    ok(dir->NumberOfNames > 0, "Could not find any exported functions\n");

    for (i = 0; i < dir->NumberOfNames; i++)
    {
        DWORD map_rva, dll_rva, map_offset, dll_offset;
        char *func_name = RVAToAddr(names[i], ptr);
        BYTE *dll_func, *map_func;

        /* check only Nt functions for now */
        if (strncmp(func_name, "Zw", 2) && strncmp(func_name, "Nt", 2))
            continue;

        dll_func = (BYTE *)GetProcAddress(module, func_name);
        ok(dll_func != NULL, "%s: GetProcAddress returned NULL\n", func_name);
#if defined(__i386__)
        if (dll_func[0] == 0x90 && dll_func[1] == 0x90 &&
            dll_func[2] == 0x90 && dll_func[3] == 0x90)
#elif defined(__x86_64__)
        if (dll_func[0] == 0x48 && dll_func[1] == 0x83 &&
            dll_func[2] == 0xec && dll_func[3] == 0x08)
#endif
        {
            todo_wine ok(0, "%s: Export is a stub-function, skipping\n", func_name);
            continue;
        }

        /* check position in memory */
        dll_rva = (DWORD_PTR)dll_func - (DWORD_PTR)module;
        map_rva = funcs[ordinals[i]];
        ok(map_rva == dll_rva, "%s: Rva of mapped function (0x%x) does not match dll (0x%x)\n",
           func_name, dll_rva, map_rva);

        /* check position in file */
        map_offset = (DWORD_PTR)RtlImageRvaToVa(RtlImageNtHeader(ptr),    ptr,    map_rva, NULL) - (DWORD_PTR)ptr;
        dll_offset = (DWORD_PTR)RtlImageRvaToVa(RtlImageNtHeader(module), module, dll_rva, NULL) - (DWORD_PTR)module;
        ok(map_offset == dll_offset, "%s: File offset of mapped function (0x%x) does not match dll (0x%x)\n",
           func_name, map_offset, dll_offset);

        /* check function content */
        map_func = RVAToAddr(map_rva, ptr);
        ok(!memcmp(map_func, dll_func, 0x20), "%s: Function content does not match!\n", func_name);

        if (!strcmp(func_name, "NtSetEvent"))
            pNtSetEvent = (void *)map_func;
    }

    ok(pNtSetEvent != NULL, "Could not find NtSetEvent export\n");
    if (pNtSetEvent)
    {
        event = CreateEventA(NULL, TRUE, FALSE, NULL);
        ok(event != NULL, "CreateEvent failed with error %u\n", GetLastError());
        pNtSetEvent(event, 0);
        ok(WaitForSingleObject(event, 0) == WAIT_OBJECT_0, "Event was not signaled\n");
        pNtSetEvent(event, 0);
        ok(WaitForSingleObject(event, 0) == WAIT_OBJECT_0, "Event was not signaled\n");
        CloseHandle(event);
    }

    UnmapViewOfFile(ptr);
    CloseHandle(map);
    CloseHandle(file);
#endif
}

/* Verify linking style of import descriptors */
static void test_ImportDescriptors(void)
{
    HMODULE kernel32_module = NULL;
    PIMAGE_DOS_HEADER d_header;
    PIMAGE_NT_HEADERS nt_headers;
    DWORD import_dir_size;
    DWORD_PTR dir_offset;
    PIMAGE_IMPORT_DESCRIPTOR import_chunk;

    /* Load kernel32 module */
    kernel32_module = GetModuleHandleA("kernel32.dll");
    assert( kernel32_module != NULL );

    /* Get PE header info from module image */
    d_header = (PIMAGE_DOS_HEADER) kernel32_module;
    nt_headers = (PIMAGE_NT_HEADERS) (((char*) d_header) +
            d_header->e_lfanew);

    /* Get size of import entry directory */
    import_dir_size = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
    if (!import_dir_size)
    {
        skip("Unable to continue testing due to missing import directory.\n");
        return;
    }

    /* Get address of first import chunk */
    dir_offset = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    import_chunk = RVAToAddr(dir_offset, kernel32_module);
    ok(import_chunk != 0, "Invalid import_chunk: %p\n", import_chunk);
    if (!import_chunk) return;

    /* Iterate through import descriptors and verify set name,
     * OriginalFirstThunk, and FirstThunk.  Core Windows DLLs, such as
     * kernel32.dll, don't use Borland-style linking, where the table of
     * imported names is stored directly in FirstThunk and overwritten
     * by the relocation, instead of being stored in OriginalFirstThunk.
     * */
    for (; import_chunk->FirstThunk; import_chunk++)
    {
        LPCSTR module_name = RVAToAddr(import_chunk->Name, kernel32_module);
        PIMAGE_THUNK_DATA name_table = RVAToAddr(
                U(*import_chunk).OriginalFirstThunk, kernel32_module);
        PIMAGE_THUNK_DATA iat = RVAToAddr(
                import_chunk->FirstThunk, kernel32_module);
        ok(module_name != NULL, "Imported module name should not be NULL\n");
        ok(name_table != NULL,
                "Name table for imported module %s should not be NULL\n",
                module_name);
        ok(iat != NULL, "IAT for imported module %s should not be NULL\n",
                module_name);
    }
}

static void test_image_mapping(const char *dll_name, DWORD scn_page_access, BOOL is_dll)
{
    HANDLE hfile, hmap;
    NTSTATUS status;
    LARGE_INTEGER offset;
    SIZE_T size;
    void *addr1, *addr2;
    MEMORY_BASIC_INFORMATION info;

    if (!pNtMapViewOfSection) return;

    SetLastError(0xdeadbeef);
    hfile = CreateFileA(dll_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hmap = CreateFileMappingW(hfile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, 0);
    ok(hmap != 0, "CreateFileMapping error %d\n", GetLastError());

    offset.u.LowPart  = 0;
    offset.u.HighPart = 0;

    addr1 = NULL;
    size = 0;
    status = pNtMapViewOfSection(hmap, GetCurrentProcess(), &addr1, 0, 0, &offset,
                                 &size, 1 /* ViewShare */, 0, PAGE_READONLY);
    ok(status == STATUS_SUCCESS, "NtMapViewOfSection error %x\n", status);
    ok(addr1 != 0, "mapped address should be valid\n");

    SetLastError(0xdeadbeef);
    size = VirtualQuery((char *)addr1 + section.VirtualAddress, &info, sizeof(info));
    ok(size == sizeof(info), "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == (char *)addr1 + section.VirtualAddress, "got %p != expected %p\n", info.BaseAddress, (char *)addr1 + section.VirtualAddress);
    ok(info.RegionSize == page_size, "got %#lx != expected %#x\n", info.RegionSize, page_size);
    ok(info.Protect == scn_page_access, "got %#x != expected %#x\n", info.Protect, scn_page_access);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%#x != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#x != MEM_COMMIT\n", info.State);
    ok(info.Type == SEC_IMAGE, "%#x != SEC_IMAGE\n", info.Type);

    addr2 = NULL;
    size = 0;
    status = pNtMapViewOfSection(hmap, GetCurrentProcess(), &addr2, 0, 0, &offset,
                                 &size, 1 /* ViewShare */, 0, PAGE_READONLY);
    ok(status == STATUS_IMAGE_NOT_AT_BASE, "expected STATUS_IMAGE_NOT_AT_BASE, got %x\n", status);
    ok(addr2 != 0, "mapped address should be valid\n");
    ok(addr2 != addr1, "mapped addresses should be different\n");

    SetLastError(0xdeadbeef);
    size = VirtualQuery((char *)addr2 + section.VirtualAddress, &info, sizeof(info));
    ok(size == sizeof(info), "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == (char *)addr2 + section.VirtualAddress, "got %p != expected %p\n", info.BaseAddress, (char *)addr2 + section.VirtualAddress);
    ok(info.RegionSize == page_size, "got %#lx != expected %#x\n", info.RegionSize, page_size);
    ok(info.Protect == scn_page_access, "got %#x != expected %#x\n", info.Protect, scn_page_access);
    ok(info.AllocationBase == addr2, "%p != %p\n", info.AllocationBase, addr2);
    ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%#x != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#x != MEM_COMMIT\n", info.State);
    ok(info.Type == SEC_IMAGE, "%#x != SEC_IMAGE\n", info.Type);

    status = pNtUnmapViewOfSection(GetCurrentProcess(), addr2);
    ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection error %x\n", status);

    addr2 = MapViewOfFile(hmap, 0, 0, 0, 0);
    ok(addr2 != 0, "mapped address should be valid\n");
    ok(addr2 != addr1, "mapped addresses should be different\n");

    SetLastError(0xdeadbeef);
    size = VirtualQuery((char *)addr2 + section.VirtualAddress, &info, sizeof(info));
    ok(size == sizeof(info), "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == (char *)addr2 + section.VirtualAddress, "got %p != expected %p\n", info.BaseAddress, (char *)addr2 + section.VirtualAddress);
    ok(info.RegionSize == page_size, "got %#lx != expected %#x\n", info.RegionSize, page_size);
    ok(info.Protect == scn_page_access, "got %#x != expected %#x\n", info.Protect, scn_page_access);
    ok(info.AllocationBase == addr2, "%p != %p\n", info.AllocationBase, addr2);
    ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%#x != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#x != MEM_COMMIT\n", info.State);
    ok(info.Type == SEC_IMAGE, "%#x != SEC_IMAGE\n", info.Type);

    UnmapViewOfFile(addr2);

    SetLastError(0xdeadbeef);
    addr2 = LoadLibraryA(dll_name);
    if (is_dll)
    {
        ok(!addr2, "LoadLibrary should fail, is_dll %d\n", is_dll);
        ok(GetLastError() == ERROR_INVALID_ADDRESS, "expected ERROR_INVALID_ADDRESS, got %d\n", GetLastError());
    }
    else
    {
        BOOL ret;
        ok(addr2 != 0, "LoadLibrary error %d, is_dll %d\n", GetLastError(), is_dll);
        ok(addr2 != addr1, "mapped addresses should be different\n");

        SetLastError(0xdeadbeef);
        ret = FreeLibrary(addr2);
        ok(ret, "FreeLibrary error %d\n", GetLastError());
    }

    status = pNtUnmapViewOfSection(GetCurrentProcess(), addr1);
    ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection error %x\n", status);

    CloseHandle(hmap);
    CloseHandle(hfile);
}

static BOOL is_mem_writable(DWORD prot)
{
    switch (prot & 0xff)
    {
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return TRUE;

        default:
            return FALSE;
    }
}

static void test_VirtualProtect(void *base, void *section)
{
    static const struct test_data
    {
        DWORD prot_set, prot_get;
    } td[] =
    {
        { 0, 0 }, /* 0x00 */
        { PAGE_NOACCESS, PAGE_NOACCESS }, /* 0x01 */
        { PAGE_READONLY, PAGE_READONLY }, /* 0x02 */
        { PAGE_READONLY | PAGE_NOACCESS, 0 }, /* 0x03 */
        { PAGE_READWRITE, PAGE_WRITECOPY }, /* 0x04 */
        { PAGE_READWRITE | PAGE_NOACCESS, 0 }, /* 0x05 */
        { PAGE_READWRITE | PAGE_READONLY, 0 }, /* 0x06 */
        { PAGE_READWRITE | PAGE_READONLY | PAGE_NOACCESS, 0 }, /* 0x07 */
        { PAGE_WRITECOPY, PAGE_WRITECOPY }, /* 0x08 */
        { PAGE_WRITECOPY | PAGE_NOACCESS, 0 }, /* 0x09 */
        { PAGE_WRITECOPY | PAGE_READONLY, 0 }, /* 0x0a */
        { PAGE_WRITECOPY | PAGE_NOACCESS | PAGE_READONLY, 0 }, /* 0x0b */
        { PAGE_WRITECOPY | PAGE_READWRITE, 0 }, /* 0x0c */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_NOACCESS, 0 }, /* 0x0d */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_READONLY, 0 }, /* 0x0e */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_READONLY | PAGE_NOACCESS, 0 }, /* 0x0f */

        { PAGE_EXECUTE, PAGE_EXECUTE }, /* 0x10 */
        { PAGE_EXECUTE_READ, PAGE_EXECUTE_READ }, /* 0x20 */
        { PAGE_EXECUTE_READ | PAGE_EXECUTE, 0 }, /* 0x30 */
        { PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_WRITECOPY }, /* 0x40 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE, 0 }, /* 0x50 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ, 0 }, /* 0x60 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE, 0 }, /* 0x70 */
        { PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_WRITECOPY }, /* 0x80 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE, 0 }, /* 0x90 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READ, 0 }, /* 0xa0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE, 0 }, /* 0xb0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE, 0 }, /* 0xc0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE, 0 }, /* 0xd0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ, 0 }, /* 0xe0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE, 0 } /* 0xf0 */
    };
    DWORD ret, orig_prot, old_prot, rw_prot, exec_prot, i, j;
    MEMORY_BASIC_INFORMATION info;

    SetLastError(0xdeadbeef);
    ret = VirtualProtect(section, page_size, PAGE_NOACCESS, &old_prot);
    ok(ret, "VirtualProtect error %d\n", GetLastError());

    orig_prot = old_prot;

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        SetLastError(0xdeadbeef);
        ret = VirtualQuery(section, &info, sizeof(info));
        ok(ret, "VirtualQuery failed %d\n", GetLastError());
        ok(info.BaseAddress == section, "%d: got %p != expected %p\n", i, info.BaseAddress, section);
        ok(info.RegionSize == page_size, "%d: got %#lx != expected %#x\n", i, info.RegionSize, page_size);
        ok(info.Protect == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, info.Protect);
        ok(info.AllocationBase == base, "%d: %p != %p\n", i, info.AllocationBase, base);
        ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %#x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
        ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
        ok(info.Type == SEC_IMAGE, "%d: %#x != SEC_IMAGE\n", i, info.Type);

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(section, page_size, td[i].prot_set, &old_prot);
        if (td[i].prot_get)
        {
            ok(ret, "%d: VirtualProtect error %d, requested prot %#x\n", i, GetLastError(), td[i].prot_set);
            ok(old_prot == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, old_prot);

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(section, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %d\n", GetLastError());
            ok(info.BaseAddress == section, "%d: got %p != expected %p\n", i, info.BaseAddress, section);
            ok(info.RegionSize == page_size, "%d: got %#lx != expected %#x\n", i, info.RegionSize, page_size);
            ok(info.Protect == td[i].prot_get, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].prot_get);
            ok(info.AllocationBase == base, "%d: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %#x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
            ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
            ok(info.Type == SEC_IMAGE, "%d: %#x != SEC_IMAGE\n", i, info.Type);
        }
        else
        {
            ok(!ret, "%d: VirtualProtect should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%d: expected ERROR_INVALID_PARAMETER, got %d\n", i, GetLastError());
        }

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(section, page_size, PAGE_NOACCESS, &old_prot);
        ok(ret, "%d: VirtualProtect error %d\n", i, GetLastError());
        if (td[i].prot_get)
            ok(old_prot == td[i].prot_get, "%d: got %#x != expected %#x\n", i, old_prot, td[i].prot_get);
        else
            ok(old_prot == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, old_prot);
    }

    exec_prot = 0;

    for (i = 0; i <= 4; i++)
    {
        rw_prot = 0;

        for (j = 0; j <= 4; j++)
        {
            DWORD prot = exec_prot | rw_prot;

            SetLastError(0xdeadbeef);
            ret = VirtualProtect(section, page_size, prot, &old_prot);
            if ((rw_prot && exec_prot) || (!rw_prot && !exec_prot))
            {
                ok(!ret, "VirtualProtect(%02x) should fail\n", prot);
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
            }
            else
                ok(ret, "VirtualProtect(%02x) error %d\n", prot, GetLastError());

            rw_prot = 1 << j;
        }

        exec_prot = 1 << (i + 4);
    }

    SetLastError(0xdeadbeef);
    ret = VirtualProtect(section, page_size, orig_prot, &old_prot);
    ok(ret, "VirtualProtect error %d\n", GetLastError());
}

static void test_section_access(void)
{
    static const struct test_data
    {
        DWORD scn_file_access, scn_page_access, scn_page_access_after_write;
    } td[] =
    {
        { 0, PAGE_NOACCESS, 0 },
        { IMAGE_SCN_MEM_READ, PAGE_READONLY, 0 },
        { IMAGE_SCN_MEM_WRITE, PAGE_WRITECOPY, PAGE_READWRITE },
        { IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE, 0 },
        { IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE, PAGE_WRITECOPY, PAGE_READWRITE },
        { IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE_READ },
        { IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_READWRITE },
        { IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_READWRITE },

        { IMAGE_SCN_CNT_INITIALIZED_DATA, PAGE_NOACCESS, 0 },
        { IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ, PAGE_READONLY, 0 },
        { IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_WRITE, PAGE_WRITECOPY, PAGE_READWRITE },
        { IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE, 0 },
        { IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE, PAGE_WRITECOPY, PAGE_READWRITE },
        { IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE_READ, 0 },
        { IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_READWRITE },
        { IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_READWRITE },

        { IMAGE_SCN_CNT_UNINITIALIZED_DATA, PAGE_NOACCESS, 0 },
        { IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ, PAGE_READONLY, 0 },
        { IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_WRITE, PAGE_WRITECOPY, PAGE_READWRITE },
        { IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE, 0 },
        { IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE, PAGE_WRITECOPY, PAGE_READWRITE },
        { IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE_READ, 0 },
        { IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_READWRITE },
        { IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE, PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_READWRITE }
    };
    char buf[256];
    int i;
    DWORD dummy, file_align;
    HANDLE hfile;
    HMODULE hlib;
    char temp_path[MAX_PATH];
    char dll_name[MAX_PATH];
    SIZE_T size;
    MEMORY_BASIC_INFORMATION info;
    STARTUPINFOA sti;
    PROCESS_INFORMATION pi;
    DWORD ret;

    /* prevent displaying of the "Unable to load this DLL" message box */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    GetTempPathA(MAX_PATH, temp_path);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        IMAGE_NT_HEADERS nt_header;

        GetTempFileNameA(temp_path, "ldr", 0, dll_name);

        /*trace("creating %s\n", dll_name);*/
        hfile = CreateFileA(dll_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
        if (hfile == INVALID_HANDLE_VALUE)
        {
            ok(0, "could not create %s\n", dll_name);
            return;
        }

        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &dos_header, sizeof(dos_header), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());

        nt_header = nt_header_template;
        nt_header.FileHeader.NumberOfSections = 1;
        nt_header.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
        nt_header.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL | IMAGE_FILE_RELOCS_STRIPPED;

        nt_header.OptionalHeader.SectionAlignment = page_size;
        nt_header.OptionalHeader.FileAlignment = 0x200;
        nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + page_size;
        nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER);
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &nt_header.OptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());

        section.SizeOfRawData = sizeof(section_data);
        section.PointerToRawData = nt_header.OptionalHeader.FileAlignment;
        section.VirtualAddress = nt_header.OptionalHeader.SectionAlignment;
        section.Misc.VirtualSize = section.SizeOfRawData;
        section.Characteristics = td[i].scn_file_access;
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &section, sizeof(section), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());

        file_align = nt_header.OptionalHeader.FileAlignment - nt_header.OptionalHeader.SizeOfHeaders;
        assert(file_align < sizeof(filler));
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, filler, file_align, &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());

        /* section data */
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, section_data, sizeof(section_data), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());

        CloseHandle(hfile);

        SetLastError(0xdeadbeef);
        hlib = LoadLibraryA(dll_name);
        ok(hlib != 0, "LoadLibrary error %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        size = VirtualQuery((char *)hlib + section.VirtualAddress, &info, sizeof(info));
        ok(size == sizeof(info),
            "%d: VirtualQuery error %d\n", i, GetLastError());
        ok(info.BaseAddress == (char *)hlib + section.VirtualAddress, "%d: got %p != expected %p\n", i, info.BaseAddress, (char *)hlib + section.VirtualAddress);
        ok(info.RegionSize == page_size, "%d: got %#lx != expected %#x\n", i, info.RegionSize, page_size);
        ok(info.Protect == td[i].scn_page_access, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].scn_page_access);
        ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
        ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %#x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
        ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
        ok(info.Type == SEC_IMAGE, "%d: %#x != SEC_IMAGE\n", i, info.Type);
        if (info.Protect != PAGE_NOACCESS)
            ok(!memcmp((const char *)info.BaseAddress, section_data, section.SizeOfRawData), "wrong section data\n");

        test_VirtualProtect(hlib, (char *)hlib + section.VirtualAddress);

        /* Windows changes the WRITECOPY to WRITE protection on an image section write (for a changed page only) */
        if (is_mem_writable(info.Protect))
        {
            char *p = info.BaseAddress;
            *p = 0xfe;
            SetLastError(0xdeadbeef);
            size = VirtualQuery((char *)hlib + section.VirtualAddress, &info, sizeof(info));
            ok(size == sizeof(info), "%d: VirtualQuery error %d\n", i, GetLastError());
            /* FIXME: remove the condition below once Wine is fixed */
            todo_wine_if (info.Protect == PAGE_WRITECOPY || info.Protect == PAGE_EXECUTE_WRITECOPY)
                ok(info.Protect == td[i].scn_page_access_after_write, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].scn_page_access_after_write);
        }

        SetLastError(0xdeadbeef);
        ret = FreeLibrary(hlib);
        ok(ret, "FreeLibrary error %d\n", GetLastError());

        test_image_mapping(dll_name, td[i].scn_page_access, TRUE);

        /* reset IMAGE_FILE_DLL otherwise CreateProcess fails */
        nt_header.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_RELOCS_STRIPPED;
        SetLastError(0xdeadbeef);
        hfile = CreateFileA(dll_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        /* LoadLibrary called on an already memory-mapped file in
         * test_image_mapping() above leads to a file handle leak
         * under nt4, and inability to overwrite and delete the file
         * due to sharing violation error. Ignore it and skip the test,
         * but leave a not deletable temporary file.
         */
        ok(hfile != INVALID_HANDLE_VALUE || broken(hfile == INVALID_HANDLE_VALUE) /* nt4 */,
            "CreateFile error %d\n", GetLastError());
        if (hfile == INVALID_HANDLE_VALUE) goto nt4_is_broken;
        SetFilePointer(hfile, sizeof(dos_header), NULL, FILE_BEGIN);
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());
        CloseHandle(hfile);

        memset(&sti, 0, sizeof(sti));
        sti.cb = sizeof(sti);
        SetLastError(0xdeadbeef);
        ret = CreateProcessA(dll_name, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &sti, &pi);
        ok(ret, "CreateProcess() error %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        size = VirtualQueryEx(pi.hProcess, (char *)hlib + section.VirtualAddress, &info, sizeof(info));
        ok(size == sizeof(info),
            "%d: VirtualQuery error %d\n", i, GetLastError());
        ok(info.BaseAddress == (char *)hlib + section.VirtualAddress, "%d: got %p != expected %p\n", i, info.BaseAddress, (char *)hlib + section.VirtualAddress);
        ok(info.RegionSize == page_size, "%d: got %#lx != expected %#x\n", i, info.RegionSize, page_size);
        ok(info.Protect == td[i].scn_page_access, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].scn_page_access);
        ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
        ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %#x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
        ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
        ok(info.Type == SEC_IMAGE, "%d: %#x != SEC_IMAGE\n", i, info.Type);
        if (info.Protect != PAGE_NOACCESS)
        {
            SetLastError(0xdeadbeef);
            ret = ReadProcessMemory(pi.hProcess, info.BaseAddress, buf, section.SizeOfRawData, NULL);
            ok(ret, "ReadProcessMemory() error %d\n", GetLastError());
            ok(!memcmp(buf, section_data, section.SizeOfRawData), "wrong section data\n");
        }

        SetLastError(0xdeadbeef);
        ret = TerminateProcess(pi.hProcess, 0);
        ok(ret, "TerminateProcess() error %d\n", GetLastError());
        ret = WaitForSingleObject(pi.hProcess, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed: %x\n", ret);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        test_image_mapping(dll_name, td[i].scn_page_access, FALSE);

nt4_is_broken:
        SetLastError(0xdeadbeef);
        ret = DeleteFileA(dll_name);
        ok(ret || broken(!ret) /* nt4 */, "DeleteFile error %d\n", GetLastError());
    }
}

static void test_import_resolution(void)
{
    char temp_path[MAX_PATH];
    char dll_name[MAX_PATH];
    DWORD dummy;
    void *expect;
    char *str;
    HANDLE hfile;
    HMODULE mod, mod2;
    struct imports
    {
        IMAGE_IMPORT_DESCRIPTOR descr[2];
        IMAGE_THUNK_DATA original_thunks[2];
        IMAGE_THUNK_DATA thunks[2];
        char module[16];
        struct { WORD hint; char name[32]; } function;
        IMAGE_TLS_DIRECTORY tls;
        char tls_data[16];
        SHORT tls_index;
    } data, *ptr;
    IMAGE_NT_HEADERS nt;
    IMAGE_SECTION_HEADER section;
    int test;

    for (test = 0; test < 3; test++)
    {
#define DATA_RVA(ptr) (page_size + ((char *)(ptr) - (char *)&data))
        nt = nt_header_template;
        nt.FileHeader.NumberOfSections = 1;
        nt.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
        nt.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_RELOCS_STRIPPED;
        if (test != 2) nt.FileHeader.Characteristics |= IMAGE_FILE_DLL;
        nt.OptionalHeader.SectionAlignment = page_size;
        nt.OptionalHeader.FileAlignment = 0x200;
        nt.OptionalHeader.ImageBase = 0x12340000;
        nt.OptionalHeader.SizeOfImage = 2 * page_size;
        nt.OptionalHeader.SizeOfHeaders = nt.OptionalHeader.FileAlignment;
        nt.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        memset( nt.OptionalHeader.DataDirectory, 0, sizeof(nt.OptionalHeader.DataDirectory) );
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = sizeof(data.descr);
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = DATA_RVA(data.descr);
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = sizeof(data.tls);
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = DATA_RVA(&data.tls);

        memset( &data, 0, sizeof(data) );
        U(data.descr[0]).OriginalFirstThunk = DATA_RVA( data.original_thunks );
        data.descr[0].FirstThunk = DATA_RVA( data.thunks );
        data.descr[0].Name = DATA_RVA( data.module );
        strcpy( data.module, "kernel32.dll" );
        strcpy( data.function.name, "CreateEventA" );
        data.original_thunks[0].u1.AddressOfData = DATA_RVA( &data.function );
        data.thunks[0].u1.AddressOfData = 0xdeadbeef;

        data.tls.StartAddressOfRawData = nt.OptionalHeader.ImageBase + DATA_RVA( data.tls_data );
        data.tls.EndAddressOfRawData = data.tls.StartAddressOfRawData + sizeof(data.tls_data);
        data.tls.AddressOfIndex = nt.OptionalHeader.ImageBase + DATA_RVA( &data.tls_index );
        strcpy( data.tls_data, "hello world" );
        data.tls_index = 9999;

        GetTempPathA(MAX_PATH, temp_path);
        GetTempFileNameA(temp_path, "ldr", 0, dll_name);

        hfile = CreateFileA(dll_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
        ok( hfile != INVALID_HANDLE_VALUE, "creation failed\n" );

        memset( &section, 0, sizeof(section) );
        memcpy( section.Name, ".text", sizeof(".text") );
        section.PointerToRawData = nt.OptionalHeader.FileAlignment;
        section.VirtualAddress = nt.OptionalHeader.SectionAlignment;
        section.Misc.VirtualSize = sizeof(data);
        section.SizeOfRawData = sizeof(data);
        section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

        WriteFile(hfile, &dos_header, sizeof(dos_header), &dummy, NULL);
        WriteFile(hfile, &nt, sizeof(nt), &dummy, NULL);
        WriteFile(hfile, &section, sizeof(section), &dummy, NULL);

        SetFilePointer( hfile, section.PointerToRawData, NULL, SEEK_SET );
        WriteFile(hfile, &data, sizeof(data), &dummy, NULL);

        CloseHandle( hfile );

        switch (test)
        {
        case 0:  /* normal load */
            mod = LoadLibraryA( dll_name );
            ok( mod != NULL, "failed to load err %u\n", GetLastError() );
            if (!mod) break;
            ptr = (struct imports *)((char *)mod + page_size);
            expect = GetProcAddress( GetModuleHandleA( data.module ), data.function.name );
            ok( (void *)ptr->thunks[0].u1.Function == expect, "thunk %p instead of %p for %s.%s\n",
                (void *)ptr->thunks[0].u1.Function, expect, data.module, data.function.name );
            ok( ptr->tls_index < 32 || broken(ptr->tls_index == 9999), /* before vista */
                "wrong tls index %d\n", ptr->tls_index );
            if (ptr->tls_index != 9999)
            {
                str = ((char **)NtCurrentTeb()->ThreadLocalStoragePointer)[ptr->tls_index];
                ok( !strcmp( str, "hello world" ), "wrong tls data '%s' at %p\n", str, str );
            }
            FreeLibrary( mod );
            break;
        case 1:  /* load with DONT_RESOLVE_DLL_REFERENCES doesn't resolve imports */
            mod = LoadLibraryExA( dll_name, 0, DONT_RESOLVE_DLL_REFERENCES );
            ok( mod != NULL, "failed to load err %u\n", GetLastError() );
            if (!mod) break;
            ptr = (struct imports *)((char *)mod + page_size);
            ok( ptr->thunks[0].u1.Function == 0xdeadbeef, "thunk resolved to %p for %s.%s\n",
                (void *)ptr->thunks[0].u1.Function, data.module, data.function.name );
            ok( ptr->tls_index == 9999, "wrong tls index %d\n", ptr->tls_index );

            mod2 = LoadLibraryA( dll_name );
            ok( mod2 == mod, "loaded twice %p / %p\n", mod, mod2 );
            ok( ptr->thunks[0].u1.Function == 0xdeadbeef, "thunk resolved to %p for %s.%s\n",
                (void *)ptr->thunks[0].u1.Function, data.module, data.function.name );
            ok( ptr->tls_index == 9999, "wrong tls index %d\n", ptr->tls_index );
            FreeLibrary( mod2 );
            FreeLibrary( mod );
            break;
        case 2:  /* load without IMAGE_FILE_DLL doesn't resolve imports */
            mod = LoadLibraryA( dll_name );
            ok( mod != NULL, "failed to load err %u\n", GetLastError() );
            if (!mod) break;
            ptr = (struct imports *)((char *)mod + page_size);
            ok( ptr->thunks[0].u1.Function == 0xdeadbeef, "thunk resolved to %p for %s.%s\n",
                (void *)ptr->thunks[0].u1.Function, data.module, data.function.name );
            ok( ptr->tls_index == 9999, "wrong tls index %d\n", ptr->tls_index );
            FreeLibrary( mod );
            break;
        }
        DeleteFileA( dll_name );
#undef DATA_RVA
    }
}

#define MAX_COUNT 10
static HANDLE attached_thread[MAX_COUNT];
static DWORD attached_thread_count;
HANDLE stop_event, event, mutex, semaphore, loader_lock_event, peb_lock_event, heap_lock_event, ack_event;
static int test_dll_phase, inside_loader_lock, inside_peb_lock, inside_heap_lock;
static LONG fls_callback_count;

static DWORD WINAPI mutex_thread_proc(void *param)
{
    HANDLE wait_list[4];
    DWORD ret;

    ret = WaitForSingleObject(mutex, 0);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#x\n", ret);

    SetEvent(param);

    wait_list[0] = stop_event;
    wait_list[1] = loader_lock_event;
    wait_list[2] = peb_lock_event;
    wait_list[3] = heap_lock_event;

    trace("%04x: mutex_thread_proc: starting\n", GetCurrentThreadId());
    while (1)
    {
        ret = WaitForMultipleObjects(sizeof(wait_list)/sizeof(wait_list[0]), wait_list, FALSE, 50);
        if (ret == WAIT_OBJECT_0) break;
        else if (ret == WAIT_OBJECT_0 + 1)
        {
            ULONG_PTR loader_lock_magic;
            trace("%04x: mutex_thread_proc: Entering loader lock\n", GetCurrentThreadId());
            ret = pLdrLockLoaderLock(0, NULL, &loader_lock_magic);
            ok(!ret, "LdrLockLoaderLock error %#x\n", ret);
            inside_loader_lock++;
            SetEvent(ack_event);
        }
        else if (ret == WAIT_OBJECT_0 + 2)
        {
            trace("%04x: mutex_thread_proc: Entering PEB lock\n", GetCurrentThreadId());
            pRtlAcquirePebLock();
            inside_peb_lock++;
            SetEvent(ack_event);
        }
        else if (ret == WAIT_OBJECT_0 + 3)
        {
            trace("%04x: mutex_thread_proc: Entering heap lock\n", GetCurrentThreadId());
            HeapLock(GetProcessHeap());
            inside_heap_lock++;
            SetEvent(ack_event);
        }
    }

    trace("%04x: mutex_thread_proc: exiting\n", GetCurrentThreadId());
    return 196;
}

static DWORD WINAPI semaphore_thread_proc(void *param)
{
    DWORD ret;

    ret = WaitForSingleObject(semaphore, 0);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#x\n", ret);

    SetEvent(param);

    while (1)
    {
        if (winetest_debug > 1)
            trace("%04x: semaphore_thread_proc: still alive\n", GetCurrentThreadId());
        if (WaitForSingleObject(stop_event, 50) != WAIT_TIMEOUT) break;
    }

    trace("%04x: semaphore_thread_proc: exiting\n", GetCurrentThreadId());
    return 196;
}

static DWORD WINAPI noop_thread_proc(void *param)
{
    if (param)
    {
        LONG *noop_thread_started = param;
        InterlockedIncrement(noop_thread_started);
    }

    trace("%04x: noop_thread_proc: exiting\n", GetCurrentThreadId());
    return 195;
}

static VOID WINAPI fls_callback(PVOID lpFlsData)
{
    ok(lpFlsData == (void*) 0x31415, "lpFlsData is %p, expected %p\n", lpFlsData, (void*) 0x31415);
    InterlockedIncrement(&fls_callback_count);
}

static BOOL WINAPI dll_entry_point(HINSTANCE hinst, DWORD reason, LPVOID param)
{
    static LONG noop_thread_started;
    static DWORD fls_index = FLS_OUT_OF_INDEXES;
    static int fls_count = 0;
    static int thread_detach_count = 0;
    DWORD ret;

    ok(!inside_loader_lock, "inside_loader_lock should not be set\n");
    ok(!inside_peb_lock, "inside_peb_lock should not be set\n");

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        trace("dll: %p, DLL_PROCESS_ATTACH, %p\n", hinst, param);

        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %d\n", ret);

        /* Set up the FLS slot, if FLS is available */
        if (pFlsGetValue)
        {
            void* value;
            BOOL bret;
            ret = pFlsAlloc(&fls_callback);
            ok(ret != FLS_OUT_OF_INDEXES, "FlsAlloc returned %d\n", ret);
            fls_index = ret;
            SetLastError(0xdeadbeef);
            value = pFlsGetValue(fls_index);
            ok(!value, "FlsGetValue returned %p, expected NULL\n", value);
            ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue failed with error %u\n", GetLastError());
            bret = pFlsSetValue(fls_index, (void*) 0x31415);
            ok(bret, "FlsSetValue failed\n");
            fls_count++;
        }

        break;
    case DLL_PROCESS_DETACH:
    {
        DWORD code, expected_code, i;
        HANDLE handle, process;
        void *addr;
        SIZE_T size;
        LARGE_INTEGER offset;
        DEBUG_EVENT de;

        trace("dll: %p, DLL_PROCESS_DETACH, %p\n", hinst, param);

        if (test_dll_phase == 4 || test_dll_phase == 5)
        {
            ok(0, "dll_entry_point(DLL_PROCESS_DETACH) should not be called\n");
            break;
        }

        /* The process should already deadlock at this point */
        if (test_dll_phase == 6)
        {
            /* In reality, code below never gets executed, probably some other
             * code tries to access process heap and deadlocks earlier, even XP
             * doesn't call the DLL entry point on process detach either.
             */
            HeapLock(GetProcessHeap());
            ok(0, "dll_entry_point: process should already deadlock\n");
            break;
        }

        if (test_dll_phase == 0 || test_dll_phase == 1 || test_dll_phase == 3)
            ok(param != NULL, "dll: param %p\n", param);
        else
            ok(!param, "dll: param %p\n", param);

        if (test_dll_phase == 0 || test_dll_phase == 1) expected_code = 195;
        else if (test_dll_phase == 3) expected_code = 196;
        else expected_code = STILL_ACTIVE;

        if (test_dll_phase == 3)
        {
            ret = pRtlDllShutdownInProgress();
            ok(ret, "RtlDllShutdownInProgress returned %d\n", ret);
        }
        else
        {
            ret = pRtlDllShutdownInProgress();

            /* FIXME: remove once Wine is fixed */
            todo_wine_if (!(expected_code == STILL_ACTIVE || expected_code == 196))
                ok(!ret || broken(ret) /* before Vista */, "RtlDllShutdownInProgress returned %d\n", ret);
        }

        /* In the case that the process is terminating, FLS slots should still be accessible, but
         * the callback should be already run for this thread and the contents already NULL.
         * Note that this is broken for Win2k3, which runs the callbacks *after* the DLL entry
         * point has already run.
         */
        if (param && pFlsGetValue)
        {
            void* value;
            SetLastError(0xdeadbeef);
            value = pFlsGetValue(fls_index);
            todo_wine
            {
                ok(broken(value == (void*) 0x31415) || /* Win2k3 */
                   value == NULL, "FlsGetValue returned %p, expected NULL\n", value);
            }
            ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue failed with error %u\n", GetLastError());
            todo_wine
            {
                ok(broken(fls_callback_count == thread_detach_count) || /* Win2k3 */
                   fls_callback_count == thread_detach_count + 1,
                   "wrong FLS callback count %d, expected %d\n", fls_callback_count, thread_detach_count + 1);
            }
        }
        if (pFlsFree)
        {
            BOOL ret;
            /* Call FlsFree now and run the remaining callbacks from uncleanly terminated threads */
            ret = pFlsFree(fls_index);
            ok(ret, "FlsFree failed with error %u\n", GetLastError());
            fls_index = FLS_OUT_OF_INDEXES;
            todo_wine
            {
                ok(fls_callback_count == fls_count,
                   "wrong FLS callback count %d, expected %d\n", fls_callback_count, fls_count);
            }
        }

        ok(attached_thread_count >= 2, "attached thread count should be >= 2\n");

        for (i = 0; i < attached_thread_count; i++)
        {
            /* Calling GetExitCodeThread() without waiting for thread termination
             * leads to different results due to a race condition.
             */
            if (expected_code != STILL_ACTIVE)
            {
                ret = WaitForSingleObject(attached_thread[i], 1000);
                ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#x\n", ret);
            }
            ret = GetExitCodeThread(attached_thread[i], &code);
            trace("dll: GetExitCodeThread(%u) => %d,%u\n", i, ret, code);
            ok(ret == 1, "GetExitCodeThread returned %d, expected 1\n", ret);
            ok(code == expected_code, "expected thread exit code %u, got %u\n", expected_code, code);
        }

        ret = WaitForSingleObject(event, 0);
        ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);

        ret = WaitForSingleObject(mutex, 0);
        if (expected_code == STILL_ACTIVE)
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
        else
            ok(ret == WAIT_ABANDONED, "expected WAIT_ABANDONED, got %#x\n", ret);

        /* semaphore is not abandoned on thread termination */
        ret = WaitForSingleObject(semaphore, 0);
        ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);

        if (expected_code == STILL_ACTIVE)
        {
            ret = WaitForSingleObject(attached_thread[0], 0);
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
            ret = WaitForSingleObject(attached_thread[1], 0);
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
        }
        else
        {
            ret = WaitForSingleObject(attached_thread[0], 0);
            ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#x\n", ret);
            ret = WaitForSingleObject(attached_thread[1], 0);
            ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#x\n", ret);
        }

        /* win7 doesn't allow creating a thread during process shutdown but
         * earlier Windows versions allow it.
         */
        noop_thread_started = 0;
        SetLastError(0xdeadbeef);
        handle = CreateThread(NULL, 0, noop_thread_proc, &noop_thread_started, 0, &ret);
        if (param)
        {
            ok(!handle || broken(handle != 0) /* before win7 */, "CreateThread should fail\n");
            if (!handle)
                ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
            else
            {
                ret = WaitForSingleObject(handle, 1000);
                ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
                CloseHandle(handle);
            }
        }
        else
        {
            ok(handle != 0, "CreateThread error %d\n", GetLastError());
            ret = WaitForSingleObject(handle, 1000);
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
            ok(!noop_thread_started || broken(noop_thread_started) /* XP64 */, "thread shouldn't start yet\n");
            CloseHandle(handle);
        }

        SetLastError(0xdeadbeef);
        process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, GetCurrentProcessId());
        ok(process != NULL, "OpenProcess error %d\n", GetLastError());

        noop_thread_started = 0;
        SetLastError(0xdeadbeef);
        handle = CreateRemoteThread(process, NULL, 0, noop_thread_proc, &noop_thread_started, 0, &ret);
        if (param)
        {
            ok(!handle || broken(handle != 0) /* before win7 */, "CreateRemoteThread should fail\n");
            if (!handle)
                ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
            else
            {
                ret = WaitForSingleObject(handle, 1000);
                ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
                CloseHandle(handle);
            }
        }
        else
        {
            ok(handle != 0, "CreateRemoteThread error %d\n", GetLastError());
            ret = WaitForSingleObject(handle, 1000);
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
            ok(!noop_thread_started || broken(noop_thread_started) /* XP64 */, "thread shouldn't start yet\n");
            CloseHandle(handle);
        }

        SetLastError(0xdeadbeef);
        handle = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, NULL);
        ok(handle != 0, "CreateFileMapping error %d\n", GetLastError());

        offset.u.LowPart = 0;
        offset.u.HighPart = 0;
        addr = NULL;
        size = 0;
        ret = pNtMapViewOfSection(handle, process, &addr, 0, 0, &offset,
                                  &size, 1 /* ViewShare */, 0, PAGE_READONLY);
        ok(ret == STATUS_SUCCESS, "NtMapViewOfSection error %#x\n", ret);
        ret = pNtUnmapViewOfSection(process, addr);
        ok(ret == STATUS_SUCCESS, "NtUnmapViewOfSection error %#x\n", ret);

        CloseHandle(handle);
        CloseHandle(process);

        handle = GetModuleHandleA("winver.exe");
        ok(!handle, "winver.exe shouldn't be loaded yet\n");
        SetLastError(0xdeadbeef);
        handle = LoadLibraryA("winver.exe");
        ok(handle != 0, "LoadLibrary error %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = FreeLibrary(handle);
        ok(ret, "FreeLibrary error %d\n", GetLastError());
        handle = GetModuleHandleA("winver.exe");
        if (param)
            ok(handle != 0, "winver.exe should not be unloaded\n");
        else
        todo_wine
            ok(!handle || broken(handle != 0) /* before win7 */, "winver.exe should be unloaded\n");

        SetLastError(0xdeadbeef);
        ret = WaitForDebugEvent(&de, 0);
        ok(!ret, "WaitForDebugEvent should fail\n");
todo_wine
        ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        ret = DebugActiveProcess(GetCurrentProcessId());
        ok(!ret, "DebugActiveProcess should fail\n");
        ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        ret = WaitForDebugEvent(&de, 0);
        ok(!ret, "WaitForDebugEvent should fail\n");
        ok(GetLastError() == ERROR_SEM_TIMEOUT, "expected ERROR_SEM_TIMEOUT, got %d\n", GetLastError());

        if (test_dll_phase == 2)
        {
            trace("dll: call ExitProcess()\n");
            *child_failures = winetest_get_failures();
            ExitProcess(197);
        }
        trace("dll: %p, DLL_PROCESS_DETACH, %p => DONE\n", hinst, param);
        break;
    }
    case DLL_THREAD_ATTACH:
        trace("dll: %p, DLL_THREAD_ATTACH, %p\n", hinst, param);

        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %d\n", ret);

        if (attached_thread_count < MAX_COUNT)
        {
            DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &attached_thread[attached_thread_count],
                            0, TRUE, DUPLICATE_SAME_ACCESS);
            attached_thread_count++;
        }

        /* Make sure the FLS slot is empty, if FLS is available */
        if (pFlsGetValue)
        {
            void* value;
            BOOL ret;
            SetLastError(0xdeadbeef);
            value = pFlsGetValue(fls_index);
            ok(!value, "FlsGetValue returned %p, expected NULL\n", value);
            todo_wine
                ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue failed with error %u\n", GetLastError());
            ret = pFlsSetValue(fls_index, (void*) 0x31415);
            ok(ret, "FlsSetValue failed\n");
            fls_count++;
        }

        break;
    case DLL_THREAD_DETACH:
        trace("dll: %p, DLL_THREAD_DETACH, %p\n", hinst, param);
        thread_detach_count++;

        ret = pRtlDllShutdownInProgress();
        /* win7 doesn't allow creating a thread during process shutdown but
         * earlier Windows versions allow it. In that case DLL_THREAD_DETACH is
         * sent on thread exit, but DLL_THREAD_ATTACH is never received.
         */
        if (noop_thread_started)
            ok(ret, "RtlDllShutdownInProgress returned %d\n", ret);
        else
            ok(!ret, "RtlDllShutdownInProgress returned %d\n", ret);

        /* FLS data should already be destroyed, if FLS is available.
         * Note that this is broken for Win2k3, which runs the callbacks *after* the DLL entry
         * point has already run.
         */
        if (pFlsGetValue && fls_index != FLS_OUT_OF_INDEXES)
        {
            void* value;
            SetLastError(0xdeadbeef);
            value = pFlsGetValue(fls_index);
            todo_wine
            {
                ok(broken(value == (void*) 0x31415) || /* Win2k3 */
                   !value, "FlsGetValue returned %p, expected NULL\n", value);
            }
            ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue failed with error %u\n", GetLastError());
        }

        break;
    default:
        trace("dll: %p, %d, %p\n", hinst, reason, param);
        break;
    }

    *child_failures = winetest_get_failures();

    return TRUE;
}

static void child_process(const char *dll_name, DWORD target_offset)
{
    void *target;
    DWORD ret, dummy, i, code, expected_code;
    HANDLE file, thread, process;
    HMODULE hmod;
    struct PROCESS_BASIC_INFORMATION_PRIVATE pbi;
    DWORD_PTR affinity;

    trace("phase %d: writing %p at %#x\n", test_dll_phase, dll_entry_point, target_offset);

    SetLastError(0xdeadbeef);
    mutex = CreateMutexW(NULL, FALSE, NULL);
    ok(mutex != 0, "CreateMutex error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    semaphore = CreateSemaphoreW(NULL, 1, 1, NULL);
    ok(semaphore != 0, "CreateSemaphore error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(event != 0, "CreateEvent error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    loader_lock_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(loader_lock_event != 0, "CreateEvent error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    peb_lock_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(peb_lock_event != 0, "CreateEvent error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    heap_lock_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(heap_lock_event != 0, "CreateEvent error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ack_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(ack_event != 0, "CreateEvent error %d\n", GetLastError());

    file = CreateFileA(dll_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
    {
        ok(0, "could not open %s\n", dll_name);
        return;
    }
    SetFilePointer(file, target_offset, NULL, FILE_BEGIN);
    SetLastError(0xdeadbeef);
    target = dll_entry_point;
    ret = WriteFile(file, &target, sizeof(target), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());
    CloseHandle(file);

    SetLastError(0xdeadbeef);
    hmod = LoadLibraryA(dll_name);
    ok(hmod != 0, "LoadLibrary error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(stop_event != 0, "CreateEvent error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    thread = CreateThread(NULL, 0, mutex_thread_proc, event, 0, &dummy);
    ok(thread != 0, "CreateThread error %d\n", GetLastError());
    WaitForSingleObject(event, 3000);
    CloseHandle(thread);

    ResetEvent(event);

    SetLastError(0xdeadbeef);
    thread = CreateThread(NULL, 0, semaphore_thread_proc, event, 0, &dummy);
    ok(thread != 0, "CreateThread error %d\n", GetLastError());
    WaitForSingleObject(event, 3000);
    CloseHandle(thread);

    ResetEvent(event);
    Sleep(100);

    ok(attached_thread_count == 2, "attached thread count should be 2\n");
    for (i = 0; i < attached_thread_count; i++)
    {
        ret = GetExitCodeThread(attached_thread[i], &code);
        trace("child: GetExitCodeThread(%u) => %d,%u\n", i, ret, code);
        ok(ret == 1, "GetExitCodeThread returned %d, expected 1\n", ret);
        ok(code == STILL_ACTIVE, "expected thread exit code STILL_ACTIVE, got %u\n", code);
    }

    ret = WaitForSingleObject(attached_thread[0], 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
    ret = WaitForSingleObject(attached_thread[1], 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);

    ret = WaitForSingleObject(event, 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
    ret = WaitForSingleObject(mutex, 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
    ret = WaitForSingleObject(semaphore, 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);

    ret = pRtlDllShutdownInProgress();
    ok(!ret, "RtlDllShutdownInProgress returned %d\n", ret);

    SetLastError(0xdeadbeef);
    process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, GetCurrentProcessId());
    ok(process != NULL, "OpenProcess error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = TerminateProcess(0, 195);
    ok(!ret, "TerminateProcess(0) should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    Sleep(100);

    affinity = 1;
    ret = pNtSetInformationProcess(process, ProcessAffinityMask, &affinity, sizeof(affinity));
    ok(!ret, "NtSetInformationProcess error %#x\n", ret);

    switch (test_dll_phase)
    {
    case 0:
        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %d\n", ret);

        trace("call NtTerminateProcess(0, 195)\n");
        ret = pNtTerminateProcess(0, 195);
        ok(!ret, "NtTerminateProcess error %#x\n", ret);

        memset(&pbi, 0, sizeof(pbi));
        ret = pNtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
        ok(!ret, "NtQueryInformationProcess error %#x\n", ret);
        ok(pbi.ExitStatus == STILL_ACTIVE || pbi.ExitStatus == 195,
           "expected STILL_ACTIVE, got %lu\n", pbi.ExitStatus);
        affinity = 1;
        ret = pNtSetInformationProcess(process, ProcessAffinityMask, &affinity, sizeof(affinity));
        ok(!ret, "NtSetInformationProcess error %#x\n", ret);

        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %d\n", ret);

        hmod = GetModuleHandleA(dll_name);
        ok(hmod != 0, "DLL should not be unloaded\n");

        SetLastError(0xdeadbeef);
        thread = CreateThread(NULL, 0, noop_thread_proc, &dummy, 0, &ret);
        ok(!thread || broken(thread != 0) /* before win7 */, "CreateThread should fail\n");
        if (!thread)
            ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
        else
        {
            ret = WaitForSingleObject(thread, 1000);
            ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#x\n", ret);
            CloseHandle(thread);
        }

        trace("call LdrShutdownProcess()\n");
        pLdrShutdownProcess();

        ret = pRtlDllShutdownInProgress();
        ok(ret, "RtlDllShutdownInProgress returned %d\n", ret);

        hmod = GetModuleHandleA(dll_name);
        ok(hmod != 0, "DLL should not be unloaded\n");

        memset(&pbi, 0, sizeof(pbi));
        ret = pNtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
        ok(!ret, "NtQueryInformationProcess error %#x\n", ret);
        ok(pbi.ExitStatus == STILL_ACTIVE || pbi.ExitStatus == 195,
           "expected STILL_ACTIVE, got %lu\n", pbi.ExitStatus);
        affinity = 1;
        ret = pNtSetInformationProcess(process, ProcessAffinityMask, &affinity, sizeof(affinity));
        ok(!ret, "NtSetInformationProcess error %#x\n", ret);
        break;

    case 1: /* normal ExitProcess */
        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %d\n", ret);
        break;

    case 2: /* ExitProcess will be called by the PROCESS_DETACH handler */
        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %d\n", ret);

        trace("call FreeLibrary(%p)\n", hmod);
        SetLastError(0xdeadbeef);
        ret = FreeLibrary(hmod);
        ok(ret, "FreeLibrary error %d\n", GetLastError());
        hmod = GetModuleHandleA(dll_name);
        ok(!hmod, "DLL should be unloaded\n");

        if (test_dll_phase == 2)
            ok(0, "FreeLibrary+ExitProcess should never return\n");

        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %d\n", ret);

        break;

    case 3:
        trace("signalling thread exit\n");
        SetEvent(stop_event);
        break;

    case 4:
        trace("setting loader_lock_event\n");
        SetEvent(loader_lock_event);
        WaitForSingleObject(ack_event, 1000);
        ok(inside_loader_lock != 0, "inside_loader_lock is not set\n");

        /* calling NtTerminateProcess should not cause a deadlock */
        trace("call NtTerminateProcess(0, 198)\n");
        ret = pNtTerminateProcess(0, 198);
        ok(!ret, "NtTerminateProcess error %#x\n", ret);

        *child_failures = winetest_get_failures();

        /* Windows fails to release loader lock acquired from another thread,
         * so the LdrUnlockLoaderLock call fails here and ExitProcess deadlocks
         * later on, so NtTerminateProcess is used instead.
         */
        trace("call NtTerminateProcess(GetCurrentProcess(), 198)\n");
        pNtTerminateProcess(GetCurrentProcess(), 198);
        ok(0, "NtTerminateProcess should not return\n");
        break;

    case 5:
        trace("setting peb_lock_event\n");
        SetEvent(peb_lock_event);
        WaitForSingleObject(ack_event, 1000);
        ok(inside_peb_lock != 0, "inside_peb_lock is not set\n");

        *child_failures = winetest_get_failures();

        /* calling ExitProcess should cause a deadlock */
        trace("call ExitProcess(198)\n");
        ExitProcess(198);
        ok(0, "ExitProcess should not return\n");
        break;

    case 6:
        trace("setting heap_lock_event\n");
        SetEvent(heap_lock_event);
        WaitForSingleObject(ack_event, 1000);
        ok(inside_heap_lock != 0, "inside_heap_lock is not set\n");

        *child_failures = winetest_get_failures();

        /* calling ExitProcess should cause a deadlock */
        trace("call ExitProcess(1)\n");
        ExitProcess(1);
        ok(0, "ExitProcess should not return\n");
        break;

    default:
        assert(0);
        break;
    }

    if (test_dll_phase == 0) expected_code = 195;
    else if (test_dll_phase == 3) expected_code = 196;
    else if (test_dll_phase == 4) expected_code = 198;
    else expected_code = STILL_ACTIVE;

    if (expected_code == STILL_ACTIVE)
    {
        ret = WaitForSingleObject(attached_thread[0], 100);
        ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
        ret = WaitForSingleObject(attached_thread[1], 100);
        ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#x\n", ret);
    }
    else
    {
        ret = WaitForSingleObject(attached_thread[0], 2000);
        ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#x\n", ret);
        ret = WaitForSingleObject(attached_thread[1], 2000);
        ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#x\n", ret);
    }

    for (i = 0; i < attached_thread_count; i++)
    {
        ret = GetExitCodeThread(attached_thread[i], &code);
        trace("child: GetExitCodeThread(%u) => %d,%u\n", i, ret, code);
        ok(ret == 1, "GetExitCodeThread returned %d, expected 1\n", ret);
        ok(code == expected_code, "expected thread exit code %u, got %u\n", expected_code, code);
    }

    *child_failures = winetest_get_failures();

    trace("call ExitProcess(195)\n");
    ExitProcess(195);
}

static void test_ExitProcess(void)
{
#include "pshpack1.h"
#ifdef __x86_64__
    static struct section_data
    {
        BYTE mov_rax[2];
        void *target;
        BYTE jmp_rax[2];
    } section_data = { { 0x48,0xb8 }, dll_entry_point, { 0xff,0xe0 } };
#else
    static struct section_data
    {
        BYTE mov_eax;
        void *target;
        BYTE jmp_eax[2];
    } section_data = { 0xb8, dll_entry_point, { 0xff,0xe0 } };
#endif
#include "poppack.h"
    DWORD dummy, file_align;
    HANDLE file, thread, process, hmap, hmap_dup;
    char temp_path[MAX_PATH], dll_name[MAX_PATH], cmdline[MAX_PATH * 2];
    DWORD ret, target_offset, old_prot;
    char **argv, buf[256];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { sizeof(si) };
    CONTEXT ctx;
    struct PROCESS_BASIC_INFORMATION_PRIVATE pbi;
    MEMORY_BASIC_INFORMATION mbi;
    DWORD_PTR affinity;
    void *addr;
    LARGE_INTEGER offset;
    SIZE_T size;
    IMAGE_NT_HEADERS nt_header;

#if !defined(__i386__) && !defined(__x86_64__)
    skip("x86 specific ExitProcess test\n");
    return;
#endif

    if (!pRtlDllShutdownInProgress)
    {
        win_skip("RtlDllShutdownInProgress is not available on this platform (XP+)\n");
        return;
    }
    if (!pNtQueryInformationProcess || !pNtSetInformationProcess)
    {
        win_skip("NtQueryInformationProcess/NtSetInformationProcess are not available on this platform\n");
        return;
    }
    if (!pNtAllocateVirtualMemory || !pNtFreeVirtualMemory)
    {
        win_skip("NtAllocateVirtualMemory/NtFreeVirtualMemory are not available on this platform\n");
        return;
    }

    /* prevent displaying of the "Unable to load this DLL" message box */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "ldr", 0, dll_name);

    /*trace("creating %s\n", dll_name);*/
    file = CreateFileA(dll_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
    {
        ok(0, "could not create %s\n", dll_name);
        return;
    }

    SetLastError(0xdeadbeef);
    ret = WriteFile(file, &dos_header, sizeof(dos_header), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    nt_header = nt_header_template;
    nt_header.FileHeader.NumberOfSections = 1;
    nt_header.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
    nt_header.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL | IMAGE_FILE_RELOCS_STRIPPED;

    nt_header.OptionalHeader.AddressOfEntryPoint = 0x1000;
    nt_header.OptionalHeader.SectionAlignment = 0x1000;
    nt_header.OptionalHeader.FileAlignment = 0x200;
    nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000;
    nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER);
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, &nt_header.OptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    section.SizeOfRawData = sizeof(section_data);
    section.PointerToRawData = nt_header.OptionalHeader.FileAlignment;
    section.VirtualAddress = nt_header.OptionalHeader.SectionAlignment;
    section.Misc.VirtualSize = sizeof(section_data);
    section.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE;
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, &section, sizeof(section), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    file_align = nt_header.OptionalHeader.FileAlignment - nt_header.OptionalHeader.SizeOfHeaders;
    assert(file_align < sizeof(filler));
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, filler, file_align, &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    target_offset = SetFilePointer(file, 0, NULL, FILE_CURRENT) + FIELD_OFFSET(struct section_data, target);

    /* section data */
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, &section_data, sizeof(section_data), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    CloseHandle(file);

    winetest_get_mainargs(&argv);

    /* phase 0 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %u 0", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %d\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 10000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 195, "expected exit code 195, got %u\n", ret);
    if (*child_failures)
    {
        trace("%d failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* phase 1 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %u 1", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %d\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 10000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 195, "expected exit code 195, got %u\n", ret);
    if (*child_failures)
    {
        trace("%d failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* phase 2 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %u 2", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %d\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 10000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 197, "expected exit code 197, got %u\n", ret);
    if (*child_failures)
    {
        trace("%d failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* phase 3 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %u 3", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %d\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 10000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 195, "expected exit code 195, got %u\n", ret);
    if (*child_failures)
    {
        trace("%d failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* phase 4 */
    if (pLdrLockLoaderLock && pLdrUnlockLoaderLock)
    {
        *child_failures = -1;
        sprintf(cmdline, "\"%s\" loader %s %u 4", argv[0], dll_name, target_offset);
        ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess(%s) error %d\n", cmdline, GetLastError());
        ret = WaitForSingleObject(pi.hProcess, 10000);
        ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
        if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
        GetExitCodeProcess(pi.hProcess, &ret);
        ok(ret == 198, "expected exit code 198, got %u\n", ret);
        if (*child_failures)
        {
            trace("%d failures in child process\n", *child_failures);
            winetest_add_failures(*child_failures);
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else
        win_skip("LdrLockLoaderLock/LdrUnlockLoaderLock are not available on this platform\n");

    /* phase 5 */
    if (pRtlAcquirePebLock && pRtlReleasePebLock)
    {
        *child_failures = -1;
        sprintf(cmdline, "\"%s\" loader %s %u 5", argv[0], dll_name, target_offset);
        ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess(%s) error %d\n", cmdline, GetLastError());
        ret = WaitForSingleObject(pi.hProcess, 5000);
        ok(ret == WAIT_TIMEOUT, "child process should fail to terminate\n");
        if (ret != WAIT_OBJECT_0)
        {
            trace("terminating child process\n");
            TerminateProcess(pi.hProcess, 199);
        }
        ret = WaitForSingleObject(pi.hProcess, 1000);
        ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
        GetExitCodeProcess(pi.hProcess, &ret);
        ok(ret == 199, "expected exit code 199, got %u\n", ret);
        if (*child_failures)
        {
            trace("%d failures in child process\n", *child_failures);
            winetest_add_failures(*child_failures);
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else
        win_skip("RtlAcquirePebLock/RtlReleasePebLock are not available on this platform\n");

    /* phase 6 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %u 6", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %d\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 5000);
    ok(ret == WAIT_TIMEOUT || broken(ret == WAIT_OBJECT_0) /* XP */, "child process should fail to terminate\n");
    if (ret != WAIT_OBJECT_0)
    {
        trace("terminating child process\n");
        TerminateProcess(pi.hProcess, 201);
    }
    ret = WaitForSingleObject(pi.hProcess, 1000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 201 || broken(ret == 1) /* XP */, "expected exit code 201, got %u\n", ret);
    if (*child_failures)
    {
        trace("%d failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* test remote process termination */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(argv[0], NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %d\n", argv[0], GetLastError());

    SetLastError(0xdeadbeef);
    addr = VirtualAllocEx(pi.hProcess, NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
    ok(addr != NULL, "VirtualAllocEx error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = VirtualProtectEx(pi.hProcess, addr, 4096, PAGE_READONLY, &old_prot);
    ok(ret, "VirtualProtectEx error %d\n", GetLastError());
    ok(old_prot == PAGE_READWRITE, "expected PAGE_READWRITE, got %#x\n", old_prot);
    SetLastError(0xdeadbeef);
    size = VirtualQueryEx(pi.hProcess, NULL, &mbi, sizeof(mbi));
    ok(size == sizeof(mbi), "VirtualQueryEx error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadProcessMemory(pi.hProcess, addr, buf, 4, &size);
    ok(ret, "ReadProcessMemory error %d\n", GetLastError());
    ok(size == 4, "expected 4, got %lu\n", size);

    SetLastError(0xdeadbeef);
    hmap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, NULL);
    ok(hmap != 0, "CreateFileMapping error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(GetCurrentProcess(), hmap, pi.hProcess, &hmap_dup,
                          0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "DuplicateHandle error %d\n", GetLastError());

    offset.u.LowPart = 0;
    offset.u.HighPart = 0;
    addr = NULL;
    size = 0;
    ret = pNtMapViewOfSection(hmap, pi.hProcess, &addr, 0, 0, &offset,
                              &size, 1 /* ViewShare */, 0, PAGE_READONLY);
    ok(!ret, "NtMapViewOfSection error %#x\n", ret);
    ret = pNtUnmapViewOfSection(pi.hProcess, addr);
    ok(!ret, "NtUnmapViewOfSection error %#x\n", ret);

    SetLastError(0xdeadbeef);
    thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void *)0xdeadbeef, NULL, CREATE_SUSPENDED, &ret);
    ok(thread != 0, "CreateRemoteThread error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = GetThreadContext(thread, &ctx);
    ok(ret, "GetThreadContext error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = SetThreadContext(thread, &ctx);
    ok(ret, "SetThreadContext error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = SetThreadPriority(thread, 0);
    ok(ret, "SetThreadPriority error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = TerminateThread(thread, 199);
    ok(ret, "TerminateThread error %d\n", GetLastError());
    /* Calling GetExitCodeThread() without waiting for thread termination
     * leads to different results due to a race condition.
     */
    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed: %x\n", ret);
    GetExitCodeThread(thread, &ret);
    ok(ret == 199, "expected exit code 199, got %u\n", ret);

    SetLastError(0xdeadbeef);
    ret = TerminateProcess(pi.hProcess, 198);
    ok(ret, "TerminateProcess error %d\n", GetLastError());
    /* Checking process state without waiting for process termination
     * leads to different results due to a race condition.
     */
    ret = WaitForSingleObject(pi.hProcess, 1000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed: %x\n", ret);

    SetLastError(0xdeadbeef);
    process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, pi.dwProcessId);
    ok(process != NULL, "OpenProcess error %d\n", GetLastError());
    CloseHandle(process);

    memset(&pbi, 0, sizeof(pbi));
    ret = pNtQueryInformationProcess(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok(!ret, "NtQueryInformationProcess error %#x\n", ret);
    ok(pbi.ExitStatus == 198, "expected 198, got %lu\n", pbi.ExitStatus);
    affinity = 1;
    ret = pNtSetInformationProcess(pi.hProcess, ProcessAffinityMask, &affinity, sizeof(affinity));
    ok(ret == STATUS_PROCESS_IS_TERMINATING, "expected STATUS_PROCESS_IS_TERMINATING, got %#x\n", ret);

    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = GetThreadContext(thread, &ctx);
    ok(!ret || broken(ret) /* XP 64-bit */, "GetThreadContext should fail\n");
    if (!ret)
        ok(GetLastError() == ERROR_INVALID_PARAMETER ||
           GetLastError() == ERROR_GEN_FAILURE /* win7 64-bit */ ||
           GetLastError() == ERROR_INVALID_FUNCTION /* vista 64-bit */,
           "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = SetThreadContext(thread, &ctx);
    ok(!ret || broken(ret) /* XP 64-bit */, "SetThreadContext should fail\n");
    if (!ret)
        ok(GetLastError() == ERROR_ACCESS_DENIED ||
           GetLastError() == ERROR_GEN_FAILURE /* win7 64-bit */ ||
           GetLastError() == ERROR_INVALID_FUNCTION /* vista 64-bit */,
           "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = SetThreadPriority(thread, 0);
    ok(ret, "SetThreadPriority error %d\n", GetLastError());
    CloseHandle(thread);

    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = GetThreadContext(pi.hThread, &ctx);
    ok(!ret || broken(ret) /* XP 64-bit */, "GetThreadContext should fail\n");
    if (!ret)
        ok(GetLastError() == ERROR_INVALID_PARAMETER ||
           GetLastError() == ERROR_GEN_FAILURE /* win7 64-bit */ ||
           GetLastError() == ERROR_INVALID_FUNCTION /* vista 64-bit */,
           "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = SetThreadContext(pi.hThread, &ctx);
    ok(!ret || broken(ret) /* XP 64-bit */, "SetThreadContext should fail\n");
    if (!ret)
        ok(GetLastError() == ERROR_ACCESS_DENIED ||
           GetLastError() == ERROR_GEN_FAILURE /* win7 64-bit */ ||
           GetLastError() == ERROR_INVALID_FUNCTION /* vista 64-bit */,
           "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = VirtualProtectEx(pi.hProcess, addr, 4096, PAGE_READWRITE, &old_prot);
    ok(!ret, "VirtualProtectEx should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    size = 0;
    ret = ReadProcessMemory(pi.hProcess, addr, buf, 4, &size);
    ok(!ret, "ReadProcessMemory should fail\n");
    ok(GetLastError() == ERROR_PARTIAL_COPY || GetLastError() == ERROR_ACCESS_DENIED,
       "expected ERROR_PARTIAL_COPY, got %d\n", GetLastError());
    ok(!size, "expected 0, got %lu\n", size);
    SetLastError(0xdeadbeef);
    ret = VirtualFreeEx(pi.hProcess, addr, 0, MEM_RELEASE);
    ok(!ret, "VirtualFreeEx should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    addr = VirtualAllocEx(pi.hProcess, NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
    ok(!addr, "VirtualAllocEx should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    size = VirtualQueryEx(pi.hProcess, NULL, &mbi, sizeof(mbi));
    ok(!size, "VirtualQueryEx should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* CloseHandle() call below leads to premature process termination
     * under some Windows versions.
     */
if (0)
{
    SetLastError(0xdeadbeef);
    ret = CloseHandle(hmap_dup);
    ok(ret, "CloseHandle should not fail\n");
}

    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(GetCurrentProcess(), hmap, pi.hProcess, &hmap_dup,
                          0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(!ret, "DuplicateHandle should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    offset.u.LowPart = 0;
    offset.u.HighPart = 0;
    addr = NULL;
    size = 0;
    ret = pNtMapViewOfSection(hmap, pi.hProcess, &addr, 0, 0, &offset,
                              &size, 1 /* ViewShare */, 0, PAGE_READONLY);
    ok(ret == STATUS_PROCESS_IS_TERMINATING, "expected STATUS_PROCESS_IS_TERMINATING, got %#x\n", ret);

    SetLastError(0xdeadbeef);
    thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void *)0xdeadbeef, NULL, CREATE_SUSPENDED, &ret);
    ok(!thread, "CreateRemoteThread should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DebugActiveProcess(pi.dwProcessId);
    ok(!ret, "DebugActiveProcess should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED /* 64-bit */ || GetLastError() == ERROR_NOT_SUPPORTED /* 32-bit */,
      "ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 198 || broken(ret != 198) /* some 32-bit XP version in a VM returns random exit code */,
       "expected exit code 198, got %u\n", ret);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    ret = DeleteFileA(dll_name);
    ok(ret, "DeleteFile error %d\n", GetLastError());
}

static PVOID WINAPI failuredllhook(ULONG ul, DELAYLOAD_INFO* pd)
{
    ok(ul == 4, "expected 4, got %u\n", ul);
    ok(!!pd, "no delayload info supplied\n");
    if (pd)
    {
        ok(pd->Size == sizeof(*pd), "got %u\n", pd->Size);
        ok(!!pd->DelayloadDescriptor, "no DelayloadDescriptor supplied\n");
        if (pd->DelayloadDescriptor)
        {
            ok(pd->DelayloadDescriptor->Attributes.AllAttributes == 1,
               "expected 1, got %u\n", pd->DelayloadDescriptor->Attributes.AllAttributes);
            ok(pd->DelayloadDescriptor->DllNameRVA == 0x2000,
               "expected 0x2000, got %x\n", pd->DelayloadDescriptor->DllNameRVA);
            ok(pd->DelayloadDescriptor->ModuleHandleRVA == 0x201a,
               "expected 0x201a, got %x\n", pd->DelayloadDescriptor->ModuleHandleRVA);
            ok(pd->DelayloadDescriptor->ImportAddressTableRVA > pd->DelayloadDescriptor->ModuleHandleRVA,
               "expected %x > %x\n", pd->DelayloadDescriptor->ImportAddressTableRVA,
               pd->DelayloadDescriptor->ModuleHandleRVA);
            ok(pd->DelayloadDescriptor->ImportNameTableRVA > pd->DelayloadDescriptor->ImportAddressTableRVA,
               "expected %x > %x\n", pd->DelayloadDescriptor->ImportNameTableRVA,
               pd->DelayloadDescriptor->ImportAddressTableRVA);
            ok(pd->DelayloadDescriptor->BoundImportAddressTableRVA == 0,
               "expected 0, got %x\n", pd->DelayloadDescriptor->BoundImportAddressTableRVA);
            ok(pd->DelayloadDescriptor->UnloadInformationTableRVA == 0,
               "expected 0, got %x\n", pd->DelayloadDescriptor->UnloadInformationTableRVA);
            ok(pd->DelayloadDescriptor->TimeDateStamp == 0,
               "expected 0, got %x\n", pd->DelayloadDescriptor->TimeDateStamp);
        }

        ok(!!pd->ThunkAddress, "no ThunkAddress supplied\n");
        if (pd->ThunkAddress)
            ok(pd->ThunkAddress->u1.Ordinal, "no ThunkAddress value supplied\n");

        ok(!!pd->TargetDllName, "no TargetDllName supplied\n");
        if (pd->TargetDllName)
            ok(!strcmp(pd->TargetDllName, "secur32.dll"),
               "expected \"secur32.dll\", got \"%s\"\n", pd->TargetDllName);

        ok(pd->TargetApiDescriptor.ImportDescribedByName == 0,
           "expected 0, got %x\n", pd->TargetApiDescriptor.ImportDescribedByName);
        ok(pd->TargetApiDescriptor.Description.Ordinal == 0 ||
           pd->TargetApiDescriptor.Description.Ordinal == 999,
           "expected 0, got %x\n", pd->TargetApiDescriptor.Description.Ordinal);

        ok(!!pd->TargetModuleBase, "no TargetModuleBase supplied\n");
        ok(pd->Unused == NULL, "expected NULL, got %p\n", pd->Unused);
        ok(pd->LastError, "no LastError supplied\n");
    }
    cb_count++;
    return (void*)0xdeadbeef;
}

static void test_ResolveDelayLoadedAPI(void)
{
    static const char test_dll[] = "secur32.dll";
    static const char test_func[] = "SealMessage";
    char temp_path[MAX_PATH];
    char dll_name[MAX_PATH];
    IMAGE_DELAYLOAD_DESCRIPTOR idd, *delaydir;
    IMAGE_THUNK_DATA itd32;
    HANDLE hfile;
    HMODULE hlib;
    DWORD dummy, file_size, i;
    WORD hint = 0;
    BOOL ret;
    IMAGE_NT_HEADERS nt_header;

    static const struct test_data
    {
        BOOL func;
        UINT_PTR ordinal;
        BOOL succeeds;
    } td[] =
    {
        {
            TRUE, 0, TRUE
        },
        {
            FALSE, IMAGE_ORDINAL_FLAG | 2, TRUE
        },
        {
            FALSE, IMAGE_ORDINAL_FLAG | 5, TRUE
        },
        {
            FALSE, IMAGE_ORDINAL_FLAG | 0, FALSE
        },
        {
            FALSE, IMAGE_ORDINAL_FLAG | 999, FALSE
        },
    };

    if (!pResolveDelayLoadedAPI)
    {
        win_skip("ResolveDelayLoadedAPI is not available\n");
        return;
    }

    if (0) /* crashes on native */
    {
        SetLastError(0xdeadbeef);
        ok(!pResolveDelayLoadedAPI(NULL, NULL, NULL, NULL, NULL, 0),
           "ResolveDelayLoadedAPI succeeded\n");
        ok(GetLastError() == 0xdeadbeef, "GetLastError changed to %x\n", GetLastError());

        cb_count = 0;
        SetLastError(0xdeadbeef);
        ok(!pResolveDelayLoadedAPI(NULL, NULL, failuredllhook, NULL, NULL, 0),
           "ResolveDelayLoadedAPI succeeded\n");
        ok(GetLastError() == 0xdeadbeef, "GetLastError changed to %x\n", GetLastError());
        ok(cb_count == 1, "Wrong callback count: %d\n", cb_count);
    }

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "ldr", 0, dll_name);
    trace("creating %s\n", dll_name);
    hfile = CreateFileA(dll_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    if (hfile == INVALID_HANDLE_VALUE)
    {
        ok(0, "could not create %s\n", dll_name);
        return;
    }

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &dos_header, sizeof(dos_header), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    nt_header = nt_header_template;
    nt_header.FileHeader.NumberOfSections = 2;
    nt_header.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);

    nt_header.OptionalHeader.SectionAlignment = 0x1000;
    nt_header.OptionalHeader.FileAlignment = 0x1000;
    nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x2200;
    nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + 2 * sizeof(IMAGE_SECTION_HEADER);
    nt_header.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress = 0x1000;
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size = 0x100;

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &nt_header.OptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    /* sections */
    section.PointerToRawData = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;
    section.VirtualAddress = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;
    section.Misc.VirtualSize = 2 * sizeof(idd);
    section.SizeOfRawData = section.Misc.VirtualSize;
    section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &section, sizeof(section), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    section.PointerToRawData = 0x2000;
    section.VirtualAddress = 0x2000;
    i = sizeof(td)/sizeof(td[0]);
    section.Misc.VirtualSize = sizeof(test_dll) + sizeof(hint) + sizeof(test_func) + sizeof(HMODULE) +
                               2 * (i + 1) * sizeof(IMAGE_THUNK_DATA);
    ok(section.Misc.VirtualSize <= 0x1000, "Too much tests, add a new section!\n");
    section.SizeOfRawData = section.Misc.VirtualSize;
    section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &section, sizeof(section), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    /* fill up to delay data */
    SetFilePointer( hfile, nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress, NULL, SEEK_SET );

    /* delay data */
    idd.Attributes.AllAttributes = 1;
    idd.DllNameRVA = 0x2000;
    idd.ModuleHandleRVA = idd.DllNameRVA + sizeof(test_dll) + sizeof(hint) + sizeof(test_func);
    idd.ImportAddressTableRVA = idd.ModuleHandleRVA + sizeof(HMODULE);
    idd.ImportNameTableRVA = idd.ImportAddressTableRVA + (i + 1) * sizeof(IMAGE_THUNK_DATA);
    idd.BoundImportAddressTableRVA = 0;
    idd.UnloadInformationTableRVA = 0;
    idd.TimeDateStamp = 0;

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &idd, sizeof(idd), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, filler, sizeof(idd), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    /* fill up to extended delay data */
    SetFilePointer( hfile, idd.DllNameRVA, NULL, SEEK_SET );

    /* extended delay data */
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, test_dll, sizeof(test_dll), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &hint, sizeof(hint), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, test_func, sizeof(test_func), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    SetFilePointer( hfile, idd.ImportAddressTableRVA, NULL, SEEK_SET );

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        /* 0x1a00 is an empty space between delay data and extended delay data, real thunks are not necessary */
        itd32.u1.Function = nt_header.OptionalHeader.ImageBase + 0x1a00 + i * 0x20;
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &itd32, sizeof(itd32), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());
    }

    itd32.u1.Function = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &itd32, sizeof(itd32), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        if (td[i].func)
            itd32.u1.AddressOfData = idd.DllNameRVA + sizeof(test_dll);
        else
            itd32.u1.Ordinal = td[i].ordinal;
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &itd32, sizeof(itd32), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());
    }

    itd32.u1.Ordinal = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &itd32, sizeof(itd32), &dummy, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());

    /* fill up to eof */
    SetFilePointer( hfile, section.VirtualAddress + section.Misc.VirtualSize, NULL, SEEK_SET );
    SetEndOfFile( hfile );
    CloseHandle(hfile);

    SetLastError(0xdeadbeef);
    hlib = LoadLibraryA(dll_name);
    ok(hlib != NULL, "LoadLibrary error %u\n", GetLastError());
    if (!hlib)
    {
        skip("couldn't load %s.\n", dll_name);
        DeleteFileA(dll_name);
        return;
    }

    delaydir = pRtlImageDirectoryEntryToData(hlib, TRUE, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT, &file_size);
    if (!delaydir)
    {
        skip("haven't found section for delay import directory.\n");
        FreeLibrary(hlib);
        DeleteFileA(dll_name);
        return;
    }

    for (;;)
    {
        IMAGE_THUNK_DATA *itdn, *itda;
        HMODULE htarget;

        if (!delaydir->DllNameRVA ||
            !delaydir->ImportAddressTableRVA ||
            !delaydir->ImportNameTableRVA) break;

        itdn = RVAToAddr(delaydir->ImportNameTableRVA, hlib);
        itda = RVAToAddr(delaydir->ImportAddressTableRVA, hlib);
        htarget = LoadLibraryA(RVAToAddr(delaydir->DllNameRVA, hlib));

        for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
        {
            void *ret, *load;

            if (IMAGE_SNAP_BY_ORDINAL(itdn[i].u1.Ordinal))
                load = (void *)GetProcAddress(htarget, (LPSTR)IMAGE_ORDINAL(itdn[i].u1.Ordinal));
            else
            {
                const IMAGE_IMPORT_BY_NAME* iibn = RVAToAddr(itdn[i].u1.AddressOfData, hlib);
                load = (void *)GetProcAddress(htarget, (char*)iibn->Name);
            }

            cb_count = 0;
            ret = pResolveDelayLoadedAPI(hlib, delaydir, failuredllhook, NULL, &itda[i], 0);
            if (td[i].succeeds)
            {
                ok(ret != NULL, "Test %u: ResolveDelayLoadedAPI failed\n", i);
                ok(ret == load, "Test %u: expected %p, got %p\n", i, load, ret);
                ok(ret == (void*)itda[i].u1.AddressOfData, "Test %u: expected %p, got %p\n",
                   i, ret, (void*)itda[i].u1.AddressOfData);
                ok(!cb_count, "Test %u: Wrong callback count: %d\n", i, cb_count);
            }
            else
            {
                ok(ret == (void*)0xdeadbeef, "Test %u: ResolveDelayLoadedAPI succeeded with %p\n", i, ret);
                ok(cb_count, "Test %u: Wrong callback count: %d\n", i, cb_count);
            }
        }
        delaydir++;
    }

    FreeLibrary(hlib);
    trace("deleting %s\n", dll_name);
    DeleteFileA(dll_name);
}

static void test_InMemoryOrderModuleList(void)
{
    PEB_LDR_DATA *ldr = NtCurrentTeb()->Peb->LdrData;
    LIST_ENTRY *entry1, *mark1 = &ldr->InLoadOrderModuleList;
    LIST_ENTRY *entry2, *mark2 = &ldr->InMemoryOrderModuleList;
    LDR_DATA_TABLE_ENTRY *module1, *module2;

    ok(ldr->Initialized == TRUE, "expected TRUE, got %u\n", ldr->Initialized);

    for (entry1 = mark1->Flink, entry2 = mark2->Flink;
         entry1 != mark1 && entry2 != mark2;
         entry1 = entry1->Flink, entry2 = entry2->Flink)
    {
        module1 = CONTAINING_RECORD(entry1, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        module2 = CONTAINING_RECORD(entry2, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        ok(module1 == module2, "expected module1 == module2, got %p and %p\n", module1, module2);
    }
    ok(entry1 == mark1, "expected entry1 == mark1, got %p and %p\n", entry1, mark1);
    ok(entry2 == mark2, "expected entry2 == mark2, got %p and %p\n", entry2, mark2);
}

static inline WCHAR toupperW(WCHAR c)
{
    WCHAR tmp = c;
    CharUpperBuffW(&tmp, 1);
    return tmp;
}

static ULONG hash_basename(const WCHAR *basename)
{
    WORD version = MAKEWORD(NtCurrentTeb()->Peb->OSMinorVersion,
                            NtCurrentTeb()->Peb->OSMajorVersion);
    ULONG hash = 0;

    if (version >= 0x0602)
    {
        for (; *basename; basename++)
            hash = hash * 65599 + toupperW(*basename);
    }
    else if (version == 0x0601)
    {
        for (; *basename; basename++)
            hash = hash + 65599 * toupperW(*basename);
    }
    else
        hash = toupperW(basename[0]) - 'A';

    return hash & 31;
}

static void test_HashLinks(void)
{
    static WCHAR ntdllW[] = {'n','t','d','l','l','.','d','l','l',0};
    static WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2','.','d','l','l',0};

    LIST_ENTRY *hash_map, *entry, *mark;
    LDR_DATA_TABLE_ENTRY *module;
    BOOL found;

    entry = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    entry = entry->Flink;

    module = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
    entry = module->HashLinks.Blink;

    hash_map = entry - hash_basename(module->BaseDllName.Buffer);

    mark = &hash_map[hash_basename(ntdllW)];
    found = FALSE;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        module = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, HashLinks);
        if (!lstrcmpiW(module->BaseDllName.Buffer, ntdllW))
        {
            found = TRUE;
            break;
        }
    }
    ok(found, "Could not find ntdll\n");

    mark = &hash_map[hash_basename(kernel32W)];
    found = FALSE;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        module = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, HashLinks);
        if (!lstrcmpiW(module->BaseDllName.Buffer, kernel32W))
        {
            found = TRUE;
            break;
        }
    }
    ok(found, "Could not find kernel32\n");
}

START_TEST(loader)
{
    int argc;
    char **argv;
    HANDLE ntdll, mapping, kernel32;
    SYSTEM_INFO si;

    ntdll = GetModuleHandleA("ntdll.dll");
    kernel32 = GetModuleHandleA("kernel32.dll");
    pNtCreateSection = (void *)GetProcAddress(ntdll, "NtCreateSection");
    pNtQuerySection = (void *)GetProcAddress(ntdll, "NtQuerySection");
    pNtMapViewOfSection = (void *)GetProcAddress(ntdll, "NtMapViewOfSection");
    pNtUnmapViewOfSection = (void *)GetProcAddress(ntdll, "NtUnmapViewOfSection");
    pNtTerminateProcess = (void *)GetProcAddress(ntdll, "NtTerminateProcess");
    pNtQueryInformationProcess = (void *)GetProcAddress(ntdll, "NtQueryInformationProcess");
    pNtSetInformationProcess = (void *)GetProcAddress(ntdll, "NtSetInformationProcess");
    pLdrShutdownProcess = (void *)GetProcAddress(ntdll, "LdrShutdownProcess");
    pRtlDllShutdownInProgress = (void *)GetProcAddress(ntdll, "RtlDllShutdownInProgress");
    pNtAllocateVirtualMemory = (void *)GetProcAddress(ntdll, "NtAllocateVirtualMemory");
    pNtFreeVirtualMemory = (void *)GetProcAddress(ntdll, "NtFreeVirtualMemory");
    pLdrLockLoaderLock = (void *)GetProcAddress(ntdll, "LdrLockLoaderLock");
    pLdrUnlockLoaderLock = (void *)GetProcAddress(ntdll, "LdrUnlockLoaderLock");
    pRtlAcquirePebLock = (void *)GetProcAddress(ntdll, "RtlAcquirePebLock");
    pRtlReleasePebLock = (void *)GetProcAddress(ntdll, "RtlReleasePebLock");
    pRtlImageDirectoryEntryToData = (void *)GetProcAddress(ntdll, "RtlImageDirectoryEntryToData");
    pFlsAlloc = (void *)GetProcAddress(kernel32, "FlsAlloc");
    pFlsSetValue = (void *)GetProcAddress(kernel32, "FlsSetValue");
    pFlsGetValue = (void *)GetProcAddress(kernel32, "FlsGetValue");
    pFlsFree = (void *)GetProcAddress(kernel32, "FlsFree");
    pIsWow64Process = (void *)GetProcAddress(kernel32, "IsWow64Process");
    pResolveDelayLoadedAPI = (void *)GetProcAddress(kernel32, "ResolveDelayLoadedAPI");

    if (pIsWow64Process) pIsWow64Process( GetCurrentProcess(), &is_wow64 );
    GetSystemInfo( &si );
    page_size = si.dwPageSize;
    dos_header.e_magic = IMAGE_DOS_SIGNATURE;
    dos_header.e_lfanew = sizeof(dos_header);

    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, "winetest_loader");
    ok(mapping != 0, "CreateFileMapping failed\n");
    child_failures = MapViewOfFile(mapping, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 4096);
    if (*child_failures == -1)
    {
        *child_failures = 0;
    }
    else
        *child_failures = -1;

    argc = winetest_get_mainargs(&argv);
    if (argc > 4)
    {
        test_dll_phase = atoi(argv[4]);
        child_process(argv[2], atol(argv[3]));
        return;
    }

    test_Loader();
    test_FakeDLL();
    test_filenames();
    test_ResolveDelayLoadedAPI();
    test_ImportDescriptors();
    test_section_access();
    test_import_resolution();
    test_ExitProcess();
    test_InMemoryOrderModuleList();
    test_HashLinks();
}
