/*
 * PROJECT:     shimdbg
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test tool for SHIM engine caching
 * COPYRIGHT:   Copyright 2016-2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <ntndk.h>

NTSYSAPI ULONG NTAPI vDbgPrintEx(_In_ ULONG ComponentId, _In_ ULONG Level, _In_z_ PCCH Format, _In_ va_list ap);
#define DPFLTR_ERROR_LEVEL 0

void xprintf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    vDbgPrintEx(-1, DPFLTR_ERROR_LEVEL, fmt, ap);
    va_end(ap);
}


void CallApphelp(APPHELPCACHESERVICECLASS Service,
                PAPPHELP_CACHE_SERVICE_LOOKUP CacheEntry)
{
    NTSTATUS Status = NtApphelpCacheControl(Service, CacheEntry);
    xprintf("NtApphelpCacheControl returned 0x%x\n", (unsigned int)Status);
}

HANDLE MapFile(char* filename, UNICODE_STRING* PathName, int MapIt)
{
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE FileHandle = NULL;
    RtlCreateUnicodeStringFromAsciiz(PathName, filename);
    if (MapIt)
    {
        InitializeObjectAttributes(&LocalObjectAttributes, PathName,
            OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = NtOpenFile(&FileHandle,
                    SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_READ_DATA | FILE_EXECUTE,
                    &LocalObjectAttributes, &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
        if (!NT_SUCCESS(Status))
        {
            xprintf("Failed opening the file, using a NULL handle\n");
            FileHandle = NULL;
        }
    }
    return FileHandle;
}

void CallApphelpWithImage(char* filename, int MapIt,
                        APPHELPCACHESERVICECLASS Service, char* ServiceName)
{
    UNICODE_STRING PathName = {0};
    APPHELP_CACHE_SERVICE_LOOKUP CacheEntry;

    HANDLE FileHandle = MapFile(filename, &PathName, MapIt);

    xprintf("Calling %s %s mapping\n", ServiceName, (MapIt ? "with" : "without"));

    RtlInitUnicodeString(&CacheEntry.ImageName, PathName.Buffer);
    CacheEntry.ImageHandle = FileHandle ? FileHandle : (HANDLE)-1;
    CallApphelp(Service, &CacheEntry);
    // we piggy-back on the PathName, so let the Cleanup take care of the string
    //RtlFreeUnicodeString(&CacheEntry.ImageName);

    if (FileHandle)
        NtClose(FileHandle);
    RtlFreeUnicodeString(&PathName);
}

int IsOpt(char* argv, const char* check)
{
    if( argv && (argv[0] == '-' || argv[0] == '/') ) {
        return !_strnicmp(argv + 1, check, strlen(check));
    }
    return 0;
}

int HandleImageArg(int argc, char* argv[], int* pn, char MapItChar,
                    APPHELPCACHESERVICECLASS Service, char* ServiceName)
{
    int n = *pn;
    if (n+1 < argc)
    {
        int MapIt = argv[n][1] == MapItChar;
        CallApphelpWithImage(argv[n+1], MapIt, Service, ServiceName);
        (*pn) += 1;
        return 0;
    }
    xprintf("Error: no image name specified\n");
    return 1;
}

typedef WORD TAG;
typedef UINT64 QWORD;

#define TAG_TYPE_MASK 0xF000
#define TAG_TYPE_DWORD 0x4000
#define TAG_TYPE_QWORD 0x5000
#define TAG_TYPE_STRINGREF 0x6000

#define ATTRIBUTE_AVAILABLE 0x1
#define ATTRIBUTE_FAILED 0x2

typedef struct tagATTRINFO
{
    TAG   type;
    DWORD flags;  /* ATTRIBUTE_AVAILABLE, ATTRIBUTE_FAILED */
    union
    {
        QWORD qwattr;
        DWORD dwattr;
        WCHAR *lpattr;
    };
} ATTRINFO, *PATTRINFO;

static PVOID hdll;
static LPCWSTR (WINAPI *pSdbTagToString)(TAG);
static BOOL (WINAPI *pSdbGetFileAttributes)(LPCWSTR, PATTRINFO *, LPDWORD);
static BOOL (WINAPI *pSdbFreeFileAttributes)(PATTRINFO);

static BOOL InitApphelp()
{
    if (!hdll)
    {
        static UNICODE_STRING DllName = RTL_CONSTANT_STRING(L"apphelp.dll");
        static ANSI_STRING SdbTagToString = RTL_CONSTANT_STRING("SdbTagToString");
        static ANSI_STRING SdbGetFileAttributes = RTL_CONSTANT_STRING("SdbGetFileAttributes");
        static ANSI_STRING SdbFreeFileAttributes = RTL_CONSTANT_STRING("SdbFreeFileAttributes");
        if (!NT_SUCCESS(LdrLoadDll(NULL, NULL, &DllName, &hdll)))
        {
            xprintf("Unable to load apphelp.dll\n");
            return FALSE;
        }
        if (!NT_SUCCESS(LdrGetProcedureAddress(hdll, &SdbTagToString, 0, (PVOID)&pSdbTagToString)) ||
            !NT_SUCCESS(LdrGetProcedureAddress(hdll, &SdbGetFileAttributes, 0, (PVOID)&pSdbGetFileAttributes)) ||
            !NT_SUCCESS(LdrGetProcedureAddress(hdll, &SdbFreeFileAttributes, 0, (PVOID)&pSdbFreeFileAttributes)))
        {
            LdrUnloadDll(hdll);
            hdll = NULL;
            xprintf("Unable to resolve functions\n");
            return FALSE;
        }
    }
    return TRUE;
}


int HandleDumpAttributes(int argc, char* argv[], int* pn, const char* opt)
{
    UNICODE_STRING FileName;
    PATTRINFO attr;
    DWORD num_attr, n;
    int argn = *pn;
    const char* arg;

    if (!InitApphelp())
        return 1;

    if (strlen(argv[argn]) > (strlen(opt)+1))
    {
        arg = argv[argn] + strlen(opt);
    }
    else if (argn+1 >= argc)
    {
        xprintf("Error: no image name specified\n");
        return 1;
    }
    else
    {
        arg = argv[argn+1];
        (*pn) += 1;
    }

    RtlCreateUnicodeStringFromAsciiz(&FileName, arg);

    if (pSdbGetFileAttributes(FileName.Buffer, &attr, &num_attr))
    {
        xprintf("Dumping attributes for %s\n", arg);
        for (n = 0; n < num_attr; ++n)
        {
            TAG tagType;
            LPCWSTR tagName;
            if (attr[n].flags != ATTRIBUTE_AVAILABLE)
                continue;

            tagName = pSdbTagToString(attr[n].type);

            tagType = attr[n].type & TAG_TYPE_MASK;
            switch (tagType)
            {
            case TAG_TYPE_DWORD:
                xprintf("<%ls>0x%lx</%ls>\n", tagName, attr[n].dwattr, tagName);
                break;
            case TAG_TYPE_STRINGREF:
                xprintf("<%ls>%ls</%ls>\n", tagName, attr[n].lpattr, tagName);
                break;
            case TAG_TYPE_QWORD:
                xprintf("<%ls>0x%I64x</%ls>\n", tagName, attr[n].qwattr, tagName);
                break;
            default:
                xprintf("<!-- Unknown tag type: 0x%x (from 0x%x)\n", tagType, attr[n].type);
                break;
            }
        }
        xprintf("Done\n");
    }
    else
    {
        xprintf("Unable to get attributes from %s\n", arg);
    }


    RtlFreeUnicodeString(&FileName);
    return 0;
}

UNICODE_STRING AppCompatCacheKey = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCompatCache");
OBJECT_ATTRIBUTES AppCompatKeyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&AppCompatCacheKey, OBJ_CASE_INSENSITIVE);
UNICODE_STRING AppCompatCacheValue = RTL_CONSTANT_STRING(L"AppCompatCache");
#define REG_BINARY                  ( 3 )   // Free form binary


/* produce a hex dump, stolen from rdesktop.c */
void hexdump(unsigned char *p, unsigned int len)
{
    unsigned char *line = p;
    unsigned int i, thisline, offset = 0;

    while (offset < len)
    {
        xprintf("%04x ", offset);
        thisline = len - offset;
        if (thisline > 16)
            thisline = 16;

        for (i = 0; i < thisline; i++)
            xprintf("%02x ", line[i]);

        for (; i < 16; i++)
            xprintf("   ");

        for (i = 0; i < thisline; i++)
            xprintf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');

        xprintf("\n");
        offset += thisline;
        line += thisline;
    }
}

void DumpRegistryData(int IncludeDump)
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    KEY_VALUE_PARTIAL_INFORMATION KeyValueObject;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = &KeyValueObject;
    ULONG KeyInfoSize, ResultSize;

    xprintf("Dumping AppCompatCache registry key\n");

    Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &AppCompatKeyAttributes);

    Status = NtQueryValueKey(KeyHandle, &AppCompatCacheValue,
                KeyValuePartialInformation, KeyValueInformation,
                sizeof(KeyValueObject), &ResultSize);

    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + KeyValueInformation->DataLength;
        KeyValueInformation = malloc(KeyInfoSize);
        if (KeyValueInformation != NULL)
        {
            Status = NtQueryValueKey(KeyHandle, &AppCompatCacheValue,
                        KeyValuePartialInformation, KeyValueInformation,
                        KeyInfoSize, &ResultSize);
        }
    }

    if (NT_SUCCESS(Status) && KeyValueInformation->Type == REG_BINARY)
    {
        ULONG crc;
        if (IncludeDump)
            hexdump(KeyValueInformation->Data, KeyValueInformation->DataLength);
        crc = RtlComputeCrc32(0, KeyValueInformation->Data, KeyValueInformation->DataLength);
        xprintf("Len: %lu, Crc: 0x%lx\n", KeyValueInformation->DataLength, crc);
    }
    else
    {
        xprintf("Failed reading AppCompatCache from registry (0x%lx)\n", Status);
    }

    if (KeyValueInformation != &KeyValueObject)
        free(KeyValueInformation);

    if (KeyHandle)
        NtClose(KeyHandle);
}

int _getch();

int main(int argc, char* argv[])
{
    int n, unhandled = 0, keepopen = 0;

    for (n = 1; n < argc; ++n)
    {
        char* arg = argv[n];
        if (IsOpt(arg, "d"))
        {
            xprintf("Calling ApphelpCacheServiceDump\n");
            CallApphelp(ApphelpCacheServiceDump, NULL);
            unhandled = 0;
        }
        else if (IsOpt(arg, "h"))
        {
            DumpRegistryData(arg[1] == 'h');
            unhandled = 0;
        }
        else if (IsOpt(arg, "f"))
        {
            xprintf("Calling ApphelpCacheServiceFlush\n");
            CallApphelp(ApphelpCacheServiceFlush, NULL);
            unhandled = 0;
        }
        else if (IsOpt(arg, "z"))
        {
            xprintf("Calling ApphelpDBGReadRegistry\n");
            CallApphelp(ApphelpDBGReadRegistry, NULL);
            unhandled = 0;
        }
        else if (IsOpt(arg, "x"))
        {
            xprintf("Calling ApphelpDBGWriteRegistry\n");
            CallApphelp(ApphelpDBGWriteRegistry, NULL);
            unhandled = 0;
        }
        else if (IsOpt(arg, "l"))
        {
            unhandled |= HandleImageArg(argc, argv, &n, 'l',
                        ApphelpCacheServiceLookup, "ApphelpCacheServiceLookup");
        }
        else if (IsOpt(arg, "u"))
        {
            unhandled |= HandleImageArg(argc, argv, &n, 'u',
                        ApphelpCacheServiceUpdate, "ApphelpCacheServiceUpdate");
        }
        else if (IsOpt(arg, "r"))
        {
            unhandled |= HandleImageArg(argc, argv, &n, 'r',
                        ApphelpCacheServiceRemove, "ApphelpCacheServiceRemove");
        }
        else if (IsOpt(arg, "a"))
        {
            unhandled |= HandleDumpAttributes(argc, argv, &n, "a");
        }
        else if (IsOpt(arg, "k"))
        {
            keepopen = 1;
        }
        else
        {
            unhandled = 1;
        }
    }
    if (unhandled || argc == 1)
    {
        xprintf("Usage: %s [-d|-z|-x|-h|-H|-f|-[l|L] <image>|-[u|U] <image>|-[r|R] <image>|-k]\n", argv[0]);
        xprintf("           -d: Dump shim cache over debug output\n");
        xprintf("           -z: DEBUG Read shim cache from registry\n");
        xprintf("           -x: DEBUG Write shim cache to registry\n");
        xprintf("           -h: Hexdump shim registry key\n");
        xprintf("           -H: Crc + Length from shim registry key only\n");
        xprintf("           -f: Flush (clear) the shim cache\n");
        xprintf("           -l: Lookup <image> in the shim cache\n");
        xprintf("           -L: Lookup <image> in the shim cache without mapping it\n");
        xprintf("           -u: Update (insert) <image> in the shim cache\n");
        xprintf("           -U: Update (insert) <image> in the shim cache without mapping it\n");
        xprintf("           -r: Remove <image> from the shim cache\n");
        xprintf("           -R: Remove <image> from the shim cache without mapping it\n");
        xprintf("           -a: Dump file attributes as used in the appcompat database\n");
        xprintf("           -k: Keep the console open\n");
    }
    if (keepopen)
    {
        _getch();
    }
    return unhandled;
}
