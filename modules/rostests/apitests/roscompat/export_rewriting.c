/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for the new roscompat export rewriting
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */


#include <apitest.h>
#include <windef.h>
#include <ntndk.h>


/* Adapted from modules/rostests/apitests/ntdll/NtMapViewOfSection.c */
const size_t kImageFileSize = 0x380;
static struct _MY_IMAGE_FILE
{
    IMAGE_DOS_HEADER doshdr;
    IMAGE_NT_HEADERS32 nthdrs;
    IMAGE_SECTION_HEADER text_header;
    IMAGE_SECTION_HEADER rdata_header;
    BYTE pad[0x78];
    BYTE text_data[0x80];
    BYTE rdata_data[0x100];
} ImageFile =
{
    /* IMAGE_DOS_HEADER */
    {
        IMAGE_DOS_SIGNATURE, 0x90, 3, 0, 4, 0, 0xFFFF, 0, 0xB8, 0, 0, 0, 0x40,
        0, { 0 }, 0, 0, { 0 }, 0x40
    },
    /* IMAGE_NT_HEADERS32 */
    {
        IMAGE_NT_SIGNATURE, /* Signature */
        /* IMAGE_FILE_HEADER */
        {
            IMAGE_FILE_MACHINE_I386, /* Machine */
            2, /* NumberOfSections */
            0x5e52b4d0, /* TimeDateStamp */
            0, /* PointerToSymbolTable */
            0, /* NumberOfSymbols */
            0xE0, /* SizeOfOptionalHeader */
            IMAGE_FILE_RELOCS_STRIPPED | IMAGE_FILE_EXECUTABLE_IMAGE |
            IMAGE_FILE_LINE_NUMS_STRIPPED | IMAGE_FILE_LOCAL_SYMS_STRIPPED |
            IMAGE_FILE_32BIT_MACHINE, /* Characteristics */
        },
        /* IMAGE_OPTIONAL_HEADER32 */
        {
            IMAGE_NT_OPTIONAL_HDR32_MAGIC, /* Magic */
            5, /* MajorLinkerVersion */
            0xc, /* MinorLinkerVersion */
            0x80, /* SizeOfCode */
            0x100, /* SizeOfInitializedData */
            0, /* SizeOfUninitializedData */
            0x200, /* AddressOfEntryPoint */
            0x200, /* BaseOfCode */
            0x280, /* BaseOfData */
            0x400000, /* ImageBase */
            0x80, /* SectionAlignment */
            0x80, /* FileAlignment */
            4, /* MajorOperatingSystemVersion */
            0, /* MinorOperatingSystemVersion */
            0, /* MajorImageVersion */
            0, /* MinorImageVersion */
            4, /* MajorSubsystemVersion */
            0, /* MinorSubsystemVersion */
            0, /* Win32VersionValue */
            0x380, /* SizeOfImage */
            0x200, /* SizeOfHeaders */
            0x0, /* CheckSum */
            IMAGE_SUBSYSTEM_WINDOWS_CUI, /* Subsystem */
            0, /* DllCharacteristics */
            0x100000, /* SizeOfStackReserve */
            0x1000, /* SizeOfStackCommit */
            0x100000, /* SizeOfHeapReserve */
            0x1000, /* SizeOfHeapCommit */
            0, /* LoaderFlags */
            0x10, /* NumberOfRvaAndSizes */
            /* IMAGE_DATA_DIRECTORY */
            {
                { 0 }, /* Export Table */
                { 0x28C, 0x28 }, /* Import Table */
                { 0 }, /* Resource Table */
                { 0 }, /* Exception Table */
                { 0 }, /* Certificate Table */
                { 0 }, /* Base Relocation Table */
                { 0 }, /* Debug */
                { 0 }, /* Copyright */
                { 0 }, /* Global Ptr */
                { 0 }, /* TLS Table */
                { 0 }, /* Load Config Table */
                { 0 }, /* Bound Import */
                { 0x280, 0xC }, /* IAT */
                { 0 }, /* Delay Import Descriptor */
                { 0 }, /* CLI Header */
                { 0 } /* Reserved */
            }
        }
    },
    /* IMAGE_SECTION_HEADER */
    {
        ".text", /* Name */
        { 0x18 }, /* Misc.VirtualSize */
        0x200, /* VirtualAddress */
        0x80, /* SizeOfRawData */
        0x200, /* PointerToRawData */
        0, /* PointerToRelocations */
        0, /* PointerToLinenumbers */
        0, /* NumberOfRelocations */
        0, /* NumberOfLinenumbers */
        IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE |
        IMAGE_SCN_CNT_CODE, /* Characteristics */
    },
    /* IMAGE_SECTION_HEADER */
    {
        ".rdata", /* Name */
        { 0x84 }, /* Misc.VirtualSize */
        0x280, /* VirtualAddress */
        0x100, /* SizeOfRawData */
        0x280, /* PointerToRawData */
        0, /* PointerToRelocations */
        0, /* PointerToLinenumbers */
        0, /* NumberOfRelocations */
        0, /* NumberOfLinenumbers */
        IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA, /* Characteristics */
    },
    /* pad */
    { 0 },
    /* text */
    {
        0x90,                               // nop
        0x90,                               // nop
        0x90,                               // nop
        0x90,                               // nop
        0x90,                               // nop

        0xA1, 0x84, 0x02, 0x40, 0x00,       // mov     eax, ds:InitializeCriticalSectionAndSpinCount
        0x6A, 0x00,                         // push    0
        0xE8, 0x01, 0x00, 0x00, 0x00,       // call    ExitProcess

        0xCC,                               // align 2

//ExitProcess:
        0xFF, 0x25, 0x80, 0x02, 0x40, 00    // jmp     ds:__imp_ExitProcess
    },
    /* rdata */
    {
        0xE8, 0x02, 0x00, 0x00,     // extrn ExitProcess:dword
        0xC0, 0x02, 0x00, 0x00,     // extrn InitializeCriticalSectionAndSpinCount:dword
        0x00, 0x00, 0x00, 0x00,


        0xB4, 0x02, 0x00, 0x00,     // Import Name Table
        0x00, 0x00, 0x00, 0x00,     // Time stamp
        0x00, 0x00, 0x00, 0x00,     // Forwarder Chain
        0xF6, 0x02, 0x00, 0x00,     // dd rva aKernel32Dll     ; DLL Name
        0x80, 0x02, 0x00, 0x00,     // dd rva ExitProcess      ; Import Address Table


        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        // Import names
        0xE8, 0x02, 0x00, 0x00,     // Offset ExitProcess
        0xC0, 0x02, 0x00, 0x00,     // Offset InitializeCriticalSectionAndSpinCount
        0x00, 0x00, 0x00, 0x00,

        0xCC, 0x01,
        'I','n','i','t','i','a','l','i','z','e','C','r','i','t','i','c','a','l','S','e','c','t','i','o','n','A','n','d','S','p','i','n','C','o','u','n','t', 0x00,

        0x9B, 0x00,
        'E','x','i','t','P','r','o','c','e','s','s', 0x00,

        'k','e','r','n','e','l','3','2','.','d','l','l', 0x0
    }
};

C_ASSERT(FIELD_OFFSET(struct _MY_IMAGE_FILE, nthdrs) == 0x40);
C_ASSERT(FIELD_OFFSET(struct _MY_IMAGE_FILE, text_header) == 0x138);
C_ASSERT(FIELD_OFFSET(struct _MY_IMAGE_FILE, rdata_header) == 0x160);
C_ASSERT(FIELD_OFFSET(struct _MY_IMAGE_FILE, text_data) == 0x200);
C_ASSERT(FIELD_OFFSET(struct _MY_IMAGE_FILE, rdata_data) == 0x280);


static void write_file(const char *filename, const void* data, int data_size, const char* reason)
{
    DWORD size;

    HANDLE hf = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hf != INVALID_HANDLE_VALUE, "Failed to write file (%s)\n", reason);
    WriteFile(hf, data, data_size, &size, NULL);
    CloseHandle(hf);
    ok(size == data_size, "Not all data written (%s)\n", reason);
}

static void write_exe(const char* filename, const char* function_name, const char* reason)
{
    PCHAR p;
    size_t n;
    PBYTE data = LocalAlloc(LMEM_ZEROINIT, kImageFileSize);

    memcpy(data, &ImageFile, sizeof(ImageFile));

    p = (PCHAR)data + FIELD_OFFSET(struct _MY_IMAGE_FILE, rdata_data);
    for (n = 0; n < sizeof(ImageFile.rdata_data); ++n)
    {
        if (!stricmp(p + n, "InitializeCriticalSectionAndSpinCount"))
        {
            strcpy(p + n, function_name);
            break;
        }
    }
    ok(n < sizeof(ImageFile.rdata_data), "Unable to find function offset! (%s)\n", reason);

    write_file(filename, data, kImageFileSize, reason);

    LocalFree(data);
}


static HANDLE create_process(char* process_name, const char* reason)
{
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    BOOL res;

    res = CreateProcessA(NULL, process_name, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    if (!res)
        return NULL;

    CloseHandle(pi.hThread);
    return pi.hProcess;
}


static DWORD run_exe(const char* function_name, const char* reason)
{
    char tmp_file[MAX_PATH];
    char tmp_dir[MAX_PATH];
    DWORD dwExitCode = 123;
    HANDLE hProc;

    GetTempPathA(_countof(tmp_dir), tmp_dir);
    GetTempFileNameA(tmp_dir, "RCO", 0, tmp_file);

    write_exe(tmp_file, function_name, reason);

    hProc = create_process(tmp_file, reason);
    ok(hProc != NULL, "Failed to create process (%s)\n", reason);
    if (hProc)
    {
        dwExitCode = WaitForSingleObject(hProc, 2000);
        ok(dwExitCode == WAIT_OBJECT_0, "Error waiting for proc: %lu (%s)\n", dwExitCode, reason);
        dwExitCode = 123;
        GetExitCodeProcess(hProc, &dwExitCode);
        ok(dwExitCode != STILL_ACTIVE, "Process still running (%s)\n", reason);
    }

    DeleteFileA(tmp_file);

    return dwExitCode;
}

struct test_function
{
    const char* function;
    DWORD dwMinVersion;
    DWORD dwMaxVersion;
} all_functions[] = {
    { "InitializeCriticalSectionAndSpinCount",  0x500, 0xfff },
    { "BaseProcessInitPostImport",              0x501, 0x502 },
    { "BaseGenerateAppCompatData",              0x600, 0xfff },
    { "GetCurrentPackageId",                    0x602, 0xfff },
};

static void test_functions(DWORD dwVersion)
{
    DWORD dwResult, dwExpectedResult;
    size_t n;
    char reason[100];

    for (n = 0; n < _countof(all_functions); ++n)
    {
        sprintf(reason, "%s on 0x%lx", all_functions[n].function, dwVersion);

        dwExpectedResult = (all_functions[n].dwMinVersion > dwVersion || all_functions[n].dwMaxVersion < dwVersion) ? STATUS_ENTRYPOINT_NOT_FOUND : 0;

        dwResult = run_exe(all_functions[n].function, reason);
        ok(dwResult == dwExpectedResult, "Expected %ld, got %ld (%s)\n", dwExpectedResult, dwResult, reason);
    }
}

struct test_shim
{
    const char* name;
    DWORD dwVersion;
} all_shims[] = {
    { "VISTASP2",   0x600 },
    { "WIN7SP1",    0x601 },
    { "WIN8RTM",    0x602 },
    { "WIN81RTM",   0x603 },
};


START_TEST(export_rewriting)
{
    DWORD dwResult;
    size_t n;

    // Ensure child processes do not inherit whatever we are launched with
    SetEnvironmentVariableA("__COMPAT_LAYER", NULL);

    // Verify that the test application runs fine in normal mode
    dwResult = run_exe("InitializeCriticalSectionAndSpinCount", "verify_ok");
    ok_hex(dwResult, 0);

    // Verify that we can detect a function resolve failure
    dwResult = run_exe("InitializeStuffThatDoesNotExist", "verify_fail");
    ok_hex(dwResult, STATUS_ENTRYPOINT_NOT_FOUND);

    // First, test all functions against our 'current' version
    test_functions(0x502);

    for (n = 0; n < _countof(all_shims); ++n)
    {
        // Apply a shim, elevating the NT version
        SetEnvironmentVariableA("__COMPAT_LAYER", all_shims[n].name);
        // Now test all functions against this version
        test_functions(all_shims[n].dwVersion);
    }
}

