/*
 * PROJECT:     xml2sdb
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Implement platform agnostic read / write / allocation functions, parse commandline
 * COPYRIGHT:   Copyright 2016,2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "xml2sdb.h"
#include "sdbpapi.h"
#include "sdbstringtable.h"
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

extern "C"
{
ULONG g_ShimDebugLevel = SHIM_WARN;

LPVOID WINAPI SdbpAlloc(SIZE_T size)
{
    return ::calloc(1, size);
}

LPVOID WINAPI SdbpReAlloc(LPVOID mem, SIZE_T size, SIZE_T oldSize)
{
    LPVOID newMem = ::realloc(mem, size);
    if (newMem && size > oldSize)
    {
        memset((BYTE*)newMem + oldSize, 0, size - oldSize);
    }
    return newMem;
}

void WINAPI SdbpFree(LPVOID mem)
{
    return ::free(mem);
}

DWORD SdbpStrlen(PCWSTR string)
{
    size_t len = 0;
    while (string[len])
        len++;
    return len;
}

DWORD WINAPI SdbpStrsize(PCWSTR string)
{
    return (SdbpStrlen(string) + 1) * sizeof(WCHAR);
}

PDB WINAPI SdbpCreate(LPCWSTR path, PATH_TYPE type, BOOL write)
{
    PDB pdb;
    FILE* f;
    std::string pathA(path, path + SdbpStrlen(path));

    f = fopen(pathA.c_str(), write ? "wb" : "rb");
    if (!f)
        return NULL;

    pdb = (PDB)SdbAlloc(sizeof(DB));
    pdb->file = f;
    pdb->for_write = write;

    return pdb;
}

void WINAPI SdbpFlush(PDB pdb)
{
    ASSERT(pdb->for_write);

    fwrite(pdb->data, pdb->write_iter, 1, (FILE*)pdb->file);
}

void WINAPI SdbCloseDatabase(PDB pdb)
{
    if (!pdb)
        return;

    if (pdb->file)
        fclose((FILE*)pdb->file);
    if (pdb->string_buffer)
        SdbCloseDatabase(pdb->string_buffer);
    if (pdb->string_lookup)
        SdbpTableDestroy(&pdb->string_lookup);
    SdbFree(pdb->data);
    SdbFree(pdb);
}

BOOL WINAPI SdbpCheckTagType(TAG tag, WORD type)
{
    if ((tag & TAG_TYPE_MASK) != type)
        return FALSE;
    return TRUE;
}

BOOL WINAPI SdbpReadData(PDB pdb, PVOID dest, DWORD offset, DWORD num)
{
    DWORD size = offset + num;

    /* Either overflow or no data to read */
    if (size <= offset)
        return FALSE;

    /* Overflow */
    if (pdb->size < size)
        return FALSE;

    memcpy(dest, pdb->data + offset, num);
    return TRUE;
}

TAG WINAPI SdbGetTagFromTagID(PDB pdb, TAGID tagid)
{
    TAG data;
    if (!SdbpReadData(pdb, &data, tagid, sizeof(data)))
        return TAG_NULL;
    return data;
}

BOOL WINAPI SdbpCheckTagIDType(PDB pdb, TAGID tagid, WORD type)
{
    TAG tag = SdbGetTagFromTagID(pdb, tagid);
    if (tag == TAG_NULL)
        return FALSE;
    return SdbpCheckTagType(tag, type);
}

BOOL WINAPIV ShimDbgPrint(SHIM_LOG_LEVEL Level, PCSTR FunctionName, PCSTR Format, ...)
{
    va_list ArgList;
    const char* LevelStr;

    if ((ULONG)Level > g_ShimDebugLevel)
        return FALSE;

    switch (Level)
    {
    case SHIM_ERR:
        LevelStr = "Err ";
        break;
    case SHIM_WARN:
        LevelStr = "Warn";
        break;
    case SHIM_INFO:
        LevelStr = "Info";
        break;
    default:
        LevelStr = "User";
        break;
    }
    printf("[%s][%-20s] ", LevelStr, FunctionName);
    va_start(ArgList, Format);
    vprintf(Format, ArgList);
    va_end(ArgList);
    return TRUE;
}


#define TICKSPERSEC        10000000
#if defined(__GNUC__)
#define TICKSTO1970         0x019db1ded53e8000LL
#else
#define TICKSTO1970         0x019db1ded53e8000i64
#endif
VOID NTAPI RtlSecondsSince1970ToTime(IN ULONG SecondsSince1970,
                          OUT PLARGE_INTEGER Time)
{
    Time->QuadPart = ((LONGLONG)SecondsSince1970 * TICKSPERSEC) + TICKSTO1970;
}


}


bool xml_2_db(const char* xml, const WCHAR* sdb);

static bool run_one(std::string& input, std::string& output)
{
    sdbstring outputW(output.begin(), output.end());
    if (!xml_2_db(input.c_str(), outputW.c_str()))
        return false;
    input = output = "";
    return true;
}

static std::string get_strarg(int argc, char* argv[], int& i)
{
    if (argv[i][2] != 0)
        return std::string(argv[i] + 2);

    ++i;
    if (i >= argc || !argv[i])
        return std::string();
    return argv[i];
}

static void update_loglevel(int argc, char* argv[], int& i)
{
    std::string value = get_strarg(argc, argv, i);
    g_ShimDebugLevel = strtoul(value.c_str(), NULL, 10);
}

int main(int argc, char * argv[])
{
    std::string input, output;
    srand(time(0));

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] != '/' && argv[i][0] != '-')
            continue;

        switch(argv[i][1])
        {
        case 'i':
            input = get_strarg(argc, argv, i);
            break;
        case 'o':
            output = get_strarg(argc, argv, i);
            break;
        case 'l':
            update_loglevel(argc, argv, i);
            break;
        }
        if (input.empty() || output.empty())
            continue;

        if (!run_one(input, output))
        {
            printf("Failed converting '%s' to '%s'\n", input.c_str(), output.c_str());
            return 1;
        }
    }
    return 0;
}
