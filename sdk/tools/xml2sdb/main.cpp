/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS xml to sdb converter
 * FILE:        sdk/tools/xml2sdb/main.cpp
 * PURPOSE:     Implement platform agnostic read / write / allocation functions, parse commandline
 * PROGRAMMERS: Mark Jansen
 *
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
    PDB db;
    FILE* f;
    std::string pathA(path, path + SdbpStrlen(path));

    f = fopen(pathA.c_str(), write ? "wb" : "rb");
    if (!f)
        return NULL;

    db = (PDB)SdbAlloc(sizeof(DB));
    db->file = f;

    return db;
}

void WINAPI SdbpFlush(PDB db)
{
    fwrite(db->data, db->write_iter, 1, (FILE*)db->file);
}

void WINAPI SdbCloseDatabase(PDB db)
{
    if (!db)
        return;

    if (db->file)
        fclose((FILE*)db->file);
    if (db->string_buffer)
        SdbCloseDatabase(db->string_buffer);
    if (db->string_lookup)
        SdbpTableDestroy(&db->string_lookup);
    SdbFree(db->data);
    SdbFree(db);
}

BOOL WINAPI SdbpCheckTagType(TAG tag, WORD type)
{
    if ((tag & TAG_TYPE_MASK) != type)
        return FALSE;
    return TRUE;
}

BOOL WINAPI SdbpReadData(PDB db, PVOID dest, DWORD offset, DWORD num)
{
    DWORD size = offset + num;

    /* Either overflow or no data to read */
    if (size <= offset)
        return FALSE;

    /* Overflow */
    if (db->size < size)
        return FALSE;

    memcpy(dest, db->data + offset, num);
    return TRUE;
}

TAG WINAPI SdbGetTagFromTagID(PDB db, TAGID tagid)
{
    TAG data;
    if (!SdbpReadData(db, &data, tagid, sizeof(data)))
        return TAG_NULL;
    return data;
}

BOOL WINAPI SdbpCheckTagIDType(PDB db, TAGID tagid, WORD type)
{
    TAG tag = SdbGetTagFromTagID(db, tagid);
    if (tag == TAG_NULL)
        return FALSE;
    return SdbpCheckTagType(tag, type);
}

BOOL WINAPIV ShimDbgPrint(SHIM_LOG_LEVEL Level, PCSTR FunctionName, PCSTR Format, ...)
{
    va_list ArgList;
    const char* LevelStr;

    if (Level > g_ShimDebugLevel)
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
    if (argv[i][2] == 0)
    {
        ++i;
        if (i >= argc || !argv[i])
            return std::string();
        return argv[i];
    }
    return std::string(argv[i] + 2);
}

// -i R:\src\apphelp\reactos\media\sdb\sysmain.xml -oR:\build\apphelp\devenv_msvc\media\sdb\ros2.sdb
int main(int argc, char * argv[])
{
    std::string input, output;
    srand(time(0));

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '/' || argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 'i':
                input = get_strarg(argc, argv, i);
                break;
            case 'o':
                output = get_strarg(argc, argv, i);
                break;
            }
            if (!input.empty() && !output.empty())
            {
                if (!run_one(input, output))
                {
                    printf("Failed converting '%s' to '%s'\n", input.c_str(), output.c_str());
                    return 1;
                }
            }
        }
    }
    return 0;
}
