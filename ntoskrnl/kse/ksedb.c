/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel Shim Engine driver database (drvmain.sdb)
 * COPYRIGHT:   Copyright 2026 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntoskrnl.h>

/* Win32 types used by the shim database reader of apphelp */
typedef int BOOL;
#ifndef WINAPI
#define WINAPI NTAPI
#endif

/* Pool tag from internal/tag.h, which has the same name as a database tag */
#undef TAG_SHIM

#include <appcompat/sdbtypes.h>
#include <appcompat/sdbtagid.h>

#define NDEBUG
#include <debug.h>

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/kshim/drvmain.htm

/* The database is read with the shim database reader of apphelp (sdbread.c) */
TAG WINAPI SdbGetTagFromTagID(PDB pdb, TAGID tagid);
TAGID WINAPI SdbGetFirstChild(PDB pdb, TAGID parent);
TAGID WINAPI SdbGetNextChild(PDB pdb, TAGID parent, TAGID prev_child);
TAGID WINAPI SdbFindFirstTag(PDB pdb, TAGID parent, TAG tag);
TAGID WINAPI SdbFindNextTag(PDB pdb, TAGID parent, TAGID prev_child);
DWORD WINAPI SdbReadDWORDTag(PDB pdb, TAGID tagid, DWORD ret);
LPWSTR WINAPI SdbGetStringTagPtr(PDB pdb, TAGID tagid);

#define SDB_HEADER_SIZE     12
#define SDB_HEADER_MAGIC    'fbds'

static DB KseDriverDatabase;
static TAGID KseDatabaseRoot;


/* Helpers that sdbread.c expects from the rest of apphelp */

ULONG g_ShimDebugLevel = 1;

BOOL
__cdecl
ShimDbgPrint(
    IN ULONG Level,
    IN PCSTR FunctionName,
    IN PCSTR Format,
    ...)
{
    va_list ap;

    UNREFERENCED_PARAMETER(Level);

    DbgPrint("KSE: [%s] ", FunctionName);
    va_start(ap, Format);
    vDbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, Format, ap);
    va_end(ap);
    return TRUE;
}

BOOL
WINAPI
SdbpCheckTagIDType(
    IN PDB pdb,
    IN TAGID tagid,
    IN WORD type)
{
    TAG tag = SdbGetTagFromTagID(pdb, tagid);
    if (tag == TAG_NULL)
        return FALSE;
    return (tag & TAG_TYPE_MASK) == type;
}

BOOL
WINAPI
SdbIsNullGUID(
    IN CONST GUID *Guid)
{
    static const GUID NullGuid = { 0 };
    return IsEqualGUID(Guid, &NullGuid);
}


NTSTATUS
NTAPI
KseDbInitialize(
    IN PVOID DatabaseImage,
    IN ULONG DatabaseSize)
{
    PULONG Header = DatabaseImage;
    PUCHAR Data;

    if (!DatabaseImage || DatabaseSize <= SDB_HEADER_SIZE || Header[2] != SDB_HEADER_MAGIC)
        return STATUS_INVALID_IMAGE_FORMAT;

    /* The loader block is freed after boot, so keep a copy */
    Data = ExAllocatePoolWithTag(PagedPool, DatabaseSize, 'KSEd');
    if (!Data)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlCopyMemory(Data, DatabaseImage, DatabaseSize);

    RtlZeroMemory(&KseDriverDatabase, sizeof(KseDriverDatabase));
    KseDriverDatabase.data = Data;
    KseDriverDatabase.size = DatabaseSize;
    KseDriverDatabase.major = Header[0];
    KseDriverDatabase.minor = Header[1];
    KseDriverDatabase.stringtable = SdbFindFirstTag(&KseDriverDatabase, TAGID_ROOT, TAG_STRINGTABLE);

    KseDatabaseRoot = SdbFindFirstTag(&KseDriverDatabase, TAGID_ROOT, TAG_DATABASE);
    if (KseDatabaseRoot == TAGID_NULL)
    {
        ExFreePoolWithTag(Data, 'KSEd');
        RtlZeroMemory(&KseDriverDatabase, sizeof(KseDriverDatabase));
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    DPRINT1("KSE: Loaded driver database version %lu.%lu (%lu bytes)\n",
            KseDriverDatabase.major, KseDriverDatabase.minor, DatabaseSize);
    return STATUS_SUCCESS;
}

static BOOLEAN
KsepDbStringEquals(
    IN TAGID Parent,
    IN TAG Tag,
    IN PCUNICODE_STRING String)
{
    UNICODE_STRING Value;
    LPWSTR Ptr;

    Ptr = SdbGetStringTagPtr(&KseDriverDatabase, SdbFindFirstTag(&KseDriverDatabase, Parent, Tag));
    if (!Ptr)
        return FALSE;

    RtlInitUnicodeString(&Value, Ptr);
    return RtlEqualUnicodeString(&Value, String, TRUE);
}

/* All attributes of a MATCHING_FILE must match the driver */
static BOOLEAN
KsepDbMatchingFileMatches(
    IN TAGID MatchingFile,
    IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    static const UNICODE_STRING Self = RTL_CONSTANT_STRING(L"*");
    PIMAGE_NT_HEADERS NtHeaders;
    TAGID Attribute;
    TAG Tag;

    /* Only the driver itself can be checked here */
    if (!KsepDbStringEquals(MatchingFile, TAG_NAME, &Self) &&
        !KsepDbStringEquals(MatchingFile, TAG_NAME, &LdrEntry->BaseDllName))
    {
        return FALSE;
    }

    NtHeaders = RtlImageNtHeader(LdrEntry->DllBase);
    if (!NtHeaders)
        return FALSE;

    for (Attribute = SdbGetFirstChild(&KseDriverDatabase, MatchingFile);
         Attribute != TAGID_NULL;
         Attribute = SdbGetNextChild(&KseDriverDatabase, MatchingFile, Attribute))
    {
        Tag = SdbGetTagFromTagID(&KseDriverDatabase, Attribute);
        switch (Tag)
        {
            case TAG_NAME:
                break;

            case TAG_LINK_DATE:
                if (SdbReadDWORDTag(&KseDriverDatabase, Attribute, 0) != NtHeaders->FileHeader.TimeDateStamp)
                    return FALSE;
                break;

            case TAG_CHECKSUM:
                if (SdbReadDWORDTag(&KseDriverDatabase, Attribute, 0) != NtHeaders->OptionalHeader.CheckSum)
                    return FALSE;
                break;

            default:
                /* Do not shim a driver based on attributes that were not checked */
                DPRINT1("KSE: Unsupported matching attribute 0x%x for %wZ\n", Tag, &LdrEntry->BaseDllName);
                return FALSE;
        }
    }

    return TRUE;
}

VOID
NTAPI
KseDbGetShimsForDriver(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry,
    IN OUT PKSE_SHIM* Shims,
    IN OUT PULONG NumberOfShims)
{
    TAGID Driver, MatchingFile, ShimRef;
    UNICODE_STRING ShimName;
    LPWSTR Name;
    BOOLEAN Match;

    if (KseDatabaseRoot == TAGID_NULL)
        return;

    for (Driver = SdbFindFirstTag(&KseDriverDatabase, KseDatabaseRoot, TAG_KDRIVER);
         Driver != TAGID_NULL;
         Driver = SdbFindNextTag(&KseDriverDatabase, KseDatabaseRoot, Driver))
    {
        if (!KsepDbStringEquals(Driver, TAG_NAME, &LdrEntry->BaseDllName))
            continue;

        Match = TRUE;
        for (MatchingFile = SdbFindFirstTag(&KseDriverDatabase, Driver, TAG_MATCHING_FILE);
             MatchingFile != TAGID_NULL && Match;
             MatchingFile = SdbFindNextTag(&KseDriverDatabase, Driver, MatchingFile))
        {
            Match = KsepDbMatchingFileMatches(MatchingFile, LdrEntry);
        }

        if (!Match)
        {
            DPRINT1("KSE: %wZ is in the driver database, but does not match\n", &LdrEntry->BaseDllName);
            continue;
        }

        for (ShimRef = SdbFindFirstTag(&KseDriverDatabase, Driver, TAG_KSHIM_REF);
             ShimRef != TAGID_NULL;
             ShimRef = SdbFindNextTag(&KseDriverDatabase, Driver, ShimRef))
        {
            Name = SdbGetStringTagPtr(&KseDriverDatabase, SdbFindFirstTag(&KseDriverDatabase, ShimRef, TAG_NAME));
            if (!Name)
                continue;

            RtlInitUnicodeString(&ShimName, Name);
            KsepAddShimByName(&ShimName, Shims, NumberOfShims);
        }
    }
}
