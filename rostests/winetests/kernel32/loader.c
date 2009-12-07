/*
 * Unit test suite for the PE loader.
 *
 * Copyright 2006 Dmitry Timoshkov
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

#include "windef.h"
#include "winbase.h"
#include "wine/test.h"

#define ALIGN_SIZE(size, alignment) (((size) + (alignment - 1)) & ~((alignment - 1)))

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
        DWORD error; /* 0 means LoadLibrary should succeed */
        DWORD alt_error; /* alternate error */
    } td[] =
    {
        { &dos_header, sizeof(dos_header),
          1, 0, 0, 0, 0, 0,
          ERROR_BAD_EXE_FORMAT
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0xe00,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_BAD_EXE_FORMAT /* XP doesn't like too small image size */
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_SUCCESS
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          0x1f00,
          0x1000,
          ERROR_SUCCESS
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x200, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_SUCCESS, ERROR_INVALID_ADDRESS /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x200, 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_BAD_EXE_FORMAT /* XP doesn't like alignments */
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_SUCCESS
        },
        { &dos_header, sizeof(dos_header),
          1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          0x200,
          ERROR_SUCCESS
        },
        /* Mandatory are all fields up to SizeOfHeaders, everything else
         * is really optional (at least that's true for XP).
         */
        { &dos_header, sizeof(dos_header),
          1, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          sizeof(dos_header) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(IMAGE_SECTION_HEADER) + 0x10,
          sizeof(dos_header) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0xd0, /* beyond of the end of file */
          0xc0, /* beyond of the end of file */
          ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0x1000,
          0,
          ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          1,
          0,
          ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT /* vista is more strict */
        },
#if 0 /* not power of 2 alignments need more test cases */
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x300, 0x300,
          1,
          0,
          ERROR_BAD_EXE_FORMAT /* alignment is not power of 2 */
        },
#endif
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 4, 4,
          1,
          0,
          ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 1, 1,
          1,
          0,
          ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT /* vista is more strict */
        },
        { &dos_header, sizeof(dos_header),
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0,
          0,
          ERROR_BAD_EXE_FORMAT /* image size == 0 -> failure */
        },
        /* the following data mimics the PE image which upack creates */
        { &dos_header, 0x10,
          1, 0x148, 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          0x200,
          ERROR_SUCCESS
        },
        /* Minimal PE image that XP is able to load: 92 bytes */
        { &dos_header, 0x04,
          0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum),
          0x04 /* also serves as e_lfanew in the truncated MZ header */, 0x04,
          1,
          0,
          ERROR_SUCCESS, ERROR_BAD_EXE_FORMAT /* vista is more strict */
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
        ok(WriteFile(hfile, td[i].dos_header, td[i].size_of_dos_header, &dummy, NULL),
           "WriteFile error %d\n", GetLastError());

        nt_header.FileHeader.NumberOfSections = td[i].number_of_sections;
        nt_header.FileHeader.SizeOfOptionalHeader = td[i].size_of_optional_header;

        nt_header.OptionalHeader.SectionAlignment = td[i].section_alignment;
        nt_header.OptionalHeader.FileAlignment = td[i].file_alignment;
        nt_header.OptionalHeader.SizeOfImage = td[i].size_of_image;
        nt_header.OptionalHeader.SizeOfHeaders = td[i].size_of_headers;
        SetLastError(0xdeadbeef);
        ok(WriteFile(hfile, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL),
           "WriteFile error %d\n", GetLastError());

        if (nt_header.FileHeader.SizeOfOptionalHeader)
        {
            SetLastError(0xdeadbeef);
            ok(WriteFile(hfile, &nt_header.OptionalHeader,
                         min(nt_header.FileHeader.SizeOfOptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER)),
                         &dummy, NULL),
               "WriteFile error %d\n", GetLastError());
            if (nt_header.FileHeader.SizeOfOptionalHeader > sizeof(IMAGE_OPTIONAL_HEADER))
            {
                file_align = nt_header.FileHeader.SizeOfOptionalHeader - sizeof(IMAGE_OPTIONAL_HEADER);
                assert(file_align < sizeof(filler));
                SetLastError(0xdeadbeef);
                ok(WriteFile(hfile, filler, file_align, &dummy, NULL),
                   "WriteFile error %d\n", GetLastError());
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
            ok(WriteFile(hfile, &section, sizeof(section), &dummy, NULL),
               "WriteFile error %d\n", GetLastError());

            /* section data */
            SetLastError(0xdeadbeef);
            ok(WriteFile(hfile, section_data, sizeof(section_data), &dummy, NULL),
               "WriteFile error %d\n", GetLastError());
        }

        file_size = GetFileSize(hfile, NULL);
        CloseHandle(hfile);

        SetLastError(0xdeadbeef);
        hlib = LoadLibrary(dll_name);
        if (hlib)
        {
            MEMORY_BASIC_INFORMATION info;

            ok( td[i].error == ERROR_SUCCESS, "%d: should have failed\n", i );

            SetLastError(0xdeadbeef);
            ok(VirtualQuery(hlib, &info, sizeof(info)) == sizeof(info),
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
            ok(VirtualQuery((char *)hlib + info.RegionSize, &info, sizeof(info)) == sizeof(info),
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
                ok(VirtualQuery((char *)hlib + section.VirtualAddress, &info, sizeof(info)) == sizeof(info),
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
            ok(FreeLibrary(hlib), "FreeLibrary error %d\n", GetLastError());

            SetLastError(0xdeadbeef);
            hlib = GetModuleHandle(dll_name);
            ok(hlib != 0, "GetModuleHandle error %u\n", GetLastError());

            SetLastError(0xdeadbeef);
            ok(FreeLibrary(hlib_as_data_file), "FreeLibrary error %d\n", GetLastError());

            hlib = GetModuleHandle(dll_name);
            ok(!hlib, "GetModuleHandle should fail\n");

            SetLastError(0xdeadbeef);
            hlib_as_data_file = LoadLibraryEx(dll_name, 0, LOAD_LIBRARY_AS_DATAFILE);
            ok(hlib_as_data_file != 0, "LoadLibraryEx error %u\n", GetLastError());
            ok((ULONG_PTR)hlib_as_data_file & 1, "hlib_as_data_file is even\n");

            hlib = GetModuleHandle(dll_name);
            ok(!hlib, "GetModuleHandle should fail\n");

            SetLastError(0xdeadbeef);
            ok(FreeLibrary(hlib_as_data_file), "FreeLibrary error %d\n", GetLastError());
        }
        else
        {
            ok(td[i].error || td[i].alt_error, "%d: LoadLibrary should succeed\n", i);

            if (GetLastError() == ERROR_GEN_FAILURE) /* Win9x, broken behaviour */
            {
                trace("skipping the loader test on Win9x\n");
                DeleteFile(dll_name);
                return;
            }

            ok(td[i].error == GetLastError() || td[i].alt_error == GetLastError(),
               "%d: expected error %d or %d, got %d\n",
               i, td[i].error, td[i].alt_error, GetLastError());
        }

        SetLastError(0xdeadbeef);
        ok(DeleteFile(dll_name), "DeleteFile error %d\n", GetLastError());
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

START_TEST(loader)
{
    test_Loader();
    test_ImportDescriptors();
}
