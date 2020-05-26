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

#define NONAMELESSUNION
#include <windef.h>
#include <winbase.h>
#include <winver.h>
#include <winnt.h>
#include <imagehlp.h>

#include "wine/test.h"

static HMODULE hImageHlp;

static BOOL (WINAPI *pImageGetDigestStream)(HANDLE, DWORD, DIGEST_FUNCTION, DIGEST_HANDLE);
static BOOL (WINAPI *pBindImageEx)(DWORD Flags, const char *ImageName, const char *DllPath,
                                   const char *SymbolPath, PIMAGEHLP_STATUS_ROUTINE StatusRoutine);
static DWORD (WINAPI *pGetImageUnusedHeaderBytes)(PLOADED_IMAGE, LPDWORD);
static PLOADED_IMAGE (WINAPI *pImageLoad)(PCSTR, PCSTR);
static BOOL (WINAPI *pImageUnload)(PLOADED_IMAGE);


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
struct Imports {
    IMAGE_IMPORT_DESCRIPTOR descriptors[2];
    IMAGE_THUNK_DATA32 original_thunks[2];
    IMAGE_THUNK_DATA32 thunks[2];
    struct __IMPORT_BY_NAME {
        WORD hint;
        char funcname[0x20];
    } ibn;
    char dllname[0x10];
};
#define EXIT_PROCESS (VA_START+RVA_IDATA+FIELD_OFFSET(struct Imports, thunks))

static struct _PeImage {
    IMAGE_DOS_HEADER dos_header;
    char __alignment1[FILE_PE_START - sizeof(IMAGE_DOS_HEADER)];
    IMAGE_NT_HEADERS32 nt_headers;
    IMAGE_SECTION_HEADER sections[NUM_SECTIONS];
    char __alignment2[FILE_TEXT - FILE_PE_START - sizeof(IMAGE_NT_HEADERS32) -
        NUM_SECTIONS * sizeof(IMAGE_SECTION_HEADER)];
    unsigned char text_section[FILE_IDATA-FILE_TEXT];
    struct Imports idata_section;
    char __alignment3[FILE_TOTAL-FILE_IDATA-sizeof(struct Imports)];
} bin = {
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
             {RVA_IDATA, sizeof(struct Imports)}
            }
        }
    },
    /* sections */
    {
        {".text", {0x100}, RVA_TEXT, FILE_IDATA-FILE_TEXT, FILE_TEXT,
            0, 0, 0, 0, IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ},
        {".bss", {0x400}, RVA_BSS, 0, 0, 0, 0, 0, 0,
            IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE},
        {".idata", {sizeof(struct Imports)}, RVA_IDATA, FILE_TOTAL-FILE_IDATA, FILE_IDATA, 0,
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
            {{RVA_IDATA + FIELD_OFFSET(struct Imports, original_thunks)}, 0, 0,
            RVA_IDATA + FIELD_OFFSET(struct Imports, dllname),
            RVA_IDATA + FIELD_OFFSET(struct Imports, thunks)
            },
            {{0}, 0, 0, 0, 0}
        },
        {{{RVA_IDATA+FIELD_OFFSET(struct Imports, ibn)}}, {{0}}},
        {{{RVA_IDATA+FIELD_OFFSET(struct Imports, ibn)}}, {{0}}},
        {0,"ExitProcess"},
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

static int status_routine_called[BindSymbolsNotUpdated+1];


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
        ok(expected->cUpdates == got->cUpdates, "%s: expected %d updates, got %d\n",
            header, expected->cUpdates, got->cUpdates);
    for (i = 0; i < min(expected->cUpdates, got->cUpdates); i++)
    {
        ok(expected->updates[i].cb == got->updates[i].cb, "%s, update %d: expected %d bytes, got %d\n",
                header, i, expected->updates[i].cb, got->updates[i].cb);
        if (expected->updates[i].cb && expected->updates[i].cb == got->updates[i].cb)
            ok(!memcmp(expected->updates[i].pb, got->updates[i].pb, got->updates[i].cb),
                    "%s, update %d: unexpected value\n", header, i);
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
    {sizeof(bin.idata_section.descriptors[0].u.OriginalFirstThunk),
        &bin.idata_section.descriptors[0].u.OriginalFirstThunk},
    {FIELD_OFFSET(struct Imports, thunks)-
        (FIELD_OFFSET(struct Imports, descriptors)+FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, Name)),
        &bin.idata_section.descriptors[0].Name},
    {FILE_TOTAL-FILE_IDATA-FIELD_OFFSET(struct Imports, ibn),
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

static void update_checksum(void)
{
    WORD const * ptr;
    DWORD size;
    DWORD sum = 0;

    bin.nt_headers.OptionalHeader.CheckSum = 0;

    for(ptr = (WORD const *)&bin, size = (sizeof(bin)+1)/sizeof(WORD); size > 0; ptr++, size--)
    {
        sum += *ptr;
        if (HIWORD(sum) != 0)
        {
            sum = LOWORD(sum) + HIWORD(sum);
        }
    }
    sum = (WORD)(LOWORD(sum) + HIWORD(sum));
    sum += sizeof(bin);

    bin.nt_headers.OptionalHeader.CheckSum = sum;
}

static BOOL CALLBACK testing_status_routine(IMAGEHLP_STATUS_REASON reason, const char *ImageName,
                                            const char *DllName, ULONG_PTR Va, ULONG_PTR Parameter)
{
    char kernel32_path[MAX_PATH];

    if (0 <= (int)reason && reason <= BindSymbolsNotUpdated)
      status_routine_called[reason]++;
    else
      ok(0, "expected reason between 0 and %d, got %d\n", BindSymbolsNotUpdated+1, reason);

    switch(reason)
    {
        case BindImportModule:
            ok(!strcmp(DllName, "KERNEL32.DLL"), "expected DllName to be KERNEL32.DLL, got %s\n",
               DllName);
            break;

        case BindImportProcedure:
        case BindForwarderNOT:
            GetSystemDirectoryA(kernel32_path, MAX_PATH);
            strcat(kernel32_path, "\\KERNEL32.DLL");
            ok(!lstrcmpiA(DllName, kernel32_path), "expected DllName to be %s, got %s\n",
               kernel32_path, DllName);
            ok(!strcmp((char *)Parameter, "ExitProcess"),
               "expected Parameter to be ExitProcess, got %s\n", (char *)Parameter);
            break;

        default:
            ok(0, "got unexpected reason %d\n", reason);
            break;
    }
    return TRUE;
}

static void test_get_digest_stream(void)
{
    BOOL ret;
    HANDLE file;
    char temp_file[MAX_PATH];
    DWORD count;
    struct update_accum accum = { 0, NULL };

    if (!pImageGetDigestStream)
    {
        win_skip("ImageGetDigestStream function is not available\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pImageGetDigestStream(NULL, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    file = create_temp_file(temp_file);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("couldn't create temp file\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pImageGetDigestStream(file, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pImageGetDigestStream(NULL, 0, accumulating_stream_output, &accum);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    /* Even with "valid" parameters, it fails with an empty file */
    SetLastError(0xdeadbeef);
    ret = pImageGetDigestStream(file, 0, accumulating_stream_output, &accum);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    /* Finally, with a valid executable in the file, it succeeds.  Note that
     * the file pointer need not be positioned at the beginning.
     */
    update_checksum();
    WriteFile(file, &bin, sizeof(bin), &count, NULL);
    FlushFileBuffers(file);

    /* zero out some fields ImageGetDigestStream would zero out */
    bin.nt_headers.OptionalHeader.CheckSum = 0;
    bin.nt_headers.OptionalHeader.SizeOfInitializedData = 0;
    bin.nt_headers.OptionalHeader.SizeOfImage = 0;

    ret = pImageGetDigestStream(file, 0, accumulating_stream_output, &accum);
    ok(ret, "ImageGetDigestStream failed: %d\n", GetLastError());
    check_updates("flags = 0", &a1, &accum);
    free_updates(&accum);
    ret = pImageGetDigestStream(file, CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO,
     accumulating_stream_output, &accum);
    ok(ret, "ImageGetDigestStream failed: %d\n", GetLastError());
    check_updates("flags = CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO", &a2, &accum);
    free_updates(&accum);
    CloseHandle(file);
    DeleteFileA(temp_file);
}

static void test_bind_image_ex(void)
{
    BOOL ret;
    HANDLE file;
    char temp_file[MAX_PATH];
    DWORD count;

    if (!pBindImageEx)
    {
        win_skip("BindImageEx function is not available\n");
        return;
    }

    /* call with a non-existent file */
    SetLastError(0xdeadbeef);
    ret = pBindImageEx(BIND_NO_BOUND_IMPORTS | BIND_NO_UPDATE | BIND_ALL_IMAGES, "nonexistent.dll", 0, 0,
                       testing_status_routine);
    ok(!ret && ((GetLastError() == ERROR_FILE_NOT_FOUND) ||
       (GetLastError() == ERROR_INVALID_PARAMETER)),
       "expected ERROR_FILE_NOT_FOUND or ERROR_INVALID_PARAMETER, got %d\n",
       GetLastError());

    file = create_temp_file(temp_file);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("couldn't create temp file\n");
        return;
    }

    WriteFile(file, &bin, sizeof(bin), &count, NULL);
    CloseHandle(file);

    /* call with a proper PE file, but with StatusRoutine set to NULL */
    ret = pBindImageEx(BIND_NO_BOUND_IMPORTS | BIND_NO_UPDATE | BIND_ALL_IMAGES, temp_file, 0, 0,
                       NULL);
    ok(ret, "BindImageEx failed: %d\n", GetLastError());

    /* call with a proper PE file and StatusRoutine */
    ret = pBindImageEx(BIND_NO_BOUND_IMPORTS | BIND_NO_UPDATE | BIND_ALL_IMAGES, temp_file, 0, 0,
                       testing_status_routine);
    ok(ret, "BindImageEx failed: %d\n", GetLastError());

    ok(status_routine_called[BindImportModule] == 1,
       "StatusRoutine was called %d times\n", status_routine_called[BindImportModule]);

    ok((status_routine_called[BindImportProcedure] == 1)
#if defined(_WIN64)
       || broken(status_routine_called[BindImportProcedure] == 0) /* < Win8 */
#endif
       , "StatusRoutine was called %d times\n", status_routine_called[BindImportProcedure]);

    DeleteFileA(temp_file);
}

static void test_image_load(void)
{
    char temp_file[MAX_PATH];
    PLOADED_IMAGE img;
    DWORD ret, count;
    HANDLE file;

    if (!pImageLoad || !pImageUnload || !pGetImageUnusedHeaderBytes)
    {
        win_skip("ImageLoad functions are not available\n");
        return;
    }

    file = create_temp_file(temp_file);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("couldn't create temp file\n");
        return;
    }

    WriteFile(file, &bin, sizeof(bin), &count, NULL);
    CloseHandle(file);

    img = pImageLoad(temp_file, NULL);
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
           "unexpected NumberOfSections, got %d instead of 3\n", img->NumberOfSections);
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
           "unexpected Characteristics, got 0x%x instead of 0x102\n", img->Characteristics);
        ok(img->fSystemImage == 0,
           "unexpected fSystemImage, got %d instead of 0\n", img->fSystemImage);
        ok(img->fDOSImage == 0,
           "unexpected fDOSImage, got %d instead of 0\n", img->fDOSImage);
        todo_wine ok(img->fReadOnly == 1 || broken(!img->fReadOnly) /* <= WinXP */,
           "unexpected fReadOnly, got %d instead of 1\n", img->fReadOnly);
        todo_wine ok(img->Version == 1 || broken(!img->Version) /* <= WinXP */,
           "unexpected Version, got %d instead of 1\n", img->Version);
        ok(img->SizeOfImage == 0x600,
           "unexpected SizeOfImage, got 0x%x instead of 0x600\n", img->SizeOfImage);

        count = 0xdeadbeef;
        ret = pGetImageUnusedHeaderBytes(img, &count);
        todo_wine
        ok(ret == 448, "GetImageUnusedHeaderBytes returned %u instead of 448\n", ret);
        todo_wine
        ok(count == 64, "unexpected size for unused header bytes, got %u instead of 64\n", count);

        pImageUnload(img);
    }

    DeleteFileA(temp_file);
}

START_TEST(image)
{
    hImageHlp = LoadLibraryA("imagehlp.dll");

    if (!hImageHlp)
    {
        win_skip("ImageHlp unavailable\n");
        return;
    }

    pImageGetDigestStream = (void *) GetProcAddress(hImageHlp, "ImageGetDigestStream");
    pBindImageEx = (void *) GetProcAddress(hImageHlp, "BindImageEx");
    pGetImageUnusedHeaderBytes = (void *) GetProcAddress(hImageHlp, "GetImageUnusedHeaderBytes");
    pImageLoad = (void *) GetProcAddress(hImageHlp, "ImageLoad");
    pImageUnload = (void *) GetProcAddress(hImageHlp, "ImageUnload");

    test_get_digest_stream();
    test_bind_image_ex();
    test_image_load();

    FreeLibrary(hImageHlp);
}
