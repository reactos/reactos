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

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "wine/test.h"
#include "delayloadhandler.h"

/* PROCESS_ALL_ACCESS in Vista+ PSDKs is incompatible with older Windows versions */
#define PROCESS_ALL_ACCESS_NT4 (PROCESS_ALL_ACCESS & ~0xf000)

#define ALIGN_SIZE(size, alignment) (((size) + ((ULONG_PTR)(alignment) - 1)) & ~(((ULONG_PTR)(alignment) - 1)))

struct PROCESS_BASIC_INFORMATION_PRIVATE
{
    NTSTATUS  ExitStatus;
    PPEB      PebBaseAddress;
    DWORD_PTR AffinityMask;
    DWORD_PTR BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
};

static LONG *child_failures;
static WORD cb_count, cb_count_sys;
static DWORD page_size;
static BOOL is_win64 = sizeof(void *) > sizeof(int);
static BOOL is_wow64;
static char system_dir[MAX_PATH];
static char syswow_dir[MAX_PATH]; /* only available if is_wow64 */

static NTSTATUS (WINAPI *pNtCreateSection)(HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES *,
                                           const LARGE_INTEGER *, ULONG, ULONG, HANDLE );
static NTSTATUS (WINAPI *pNtQuerySection)(HANDLE, SECTION_INFORMATION_CLASS, void *, SIZE_T, SIZE_T *);
static NTSTATUS (WINAPI *pNtMapViewOfSection)(HANDLE, HANDLE, PVOID *, ULONG_PTR, SIZE_T, const LARGE_INTEGER *, SIZE_T *, ULONG, ULONG, ULONG);
static NTSTATUS (WINAPI *pNtUnmapViewOfSection)(HANDLE, PVOID);
static NTSTATUS (WINAPI *pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtSetInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);
static NTSTATUS (WINAPI *pNtTerminateProcess)(HANDLE, DWORD);
static void (WINAPI *pLdrShutdownProcess)(void);
static BOOLEAN (WINAPI *pRtlDllShutdownInProgress)(void);
static NTSTATUS (WINAPI *pNtAllocateVirtualMemory)(HANDLE, PVOID *, ULONG_PTR, SIZE_T *, ULONG, ULONG);
static NTSTATUS (WINAPI *pNtFreeVirtualMemory)(HANDLE, PVOID *, SIZE_T *, ULONG);
static NTSTATUS (WINAPI *pLdrLockLoaderLock)(ULONG, ULONG *, ULONG_PTR *);
static NTSTATUS (WINAPI *pLdrUnlockLoaderLock)(ULONG, ULONG_PTR);
static NTSTATUS (WINAPI *pLdrLoadDll)(LPCWSTR,DWORD,const UNICODE_STRING *,HMODULE*);
static NTSTATUS (WINAPI *pLdrUnloadDll)(HMODULE);
static void (WINAPI *pRtlInitUnicodeString)(PUNICODE_STRING,LPCWSTR);
static void (WINAPI *pRtlAcquirePebLock)(void);
static void (WINAPI *pRtlReleasePebLock)(void);
static PVOID    (WINAPI *pResolveDelayLoadedAPI)(PVOID, PCIMAGE_DELAYLOAD_DESCRIPTOR,
                                                 PDELAYLOAD_FAILURE_DLL_CALLBACK,
                                                 PDELAYLOAD_FAILURE_SYSTEM_ROUTINE,
                                                 PIMAGE_THUNK_DATA ThunkAddress,ULONG);
static PVOID (WINAPI *pRtlImageDirectoryEntryToData)(HMODULE,BOOL,WORD,ULONG *);
static PIMAGE_NT_HEADERS (WINAPI *pRtlImageNtHeader)(HMODULE);
static DWORD (WINAPI *pFlsAlloc)(PFLS_CALLBACK_FUNCTION);
static BOOL (WINAPI *pFlsSetValue)(DWORD, PVOID);
static PVOID (WINAPI *pFlsGetValue)(DWORD);
static BOOL (WINAPI *pFlsFree)(DWORD);
static BOOL (WINAPI *pIsWow64Process)(HANDLE,PBOOL);
static BOOL (WINAPI *pWow64DisableWow64FsRedirection)(void **);
static BOOL (WINAPI *pWow64RevertWow64FsRedirection)(void *);
static HMODULE (WINAPI *pLoadPackagedLibrary)(LPCWSTR lpwLibFileName, DWORD Reserved);
static NTSTATUS  (WINAPI *pLdrRegisterDllNotification)(ULONG, PLDR_DLL_NOTIFICATION_FUNCTION, void *, void **);

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
      0x10000000, /* ImageBase */
#else
      0x123450000, /* ImageBase */
#endif
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
      IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | IMAGE_DLLCHARACTERISTICS_NX_COMPAT, /* DllCharacteristics */
      0x100000, /* SizeOfStackReserve */
      0x1000, /* SizeOfStackCommit */
      0x100000, /* SizeOfHeapReserve */
      0x1000, /* SizeOfHeapCommit */
      0, /* LoaderFlags */
      IMAGE_NUMBEROF_DIRECTORY_ENTRIES, /* NumberOfRvaAndSizes */
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

/* return an alternate machine of the same 32/64 bitness */
static WORD get_alt_machine( WORD orig_machine )
{
    switch (orig_machine)
    {
    case IMAGE_FILE_MACHINE_I386:  return IMAGE_FILE_MACHINE_ARMNT;
    case IMAGE_FILE_MACHINE_AMD64: return IMAGE_FILE_MACHINE_ARM64;
    case IMAGE_FILE_MACHINE_ARMNT: return IMAGE_FILE_MACHINE_I386;
    case IMAGE_FILE_MACHINE_ARM64: return IMAGE_FILE_MACHINE_AMD64;
    }
    return 0;
}

/* return the machine of the alternate 32/64 bitness */
static WORD get_alt_bitness_machine( WORD orig_machine )
{
    switch (orig_machine)
    {
    case IMAGE_FILE_MACHINE_I386:  return IMAGE_FILE_MACHINE_AMD64;
    case IMAGE_FILE_MACHINE_AMD64: return IMAGE_FILE_MACHINE_I386;
    case IMAGE_FILE_MACHINE_ARMNT: return IMAGE_FILE_MACHINE_ARM64;
    case IMAGE_FILE_MACHINE_ARM64: return IMAGE_FILE_MACHINE_ARMNT;
    }
    return 0;
}

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
    ok( hfile != INVALID_HANDLE_VALUE, "failed to create %s err %lu\n", dll_name, GetLastError() );
    if (hfile == INVALID_HANDLE_VALUE) return 0;

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, dos_header, dos_size, &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    if (nt_header->FileHeader.SizeOfOptionalHeader)
    {
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &nt_header->OptionalHeader,
                        sizeof(IMAGE_OPTIONAL_HEADER),
                        &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());
        if (nt_header->FileHeader.SizeOfOptionalHeader > sizeof(IMAGE_OPTIONAL_HEADER))
        {
            file_align = nt_header->FileHeader.SizeOfOptionalHeader - sizeof(IMAGE_OPTIONAL_HEADER);
            assert(file_align < sizeof(filler));
            SetLastError(0xdeadbeef);
            ret = WriteFile(hfile, filler, file_align, &dummy, NULL);
            ok(ret, "WriteFile error %ld\n", GetLastError());
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
        ok(ret, "WriteFile error %ld\n", GetLastError());

        /* section data */
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, section_data, sizeof(section_data), &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());
    }

    /* Minimal PE image that Windows7+ is able to load: 268 bytes */
    size = GetFileSize(hfile, NULL);
    if (size < 268)
    {
        file_align = 268 - size;
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, filler, file_align, &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());
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
    ok( hfile != INVALID_HANDLE_VALUE, "failed to create %s err %lu\n", dll_name, GetLastError() );
    if (hfile == INVALID_HANDLE_VALUE) return 0;

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, dos_header, sizeof(*dos_header), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, nt_header, offsetof(IMAGE_NT_HEADERS, OptionalHeader) + nt_header->FileHeader.SizeOfOptionalHeader, &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, sections, sizeof(*sections) * nt_header->FileHeader.NumberOfSections,
                    &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    for (i = 0; i < nt_header->FileHeader.NumberOfSections; i++)
    {
        SetFilePointer(hfile, sections[i].PointerToRawData, NULL, FILE_BEGIN);
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, section_data, sections[i].SizeOfRawData, &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());
    }
    size = GetFileSize(hfile, NULL);
    CloseHandle(hfile);
    return size;
}

static BOOL query_image_section( int id, const char *dll_name, const IMAGE_NT_HEADERS *nt_header,
                                 const void *section_data )
{
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
    BOOL truncated;

    file = CreateFileA( dll_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "%u: CreateFile error %ld\n", id, GetLastError() );
    file_size = GetFileSize( file, NULL );

    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, NULL, PAGE_READONLY, SEC_IMAGE, file );
    ok( !status, "%u: NtCreateSection failed err %lx\n", id, status );
    if (status)
    {
        CloseHandle( file );
        return FALSE;
    }
    status = pNtQuerySection( mapping, SectionImageInformation, &image, sizeof(image), &info_size );
    ok( !status, "%u: NtQuerySection failed err %lx\n", id, status );
    ok( info_size == sizeof(image), "%u: NtQuerySection wrong size %Iu\n", id, info_size );
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
        (image.ImageDynamicallyRelocated && LOWORD(image.TransferAddress) == LOWORD(entry_point)),
        "%u: TransferAddress wrong %p / %p (%08lx)\n", id,
        image.TransferAddress, entry_point, nt_header->OptionalHeader.AddressOfEntryPoint );
    ok( image.ZeroBits == 0, "%u: ZeroBits wrong %08lx\n", id, image.ZeroBits );
    ok( image.MaximumStackSize == max_stack,
        "%u: MaximumStackSize wrong %Ix / %Ix\n", id, image.MaximumStackSize, max_stack );
    ok( image.CommittedStackSize == commit_stack,
        "%u: CommittedStackSize wrong %Ix / %Ix\n", id, image.CommittedStackSize, commit_stack );
    ok( image.SubSystemType == nt_header->OptionalHeader.Subsystem,
        "%u: SubSystemType wrong %08lx / %08x\n", id,
        image.SubSystemType, nt_header->OptionalHeader.Subsystem );
    ok( image.MinorSubsystemVersion == nt_header->OptionalHeader.MinorSubsystemVersion,
        "%u: MinorSubsystemVersion wrong %04x / %04x\n", id,
        image.MinorSubsystemVersion, nt_header->OptionalHeader.MinorSubsystemVersion );
    ok( image.MajorSubsystemVersion == nt_header->OptionalHeader.MajorSubsystemVersion,
        "%u: MajorSubsystemVersion wrong %04x / %04x\n", id,
        image.MajorSubsystemVersion, nt_header->OptionalHeader.MajorSubsystemVersion );
    ok( image.MajorOperatingSystemVersion == nt_header->OptionalHeader.MajorOperatingSystemVersion ||
        broken( !image.MajorOperatingSystemVersion), /* before win10 */
        "%u: MajorOperatingSystemVersion wrong %04x / %04x\n", id,
        image.MajorOperatingSystemVersion, nt_header->OptionalHeader.MajorOperatingSystemVersion );
    ok( image.MinorOperatingSystemVersion == nt_header->OptionalHeader.MinorOperatingSystemVersion,
        "%u: MinorOperatingSystemVersion wrong %04x / %04x\n", id,
        image.MinorOperatingSystemVersion, nt_header->OptionalHeader.MinorOperatingSystemVersion );
    ok( image.ImageCharacteristics == nt_header->FileHeader.Characteristics,
        "%u: ImageCharacteristics wrong %04x / %04x\n", id,
        image.ImageCharacteristics, nt_header->FileHeader.Characteristics );
    ok( image.DllCharacteristics == nt_header->OptionalHeader.DllCharacteristics,
        "%u: DllCharacteristics wrong %04x / %04x\n", id,
        image.DllCharacteristics, nt_header->OptionalHeader.DllCharacteristics );
    ok( image.Machine == nt_header->FileHeader.Machine, "%u: Machine wrong %04x / %04x\n", id,
        image.Machine, nt_header->FileHeader.Machine );
    ok( image.LoaderFlags == (cor_header != NULL), "%u: LoaderFlags wrong %08lx\n", id, image.LoaderFlags );
    ok( image.ImageFileSize == file_size,
        "%u: ImageFileSize wrong %08lx / %08lx\n", id, image.ImageFileSize, file_size );
    ok( image.CheckSum == nt_header->OptionalHeader.CheckSum,
        "%u: CheckSum wrong %08lx / %08lx\n", id,
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
        ok( image.ComPlusILOnly,
            "%u: wrong ComPlusILOnly flags %02x\n", id, image.ImageFlags );
        if (nt_header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
            !(cor_header->Flags & COMIMAGE_FLAGS_32BITREQUIRED))
            ok( image.ComPlusNativeReady,
                "%u: wrong ComPlusNativeReady flags %02x\n", id, image.ImageFlags );
        else
            ok( !image.ComPlusNativeReady,
                "%u: wrong ComPlusNativeReady flags %02x\n", id, image.ImageFlags );
        if (nt_header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
            (cor_header->Flags & COMIMAGE_FLAGS_32BITPREFERRED))
            ok( image.ComPlusPrefer32bit ||
                broken( !image.MajorOperatingSystemVersion ), /* before win10 */
                "%u: wrong ComPlusPrefer32bit flags %02x\n", id, image.ImageFlags );
        else
            ok( !image.ComPlusPrefer32bit, "%u: wrong ComPlusPrefer32bit flags %02x\n", id, image.ImageFlags );
    }
    else
    {
        ok( !image.ComPlusILOnly, "%u: wrong ComPlusILOnly flags %02x\n", id, image.ImageFlags );
        ok( !image.ComPlusNativeReady, "%u: wrong ComPlusNativeReady flags %02x\n", id, image.ImageFlags );
        ok( !image.ComPlusPrefer32bit, "%u: wrong ComPlusPrefer32bit flags %02x\n", id, image.ImageFlags );
    }
    if (!(nt_header->OptionalHeader.SectionAlignment % page_size))
        ok( !image.ImageMappedFlat, "%u: wrong ImageMappedFlat flags %02x\n", id, image.ImageFlags );
    else
        ok( image.ImageMappedFlat,
        "%u: wrong ImageMappedFlat flags %02x\n", id, image.ImageFlags );

    if (!(nt_header->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE))
        ok( !image.ImageDynamicallyRelocated || broken( image.ComPlusILOnly ), /* <= win7 */
            "%u: wrong ImageDynamicallyRelocated flags %02x\n", id, image.ImageFlags );
    else if (image.ImageContainsCode && !image.ImageMappedFlat && !cor_header)
        ok( image.ImageDynamicallyRelocated,
            "%u: wrong ImageDynamicallyRelocated flags %02x\n", id, image.ImageFlags );
    else
        ok( !image.ImageDynamicallyRelocated || broken(TRUE), /* <= win8 */
            "%u: wrong ImageDynamicallyRelocated flags %02x\n", id, image.ImageFlags );
    ok( !image.BaseBelow4gb, "%u: wrong BaseBelow4gb flags %02x\n", id, image.ImageFlags );

    map_size.QuadPart = (nt_header->OptionalHeader.SizeOfImage + page_size - 1) & ~(page_size - 1);
    status = pNtQuerySection( mapping, SectionBasicInformation, &info, sizeof(info), NULL );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( info.Size.QuadPart == map_size.QuadPart, "NtQuerySection wrong size %lx%08lx / %lx%08lx\n",
        info.Size.u.HighPart, info.Size.u.LowPart, map_size.u.HighPart, map_size.u.LowPart );
    CloseHandle( mapping );

    map_size.QuadPart = (nt_header->OptionalHeader.SizeOfImage + page_size - 1) & ~(page_size - 1);
    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, &map_size, PAGE_READONLY, SEC_IMAGE, file );
    ok( !status, "%u: NtCreateSection failed err %lx\n", id, status );
    status = pNtQuerySection( mapping, SectionBasicInformation, &info, sizeof(info), NULL );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( info.Size.QuadPart == map_size.QuadPart, "NtQuerySection wrong size %lx%08lx / %lx%08lx\n",
        info.Size.u.HighPart, info.Size.u.LowPart, map_size.u.HighPart, map_size.u.LowPart );
    CloseHandle( mapping );

    map_size.QuadPart++;
    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, &map_size, PAGE_READONLY, SEC_IMAGE, file );
    ok( status == STATUS_SECTION_TOO_BIG, "%u: NtCreateSection failed err %lx\n", id, status );

    SetFilePointerEx( file, map_size, NULL, FILE_BEGIN );
    SetEndOfFile( file );
    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, &map_size, PAGE_READONLY, SEC_IMAGE, file );
    ok( status == STATUS_SECTION_TOO_BIG, "%u: NtCreateSection failed err %lx\n", id, status );

    map_size.QuadPart = 1;
    status = pNtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                               NULL, &map_size, PAGE_READONLY, SEC_IMAGE, file );
    ok( !status, "%u: NtCreateSection failed err %lx\n", id, status );
    status = pNtQuerySection( mapping, SectionBasicInformation, &info, sizeof(info), NULL );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( info.Size.QuadPart == map_size.QuadPart, "NtQuerySection wrong size %lx%08lx / %lx%08lx\n",
        info.Size.u.HighPart, info.Size.u.LowPart, map_size.u.HighPart, map_size.u.LowPart );
    CloseHandle( mapping );

    CloseHandle( file );
    return image.ImageContainsCode && (!cor_header || !(cor_header->Flags & COMIMAGE_FLAGS_ILONLY));
}

static const WCHAR wldr_nameW[] = {'w','l','d','r','t','e','s','t','.','d','l','l',0};
static WCHAR load_test_name[MAX_PATH], load_fallback_name[MAX_PATH];
static WCHAR load_path[MAX_PATH];

static void init_load_path( const char *fallback_dll )
{
    static const WCHAR pathW[] = {'P','A','T','H',0};
    static const WCHAR ldrW[] = {'l','d','r',0};
    static const WCHAR sepW[] = {';',0};
    static const WCHAR bsW[] = {'\\',0};
    WCHAR path[MAX_PATH];

    GetTempPathW( MAX_PATH, path );
    GetTempFileNameW( path, ldrW, 0, load_test_name );
    GetTempFileNameW( path, ldrW, 0, load_fallback_name );
    DeleteFileW( load_test_name );
    ok( CreateDirectoryW( load_test_name, NULL ), "failed to create dir\n" );
    DeleteFileW( load_fallback_name );
    ok( CreateDirectoryW( load_fallback_name, NULL ), "failed to create dir\n" );
    lstrcpyW( load_path, load_test_name );
    lstrcatW( load_path, sepW );
    lstrcatW( load_path, load_fallback_name );
    lstrcatW( load_path, sepW );
    GetEnvironmentVariableW( pathW, load_path + lstrlenW(load_path),
                             ARRAY_SIZE(load_path) - lstrlenW(load_path) );
    lstrcatW( load_test_name, bsW );
    lstrcatW( load_test_name, wldr_nameW );
    lstrcatW( load_fallback_name, bsW );
    lstrcatW( load_fallback_name, wldr_nameW );
    MultiByteToWideChar( CP_ACP, 0, fallback_dll, -1, path, MAX_PATH );
    MoveFileW( path, load_fallback_name );
}

static void delete_load_path(void)
{
    WCHAR *p;

    DeleteFileW( load_test_name );
    for (p = load_test_name + lstrlenW(load_test_name) - 1; *p != '\\'; p--) ;
    *p = 0;
    RemoveDirectoryW( load_test_name );
    DeleteFileW( load_fallback_name );
    for (p = load_fallback_name + lstrlenW(load_fallback_name) - 1; *p != '\\'; p--) ;
    *p = 0;
    RemoveDirectoryW( load_fallback_name );
}

static UINT get_com_dir_size( const IMAGE_NT_HEADERS *nt )
{
    if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        return ((const IMAGE_NT_HEADERS32 *)nt)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;
    else if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return ((const IMAGE_NT_HEADERS64 *)nt)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;
    else
        return 0;
}

/* helper to test image section mapping */
static NTSTATUS map_image_section( const IMAGE_NT_HEADERS *nt_header, const IMAGE_SECTION_HEADER *sections,
                                   const void *section_data, int line )
{
    char dll_name[MAX_PATH];
    WCHAR path[MAX_PATH];
    UNICODE_STRING name;
    LARGE_INTEGER size;
    HANDLE file, map;
    NTSTATUS status, expect_status, ldr_status;
    ULONG file_size;
    BOOL has_code = FALSE, il_only = FALSE, want_32bit = FALSE, expect_fallback = FALSE, wrong_machine = FALSE;
    HMODULE mod = 0, ldr_mod;

    file_size = create_test_dll_sections( &dos_header, nt_header, sections, section_data, dll_name );

    file = CreateFileA(dll_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    size.QuadPart = file_size;
    status = pNtCreateSection(&map, STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                              NULL, &size, PAGE_READONLY, SEC_IMAGE, file );
    expect_status = status;

    if (get_com_dir_size( nt_header ))
    {
        /* invalid COR20 header seems to corrupt internal loader state on Windows */
        if (get_com_dir_size( nt_header ) < sizeof(IMAGE_COR20_HEADER)) goto done;
        if (!((const IMAGE_COR20_HEADER *)section_data)->Flags) goto done;
    }

    if (!status)
    {
        SECTION_BASIC_INFORMATION info;
        SIZE_T info_size = 0xdeadbeef;
        NTSTATUS ret = pNtQuerySection( map, SectionBasicInformation, &info, sizeof(info), &info_size );
        ok( !ret, "NtQuerySection failed err %lx\n", ret );
        ok( info_size == sizeof(info), "NtQuerySection wrong size %Iu\n", info_size );
        ok( info.Attributes == (SEC_IMAGE | SEC_FILE), "NtQuerySection wrong attr %lx\n", info.Attributes );
        ok( info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", info.BaseAddress );
        ok( info.Size.QuadPart == file_size, "NtQuerySection wrong size %lx%08lx / %08lx\n",
            info.Size.u.HighPart, info.Size.u.LowPart, file_size );
        has_code = query_image_section( line, dll_name, nt_header, section_data );

        if (get_com_dir_size( nt_header ))
        {
            const IMAGE_COR20_HEADER *cor_header = section_data;
            il_only = (cor_header->Flags & COMIMAGE_FLAGS_ILONLY) != 0;
            if (il_only) want_32bit = (cor_header->Flags & COMIMAGE_FLAGS_32BITREQUIRED) != 0;
        }

        SetLastError( 0xdeadbeef );
        mod = LoadLibraryExA( dll_name, 0, DONT_RESOLVE_DLL_REFERENCES );
        /* test loading dll of wrong 32/64 bitness */
        if (nt_header->OptionalHeader.Magic == (is_win64 ? IMAGE_NT_OPTIONAL_HDR32_MAGIC
                                                         : IMAGE_NT_OPTIONAL_HDR64_MAGIC))
        {
            if (!has_code && is_win64)
            {
                ok_(__FILE__,line)( mod != NULL || want_32bit || broken(il_only), /* <= win7 */
                    "loading failed err %lu\n", GetLastError() );
            }
            else
            {
                ok_(__FILE__, line)( !mod, "loading succeeded\n" );
                ok_(__FILE__, line)( GetLastError() == ERROR_BAD_EXE_FORMAT, "wrong error %lu\n", GetLastError() );
                if (nt_header_template.FileHeader.Machine == IMAGE_FILE_MACHINE_ARM64)
                {
                    wrong_machine = TRUE;
                    expect_status = STATUS_INVALID_IMAGE_FORMAT;
                }
            }
        }
        else
        {
            wrong_machine = (nt_header->FileHeader.Machine == get_alt_machine( nt_header_template.FileHeader.Machine ));

            ok( mod != NULL || broken(il_only) || /* <= win7 */
                broken( wrong_machine ), /* win8 */
                "%u: loading failed err %lu\n", line, GetLastError() );
        }
        if (mod) FreeLibrary( mod );
        expect_fallback = !mod;
    }

    /* test fallback to another dll further in the load path */

    MultiByteToWideChar( CP_ACP, 0, dll_name, -1, path, MAX_PATH );
    CopyFileW( path, load_test_name, FALSE );
    pRtlInitUnicodeString( &name, wldr_nameW );
    ldr_status = pLdrLoadDll( load_path, 0, &name, &ldr_mod );
    if (!ldr_status)
    {
        GetModuleFileNameW( ldr_mod, path, MAX_PATH );
        if (!lstrcmpiW( path, load_test_name ))
        {
            if (!expect_status)
                ok( !expect_fallback, "%u: got test dll but expected fallback\n", line );
            else
                ok( !expect_fallback, "%u: got test dll but expected failure %lx\n", line, expect_status );
        }
        else if (!lstrcmpiW( path, load_fallback_name ))
        {
            trace( "%u: loaded fallback\n", line );
            ok( !expect_status, "%u: got fallback but expected failure %lx\n", line, expect_status );
            ok( expect_fallback ||
                /* win10 also falls back for 32-bit dll without code, even though it could be loaded */
                (is_win64 && !has_code &&
                 nt_header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC),
                "%u: got fallback but expected test dll\n", line );
        }
        else ok( 0, "%u: got unexpected path %s instead of %s\n", line, wine_dbgstr_w(path), wine_dbgstr_w(load_test_name));
        pLdrUnloadDll( ldr_mod );
    }
    else if (ldr_status == STATUS_DLL_INIT_FAILED ||
             ldr_status == STATUS_ACCESS_VIOLATION ||
             ldr_status == STATUS_ILLEGAL_INSTRUCTION)
    {
        /* some dlls with invalid entry point will crash, but this means we loaded the test dll */
        ok( !expect_fallback, "%u: got test dll but expected fallback\n", line );
    }
    else
    {
        ok( ldr_status == expect_status ||
            (wrong_machine && !expect_status && ldr_status == STATUS_INVALID_IMAGE_FORMAT) ||
            broken(il_only && !expect_status && ldr_status == STATUS_INVALID_IMAGE_FORMAT) ||
            broken(nt_header->Signature == IMAGE_OS2_SIGNATURE && ldr_status == STATUS_INVALID_IMAGE_NE_FORMAT),
            "%u: wrong status %lx/%lx\n", line, ldr_status, expect_status );
        ok( !expect_fallback || wrong_machine || broken(il_only),
            "%u: failed with %lx expected fallback\n", line, ldr_status );
    }

done:
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
          { ERROR_SUCCESS, ERROR_INVALID_ADDRESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
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
        /* Minimal PE image initially created for Windows 7 and accepted from
         * Vista up to Windows 10 1709 with some unexplained exceptions:
         * 268 bytes
         */
        { 0x04,
          0, 0xf0, /* optional header size just forces 0xf0 bytes to be written,
                      0 or another number don't change the behaviour, what really
                      matters is file size regardless of values in the headers */
          0x04 /* also serves as e_lfanew in the truncated MZ header */, 0x04,
          0x40, /* minimal image size that Windows7 accepts */
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* rejected by win10 1809+ */
        },
        /* the following data mimics the PE image which 8k demos have */
        { 0x04,
          0, 0x08,
          0x04 /* also serves as e_lfanew in the truncated MZ header */, 0x04,
          0x2000,
          0x40,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT }
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
    WORD orig_machine = nt_header_template.FileHeader.Machine;
    IMAGE_NT_HEADERS nt_header;
    IMAGE_COR20_HEADER cor_header;

    /* prevent displaying of the "Unable to load this DLL" message box */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        winetest_push_context("%d", i);
        nt_header = nt_header_template;
        nt_header.FileHeader.NumberOfSections = td[i].number_of_sections;
        nt_header.FileHeader.SizeOfOptionalHeader = td[i].size_of_optional_header;

        nt_header.OptionalHeader.SectionAlignment = td[i].section_alignment;
        nt_header.OptionalHeader.FileAlignment = td[i].file_alignment;
        nt_header.OptionalHeader.SizeOfImage = td[i].size_of_image;
        nt_header.OptionalHeader.SizeOfHeaders = td[i].size_of_headers;

        section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
        file_size = create_test_dll( &dos_header, td[i].size_of_dos_header, &nt_header, dll_name );

        SetLastError(0xdeadbeef);
        hlib = LoadLibraryA(dll_name);
        if (hlib)
        {
            MEMORY_BASIC_INFORMATION info;
            void *ptr;

            ok( td[i].errors[0] == ERROR_SUCCESS, "should have failed\n" );

            SetLastError(0xdeadbeef);
            size = VirtualQuery(hlib, &info, sizeof(info));
            ok(size == sizeof(info),
                "%d: VirtualQuery error %ld\n", i, GetLastError());
            ok(info.BaseAddress == hlib, "%p != %p\n", info.BaseAddress, hlib);
            ok(info.AllocationBase == hlib, "%p != %p\n", info.AllocationBase, hlib);
            ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%lx != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
            ok(info.RegionSize == ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size), "got %Ix != expected %x\n",
               info.RegionSize, (UINT)ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size));
            ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
            if (nt_header.OptionalHeader.SectionAlignment < page_size)
                ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%lx != PAGE_EXECUTE_WRITECOPY\n", info.Protect);
            else
                ok(info.Protect == PAGE_READONLY, "%lx != PAGE_READONLY\n", info.Protect);
            ok(info.Type == SEC_IMAGE, "%lx != SEC_IMAGE\n", info.Type);

            SetLastError(0xdeadbeef);
            ptr = VirtualAlloc(hlib, page_size, MEM_COMMIT, info.Protect);
            ok(!ptr, "VirtualAlloc should fail\n");
            ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());

            SetLastError(0xdeadbeef);
            size = VirtualQuery((char *)hlib + info.RegionSize, &info, sizeof(info));
            ok(size == sizeof(info), "VirtualQuery error %ld\n", GetLastError());
            if (nt_header.OptionalHeader.SectionAlignment == page_size ||
                nt_header.OptionalHeader.SectionAlignment == nt_header.OptionalHeader.FileAlignment)
            {
                ok(info.BaseAddress == (char *)hlib + ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size), "got %p != expected %p\n",
                   info.BaseAddress, (char *)hlib + ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size));
                ok(info.AllocationBase == 0, "%p != 0\n", info.AllocationBase);
                ok(info.AllocationProtect == 0, "%lx != 0\n", info.AllocationProtect);
                /*ok(info.RegionSize == not_practical_value, "%d: %lx != not_practical_value\n", i, info.RegionSize);*/
                ok(info.State == MEM_FREE, "%lx != MEM_FREE\n", info.State);
                ok(info.Type == 0, "%lx != 0\n", info.Type);
                ok(info.Protect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.Protect);
            }
            else
            {
                ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%lx != PAGE_EXECUTE_WRITECOPY\n", info.Protect);
                ok(info.BaseAddress == hlib, "got %p != expected %p\n", info.BaseAddress, hlib);
                ok(info.AllocationBase == hlib, "%p != %p\n", info.AllocationBase, hlib);
                ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%lx != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
                ok(info.RegionSize == ALIGN_SIZE(file_size, page_size), "got %Ix != expected %x\n",
                   info.RegionSize, (UINT)ALIGN_SIZE(file_size, page_size));
                ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
                ok(info.Protect == PAGE_READONLY, "%lx != PAGE_READONLY\n", info.Protect);
                ok(info.Type == SEC_IMAGE, "%lx != SEC_IMAGE\n", info.Type);
            }

            /* header: check the zeroing of alignment */
            if (nt_header.OptionalHeader.SectionAlignment >= page_size)
            {
                const char *start;

                start = (const char *)hlib + nt_header.OptionalHeader.SizeOfHeaders;
                size = ALIGN_SIZE((ULONG_PTR)start, page_size) - (ULONG_PTR)start;
                ok(!memcmp(start, filler, size), "header alignment is not cleared\n");
            }

            if (nt_header.FileHeader.NumberOfSections)
            {
                SetLastError(0xdeadbeef);
                size = VirtualQuery((char *)hlib + section.VirtualAddress, &info, sizeof(info));
                ok(size == sizeof(info),
                    "VirtualQuery error %ld\n", GetLastError());
                if (nt_header.OptionalHeader.SectionAlignment < page_size)
                {
                    ok(info.BaseAddress == hlib, "got %p != expected %p\n", info.BaseAddress, hlib);
                    ok(info.RegionSize == ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size), "got %Ix != expected %x\n",
                       info.RegionSize, (UINT)ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, page_size));
                    ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%lx != PAGE_EXECUTE_WRITECOPY\n", info.Protect);
                }
                else
                {
                    ok(info.BaseAddress == (char *)hlib + section.VirtualAddress, "got %p != expected %p\n", info.BaseAddress, (char *)hlib + section.VirtualAddress);
                    ok(info.RegionSize == ALIGN_SIZE(section.Misc.VirtualSize, page_size), "got %Ix != expected %x\n",
                       info.RegionSize, (UINT)ALIGN_SIZE(section.Misc.VirtualSize, page_size));
                    ok(info.Protect == PAGE_READONLY, "%lx != PAGE_READONLY\n", info.Protect);
                }
                ok(info.AllocationBase == hlib, "%p != %p\n", info.AllocationBase, hlib);
                ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%lx != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
                ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
                ok(info.Type == SEC_IMAGE, "%lx != SEC_IMAGE\n", info.Type);

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
                    ok(memcmp(start, filler, size), "alignment should not be cleared\n");
                }

                SetLastError(0xdeadbeef);
                ptr = VirtualAlloc((char *)hlib + section.VirtualAddress, page_size, MEM_COMMIT, info.Protect);
                ok(!ptr, "VirtualAlloc should fail\n");
                ok(GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_INVALID_ADDRESS,
                   "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
            }

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryExA(dll_name, 0, LOAD_LIBRARY_AS_DATAFILE);
            ok(hlib_as_data_file != 0, "LoadLibraryEx error %lu\n", GetLastError());
            ok(hlib_as_data_file == hlib, "hlib_as_file and hlib are different\n");

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib);
            ok(ret, "FreeLibrary error %ld\n", GetLastError());

            SetLastError(0xdeadbeef);
            hlib = GetModuleHandleA(dll_name);
            ok(hlib != 0, "GetModuleHandle error %lu\n", GetLastError());

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib_as_data_file);
            ok(ret, "FreeLibrary error %ld\n", GetLastError());

            hlib = GetModuleHandleA(dll_name);
            ok(!hlib, "GetModuleHandle should fail\n");

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryExA(dll_name, 0, LOAD_LIBRARY_AS_DATAFILE);
            ok(hlib_as_data_file != 0, "LoadLibraryEx error %lu\n", GetLastError());
            ok(((ULONG_PTR)hlib_as_data_file & 3) == 1, "hlib_as_data_file got %p\n", hlib_as_data_file);

            hlib = GetModuleHandleA(dll_name);
            ok(!hlib, "GetModuleHandle should fail\n");

            SetLastError(0xdeadbeef);
            h = CreateFileA( dll_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
            ok( h != INVALID_HANDLE_VALUE, "open failed err %lu\n", GetLastError() );
            CloseHandle( h );

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib_as_data_file);
            ok(ret, "FreeLibrary error %ld\n", GetLastError());

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryExA(dll_name, 0, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);
            ok(hlib_as_data_file != 0, "LoadLibraryEx error %lu\n", GetLastError());

            SetLastError(0xdeadbeef);
            h = CreateFileA( dll_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
            ok( h == INVALID_HANDLE_VALUE, "open succeeded\n" );
            ok( GetLastError() == ERROR_SHARING_VIOLATION, "wrong error %lu\n", GetLastError() );
            CloseHandle( h );

            SetLastError(0xdeadbeef);
            h = CreateFileA( dll_name, GENERIC_READ | DELETE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
            ok( h != INVALID_HANDLE_VALUE, "open failed err %lu\n", GetLastError() );
            CloseHandle( h );

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib_as_data_file);
            ok(ret, "FreeLibrary error %ld\n", GetLastError());

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryExA(dll_name, 0, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
            ok(hlib_as_data_file != 0, "LoadLibraryEx error %lu\n", GetLastError());
            ok(((ULONG_PTR)hlib_as_data_file & 3) == 2, "hlib_as_data_file got %p\n",
               hlib_as_data_file);

            hlib = GetModuleHandleA(dll_name);
            ok(!hlib, "GetModuleHandle should fail\n");

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib_as_data_file);
            ok(ret, "FreeLibrary error %ld\n", GetLastError());

            SetLastError(0xdeadbeef);
            ret = DeleteFileA(dll_name);
            ok(ret, "DeleteFile error %ld\n", GetLastError());

            nt_header.OptionalHeader.AddressOfEntryPoint = 0x12345678;
            file_size = create_test_dll( &dos_header, td[i].size_of_dos_header, &nt_header, dll_name );
            if (!file_size)
            {
                ok(0, "could not create %s\n", dll_name);
                winetest_pop_context();
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
                 ! error_match && error_index < ARRAY_SIZE(td[i].errors);
                 error_index++)
            {
                error_match = td[i].errors[error_index] == GetLastError();
            }
            ok(error_match, "unexpected error %ld\n", GetLastError());
        }

        SetLastError(0xdeadbeef);
        ret = DeleteFileA(dll_name);
        ok(ret, "DeleteFile error %ld\n", GetLastError());
        winetest_pop_context();
    }

    nt_header = nt_header_template;
    nt_header.FileHeader.NumberOfSections = 1;
    nt_header.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);

    nt_header.OptionalHeader.SectionAlignment = page_size;
    nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
    nt_header.OptionalHeader.FileAlignment = page_size;
    nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER);
    nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + page_size;

    section.SizeOfRawData = sizeof(section_data);
    section.PointerToRawData = page_size;
    section.VirtualAddress = page_size;
    section.Misc.VirtualSize = page_size;

    create_test_dll_sections( &dos_header, &nt_header, &section, section_data, dll_name );
    init_load_path( dll_name );

    nt_header.OptionalHeader.AddressOfEntryPoint = 0x1234;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == (nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_ARM64 ? STATUS_INVALID_IMAGE_FORMAT : STATUS_SUCCESS),
        "NtCreateSection error %08lx\n", status );

    nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == (nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_ARM64 ? STATUS_INVALID_IMAGE_FORMAT : STATUS_SUCCESS),
        "NtCreateSection error %08lx\n", status );

    nt_header.OptionalHeader.SizeOfCode = 0x1000;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == (nt_header.FileHeader.Machine == IMAGE_FILE_MACHINE_ARM64 ? STATUS_INVALID_IMAGE_FORMAT : STATUS_SUCCESS),
        "NtCreateSection error %08lx\n", status );

    nt_header.OptionalHeader.SizeOfCode = 0;
    nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;

    dos_header.e_magic = 0;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_NOT_MZ, "NtCreateSection error %08lx\n", status );

    dos_header.e_magic = IMAGE_DOS_SIGNATURE;
    nt_header.Signature = IMAGE_OS2_SIGNATURE;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_NE_FORMAT, "NtCreateSection error %08lx\n", status );
    for (i = 0; i < 16; i++)
    {
        ((IMAGE_OS2_HEADER *)&nt_header)->ne_exetyp = i;
        status = map_image_section( &nt_header, &section, section_data, __LINE__ );
        switch (i)
        {
        case 2:
            ok( status == STATUS_INVALID_IMAGE_WIN_16, "NtCreateSection %u error %08lx\n", i, status );
            break;
        case 5:
            ok( status == STATUS_INVALID_IMAGE_PROTECT, "NtCreateSection %u error %08lx\n", i, status );
            break;
        default:
            ok( status == STATUS_INVALID_IMAGE_NE_FORMAT, "NtCreateSection %u error %08lx\n", i, status );
            break;
        }
    }
    ((IMAGE_OS2_HEADER *)&nt_header)->ne_exetyp = ((IMAGE_OS2_HEADER *)&nt_header_template)->ne_exetyp;

    dos_header.e_lfanew = 0x98760000;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_PROTECT, "NtCreateSection error %08lx\n", status );

    dos_header.e_lfanew = sizeof(dos_header);
    nt_header.Signature = 0xdeadbeef;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_PROTECT, "NtCreateSection error %08lx\n", status );

    nt_header.Signature = IMAGE_NT_SIGNATURE;
    nt_header.OptionalHeader.Magic = 0xdead;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_FORMAT, "NtCreateSection error %08lx\n", status );

    nt_header.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
    nt_header.FileHeader.Machine = 0xdead;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_FORMAT, "NtCreateSection error %08lx\n", status );

    nt_header.FileHeader.Machine = IMAGE_FILE_MACHINE_UNKNOWN;
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_FORMAT, "NtCreateSection error %08lx\n", status );

    nt_header.FileHeader.Machine = get_alt_bitness_machine( orig_machine );
    status = map_image_section( &nt_header, &section, section_data, __LINE__ );
    ok( status == STATUS_INVALID_IMAGE_FORMAT, "NtCreateSection error %08lx\n", status );

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
    cor_header.EntryPointToken = 0xbeef;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

    cor_header.MinorRuntimeVersion = 5;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

    cor_header.MajorRuntimeVersion = 3;
    cor_header.MinorRuntimeVersion = 0;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

    cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITREQUIRED;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

    cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITPREFERRED;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

    cor_header.Flags = 0;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = 1;
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = 1;
    status = map_image_section( &nt_header, &section, &cor_header, __LINE__ );
    ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

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
            "NtCreateSection error %08lx\n", status );

        switch (orig_machine)
        {
        case IMAGE_FILE_MACHINE_I386: nt64.FileHeader.Machine = IMAGE_FILE_MACHINE_ARM64; break;
        case IMAGE_FILE_MACHINE_ARMNT: nt64.FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64; break;
        }
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_INVALID_IMAGE_FORMAT : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        nt64.FileHeader.Machine = get_alt_bitness_machine( orig_machine );
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        nt64.OptionalHeader.SizeOfCode = 0;
        nt64.OptionalHeader.AddressOfEntryPoint = 0x1000;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        nt64.OptionalHeader.SizeOfCode = 0;
        nt64.OptionalHeader.AddressOfEntryPoint = 0;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        nt64.OptionalHeader.SizeOfCode = 0x1000;
        nt64.OptionalHeader.AddressOfEntryPoint = 0;
        nt64.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        nt64.OptionalHeader.SizeOfCode = 0;
        nt64.OptionalHeader.AddressOfEntryPoint = 0;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, section_data, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        nt64.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        nt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = page_size;
        nt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = sizeof(cor_header);
        cor_header.MajorRuntimeVersion = 2;
        cor_header.MinorRuntimeVersion = 4;
        cor_header.Flags = COMIMAGE_FLAGS_ILONLY;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        nt64.OptionalHeader.SizeOfCode = 0x1000;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        cor_header.MinorRuntimeVersion = 5;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITREQUIRED;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITPREFERRED;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        cor_header.Flags = 0;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );

        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = 1;
        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = 1;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt64, &section, &cor_header, __LINE__ );
        ok( status == (is_wow64 ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_WIN_64),
            "NtCreateSection error %08lx\n", status );
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
        ok( status == STATUS_INVALID_IMAGE_FORMAT, "NtCreateSection error %08lx\n", status );

        if (orig_machine == IMAGE_FILE_MACHINE_AMD64)
        {
            nt32.FileHeader.Machine = IMAGE_FILE_MACHINE_ARMNT;
            status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
            ok( status == STATUS_INVALID_IMAGE_FORMAT || broken(!status) /* win8 */,
                "NtCreateSection error %08lx\n", status );
        }

        nt32.FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        nt32.OptionalHeader.SizeOfCode = 0;
        nt32.OptionalHeader.AddressOfEntryPoint = 0x1000;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        nt32.OptionalHeader.SizeOfCode = 0;
        nt32.OptionalHeader.AddressOfEntryPoint = 0;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        nt32.OptionalHeader.SizeOfCode = 0x1000;
        nt32.OptionalHeader.AddressOfEntryPoint = 0;
        nt32.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        nt32.OptionalHeader.SizeOfCode = 0;
        nt32.OptionalHeader.AddressOfEntryPoint = 0;
        section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, section_data, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        nt32.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        nt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = page_size;
        nt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = sizeof(cor_header);
        cor_header.MajorRuntimeVersion = 2;
        cor_header.MinorRuntimeVersion = 4;
        cor_header.Flags = COMIMAGE_FLAGS_ILONLY;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        nt32.OptionalHeader.SizeOfCode = 0x1000;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        cor_header.MinorRuntimeVersion = 5;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITREQUIRED;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        cor_header.Flags = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITPREFERRED;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        cor_header.Flags = 0;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );

        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = 1;
        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = 1;
        status = map_image_section( (IMAGE_NT_HEADERS *)&nt32, &section, &cor_header, __LINE__ );
        ok( status == STATUS_SUCCESS, "NtCreateSection error %08lx\n", status );
    }

    section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
    delete_load_path();
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
    nt_header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
    nt_header.OptionalHeader.FileAlignment = page_size;
    nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER);
    nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + page_size;

    create_test_dll( &dos_header, sizeof(dos_header), &nt_header, dll_name );
    strcpy( long_path, dll_name );
    strcpy( strrchr( long_path, '\\' ), "\\this-is-a-long-name.dll" );
    ret = MoveFileA( dll_name, long_path );
    ok( ret, "MoveFileA failed err %lu\n", GetLastError() );
    GetShortPathNameA( long_path, short_path, MAX_PATH );

    mod = LoadLibraryA( short_path );
    ok( mod != NULL, "loading failed err %lu\n", GetLastError() );
    GetModuleFileNameA( mod, buffer, MAX_PATH );
    ok( !lstrcmpiA( buffer, short_path ), "got wrong path %s / %s\n", buffer, short_path );
    mod2 = GetModuleHandleA( short_path );
    ok( mod == mod2, "wrong module %p for %s\n", mod2, short_path );
    mod2 = GetModuleHandleA( long_path );
    ok( mod == mod2, "wrong module %p for %s\n", mod2, long_path );
    mod2 = LoadLibraryA( long_path );
    ok( mod2 != NULL, "loading failed err %lu\n", GetLastError() );
    ok( mod == mod2, "library loaded twice\n" );
    GetModuleFileNameA( mod2, buffer, MAX_PATH );
    ok( !lstrcmpiA( buffer, short_path ), "got wrong path %s / %s\n", buffer, short_path );
    FreeLibrary( mod2 );
    FreeLibrary( mod );

    mod = LoadLibraryA( long_path );
    ok( mod != NULL, "loading failed err %lu\n", GetLastError() );
    GetModuleFileNameA( mod, buffer, MAX_PATH );
    ok( !lstrcmpiA( buffer, long_path ), "got wrong path %s / %s\n", buffer, long_path );
    mod2 = GetModuleHandleA( short_path );
    ok( mod == mod2, "wrong module %p for %s\n", mod2, short_path );
    mod2 = GetModuleHandleA( long_path );
    ok( mod == mod2, "wrong module %p for %s\n", mod2, long_path );
    mod2 = LoadLibraryA( short_path );
    ok( mod2 != NULL, "loading failed err %lu\n", GetLastError() );
    ok( mod == mod2, "library loaded twice\n" );
    GetModuleFileNameA( mod2, buffer, MAX_PATH );
    ok( !lstrcmpiA( buffer, long_path ), "got wrong path %s / %s\n", buffer, long_path );
    FreeLibrary( mod2 );
    FreeLibrary( mod );

    strcpy( dll_name, long_path );
    strcpy( strrchr( dll_name, '\\' ), "\\this-is-another-name.dll" );
    ret = CreateHardLinkA( dll_name, long_path, NULL );
    ok( ret, "CreateHardLinkA failed err %lu\n", GetLastError() );
    if (ret)
    {
        mod = LoadLibraryA( dll_name );
        ok( mod != NULL, "loading failed err %lu\n", GetLastError() );
        GetModuleFileNameA( mod, buffer, MAX_PATH );
        ok( !lstrcmpiA( buffer, dll_name ), "got wrong path %s / %s\n", buffer, dll_name );
        mod2 = GetModuleHandleA( long_path );
        ok( mod == mod2, "wrong module %p for %s\n", mod2, long_path );
        mod2 = LoadLibraryA( long_path );
        ok( mod2 != NULL, "loading failed err %lu\n", GetLastError() );
        ok( mod == mod2, "library loaded twice\n" );
        GetModuleFileNameA( mod2, buffer, MAX_PATH );
        ok( !lstrcmpiA( buffer, dll_name ), "got wrong path %s / %s\n", buffer, short_path );
        FreeLibrary( mod2 );
        FreeLibrary( mod );
        DeleteFileA( dll_name );
    }
    DeleteFileA( long_path );
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
                import_chunk->OriginalFirstThunk, kernel32_module);
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
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    hmap = CreateFileMappingW(hfile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, 0);
    ok(hmap != 0, "CreateFileMapping error %ld\n", GetLastError());

    offset.u.LowPart  = 0;
    offset.u.HighPart = 0;

    addr1 = NULL;
    size = 0;
    status = pNtMapViewOfSection(hmap, GetCurrentProcess(), &addr1, 0, 0, &offset,
                                 &size, 1 /* ViewShare */, 0, PAGE_READONLY);
    ok(NT_SUCCESS(status), "NtMapViewOfSection error %lx\n", status);
    ok(addr1 != 0, "mapped address should be valid\n");

    SetLastError(0xdeadbeef);
    size = VirtualQuery((char *)addr1 + section.VirtualAddress, &info, sizeof(info));
    ok(size == sizeof(info), "VirtualQuery error %ld\n", GetLastError());
    ok(info.BaseAddress == (char *)addr1 + section.VirtualAddress, "got %p != expected %p\n", info.BaseAddress, (char *)addr1 + section.VirtualAddress);
    ok(info.RegionSize == page_size, "got %#Ix != expected %#lx\n", info.RegionSize, page_size);
    ok(info.Protect == scn_page_access, "got %#lx != expected %#lx\n", info.Protect, scn_page_access);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%#lx != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#lx != MEM_COMMIT\n", info.State);
    ok(info.Type == SEC_IMAGE, "%#lx != SEC_IMAGE\n", info.Type);

    addr2 = NULL;
    size = 0;
    status = pNtMapViewOfSection(hmap, GetCurrentProcess(), &addr2, 0, 0, &offset,
                                 &size, 1 /* ViewShare */, 0, PAGE_READONLY);
    ok(status == STATUS_IMAGE_NOT_AT_BASE, "expected STATUS_IMAGE_NOT_AT_BASE, got %lx\n", status);
    ok(addr2 != 0, "mapped address should be valid\n");
    ok(addr2 != addr1, "mapped addresses should be different\n");

    SetLastError(0xdeadbeef);
    size = VirtualQuery((char *)addr2 + section.VirtualAddress, &info, sizeof(info));
    ok(size == sizeof(info), "VirtualQuery error %ld\n", GetLastError());
    ok(info.BaseAddress == (char *)addr2 + section.VirtualAddress, "got %p != expected %p\n", info.BaseAddress, (char *)addr2 + section.VirtualAddress);
    ok(info.RegionSize == page_size, "got %#Ix != expected %#lx\n", info.RegionSize, page_size);
    ok(info.Protect == scn_page_access, "got %#lx != expected %#lx\n", info.Protect, scn_page_access);
    ok(info.AllocationBase == addr2, "%p != %p\n", info.AllocationBase, addr2);
    ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%#lx != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#lx != MEM_COMMIT\n", info.State);
    ok(info.Type == SEC_IMAGE, "%#lx != SEC_IMAGE\n", info.Type);

    status = pNtUnmapViewOfSection(GetCurrentProcess(), addr2);
    ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection error %lx\n", status);

    addr2 = MapViewOfFile(hmap, 0, 0, 0, 0);
    ok(addr2 != 0, "mapped address should be valid\n");
    ok(addr2 != addr1, "mapped addresses should be different\n");

    SetLastError(0xdeadbeef);
    size = VirtualQuery((char *)addr2 + section.VirtualAddress, &info, sizeof(info));
    ok(size == sizeof(info), "VirtualQuery error %ld\n", GetLastError());
    ok(info.BaseAddress == (char *)addr2 + section.VirtualAddress, "got %p != expected %p\n", info.BaseAddress, (char *)addr2 + section.VirtualAddress);
    ok(info.RegionSize == page_size, "got %#Ix != expected %#lx\n", info.RegionSize, page_size);
    ok(info.Protect == scn_page_access, "got %#lx != expected %#lx\n", info.Protect, scn_page_access);
    ok(info.AllocationBase == addr2, "%p != %p\n", info.AllocationBase, addr2);
    ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%#lx != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#lx != MEM_COMMIT\n", info.State);
    ok(info.Type == SEC_IMAGE, "%#lx != SEC_IMAGE\n", info.Type);

    UnmapViewOfFile(addr2);

    SetLastError(0xdeadbeef);
    addr2 = LoadLibraryA(dll_name);
    if (!addr2)
    {
        ok(is_dll, "LoadLibrary should fail, is_dll %d\n", is_dll);
        ok(GetLastError() == ERROR_INVALID_ADDRESS, "expected ERROR_INVALID_ADDRESS, got %ld\n", GetLastError());
    }
    else
    {
        BOOL ret;
        ok(addr2 != 0, "LoadLibrary error %ld, is_dll %d\n", GetLastError(), is_dll);
        ok(addr2 != addr1, "mapped addresses should be different\n");

        SetLastError(0xdeadbeef);
        ret = FreeLibrary(addr2);
        ok(ret, "FreeLibrary error %ld\n", GetLastError());
    }

    status = pNtUnmapViewOfSection(GetCurrentProcess(), addr1);
    ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection error %lx\n", status);

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
    ok(ret, "VirtualProtect error %ld\n", GetLastError());

    orig_prot = old_prot;

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        SetLastError(0xdeadbeef);
        ret = VirtualQuery(section, &info, sizeof(info));
        ok(ret, "VirtualQuery failed %ld\n", GetLastError());
        ok(info.BaseAddress == section, "%ld: got %p != expected %p\n", i, info.BaseAddress, section);
        ok(info.RegionSize == page_size, "%ld: got %#Ix != expected %#lx\n", i, info.RegionSize, page_size);
        ok(info.Protect == PAGE_NOACCESS, "%ld: got %#lx != expected PAGE_NOACCESS\n", i, info.Protect);
        ok(info.AllocationBase == base, "%ld: %p != %p\n", i, info.AllocationBase, base);
        ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%ld: %#lx != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
        ok(info.State == MEM_COMMIT, "%ld: %#lx != MEM_COMMIT\n", i, info.State);
        ok(info.Type == SEC_IMAGE, "%ld: %#lx != SEC_IMAGE\n", i, info.Type);

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(section, page_size, td[i].prot_set, &old_prot);
        if (td[i].prot_get)
        {
            ok(ret, "%ld: VirtualProtect error %ld, requested prot %#lx\n", i, GetLastError(), td[i].prot_set);
            ok(old_prot == PAGE_NOACCESS, "%ld: got %#lx != expected PAGE_NOACCESS\n", i, old_prot);

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(section, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %ld\n", GetLastError());
            ok(info.BaseAddress == section, "%ld: got %p != expected %p\n", i, info.BaseAddress, section);
            ok(info.RegionSize == page_size, "%ld: got %#Ix != expected %#lx\n", i, info.RegionSize, page_size);
            ok(info.Protect == td[i].prot_get, "%ld: got %#lx != expected %#lx\n", i, info.Protect, td[i].prot_get);
            ok(info.AllocationBase == base, "%ld: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%ld: %#lx != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
            ok(info.State == MEM_COMMIT, "%ld: %#lx != MEM_COMMIT\n", i, info.State);
            ok(info.Type == SEC_IMAGE, "%ld: %#lx != SEC_IMAGE\n", i, info.Type);
        }
        else
        {
            ok(!ret, "%ld: VirtualProtect should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%ld: expected ERROR_INVALID_PARAMETER, got %ld\n", i, GetLastError());
        }

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(section, page_size, PAGE_NOACCESS, &old_prot);
        ok(ret, "%ld: VirtualProtect error %ld\n", i, GetLastError());
        if (td[i].prot_get)
            ok(old_prot == td[i].prot_get, "%ld: got %#lx != expected %#lx\n", i, old_prot, td[i].prot_get);
        else
            ok(old_prot == PAGE_NOACCESS, "%ld: got %#lx != expected PAGE_NOACCESS\n", i, old_prot);
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
                ok(!ret, "VirtualProtect(%02lx) should fail\n", prot);
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
            }
            else
                ok(ret, "VirtualProtect(%02lx) error %ld\n", prot, GetLastError());

            rw_prot = 1 << j;
        }

        exec_prot = 1 << (i + 4);
    }

    SetLastError(0xdeadbeef);
    ret = VirtualProtect(section, page_size, orig_prot, &old_prot);
    ok(ret, "VirtualProtect error %ld\n", GetLastError());
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
    DWORD dummy;
    HANDLE hfile;
    HMODULE hlib;
    char temp_path[MAX_PATH];
    char dll_name[MAX_PATH];
    SIZE_T size;
    PEB child_peb;
    PROCESS_BASIC_INFORMATION pbi;
    SECTION_IMAGE_INFORMATION image_info;
    MEMORY_BASIC_INFORMATION info;
    STARTUPINFOA sti;
    PROCESS_INFORMATION pi;
    NTSTATUS status;
    DWORD ret;

    /* prevent displaying of the "Unable to load this DLL" message box */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    GetTempPathA(MAX_PATH, temp_path);

    for (i = 0; i < ARRAY_SIZE(td); i++)
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
        ok(ret, "WriteFile error %ld\n", GetLastError());

        nt_header = nt_header_template;
        nt_header.OptionalHeader.SectionAlignment = page_size;
        nt_header.OptionalHeader.FileAlignment = 0x200;
        nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + page_size;
        nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER);

        section.SizeOfRawData = sizeof(section_data);
        section.PointerToRawData = nt_header.OptionalHeader.FileAlignment;
        section.VirtualAddress = nt_header.OptionalHeader.SectionAlignment;
        section.Misc.VirtualSize = section.SizeOfRawData;
        section.Characteristics = td[i].scn_file_access;
        SetLastError(0xdeadbeef);

        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &nt_header, sizeof(nt_header), &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());

        ret = WriteFile(hfile, &section, sizeof(section), &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());

        /* section data */
        SetFilePointer( hfile, nt_header.OptionalHeader.FileAlignment, NULL, FILE_BEGIN );
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, section_data, sizeof(section_data), &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());

        CloseHandle(hfile);
        SetLastError(0xdeadbeef);
        hlib = LoadLibraryExA(dll_name, NULL, DONT_RESOLVE_DLL_REFERENCES);
        ok(hlib != 0, "LoadLibrary error %ld\n", GetLastError());

        SetLastError(0xdeadbeef);
        size = VirtualQuery((char *)hlib + section.VirtualAddress, &info, sizeof(info));
        ok(size == sizeof(info),
            "%d: VirtualQuery error %ld\n", i, GetLastError());
        ok(info.BaseAddress == (char *)hlib + section.VirtualAddress, "%d: got %p != expected %p\n", i, info.BaseAddress, (char *)hlib + section.VirtualAddress);
        ok(info.RegionSize == page_size, "%d: got %#Ix != expected %#lx\n", i, info.RegionSize, page_size);
        ok(info.Protect == td[i].scn_page_access, "%d: got %#lx != expected %#lx\n", i, info.Protect, td[i].scn_page_access);
        ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
        ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %#lx != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
        ok(info.State == MEM_COMMIT, "%d: %#lx != MEM_COMMIT\n", i, info.State);
        ok(info.Type == SEC_IMAGE, "%d: %#lx != SEC_IMAGE\n", i, info.Type);
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
            ok(size == sizeof(info), "%d: VirtualQuery error %ld\n", i, GetLastError());
            /* FIXME: remove the condition below once Wine is fixed */
            todo_wine_if (info.Protect == PAGE_WRITECOPY || info.Protect == PAGE_EXECUTE_WRITECOPY)
                ok(info.Protect == td[i].scn_page_access_after_write, "%d: got %#lx != expected %#lx\n", i, info.Protect, td[i].scn_page_access_after_write);
        }

        SetLastError(0xdeadbeef);
        ret = FreeLibrary(hlib);
        ok(ret, "FreeLibrary error %ld\n", GetLastError());

        test_image_mapping(dll_name, td[i].scn_page_access, TRUE);

        /* reset IMAGE_FILE_DLL otherwise CreateProcess fails */
        nt_header.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE;
        SetLastError(0xdeadbeef);
        hfile = CreateFileA(dll_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        /* LoadLibrary called on an already memory-mapped file in
         * test_image_mapping() above leads to a file handle leak
         * under nt4, and inability to overwrite and delete the file
         * due to sharing violation error. Ignore it and skip the test,
         * but leave a not deletable temporary file.
         */
        ok(hfile != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());
        SetFilePointer(hfile, sizeof(dos_header), NULL, FILE_BEGIN);
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());
        CloseHandle(hfile);

        memset(&sti, 0, sizeof(sti));
        sti.cb = sizeof(sti);
        SetLastError(0xdeadbeef);
        ret = CreateProcessA(dll_name, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &sti, &pi);
        ok(ret, "CreateProcess() error %ld\n", GetLastError());

        status = pNtQueryInformationProcess( pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL );
        ok( !status, "ProcessBasicInformation got %lx\n", status );
        ret = ReadProcessMemory( pi.hProcess, pbi.PebBaseAddress, &child_peb, sizeof(child_peb), NULL );
        ok( ret, "ReadProcessMemory failed err %lu\n", GetLastError() );
        hlib = child_peb.ImageBaseAddress;

        SetLastError(0xdeadbeef);
        size = VirtualQueryEx(pi.hProcess, (char *)hlib + section.VirtualAddress, &info, sizeof(info));
        ok(size == sizeof(info),
            "%d: VirtualQuery error %ld\n", i, GetLastError());
        ok(info.BaseAddress == (char *)hlib + section.VirtualAddress, "%d: got %p != expected %p\n", i, info.BaseAddress, (char *)hlib + section.VirtualAddress);
        ok(info.RegionSize == page_size, "%d: got %#Ix != expected %#lx\n", i, info.RegionSize, page_size);
        ok(info.Protect == td[i].scn_page_access, "%d: got %#lx != expected %#lx\n", i, info.Protect, td[i].scn_page_access);
        ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
        ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %#lx != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
        ok(info.State == MEM_COMMIT, "%d: %#lx != MEM_COMMIT\n", i, info.State);
        ok(info.Type == SEC_IMAGE, "%d: %#lx != SEC_IMAGE\n", i, info.Type);
        if (info.Protect != PAGE_NOACCESS)
        {
            SetLastError(0xdeadbeef);
            ret = ReadProcessMemory(pi.hProcess, info.BaseAddress, buf, section.SizeOfRawData, NULL);
            ok(ret, "ReadProcessMemory() error %ld\n", GetLastError());
            ok(!memcmp(buf, section_data, section.SizeOfRawData), "wrong section data\n");
        }

        status = NtQueryInformationProcess(pi.hProcess, ProcessImageInformation,
                &image_info, sizeof(image_info), NULL );
        ok(!status, "Got unexpected status %#lx.\n", status);
        ok(!(image_info.ImageCharacteristics & IMAGE_FILE_DLL),
                "Got unexpected characteristics %#x.\n", nt_header.FileHeader.Characteristics);
        status = NtUnmapViewOfSection(pi.hProcess, info.BaseAddress);
        ok(!status, "Got unexpected status %#lx.\n", status);
        status = NtQueryInformationProcess(pi.hProcess, ProcessImageInformation,
                &image_info, sizeof(image_info), NULL );
        ok(!status, "Got unexpected status %#lx.\n", status);
        ok(!(image_info.ImageCharacteristics & IMAGE_FILE_DLL),
                "Got unexpected characteristics %#x.\n", nt_header.FileHeader.Characteristics);

        SetLastError(0xdeadbeef);
        ret = TerminateProcess(pi.hProcess, 0);
        ok(ret, "TerminateProcess() error %ld\n", GetLastError());
        ret = WaitForSingleObject(pi.hProcess, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed: %lx\n", ret);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        test_image_mapping(dll_name, td[i].scn_page_access, FALSE);

        DeleteFileA(dll_name);
    }
}

static void check_tls_index(HANDLE dll, BOOL tls_initialized)
{
    BOOL found_dll = FALSE;
    LIST_ENTRY *root = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (LIST_ENTRY *entry = root->Flink; entry != root; entry = entry->Flink)
    {
        LDR_DATA_TABLE_ENTRY *mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        if (wcsicmp(L"ntdll.dll", mod->BaseDllName.Buffer) == 0)
        {
            /* Pick ntdll as a dll that definitely won't have TLS */
            ok(mod->TlsIndex == 0, "ntdll.dll TlsIndex: %d instead of 0\n", mod->TlsIndex);
        }
        else if (mod->DllBase == dll)
        {
            SHORT expected = tls_initialized ? -1 : 0;
            ok(mod->TlsIndex == expected, "Test exe TlsIndex: %d instead of %d\n", mod->TlsIndex, expected);
            found_dll = TRUE;
        }
        else
        {
            ok(mod->TlsIndex == 0 || mod->TlsIndex == -1, "%s TlsIndex: %d\n",
               debugstr_w(mod->BaseDllName.Buffer), mod->TlsIndex);
        }
    }
    ok(found_dll, "Couldn't find dll %p in module list\n", dll);
}

static int tls_init_fn_output;

static DWORD WINAPI tls_thread_fn(void* tlsidx_v)
{
    int tls_index = (int)(DWORD_PTR)(tlsidx_v);
    const char* str = ((char **)NtCurrentTeb()->ThreadLocalStoragePointer)[tls_index];
    ok( !strcmp( str, "hello world" ), "wrong tls data '%s' at %p\n", str, str );
    ok( tls_init_fn_output == DLL_THREAD_ATTACH,
        "tls init function didn't run or got wrong reason: %d instead of %d\n", tls_init_fn_output, DLL_THREAD_ATTACH );
    tls_init_fn_output = 9999;
    return 0;
}

static void test_import_resolution(void)
{
    char temp_path[MAX_PATH];
    char dll_name[MAX_PATH];
    DWORD dummy;
    void *expect, *tmp;
    char *str;
    SIZE_T size;
    HANDLE hfile, mapping;
    HMODULE mod, mod2;
    NTSTATUS status;
    LARGE_INTEGER offset;
    struct imports
    {
        IMAGE_IMPORT_DESCRIPTOR descr[2];
        IMAGE_THUNK_DATA original_thunks[2];
        IMAGE_THUNK_DATA thunks[2];
        char module[16];
        struct { WORD hint; char name[32]; } function;
        IMAGE_TLS_DIRECTORY tls;
        UINT_PTR tls_init_fn_list[2];
        char tls_data[16];
        SHORT tls_index;
        SHORT tls_index_hi;
        int* tls_init_fn_output;
        UCHAR tls_init_fn[64]; /* Note: Uses rip-relative address of tls_init_fn_output, don't separate */
        UCHAR entry_point_fn[16];
        struct
        {
            IMAGE_BASE_RELOCATION reloc;
            USHORT type_off[32];
        } rel;
    } data, *ptr;
    IMAGE_NT_HEADERS nt, *pnt;
    IMAGE_SECTION_HEADER section;
    SECTION_IMAGE_INFORMATION image;
    int test, tls_index_save, nb_rel;
#if defined(__i386__)
    static const UCHAR tls_init_code[] = {
        0xE8, 0x00, 0x00, 0x00, 0x00, /* call 1f */
        0x59,                         /* 1: pop ecx */
        0x8B, 0x49, 0xF7,             /* mov ecx, [ecx - 9] ; mov ecx, [tls_init_fn_output] */
        0x8B, 0x54, 0x24, 0x08,       /* mov edx, [esp + 8] */
        0x89, 0x11,                   /* mov [ecx], edx */
        0xB8, 0x01, 0x00, 0x00, 0x00, /* mov eax, 1 */
        0xC2, 0x0C, 0x00,             /* ret 12 */
    };
    static const UCHAR entry_point_code[] = {
        0xB8, 0x01, 0x00, 0x00, 0x00, /* mov eax, 1 */
        0xC2, 0x0C, 0x00,             /* ret 12 */
    };
#elif defined(__x86_64__)
    static const UCHAR tls_init_code[] = {
        0x48, 0x8B, 0x0D, 0xF1, 0xFF, 0xFF, 0xFF, /* mov rcx, [rip + tls_init_fn_output] */
        0x89, 0x11,                               /* mov [rcx], edx */
        0xB8, 0x01, 0x00, 0x00, 0x00,             /* mov eax, 1 */
        0xC3,                                     /* ret */
    };
    static const UCHAR entry_point_code[] = {
        0xB8, 0x01, 0x00, 0x00, 0x00, /* mov eax, 1 */
        0xC3,                         /* ret */
    };
#else
    static const UCHAR tls_init_code[] = { 0x00 };
    static const UCHAR entry_point_code[] = { 0x00 };
#endif

    for (test = 0; test < 7; test++)
    {
#define DATA_RVA(ptr) (page_size + ((char *)(ptr) - (char *)&data))
#ifdef _WIN64
#define ADD_RELOC(field) data.rel.type_off[nb_rel++] = (IMAGE_REL_BASED_DIR64 << 12) + offsetof( struct imports, field )
#else
#define ADD_RELOC(field) data.rel.type_off[nb_rel++] = (IMAGE_REL_BASED_HIGHLOW << 12) + offsetof( struct imports, field )
#endif
        winetest_push_context( "%u", test );
        nt = nt_header_template;
        nt.FileHeader.NumberOfSections = 1;
        nt.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
        nt.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE;
        if (test != 2 && test != 5) nt.FileHeader.Characteristics |= IMAGE_FILE_DLL;
        nt.OptionalHeader.SectionAlignment = page_size;
        nt.OptionalHeader.FileAlignment = 0x200;
        nt.OptionalHeader.SizeOfImage = 2 * page_size;
        nt.OptionalHeader.SizeOfHeaders = nt.OptionalHeader.FileAlignment;
        nt.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
        if (test < 6) nt.OptionalHeader.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
        nt.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        memset( nt.OptionalHeader.DataDirectory, 0, sizeof(nt.OptionalHeader.DataDirectory) );
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = sizeof(data.descr);
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = DATA_RVA(data.descr);
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = sizeof(data.tls);
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = DATA_RVA(&data.tls);

        memset( &data, 0, sizeof(data) );
        data.descr[0].OriginalFirstThunk = DATA_RVA( data.original_thunks );
        data.descr[0].FirstThunk = DATA_RVA( data.thunks );
        data.descr[0].Name = DATA_RVA( data.module );
        strcpy( data.module, "kernel32.dll" );
        strcpy( data.function.name, "CreateEventA" );
        data.original_thunks[0].u1.AddressOfData = DATA_RVA( &data.function );
        data.thunks[0].u1.AddressOfData = 0xdeadbeef;
        nb_rel = 0;

        data.tls.StartAddressOfRawData = nt.OptionalHeader.ImageBase + DATA_RVA( data.tls_data );
        ADD_RELOC( tls.StartAddressOfRawData );
        data.tls.EndAddressOfRawData = data.tls.StartAddressOfRawData + sizeof(data.tls_data);
        ADD_RELOC( tls.EndAddressOfRawData );
        data.tls.AddressOfIndex = nt.OptionalHeader.ImageBase + DATA_RVA( &data.tls_index );
        ADD_RELOC( tls.AddressOfIndex );
        strcpy( data.tls_data, "hello world" );
        data.tls_index = 9999;
        data.tls_index_hi = 9999;

        if (test == 3 && sizeof(tls_init_code) > 1)
        {
            /* Windows doesn't consistently call tls init functions on dlls without entry points */
            assert(sizeof(tls_init_code) <= sizeof(data.tls_init_fn));
            assert(sizeof(entry_point_code) <= sizeof(data.entry_point_fn));
            memcpy(data.tls_init_fn, tls_init_code, sizeof(tls_init_code));
            memcpy(data.entry_point_fn, entry_point_code, sizeof(entry_point_code));
            tls_init_fn_output = 9999;
            data.tls_init_fn_output = &tls_init_fn_output;
            data.tls_init_fn_list[0] = nt.OptionalHeader.ImageBase + DATA_RVA(&data.tls_init_fn);
            ADD_RELOC( tls_init_fn_list[0] );
            data.tls.AddressOfCallBacks = nt.OptionalHeader.ImageBase + DATA_RVA(&data.tls_init_fn_list);
            ADD_RELOC( tls.AddressOfCallBacks );
            nt.OptionalHeader.AddressOfEntryPoint = DATA_RVA(&data.entry_point_fn);
        }

        if (nb_rel % 2) nb_rel++;
        data.rel.reloc.VirtualAddress = nt.OptionalHeader.SectionAlignment;
        data.rel.reloc.SizeOfBlock = (char *)&data.rel.type_off[nb_rel] - (char *)&data.rel.reloc;
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = data.rel.reloc.SizeOfBlock;
        nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = DATA_RVA(&data.rel);

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
        if (test == 3) section.Characteristics |= IMAGE_SCN_MEM_EXECUTE;

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
            ok( mod != NULL, "failed to load err %lu\n", GetLastError() );
            if (!mod) break;
            ptr = (struct imports *)((char *)mod + page_size);
            expect = GetProcAddress( GetModuleHandleA( data.module ), data.function.name );
            ok( (void *)ptr->thunks[0].u1.Function == expect, "thunk %p instead of %p for %s.%s\n",
                (void *)ptr->thunks[0].u1.Function, expect, data.module, data.function.name );
            ok( ptr->tls_index < 32, "wrong tls index %d\n", ptr->tls_index );
            str = ((char **)NtCurrentTeb()->ThreadLocalStoragePointer)[ptr->tls_index];
            ok( !strcmp( str, "hello world" ), "wrong tls data '%s' at %p\n", str, str );
            ok(ptr->tls_index_hi == 0, "TLS Index written as a short, high half: %d\n", ptr->tls_index_hi);
            check_tls_index(mod, ptr->tls_index != 9999);
            FreeLibrary( mod );
            break;
        case 1:  /* load with DONT_RESOLVE_DLL_REFERENCES doesn't resolve imports */
            mod = LoadLibraryExA( dll_name, 0, DONT_RESOLVE_DLL_REFERENCES );
            ok( mod != NULL, "failed to load err %lu\n", GetLastError() );
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
            check_tls_index(mod, ptr->tls_index != 9999);
            FreeLibrary( mod2 );
            FreeLibrary( mod );
            break;
        case 2:  /* load without IMAGE_FILE_DLL doesn't resolve imports */
            mod = LoadLibraryA( dll_name );
            ok( mod != NULL, "failed to load err %lu\n", GetLastError() );
            if (!mod) break;
            ptr = (struct imports *)((char *)mod + page_size);
            ok( ptr->thunks[0].u1.Function == 0xdeadbeef, "thunk resolved to %p for %s.%s\n",
                (void *)ptr->thunks[0].u1.Function, data.module, data.function.name );
            ok( ptr->tls_index == 9999, "wrong tls index %d\n", ptr->tls_index );
            check_tls_index(mod, ptr->tls_index != 9999);
            FreeLibrary( mod );
            break;
        case 3:  /* load with tls init function */
            mod = LoadLibraryA( dll_name );
            ok( mod != NULL, "failed to load err %lu\n", GetLastError() );
            if (!mod) break;
            ptr = (struct imports *)((char *)mod + page_size);
            tls_index_save = ptr->tls_index;
            ok( ptr->tls_index < 32, "wrong tls index %d\n", ptr->tls_index );
            if (sizeof(tls_init_code) > 1)
            {
                str = ((char **)NtCurrentTeb()->ThreadLocalStoragePointer)[ptr->tls_index];
                ok( !strcmp( str, "hello world" ), "wrong tls data '%s' at %p\n", str, str );
                /* tls init function will write the reason to *tls_init_fn_output */
                ok( tls_init_fn_output == DLL_PROCESS_ATTACH,
                    "tls init function didn't run or got wrong reason: %d instead of %d\n", tls_init_fn_output, DLL_PROCESS_ATTACH );
                tls_init_fn_output = 9999;
                WaitForSingleObject(CreateThread(NULL, 0, tls_thread_fn, (void*)(DWORD_PTR)ptr->tls_index, 0, NULL), INFINITE);
                ok( tls_init_fn_output == DLL_THREAD_DETACH,
                    "tls init function didn't run or got wrong reason: %d instead of %d\n", tls_init_fn_output, DLL_THREAD_DETACH );
            }
            check_tls_index(mod, ptr->tls_index != 9999);
            tls_init_fn_output = 9999;
            FreeLibrary( mod );
            if (tls_index_save != 9999 && sizeof(tls_init_code) > 1)
                ok( tls_init_fn_output == DLL_PROCESS_DETACH,
                    "tls init function didn't run or got wrong reason: %d instead of %d\n", tls_init_fn_output, DLL_PROCESS_DETACH );
            break;
        case 4:  /* map with ntdll */
        case 5:  /* map with ntdll, without IMAGE_FILE_DLL */
        case 6:  /* map with ntdll, without IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE */
            hfile = CreateFileA(dll_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
            ok( hfile != INVALID_HANDLE_VALUE, "CreateFile failed err %lu\n", GetLastError() );
            mapping = CreateFileMappingA( hfile, NULL, SEC_IMAGE | PAGE_READONLY, 0, 0, NULL );
            CloseHandle( hfile );
            if (test == 6 &&
                (nt_header_template.FileHeader.Machine == IMAGE_FILE_MACHINE_ARMNT ||
                 nt_header_template.FileHeader.Machine == IMAGE_FILE_MACHINE_ARM64))
            {
                ok( !mapping, "CreateFileMappingA succeeded\n" );
                ok( GetLastError() == ERROR_BAD_EXE_FORMAT, "wrong error %lu\n", GetLastError() );
                break;
            }
            status = pNtQuerySection( mapping, SectionImageInformation, &image, sizeof(image), &size );
            ok( !status, "NtQuerySection failed %lx\n", status );
            ok( test == 6 ? !image.ImageDynamicallyRelocated : image.ImageDynamicallyRelocated,
                "image flags %x\n", image.ImageFlags);
            ok( !image.ImageContainsCode, "contains code %x\n", image.ImageContainsCode);
            ok( mapping != 0, "CreateFileMappingA failed err %lu\n", GetLastError() );
            /* make sure that the address is not available */
            tmp = VirtualAlloc( (void *)nt.OptionalHeader.ImageBase, 0x10000,
                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
            mod = NULL;
            size = 0;
            offset.QuadPart = 0;
            status = pNtMapViewOfSection( mapping, GetCurrentProcess(), (void **)&mod, 0, 0, &offset,
                                          &size, 1 /* ViewShare */, 0, PAGE_READONLY );
            todo_wine_if (test == 5)
            ok( status == (test == 6 ? STATUS_IMAGE_NOT_AT_BASE : STATUS_SUCCESS),
                "NtMapViewOfSection failed %lx\n", status );
            ok( mod != (void *)nt.OptionalHeader.ImageBase,  "loaded at image base %p\n", mod );
            pnt = pRtlImageNtHeader( mod );
            ptr = (void *)((char *)mod + page_size);
            if (test == 6)
            {
                ok( (void *)pnt->OptionalHeader.ImageBase != mod, "not relocated from %p\n", mod );
                ok( (char *)ptr->tls.StartAddressOfRawData == (char *)nt.OptionalHeader.ImageBase + DATA_RVA( data.tls_data ),
                    "tls relocated %p / %p\n", (void *)ptr->tls.StartAddressOfRawData,
                    (char *)nt.OptionalHeader.ImageBase + DATA_RVA( data.tls_data ));
            }
            else todo_wine_if (test == 5)
            {
                ok( (void *)pnt->OptionalHeader.ImageBase == mod, "not at base %p / %p\n",
                    (void *)pnt->OptionalHeader.ImageBase, mod );
                ok( (char *)ptr->tls.StartAddressOfRawData == (char *)mod + DATA_RVA( data.tls_data ),
                    "tls not relocated %p / %p\n", (void *)ptr->tls.StartAddressOfRawData,
                    (char *)mod + DATA_RVA( data.tls_data ));
            }
            UnmapViewOfFile( mod );
            CloseHandle( mapping );
            if (tmp) VirtualFree( tmp, 0, MEM_RELEASE );
            break;
        }
        DeleteFileA( dll_name );
        winetest_pop_context();
#undef DATA_RVA
    }
}

static HANDLE gen_forward_chain_testdll( char testdll_path[MAX_PATH],
                                         const char source_dll[MAX_PATH],
                                         BOOL is_export, BOOL is_import,
                                         DWORD *exp_func_base_rva,
                                         DWORD *imp_thunk_base_rva )
{
    DWORD text_rva = page_size;  /* assumes that the PE/COFF headers fit in a page */
    DWORD text_size = page_size;
    DWORD edata_rva = text_rva + text_size;
    DWORD edata_size = page_size;
    DWORD idata_rva = edata_rva + text_size;
    DWORD idata_size = page_size;
    DWORD eof_rva = idata_rva + edata_size;
    const IMAGE_SECTION_HEADER sections[3] = {
        {
            .Name = ".text",
            .Misc = { .VirtualSize = text_size },
            .VirtualAddress   = text_rva,
            .SizeOfRawData    = text_size,
            .PointerToRawData = text_rva,
            .Characteristics  = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE,
        },
        {
            .Name = ".edata",
            .Misc = { .VirtualSize = edata_size },
            .VirtualAddress   = edata_rva,
            .SizeOfRawData    = edata_size,
            .PointerToRawData = edata_rva,
            .Characteristics  = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,
        },
        {
            .Name = ".idata",
            .Misc = { .VirtualSize = edata_size },
            .VirtualAddress   = idata_rva,
            .SizeOfRawData    = idata_size,
            .PointerToRawData = idata_rva,
            .Characteristics  = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE,
        },
    };
    struct expdesc
    {
        const IMAGE_EXPORT_DIRECTORY dir;

        DWORD functions[2];

        const DWORD names[2];
        const WORD name_ords[2];
        const char str_forward_test_func[32];
        const char str_forward_test_func2[32];

        char dll_name[MAX_PATH];         /* dynamically populated */
        char strpool[2][MAX_PATH + 16];  /* for names of export forwarders */
    } expdesc = {
        .dir = {
            .Characteristics       = 0,
            .TimeDateStamp         = 0x12345678,
            .Name                  = edata_rva + offsetof(struct expdesc, dll_name),
            .Base                  = 1,
            .NumberOfFunctions     = ARRAY_SIZE(expdesc.functions),
            .NumberOfNames         = ARRAY_SIZE(expdesc.names),
            .AddressOfFunctions    = edata_rva + offsetof(struct expdesc, functions),
            .AddressOfNames        = edata_rva + offsetof(struct expdesc, names),
            .AddressOfNameOrdinals = edata_rva + offsetof(struct expdesc, name_ords),
        },
        .functions = {
            text_rva + 0x4,  /* may be overwritten */
            text_rva + 0x8,  /* may be overwritten */
        },
        .names = {
            edata_rva + offsetof(struct expdesc, str_forward_test_func),
            edata_rva + offsetof(struct expdesc, str_forward_test_func2),
        },
        .name_ords = {
            0,
            1,
        },
        .str_forward_test_func = "forward_test_func",
        .str_forward_test_func2 = "forward_test_func2",
    };
    struct impdesc
    {
        const IMAGE_IMPORT_DESCRIPTOR descr[2];
        const IMAGE_THUNK_DATA original_thunks[3];
        const IMAGE_THUNK_DATA thunks[3];
        const struct { WORD hint; char name[32]; } impname_forward_test_func;

        char module[MAX_PATH];  /* dynamically populated */
    } impdesc = {
        .descr = {
            {
                .OriginalFirstThunk = idata_rva + offsetof(struct impdesc, original_thunks),
                .TimeDateStamp      = 0,
                .ForwarderChain     = -1,
                .Name               = idata_rva + offsetof(struct impdesc, module),
                .FirstThunk         = idata_rva + offsetof(struct impdesc, thunks),
            },
            {{ 0 }},
        },
        .original_thunks = {
            {{ idata_rva + offsetof(struct impdesc, impname_forward_test_func) }},
            {{ IMAGE_ORDINAL_FLAG | 2 }},
            {{ 0 }},
        },
        .thunks = {
            {{ idata_rva + offsetof(struct impdesc, impname_forward_test_func) }},
            {{ IMAGE_ORDINAL_FLAG | 2 }},
            {{ 0 }},
        },
        .impname_forward_test_func = { 0, "forward_test_func" },
    };
    IMAGE_NT_HEADERS nt_header;
    char temp_path[MAX_PATH];
    HANDLE file, file_w;
    LARGE_INTEGER qpc;
    DWORD outlen;
    BOOL ret;
    int res;

    QueryPerformanceCounter( &qpc );
    res = snprintf( expdesc.dll_name, ARRAY_SIZE(expdesc.dll_name),
                    "ldr%05lx.dll", qpc.LowPart & 0xfffffUL );
    ok( res > 0 && res < ARRAY_SIZE(expdesc.dll_name), "snprintf failed\n" );

    if (source_dll)
    {
        const char *export_names[2] = {
            "forward_test_func",
            "#2",
        };
        const char *backslash = strrchr( source_dll, '\\' );
        const char *dllname = backslash ? backslash + 1 : source_dll;
        const char *dot = strrchr( dllname, '.' );
        size_t ext_start = dot ? dot - dllname : strlen(dllname);
        size_t i;

        res = snprintf( impdesc.module, ARRAY_SIZE(impdesc.module), "%s", dllname );
        ok( res > 0 && res < ARRAY_SIZE(impdesc.module), "snprintf() failed\n" );

        for (i = 0; i < ARRAY_SIZE(export_names); i++)
        {
            char *buf;
            size_t buf_size;

            assert( i < ARRAY_SIZE(expdesc.strpool) );
            buf = expdesc.strpool[i];
            buf_size = ARRAY_SIZE(expdesc.strpool[i]);

            assert( ext_start < buf_size );
            memcpy( buf, dllname, ext_start );
            buf += ext_start;
            buf_size -= ext_start;

            res = snprintf( buf, buf_size, ".%s", export_names[i] );
            ok( res > 0 && res < buf_size, "snprintf() failed\n" );

            assert( i < ARRAY_SIZE(expdesc.functions) );
            expdesc.functions[i] = edata_rva + (expdesc.strpool[i] - (char *)&expdesc);
        }
    }

    nt_header = nt_header_template;
    nt_header.FileHeader.TimeDateStamp = 0x12345678;
    nt_header.FileHeader.NumberOfSections = ARRAY_SIZE(sections);
    nt_header.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);

    nt_header.OptionalHeader.SizeOfCode = text_size;
    nt_header.OptionalHeader.SectionAlignment = page_size;
    nt_header.OptionalHeader.FileAlignment = page_size;
    nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + sizeof(sections);
    nt_header.OptionalHeader.SizeOfImage = eof_rva;
    if (is_export)
    {
        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = edata_rva;
        nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = sizeof(expdesc);
    }
    /* Always have an import descriptor (even if empty) just like a real DLL */
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = idata_rva;
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = is_import ? sizeof(impdesc) : sizeof(IMAGE_IMPORT_DESCRIPTOR);

    ok( nt_header.OptionalHeader.SizeOfHeaders <= text_rva,
        "headers (size %#lx) should not overlap with text area (RVA %#lx)\n",
        nt_header.OptionalHeader.SizeOfHeaders, text_rva );

    outlen = GetTempPathA( ARRAY_SIZE(temp_path), temp_path );
    ok( outlen > 0 && outlen < ARRAY_SIZE(temp_path), "GetTempPathA() err=%lu\n", GetLastError() );

    res = snprintf( testdll_path, MAX_PATH, "%s\\%s", temp_path, expdesc.dll_name );
    ok( res > 0 && res < MAX_PATH, "snprintf failed\n" );

    /* Open file handle that will be deleted on close or process termination */
    file = CreateFileA( testdll_path,
                        DELETE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                        NULL );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile(%s) for delete returned error %lu\n",
        wine_dbgstr_a( testdll_path ), GetLastError() );

    /* Open file again with write access */
    file_w = CreateFileA( testdll_path,
                          GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_DELETE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );
    ok( file_w != INVALID_HANDLE_VALUE, "CreateFile(%s) for write returned error %lu\n",
        wine_dbgstr_a( testdll_path ), GetLastError() );

    ret = WriteFile( file_w, &dos_header, sizeof(dos_header), &outlen, NULL );
    ok( ret && outlen == sizeof(dos_header), "write dos_header: err=%lu outlen=%lu\n", GetLastError(), outlen );

    ret = WriteFile( file_w, &nt_header, sizeof(nt_header), &outlen, NULL );
    ok( ret && outlen == sizeof(nt_header), "write nt_header: err=%lu outlen=%lu\n", GetLastError(), outlen );

    ret = WriteFile( file_w, sections, sizeof(sections), &outlen, NULL );
    ok( ret && outlen == sizeof(sections), "write sections: err=%lu outlen=%lu\n", GetLastError(), outlen );

    if (is_export)
    {
        SetFilePointer( file_w, edata_rva, NULL, FILE_BEGIN );
        ret = WriteFile( file_w, &expdesc, sizeof(expdesc), &outlen, NULL );
        ok( ret && outlen == sizeof(expdesc), "write expdesc: err=%lu outlen=%lu\n", GetLastError(), outlen );
    }

    if (is_import)
    {
        SetFilePointer( file_w, idata_rva, NULL, FILE_BEGIN );
        ret = WriteFile( file_w, &impdesc, sizeof(impdesc), &outlen, NULL );
        ok( ret && outlen == sizeof(impdesc), "write impdesc: err=%lu outlen=%lu\n", GetLastError(), outlen );
    }

    ret = SetFilePointer( file_w, eof_rva, NULL, FILE_BEGIN );
    ok( ret, "%lu\n", GetLastError() );
    ret = SetEndOfFile( file_w );
    ok( ret, "%lu\n", GetLastError() );

    ret = CloseHandle( file_w );
    ok( ret, "%lu\n", GetLastError() );

    if (exp_func_base_rva)
    {
        *exp_func_base_rva = is_export ? edata_rva + ((char *)&expdesc.functions - (char *)&expdesc) : 0;
    }

    if (imp_thunk_base_rva)
    {
        *imp_thunk_base_rva = is_import ? idata_rva + ((char *)&impdesc.thunks - (char *)&impdesc) : 0;
    }

    return file;
}

static void subtest_export_forwarder_dep_chain( size_t num_chained_export_modules,
                                                size_t exporter_index,
                                                BOOL test_static_import )
{
    size_t num_modules = num_chained_export_modules + !!test_static_import;
    size_t importer_index = test_static_import ? num_modules - 1 : 0;
    DWORD imp_thunk_base_rva, exp_func_base_rva;
    size_t ultimate_depender_index = 0; /* latest module depending on modules earlier in chain */
    char temp_paths[4][MAX_PATH];
    HANDLE temp_files[4];
    UINT_PTR exports[2];
    HMODULE modules[4];
    BOOL res;
    size_t i;

    assert(exporter_index < num_chained_export_modules);
    assert(num_modules > 1);
    assert(num_modules <= ARRAY_SIZE(temp_paths));
    assert(num_modules <= ARRAY_SIZE(temp_files));
    assert(num_modules <= ARRAY_SIZE(modules));

    if (winetest_debug > 1)
        trace( "Generate a chain of test DLL fixtures\n" );

    for (i = 0; i < num_modules; i++)
    {
        temp_files[i] = gen_forward_chain_testdll( temp_paths[i],
                                                   i >= 1 ? temp_paths[i - 1] : NULL,
                                                   i < num_chained_export_modules,
                                                   importer_index && i == importer_index,
                                                   i == 0 ? &exp_func_base_rva : NULL,
                                                   i == importer_index ? &imp_thunk_base_rva : NULL );
    }

    if (winetest_debug > 1)
        trace( "Load the entire test DLL chain\n" );

    for (i = 0; i < num_modules; i++)
    {
        HMODULE module;

        ok( !GetModuleHandleA( temp_paths[i] ), "%s already loaded\n",
            wine_dbgstr_a( temp_paths[i] ) );

        modules[i] = LoadLibraryA( temp_paths[i] );
        ok( !!modules[i], "LoadLibraryA(temp_paths[%Iu] = %s) err=%lu\n",
            i, wine_dbgstr_a( temp_paths[i] ), GetLastError() );

        if (i == importer_index)
        {
            /* Statically importing export forwarder introduces a load-time dependency */
            ultimate_depender_index = max( ultimate_depender_index, importer_index );
        }

        module = GetModuleHandleA( temp_paths[i] );
        ok( module == modules[i], "modules[%Iu] expected %p, got %p err=%lu\n",
            i, modules[i], module, GetLastError() );
    }

    if (winetest_debug > 1)
        trace( "Get address of exported functions from the source module\n" );

    for (i = 0; i < ARRAY_SIZE(exports); i++)
    {
        char *mod_base = (char *)modules[0];  /* source (non-forward) DLL */
        exports[i] = (UINT_PTR)(mod_base + ((DWORD *)(mod_base + exp_func_base_rva))[i]);
    }

    if (winetest_debug > 1)
        trace( "Check import address table of the importer DLL, if any\n" );

    if (importer_index)
    {
        UINT_PTR *imp_thunk_base = (UINT_PTR *)((char *)modules[importer_index] + imp_thunk_base_rva);
        for (i = 0; i < ARRAY_SIZE(exports); i++)
        {
            ok( imp_thunk_base[i] == exports[i], "import thunk mismatch [%Iu]: (%#Ix, %#Ix)\n",
                i, imp_thunk_base[i], exports[i] );
        }
    }

    if (winetest_debug > 1)
        trace( "Call GetProcAddress() on the exporter DLL, if any\n" );

    if (exporter_index)
    {
        UINT_PTR proc;

        proc = (UINT_PTR)GetProcAddress( modules[exporter_index], "forward_test_func" );
        ok( proc == exports[0], "GetProcAddress mismatch [0]: (%#Ix, %#Ix)\n", proc, exports[0] );

        proc = (UINT_PTR)GetProcAddress( modules[exporter_index], (LPSTR)2 );
        ok( proc == exports[1], "GetProcAddress mismatch [1]: (%#Ix, %#Ix)\n", proc, exports[1] );

        /* Dynamically importing export forwarder introduces a runtime dependency */
        ultimate_depender_index = max( ultimate_depender_index, exporter_index );
    }

    if (winetest_debug > 1)
        trace( "Unreference modules except the ultimate dependant DLL\n" );

    for (i = 0; i < ultimate_depender_index; i++)
    {
        HMODULE module;

        res = FreeLibrary( modules[i] );
        ok( res, "FreeLibrary(modules[%Iu]) err=%lu\n", i, GetLastError() );

        /* FreeLibrary() should *not* unload the DLL immediately */
        module = GetModuleHandleA( temp_paths[i] );
        todo_wine_if(i < ultimate_depender_index && i + 1 != importer_index)
        ok( module == modules[i], "modules[%Iu] expected %p, got %p (unloaded?) err=%lu\n",
            i, modules[i], module, GetLastError() );
    }

    if (winetest_debug > 1)
        trace( "The ultimate dependant DLL should keep other DLLs from being unloaded\n" );

    for (i = 0; i < num_modules; i++)
    {
        HMODULE module = GetModuleHandleA( temp_paths[i] );

        todo_wine_if(i < ultimate_depender_index && i + 1 != importer_index)
        ok( module == modules[i], "modules[%Iu] expected %p, got %p (unloaded?) err=%lu\n",
            i, modules[i], module, GetLastError() );
    }

    if (winetest_debug > 1)
        trace( "Unreference the remaining modules (including the dependant DLL)\n" );

    for (i = ultimate_depender_index; i < num_modules; i++)
    {
        res = FreeLibrary( modules[i] );
        ok( res, "FreeLibrary(modules[%Iu]) err=%lu\n", i, GetLastError() );

        /* FreeLibrary() should unload the DLL immediately */
        ok( !GetModuleHandleA( temp_paths[i] ), "modules[%Iu] should not be kept loaded (2)\n", i );
    }

    if (winetest_debug > 1)
        trace( "All modules should be unloaded; the unloading process should not reload any DLL\n" );

    for (i = 0; i < num_modules; i++)
    {
        ok( !GetModuleHandleA( temp_paths[i] ), "modules[%Iu] should not be kept loaded (3)\n", i );
    }

    if (winetest_debug > 1)
        trace( "Close and delete temp files\n" );

    for (i = 0; i < num_modules; i++)
    {
        /* handles should be delete-on-close */
        CloseHandle( temp_files[i] );
    }
}

static void test_export_forwarder_dep_chain(void)
{
    winetest_push_context( "no import" );
    /* export forwarder does not introduce a dependency on its own */
    subtest_export_forwarder_dep_chain( 2, 0, FALSE );
    winetest_pop_context();

    winetest_push_context( "static import of export forwarder" );
    subtest_export_forwarder_dep_chain( 2, 0, TRUE );
    winetest_pop_context();

    winetest_push_context( "static import of chained export forwarder" );
    subtest_export_forwarder_dep_chain( 3, 0, TRUE );
    winetest_pop_context();

    winetest_push_context( "dynamic import of export forwarder" );
    subtest_export_forwarder_dep_chain( 2, 1, FALSE );
    winetest_pop_context();

    winetest_push_context( "dynamic import of chained export forwarder" );
    subtest_export_forwarder_dep_chain( 3, 2, FALSE );
    winetest_pop_context();
}

#define MAX_COUNT 10
static HANDLE attached_thread[MAX_COUNT];
static DWORD attached_thread_count;
static HANDLE event, mutex, semaphore;
static HANDLE stop_event, loader_lock_event, peb_lock_event, heap_lock_event, cs_lock_event, ack_event;
static CRITICAL_SECTION cs_lock;
static int test_dll_phase, inside_loader_lock, inside_peb_lock, inside_heap_lock, inside_cs_lock;
static LONG fls_callback_count;

static DWORD WINAPI mutex_thread_proc(void *param)
{
    HANDLE wait_list[5];
    DWORD ret;

    ret = WaitForSingleObject(mutex, 0);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#lx\n", ret);

    SetEvent(param);

    wait_list[0] = stop_event;
    wait_list[1] = loader_lock_event;
    wait_list[2] = peb_lock_event;
    wait_list[3] = heap_lock_event;
    wait_list[4] = cs_lock_event;

    trace("%04lx: mutex_thread_proc: starting\n", GetCurrentThreadId());
    while (1)
    {
        ret = WaitForMultipleObjects(ARRAY_SIZE(wait_list), wait_list, FALSE, 50);
        if (ret == WAIT_OBJECT_0) break;
        else if (ret == WAIT_OBJECT_0 + 1)
        {
            ULONG_PTR loader_lock_magic;
            trace("%04lx: mutex_thread_proc: Entering loader lock\n", GetCurrentThreadId());
            ret = pLdrLockLoaderLock(0, NULL, &loader_lock_magic);
            ok(!ret, "LdrLockLoaderLock error %#lx\n", ret);
            inside_loader_lock++;
            SetEvent(ack_event);
        }
        else if (ret == WAIT_OBJECT_0 + 2)
        {
            trace("%04lx: mutex_thread_proc: Entering PEB lock\n", GetCurrentThreadId());
            pRtlAcquirePebLock();
            inside_peb_lock++;
            SetEvent(ack_event);
        }
        else if (ret == WAIT_OBJECT_0 + 3)
        {
            trace("%04lx: mutex_thread_proc: Entering heap lock\n", GetCurrentThreadId());
            HeapLock(GetProcessHeap());
            inside_heap_lock++;
            SetEvent(ack_event);
        }
        else if (ret == WAIT_OBJECT_0 + 4)
        {
            trace("%04lx: mutex_thread_proc: Entering CS lock\n", GetCurrentThreadId());
            EnterCriticalSection(&cs_lock);
            inside_cs_lock++;
            SetEvent(ack_event);
        }
    }

    trace("%04lx: mutex_thread_proc: exiting\n", GetCurrentThreadId());
    return 196;
}

static DWORD WINAPI semaphore_thread_proc(void *param)
{
    DWORD ret;

    ret = WaitForSingleObject(semaphore, 0);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#lx\n", ret);

    SetEvent(param);

    while (1)
    {
        if (winetest_debug > 1)
            trace("%04lx: semaphore_thread_proc: still alive\n", GetCurrentThreadId());
        if (WaitForSingleObject(stop_event, 50) != WAIT_TIMEOUT) break;
    }

    trace("%04lx: semaphore_thread_proc: exiting\n", GetCurrentThreadId());
    return 196;
}

static DWORD WINAPI noop_thread_proc(void *param)
{
    if (param)
    {
        LONG *noop_thread_started = param;
        InterlockedIncrement(noop_thread_started);
    }

    trace("%04lx: noop_thread_proc: exiting\n", GetCurrentThreadId());
    return 195;
}

static VOID WINAPI fls_callback(PVOID lpFlsData)
{
    ok(lpFlsData == (void*) 0x31415, "lpFlsData is %p, expected %p\n", lpFlsData, (void*) 0x31415);
    InterlockedIncrement(&fls_callback_count);
}

static LIST_ENTRY *fls_list_head;

static unsigned int check_linked_list(const LIST_ENTRY *le, const LIST_ENTRY *search_entry, unsigned int *index_found)
{
    unsigned int count = 0;
    LIST_ENTRY *entry;

    *index_found = ~0;

    for (entry = le->Flink; entry != le; entry = entry->Flink)
    {
        if (entry == search_entry)
        {
            ok(*index_found == ~0, "Duplicate list entry.\n");
            *index_found = count;
        }
        ++count;
    }
    return count;
}

static BOOL WINAPI dll_entry_point(HINSTANCE hinst, DWORD reason, LPVOID param)
{
    static LONG noop_thread_started;
    static DWORD fls_index = FLS_OUT_OF_INDEXES, fls_index2 = FLS_OUT_OF_INDEXES;
    static int fls_count = 0;
    static int thread_detach_count = 0;
    static int thread_count;
    DWORD ret;

    ok(!inside_loader_lock, "inside_loader_lock should not be set\n");
    ok(!inside_peb_lock, "inside_peb_lock should not be set\n");

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        trace("dll: %p, DLL_PROCESS_ATTACH, %p\n", hinst, param);

        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);

        /* Set up the FLS slot, if FLS is available */
        if (pFlsGetValue)
        {
            void* value;
            BOOL bret;
            ret = pFlsAlloc(&fls_callback);
            ok(ret != FLS_OUT_OF_INDEXES, "FlsAlloc returned %ld\n", ret);
            fls_index = ret;
            SetLastError(0xdeadbeef);
            value = pFlsGetValue(fls_index);
            ok(!value, "FlsGetValue returned %p, expected NULL\n", value);
            ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue failed with error %lu\n", GetLastError());
            bret = pFlsSetValue(fls_index, (void*) 0x31415);
            ok(bret, "FlsSetValue failed\n");
            fls_count++;

            fls_index2 = pFlsAlloc(&fls_callback);
            ok(fls_index2 != FLS_OUT_OF_INDEXES, "FlsAlloc returned %ld\n", ret);
        }
        ++thread_count;
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
            todo_wine
            ok(0, "dll_entry_point: process should already deadlock\n");
            break;
        }
        else if (test_dll_phase == 7)
        {
            EnterCriticalSection(&cs_lock);
        }

        if (test_dll_phase == 0 || test_dll_phase == 1 || test_dll_phase == 3 || test_dll_phase == 7)
            ok(param != NULL, "dll: param %p\n", param);
        else
            ok(!param, "dll: param %p\n", param);

        if (test_dll_phase == 0 || test_dll_phase == 1) expected_code = 195;
        else if (test_dll_phase == 3) expected_code = 196;
        else if (test_dll_phase == 7) expected_code = 199;
        else expected_code = STILL_ACTIVE;

        ret = pRtlDllShutdownInProgress();
        if (test_dll_phase == 0 || test_dll_phase == 1 || test_dll_phase == 3)
        {
            ok(ret, "RtlDllShutdownInProgress returned %ld\n", ret);
        }
        else
        {
            /* FIXME: remove once Wine is fixed */
            todo_wine_if (!(expected_code == STILL_ACTIVE || expected_code == 196))
                ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);
        }

        /* In the case that the process is terminating, FLS slots should still be accessible, but
         * the callback should be already run for this thread and the contents already NULL.
         */
        if (param && pFlsGetValue)
        {
            void* value;
            SetLastError(0xdeadbeef);
            value = pFlsGetValue(fls_index);
            ok(value == NULL, "FlsGetValue returned %p, expected NULL\n", value);
            ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue failed with error %lu\n", GetLastError());
            ok(fls_callback_count == thread_detach_count + 1,
                "wrong FLS callback count %ld, expected %d\n", fls_callback_count, thread_detach_count + 1);
        }
        if (pFlsFree)
        {
            BOOL ret;
            /* Call FlsFree now and run the remaining callbacks from uncleanly terminated threads */
            ret = pFlsFree(fls_index);
            ok(ret, "FlsFree failed with error %lu\n", GetLastError());
            fls_index = FLS_OUT_OF_INDEXES;
            ok(fls_callback_count == fls_count,
                "wrong FLS callback count %ld, expected %d\n", fls_callback_count, fls_count);
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
                ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#lx\n", ret);
            }
            ret = GetExitCodeThread(attached_thread[i], &code);
            trace("dll: GetExitCodeThread(%lu) => %ld,%lu\n", i, ret, code);
            ok(ret == 1, "GetExitCodeThread returned %ld, expected 1\n", ret);
            ok(code == expected_code, "expected thread exit code %lu, got %lu\n", expected_code, code);
        }

        ret = WaitForSingleObject(event, 0);
        ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);

        ret = WaitForSingleObject(mutex, 0);
        if (expected_code == STILL_ACTIVE)
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
        else
            ok(ret == WAIT_ABANDONED, "expected WAIT_ABANDONED, got %#lx\n", ret);

        /* semaphore is not abandoned on thread termination */
        ret = WaitForSingleObject(semaphore, 0);
        ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);

        if (expected_code == STILL_ACTIVE)
        {
            ret = WaitForSingleObject(attached_thread[0], 0);
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
            ret = WaitForSingleObject(attached_thread[1], 0);
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
        }
        else
        {
            ret = WaitForSingleObject(attached_thread[0], 0);
            ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#lx\n", ret);
            ret = WaitForSingleObject(attached_thread[1], 0);
            ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#lx\n", ret);
        }

        /* win7 doesn't allow creating a thread during process shutdown but
         * earlier Windows versions allow it.
         */
        noop_thread_started = 0;
        SetLastError(0xdeadbeef);
        handle = CreateThread(NULL, 0, noop_thread_proc, &noop_thread_started, 0, &ret);
        if (param)
        {
            ok(!handle, "CreateThread should fail\n");
            ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
        }
        else
        {
            ok(handle != 0, "CreateThread error %ld\n", GetLastError());
            ret = WaitForSingleObject(handle, 1000);
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
            ok(!noop_thread_started, "thread shouldn't start yet\n");
            CloseHandle(handle);
        }

        SetLastError(0xdeadbeef);
        process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, GetCurrentProcessId());
        ok(process != NULL, "OpenProcess error %ld\n", GetLastError());

        noop_thread_started = 0;
        SetLastError(0xdeadbeef);
        handle = CreateRemoteThread(process, NULL, 0, noop_thread_proc, &noop_thread_started, 0, &ret);
        if (param)
        {
            ok(!handle, "CreateRemoteThread should fail\n");
            ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
        }
        else
        {
            ok(handle != 0, "CreateRemoteThread error %ld\n", GetLastError());
            ret = WaitForSingleObject(handle, 1000);
            ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
            ok(!noop_thread_started, "thread shouldn't start yet\n");
            CloseHandle(handle);
        }

        SetLastError(0xdeadbeef);
        handle = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, NULL);
        ok(handle != 0, "CreateFileMapping error %ld\n", GetLastError());

        offset.u.LowPart = 0;
        offset.u.HighPart = 0;
        addr = NULL;
        size = 0;
        ret = pNtMapViewOfSection(handle, process, &addr, 0, 0, &offset,
                                  &size, 1 /* ViewShare */, 0, PAGE_READONLY);
        ok(ret == STATUS_SUCCESS, "NtMapViewOfSection error %#lx\n", ret);
        ret = pNtUnmapViewOfSection(process, addr);
        ok(ret == STATUS_SUCCESS, "NtUnmapViewOfSection error %#lx\n", ret);

        CloseHandle(handle);
        CloseHandle(process);

        handle = GetModuleHandleA("winver.exe");
        ok(!handle, "winver.exe shouldn't be loaded yet\n");
        SetLastError(0xdeadbeef);
        handle = LoadLibraryA("winver.exe");
        ok(handle != 0, "LoadLibrary error %ld\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = FreeLibrary(handle);
        ok(ret, "FreeLibrary error %ld\n", GetLastError());
        handle = GetModuleHandleA("winver.exe");
        if (param)
            ok(handle != 0, "winver.exe should not be unloaded\n");
        else
        todo_wine
            ok(!handle, "winver.exe should be unloaded\n");

        SetLastError(0xdeadbeef);
        ret = WaitForDebugEvent(&de, 0);
        ok(!ret, "WaitForDebugEvent should fail\n");
        ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

        SetLastError(0xdeadbeef);
        ret = DebugActiveProcess(GetCurrentProcessId());
        ok(!ret, "DebugActiveProcess should fail\n");
        ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());

        SetLastError(0xdeadbeef);
        ret = WaitForDebugEvent(&de, 0);
        ok(!ret, "WaitForDebugEvent should fail\n");
        ok(GetLastError() == ERROR_SEM_TIMEOUT, "expected ERROR_SEM_TIMEOUT, got %ld\n", GetLastError());

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

        ++thread_count;

        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);

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
            ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue failed with error %lu\n", GetLastError());
            ret = pFlsSetValue(fls_index, (void*) 0x31415);
            ok(ret, "FlsSetValue failed\n");
            fls_count++;
        }

        break;
    case DLL_THREAD_DETACH:
        trace("dll: %p, DLL_THREAD_DETACH, %p\n", hinst, param);
        --thread_count;
        thread_detach_count++;

        ret = pRtlDllShutdownInProgress();
        /* win7 doesn't allow creating a thread during process shutdown but
         * earlier Windows versions allow it. In that case DLL_THREAD_DETACH is
         * sent on thread exit, but DLL_THREAD_ATTACH is never received.
         */
        if (noop_thread_started)
            ok(ret, "RtlDllShutdownInProgress returned %ld\n", ret);
        else
            ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);

        /* FLS data should already be destroyed, if FLS is available.
         */
        if (pFlsGetValue && fls_index != FLS_OUT_OF_INDEXES)
        {
            unsigned int index, count;
            void* value;
            BOOL bret;

            SetLastError(0xdeadbeef);
            value = pFlsGetValue(fls_index);
            ok(!value, "FlsGetValue returned %p, expected NULL\n", value);
            ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue failed with error %lu\n", GetLastError());

            bret = pFlsSetValue(fls_index2, (void*) 0x31415);
            ok(bret, "FlsSetValue failed\n");

            if (fls_list_head)
            {
                count = check_linked_list(fls_list_head, &NtCurrentTeb()->FlsSlots->fls_list_entry, &index);
                ok(count <= thread_count, "Got unexpected count %u, thread_count %u.\n", count, thread_count);
                ok(index == ~0, "Got unexpected index %u.\n", index);
            }
        }

        break;
    default:
        trace("dll: %p, %ld, %p\n", hinst, reason, param);
        break;
    }

    *child_failures = winetest_get_failures();

    return TRUE;
}

static void CALLBACK ldr_notify_callback(ULONG reason, LDR_DLL_NOTIFICATION_DATA *data, void *context)
{
    /* If some DLL happens to be loaded during process shutdown load notification is called but never unload
     * notification. */
    ok(reason == LDR_DLL_NOTIFICATION_REASON_LOADED, "got reason %lu.\n", reason);
}

static void child_process(const char *dll_name, DWORD target_offset)
{
    void *target;
    DWORD ret, dummy, i, code, expected_code;
    HANDLE file, thread, process;
    HMODULE hmod;
    struct PROCESS_BASIC_INFORMATION_PRIVATE pbi;
    DWORD_PTR affinity;
    void *cookie;

    trace("phase %d: writing %p at %#lx\n", test_dll_phase, dll_entry_point, target_offset);

    if (pFlsAlloc)
    {
        fls_list_head = NtCurrentTeb()->Peb->FlsListHead.Flink ? &NtCurrentTeb()->Peb->FlsListHead
                : NtCurrentTeb()->FlsSlots->fls_list_entry.Flink;
    }

    SetLastError(0xdeadbeef);
    mutex = CreateMutexW(NULL, FALSE, NULL);
    ok(mutex != 0, "CreateMutex error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    semaphore = CreateSemaphoreW(NULL, 1, 1, NULL);
    ok(semaphore != 0, "CreateSemaphore error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(event != 0, "CreateEvent error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    loader_lock_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(loader_lock_event != 0, "CreateEvent error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    peb_lock_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(peb_lock_event != 0, "CreateEvent error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    heap_lock_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(heap_lock_event != 0, "CreateEvent error %ld\n", GetLastError());

    InitializeCriticalSection(&cs_lock);
    SetLastError(0xdeadbeef);
    cs_lock_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(cs_lock_event != 0, "CreateEvent error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ack_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(ack_event != 0, "CreateEvent error %ld\n", GetLastError());

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
    ok(ret, "WriteFile error %ld\n", GetLastError());
    CloseHandle(file);

    SetLastError(0xdeadbeef);
    hmod = LoadLibraryA(dll_name);
    ok(hmod != 0, "LoadLibrary error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(stop_event != 0, "CreateEvent error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    thread = CreateThread(NULL, 0, mutex_thread_proc, event, 0, &dummy);
    ok(thread != 0, "CreateThread error %ld\n", GetLastError());
    WaitForSingleObject(event, 3000);
    CloseHandle(thread);

    ResetEvent(event);

    SetLastError(0xdeadbeef);
    thread = CreateThread(NULL, 0, semaphore_thread_proc, event, 0, &dummy);
    ok(thread != 0, "CreateThread error %ld\n", GetLastError());
    WaitForSingleObject(event, 3000);
    CloseHandle(thread);

    ResetEvent(event);
    Sleep(100);

    ok(attached_thread_count == 2, "attached thread count should be 2\n");
    for (i = 0; i < attached_thread_count; i++)
    {
        ret = GetExitCodeThread(attached_thread[i], &code);
        trace("child: GetExitCodeThread(%lu) => %ld,%lu\n", i, ret, code);
        ok(ret == 1, "GetExitCodeThread returned %ld, expected 1\n", ret);
        ok(code == STILL_ACTIVE, "expected thread exit code STILL_ACTIVE, got %lu\n", code);
    }

    ret = WaitForSingleObject(attached_thread[0], 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
    ret = WaitForSingleObject(attached_thread[1], 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);

    ret = WaitForSingleObject(event, 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
    ret = WaitForSingleObject(mutex, 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
    ret = WaitForSingleObject(semaphore, 0);
    ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);

    ret = pRtlDllShutdownInProgress();
    ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);

    SetLastError(0xdeadbeef);
    process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, GetCurrentProcessId());
    ok(process != NULL, "OpenProcess error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = TerminateProcess(0, 195);
    ok(!ret, "TerminateProcess(0) should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    Sleep(100);

    affinity = 1;
    ret = pNtSetInformationProcess(process, ProcessAffinityMask, &affinity, sizeof(affinity));
    ok(!ret, "NtSetInformationProcess error %#lx\n", ret);

    switch (test_dll_phase)
    {
    case 0:
        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);

        trace("call NtTerminateProcess(0, 195)\n");
        ret = pNtTerminateProcess(0, 195);
        ok(!ret, "NtTerminateProcess error %#lx\n", ret);

        memset(&pbi, 0, sizeof(pbi));
        ret = pNtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
        ok(!ret, "NtQueryInformationProcess error %#lx\n", ret);
        ok(pbi.ExitStatus == STILL_ACTIVE || pbi.ExitStatus == 195,
           "expected STILL_ACTIVE, got %lu\n", pbi.ExitStatus);
        affinity = 1;
        ret = pNtSetInformationProcess(process, ProcessAffinityMask, &affinity, sizeof(affinity));
        ok(!ret, "NtSetInformationProcess error %#lx\n", ret);

        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);

        hmod = GetModuleHandleA(dll_name);
        ok(hmod != 0, "DLL should not be unloaded\n");

        SetLastError(0xdeadbeef);
        thread = CreateThread(NULL, 0, noop_thread_proc, &dummy, 0, &ret);
        ok(!thread, "CreateThread should fail\n");
        ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());

        trace("call LdrShutdownProcess()\n");
        pLdrRegisterDllNotification(0, ldr_notify_callback, NULL, &cookie);
        pLdrShutdownProcess();

        ret = pRtlDllShutdownInProgress();
        ok(ret, "RtlDllShutdownInProgress returned %ld\n", ret);

        hmod = GetModuleHandleA(dll_name);
        ok(hmod != 0, "DLL should not be unloaded\n");

        memset(&pbi, 0, sizeof(pbi));
        ret = pNtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
        ok(!ret, "NtQueryInformationProcess error %#lx\n", ret);
        ok(pbi.ExitStatus == STILL_ACTIVE || pbi.ExitStatus == 195,
           "expected STILL_ACTIVE, got %lu\n", pbi.ExitStatus);
        affinity = 1;
        ret = pNtSetInformationProcess(process, ProcessAffinityMask, &affinity, sizeof(affinity));
        ok(!ret, "NtSetInformationProcess error %#lx\n", ret);
        break;

    case 1: /* normal ExitProcess */
        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);
        break;

    case 2: /* ExitProcess will be called by the PROCESS_DETACH handler */
        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);

        trace("call FreeLibrary(%p)\n", hmod);
        SetLastError(0xdeadbeef);
        ret = FreeLibrary(hmod);
        ok(ret, "FreeLibrary error %ld\n", GetLastError());
        hmod = GetModuleHandleA(dll_name);
        ok(!hmod, "DLL should be unloaded\n");

        if (test_dll_phase == 2)
            ok(0, "FreeLibrary+ExitProcess should never return\n");

        ret = pRtlDllShutdownInProgress();
        ok(!ret, "RtlDllShutdownInProgress returned %ld\n", ret);

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
        ok(!ret, "NtTerminateProcess error %#lx\n", ret);

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

    case 7:
        trace("setting cs_lock_event\n");
        SetEvent(cs_lock_event);
        WaitForSingleObject(ack_event, 1000);
        ok(inside_cs_lock != 0, "inside_cs_lock is not set\n");

        *child_failures = winetest_get_failures();

        /* calling ExitProcess should not cause a deadlock */
        trace("call ExitProcess(199)\n");
        ExitProcess(199);
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
        ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
        ret = WaitForSingleObject(attached_thread[1], 100);
        ok(ret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %#lx\n", ret);
    }
    else
    {
        ret = WaitForSingleObject(attached_thread[0], 2000);
        ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#lx\n", ret);
        ret = WaitForSingleObject(attached_thread[1], 2000);
        ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %#lx\n", ret);
    }

    for (i = 0; i < attached_thread_count; i++)
    {
        ret = GetExitCodeThread(attached_thread[i], &code);
        trace("child: GetExitCodeThread(%lu) => %ld,%lu\n", i, ret, code);
        ok(ret == 1, "GetExitCodeThread returned %ld, expected 1\n", ret);
        ok(code == expected_code, "expected thread exit code %lu, got %lu\n", expected_code, code);
    }

    *child_failures = winetest_get_failures();

    trace("call ExitProcess(195)\n");
    ExitProcess(195);
}

static void test_ExitProcess(void)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__)
#include "pshpack1.h"
#ifdef __x86_64__
    static struct section_data
    {
        BYTE mov_rax[2];
        void *target;
        BYTE jmp_rax[2];
    } section_data = { { 0x48,0xb8 }, dll_entry_point, { 0xff,0xe0 } };
#elif defined(__i386__)
    static struct section_data
    {
        BYTE mov_eax;
        void *target;
        BYTE jmp_eax[2];
    } section_data = { 0xb8, dll_entry_point, { 0xff,0xe0 } };
#elif defined(__aarch64__)
    static struct section_data
    {
        DWORD ldr;  /* ldr x0,target */
        DWORD br;   /* br x0 */
        void *target;
    } section_data = { 0x58000040, 0xd61f0000, dll_entry_point };
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
    ok(ret, "WriteFile error %ld\n", GetLastError());

    nt_header = nt_header_template;
    nt_header.OptionalHeader.AddressOfEntryPoint = 0x1000;
    nt_header.OptionalHeader.SectionAlignment = 0x1000;
    nt_header.OptionalHeader.FileAlignment = 0x200;
    nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000;
    nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER);
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, &nt_header.OptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    section.SizeOfRawData = sizeof(section_data);
    section.PointerToRawData = nt_header.OptionalHeader.FileAlignment;
    section.VirtualAddress = nt_header.OptionalHeader.SectionAlignment;
    section.Misc.VirtualSize = sizeof(section_data);
    section.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE;
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, &section, sizeof(section), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    file_align = nt_header.OptionalHeader.FileAlignment - nt_header.OptionalHeader.SizeOfHeaders;
    assert(file_align < sizeof(filler));
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, filler, file_align, &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    target_offset = SetFilePointer(file, 0, NULL, FILE_CURRENT) + FIELD_OFFSET(struct section_data, target);

    /* section data */
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, &section_data, sizeof(section_data), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    CloseHandle(file);

    winetest_get_mainargs(&argv);

    /* phase 0 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %lu 0", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 10000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 195, "expected exit code 195, got %lu\n", ret);
    if (*child_failures)
    {
        trace("%ld failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* phase 1 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %lu 1", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 10000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 195, "expected exit code 195, got %lu\n", ret);
    if (*child_failures)
    {
        trace("%ld failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* phase 2 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %lu 2", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 10000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 197, "expected exit code 197, got %lu\n", ret);
    if (*child_failures)
    {
        trace("%ld failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* phase 3 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %lu 3", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 10000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 195, "expected exit code 195, got %lu\n", ret);
    if (*child_failures)
    {
        trace("%ld failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* phase 4 */
    if (pLdrLockLoaderLock && pLdrUnlockLoaderLock)
    {
        *child_failures = -1;
        sprintf(cmdline, "\"%s\" loader %s %lu 4", argv[0], dll_name, target_offset);
        ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
        ret = WaitForSingleObject(pi.hProcess, 10000);
        ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
        if (ret != WAIT_OBJECT_0) TerminateProcess(pi.hProcess, 0);
        GetExitCodeProcess(pi.hProcess, &ret);
        ok(ret == 198, "expected exit code 198, got %lu\n", ret);
        if (*child_failures)
        {
            trace("%ld failures in child process\n", *child_failures);
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
        sprintf(cmdline, "\"%s\" loader %s %lu 5", argv[0], dll_name, target_offset);
        ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
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
        ok(ret == 199, "expected exit code 199, got %lu\n", ret);
        if (*child_failures)
        {
            trace("%ld failures in child process\n", *child_failures);
            winetest_add_failures(*child_failures);
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else
        win_skip("RtlAcquirePebLock/RtlReleasePebLock are not available on this platform\n");

    /* phase 6 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %lu 6", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 5000);
    todo_wine
    ok(ret == WAIT_TIMEOUT, "child process should fail to terminate\n");
    if (ret != WAIT_OBJECT_0)
    {
        trace("terminating child process\n");
        TerminateProcess(pi.hProcess, 201);
    }
    ret = WaitForSingleObject(pi.hProcess, 1000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    GetExitCodeProcess(pi.hProcess, &ret);
    todo_wine
    ok(ret == 201, "expected exit code 201, got %lu\n", ret);
    if (*child_failures)
    {
        trace("%ld failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* phase 7 */
    *child_failures = -1;
    sprintf(cmdline, "\"%s\" loader %s %lu 7", argv[0], dll_name, target_offset);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 5000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    if (ret != WAIT_OBJECT_0)
    {
        trace("terminating child process\n");
        TerminateProcess(pi.hProcess, 199);
    }
    ret = WaitForSingleObject(pi.hProcess, 1000);
    ok(ret == WAIT_OBJECT_0, "child process failed to terminate\n");
    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 199, "expected exit code 199, got %lu\n", ret);
    if (*child_failures)
    {
        trace("%ld failures in child process\n", *child_failures);
        winetest_add_failures(*child_failures);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* test remote process termination */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(argv[0], NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess(%s) error %ld\n", argv[0], GetLastError());

    SetLastError(0xdeadbeef);
    addr = VirtualAllocEx(pi.hProcess, NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
    ok(addr != NULL, "VirtualAllocEx error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = VirtualProtectEx(pi.hProcess, addr, 4096, PAGE_READONLY, &old_prot);
    ok(ret, "VirtualProtectEx error %ld\n", GetLastError());
    ok(old_prot == PAGE_READWRITE, "expected PAGE_READWRITE, got %#lx\n", old_prot);
    SetLastError(0xdeadbeef);
    size = VirtualQueryEx(pi.hProcess, NULL, &mbi, sizeof(mbi));
    ok(size == sizeof(mbi), "VirtualQueryEx error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadProcessMemory(pi.hProcess, addr, buf, 4, &size);
    ok(ret, "ReadProcessMemory error %ld\n", GetLastError());
    ok(size == 4, "expected 4, got %Iu\n", size);

    SetLastError(0xdeadbeef);
    hmap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, NULL);
    ok(hmap != 0, "CreateFileMapping error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(GetCurrentProcess(), hmap, pi.hProcess, &hmap_dup,
                          0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "DuplicateHandle error %ld\n", GetLastError());

    offset.u.LowPart = 0;
    offset.u.HighPart = 0;
    addr = NULL;
    size = 0;
    ret = pNtMapViewOfSection(hmap, pi.hProcess, &addr, 0, 0, &offset,
                              &size, 1 /* ViewShare */, 0, PAGE_READONLY);
    ok(!ret, "NtMapViewOfSection error %#lx\n", ret);
    ret = pNtUnmapViewOfSection(pi.hProcess, addr);
    ok(!ret, "NtUnmapViewOfSection error %#lx\n", ret);

    SetLastError(0xdeadbeef);
    thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void *)0xdeadbeef, NULL, CREATE_SUSPENDED, &ret);
    ok(thread != 0, "CreateRemoteThread error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = GetThreadContext(thread, &ctx);
    ok(ret, "GetThreadContext error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = SetThreadContext(thread, &ctx);
    ok(ret, "SetThreadContext error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = SetThreadPriority(thread, 0);
    ok(ret, "SetThreadPriority error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = TerminateThread(thread, 199);
    ok(ret, "TerminateThread error %ld\n", GetLastError());
    /* Calling GetExitCodeThread() without waiting for thread termination
     * leads to different results due to a race condition.
     */
    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed: %lx\n", ret);
    GetExitCodeThread(thread, &ret);
    ok(ret == 199, "expected exit code 199, got %lu\n", ret);

    SetLastError(0xdeadbeef);
    ret = TerminateProcess(pi.hProcess, 198);
    ok(ret, "TerminateProcess error %ld\n", GetLastError());
    /* Checking process state without waiting for process termination
     * leads to different results due to a race condition.
     */
    ret = WaitForSingleObject(pi.hProcess, 1000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed: %lx\n", ret);

    SetLastError(0xdeadbeef);
    process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, pi.dwProcessId);
    ok(process != NULL, "OpenProcess error %ld\n", GetLastError());
    CloseHandle(process);

    memset(&pbi, 0, sizeof(pbi));
    ret = pNtQueryInformationProcess(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok(!ret, "NtQueryInformationProcess error %#lx\n", ret);
    ok(pbi.ExitStatus == 198, "expected 198, got %lu\n", pbi.ExitStatus);
    affinity = 1;
    ret = pNtSetInformationProcess(pi.hProcess, ProcessAffinityMask, &affinity, sizeof(affinity));
    ok(ret == STATUS_PROCESS_IS_TERMINATING, "expected STATUS_PROCESS_IS_TERMINATING, got %#lx\n", ret);

    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = GetThreadContext(thread, &ctx);
    ok(!ret, "GetThreadContext should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_GEN_FAILURE /* win7 64-bit */ ||
       GetLastError() == ERROR_INVALID_FUNCTION /* vista 64-bit */ ||
       GetLastError() == ERROR_ACCESS_DENIED /* Win10 32-bit */,
       "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = SetThreadContext(thread, &ctx);
    ok(!ret, "SetThreadContext should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED ||
       GetLastError() == ERROR_GEN_FAILURE /* win7 64-bit */ ||
       GetLastError() == ERROR_INVALID_FUNCTION /* vista 64-bit */,
       "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = SetThreadPriority(thread, 0);
    ok(ret, "SetThreadPriority error %ld\n", GetLastError());
    CloseHandle(thread);

    ret = WaitForSingleObject(pi.hThread, 1000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed: %lx\n", ret);
    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = GetThreadContext(pi.hThread, &ctx);
    ok(!ret, "GetThreadContext should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_GEN_FAILURE /* win7 64-bit */ ||
       GetLastError() == ERROR_INVALID_FUNCTION /* vista 64-bit */ ||
       GetLastError() == ERROR_ACCESS_DENIED /* Win10 32-bit */,
       "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ctx.ContextFlags = CONTEXT_INTEGER;
    ret = SetThreadContext(pi.hThread, &ctx);
    ok(!ret, "SetThreadContext should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED ||
       GetLastError() == ERROR_GEN_FAILURE /* win7 64-bit */ ||
       GetLastError() == ERROR_INVALID_FUNCTION /* vista 64-bit */,
       "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = VirtualProtectEx(pi.hProcess, addr, 4096, PAGE_READWRITE, &old_prot);
    ok(!ret, "VirtualProtectEx should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    size = 0;
    ret = ReadProcessMemory(pi.hProcess, addr, buf, 4, &size);
    ok(!ret, "ReadProcessMemory should fail\n");
    ok(GetLastError() == ERROR_PARTIAL_COPY || GetLastError() == ERROR_ACCESS_DENIED,
       "expected ERROR_PARTIAL_COPY, got %ld\n", GetLastError());
    ok(!size, "expected 0, got %Iu\n", size);
    SetLastError(0xdeadbeef);
    ret = VirtualFreeEx(pi.hProcess, addr, 0, MEM_RELEASE);
    ok(!ret, "VirtualFreeEx should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    addr = VirtualAllocEx(pi.hProcess, NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
    ok(!addr, "VirtualAllocEx should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    size = VirtualQueryEx(pi.hProcess, NULL, &mbi, sizeof(mbi));
    ok(!size, "VirtualQueryEx should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());

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
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());

    offset.u.LowPart = 0;
    offset.u.HighPart = 0;
    addr = NULL;
    size = 0;
    ret = pNtMapViewOfSection(hmap, pi.hProcess, &addr, 0, 0, &offset,
                              &size, 1 /* ViewShare */, 0, PAGE_READONLY);
    ok(ret == STATUS_PROCESS_IS_TERMINATING, "expected STATUS_PROCESS_IS_TERMINATING, got %#lx\n", ret);

    SetLastError(0xdeadbeef);
    thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void *)0xdeadbeef, NULL, CREATE_SUSPENDED, &ret);
    ok(!thread, "CreateRemoteThread should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DebugActiveProcess(pi.dwProcessId);
    ok(!ret, "DebugActiveProcess should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED /* 64-bit */ || GetLastError() == ERROR_NOT_SUPPORTED /* 32-bit */,
      "ERROR_ACCESS_DENIED, got %ld\n", GetLastError());

    GetExitCodeProcess(pi.hProcess, &ret);
    ok(ret == 198, "expected exit code 198, got %lu\n", ret);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    ret = DeleteFileA(dll_name);
    ok(ret, "DeleteFile error %ld\n", GetLastError());
#else
    skip("x86 specific ExitProcess test\n");
#endif
}

static PVOID WINAPI failuredllhook(ULONG ul, DELAYLOAD_INFO* pd)
{
    ok(ul == 4, "expected 4, got %lu\n", ul);
    ok(!!pd, "no delayload info supplied\n");
    if (pd)
    {
        ok(pd->Size == sizeof(*pd), "got %lu\n", pd->Size);
        ok(!!pd->DelayloadDescriptor, "no DelayloadDescriptor supplied\n");
        if (pd->DelayloadDescriptor)
        {
            ok(pd->DelayloadDescriptor->Attributes.AllAttributes == 1,
               "expected 1, got %lu\n", pd->DelayloadDescriptor->Attributes.AllAttributes);
            ok(pd->DelayloadDescriptor->DllNameRVA == 0x2000,
               "expected 0x2000, got %lx\n", pd->DelayloadDescriptor->DllNameRVA);
            ok(pd->DelayloadDescriptor->ModuleHandleRVA == 0x201a,
               "expected 0x201a, got %lx\n", pd->DelayloadDescriptor->ModuleHandleRVA);
            ok(pd->DelayloadDescriptor->ImportAddressTableRVA > pd->DelayloadDescriptor->ModuleHandleRVA,
               "expected %lx > %lx\n", pd->DelayloadDescriptor->ImportAddressTableRVA,
               pd->DelayloadDescriptor->ModuleHandleRVA);
            ok(pd->DelayloadDescriptor->ImportNameTableRVA > pd->DelayloadDescriptor->ImportAddressTableRVA,
               "expected %lx > %lx\n", pd->DelayloadDescriptor->ImportNameTableRVA,
               pd->DelayloadDescriptor->ImportAddressTableRVA);
            ok(pd->DelayloadDescriptor->BoundImportAddressTableRVA == 0,
               "expected 0, got %lx\n", pd->DelayloadDescriptor->BoundImportAddressTableRVA);
            ok(pd->DelayloadDescriptor->UnloadInformationTableRVA == 0,
               "expected 0, got %lx\n", pd->DelayloadDescriptor->UnloadInformationTableRVA);
            ok(pd->DelayloadDescriptor->TimeDateStamp == 0,
               "expected 0, got %lx\n", pd->DelayloadDescriptor->TimeDateStamp);
        }

        ok(!!pd->ThunkAddress, "no ThunkAddress supplied\n");
        if (pd->ThunkAddress)
            ok(pd->ThunkAddress->u1.Ordinal, "no ThunkAddress value supplied\n");

        ok(!!pd->TargetDllName, "no TargetDllName supplied\n");
        if (pd->TargetDllName)
            ok(!strcmp(pd->TargetDllName, "secur32.dll"),
               "expected \"secur32.dll\", got \"%s\"\n", pd->TargetDllName);

        ok(pd->TargetApiDescriptor.ImportDescribedByName == 0,
           "expected 0, got %lx\n", pd->TargetApiDescriptor.ImportDescribedByName);
        ok(pd->TargetApiDescriptor.Description.Ordinal == 0 ||
           pd->TargetApiDescriptor.Description.Ordinal == 999,
           "expected 0, got %lx\n", pd->TargetApiDescriptor.Description.Ordinal);

        ok(!!pd->TargetModuleBase, "no TargetModuleBase supplied\n");
        ok(pd->Unused == NULL, "expected NULL, got %p\n", pd->Unused);
        ok(pd->LastError, "no LastError supplied\n");
    }
    cb_count++;
    return (void*)0xdeadbeef;
}

static PVOID WINAPI failuresyshook(const char *dll, const char *function)
{
    ok(!strcmp(dll, "secur32.dll"), "wrong dll: %s\n", dll);
    ok(!((ULONG_PTR)function >> 16), "expected ordinal, got %p\n", function);
    cb_count_sys++;
    return (void*)0x12345678;
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
        ok(GetLastError() == 0xdeadbeef, "GetLastError changed to %lx\n", GetLastError());

        cb_count = 0;
        SetLastError(0xdeadbeef);
        ok(!pResolveDelayLoadedAPI(NULL, NULL, failuredllhook, NULL, NULL, 0),
           "ResolveDelayLoadedAPI succeeded\n");
        ok(GetLastError() == 0xdeadbeef, "GetLastError changed to %lx\n", GetLastError());
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
    ok(ret, "WriteFile error %ld\n", GetLastError());

    nt_header = nt_header_template;
    nt_header.FileHeader.NumberOfSections = 2;
    nt_header.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);

    nt_header.OptionalHeader.SectionAlignment = 0x1000;
    nt_header.OptionalHeader.FileAlignment = 0x1000;
    nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x2200;
    nt_header.OptionalHeader.SizeOfHeaders = sizeof(dos_header) + sizeof(nt_header) + 2 * sizeof(IMAGE_SECTION_HEADER);
    nt_header.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress = 0x1000;
    nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size = 2 * sizeof(idd);

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &nt_header.OptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    /* sections */
    section.PointerToRawData = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;
    section.VirtualAddress = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;
    section.Misc.VirtualSize = 0x1000;
    section.SizeOfRawData = 2 * sizeof(idd);
    section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &section, sizeof(section), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    section.PointerToRawData = 0x2000;
    section.VirtualAddress = 0x2000;
    i = ARRAY_SIZE(td);
    section.SizeOfRawData = sizeof(test_dll) + sizeof(hint) + sizeof(test_func) + sizeof(HMODULE) +
                               2 * (i + 1) * sizeof(IMAGE_THUNK_DATA);
    ok(section.SizeOfRawData <= 0x1000, "Too much tests, add a new section!\n");
    section.Misc.VirtualSize = 0x1000;
    section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &section, sizeof(section), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

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
    ok(ret, "WriteFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, filler, sizeof(idd), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    /* fill up to extended delay data */
    SetFilePointer( hfile, idd.DllNameRVA, NULL, SEEK_SET );

    /* extended delay data */
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, test_dll, sizeof(test_dll), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &hint, sizeof(hint), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, test_func, sizeof(test_func), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    SetFilePointer( hfile, idd.ImportAddressTableRVA, NULL, SEEK_SET );

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        /* 0x1a00 is an empty space between delay data and extended delay data, real thunks are not necessary */
        itd32.u1.Function = nt_header.OptionalHeader.ImageBase + 0x1a00 + i * 0x20;
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &itd32, sizeof(itd32), &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());
    }

    itd32.u1.Function = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &itd32, sizeof(itd32), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        if (td[i].func)
            itd32.u1.AddressOfData = idd.DllNameRVA + sizeof(test_dll);
        else
            itd32.u1.Ordinal = td[i].ordinal;
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &itd32, sizeof(itd32), &dummy, NULL);
        ok(ret, "WriteFile error %ld\n", GetLastError());
    }

    itd32.u1.Ordinal = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, &itd32, sizeof(itd32), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    /* fill up to eof */
    SetFilePointer( hfile, section.VirtualAddress + section.Misc.VirtualSize, NULL, SEEK_SET );
    SetEndOfFile( hfile );
    CloseHandle(hfile);

    SetLastError(0xdeadbeef);
    hlib = LoadLibraryA(dll_name);
    ok(hlib != NULL, "LoadLibrary error %lu\n", GetLastError());
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

        for (i = 0; i < ARRAY_SIZE(td); i++)
        {
            void *ret, *load;

            /* relocate thunk address by hand since we don't generate reloc records */
            itda[i].u1.AddressOfData += (char *)hlib - (char *)nt_header.OptionalHeader.ImageBase;

            if (IMAGE_SNAP_BY_ORDINAL(itdn[i].u1.Ordinal))
                load = (void *)GetProcAddress(htarget, (LPSTR)IMAGE_ORDINAL(itdn[i].u1.Ordinal));
            else
            {
                const IMAGE_IMPORT_BY_NAME* iibn = RVAToAddr(itdn[i].u1.AddressOfData, hlib);
                load = (void *)GetProcAddress(htarget, (char*)iibn->Name);
            }

            /* test without failure dll callback */
            cb_count = cb_count_sys = 0;
            ret = pResolveDelayLoadedAPI(hlib, delaydir, NULL, failuresyshook, &itda[i], 0);
            if (td[i].succeeds)
            {
                ok(ret != NULL, "Test %lu: ResolveDelayLoadedAPI failed\n", i);
                ok(ret == load, "Test %lu: expected %p, got %p\n", i, load, ret);
                ok(ret == (void*)itda[i].u1.AddressOfData, "Test %lu: expected %p, got %p\n",
                   i, ret, (void*)itda[i].u1.AddressOfData);
                ok(!cb_count, "Test %lu: Wrong callback count: %d\n", i, cb_count);
                ok(!cb_count_sys, "Test %lu: Wrong sys callback count: %d\n", i, cb_count_sys);
            }
            else
            {
                ok(ret == (void*)0x12345678, "Test %lu: ResolveDelayLoadedAPI succeeded with %p\n", i, ret);
                ok(!cb_count, "Test %lu: Wrong callback count: %d\n", i, cb_count);
                ok(cb_count_sys == 1, "Test %lu: Wrong sys callback count: %d\n", i, cb_count_sys);
            }

            /* test with failure dll callback */
            cb_count = cb_count_sys = 0;
            ret = pResolveDelayLoadedAPI(hlib, delaydir, failuredllhook, failuresyshook, &itda[i], 0);
            if (td[i].succeeds)
            {
                ok(ret != NULL, "Test %lu: ResolveDelayLoadedAPI failed\n", i);
                ok(ret == load, "Test %lu: expected %p, got %p\n", i, load, ret);
                ok(ret == (void*)itda[i].u1.AddressOfData, "Test %lu: expected %p, got %p\n",
                   i, ret, (void*)itda[i].u1.AddressOfData);
                ok(!cb_count, "Test %lu: Wrong callback count: %d\n", i, cb_count);
                ok(!cb_count_sys, "Test %lu: Wrong sys callback count: %d\n", i, cb_count_sys);
            }
            else
            {
                if (ret == (void*)0x12345678)
                {
                    /* Win10+ sometimes buffers the address of the stub function */
                    ok(!cb_count, "Test %lu: Wrong callback count: %d\n", i, cb_count);
                    ok(!cb_count_sys, "Test %lu: Wrong sys callback count: %d\n", i, cb_count_sys);
                }
                else if (ret == (void*)0xdeadbeef)
                {
                    ok(cb_count == 1, "Test %lu: Wrong callback count: %d\n", i, cb_count);
                    ok(!cb_count_sys, "Test %lu: Wrong sys callback count: %d\n", i, cb_count_sys);
                }
                else
                    ok(0, "Test %lu: ResolveDelayLoadedAPI succeeded with %p\n", i, ret);
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

static BOOL is_path_made_of(const char *filename, const char *pfx, const char *sfx)
{
    const size_t len = strlen(pfx);
    return !strncasecmp(filename, pfx, len) && filename[len] == '\\' &&
        !strcasecmp(filename + len + 1, sfx);
}

static void test_wow64_redirection_for_dll(const char *libname, BOOL will_fail)
{
    HMODULE lib;
    char buf[256];
    const char *modname;

    if (!GetModuleHandleA(libname))
    {
        lib = LoadLibraryExA(libname, NULL, 0);
        ok(lib != NULL, "Loading %s should succeed with WOW64 redirection disabled\n", libname);
        /* Win 7/2008R2 return the un-redirected path (i.e. c:\windows\system32\dwrite.dll), test loading it. */
        GetModuleFileNameA(lib, buf, sizeof(buf));
        FreeLibrary(lib);
        lib = LoadLibraryExA(buf, NULL, 0);
        ok(lib != NULL, "Loading %s from full path should succeed with WOW64 redirection disabled\n", libname);
        if (lib)
            FreeLibrary(lib);
        modname = strrchr(libname, '\\');
        modname = modname ? modname + 1 : libname;
        todo_wine_if(will_fail)
            ok(is_path_made_of(buf, system_dir, modname) ||
               /* Win7 report from syswow64 */ broken(is_path_made_of(buf, syswow_dir, modname)),
               "Unexpected loaded DLL name %s for %s\n", buf, libname);
    }
    else
    {
        skip("%s was already loaded in the process\n", libname);
    }
}

static void test_wow64_redirection(void)
{
    void *OldValue;
    char buffer[MAX_PATH];
    static const char *dlls[] = {"wlanapi.dll", "dxgi.dll", "dwrite.dll"};
    unsigned i;

    if (!is_wow64)
        return;

    /* Disable FS redirection, then test loading system libraries (pick ones that shouldn't
     * already be loaded in this process).
     */
    ok(pWow64DisableWow64FsRedirection(&OldValue), "Disabling FS redirection failed\n");
    for (i = 0; i < ARRAY_SIZE(dlls); i++)
    {
        test_wow64_redirection_for_dll(dlls[i], FALSE);
        /* even absolute paths to syswow64 are loaded with path to system32 */
        snprintf(buffer, ARRAY_SIZE(buffer), "%s\\%s", syswow_dir, dlls[i]);
        test_wow64_redirection_for_dll(buffer, TRUE);
    }

    ok(pWow64RevertWow64FsRedirection(OldValue), "Re-enabling FS redirection failed\n");
    /* and results don't depend whether redirection is enabled or not */
    for (i = 0; i < ARRAY_SIZE(dlls); i++)
    {
        test_wow64_redirection_for_dll(dlls[i], FALSE);
        snprintf(buffer, ARRAY_SIZE(buffer), "%s\\%s", syswow_dir, dlls[i]);
        test_wow64_redirection_for_dll(buffer, TRUE);
    }
}

static void test_dll_file( const char *name )
{
    HMODULE module = GetModuleHandleA( name );
    IMAGE_NT_HEADERS *nt, *nt_file;
    IMAGE_SECTION_HEADER *sec, *sec_file;
    char path[MAX_PATH];
    HANDLE file, mapping;
    int i = 0;
    void *ptr;

    GetModuleFileNameA( module, path, MAX_PATH );
    file = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "can't open '%s': %lu\n", path, GetLastError() );

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 0, NULL );
    ok( mapping != NULL, "%s: CreateFileMappingW failed err %lu\n", name, GetLastError() );
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    ok( ptr != NULL, "%s: MapViewOfFile failed err %lu\n", name, GetLastError() );
    CloseHandle( mapping );
    CloseHandle( file );

    nt = pRtlImageNtHeader( module );
    nt_file = pRtlImageNtHeader( ptr );
    ok( nt_file != NULL, "%s: invalid header\n", path );
#define OK_FIELD(x, f) ok( nt->x == nt_file->x, "%s:%u: wrong " #x " " f " / " f "\n", name, i, nt->x, nt_file->x )
    OK_FIELD( FileHeader.NumberOfSections, "%x" );
    OK_FIELD( OptionalHeader.AddressOfEntryPoint, "%lx" );
    OK_FIELD( OptionalHeader.NumberOfRvaAndSizes, "%lx" );
    for (i = 0; i < nt->OptionalHeader.NumberOfRvaAndSizes; i++)
    {
        OK_FIELD( OptionalHeader.DataDirectory[i].VirtualAddress, "%lx" );
        OK_FIELD( OptionalHeader.DataDirectory[i].Size, "%lx" );
    }
    sec = IMAGE_FIRST_SECTION( nt );
    sec_file = IMAGE_FIRST_SECTION( nt_file );
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
        ok( !memcmp( sec + i, sec_file + i, sizeof(*sec) ), "%s: wrong section %d\n", name, i );
    UnmapViewOfFile( ptr );
#undef OK_FIELD
}

static void test_LoadPackagedLibrary(void)
{
    HMODULE h;

    if (!pLoadPackagedLibrary)
    {
        win_skip("LoadPackagedLibrary is not available.\n");
        return;
    }

    SetLastError( 0xdeadbeef );
    h = pLoadPackagedLibrary(L"kernel32.dll", 0);
    ok(!h && GetLastError() == APPMODEL_ERROR_NO_PACKAGE, "Got unexpected handle %p, GetLastError() %lu.\n",
            h, GetLastError());
}

static void test_Wow64Transition(void)
{
    char buffer[400];
    MEMORY_SECTION_NAME *name = (MEMORY_SECTION_NAME *)buffer;
    const WCHAR *filepart;
    void **pWow64Transition;
    NTSTATUS status;

    if (!(pWow64Transition = (void *)GetProcAddress(GetModuleHandleA("ntdll"), "Wow64Transition")))
    {
        skip("Wow64Transition is not present\n");
        return;
    }
    if (!is_wow64)
    {
        skip("Wow64Transition is not patched\n");
        return;
    }

    status = NtQueryVirtualMemory(GetCurrentProcess(), *pWow64Transition,
                                  MemoryMappedFilenameInformation, name, sizeof(buffer), NULL);
    ok(!status, "got %#lx\n", status);
    filepart = name->SectionFileName.Buffer + name->SectionFileName.Length / sizeof(WCHAR);
    while (*filepart != '\\') --filepart;
    ok(!wcsnicmp(filepart, L"\\wow64cpu.dll", wcslen(L"\\wow64cpu.dll")), "got file name %s\n",
            debugstr_wn(name->SectionFileName.Buffer, name->SectionFileName.Length / sizeof(WCHAR)));
}

START_TEST(loader)
{
    int argc;
    char **argv;
    HANDLE ntdll, mapping, kernel32;
    SYSTEM_INFO si;
    DWORD len;

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
    pLdrLoadDll = (void *)GetProcAddress(ntdll, "LdrLoadDll");
    pLdrUnloadDll = (void *)GetProcAddress(ntdll, "LdrUnloadDll");
    pRtlInitUnicodeString = (void *)GetProcAddress(ntdll, "RtlInitUnicodeString");
    pRtlAcquirePebLock = (void *)GetProcAddress(ntdll, "RtlAcquirePebLock");
    pRtlReleasePebLock = (void *)GetProcAddress(ntdll, "RtlReleasePebLock");
    pRtlImageDirectoryEntryToData = (void *)GetProcAddress(ntdll, "RtlImageDirectoryEntryToData");
    pRtlImageNtHeader = (void *)GetProcAddress(ntdll, "RtlImageNtHeader");
    pLdrRegisterDllNotification = (void *)GetProcAddress(ntdll, "LdrRegisterDllNotification");
    pFlsAlloc = (void *)GetProcAddress(kernel32, "FlsAlloc");
    pFlsSetValue = (void *)GetProcAddress(kernel32, "FlsSetValue");
    pFlsGetValue = (void *)GetProcAddress(kernel32, "FlsGetValue");
    pFlsFree = (void *)GetProcAddress(kernel32, "FlsFree");
    pIsWow64Process = (void *)GetProcAddress(kernel32, "IsWow64Process");
    pWow64DisableWow64FsRedirection = (void *)GetProcAddress(kernel32, "Wow64DisableWow64FsRedirection");
    pWow64RevertWow64FsRedirection = (void *)GetProcAddress(kernel32, "Wow64RevertWow64FsRedirection");
    pResolveDelayLoadedAPI = (void *)GetProcAddress(kernel32, "ResolveDelayLoadedAPI");
    pLoadPackagedLibrary = (void *)GetProcAddress(kernel32, "LoadPackagedLibrary");

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

    len = GetSystemDirectoryA(system_dir, ARRAY_SIZE(system_dir));
    ok(len && len < ARRAY_SIZE(system_dir), "Couldn't get system directory: %lu\n", GetLastError());
    if (is_wow64)
    {
        len = GetSystemWow64DirectoryA(syswow_dir, ARRAY_SIZE(syswow_dir));
        ok(len && len < ARRAY_SIZE(syswow_dir), "Couldn't get wow directory: %lu\n", GetLastError());
    }

    test_filenames();
    test_ResolveDelayLoadedAPI();
    test_ImportDescriptors();
    test_section_access();
    test_import_resolution();
    test_export_forwarder_dep_chain();
    test_ExitProcess();
    test_InMemoryOrderModuleList();
    test_LoadPackagedLibrary();
    test_wow64_redirection();
    test_dll_file( "ntdll.dll" );
    test_dll_file( "kernel32.dll" );
    test_dll_file( "advapi32.dll" );
    test_dll_file( "user32.dll" );
    test_Wow64Transition();
    /* loader test must be last, it can corrupt the internal loader state on Windows */
    test_Loader();
}
