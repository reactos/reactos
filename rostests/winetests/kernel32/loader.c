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
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/test.h"

#define ALIGN_SIZE(size, alignment) (((size) + (alignment - 1)) & ~((alignment - 1)))

static NTSTATUS (WINAPI *pNtMapViewOfSection)(HANDLE, HANDLE, PVOID *, ULONG, SIZE_T, const LARGE_INTEGER *, SIZE_T *, ULONG, ULONG, ULONG);
static NTSTATUS (WINAPI *pNtUnmapViewOfSection)(HANDLE, PVOID);

static PVOID RVAToAddr(DWORD_PTR rva, HMODULE module)
{
    if (rva == 0)
        return NULL;
    return ((char*) module) + rva;
}

static const struct
{
    WORD e_magic;      /* 00: MZ Header signature */
    WORD unused[29];
    DWORD e_lfanew;    /* 3c: Offset to extended header */
} dos_header =
{
    IMAGE_DOS_SIGNATURE, { 0 }, sizeof(dos_header)
};

static IMAGE_NT_HEADERS nt_header =
{
    IMAGE_NT_SIGNATURE, /* Signature */
    {
#if defined __i386__
      IMAGE_FILE_MACHINE_I386, /* Machine */
#elif defined __x86_64__
      IMAGE_FILE_MACHINE_AMD64, /* Machine */
#elif defined __powerpc__
      IMAGE_FILE_MACHINE_POWERPC, /* Machine */
#elif defined __sparc__
      IMAGE_FILE_MACHINE_SPARC, /* Machine */
#elif defined __arm__
      IMAGE_FILE_MACHINE_ARMNT, /* Machine */
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
      sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000, /* SizeOfImage */
      sizeof(dos_header) + sizeof(nt_header), /* SizeOfHeaders */
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
    { 0x10 }, /* Misc */
    0, /* VirtualAddress */
    0x0a, /* SizeOfRawData */
    0, /* PointerToRawData */
    0, /* PointerToRelocations */
    0, /* PointerToLinenumbers */
    0, /* NumberOfRelocations */
    0, /* NumberOfLinenumbers */
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ, /* Characteristics */
};

static void test_Loader(void)
{
    static const struct test_data
    {
        const void *dos_header;
        DWORD size_of_dos_header;
        WORD number_of_sections, size_of_optional_header;
        DWORD section_alignment, file_alignment;
        DWORD size_of_image, size_of_headers;
        DWORD errors[4]; /* 0 means LoadLibrary should succeed */
    } td[] =
    {
        { &dos_header, sizeof(dos_header),
          1, 0, 0, 0, 0, 0,
          { ERROR_BAD_EXE_FORMAT }
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0xe00,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_BAD_EXE_FORMAT } /* XP doesn't like too small image size */
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_SUCCESS }
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          0x1f00,
          0x1000,
          { ERROR_SUCCESS }
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x200, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_SUCCESS, ERROR_INVALID_ADDRESS } /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x200, 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_BAD_EXE_FORMAT } /* XP doesn't like alignments */
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_SUCCESS }
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          0x200,
          { ERROR_SUCCESS }
        },
        /* Mandatory are all fields up to SizeOfHeaders, everything else
         * is really optional (at least that's true for XP).
         */
        { &dos_header, sizeof(dos_header),
          1, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          sizeof(dos_header) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(IMAGE_SECTION_HEADER) + 0x10,
          sizeof(dos_header) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(IMAGE_SECTION_HEADER),
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT, ERROR_INVALID_ADDRESS,
            ERROR_NOACCESS }
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0xd0, /* beyond of the end of file */
          0xc0, /* beyond of the end of file */
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0x1000,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          1,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
#if 0 /* not power of 2 alignments need more test cases */
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x300, 0x300,
          1,
          0,
          { ERROR_BAD_EXE_FORMAT } /* alignment is not power of 2 */
        },
#endif
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 4, 4,
          1,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 1, 1,
          1,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0,
          0,
          { ERROR_BAD_EXE_FORMAT } /* image size == 0 -> failure */
        },
        /* the following data mimics the PE image which upack creates */
        { &dos_header, 0x10,
          1, 0x148, 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          0x200,
          { ERROR_SUCCESS }
        },
        /* Minimal PE image that XP is able to load: 92 bytes */
        { &dos_header, 0x04,
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum),
          0x04 /* also serves as e_lfanew in the truncated MZ header */, 0x04,
          1,
          0,
          { ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT } /* vista is more strict */
        }
    };
    static const char filler[0x1000];
    static const char section_data[0x10] = "section data";
    int i;
    DWORD dummy, file_size, file_align;
    HANDLE hfile;
    HMODULE hlib, hlib_as_data_file;
    SYSTEM_INFO si;
    char temp_path[MAX_PATH];
    char dll_name[MAX_PATH];
    SIZE_T size;
    BOOL ret;

    GetSystemInfo(&si);
    trace("system page size 0x%04x\n", si.dwPageSize);

    /* prevent displaying of the "Unable to load this DLL" message box */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    GetTempPath(MAX_PATH, temp_path);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        GetTempFileName(temp_path, "ldr", 0, dll_name);

        /*trace("creating %s\n", dll_name);*/
        hfile = CreateFileA(dll_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
        if (hfile == INVALID_HANDLE_VALUE)
        {
            ok(0, "could not create %s\n", dll_name);
            break;
        }

        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, td[i].dos_header, td[i].size_of_dos_header, &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());

        nt_header.FileHeader.NumberOfSections = td[i].number_of_sections;
        nt_header.FileHeader.SizeOfOptionalHeader = td[i].size_of_optional_header;

        nt_header.OptionalHeader.SectionAlignment = td[i].section_alignment;
        nt_header.OptionalHeader.FileAlignment = td[i].file_alignment;
        nt_header.OptionalHeader.SizeOfImage = td[i].size_of_image;
        nt_header.OptionalHeader.SizeOfHeaders = td[i].size_of_headers;
        SetLastError(0xdeadbeef);
        ret = WriteFile(hfile, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL);
        ok(ret, "WriteFile error %d\n", GetLastError());

        if (nt_header.FileHeader.SizeOfOptionalHeader)
        {
            SetLastError(0xdeadbeef);
            ret = WriteFile(hfile, &nt_header.OptionalHeader,
                            min(nt_header.FileHeader.SizeOfOptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER)),
                            &dummy, NULL);
            ok(ret, "WriteFile error %d\n", GetLastError());
            if (nt_header.FileHeader.SizeOfOptionalHeader > sizeof(IMAGE_OPTIONAL_HEADER))
            {
                file_align = nt_header.FileHeader.SizeOfOptionalHeader - sizeof(IMAGE_OPTIONAL_HEADER);
                assert(file_align < sizeof(filler));
                SetLastError(0xdeadbeef);
                ret = WriteFile(hfile, filler, file_align, &dummy, NULL);
                ok(ret, "WriteFile error %d\n", GetLastError());
            }
        }

        assert(nt_header.FileHeader.NumberOfSections <= 1);
        if (nt_header.FileHeader.NumberOfSections)
        {
            if (nt_header.OptionalHeader.SectionAlignment >= si.dwPageSize)
            {
                section.PointerToRawData = td[i].size_of_dos_header;
                section.VirtualAddress = nt_header.OptionalHeader.SectionAlignment;
                section.Misc.VirtualSize = section.SizeOfRawData * 10;
            }
            else
            {
                section.PointerToRawData = nt_header.OptionalHeader.SizeOfHeaders;
                section.VirtualAddress = nt_header.OptionalHeader.SizeOfHeaders;
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

        file_size = GetFileSize(hfile, NULL);
        CloseHandle(hfile);

        SetLastError(0xdeadbeef);
        hlib = LoadLibrary(dll_name);
        if (hlib)
        {
            MEMORY_BASIC_INFORMATION info;

            ok( td[i].errors[0] == ERROR_SUCCESS, "%d: should have failed\n", i );

            SetLastError(0xdeadbeef);
            size = VirtualQuery(hlib, &info, sizeof(info));
            ok(size == sizeof(info),
                "%d: VirtualQuery error %d\n", i, GetLastError());
            ok(info.BaseAddress == hlib, "%d: %p != %p\n", i, info.BaseAddress, hlib);
            ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
            ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
            ok(info.RegionSize == ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize), "%d: got %lx != expected %x\n",
               i, info.RegionSize, ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize));
            ok(info.State == MEM_COMMIT, "%d: %x != MEM_COMMIT\n", i, info.State);
            if (nt_header.OptionalHeader.SectionAlignment < si.dwPageSize)
                ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.Protect);
            else
                ok(info.Protect == PAGE_READONLY, "%d: %x != PAGE_READONLY\n", i, info.Protect);
            ok(info.Type == SEC_IMAGE, "%d: %x != SEC_IMAGE\n", i, info.Type);

            SetLastError(0xdeadbeef);
            size = VirtualQuery((char *)hlib + info.RegionSize, &info, sizeof(info));
            ok(size == sizeof(info),
                "%d: VirtualQuery error %d\n", i, GetLastError());
            if (nt_header.OptionalHeader.SectionAlignment == si.dwPageSize ||
                nt_header.OptionalHeader.SectionAlignment == nt_header.OptionalHeader.FileAlignment)
            {
                ok(info.BaseAddress == (char *)hlib + ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize), "%d: got %p != expected %p\n",
                   i, info.BaseAddress, (char *)hlib + ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize));
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
                ok(info.RegionSize == ALIGN_SIZE(file_size, si.dwPageSize), "%d: got %lx != expected %x\n",
                   i, info.RegionSize, ALIGN_SIZE(file_size, si.dwPageSize));
                ok(info.State == MEM_COMMIT, "%d: %x != MEM_COMMIT\n", i, info.State);
                ok(info.Protect == PAGE_READONLY, "%d: %x != PAGE_READONLY\n", i, info.Protect);
                ok(info.Type == SEC_IMAGE, "%d: %x != SEC_IMAGE\n", i, info.Type);
            }

            /* header: check the zeroing of alignment */
            if (nt_header.OptionalHeader.SectionAlignment >= si.dwPageSize)
            {
                const char *start;
                int size;

                start = (const char *)hlib + nt_header.OptionalHeader.SizeOfHeaders;
                size = ALIGN_SIZE((ULONG_PTR)start, si.dwPageSize) - (ULONG_PTR)start;
                ok(!memcmp(start, filler, size), "%d: header alignment is not cleared\n", i);
            }

            if (nt_header.FileHeader.NumberOfSections)
            {
                SetLastError(0xdeadbeef);
                size = VirtualQuery((char *)hlib + section.VirtualAddress, &info, sizeof(info));
                ok(size == sizeof(info),
                    "%d: VirtualQuery error %d\n", i, GetLastError());
                if (nt_header.OptionalHeader.SectionAlignment < si.dwPageSize)
                {
                    ok(info.BaseAddress == hlib, "%d: got %p != expected %p\n", i, info.BaseAddress, hlib);
                    ok(info.RegionSize == ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize), "%d: got %lx != expected %x\n",
                       i, info.RegionSize, ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize));
                    ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.Protect);
                }
                else
                {
                    ok(info.BaseAddress == (char *)hlib + section.VirtualAddress, "%d: got %p != expected %p\n", i, info.BaseAddress, (char *)hlib + section.VirtualAddress);
                    ok(info.RegionSize == ALIGN_SIZE(section.Misc.VirtualSize, si.dwPageSize), "%d: got %lx != expected %x\n",
                       i, info.RegionSize, ALIGN_SIZE(section.Misc.VirtualSize, si.dwPageSize));
                    ok(info.Protect == PAGE_READONLY, "%d: %x != PAGE_READONLY\n", i, info.Protect);
                }
                ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
                ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
                ok(info.State == MEM_COMMIT, "%d: %x != MEM_COMMIT\n", i, info.State);
                ok(info.Type == SEC_IMAGE, "%d: %x != SEC_IMAGE\n", i, info.Type);

                if (nt_header.OptionalHeader.SectionAlignment >= si.dwPageSize)
                    ok(!memcmp((const char *)hlib + section.VirtualAddress + section.PointerToRawData, &nt_header, section.SizeOfRawData), "wrong section data\n");
                else
                    ok(!memcmp((const char *)hlib + section.PointerToRawData, section_data, section.SizeOfRawData), "wrong section data\n");

                /* check the zeroing of alignment */
                if (nt_header.OptionalHeader.SectionAlignment >= si.dwPageSize)
                {
                    const char *start;
                    int size;

                    start = (const char *)hlib + section.VirtualAddress + section.PointerToRawData + section.SizeOfRawData;
                    size = ALIGN_SIZE((ULONG_PTR)start, si.dwPageSize) - (ULONG_PTR)start;
                    ok(memcmp(start, filler, size), "%d: alignment should not be cleared\n", i);
                }
            }

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryEx(dll_name, 0, LOAD_LIBRARY_AS_DATAFILE);
            ok(hlib_as_data_file != 0, "LoadLibraryEx error %u\n", GetLastError());
            ok(hlib_as_data_file == hlib, "hlib_as_file and hlib are different\n");

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib);
            ok(ret, "FreeLibrary error %d\n", GetLastError());

            SetLastError(0xdeadbeef);
            hlib = GetModuleHandle(dll_name);
            ok(hlib != 0, "GetModuleHandle error %u\n", GetLastError());

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib_as_data_file);
            ok(ret, "FreeLibrary error %d\n", GetLastError());

            hlib = GetModuleHandle(dll_name);
            ok(!hlib, "GetModuleHandle should fail\n");

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryEx(dll_name, 0, LOAD_LIBRARY_AS_DATAFILE);
            ok(hlib_as_data_file != 0, "LoadLibraryEx error %u\n", GetLastError());
            ok((ULONG_PTR)hlib_as_data_file & 1, "hlib_as_data_file is even\n");

            hlib = GetModuleHandle(dll_name);
            ok(!hlib, "GetModuleHandle should fail\n");

            SetLastError(0xdeadbeef);
            ret = FreeLibrary(hlib_as_data_file);
            ok(ret, "FreeLibrary error %d\n", GetLastError());
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
        ret = DeleteFile(dll_name);
        ok(ret, "DeleteFile error %d\n", GetLastError());
    }
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
    SYSTEM_INFO si;

    if (!pNtMapViewOfSection) return;

    GetSystemInfo(&si);

    SetLastError(0xdeadbeef);
    hfile = CreateFile(dll_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hmap = CreateFileMapping(hfile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, 0);
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
    ok(info.RegionSize == si.dwPageSize, "got %#lx != expected %#x\n", info.RegionSize, si.dwPageSize);
    ok(info.Protect == scn_page_access, "got %#x != expected %#x\n", info.Protect, scn_page_access);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%#x != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#x != MEM_COMMIT\n", info.State);
    ok(info.Type == SEC_IMAGE, "%#x != SEC_IMAGE\n", info.Type);

    addr2 = NULL;
    size = 0;
    status = pNtMapViewOfSection(hmap, GetCurrentProcess(), &addr2, 0, 0, &offset,
                                 &size, 1 /* ViewShare */, 0, PAGE_READONLY);
    /* FIXME: remove once Wine is fixed */
    if (status != STATUS_IMAGE_NOT_AT_BASE)
    {
        todo_wine {
        ok(status == STATUS_IMAGE_NOT_AT_BASE, "expected STATUS_IMAGE_NOT_AT_BASE, got %x\n", status);
        ok(addr2 != 0, "mapped address should be valid\n");
        }
        goto wine_is_broken;
    }
    ok(status == STATUS_IMAGE_NOT_AT_BASE, "expected STATUS_IMAGE_NOT_AT_BASE, got %x\n", status);
    ok(addr2 != 0, "mapped address should be valid\n");
    ok(addr2 != addr1, "mapped addresses should be different\n");

    SetLastError(0xdeadbeef);
    size = VirtualQuery((char *)addr2 + section.VirtualAddress, &info, sizeof(info));
    ok(size == sizeof(info), "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == (char *)addr2 + section.VirtualAddress, "got %p != expected %p\n", info.BaseAddress, (char *)addr2 + section.VirtualAddress);
    ok(info.RegionSize == si.dwPageSize, "got %#lx != expected %#x\n", info.RegionSize, si.dwPageSize);
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
    ok(info.RegionSize == si.dwPageSize, "got %#lx != expected %#x\n", info.RegionSize, si.dwPageSize);
    ok(info.Protect == scn_page_access, "got %#x != expected %#x\n", info.Protect, scn_page_access);
    ok(info.AllocationBase == addr2, "%p != %p\n", info.AllocationBase, addr2);
    ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%#x != PAGE_EXECUTE_WRITECOPY\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#x != MEM_COMMIT\n", info.State);
    ok(info.Type == SEC_IMAGE, "%#x != SEC_IMAGE\n", info.Type);

    UnmapViewOfFile(addr2);

    SetLastError(0xdeadbeef);
    addr2 = LoadLibrary(dll_name);
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

wine_is_broken:
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
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    trace("system page size %#x\n", si.dwPageSize);

    SetLastError(0xdeadbeef);
    ret = VirtualProtect(section, si.dwPageSize, PAGE_NOACCESS, &old_prot);
    ok(ret, "VirtualProtect error %d\n", GetLastError());

    orig_prot = old_prot;

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        SetLastError(0xdeadbeef);
        ret = VirtualQuery(section, &info, sizeof(info));
        ok(ret, "VirtualQuery failed %d\n", GetLastError());
        ok(info.BaseAddress == section, "%d: got %p != expected %p\n", i, info.BaseAddress, section);
        ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
        ok(info.Protect == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, info.Protect);
        ok(info.AllocationBase == base, "%d: %p != %p\n", i, info.AllocationBase, base);
        ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %#x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
        ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
        ok(info.Type == SEC_IMAGE, "%d: %#x != SEC_IMAGE\n", i, info.Type);

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(section, si.dwPageSize, td[i].prot_set, &old_prot);
        if (td[i].prot_get)
        {
            ok(ret, "%d: VirtualProtect error %d, requested prot %#x\n", i, GetLastError(), td[i].prot_set);
            ok(old_prot == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, old_prot);

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(section, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %d\n", GetLastError());
            ok(info.BaseAddress == section, "%d: got %p != expected %p\n", i, info.BaseAddress, section);
            ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
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
        ret = VirtualProtect(section, si.dwPageSize, PAGE_NOACCESS, &old_prot);
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
            ret = VirtualProtect(section, si.dwPageSize, prot, &old_prot);
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
    ret = VirtualProtect(section, si.dwPageSize, orig_prot, &old_prot);
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
    static const char filler[0x1000];
    static const char section_data[0x10] = "section data";
    char buf[256];
    int i;
    DWORD dummy, file_align;
    HANDLE hfile;
    HMODULE hlib;
    SYSTEM_INFO si;
    char temp_path[MAX_PATH];
    char dll_name[MAX_PATH];
    SIZE_T size;
    MEMORY_BASIC_INFORMATION info;
    STARTUPINFO sti;
    PROCESS_INFORMATION pi;
    DWORD ret;

    GetSystemInfo(&si);
    trace("system page size %#x\n", si.dwPageSize);

    /* prevent displaying of the "Unable to load this DLL" message box */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    GetTempPath(MAX_PATH, temp_path);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        GetTempFileName(temp_path, "ldr", 0, dll_name);

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

        nt_header.FileHeader.NumberOfSections = 1;
        nt_header.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
        nt_header.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL | IMAGE_FILE_RELOCS_STRIPPED;

        nt_header.OptionalHeader.SectionAlignment = si.dwPageSize;
        nt_header.OptionalHeader.FileAlignment = 0x200;
        nt_header.OptionalHeader.SizeOfImage = sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + si.dwPageSize;
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
        hlib = LoadLibrary(dll_name);
        ok(hlib != 0, "LoadLibrary error %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        size = VirtualQuery((char *)hlib + section.VirtualAddress, &info, sizeof(info));
        ok(size == sizeof(info),
            "%d: VirtualQuery error %d\n", i, GetLastError());
        ok(info.BaseAddress == (char *)hlib + section.VirtualAddress, "%d: got %p != expected %p\n", i, info.BaseAddress, (char *)hlib + section.VirtualAddress);
        ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
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
            if (info.Protect == PAGE_WRITECOPY || info.Protect == PAGE_EXECUTE_WRITECOPY)
                todo_wine ok(info.Protect == td[i].scn_page_access_after_write, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].scn_page_access_after_write);
            else
                ok(info.Protect == td[i].scn_page_access_after_write, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].scn_page_access_after_write);
        }

        SetLastError(0xdeadbeef);
        ret = FreeLibrary(hlib);
        ok(ret, "FreeLibrary error %d\n", GetLastError());

        test_image_mapping(dll_name, td[i].scn_page_access, TRUE);

        /* reset IMAGE_FILE_DLL otherwise CreateProcess fails */
        nt_header.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_RELOCS_STRIPPED;
        SetLastError(0xdeadbeef);
        hfile = CreateFile(dll_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
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
        ret = CreateProcess(dll_name, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &sti, &pi);
        ok(ret, "CreateProcess() error %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        size = VirtualQueryEx(pi.hProcess, (char *)hlib + section.VirtualAddress, &info, sizeof(info));
        ok(size == sizeof(info),
            "%d: VirtualQuery error %d\n", i, GetLastError());
        ok(info.BaseAddress == (char *)hlib + section.VirtualAddress, "%d: got %p != expected %p\n", i, info.BaseAddress, (char *)hlib + section.VirtualAddress);
        ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
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
        ret = DeleteFile(dll_name);
        ok(ret || broken(!ret) /* nt4 */, "DeleteFile error %d\n", GetLastError());
    }
}

START_TEST(loader)
{
    pNtMapViewOfSection = (void *)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtMapViewOfSection");
    pNtUnmapViewOfSection = (void *)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtUnmapViewOfSection");

    test_Loader();
    test_ImportDescriptors();
    test_section_access();
}
