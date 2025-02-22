/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Support functions for dbghelp api test
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include <windows.h>
#include <stdio.h>
#include <zlib.h>

#include "wine/test.h"

extern IMAGE_DOS_HEADER __ImageBase;

static char szTempPath[MAX_PATH];

static const char* tmpdir()
{
    if (szTempPath[0] == '\0')
    {
        GetTempPathA(MAX_PATH, szTempPath);
        lstrcatA(szTempPath, "dbghelp_tst");
    }
    return szTempPath;
}

static int extract_one(const char* filename, const char* resid)
{
    HMODULE mod = (HMODULE)&__ImageBase;
    HGLOBAL glob;
    PVOID data, decompressed;
    uLongf size, dstsize;
    DWORD gccSize, dwErr;
    HANDLE file;
    int ret;
    HRSRC rsrc = FindResourceA(mod, resid, MAKEINTRESOURCEA(RT_RCDATA));
    ok(rsrc != 0, "Failed finding '%s' res\n", resid);
    if (!rsrc)
        return 0;

    size = SizeofResource(mod, rsrc);
    glob = LoadResource(mod, rsrc);
    ok(glob != NULL, "Failed loading '%s' res\n", resid);
    if (!glob)
        return 0;

    data = LockResource(glob);

    dstsize = 1024 * 256;
    decompressed = malloc(dstsize);

    if (uncompress(decompressed, &dstsize, data, size) != Z_OK)
    {
        ok(0, "uncompress failed for %s\n", resid);
        free(decompressed);
        return 0;
    }


    file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    gccSize = size;
    ret = WriteFile(file, decompressed, dstsize, &gccSize, NULL);
    dwErr = GetLastError();
    CloseHandle(file);
    free(decompressed);
    ok(ret, "WriteFile failed (%d)\n", dwErr);
    return ret && dstsize == gccSize;
}


int extract_msvc_dll(char szFile[MAX_PATH], char szPath[MAX_PATH])
{
    const char* dir = tmpdir();
    BOOL ret = CreateDirectoryA(dir, NULL);
    ok(ret, "CreateDirectoryA failed(%d)\n", GetLastError());

    sprintf(szFile, "%s\\uffs.pdb", dir);
    if (!extract_one(szFile, "msvc_uffs.pdb"))
        return 0;

    sprintf(szFile, "%s\\uffs.dll", dir);
    if (!extract_one(szFile, "msvc_uffs.dll"))
        return 0;

    strcpy(szPath, dir);
    return 1;
}

void cleanup_msvc_dll()
{
    char szFile[MAX_PATH];
    BOOL ret;
    const char* dir = tmpdir();

    sprintf(szFile, "%s\\uffs.pdb", dir);
    ret = DeleteFileA(szFile);
    ok(ret, "DeleteFileA failed(%d)\n", GetLastError());

    sprintf(szFile, "%s\\uffs.dll", dir);
    ret = DeleteFileA(szFile);
    ok(ret, "DeleteFileA failed(%d)\n", GetLastError());
    ret = RemoveDirectoryA(dir);
    ok(ret, "RemoveDirectoryA failed(%d)\n", GetLastError());
}

int extract_gcc_dll(char szFile[MAX_PATH])
{
    const char* dir = tmpdir();
    BOOL ret = CreateDirectoryA(dir, NULL);
    ok(ret, "CreateDirectoryA failed(%d)\n", GetLastError());

    sprintf(szFile, "%s\\uffs.dll", dir);
    if (!extract_one(szFile, "gcc_uffs.dll"))
        return 0;

    return 1;
}

void cleanup_gcc_dll()
{
    char szFile[MAX_PATH];
    BOOL ret;
    const char* dir = tmpdir();

    sprintf(szFile, "%s\\uffs.dll", dir);
    ret = DeleteFileA(szFile);
    ok(ret, "DeleteFileA failed(%d)\n", GetLastError());
    ret = RemoveDirectoryA(dir);
    ok(ret, "RemoveDirectoryA failed(%d)\n", GetLastError());
}


#if 0
static int compress_one(const char* src, const char* dest)
{
    DWORD size, size2, res;
    FILE* file = fopen(src, "rb");
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    Bytef* buffer, *buffer2;
    DWORD dwErr = GetLastError();

    buffer = malloc(size);
    res = fread(buffer, 1, size, file);

    fclose(file);

    if (res != size)
    {
        printf("Could not read file: 0x%x\n", dwErr);
        free(buffer);
        CloseHandle(file);
        return 0;
    }
    size2 = size *2;
    buffer2 = malloc(size2);
    res = compress(buffer2, &size2, buffer, size);

    free(buffer);

    if (Z_OK != res)
    {
        free(buffer2);
        return 0;
    }

    file = fopen(dest, "wb");
    res = fwrite(buffer2, 1, size2, file);
    fclose(file);

    free(buffer2);

    return size2 == res;
}

void create_compressed_files()
{
    SetCurrentDirectoryA("R:/src/trunk/reactos/modules/rostests/apitests/dbghelp");
    if (!compress_one("testdata/msvc_uffs.dll", "testdata/msvc_uffs.dll.compr"))
        printf("msvc_uffs.dll failed\n");
    if (!compress_one("testdata/msvc_uffs.pdb", "testdata/msvc_uffs.pdb.compr"))
        printf("msvc_uffs.pdb failed\n");
    if (!compress_one("testdata/gcc_uffs.dll", "testdata/gcc_uffs.dll.compr"))
        printf("gcc_uffs.dll failed\n");
}
#endif

#if 0
typedef struct _SYMBOLFILE_HEADER {
    ULONG SymbolsOffset;
    ULONG SymbolsLength;
    ULONG StringsOffset;
    ULONG StringsLength;
} SYMBOLFILE_HEADER, *PSYMBOLFILE_HEADER;

typedef struct _ROSSYM_ENTRY {
    ULONG Address;
    ULONG FunctionOffset;
    ULONG FileOffset;
    ULONG SourceLine;
} ROSSYM_ENTRY, *PROSSYM_ENTRY;


static int is_metadata(const char* name)
{
    size_t len = name ? strlen(name) : 0;
    return len > 3 && name[0] == '_' && name[1] != '_' && name[len-1] == '_' && name[len-2] == '_';
};

static void dump_rsym_internal(void* data)
{
    PSYMBOLFILE_HEADER RosSymHeader = (PSYMBOLFILE_HEADER)data;
    PROSSYM_ENTRY Entries = (PROSSYM_ENTRY)((char *)data + RosSymHeader->SymbolsOffset);
    size_t symbols = RosSymHeader->SymbolsLength / sizeof(ROSSYM_ENTRY);
    size_t i;
    char *Strings = (char *)data + RosSymHeader->StringsOffset;

    for (i = 0; i < symbols; i++)
    {
        PROSSYM_ENTRY Entry = Entries + i;
        if (!Entry->FileOffset)
        {
            if (Entry->SourceLine)
                printf("ERR: SOURCELINE (%D) ", Entry->SourceLine);
            if (is_metadata(Strings + Entry->FunctionOffset))
                printf("metadata: %s: 0x%x\n", Strings + Entry->FunctionOffset, Entry->Address);
            else
                printf("0x%x: %s\n", Entry->Address, Strings + Entry->FunctionOffset);
        }
        else
        {
            printf("0x%x: %s (%s:%u)\n", Entry->Address,
                Strings + Entry->FunctionOffset,
                Strings + Entry->FileOffset,
                Entry->SourceLine);
        }
    }

}

void dump_rsym(const char* filename)
{
    char* data;
    long size, res;
    PIMAGE_FILE_HEADER PEFileHeader;
    PIMAGE_OPTIONAL_HEADER PEOptHeader;
    PIMAGE_SECTION_HEADER PESectionHeaders;
    WORD i;

    FILE* f = fopen(filename, "rb");

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = malloc(size);
    res = fread(data, 1, size, f);
    fclose(f);

    PEFileHeader = (PIMAGE_FILE_HEADER)((char *)data + ((PIMAGE_DOS_HEADER)data)->e_lfanew + sizeof(ULONG));
    PEOptHeader = (PIMAGE_OPTIONAL_HEADER)(PEFileHeader + 1);
    PESectionHeaders = (PIMAGE_SECTION_HEADER)((char *)PEOptHeader + PEFileHeader->SizeOfOptionalHeader);

    for (i = 0; i < PEFileHeader->NumberOfSections; i++)
    {
        if (!strcmp((char *)PESectionHeaders[i].Name, ".rossym"))
        {
            dump_rsym_internal(data + PESectionHeaders[i].PointerToRawData);
            break;
        }
    }
    free(data);
}

#endif
