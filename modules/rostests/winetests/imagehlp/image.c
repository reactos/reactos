/*
 * Copyright 2008 Juan Lang
 * Copyright 2010 Andrey Turkin
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
#include <stdio.h>
#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winver.h>
#include <winnt.h>
#include <winuser.h>
#include <imagehlp.h>

#include "wine/test.h"

static char *load_resource(const char *name)
{
    static char path[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathA(ARRAY_SIZE(path), path);
    strcat(path, name);

    file = CreateFileA(path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %lu.\n",
            debugstr_a(path), GetLastError());

    res = FindResourceA(NULL, name, "TESTDLL");
    ok(!!res, "Failed to load resource, error %lu.\n", GetLastError());
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource( GetModuleHandleA(NULL), res), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res), "Failed to write resource.\n");
    CloseHandle(file);

    return path;
}

/* minimal PE file image */
#define VA_START 0x400000
#define FILE_PE_START 0x50
#define NUM_SECTIONS 3
#define FILE_TEXT 0x200
#define RVA_TEXT 0x1000
#define RVA_BSS 0x2000
#define FILE_IDATA 0x400
#define RVA_IDATA 0x3000
#define FILE_TOTAL 0x600
#define RVA_TOTAL 0x4000

#include <pshpack1.h>
struct imports
{
    IMAGE_IMPORT_DESCRIPTOR descriptors[2];
    IMAGE_THUNK_DATA32 original_thunks[2];
    IMAGE_THUNK_DATA32 thunks[2];
    struct
    {
        WORD hint;
        char funcname[0x20];
    } ibn;
    char dllname[0x10];
};
#define EXIT_PROCESS (VA_START + RVA_IDATA + FIELD_OFFSET(struct imports, thunks))

static struct image
{
    IMAGE_DOS_HEADER dos_header;
    char __alignment1[FILE_PE_START - sizeof(IMAGE_DOS_HEADER)];
    IMAGE_NT_HEADERS32 nt_headers;
    IMAGE_SECTION_HEADER sections[NUM_SECTIONS];
    char __alignment2[FILE_TEXT - FILE_PE_START - sizeof(IMAGE_NT_HEADERS32) -
        NUM_SECTIONS * sizeof(IMAGE_SECTION_HEADER)];
    unsigned char text_section[FILE_IDATA-FILE_TEXT];
    struct imports idata_section;
    char __alignment3[FILE_TOTAL-FILE_IDATA-sizeof(struct imports)];
}
bin =
{
    /* dos header */
    {IMAGE_DOS_SIGNATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}, 0, 0, {0}, FILE_PE_START},
    /* alignment before PE header */
    {0},
    /* nt headers */
    {IMAGE_NT_SIGNATURE,
        /* basic headers - 3 sections, no symbols, EXE file */
        {IMAGE_FILE_MACHINE_I386, NUM_SECTIONS, 0, 0, 0, sizeof(IMAGE_OPTIONAL_HEADER32),
            IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_EXECUTABLE_IMAGE},
        /* optional header */
        {IMAGE_NT_OPTIONAL_HDR32_MAGIC, 4, 0, FILE_IDATA-FILE_TEXT,
            FILE_TOTAL-FILE_IDATA + FILE_IDATA-FILE_TEXT, 0x400,
            RVA_TEXT, RVA_TEXT, RVA_BSS, VA_START, 0x1000, 0x200, 4, 0, 1, 0, 4, 0, 0,
            RVA_TOTAL, FILE_TEXT, 0, IMAGE_SUBSYSTEM_WINDOWS_GUI, 0,
            0x200000, 0x1000, 0x100000, 0x1000, 0, 0x10,
            {{0, 0},
             {RVA_IDATA, sizeof(struct imports)}
            }
        }
    },
    /* sections */
    {
        {".text", {0x100}, RVA_TEXT, FILE_IDATA-FILE_TEXT, FILE_TEXT,
            0, 0, 0, 0, IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ},
        {".bss", {0x400}, RVA_BSS, 0, 0, 0, 0, 0, 0,
            IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE},
        {".idata", {sizeof(struct imports)}, RVA_IDATA, FILE_TOTAL-FILE_IDATA, FILE_IDATA, 0,
            0, 0, 0, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE}
    },
    /* alignment before first section */
    {0},
    /* .text section */
    {
        0x31, 0xC0, /* xor eax, eax */
        0xFF, 0x25, EXIT_PROCESS&0xFF, (EXIT_PROCESS>>8)&0xFF, (EXIT_PROCESS>>16)&0xFF,
            (EXIT_PROCESS>>24)&0xFF, /* jmp ExitProcess */
        0
    },
    /* .idata section */
    {
        {
            {{RVA_IDATA + FIELD_OFFSET(struct imports, original_thunks)}, 0, 0,
            RVA_IDATA + FIELD_OFFSET(struct imports, dllname),
            RVA_IDATA + FIELD_OFFSET(struct imports, thunks)
            },
            {{0}, 0, 0, 0, 0}
        },
        {{{RVA_IDATA+FIELD_OFFSET(struct imports, ibn)}}, {{0}}},
        {{{RVA_IDATA+FIELD_OFFSET(struct imports, ibn)}}, {{0}}},
        {0,"ExitProcess"},
        "KERNEL32.DLL"
    },
    /* final alignment */
    {0}
};

struct imports64
{
    IMAGE_IMPORT_DESCRIPTOR descriptors[2];
    IMAGE_THUNK_DATA64 original_thunks[2];
    IMAGE_THUNK_DATA64 thunks[2];
    struct
    {
        WORD hint;
        char funcname[0x20];
    } ibn;
    char dllname[0x10];
};

static struct image64
{
    IMAGE_DOS_HEADER dos_header;
    char __alignment1[FILE_PE_START - sizeof(IMAGE_DOS_HEADER)];
    IMAGE_NT_HEADERS64 nt_headers;
    IMAGE_SECTION_HEADER sections[2];
    char __alignment2[FILE_TEXT - FILE_PE_START - sizeof(IMAGE_NT_HEADERS64) - 2 * sizeof(IMAGE_SECTION_HEADER)];
    unsigned char text_section[FILE_IDATA - FILE_TEXT];
    struct imports64 idata_section;
    char __alignment3[FILE_TOTAL - FILE_IDATA - sizeof(struct imports64)];
}
bin64 =
{
    /* dos header */
    { IMAGE_DOS_SIGNATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}, 0, 0, {0}, FILE_PE_START },
    /* alignment before PE header */
    {0},
    /* nt headers */
    {
        IMAGE_NT_SIGNATURE,
        /* basic headers - 2 sections, no symbols, EXE file */
        {IMAGE_FILE_MACHINE_AMD64, 2, 0, 0, 0, sizeof(IMAGE_OPTIONAL_HEADER64),
         IMAGE_FILE_EXECUTABLE_IMAGE},
        /* optional header */
        {
            IMAGE_NT_OPTIONAL_HDR64_MAGIC, 6, 0, FILE_IDATA - FILE_TEXT,
            FILE_TOTAL - FILE_IDATA + FILE_IDATA - FILE_TEXT, 0x400,
            RVA_TEXT, RVA_TEXT, VA_START, 0x1000, 0x200, 4, 0, 1, 0, 5, 2, 0,
            RVA_TOTAL, FILE_TEXT, 0, IMAGE_SUBSYSTEM_WINDOWS_GUI, 0,
            0x200000, 0x1000, 0x100000, 0x1000, 0, 0x10,
            {{0, 0}, {RVA_IDATA, sizeof(struct imports64)}}
        }
    },
    /* sections */
    {
        { ".text", {0x100}, RVA_TEXT, FILE_IDATA - FILE_TEXT, FILE_TEXT,
          0, 0, 0, 0, IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ },
        { ".idata", {sizeof(struct imports64)}, RVA_IDATA, FILE_TOTAL - FILE_IDATA, FILE_IDATA, 0,
          0, 0, 0, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE }
    },
    /* alignment before first section */
    {0},
    /* .text section */
    {
        0x90
    },
    /* .idata section */
    {
        { {{RVA_IDATA + FIELD_OFFSET(struct imports64, original_thunks)}, 0, 0,
            RVA_IDATA + FIELD_OFFSET(struct imports64, dllname),
            RVA_IDATA + FIELD_OFFSET(struct imports64, thunks)},
          {{0}, 0, 0, 0, 0} },
        { {{RVA_IDATA + FIELD_OFFSET(struct imports64, ibn)}}, {{0}} },
        { {{RVA_IDATA + FIELD_OFFSET(struct imports64, ibn)}}, {{0}} },
        { 0, "ExitProcess" },
        "KERNEL32.DLL"
    },
    /* final alignment */
    {0}
};
#include <poppack.h>

struct blob
{
    DWORD cb;
    BYTE *pb;
};

struct expected_blob
{
    DWORD cb;
    const void *pb;
};

struct update_accum
{
    DWORD cUpdates;
    struct blob *updates;
};

struct expected_update_accum
{
    DWORD cUpdates;
    const struct expected_blob *updates;
    BOOL  todo;
};

static BOOL WINAPI accumulating_stream_output(DIGEST_HANDLE handle, BYTE *pb,
 DWORD cb)
{
    struct update_accum *accum = (struct update_accum *)handle;
    BOOL ret = FALSE;

    if (accum->cUpdates)
        accum->updates = HeapReAlloc(GetProcessHeap(), 0, accum->updates,
         (accum->cUpdates + 1) * sizeof(struct blob));
    else
        accum->updates = HeapAlloc(GetProcessHeap(), 0, sizeof(struct blob));
    if (accum->updates)
    {
        struct blob *blob = &accum->updates[accum->cUpdates];

        blob->pb = HeapAlloc(GetProcessHeap(), 0, cb);
        if (blob->pb)
        {
            memcpy(blob->pb, pb, cb);
            blob->cb = cb;
            ret = TRUE;
        }
        accum->cUpdates++;
    }
    return ret;
}

static void check_updates(LPCSTR header, const struct expected_update_accum *expected,
        const struct update_accum *got)
{
    DWORD i;

    todo_wine_if (expected->todo)
        ok(expected->cUpdates == got->cUpdates, "%s: expected %ld updates, got %ld\n",
            header, expected->cUpdates, got->cUpdates);
    for (i = 0; i < min(expected->cUpdates, got->cUpdates); i++)
    {
        ok(expected->updates[i].cb == got->updates[i].cb, "%s, update %ld: expected %ld bytes, got %ld\n",
                header, i, expected->updates[i].cb, got->updates[i].cb);
        if (expected->updates[i].cb && expected->updates[i].cb == got->updates[i].cb)
            ok(!memcmp(expected->updates[i].pb, got->updates[i].pb, got->updates[i].cb),
                    "%s, update %ld: unexpected value\n", header, i);
    }
}

/* Frees the updates stored in accum */
static void free_updates(struct update_accum *accum)
{
    DWORD i;

    for (i = 0; i < accum->cUpdates; i++)
        HeapFree(GetProcessHeap(), 0, accum->updates[i].pb);
    HeapFree(GetProcessHeap(), 0, accum->updates);
    accum->updates = NULL;
    accum->cUpdates = 0;
}

static const struct expected_blob b1[] = {
    {FILE_PE_START,  &bin},
    /* with zeroed Checksum/SizeOfInitializedData/SizeOfImage fields */
    {sizeof(bin.nt_headers), &bin.nt_headers},
    {sizeof(bin.sections),  &bin.sections},
    {FILE_IDATA-FILE_TEXT, &bin.text_section},
    {sizeof(bin.idata_section.descriptors[0].OriginalFirstThunk),
        &bin.idata_section.descriptors[0].OriginalFirstThunk},
    {FIELD_OFFSET(struct imports, thunks)-
        (FIELD_OFFSET(struct imports, descriptors)+FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, Name)),
        &bin.idata_section.descriptors[0].Name},
    {FILE_TOTAL-FILE_IDATA-FIELD_OFFSET(struct imports, ibn),
        &bin.idata_section.ibn}
};
static const struct expected_update_accum a1 = { ARRAY_SIZE(b1), b1, TRUE };

static const struct expected_blob b2[] = {
    {FILE_PE_START,  &bin},
    /* with zeroed Checksum/SizeOfInitializedData/SizeOfImage fields */
    {sizeof(bin.nt_headers), &bin.nt_headers},
    {sizeof(bin.sections),  &bin.sections},
    {FILE_IDATA-FILE_TEXT, &bin.text_section},
    {FILE_TOTAL-FILE_IDATA, &bin.idata_section}
};
static const struct expected_update_accum a2 = { ARRAY_SIZE(b2), b2, FALSE };

static const struct expected_blob b3[] = {
    {FILE_PE_START,  &bin64},
    /* with zeroed Checksum/SizeOfInitializedData/SizeOfImage fields */
    {sizeof(bin64.nt_headers), &bin64.nt_headers},
    {sizeof(bin64.sections),  &bin64.sections},
    {FILE_IDATA - FILE_TEXT, &bin64.text_section},
    {sizeof(bin64.idata_section.descriptors[0].OriginalFirstThunk),
     &bin64.idata_section.descriptors[0].OriginalFirstThunk},
    {FIELD_OFFSET(struct imports64, thunks) -
     (FIELD_OFFSET(struct imports64, descriptors) + FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, Name)),
     &bin64.idata_section.descriptors[0].Name},
    {FILE_TOTAL - FILE_IDATA - FIELD_OFFSET(struct imports64, ibn), &bin64.idata_section.ibn}
};
static const struct expected_update_accum a3 = { ARRAY_SIZE(b3), b3, TRUE };

static const struct expected_blob b4[] = {
    {FILE_PE_START,  &bin64},
    /* with zeroed Checksum/SizeOfInitializedData/SizeOfImage fields */
    {sizeof(bin64.nt_headers), &bin64.nt_headers},
    {sizeof(bin64.sections),  &bin64.sections},
    {FILE_IDATA - FILE_TEXT, &bin64.text_section},
    {FILE_TOTAL - FILE_IDATA, &bin64.idata_section}
};
static const struct expected_update_accum a4 = { ARRAY_SIZE(b4), b4, FALSE };

/* Creates a test file and returns a handle to it.  The file's path is returned
 * in temp_file, which must be at least MAX_PATH characters in length.
 */
static HANDLE create_temp_file(char *temp_file)
{
    HANDLE file = INVALID_HANDLE_VALUE;
    char temp_path[MAX_PATH];

    if (GetTempPathA(sizeof(temp_path), temp_path))
    {
        if (GetTempFileNameA(temp_path, "img", 0, temp_file))
            file = CreateFileA(temp_file, GENERIC_READ | GENERIC_WRITE, 0, NULL,
             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    return file;
}

static DWORD compute_checksum(const WORD *image, DWORD image_size)
{
    const WORD *ptr = image;
    DWORD size, sum = 0;

    for (size = (image_size + 1) / sizeof(WORD); size > 0; ptr++, size--)
    {
        sum += *ptr;
        if (HIWORD(sum)) sum = LOWORD(sum) + HIWORD(sum);
    }
    sum = (WORD)(LOWORD(sum) + HIWORD(sum));
    sum += image_size;

    return sum;
}

static void test_get_digest_stream(void)
{
    BOOL ret;
    HANDLE file;
    char temp_file[MAX_PATH];
    DWORD count, checksum;
    struct update_accum accum = { 0, NULL };

    SetLastError(0xdeadbeef);
    ret = ImageGetDigestStream(NULL, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    file = create_temp_file(temp_file);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("couldn't create temp file\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = ImageGetDigestStream(file, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = ImageGetDigestStream(NULL, 0, accumulating_stream_output, &accum);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    /* Even with "valid" parameters, it fails with an empty file */
    SetLastError(0xdeadbeef);
    ret = ImageGetDigestStream(file, 0, accumulating_stream_output, &accum);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    /* Finally, with a valid executable in the file, it succeeds.  Note that
     * the file pointer need not be positioned at the beginning.
     */
    bin.nt_headers.OptionalHeader.CheckSum = 0;
    checksum = compute_checksum((const WORD *)&bin, sizeof(bin));
    bin.nt_headers.OptionalHeader.CheckSum = checksum;

    WriteFile(file, &bin, sizeof(bin), &count, NULL);
    FlushFileBuffers(file);

    /* zero out some fields ImageGetDigestStream would zero out */
    bin.nt_headers.OptionalHeader.CheckSum = 0;
    bin.nt_headers.OptionalHeader.SizeOfInitializedData = 0;
    bin.nt_headers.OptionalHeader.SizeOfImage = 0;

    ret = ImageGetDigestStream(file, 0, accumulating_stream_output, &accum);
    ok(ret, "ImageGetDigestStream failed: %ld\n", GetLastError());
    check_updates("flags = 0", &a1, &accum);
    free_updates(&accum);
    ret = ImageGetDigestStream(file, CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO,
     accumulating_stream_output, &accum);
    ok(ret, "ImageGetDigestStream failed: %ld\n", GetLastError());
    check_updates("flags = CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO", &a2, &accum);
    free_updates(&accum);
    CloseHandle(file);
    DeleteFileA(temp_file);

    file = create_temp_file(temp_file);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("couldn't create temp file\n");
        return;
    }

    bin64.nt_headers.OptionalHeader.CheckSum = 0;
    checksum = compute_checksum((const WORD *)&bin64, sizeof(bin64));
    bin64.nt_headers.OptionalHeader.CheckSum = checksum;

    WriteFile(file, &bin64, sizeof(bin64), &count, NULL);
    FlushFileBuffers(file);

    bin64.nt_headers.OptionalHeader.CheckSum = 0;
    bin64.nt_headers.OptionalHeader.SizeOfInitializedData = 0;
    bin64.nt_headers.OptionalHeader.SizeOfImage = 0;

    ret = ImageGetDigestStream(file, 0, accumulating_stream_output, &accum);
    ok(ret, "ImageGetDigestStream failed: %lu\n", GetLastError());
    check_updates("flags = 0", &a3, &accum);
    free_updates(&accum);
    ret = ImageGetDigestStream(file, CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO,
                               accumulating_stream_output, &accum);
    ok(ret, "ImageGetDigestStream failed: %lu\n", GetLastError());
    check_updates("flags = CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO", &a4, &accum);
    free_updates(&accum);
    CloseHandle(file);
    DeleteFileA(temp_file);
}

static unsigned int got_SysAllocString, got_GetOpenFileNameA, got_SHRegGetIntW;

static BOOL WINAPI bind_image_cb(IMAGEHLP_STATUS_REASON reason, const char *file,
        const char *module, ULONG_PTR va, ULONG_PTR param)
{
    static char last_module[MAX_PATH];

    if (winetest_debug > 1)
        trace("reason %u, file %s, module %s, va %#Ix, param %#Ix\n",
                reason, debugstr_a(file), debugstr_a(module), va, param);

    if (reason == BindImportModule)
    {
        ok(!strchr(module, '\\'), "got module name %s\n", debugstr_a(module));
        strcpy(last_module, module);
        ok(!va, "got VA %#Ix\n", va);
        ok(!param, "got param %#Ix\n", param);
    }
    else if (reason == BindImportProcedure)
    {
        char full_path[MAX_PATH];
        BOOL ret;

        todo_wine ok(!!va, "expected nonzero VA\n");
        ret = SearchPathA(NULL, last_module, ".dll", sizeof(full_path), full_path, NULL);
        ok(ret, "got error %lu\n", GetLastError());
        ok(!strcmp(module, full_path), "expected %s, got %s\n", debugstr_a(full_path), debugstr_a(module));

        if (!strcmp((const char *)param, "SysAllocString"))
        {
            ok(!strcmp(last_module, "oleaut32.dll"), "got wrong module %s\n", debugstr_a(module));
            ++got_SysAllocString;
        }
        else if (!strcmp((const char *)param, "GetOpenFileNameA"))
        {
            ok(!strcmp(last_module, "comdlg32.dll"), "got wrong module %s\n", debugstr_a(module));
            ++got_GetOpenFileNameA;
        }
        else if (!strcmp((const char *)param, "Ordinal117"))
        {
            ok(!strcmp(last_module, "shlwapi.dll"), "got wrong module %s\n", debugstr_a(module));
            ++got_SHRegGetIntW;
        }
    }
    else
    {
        ok(0, "got unexpected reason %#x\n", reason);
    }
    return TRUE;
}

static void test_bind_image_ex(void)
{
    const char *filename = load_resource("testdll.dll");
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = BindImageEx(BIND_ALL_IMAGES | BIND_NO_BOUND_IMPORTS | BIND_NO_UPDATE,
            "nonexistent.dll", 0, 0, bind_image_cb);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_PARAMETER,
            "got error %lu\n", GetLastError());

    ret = BindImageEx(BIND_ALL_IMAGES | BIND_NO_BOUND_IMPORTS | BIND_NO_UPDATE,
            filename, NULL, NULL, bind_image_cb);
    ok(ret, "got error %lu\n", GetLastError());
    ok(got_SysAllocString == 1, "got %u imports of SysAllocString\n", got_SysAllocString);
    ok(got_GetOpenFileNameA == 1, "got %u imports of GetOpenFileNameA\n", got_GetOpenFileNameA);
    todo_wine ok(got_SHRegGetIntW == 1, "got %u imports of SHRegGetIntW\n", got_SHRegGetIntW);

    ret = DeleteFileA(filename);
    ok(ret, "got error %lu\n", GetLastError());
}

static void test_image_load(void)
{
    char temp_file[MAX_PATH];
    PLOADED_IMAGE img;
    DWORD ret, count;
    HANDLE file;

    file = create_temp_file(temp_file);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("couldn't create temp file\n");
        return;
    }

    WriteFile(file, &bin, sizeof(bin), &count, NULL);
    CloseHandle(file);

    img = ImageLoad(temp_file, NULL);
    ok(img != NULL, "ImageLoad unexpectedly failed\n");

    if (img)
    {
        ok(!strcmp(img->ModuleName, temp_file),
           "unexpected ModuleName, got %s instead of %s\n", img->ModuleName, temp_file);
        ok(img->MappedAddress != NULL, "MappedAddress != NULL\n");
        if (img->MappedAddress)
        {
            ok(!memcmp(img->MappedAddress, &bin.dos_header, sizeof(bin.dos_header)),
               "MappedAddress doesn't point to IMAGE_DOS_HEADER\n");
        }
        ok(img->FileHeader != NULL, "FileHeader != NULL\n");
        if (img->FileHeader)
        {
            ok(!memcmp(img->FileHeader, &bin.nt_headers, sizeof(bin.nt_headers)),
                "FileHeader doesn't point to IMAGE_NT_HEADERS32\n");
        }
        ok(img->NumberOfSections == 3,
           "unexpected NumberOfSections, got %ld instead of 3\n", img->NumberOfSections);
        if (img->NumberOfSections >= 3)
        {
            ok(!strcmp((const char *)img->Sections[0].Name, ".text"),
               "unexpected name for section 0, expected .text, got %s\n",
               (const char *)img->Sections[0].Name);
            ok(!strcmp((const char *)img->Sections[1].Name, ".bss"),
               "unexpected name for section 1, expected .bss, got %s\n",
               (const char *)img->Sections[1].Name);
            ok(!strcmp((const char *)img->Sections[2].Name, ".idata"),
               "unexpected name for section 2, expected .idata, got %s\n",
               (const char *)img->Sections[2].Name);
        }
        ok(img->Characteristics == 0x102,
           "unexpected Characteristics, got 0x%lx instead of 0x102\n", img->Characteristics);
        ok(img->fSystemImage == 0,
           "unexpected fSystemImage, got %d instead of 0\n", img->fSystemImage);
        ok(img->fDOSImage == 0,
           "unexpected fDOSImage, got %d instead of 0\n", img->fDOSImage);
        todo_wine ok(img->fReadOnly == 1 || broken(!img->fReadOnly) /* <= WinXP */,
           "unexpected fReadOnly, got %d instead of 1\n", img->fReadOnly);
        todo_wine ok(img->Version == 1 || broken(!img->Version) /* <= WinXP */,
           "unexpected Version, got %d instead of 1\n", img->Version);
        ok(img->SizeOfImage == 0x600,
           "unexpected SizeOfImage, got 0x%lx instead of 0x600\n", img->SizeOfImage);

        count = 0xdeadbeef;
        ret = GetImageUnusedHeaderBytes(img, &count);
        todo_wine
        ok(ret == 448, "GetImageUnusedHeaderBytes returned %lu instead of 448\n", ret);
        todo_wine
        ok(count == 64, "unexpected size for unused header bytes, got %lu instead of 64\n", count);

        ImageUnload(img);
    }

    DeleteFileA(temp_file);
}

START_TEST(image)
{
    test_get_digest_stream();
    test_bind_image_ex();
    test_image_load();
}
