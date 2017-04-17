/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/freetype.c
 * PURPOSE:         FreeType font engine interface
 * PROGRAMMERS:     Copyright 2001 Huw D M Davies for CodeWeavers.
 *                  Copyright 2006 Dmitry Timoshkov for CodeWeavers.
 *                  Copyright 2016-2017 Katayama Hirofumi MZ.
 */

/** Includes ******************************************************************/

#include <win32k.h>

#include FT_GLYPH_H
#include FT_TYPE1_TABLES_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#include FT_TRIGONOMETRY_H
#include FT_BITMAP_H
#include FT_OUTLINE_H
#include FT_WINFONTS_H
#include FT_SFNT_NAMES_H
#include FT_SYNTHESIS_H
#include FT_TRUETYPE_IDS_H

#ifndef FT_INTERNAL_INTERNAL_H
    #define  FT_INTERNAL_INTERNAL_H  <freetype/internal/internal.h>
    #include FT_INTERNAL_INTERNAL_H
#endif
#include FT_INTERNAL_TRUETYPE_TYPES_H

#include <gdi/eng/floatobj.h>
#include "font.h"

#define NDEBUG
#include <debug.h>

/* TPMF_FIXED_PITCH is confusing; brain-dead api */
#ifndef _TMPF_VARIABLE_PITCH
    #define _TMPF_VARIABLE_PITCH    TMPF_FIXED_PITCH
#endif

extern const MATRIX gmxWorldToDeviceDefault;
extern const MATRIX gmxWorldToPageDefault;

/* HACK!! Fix XFORMOBJ then use 1:16 / 16:1 */
#define gmxWorldToDeviceDefault gmxWorldToPageDefault

FT_Library  library;
static const WORD gusEnglishUS = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

/* special font names */
static const UNICODE_STRING MarlettW = RTL_CONSTANT_STRING(L"Marlett");
static const UNICODE_STRING SystemW = RTL_CONSTANT_STRING(L"System");
static const UNICODE_STRING FixedSysW = RTL_CONSTANT_STRING(L"FixedSys");

/* registry */
static UNICODE_STRING FontRegPath =
    RTL_CONSTANT_STRING(L"\\REGISTRY\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");


/* The FreeType library is not thread safe, so we have
   to serialize access to it */
static PFAST_MUTEX FreeTypeLock;

static LIST_ENTRY FontListHead;
static PFAST_MUTEX FontListLock;
static BOOL RenderingEnabled = TRUE;

#define IntLockGlobalFonts \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(FontListLock)

#define IntUnLockGlobalFonts \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(FontListLock)

#define ASSERT_GLOBALFONTS_LOCK_HELD() \
  ASSERT(FontListLock->Owner == KeGetCurrentThread())

#define IntLockFreeType \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(FreeTypeLock)

#define IntUnLockFreeType \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(FreeTypeLock)

#define ASSERT_FREETYPE_LOCK_HELD() \
  ASSERT(FreeTypeLock->Owner == KeGetCurrentThread())

#define MAX_FONT_CACHE 256

static LIST_ENTRY FontCacheListHead;
static UINT FontCacheNumEntries;

static PWCHAR ElfScripts[32] =   /* These are in the order of the fsCsb[0] bits */
{
    L"Western", /* 00 */
    L"Central_European",
    L"Cyrillic",
    L"Greek",
    L"Turkish",
    L"Hebrew",
    L"Arabic",
    L"Baltic",
    L"Vietnamese", /* 08 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 15 */
    L"Thai",
    L"Japanese",
    L"CHINESE_GB2312",
    L"Hangul",
    L"CHINESE_BIG5",
    L"Hangul(Johab)",
    NULL, NULL, /* 23 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    L"Symbol" /* 31 */
};

/*
 *  For TranslateCharsetInfo
 */
#define CP_SYMBOL   42
#define MAXTCIINDEX 32
static const CHARSETINFO FontTci[MAXTCIINDEX] =
{
    /* ANSI */
    { ANSI_CHARSET, 1252, {{0,0,0,0},{FS_LATIN1,0}} },
    { EASTEUROPE_CHARSET, 1250, {{0,0,0,0},{FS_LATIN2,0}} },
    { RUSSIAN_CHARSET, 1251, {{0,0,0,0},{FS_CYRILLIC,0}} },
    { GREEK_CHARSET, 1253, {{0,0,0,0},{FS_GREEK,0}} },
    { TURKISH_CHARSET, 1254, {{0,0,0,0},{FS_TURKISH,0}} },
    { HEBREW_CHARSET, 1255, {{0,0,0,0},{FS_HEBREW,0}} },
    { ARABIC_CHARSET, 1256, {{0,0,0,0},{FS_ARABIC,0}} },
    { BALTIC_CHARSET, 1257, {{0,0,0,0},{FS_BALTIC,0}} },
    { VIETNAMESE_CHARSET, 1258, {{0,0,0,0},{FS_VIETNAMESE,0}} },
    /* reserved by ANSI */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    /* ANSI and OEM */
    { THAI_CHARSET, 874, {{0,0,0,0},{FS_THAI,0}} },
    { SHIFTJIS_CHARSET, 932, {{0,0,0,0},{FS_JISJAPAN,0}} },
    { GB2312_CHARSET, 936, {{0,0,0,0},{FS_CHINESESIMP,0}} },
    { HANGEUL_CHARSET, 949, {{0,0,0,0},{FS_WANSUNG,0}} },
    { CHINESEBIG5_CHARSET, 950, {{0,0,0,0},{FS_CHINESETRAD,0}} },
    { JOHAB_CHARSET, 1361, {{0,0,0,0},{FS_JOHAB,0}} },
    /* Reserved for alternate ANSI and OEM */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    /* Reserved for system */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { SYMBOL_CHARSET, CP_SYMBOL, {{0,0,0,0},{FS_SYMBOL,0}} }
};

/* list head */
static RTL_STATIC_LIST_HEAD(FontSubstListHead);

static void
SharedMem_AddRef(PSHARED_MEM Ptr)
{
    ASSERT_FREETYPE_LOCK_HELD();

    ++Ptr->RefCount;
}

static PSHARED_FACE
SharedFace_Create(FT_Face Face, PSHARED_MEM Memory)
{
    PSHARED_FACE Ptr;
    Ptr = ExAllocatePoolWithTag(PagedPool, sizeof(SHARED_FACE), TAG_FONT);
    if (Ptr)
    {
        Ptr->Face = Face;
        Ptr->RefCount = 1;
        Ptr->Memory = Memory;
        SharedMem_AddRef(Memory);
        DPRINT("Creating SharedFace for %s\n", Face->family_name);
    }
    return Ptr;
}

static PSHARED_MEM
SharedMem_Create(PBYTE Buffer, ULONG BufferSize, BOOL IsMapping)
{
    PSHARED_MEM Ptr;
    Ptr = ExAllocatePoolWithTag(PagedPool, sizeof(SHARED_MEM), TAG_FONT);
    if (Ptr)
    {
        Ptr->Buffer = Buffer;
        Ptr->BufferSize = BufferSize;
        Ptr->RefCount = 1;
        Ptr->IsMapping = IsMapping;
        DPRINT("Creating SharedMem for %p (%i, %p)\n", Buffer, IsMapping, Ptr);
    }
    return Ptr;
}

static void
SharedFace_AddRef(PSHARED_FACE Ptr)
{
    ASSERT_FREETYPE_LOCK_HELD();

    ++Ptr->RefCount;
}

static void
RemoveCachedEntry(PFONT_CACHE_ENTRY Entry)
{
    ASSERT_FREETYPE_LOCK_HELD();

    FT_Done_Glyph((FT_Glyph)Entry->BitmapGlyph);
    RemoveEntryList(&Entry->ListEntry);
    ExFreePoolWithTag(Entry, TAG_FONT);
    FontCacheNumEntries--;
    ASSERT(FontCacheNumEntries <= MAX_FONT_CACHE);
}

static void
RemoveCacheEntries(FT_Face Face)
{
    PLIST_ENTRY CurrentEntry;
    PFONT_CACHE_ENTRY FontEntry;

    ASSERT_FREETYPE_LOCK_HELD();

    CurrentEntry = FontCacheListHead.Flink;
    while (CurrentEntry != &FontCacheListHead)
    {
        FontEntry = CONTAINING_RECORD(CurrentEntry, FONT_CACHE_ENTRY, ListEntry);
        CurrentEntry = CurrentEntry->Flink;

        if (FontEntry->Face == Face)
        {
            RemoveCachedEntry(FontEntry);
        }
    }
}

static void SharedMem_Release(PSHARED_MEM Ptr)
{
    ASSERT_FREETYPE_LOCK_HELD();
    ASSERT(Ptr->RefCount > 0);

    if (Ptr->RefCount <= 0)
        return;

    --Ptr->RefCount;
    if (Ptr->RefCount == 0)
    {
        DPRINT("Releasing SharedMem for %p (%i, %p)\n", Ptr->Buffer, Ptr->IsMapping, Ptr);
        if (Ptr->IsMapping)
            MmUnmapViewInSystemSpace(Ptr->Buffer);
        else
            ExFreePoolWithTag(Ptr->Buffer, TAG_FONT);
        ExFreePoolWithTag(Ptr, TAG_FONT);
    }
}

static void
SharedFace_Release(PSHARED_FACE Ptr)
{
    IntLockFreeType;
    ASSERT(Ptr->RefCount > 0);

    if (Ptr->RefCount <= 0)
        return;

    --Ptr->RefCount;
    if (Ptr->RefCount == 0)
    {
        DPRINT("Releasing SharedFace for %s\n", Ptr->Face->family_name);
        RemoveCacheEntries(Ptr->Face);
        FT_Done_Face(Ptr->Face);
        SharedMem_Release(Ptr->Memory);
        ExFreePoolWithTag(Ptr, TAG_FONT);
    }
    IntUnLockFreeType;
}


/*
 * IntLoadFontSubstList --- loads the list of font substitutes
 */
BOOL FASTCALL
IntLoadFontSubstList(PLIST_ENTRY pHead)
{
    NTSTATUS                        Status;
    HANDLE                          KeyHandle;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    KEY_FULL_INFORMATION            KeyFullInfo;
    ULONG                           i, Length;
    UNICODE_STRING                  FromW, ToW;
    BYTE                            InfoBuffer[128];
    PKEY_VALUE_FULL_INFORMATION     pInfo;
    BYTE                            CharSets[FONTSUBST_FROM_AND_TO];
    LPWSTR                          pch;
    PFONTSUBST_ENTRY                pEntry;

    /* the FontSubstitutes registry key */
    static UNICODE_STRING FontSubstKey =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\"
                            L"Microsoft\\Windows NT\\CurrentVersion\\"
                            L"FontSubstitutes");

    /* open registry key */
    InitializeObjectAttributes(&ObjectAttributes, &FontSubstKey,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwOpenKey failed: 0x%08X\n", Status);
        return FALSE;   /* failure */
    }

    /* query count of values */
    Status = ZwQueryKey(KeyHandle, KeyFullInformation,
                        &KeyFullInfo, sizeof(KeyFullInfo), &Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwQueryKey failed: 0x%08X\n", Status);
        ZwClose(KeyHandle);
        return FALSE;   /* failure */
    }

    /* for each value */
    for (i = 0; i < KeyFullInfo.Values; ++i)
    {
        /* get value name */
        Status = ZwEnumerateValueKey(KeyHandle, i, KeyValueFullInformation,
                                     InfoBuffer, sizeof(InfoBuffer), &Length);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwEnumerateValueKey failed: 0x%08X\n", Status);
            break;      /* failure */
        }

        /* create FromW string */
        pInfo = (PKEY_VALUE_FULL_INFORMATION)InfoBuffer;
        Length = pInfo->NameLength / sizeof(WCHAR);
        pInfo->Name[Length] = UNICODE_NULL;   /* truncate */
        Status = RtlCreateUnicodeString(&FromW, pInfo->Name);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("RtlCreateUnicodeString failed: 0x%08X\n", Status);
            break;      /* failure */
        }

        /* query value */
        Status = ZwQueryValueKey(KeyHandle, &FromW, KeyValueFullInformation, 
                                 InfoBuffer, sizeof(InfoBuffer), &Length);
        pInfo = (PKEY_VALUE_FULL_INFORMATION)InfoBuffer;
        if (!NT_SUCCESS(Status) || !pInfo->DataLength)
        {
            DPRINT("ZwQueryValueKey failed: 0x%08X\n", Status);
            RtlFreeUnicodeString(&FromW);
            break;      /* failure */
        }

        /* create ToW string */
        pch = (LPWSTR)((PUCHAR)pInfo + pInfo->DataOffset);
        Length = pInfo->DataLength / sizeof(WCHAR);
        pch[Length] = UNICODE_NULL; /* truncate */
        Status = RtlCreateUnicodeString(&ToW, pch);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("RtlCreateUnicodeString failed: 0x%08X\n", Status);
            RtlFreeUnicodeString(&FromW);
            break;      /* failure */
        }

        /* does charset exist? (from) */
        CharSets[FONTSUBST_FROM] = DEFAULT_CHARSET;
        pch = wcsrchr(FromW.Buffer, L',');
        if (pch)
        {
            /* truncate */
            *pch = UNICODE_NULL;
            FromW.Length = (pch - FromW.Buffer) * sizeof(WCHAR);
            /* parse charset number */
            CharSets[FONTSUBST_FROM] = (BYTE)_wtoi(pch + 1);
        }

        /* does charset exist? (to) */
        CharSets[FONTSUBST_TO] = DEFAULT_CHARSET;
        pch = wcsrchr(ToW.Buffer, L',');
        if (pch)
        {
            /* truncate */
            *pch = UNICODE_NULL;
            ToW.Length = (pch - ToW.Buffer) * sizeof(WCHAR);
            /* parse charset number */
            CharSets[FONTSUBST_TO] = (BYTE)_wtoi(pch + 1);
        }

        /* allocate an entry */
        pEntry = ExAllocatePoolWithTag(PagedPool, sizeof(FONTSUBST_ENTRY), TAG_FONT);
        if (pEntry == NULL)
        {
            DPRINT("ExAllocatePoolWithTag failed\n");
            RtlFreeUnicodeString(&FromW);
            RtlFreeUnicodeString(&ToW);
            break;      /* failure */
        }

        /* store to *pEntry */
        pEntry->FontNames[FONTSUBST_FROM] = FromW;
        pEntry->FontNames[FONTSUBST_TO] = ToW;
        pEntry->CharSets[FONTSUBST_FROM] = CharSets[FONTSUBST_FROM];
        pEntry->CharSets[FONTSUBST_TO] = CharSets[FONTSUBST_TO];

        /* insert pEntry to *pHead */
        InsertTailList(pHead, &pEntry->ListEntry);
    }

    /* close now */
    ZwClose(KeyHandle);

    return NT_SUCCESS(Status);
}

BOOL FASTCALL
InitFontSupport(VOID)
{
    ULONG ulError;

    InitializeListHead(&FontListHead);
    InitializeListHead(&FontCacheListHead);
    FontCacheNumEntries = 0;
    /* Fast Mutexes must be allocated from non paged pool */
    FontListLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), TAG_INTERNAL_SYNC);
    if (FontListLock == NULL)
    {
        return FALSE;
    }

    ExInitializeFastMutex(FontListLock);
    FreeTypeLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), TAG_INTERNAL_SYNC);
    if (FreeTypeLock == NULL)
    {
        return FALSE;
    }
    ExInitializeFastMutex(FreeTypeLock);

    ulError = FT_Init_FreeType(&library);
    if (ulError)
    {
        DPRINT1("FT_Init_FreeType failed with error code 0x%x\n", ulError);
        return FALSE;
    }

    IntLoadSystemFonts();
    IntLoadFontSubstList(&FontSubstListHead);

    return TRUE;
}

VOID
FtSetCoordinateTransform(
    FT_Face face,
    PMATRIX pmx)
{
    FT_Matrix ftmatrix;
    FLOATOBJ efTemp;

    /* Create a freetype matrix, by converting to 16.16 fixpoint format */
    efTemp = pmx->efM11;
    FLOATOBJ_MulLong(&efTemp, 0x00010000);
    ftmatrix.xx = FLOATOBJ_GetLong(&efTemp);

    efTemp = pmx->efM12;
    FLOATOBJ_MulLong(&efTemp, 0x00010000);
    ftmatrix.xy = FLOATOBJ_GetLong(&efTemp);

    efTemp = pmx->efM21;
    FLOATOBJ_MulLong(&efTemp, 0x00010000);
    ftmatrix.yx = FLOATOBJ_GetLong(&efTemp);

    efTemp = pmx->efM22;
    FLOATOBJ_MulLong(&efTemp, 0x00010000);
    ftmatrix.yy = FLOATOBJ_GetLong(&efTemp);

    /* Set the transformation matrix */
    FT_Set_Transform(face, &ftmatrix, 0);
}

static BOOL
SubstituteFontByList(PLIST_ENTRY        pHead,
                     PUNICODE_STRING    pOutputName,
                     PUNICODE_STRING    pInputName,
                     BYTE               RequestedCharSet,
                     BYTE               CharSetMap[FONTSUBST_FROM_AND_TO])
{
    NTSTATUS            Status;
    PLIST_ENTRY         pListEntry;
    PFONTSUBST_ENTRY    pSubstEntry;
    BYTE                CharSets[FONTSUBST_FROM_AND_TO];

    CharSetMap[FONTSUBST_FROM] = DEFAULT_CHARSET;
    CharSetMap[FONTSUBST_TO] = RequestedCharSet;

    /* for each list entry */
    for (pListEntry = pHead->Flink;
         pListEntry != pHead;
         pListEntry = pListEntry->Flink)
    {
        pSubstEntry =
            (PFONTSUBST_ENTRY)CONTAINING_RECORD(pListEntry, FONT_ENTRY, ListEntry);

        CharSets[FONTSUBST_FROM] = pSubstEntry->CharSets[FONTSUBST_FROM];

        if (CharSets[FONTSUBST_FROM] != DEFAULT_CHARSET &&
            CharSets[FONTSUBST_FROM] != RequestedCharSet)
        {
            continue;   /* not matched */
        }

        /* does charset number exist? (to) */
        if (pSubstEntry->CharSets[FONTSUBST_TO] != DEFAULT_CHARSET)
        {
            CharSets[FONTSUBST_TO] = pSubstEntry->CharSets[FONTSUBST_TO];
        }
        else
        {
            CharSets[FONTSUBST_TO] = RequestedCharSet;
        }

        /* does font name match? */
        if (!RtlEqualUnicodeString(&pSubstEntry->FontNames[FONTSUBST_FROM],
                                   pInputName, TRUE))
        {
            continue;   /* not matched */
        }

        /* update *pOutputName */
        RtlFreeUnicodeString(pOutputName);
        Status = RtlCreateUnicodeString(pOutputName,
                                        pSubstEntry->FontNames[FONTSUBST_TO].Buffer);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("RtlCreateUnicodeString failed: 0x%08X\n", Status);
            continue;   /* cannot create string */
        }

        if (CharSetMap[FONTSUBST_FROM] == DEFAULT_CHARSET)
        {
            /* update CharSetMap */
            CharSetMap[FONTSUBST_FROM]  = CharSets[FONTSUBST_FROM];
            CharSetMap[FONTSUBST_TO]    = CharSets[FONTSUBST_TO];
        }
        return TRUE;   /* success */
    }

    return FALSE;
}

static BOOL
SubstituteFontRecurse(PUNICODE_STRING pInOutName, BYTE *pRequestedCharSet)
{
    UINT            RecurseCount = 5;
    UNICODE_STRING  OutputNameW = { 0 };
    BYTE            CharSetMap[FONTSUBST_FROM_AND_TO];
    BOOL            Found;

    if (pInOutName->Buffer[0] == UNICODE_NULL)
        return FALSE;

    while (RecurseCount-- > 0)
    {
        RtlInitUnicodeString(&OutputNameW, NULL);
        Found = SubstituteFontByList(&FontSubstListHead,
                                     &OutputNameW, pInOutName,
                                     *pRequestedCharSet, CharSetMap);
        if (!Found)
            break;

        /* update *pInOutName and *pRequestedCharSet */
        RtlFreeUnicodeString(pInOutName);
        *pInOutName = OutputNameW;
        if (CharSetMap[FONTSUBST_FROM] == DEFAULT_CHARSET ||
            CharSetMap[FONTSUBST_FROM] == *pRequestedCharSet)
        {
            *pRequestedCharSet = CharSetMap[FONTSUBST_TO];
        }
    }

    return TRUE;    /* success */
}

/*
 * IntLoadSystemFonts
 *
 * Search the system font directory and adds each font found.
 */
VOID FASTCALL
IntLoadSystemFonts(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Directory, FileName, TempString;
    IO_STATUS_BLOCK Iosb;
    HANDLE hDirectory;
    BYTE *DirInfoBuffer;
    PFILE_DIRECTORY_INFORMATION DirInfo;
    BOOLEAN bRestartScan = TRUE;
    NTSTATUS Status;
    INT i;
    static UNICODE_STRING SearchPatterns[] =
    {
        RTL_CONSTANT_STRING(L"*.ttf"),
        RTL_CONSTANT_STRING(L"*.ttc"),
        RTL_CONSTANT_STRING(L"*.otf"),
        RTL_CONSTANT_STRING(L"*.otc"),
        RTL_CONSTANT_STRING(L"*.fon"),
        RTL_CONSTANT_STRING(L"*.fnt")
    };

    RtlInitUnicodeString(&Directory, L"\\SystemRoot\\Fonts\\");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &Directory,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    Status = ZwOpenFile(
                 &hDirectory,
                 SYNCHRONIZE | FILE_LIST_DIRECTORY,
                 &ObjectAttributes,
                 &Iosb,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);

    if (NT_SUCCESS(Status))
    {
        for (i = 0; i < _countof(SearchPatterns); ++i)
        {
            DirInfoBuffer = ExAllocatePoolWithTag(PagedPool, 0x4000, TAG_FONT);
            if (DirInfoBuffer == NULL)
            {
                ZwClose(hDirectory);
                return;
            }

            FileName.Buffer = ExAllocatePoolWithTag(PagedPool, MAX_PATH * sizeof(WCHAR), TAG_FONT);
            if (FileName.Buffer == NULL)
            {
                ExFreePoolWithTag(DirInfoBuffer, TAG_FONT);
                ZwClose(hDirectory);
                return;
            }
            FileName.Length = 0;
            FileName.MaximumLength = MAX_PATH * sizeof(WCHAR);

            while (1)
            {
                Status = ZwQueryDirectoryFile(
                             hDirectory,
                             NULL,
                             NULL,
                             NULL,
                             &Iosb,
                             DirInfoBuffer,
                             0x4000,
                             FileDirectoryInformation,
                             FALSE,
                             &SearchPatterns[i],
                             bRestartScan);

                if (!NT_SUCCESS(Status) || Status == STATUS_NO_MORE_FILES)
                {
                    break;
                }

                DirInfo = (PFILE_DIRECTORY_INFORMATION)DirInfoBuffer;
                while (1)
                {
                    TempString.Buffer = DirInfo->FileName;
                    TempString.Length =
                        TempString.MaximumLength = DirInfo->FileNameLength;
                    RtlCopyUnicodeString(&FileName, &Directory);
                    RtlAppendUnicodeStringToString(&FileName, &TempString);
                    IntGdiAddFontResource(&FileName, 0);
                    if (DirInfo->NextEntryOffset == 0)
                        break;
                    DirInfo = (PFILE_DIRECTORY_INFORMATION)((ULONG_PTR)DirInfo + DirInfo->NextEntryOffset);
                }

                bRestartScan = FALSE;
            }

            ExFreePoolWithTag(FileName.Buffer, TAG_FONT);
            ExFreePoolWithTag(DirInfoBuffer, TAG_FONT);
        }
        ZwClose(hDirectory);
    }
}

static BYTE
ItalicFromStyle(const char *style_name)
{
    if (style_name == NULL || style_name[0] == 0)
        return FALSE;
    if (strstr(style_name, "Italic") != NULL)
        return TRUE;
    if (strstr(style_name, "Oblique") != NULL)
        return TRUE;
    return FALSE;
}

static LONG
WeightFromStyle(const char *style_name)
{
    if (style_name == NULL || style_name[0] == 0)
        return FW_NORMAL;
    if (strstr(style_name, "Regular") != NULL)
        return FW_REGULAR;
    if (strstr(style_name, "Normal") != NULL)
        return FW_NORMAL;
    if (strstr(style_name, "SemiBold") != NULL)
        return FW_SEMIBOLD;
    if (strstr(style_name, "UltraBold") != NULL)
        return FW_ULTRABOLD;
    if (strstr(style_name, "DemiBold") != NULL)
        return FW_DEMIBOLD;
    if (strstr(style_name, "ExtraBold") != NULL)
        return FW_EXTRABOLD;
    if (strstr(style_name, "Bold") != NULL)
        return FW_BOLD;
    if (strstr(style_name, "UltraLight") != NULL)
        return FW_ULTRALIGHT;
    if (strstr(style_name, "ExtraLight") != NULL)
        return FW_EXTRALIGHT;
    if (strstr(style_name, "Light") != NULL)
        return FW_LIGHT;
    if (strstr(style_name, "Hairline") != NULL)
        return 50;
    if (strstr(style_name, "Book") != NULL)
        return 350;
    if (strstr(style_name, "ExtraBlack") != NULL)
        return 950;
    if (strstr(style_name, "UltraBlack") != NULL)
        return 1000;
    if (strstr(style_name, "Black") != NULL)
        return FW_BLACK;
    if (strstr(style_name, "Medium") != NULL)
        return FW_MEDIUM;
    if (strstr(style_name, "Thin") != NULL)
        return FW_THIN;
    if (strstr(style_name, "Heavy") != NULL)
        return FW_HEAVY;
    return FW_NORMAL;
}

static INT FASTCALL
IntGdiLoadFontsFromMemory(PGDI_LOAD_FONT pLoadFont,
                          PSHARED_FACE SharedFace, FT_Long FontIndex, INT CharSetIndex)
{
    FT_Error            Error;
    PFONT_ENTRY         Entry;
    FONT_ENTRY_MEM*     PrivateEntry = NULL;
    FONTGDI *           FontGDI;
    NTSTATUS            Status;
    FT_Face             Face;
    ANSI_STRING         AnsiFaceName;
    FT_WinFNT_HeaderRec WinFNT;
    INT                 FaceCount = 0, CharSetCount = 0;
    PUNICODE_STRING     pFileName       = pLoadFont->pFileName;
    DWORD               Characteristics = pLoadFont->Characteristics;
    PUNICODE_STRING     pValueName = &pLoadFont->RegValueName;
    TT_OS2 *            pOS2;
    INT                 BitIndex;
    FT_UShort           os2_version;
    FT_ULong            os2_ulCodePageRange1;
    FT_UShort           os2_usWeightClass;

    if (SharedFace == NULL && CharSetIndex == -1)
    {
        /* load a face from memory */
        IntLockFreeType;
        Error = FT_New_Memory_Face(
                    library,
                    pLoadFont->Memory->Buffer,
                    pLoadFont->Memory->BufferSize,
                    ((FontIndex != -1) ? FontIndex : 0),
                    &Face);

        if (!Error)
            SharedFace = SharedFace_Create(Face, pLoadFont->Memory);

        IntUnLockFreeType;

        if (FT_IS_SFNT(Face))
            pLoadFont->IsTrueType = TRUE;

        if (Error || SharedFace == NULL)
        {
            if (SharedFace)
                SharedFace_Release(SharedFace);

            if (Error == FT_Err_Unknown_File_Format)
                DPRINT1("Unknown font file format\n");
            else
                DPRINT1("Error reading font (error code: %d)\n", Error);
            return 0;   /* failure */
        }
    }
    else
    {
        Face = SharedFace->Face;
        IntLockFreeType;
        SharedFace_AddRef(SharedFace);
        IntUnLockFreeType;
    }

    /* allocate a FONT_ENTRY */
    Entry = ExAllocatePoolWithTag(PagedPool, sizeof(FONT_ENTRY), TAG_FONT);
    if (!Entry)
    {
        SharedFace_Release(SharedFace);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;   /* failure */
    }

    /* allocate a FONTGDI */
    FontGDI = EngAllocMem(FL_ZERO_MEMORY, sizeof(FONTGDI), GDITAG_RFONT);
    if (!FontGDI)
    {
        SharedFace_Release(SharedFace);
        ExFreePoolWithTag(Entry, TAG_FONT);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;   /* failure */
    }

    /* set file name */
    if (pFileName)
    {
        FontGDI->Filename = ExAllocatePoolWithTag(PagedPool,
                                                  pFileName->Length + sizeof(UNICODE_NULL),
                                                  GDITAG_PFF);
        if (FontGDI->Filename == NULL)
        {
            EngFreeMem(FontGDI);
            SharedFace_Release(SharedFace);
            ExFreePoolWithTag(Entry, TAG_FONT);
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;   /* failure */
        }
        RtlCopyMemory(FontGDI->Filename, pFileName->Buffer, pFileName->Length);
        FontGDI->Filename[pFileName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        FontGDI->Filename = NULL;

        PrivateEntry = ExAllocatePoolWithTag(PagedPool, sizeof(FONT_ENTRY_MEM), TAG_FONT);
        if (!PrivateEntry)
        {
            if (FontGDI->Filename)
                ExFreePoolWithTag(FontGDI->Filename, GDITAG_PFF);
            EngFreeMem(FontGDI);
            SharedFace_Release(SharedFace);
            ExFreePoolWithTag(Entry, TAG_FONT);
            return 0;
        }

        PrivateEntry->Entry = Entry;
        if (pLoadFont->PrivateEntry)
        {
            InsertTailList(&pLoadFont->PrivateEntry->ListEntry, &PrivateEntry->ListEntry);
        }
        else
        {
            InitializeListHead(&PrivateEntry->ListEntry);
            pLoadFont->PrivateEntry = PrivateEntry;
        }
    }

    /* set face */
    FontGDI->SharedFace = SharedFace;
    FontGDI->CharSet = ANSI_CHARSET;
    FontGDI->OriginalItalic = ItalicFromStyle(Face->style_name);
    FontGDI->RequestItalic = FALSE;
    FontGDI->OriginalWeight = WeightFromStyle(Face->style_name);
    FontGDI->RequestWeight = FW_NORMAL;

    RtlInitAnsiString(&AnsiFaceName, Face->family_name);
    Status = RtlAnsiStringToUnicodeString(&Entry->FaceName, &AnsiFaceName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        if (PrivateEntry)
        {
            if (pLoadFont->PrivateEntry == PrivateEntry)
            {
                pLoadFont->PrivateEntry = NULL;
            }
            else
            {
                RemoveEntryList(&PrivateEntry->ListEntry);
            }
            ExFreePoolWithTag(PrivateEntry, TAG_FONT);
        }
        if (FontGDI->Filename)
            ExFreePoolWithTag(FontGDI->Filename, GDITAG_PFF);
        EngFreeMem(FontGDI);
        SharedFace_Release(SharedFace);
        ExFreePoolWithTag(Entry, TAG_FONT);
        return 0;
    }

    os2_version = 0;
    IntLockFreeType;
    pOS2 = (TT_OS2 *)FT_Get_Sfnt_Table(Face, FT_SFNT_OS2);
    if (pOS2)
    {
        os2_version = pOS2->version;
        os2_ulCodePageRange1 = pOS2->ulCodePageRange1;
        os2_usWeightClass = pOS2->usWeightClass;
    }
    IntUnLockFreeType;

    if (pOS2 && os2_version >= 1)
    {
        /* get charset and weight from OS/2 header */

        /* Make sure we do not use this pointer anymore */
        pOS2 = NULL;

        for (BitIndex = 0; BitIndex < MAXTCIINDEX; ++BitIndex)
        {
            if (os2_ulCodePageRange1 & (1 << BitIndex))
            {
                if (FontTci[BitIndex].ciCharset == DEFAULT_CHARSET)
                    continue;

                if ((CharSetIndex == -1 && CharSetCount == 0) ||
                    CharSetIndex == CharSetCount)
                {
                    FontGDI->CharSet = FontTci[BitIndex].ciCharset;
                }

                ++CharSetCount;
            }
        }

        /* set actual weight */
        FontGDI->OriginalWeight = os2_usWeightClass;
    }
    else
    {
        /* get charset from WinFNT header */
        IntLockFreeType;
        Error = FT_Get_WinFNT_Header(Face, &WinFNT);
        if (!Error)
        {
            FontGDI->CharSet = WinFNT.charset;
        }
        IntUnLockFreeType;
    }

    /* FIXME: CharSet is invalid on Marlett */
    if (RtlEqualUnicodeString(&Entry->FaceName, &MarlettW, TRUE))
    {
        FontGDI->CharSet = SYMBOL_CHARSET;
    }

    ++FaceCount;
    DPRINT("Font loaded: %s (%s)\n", Face->family_name, Face->style_name);
    DPRINT("Num glyphs: %d\n", Face->num_glyphs);
    DPRINT("CharSet: %d\n", FontGDI->CharSet);

    /* Add this font resource to the font table */
    Entry->Font = FontGDI;
    Entry->NotEnum = (Characteristics & FR_NOT_ENUM);

    if (Characteristics & FR_PRIVATE)
    {
        /* private font */
        PPROCESSINFO Win32Process = PsGetCurrentProcessWin32Process();
        IntLockProcessPrivateFonts(Win32Process);
        InsertTailList(&Win32Process->PrivateFontListHead, &Entry->ListEntry);
        IntUnLockProcessPrivateFonts(Win32Process);
    }
    else
    {
        /* global font */
        IntLockGlobalFonts;
        InsertTailList(&FontListHead, &Entry->ListEntry);
        IntUnLockGlobalFonts;
    }

    if (FontIndex == -1)
    {
        if (FT_IS_SFNT(Face))
        {
            TT_Face TrueType = (TT_Face)Face;
            if (TrueType->ttc_header.count > 1)
            {
                FT_Long i;
                for (i = 1; i < TrueType->ttc_header.count; ++i)
                {
                    FaceCount += IntGdiLoadFontsFromMemory(pLoadFont, NULL, i, -1);
                }
            }
        }
        FontIndex = 0;
    }

    if (CharSetIndex == -1)
    {
        INT i;

        if (pLoadFont->RegValueName.Length == 0)
        {
            RtlCreateUnicodeString(pValueName, Entry->FaceName.Buffer);
        }
        else
        {
            UNICODE_STRING NewString;
            USHORT Length = pValueName->Length + 3 * sizeof(WCHAR) + Entry->FaceName.Length;
            NewString.Length = 0;
            NewString.MaximumLength = Length + sizeof(WCHAR);
            NewString.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                     NewString.MaximumLength,
                                                     TAG_USTR);
            NewString.Buffer[0] = UNICODE_NULL;

            RtlAppendUnicodeStringToString(&NewString, pValueName);
            RtlAppendUnicodeToString(&NewString, L" & ");
            RtlAppendUnicodeStringToString(&NewString, &Entry->FaceName);

            RtlFreeUnicodeString(pValueName);
            *pValueName = NewString;
        }

        for (i = 1; i < CharSetCount; ++i)
        {
            /* Do not count charsets towards 'faces' loaded */
            IntGdiLoadFontsFromMemory(pLoadFont, SharedFace, FontIndex, i);
        }
    }

    return FaceCount;   /* number of loaded faces */
}

/*
 * IntGdiAddFontResource
 *
 * Adds the font resource from the specified file to the system.
 */

INT FASTCALL
IntGdiAddFontResource(PUNICODE_STRING FileName, DWORD Characteristics)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    PVOID Buffer = NULL;
    IO_STATUS_BLOCK Iosb;
    PVOID SectionObject;
    ULONG ViewSize = 0;
    LARGE_INTEGER SectionSize;
    OBJECT_ATTRIBUTES ObjectAttributes;
    GDI_LOAD_FONT   LoadFont;
    INT FontCount;
    HANDLE KeyHandle;
    static const UNICODE_STRING TrueTypePostfix = RTL_CONSTANT_STRING(L" (TrueType)");

    /* Open the font file */
    InitializeObjectAttributes(&ObjectAttributes, FileName, 0, NULL, NULL);
    Status = ZwOpenFile(
                 &FileHandle,
                 FILE_GENERIC_READ | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,
                 FILE_SHARE_READ,
                 FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not load font file: %wZ\n", FileName);
        return 0;
    }

    SectionSize.QuadPart = 0LL;
    Status = MmCreateSection(&SectionObject, SECTION_ALL_ACCESS,
                             NULL, &SectionSize, PAGE_READONLY,
                             SEC_COMMIT, FileHandle, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not map file: %wZ\n", FileName);
        ZwClose(FileHandle);
        return 0;
    }
    ZwClose(FileHandle);

    Status = MmMapViewInSystemSpace(SectionObject, &Buffer, &ViewSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not map file: %wZ\n", FileName);
        ObDereferenceObject(SectionObject);
        return 0;
    }

    LoadFont.pFileName          = FileName;
    LoadFont.Memory             = SharedMem_Create(Buffer, ViewSize, TRUE);
    LoadFont.Characteristics    = Characteristics;
    RtlInitUnicodeString(&LoadFont.RegValueName, NULL);
    LoadFont.IsTrueType         = FALSE;
    LoadFont.PrivateEntry       = NULL;
    FontCount = IntGdiLoadFontsFromMemory(&LoadFont, NULL, -1, -1);

    ObDereferenceObject(SectionObject);

    /* Release our copy */
    IntLockFreeType;
    SharedMem_Release(LoadFont.Memory);
    IntUnLockFreeType;

    if (FontCount > 0)
    {
        if (LoadFont.IsTrueType)
        {
            /* append " (TrueType)" */
            UNICODE_STRING NewString;
            USHORT Length;

            Length = LoadFont.RegValueName.Length + TrueTypePostfix.Length;
            NewString.Length = 0;
            NewString.MaximumLength = Length + sizeof(WCHAR);
            NewString.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                     NewString.MaximumLength,
                                                     TAG_USTR);
            NewString.Buffer[0] = UNICODE_NULL;

            RtlAppendUnicodeStringToString(&NewString, &LoadFont.RegValueName);
            RtlAppendUnicodeStringToString(&NewString, &TrueTypePostfix);
            RtlFreeUnicodeString(&LoadFont.RegValueName);
            LoadFont.RegValueName = NewString;
        }

        /* registry */
        InitializeObjectAttributes(&ObjectAttributes, &FontRegPath,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL, NULL);
        Status = ZwOpenKey(&KeyHandle, KEY_WRITE, &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            ULONG DataSize;
            LPWSTR pFileName = wcsrchr(FileName->Buffer, L'\\');
            if (pFileName)
            {
                pFileName++;
                DataSize = (wcslen(pFileName) + 1) * sizeof(WCHAR);
                ZwSetValueKey(KeyHandle, &LoadFont.RegValueName, 0, REG_SZ,
                              pFileName, DataSize);
            }
            ZwClose(KeyHandle);
        }
    }
    RtlFreeUnicodeString(&LoadFont.RegValueName);

    return FontCount;
}

HANDLE FASTCALL
IntGdiAddFontMemResource(PVOID Buffer, DWORD dwSize, PDWORD pNumAdded)
{
    GDI_LOAD_FONT LoadFont;
    FONT_ENTRY_COLL_MEM* EntryCollection;
    INT FaceCount;
    HANDLE Ret = 0;

    PVOID BufferCopy = ExAllocatePoolWithTag(PagedPool, dwSize, TAG_FONT);

    if (!BufferCopy)
    {
        *pNumAdded = 0;
        return NULL;
    }
    memcpy(BufferCopy, Buffer, dwSize);

    LoadFont.pFileName = NULL;
    LoadFont.Memory = SharedMem_Create(BufferCopy, dwSize, FALSE);
    LoadFont.Characteristics = FR_PRIVATE | FR_NOT_ENUM;
    RtlInitUnicodeString(&LoadFont.RegValueName, NULL);
    LoadFont.IsTrueType = FALSE;
    LoadFont.PrivateEntry = NULL;
    FaceCount = IntGdiLoadFontsFromMemory(&LoadFont, NULL, -1, -1);

    RtlFreeUnicodeString(&LoadFont.RegValueName);

    /* Release our copy */
    IntLockFreeType;
    SharedMem_Release(LoadFont.Memory);
    IntUnLockFreeType;

    if (FaceCount > 0)
    {
        EntryCollection = ExAllocatePoolWithTag(PagedPool, sizeof(FONT_ENTRY_COLL_MEM), TAG_FONT);
        if (EntryCollection)
        {
            PPROCESSINFO Win32Process = PsGetCurrentProcessWin32Process();
            EntryCollection->Entry = LoadFont.PrivateEntry;
            IntLockProcessPrivateFonts(Win32Process);
            EntryCollection->Handle = ++Win32Process->PrivateMemFontHandleCount;
            InsertTailList(&Win32Process->PrivateMemFontListHead, &EntryCollection->ListEntry);
            IntUnLockProcessPrivateFonts(Win32Process);
            Ret = (HANDLE)EntryCollection->Handle;
        }
    }
    *pNumAdded = FaceCount;

    return Ret;
}

// FIXME: Add RemoveFontResource

static VOID FASTCALL
CleanupFontEntry(PFONT_ENTRY FontEntry)
{
    PFONTGDI FontGDI = FontEntry->Font;
    PSHARED_FACE SharedFace = FontGDI->SharedFace;

    if (FontGDI->Filename)
        ExFreePoolWithTag(FontGDI->Filename, GDITAG_PFF);

    EngFreeMem(FontGDI);
    SharedFace_Release(SharedFace);
    ExFreePoolWithTag(FontEntry, TAG_FONT);
}

VOID FASTCALL
IntGdiCleanupMemEntry(PFONT_ENTRY_MEM Head)
{
    PLIST_ENTRY Entry;
    PFONT_ENTRY_MEM FontEntry;

    while (!IsListEmpty(&Head->ListEntry))
    {
        Entry = RemoveHeadList(&Head->ListEntry);
        FontEntry = CONTAINING_RECORD(Entry, FONT_ENTRY_MEM, ListEntry);

        CleanupFontEntry(FontEntry->Entry);
        ExFreePoolWithTag(FontEntry, TAG_FONT);
    }

    CleanupFontEntry(Head->Entry);
    ExFreePoolWithTag(Head, TAG_FONT);
}

static VOID FASTCALL
UnlinkFontMemCollection(PFONT_ENTRY_COLL_MEM Collection)
{
    PFONT_ENTRY_MEM FontMemEntry = Collection->Entry;
    PLIST_ENTRY ListEntry;
    RemoveEntryList(&Collection->ListEntry);

    do {
        /* Also unlink the FONT_ENTRY stuff from the PrivateFontListHead */
        RemoveEntryList(&FontMemEntry->Entry->ListEntry);

        ListEntry = FontMemEntry->ListEntry.Flink;
        FontMemEntry = CONTAINING_RECORD(ListEntry, FONT_ENTRY_MEM, ListEntry);

    } while (FontMemEntry != Collection->Entry);
}

BOOL FASTCALL
IntGdiRemoveFontMemResource(HANDLE hMMFont)
{
    PLIST_ENTRY Entry;
    PFONT_ENTRY_COLL_MEM CurrentEntry;
    PFONT_ENTRY_COLL_MEM EntryCollection = NULL;
    PPROCESSINFO Win32Process = PsGetCurrentProcessWin32Process();

    IntLockProcessPrivateFonts(Win32Process);
    Entry = Win32Process->PrivateMemFontListHead.Flink;
    while (Entry != &Win32Process->PrivateMemFontListHead)
    {
        CurrentEntry = CONTAINING_RECORD(Entry, FONT_ENTRY_COLL_MEM, ListEntry);

        if (CurrentEntry->Handle == (UINT)hMMFont)
        {
            EntryCollection = CurrentEntry;
            UnlinkFontMemCollection(CurrentEntry);
            break;
        }

        Entry = Entry->Flink;
    }
    IntUnLockProcessPrivateFonts(Win32Process);

    if (EntryCollection)
    {
        IntGdiCleanupMemEntry(EntryCollection->Entry);
        ExFreePoolWithTag(EntryCollection, TAG_FONT);
        return TRUE;
    }
    return FALSE;
}


VOID FASTCALL
IntGdiCleanupPrivateFontsForProcess(VOID)
{
    PPROCESSINFO Win32Process = PsGetCurrentProcessWin32Process();
    PLIST_ENTRY Entry;
    PFONT_ENTRY_COLL_MEM EntryCollection;

    DPRINT("IntGdiCleanupPrivateFontsForProcess()\n");
    do {
        Entry = NULL;
        EntryCollection = NULL;

        IntLockProcessPrivateFonts(Win32Process);
        if (!IsListEmpty(&Win32Process->PrivateMemFontListHead))
        {
            Entry = Win32Process->PrivateMemFontListHead.Flink;
            EntryCollection = CONTAINING_RECORD(Entry, FONT_ENTRY_COLL_MEM, ListEntry);
            UnlinkFontMemCollection(EntryCollection);
        }
        IntUnLockProcessPrivateFonts(Win32Process);

        if (EntryCollection)
        {
            IntGdiCleanupMemEntry(EntryCollection->Entry);
            ExFreePoolWithTag(EntryCollection, TAG_FONT);
        }
        else
        {
            /* No Mem fonts anymore, see if we have any other private fonts left */
            Entry = NULL;
            IntLockProcessPrivateFonts(Win32Process);
            if (!IsListEmpty(&Win32Process->PrivateFontListHead))
            {
                Entry = RemoveHeadList(&Win32Process->PrivateFontListHead);
            }
            IntUnLockProcessPrivateFonts(Win32Process);

            if (Entry)
            {
                CleanupFontEntry(CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry));
            }
        }

    } while (Entry);
}

BOOL FASTCALL
IntIsFontRenderingEnabled(VOID)
{
    BOOL Ret = RenderingEnabled;
    HDC hDC;

    hDC = IntGetScreenDC();
    if (hDC)
        Ret = (NtGdiGetDeviceCaps(hDC, BITSPIXEL) > 8) && RenderingEnabled;

    return Ret;
}

VOID FASTCALL
IntEnableFontRendering(BOOL Enable)
{
    RenderingEnabled = Enable;
}

FT_Render_Mode FASTCALL
IntGetFontRenderMode(LOGFONTW *logfont)
{
    switch (logfont->lfQuality)
    {
    case ANTIALIASED_QUALITY:
    case NONANTIALIASED_QUALITY:
        return FT_RENDER_MODE_MONO;
    case DRAFT_QUALITY:
        return FT_RENDER_MODE_LIGHT;
        /*    case CLEARTYPE_QUALITY:
                return FT_RENDER_MODE_LCD; */
    }
    return FT_RENDER_MODE_NORMAL;
}


NTSTATUS FASTCALL
TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont)
{
    PLFONT plfont;
    LOGFONTW *plf;

    plfont = LFONT_AllocFontWithHandle();
    if (!plfont)
    {
        return STATUS_NO_MEMORY;
    }

    ExInitializePushLock(&plfont->lock);
    *NewFont = plfont->BaseObject.hHmgr;
    plf = &plfont->logfont.elfEnumLogfontEx.elfLogFont;
    RtlCopyMemory(plf, lf, sizeof(LOGFONTW));
    if (lf->lfEscapement != lf->lfOrientation)
    {
        /* This should really depend on whether GM_ADVANCED is set */
        plf->lfOrientation = plf->lfEscapement;
    }
    LFONT_UnlockFont(plfont);

    return STATUS_SUCCESS;
}

/*************************************************************************
 * TranslateCharsetInfo
 *
 * Fills a CHARSETINFO structure for a character set, code page, or
 * font. This allows making the correspondance between different labelings
 * (character set, Windows, ANSI, and OEM codepages, and Unicode ranges)
 * of the same encoding.
 *
 * Only one codepage will be set in Cs->fs. If TCI_SRCFONTSIG is used,
 * only one codepage should be set in *Src.
 *
 * RETURNS
 *   TRUE on success, FALSE on failure.
 *
 */
static BOOLEAN APIENTRY
IntTranslateCharsetInfo(PDWORD Src, /* [in]
                         if flags == TCI_SRCFONTSIG: pointer to fsCsb of a FONTSIGNATURE
                         if flags == TCI_SRCCHARSET: a character set value
                         if flags == TCI_SRCCODEPAGE: a code page value */
                        LPCHARSETINFO Cs, /* [out] structure to receive charset information */
                        DWORD Flags /* [in] determines interpretation of lpSrc */)
{
    int Index = 0;

    switch (Flags)
    {
    case TCI_SRCFONTSIG:
        while (Index < MAXTCIINDEX && 0 == (*Src >> Index & 0x0001))
        {
            Index++;
        }
        break;
    case TCI_SRCCODEPAGE:
        while (Index < MAXTCIINDEX && *Src != FontTci[Index].ciACP)
        {
            Index++;
        }
        break;
    case TCI_SRCCHARSET:
        while (Index < MAXTCIINDEX && *Src != FontTci[Index].ciCharset)
        {
            Index++;
        }
        break;
    default:
        return FALSE;
    }

    if (Index >= MAXTCIINDEX || DEFAULT_CHARSET == FontTci[Index].ciCharset)
    {
        return FALSE;
    }

    RtlCopyMemory(Cs, &FontTci[Index], sizeof(CHARSETINFO));

    return TRUE;
}


static BOOL face_has_symbol_charmap(FT_Face ft_face)
{
    int i;

    for(i = 0; i < ft_face->num_charmaps; i++)
    {
        if(ft_face->charmaps[i]->encoding == FT_ENCODING_MS_SYMBOL)
            return TRUE;
    }
    return FALSE;
}


static void FASTCALL
FillTMEx(TEXTMETRICW *TM, PFONTGDI FontGDI,
         TT_OS2 *pOS2, TT_HoriHeader *pHori,
         FT_WinFNT_HeaderRec *pFNT, BOOL RealFont)
{
    FT_Fixed XScale, YScale;
    int Ascent, Descent;
    FT_Face Face = FontGDI->SharedFace->Face;

    XScale = Face->size->metrics.x_scale;
    YScale = Face->size->metrics.y_scale;

    if (pFNT)
    {
        TM->tmHeight           = pFNT->pixel_height;
        TM->tmAscent           = pFNT->ascent;
        TM->tmDescent          = TM->tmHeight - TM->tmAscent;
        TM->tmInternalLeading  = pFNT->internal_leading;
        TM->tmExternalLeading  = pFNT->external_leading;
        TM->tmAveCharWidth     = pFNT->avg_width;
        TM->tmMaxCharWidth     = pFNT->max_width;
        TM->tmOverhang         = 0;
        TM->tmDigitizedAspectX = pFNT->horizontal_resolution;
        TM->tmDigitizedAspectY = pFNT->vertical_resolution;
        TM->tmFirstChar        = pFNT->first_char;
        TM->tmLastChar         = pFNT->last_char;
        TM->tmDefaultChar      = pFNT->default_char + pFNT->first_char;
        TM->tmBreakChar        = pFNT->break_char + pFNT->first_char;
        TM->tmPitchAndFamily   = pFNT->pitch_and_family;
        if (RealFont)
        {
            TM->tmWeight       = FontGDI->OriginalWeight;
            TM->tmItalic       = FontGDI->OriginalItalic;
            TM->tmUnderlined   = pFNT->underline;
            TM->tmStruckOut    = pFNT->strike_out;
            TM->tmCharSet      = pFNT->charset;
        }
        else
        {
            TM->tmWeight       = FontGDI->RequestWeight;
            TM->tmItalic       = FontGDI->RequestItalic;
            TM->tmUnderlined   = FontGDI->RequestUnderline;
            TM->tmStruckOut    = FontGDI->RequestStrikeOut;
            TM->tmCharSet      = FontGDI->CharSet;
        }
        return;
    }

    if (pOS2->usWinAscent + pOS2->usWinDescent == 0)
    {
        Ascent = pHori->Ascender;
        Descent = -pHori->Descender;
    }
    else
    {
        Ascent = pOS2->usWinAscent;
        Descent = pOS2->usWinDescent;
    }

#if 0 /* This (Wine) code doesn't seem to work correctly for us, cmd issue */
    TM->tmAscent = (FT_MulFix(Ascent, YScale) + 32) >> 6;
    TM->tmDescent = (FT_MulFix(Descent, YScale) + 32) >> 6;
#else /* This (ros) code was previously affected by a FreeType bug, but it works now */
    TM->tmAscent = (Face->size->metrics.ascender + 32) >> 6; /* Units above baseline */
    TM->tmDescent = (32 - Face->size->metrics.descender) >> 6; /* Units below baseline */
#endif
    TM->tmInternalLeading = (FT_MulFix(Ascent + Descent - Face->units_per_EM, YScale) + 32) >> 6;

    TM->tmHeight = TM->tmAscent + TM->tmDescent;

    /* MSDN says:
     *  el = MAX(0, LineGap - ((WinAscent + WinDescent) - (Ascender - Descender)))
     */
    TM->tmExternalLeading = max(0, (FT_MulFix(pHori->Line_Gap
                                    - ((Ascent + Descent)
                                       - (pHori->Ascender - pHori->Descender)),
                                    YScale) + 32) >> 6);

    TM->tmAveCharWidth = (FT_MulFix(pOS2->xAvgCharWidth, XScale) + 32) >> 6;
    if (TM->tmAveCharWidth == 0)
    {
        TM->tmAveCharWidth = 1;
    }

    /* Correct forumla to get the maxcharwidth from unicode and ansi font */
    TM->tmMaxCharWidth = (FT_MulFix(Face->max_advance_width, XScale) + 32) >> 6;

    if (RealFont)
    {
        TM->tmWeight = FontGDI->OriginalWeight;
    }
    else
    {
        if (FontGDI->OriginalWeight != FW_DONTCARE &&
            FontGDI->OriginalWeight != FW_NORMAL)
        {
            TM->tmWeight = FontGDI->OriginalWeight;
        }
        else
        {
            TM->tmWeight = FontGDI->RequestWeight;
        }
    }

    TM->tmOverhang = 0;
    TM->tmDigitizedAspectX = 96;
    TM->tmDigitizedAspectY = 96;
    if (face_has_symbol_charmap(Face) ||
        (pOS2->usFirstCharIndex >= 0xf000 && pOS2->usFirstCharIndex < 0xf100))
    {
        USHORT cpOEM, cpAnsi;

        EngGetCurrentCodePage(&cpOEM, &cpAnsi);
        TM->tmFirstChar = 0;
        switch(cpAnsi)
        {
        case 1257: /* Baltic */
            TM->tmLastChar = 0xf8fd;
            break;
        default:
            TM->tmLastChar = 0xf0ff;
        }
        TM->tmBreakChar = 0x20;
        TM->tmDefaultChar = 0x1f;
    }
    else
    {
        TM->tmFirstChar = pOS2->usFirstCharIndex; /* Should be the first char in the cmap */
        TM->tmLastChar = pOS2->usLastCharIndex;   /* Should be min(cmap_last, os2_last) */

        if(pOS2->usFirstCharIndex <= 1)
            TM->tmBreakChar = pOS2->usFirstCharIndex + 2;
        else if (pOS2->usFirstCharIndex > 0xff)
            TM->tmBreakChar = 0x20;
        else
            TM->tmBreakChar = pOS2->usFirstCharIndex;
        TM->tmDefaultChar = TM->tmBreakChar - 1;
    }

    if (RealFont)
    {
        TM->tmItalic = FontGDI->OriginalItalic;
        TM->tmUnderlined = FALSE;
        TM->tmStruckOut  = FALSE;
    }
    else
    {
        if (FontGDI->OriginalItalic || FontGDI->RequestItalic)
        {
            TM->tmItalic = 0xFF;
        }
        else
        {
            TM->tmItalic = 0;
        }
        TM->tmUnderlined = (FontGDI->RequestUnderline ? 0xFF : 0);
        TM->tmStruckOut  = (FontGDI->RequestStrikeOut ? 0xFF : 0);
    }

    if (!FT_IS_FIXED_WIDTH(Face))
    {
        switch (pOS2->panose[PAN_PROPORTION_INDEX])
        {
            case PAN_PROP_MONOSPACED:
                TM->tmPitchAndFamily = 0;
                break;
            default:
                TM->tmPitchAndFamily = _TMPF_VARIABLE_PITCH;
                break;
        }
    }
    else
    {
        TM->tmPitchAndFamily = 0;
    }

    switch (pOS2->panose[PAN_FAMILYTYPE_INDEX])
    {
    case PAN_FAMILY_SCRIPT:
        TM->tmPitchAndFamily |= FF_SCRIPT;
        break;
    case PAN_FAMILY_DECORATIVE:
        TM->tmPitchAndFamily |= FF_DECORATIVE;
        break;

    case PAN_ANY:
    case PAN_NO_FIT:
    case PAN_FAMILY_TEXT_DISPLAY:
    case PAN_FAMILY_PICTORIAL: /* Symbol fonts get treated as if they were text */
                               /* Which is clearly not what the panose spec says. */
        if (TM->tmPitchAndFamily == 0) /* Fixed */
        {
            TM->tmPitchAndFamily = FF_MODERN;
        }
        else
        {
            switch (pOS2->panose[PAN_SERIFSTYLE_INDEX])
            {
            case PAN_ANY:
            case PAN_NO_FIT:
            default:
                TM->tmPitchAndFamily |= FF_DONTCARE;
                break;

            case PAN_SERIF_COVE:
            case PAN_SERIF_OBTUSE_COVE:
            case PAN_SERIF_SQUARE_COVE:
            case PAN_SERIF_OBTUSE_SQUARE_COVE:
            case PAN_SERIF_SQUARE:
            case PAN_SERIF_THIN:
            case PAN_SERIF_BONE:
            case PAN_SERIF_EXAGGERATED:
            case PAN_SERIF_TRIANGLE:
                TM->tmPitchAndFamily |= FF_ROMAN;
                break;

            case PAN_SERIF_NORMAL_SANS:
            case PAN_SERIF_OBTUSE_SANS:
            case PAN_SERIF_PERP_SANS:
            case PAN_SERIF_FLARED:
            case PAN_SERIF_ROUNDED:
                TM->tmPitchAndFamily |= FF_SWISS;
                break;
            }
        }
        break;
    default:
        TM->tmPitchAndFamily |= FF_DONTCARE;
    }

    if (FT_IS_SCALABLE(Face))
    {
        TM->tmPitchAndFamily |= TMPF_VECTOR;
    }
    if (FT_IS_SFNT(Face))
    {
        TM->tmPitchAndFamily |= TMPF_TRUETYPE;
    }

    TM->tmCharSet = FontGDI->CharSet;
}

static void FASTCALL
FillTM(TEXTMETRICW *TM, PFONTGDI FontGDI,
       TT_OS2 *pOS2, TT_HoriHeader *pHori,
       FT_WinFNT_HeaderRec *pFNT)
{
    FillTMEx(TM, FontGDI, pOS2, pHori, pFNT, FALSE);
}

/*************************************************************
 * IntGetOutlineTextMetrics
 *
 */
INT FASTCALL
IntGetOutlineTextMetrics(PFONTGDI FontGDI,
                         UINT Size,
                         OUTLINETEXTMETRICW *Otm)
{
    unsigned Needed;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    TT_Postscript *pPost;
    FT_Fixed XScale, YScale;
    ANSI_STRING FamilyNameA, StyleNameA;
    UNICODE_STRING FamilyNameW, StyleNameW, Regular;
    FT_WinFNT_HeaderRec Win;
    FT_Error Error;
    char *Cp;
    NTSTATUS status;
    FT_Face Face = FontGDI->SharedFace->Face;

    Needed = sizeof(OUTLINETEXTMETRICW);

    RtlInitAnsiString(&FamilyNameA, Face->family_name);
    status = RtlAnsiStringToUnicodeString(&FamilyNameW, &FamilyNameA, TRUE);
    if (!NT_SUCCESS(status))
    {
        return 0;
    }

    RtlInitAnsiString(&StyleNameA, Face->style_name);
    status = RtlAnsiStringToUnicodeString(&StyleNameW, &StyleNameA, TRUE);
    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&FamilyNameW);
        return 0;
    }

    /* These names should be read from the TT name table */

    /* Length of otmpFamilyName */
    Needed += FamilyNameW.Length + sizeof(WCHAR);

    RtlInitUnicodeString(&Regular, L"Regular");
    /* Length of otmpFaceName */
    if (RtlEqualUnicodeString(&StyleNameW, &Regular, TRUE))
    {
        Needed += FamilyNameW.Length + sizeof(WCHAR); /* Just the family name */
    }
    else
    {
        Needed += FamilyNameW.Length + StyleNameW.Length + (sizeof(WCHAR) << 1); /* family + " " + style */
    }

    /* Length of otmpStyleName */
    Needed += StyleNameW.Length + sizeof(WCHAR);

    /* Length of otmpFullName */
    Needed += FamilyNameW.Length + StyleNameW.Length + (sizeof(WCHAR) << 1);

    if (Size < Needed)
    {
        RtlFreeUnicodeString(&FamilyNameW);
        RtlFreeUnicodeString(&StyleNameW);
        return Needed;
    }

    XScale = Face->size->metrics.x_scale;
    YScale = Face->size->metrics.y_scale;

    IntLockFreeType;
    pOS2 = FT_Get_Sfnt_Table(Face, ft_sfnt_os2);
    if (NULL == pOS2)
    {
        IntUnLockFreeType;
        DPRINT1("Can't find OS/2 table - not TT font?\n");
        RtlFreeUnicodeString(&StyleNameW);
        RtlFreeUnicodeString(&FamilyNameW);
        return 0;
    }

    pHori = FT_Get_Sfnt_Table(Face, ft_sfnt_hhea);
    if (NULL == pHori)
    {
        IntUnLockFreeType;
        DPRINT1("Can't find HHEA table - not TT font?\n");
        RtlFreeUnicodeString(&StyleNameW);
        RtlFreeUnicodeString(&FamilyNameW);
        return 0;
    }

    pPost = FT_Get_Sfnt_Table(Face, ft_sfnt_post); /* We can live with this failing */

    Error = FT_Get_WinFNT_Header(Face , &Win);

    Otm->otmSize = Needed;

    FillTM(&Otm->otmTextMetrics, FontGDI, pOS2, pHori, !Error ? &Win : 0);

    Otm->otmFiller = 0;
    RtlCopyMemory(&Otm->otmPanoseNumber, pOS2->panose, PANOSE_COUNT);
    Otm->otmfsSelection = pOS2->fsSelection;
    Otm->otmfsType = pOS2->fsType;
    Otm->otmsCharSlopeRise = pHori->caret_Slope_Rise;
    Otm->otmsCharSlopeRun = pHori->caret_Slope_Run;
    Otm->otmItalicAngle = 0; /* POST table */
    Otm->otmEMSquare = Face->units_per_EM;
    Otm->otmAscent = (FT_MulFix(pOS2->sTypoAscender, YScale) + 32) >> 6;
    Otm->otmDescent = (FT_MulFix(pOS2->sTypoDescender, YScale) + 32) >> 6;
    Otm->otmLineGap = (FT_MulFix(pOS2->sTypoLineGap, YScale) + 32) >> 6;
    Otm->otmsCapEmHeight = (FT_MulFix(pOS2->sCapHeight, YScale) + 32) >> 6;
    Otm->otmsXHeight = (FT_MulFix(pOS2->sxHeight, YScale) + 32) >> 6;
    Otm->otmrcFontBox.left = (FT_MulFix(Face->bbox.xMin, XScale) + 32) >> 6;
    Otm->otmrcFontBox.right = (FT_MulFix(Face->bbox.xMax, XScale) + 32) >> 6;
    Otm->otmrcFontBox.top = (FT_MulFix(Face->bbox.yMax, YScale) + 32) >> 6;
    Otm->otmrcFontBox.bottom = (FT_MulFix(Face->bbox.yMin, YScale) + 32) >> 6;
    Otm->otmMacAscent = Otm->otmTextMetrics.tmAscent;
    Otm->otmMacDescent = -Otm->otmTextMetrics.tmDescent;
    Otm->otmMacLineGap = Otm->otmLineGap;
    Otm->otmusMinimumPPEM = 0; /* TT Header */
    Otm->otmptSubscriptSize.x = (FT_MulFix(pOS2->ySubscriptXSize, XScale) + 32) >> 6;
    Otm->otmptSubscriptSize.y = (FT_MulFix(pOS2->ySubscriptYSize, YScale) + 32) >> 6;
    Otm->otmptSubscriptOffset.x = (FT_MulFix(pOS2->ySubscriptXOffset, XScale) + 32) >> 6;
    Otm->otmptSubscriptOffset.y = (FT_MulFix(pOS2->ySubscriptYOffset, YScale) + 32) >> 6;
    Otm->otmptSuperscriptSize.x = (FT_MulFix(pOS2->ySuperscriptXSize, XScale) + 32) >> 6;
    Otm->otmptSuperscriptSize.y = (FT_MulFix(pOS2->ySuperscriptYSize, YScale) + 32) >> 6;
    Otm->otmptSuperscriptOffset.x = (FT_MulFix(pOS2->ySuperscriptXOffset, XScale) + 32) >> 6;
    Otm->otmptSuperscriptOffset.y = (FT_MulFix(pOS2->ySuperscriptYOffset, YScale) + 32) >> 6;
    Otm->otmsStrikeoutSize = (FT_MulFix(pOS2->yStrikeoutSize, YScale) + 32) >> 6;
    Otm->otmsStrikeoutPosition = (FT_MulFix(pOS2->yStrikeoutPosition, YScale) + 32) >> 6;
    if (!pPost)
    {
        Otm->otmsUnderscoreSize = 0;
        Otm->otmsUnderscorePosition = 0;
    }
    else
    {
        Otm->otmsUnderscoreSize = (FT_MulFix(pPost->underlineThickness, YScale) + 32) >> 6;
        Otm->otmsUnderscorePosition = (FT_MulFix(pPost->underlinePosition, YScale) + 32) >> 6;
    }

    IntUnLockFreeType;

    /* otmp* members should clearly have type ptrdiff_t, but M$ knows best */
    Cp = (char*) Otm + sizeof(OUTLINETEXTMETRICW);
    Otm->otmpFamilyName = (LPSTR)(Cp - (char*) Otm);
    wcscpy((WCHAR*) Cp, FamilyNameW.Buffer);
    Cp += FamilyNameW.Length + sizeof(WCHAR);
    Otm->otmpStyleName = (LPSTR)(Cp - (char*) Otm);
    wcscpy((WCHAR*) Cp, StyleNameW.Buffer);
    Cp += StyleNameW.Length + sizeof(WCHAR);
    Otm->otmpFaceName = (LPSTR)(Cp - (char*) Otm);
    wcscpy((WCHAR*) Cp, FamilyNameW.Buffer);
    if (!RtlEqualUnicodeString(&StyleNameW, &Regular, TRUE))
    {
        wcscat((WCHAR*) Cp, L" ");
        wcscat((WCHAR*) Cp, StyleNameW.Buffer);
        Cp += FamilyNameW.Length + StyleNameW.Length + (sizeof(WCHAR) << 1);
    }
    else
    {
        Cp += FamilyNameW.Length + sizeof(WCHAR);
    }
    Otm->otmpFullName = (LPSTR)(Cp - (char*) Otm);
    wcscpy((WCHAR*) Cp, FamilyNameW.Buffer);
    wcscat((WCHAR*) Cp, L" ");
    wcscat((WCHAR*) Cp, StyleNameW.Buffer);

    RtlFreeUnicodeString(&StyleNameW);
    RtlFreeUnicodeString(&FamilyNameW);

    return Needed;
}

static PFONTGDI FASTCALL
FindFaceNameInList(PUNICODE_STRING FaceName, PLIST_ENTRY Head)
{
    PLIST_ENTRY Entry;
    PFONT_ENTRY CurrentEntry;
    ANSI_STRING EntryFaceNameA;
    UNICODE_STRING EntryFaceNameW;
    FONTGDI *FontGDI;
    NTSTATUS status;

    Entry = Head->Flink;
    while (Entry != Head)
    {
        CurrentEntry = CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);

        FontGDI = CurrentEntry->Font;
        ASSERT(FontGDI);

        RtlInitAnsiString(&EntryFaceNameA, FontGDI->SharedFace->Face->family_name);
        status = RtlAnsiStringToUnicodeString(&EntryFaceNameW, &EntryFaceNameA, TRUE);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        if ((LF_FACESIZE - 1) * sizeof(WCHAR) < EntryFaceNameW.Length)
        {
            EntryFaceNameW.Length = (LF_FACESIZE - 1) * sizeof(WCHAR);
            EntryFaceNameW.Buffer[LF_FACESIZE - 1] = L'\0';
        }

        if (RtlEqualUnicodeString(FaceName, &EntryFaceNameW, TRUE))
        {
            RtlFreeUnicodeString(&EntryFaceNameW);
            return FontGDI;
        }

        RtlFreeUnicodeString(&EntryFaceNameW);
        Entry = Entry->Flink;
    }

    return NULL;
}

static PFONTGDI FASTCALL
FindFaceNameInLists(PUNICODE_STRING FaceName)
{
    PPROCESSINFO Win32Process;
    PFONTGDI Font;

    /* Search the process local list.
       We do not have to search the 'Mem' list, since those fonts are linked in the PrivateFontListHead */
    Win32Process = PsGetCurrentProcessWin32Process();
    IntLockProcessPrivateFonts(Win32Process);
    Font = FindFaceNameInList(FaceName, &Win32Process->PrivateFontListHead);
    IntUnLockProcessPrivateFonts(Win32Process);
    if (NULL != Font)
    {
        return Font;
    }

    /* Search the global list */
    IntLockGlobalFonts;
    Font = FindFaceNameInList(FaceName, &FontListHead);
    IntUnLockGlobalFonts;

    return Font;
}

/* See https://msdn.microsoft.com/en-us/library/bb165625(v=vs.90).aspx */
static BYTE
CharSetFromLangID(LANGID LangID)
{
    /* FIXME: Add more and fix if wrong */
    switch (PRIMARYLANGID(LangID))
    {
        case LANG_CHINESE:
            switch (SUBLANGID(LangID))
            {
                case SUBLANG_CHINESE_TRADITIONAL:
                    return CHINESEBIG5_CHARSET;
                case SUBLANG_CHINESE_SIMPLIFIED:
                default:
                    break;
            }
            return GB2312_CHARSET;

        case LANG_CZECH: case LANG_HUNGARIAN: case LANG_POLISH:
        case LANG_SLOVAK: case LANG_SLOVENIAN: case LANG_ROMANIAN:
            return EASTEUROPE_CHARSET;

        case LANG_RUSSIAN: case LANG_BULGARIAN: case LANG_MACEDONIAN:
        case LANG_SERBIAN: case LANG_UKRAINIAN:
            return RUSSIAN_CHARSET;

        case LANG_ARABIC:       return ARABIC_CHARSET;
        case LANG_GREEK:        return GREEK_CHARSET;
        case LANG_HEBREW:       return HEBREW_CHARSET;
        case LANG_JAPANESE:     return SHIFTJIS_CHARSET;
        case LANG_KOREAN:       return JOHAB_CHARSET;
        case LANG_TURKISH:      return TURKISH_CHARSET;
        case LANG_THAI:         return THAI_CHARSET;
        case LANG_LATVIAN:      return BALTIC_CHARSET;
        case LANG_VIETNAMESE:   return VIETNAMESE_CHARSET;

        case LANG_ENGLISH: case LANG_BASQUE: case LANG_CATALAN:
        case LANG_DANISH: case LANG_DUTCH: case LANG_FINNISH:
        case LANG_FRENCH: case LANG_GERMAN: case LANG_ITALIAN:
        case LANG_NORWEGIAN: case LANG_PORTUGUESE: case LANG_SPANISH:
        case LANG_SWEDISH: default:
            return ANSI_CHARSET;
    }
}

static void
SwapEndian(LPVOID pvData, DWORD Size)
{
    BYTE b, *pb = pvData;
    Size /= 2;
    while (Size-- > 0)
    {
        b = pb[0];
        pb[0] = pb[1];
        pb[1] = b;
        ++pb; ++pb;
    }
}

static NTSTATUS
IntGetFontLocalizedName(PUNICODE_STRING pNameW, FT_Face Face,
                        FT_UShort NameID, FT_UShort LangID)
{
    FT_SfntName Name;
    INT i, Count;
    WCHAR Buf[LF_FULLFACESIZE];
    FT_Error Error;
    NTSTATUS Status = STATUS_NOT_FOUND;
    ANSI_STRING AnsiName;

    RtlFreeUnicodeString(pNameW);

    Count = FT_Get_Sfnt_Name_Count(Face);
    for (i = 0; i < Count; ++i)
    {
        Error = FT_Get_Sfnt_Name(Face, i, &Name);
        if (Error)
            continue;

        if (Name.platform_id != TT_PLATFORM_MICROSOFT ||
            Name.encoding_id != TT_MS_ID_UNICODE_CS)
        {
            continue;   /* not Microsoft Unicode name */
        }

        if (Name.name_id != NameID || Name.language_id != LangID)
        {
            continue;   /* mismatched */
        }

        if (Name.string == NULL || Name.string_len == 0 ||
            (Name.string[0] == 0 && Name.string[1] == 0))
        {
            continue;   /* invalid string */
        }

        if (sizeof(Buf) < Name.string_len + sizeof(UNICODE_NULL))
        {
            continue;   /* name too long */
        }

        /* NOTE: Name.string is not null-terminated */
        RtlCopyMemory(Buf, Name.string, Name.string_len);
        Buf[Name.string_len / sizeof(WCHAR)] = UNICODE_NULL;

        /* Convert UTF-16 big endian to little endian */
        SwapEndian(Buf, Name.string_len);

        RtlCreateUnicodeString(pNameW, Buf);
        Status = STATUS_SUCCESS;
        break;
    }

    if (Status == STATUS_NOT_FOUND)
    {
        if (LangID != gusEnglishUS)
        {
            Status = IntGetFontLocalizedName(pNameW, Face, NameID, gusEnglishUS);
        }
    }
    if (Status == STATUS_NOT_FOUND)
    {
        RtlInitAnsiString(&AnsiName, Face->family_name);
        Status = RtlAnsiStringToUnicodeString(pNameW, &AnsiName, TRUE);
    }

    return Status;
}

static void FASTCALL
FontFamilyFillInfo(PFONTFAMILYINFO Info, LPCWSTR FaceName,
                   LPCWSTR FullName, PFONTGDI FontGDI)
{
    ANSI_STRING StyleA;
    UNICODE_STRING StyleW;
    TT_OS2 *pOS2;
    FONTSIGNATURE fs;
    CHARSETINFO CharSetInfo;
    unsigned i, Size;
    OUTLINETEXTMETRICW *Otm;
    LOGFONTW *Lf;
    TEXTMETRICW *TM;
    NEWTEXTMETRICW *Ntm;
    DWORD fs0;
    NTSTATUS status;
    FT_Face Face = FontGDI->SharedFace->Face;
    UNICODE_STRING NameW;

    RtlInitUnicodeString(&NameW, NULL);
    RtlZeroMemory(Info, sizeof(FONTFAMILYINFO));
    Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
    Otm = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
    if (!Otm)
    {
        return;
    }
    IntGetOutlineTextMetrics(FontGDI, Size, Otm);

    Lf = &Info->EnumLogFontEx.elfLogFont;
    TM = &Otm->otmTextMetrics;

    Lf->lfHeight = TM->tmHeight;
    Lf->lfWidth = TM->tmAveCharWidth;
    Lf->lfWeight = TM->tmWeight;
    Lf->lfItalic = TM->tmItalic;
    Lf->lfPitchAndFamily = (TM->tmPitchAndFamily & 0xf1) + 1;
    Lf->lfCharSet = TM->tmCharSet;
    Lf->lfOutPrecision = OUT_OUTLINE_PRECIS;
    Lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    Lf->lfQuality = PROOF_QUALITY;

    Ntm = &Info->NewTextMetricEx.ntmTm;
    Ntm->tmHeight = TM->tmHeight;
    Ntm->tmAscent = TM->tmAscent;
    Ntm->tmDescent = TM->tmDescent;
    Ntm->tmInternalLeading = TM->tmInternalLeading;
    Ntm->tmExternalLeading = TM->tmExternalLeading;
    Ntm->tmAveCharWidth = TM->tmAveCharWidth;
    Ntm->tmMaxCharWidth = TM->tmMaxCharWidth;
    Ntm->tmWeight = TM->tmWeight;
    Ntm->tmOverhang = TM->tmOverhang;
    Ntm->tmDigitizedAspectX = TM->tmDigitizedAspectX;
    Ntm->tmDigitizedAspectY = TM->tmDigitizedAspectY;
    Ntm->tmFirstChar = TM->tmFirstChar;
    Ntm->tmLastChar = TM->tmLastChar;
    Ntm->tmDefaultChar = TM->tmDefaultChar;
    Ntm->tmBreakChar = TM->tmBreakChar;
    Ntm->tmItalic = TM->tmItalic;
    Ntm->tmUnderlined = TM->tmUnderlined;
    Ntm->tmStruckOut = TM->tmStruckOut;
    Ntm->tmPitchAndFamily = TM->tmPitchAndFamily;
    Ntm->tmCharSet = TM->tmCharSet;
    Ntm->ntmFlags = TM->tmItalic ? NTM_ITALIC : 0;

    if (550 < TM->tmWeight) Ntm->ntmFlags |= NTM_BOLD;

    if (0 == Ntm->ntmFlags) Ntm->ntmFlags = NTM_REGULAR;

    Ntm->ntmSizeEM = Otm->otmEMSquare;
    Ntm->ntmCellHeight = Otm->otmEMSquare;
    Ntm->ntmAvgWidth = 0;

    Info->FontType = (0 != (TM->tmPitchAndFamily & TMPF_TRUETYPE)
                      ? TRUETYPE_FONTTYPE : 0);

    if (0 == (TM->tmPitchAndFamily & TMPF_VECTOR))
        Info->FontType |= RASTER_FONTTYPE;

    ExFreePoolWithTag(Otm, GDITAG_TEXT);

    /* face name */
    if (FaceName)
    {
        RtlStringCbCopyW(Lf->lfFaceName, sizeof(Lf->lfFaceName), FaceName);
    }
    else
    {
        status = IntGetFontLocalizedName(&NameW, Face, TT_NAME_ID_FONT_FAMILY,
                                         gusLanguageID);
        if (NT_SUCCESS(status))
        {
            /* store it */
            RtlStringCbCopyW(Lf->lfFaceName, sizeof(Lf->lfFaceName),
                             NameW.Buffer);
            RtlFreeUnicodeString(&NameW);
        }
    }

    /* full name */
    if (FullName)
    {
        RtlStringCbCopyW(Info->EnumLogFontEx.elfFullName,
                         sizeof(Info->EnumLogFontEx.elfFullName),
                         FullName);
    }
    else
    {
        status = IntGetFontLocalizedName(&NameW, Face, TT_NAME_ID_FULL_NAME,
                                         gusLanguageID);
        if (NT_SUCCESS(status))
        {
            /* store it */
            RtlStringCbCopyW(Info->EnumLogFontEx.elfFullName,
                             sizeof(Info->EnumLogFontEx.elfFullName),
                             NameW.Buffer);
            RtlFreeUnicodeString(&NameW);
        }
    }

    RtlInitAnsiString(&StyleA, Face->style_name);
    StyleW.Buffer = Info->EnumLogFontEx.elfStyle;
    StyleW.MaximumLength = sizeof(Info->EnumLogFontEx.elfStyle);
    status = RtlAnsiStringToUnicodeString(&StyleW, &StyleA, FALSE);
    if (!NT_SUCCESS(status))
    {
        return;
    }
    Info->EnumLogFontEx.elfScript[0] = UNICODE_NULL;

    IntLockFreeType;
    pOS2 = FT_Get_Sfnt_Table(Face, ft_sfnt_os2);

    if (!pOS2)
    {
        IntUnLockFreeType;
        return;
    }

    fs.fsCsb[0] = pOS2->ulCodePageRange1;
    fs.fsCsb[1] = pOS2->ulCodePageRange2;
    fs.fsUsb[0] = pOS2->ulUnicodeRange1;
    fs.fsUsb[1] = pOS2->ulUnicodeRange2;
    fs.fsUsb[2] = pOS2->ulUnicodeRange3;
    fs.fsUsb[3] = pOS2->ulUnicodeRange4;

    if (0 == pOS2->version)
    {
        FT_UInt Dummy;

        if (FT_Get_First_Char(Face, &Dummy) < 0x100)
            fs.fsCsb[0] |= FS_LATIN1;
        else
            fs.fsCsb[0] |= FS_SYMBOL;
    }
    IntUnLockFreeType;

    if (fs.fsCsb[0] == 0)
    {
        /* Let's see if we can find any interesting cmaps */
        for (i = 0; i < (UINT)Face->num_charmaps; i++)
        {
            switch (Face->charmaps[i]->encoding)
            {
            case FT_ENCODING_UNICODE:
            case FT_ENCODING_APPLE_ROMAN:
                fs.fsCsb[0] |= FS_LATIN1;
                break;
            case FT_ENCODING_MS_SYMBOL:
                fs.fsCsb[0] |= FS_SYMBOL;
                break;
            default:
                break;
            }
        }
    }

    for (i = 0; i < MAXTCIINDEX; i++)
    {
        fs0 = 1L << i;
        if (fs.fsCsb[0] & fs0)
        {
            if (!IntTranslateCharsetInfo(&fs0, &CharSetInfo, TCI_SRCFONTSIG))
            {
                CharSetInfo.ciCharset = DEFAULT_CHARSET;
            }
            if (DEFAULT_CHARSET != CharSetInfo.ciCharset)
            {
                if (ElfScripts[i])
                    wcscpy(Info->EnumLogFontEx.elfScript, ElfScripts[i]);
                else
                {
                    DPRINT1("Unknown elfscript for bit %u\n", i);
                }
            }
        }
    }
    Info->NewTextMetricEx.ntmFontSig = fs;
}

static int FASTCALL
FindFaceNameInInfo(PUNICODE_STRING FaceName, PFONTFAMILYINFO Info, DWORD InfoEntries)
{
    DWORD i;
    UNICODE_STRING InfoFaceName;

    for (i = 0; i < InfoEntries; i++)
    {
        RtlInitUnicodeString(&InfoFaceName, Info[i].EnumLogFontEx.elfLogFont.lfFaceName);
        if (RtlEqualUnicodeString(&InfoFaceName, FaceName, TRUE))
        {
            return i;
        }
    }

    return -1;
}

static BOOLEAN FASTCALL
FontFamilyInclude(LPLOGFONTW LogFont, PUNICODE_STRING FaceName,
                  PFONTFAMILYINFO Info, DWORD InfoEntries)
{
    UNICODE_STRING LogFontFaceName;

    RtlInitUnicodeString(&LogFontFaceName, LogFont->lfFaceName);
    if (0 != LogFontFaceName.Length &&
        !RtlEqualUnicodeString(&LogFontFaceName, FaceName, TRUE))
    {
        return FALSE;
    }

    return FindFaceNameInInfo(FaceName, Info, InfoEntries) < 0;
}

static BOOLEAN FASTCALL
GetFontFamilyInfoForList(LPLOGFONTW LogFont,
                         PFONTFAMILYINFO Info,
                         DWORD *Count,
                         DWORD Size,
                         PLIST_ENTRY Head)
{
    PLIST_ENTRY Entry;
    PFONT_ENTRY CurrentEntry;
    ANSI_STRING EntryFaceNameA;
    UNICODE_STRING EntryFaceNameW;
    FONTGDI *FontGDI;
    NTSTATUS status;

    Entry = Head->Flink;
    while (Entry != Head)
    {
        CurrentEntry = (PFONT_ENTRY) CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);

        FontGDI = CurrentEntry->Font;
        ASSERT(FontGDI);

        RtlInitAnsiString(&EntryFaceNameA, FontGDI->SharedFace->Face->family_name);
        status = RtlAnsiStringToUnicodeString(&EntryFaceNameW, &EntryFaceNameA, TRUE);
        if (!NT_SUCCESS(status))
        {
            return FALSE;
        }

        if ((LF_FACESIZE - 1) * sizeof(WCHAR) < EntryFaceNameW.Length)
        {
            EntryFaceNameW.Length = (LF_FACESIZE - 1) * sizeof(WCHAR);
            EntryFaceNameW.Buffer[LF_FACESIZE - 1] = L'\0';
        }

        if (FontFamilyInclude(LogFont, &EntryFaceNameW, Info, min(*Count, Size)))
        {
            if (*Count < Size)
            {
                FontFamilyFillInfo(Info + *Count, EntryFaceNameW.Buffer,
                                   NULL, FontGDI);
            }
            (*Count)++;
        }
        RtlFreeUnicodeString(&EntryFaceNameW);
        Entry = Entry->Flink;
    }

    return TRUE;
}

typedef struct FontFamilyInfoCallbackContext
{
    LPLOGFONTW LogFont;
    PFONTFAMILYINFO Info;
    DWORD Count;
    DWORD Size;
} FONT_FAMILY_INFO_CALLBACK_CONTEXT, *PFONT_FAMILY_INFO_CALLBACK_CONTEXT;

_Function_class_(RTL_QUERY_REGISTRY_ROUTINE)
static NTSTATUS APIENTRY
FontFamilyInfoQueryRegistryCallback(IN PWSTR ValueName, IN ULONG ValueType,
                                    IN PVOID ValueData, IN ULONG ValueLength,
                                    IN PVOID Context, IN PVOID EntryContext)
{
    PFONT_FAMILY_INFO_CALLBACK_CONTEXT InfoContext;
    UNICODE_STRING RegistryName, RegistryValue;
    int Existing;
    PFONTGDI FontGDI;

    if (REG_SZ != ValueType)
    {
        return STATUS_SUCCESS;
    }
    InfoContext = (PFONT_FAMILY_INFO_CALLBACK_CONTEXT) Context;
    RtlInitUnicodeString(&RegistryName, ValueName);

    /* Do we need to include this font family? */
    if (FontFamilyInclude(InfoContext->LogFont, &RegistryName, InfoContext->Info,
                          min(InfoContext->Count, InfoContext->Size)))
    {
        RtlInitUnicodeString(&RegistryValue, (PCWSTR) ValueData);
        Existing = FindFaceNameInInfo(&RegistryValue, InfoContext->Info,
                                      min(InfoContext->Count, InfoContext->Size));
        if (0 <= Existing)
        {
            /* We already have the information about the "real" font. Just copy it */
            if (InfoContext->Count < InfoContext->Size)
            {
                InfoContext->Info[InfoContext->Count] = InfoContext->Info[Existing];
                RtlStringCbCopyNW(InfoContext->Info[InfoContext->Count].EnumLogFontEx.elfLogFont.lfFaceName,
                                  sizeof(InfoContext->Info[InfoContext->Count].EnumLogFontEx.elfLogFont.lfFaceName),
                                  RegistryName.Buffer,
                                  RegistryName.Length);
            }
            InfoContext->Count++;
            return STATUS_SUCCESS;
        }

        /* Try to find information about the "real" font */
        FontGDI = FindFaceNameInLists(&RegistryValue);
        if (NULL == FontGDI)
        {
            /* "Real" font not found, discard this registry entry */
            return STATUS_SUCCESS;
        }

        /* Return info about the "real" font but with the name of the alias */
        if (InfoContext->Count < InfoContext->Size)
        {
            FontFamilyFillInfo(InfoContext->Info + InfoContext->Count,
                               RegistryName.Buffer, NULL, FontGDI);
        }
        InfoContext->Count++;
        return STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}

static BOOLEAN FASTCALL
GetFontFamilyInfoForSubstitutes(LPLOGFONTW LogFont,
                                PFONTFAMILYINFO Info,
                                DWORD *Count,
                                DWORD Size)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = {{0}};
    FONT_FAMILY_INFO_CALLBACK_CONTEXT Context;
    NTSTATUS Status;

    /* Enumerate font families found in HKLM\Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes
       The real work is done in the registry callback function */
    Context.LogFont = LogFont;
    Context.Info = Info;
    Context.Count = *Count;
    Context.Size = Size;

    QueryTable[0].QueryRoutine = FontFamilyInfoQueryRegistryCallback;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Name = NULL;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                    L"FontSubstitutes",
                                    QueryTable,
                                    &Context,
                                    NULL);
    if (NT_SUCCESS(Status))
    {
        *Count = Context.Count;
    }

    return NT_SUCCESS(Status) || STATUS_OBJECT_NAME_NOT_FOUND == Status;
}

BOOL
FASTCALL
ftGdiGetRasterizerCaps(LPRASTERIZER_STATUS lprs)
{
    if ( lprs )
    {
        lprs->nSize = sizeof(RASTERIZER_STATUS);
        lprs->wFlags = TT_AVAILABLE | TT_ENABLED;
        lprs->nLanguageID = gusLanguageID;
        return TRUE;
    }
    EngSetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

static
BOOL
SameScaleMatrix(
    PMATRIX pmx1,
    PMATRIX pmx2)
{
    return (FLOATOBJ_Equal(&pmx1->efM11, &pmx2->efM11) &&
            FLOATOBJ_Equal(&pmx1->efM12, &pmx2->efM12) &&
            FLOATOBJ_Equal(&pmx1->efM21, &pmx2->efM21) &&
            FLOATOBJ_Equal(&pmx1->efM22, &pmx2->efM22));
}

FT_BitmapGlyph APIENTRY
ftGdiGlyphCacheGet(
    FT_Face Face,
    INT GlyphIndex,
    INT Height,
    PMATRIX pmx)
{
    PLIST_ENTRY CurrentEntry;
    PFONT_CACHE_ENTRY FontEntry;

    ASSERT_FREETYPE_LOCK_HELD();

    CurrentEntry = FontCacheListHead.Flink;
    while (CurrentEntry != &FontCacheListHead)
    {
        FontEntry = CONTAINING_RECORD(CurrentEntry, FONT_CACHE_ENTRY, ListEntry);
        if ((FontEntry->Face == Face) &&
            (FontEntry->GlyphIndex == GlyphIndex) &&
            (FontEntry->Height == Height) &&
            (SameScaleMatrix(&FontEntry->mxWorldToDevice, pmx)))
            break;
        CurrentEntry = CurrentEntry->Flink;
    }

    if (CurrentEntry == &FontCacheListHead)
    {
        return NULL;
    }

    RemoveEntryList(CurrentEntry);
    InsertHeadList(&FontCacheListHead, CurrentEntry);
    return FontEntry->BitmapGlyph;
}

/* no cache */
FT_BitmapGlyph APIENTRY
ftGdiGlyphSet(
    FT_Face Face,
    FT_GlyphSlot GlyphSlot,
    FT_Render_Mode RenderMode)
{
    FT_Glyph Glyph;
    INT error;
    FT_Bitmap AlignedBitmap;
    FT_BitmapGlyph BitmapGlyph;

    error = FT_Get_Glyph(GlyphSlot, &Glyph);
    if (error)
    {
        DPRINT1("Failure getting glyph.\n");
        return NULL;
    }

    error = FT_Glyph_To_Bitmap(&Glyph, RenderMode, 0, 1);
    if (error)
    {
        FT_Done_Glyph(Glyph);
        DPRINT1("Failure rendering glyph.\n");
        return NULL;
    }

    BitmapGlyph = (FT_BitmapGlyph)Glyph;
    FT_Bitmap_New(&AlignedBitmap);
    if (FT_Bitmap_Convert(GlyphSlot->library, &BitmapGlyph->bitmap, &AlignedBitmap, 4))
    {
        DPRINT1("Conversion failed\n");
        FT_Done_Glyph((FT_Glyph)BitmapGlyph);
        return NULL;
    }

    FT_Bitmap_Done(GlyphSlot->library, &BitmapGlyph->bitmap);
    BitmapGlyph->bitmap = AlignedBitmap;

    return BitmapGlyph;
}

FT_BitmapGlyph APIENTRY
ftGdiGlyphCacheSet(
    FT_Face Face,
    INT GlyphIndex,
    INT Height,
    PMATRIX pmx,
    FT_GlyphSlot GlyphSlot,
    FT_Render_Mode RenderMode)
{
    FT_Glyph GlyphCopy;
    INT error;
    PFONT_CACHE_ENTRY NewEntry;
    FT_Bitmap AlignedBitmap;
    FT_BitmapGlyph BitmapGlyph;

    ASSERT_FREETYPE_LOCK_HELD();

    error = FT_Get_Glyph(GlyphSlot, &GlyphCopy);
    if (error)
    {
        DPRINT1("Failure caching glyph.\n");
        return NULL;
    };

    error = FT_Glyph_To_Bitmap(&GlyphCopy, RenderMode, 0, 1);
    if (error)
    {
        FT_Done_Glyph(GlyphCopy);
        DPRINT1("Failure rendering glyph.\n");
        return NULL;
    };

    NewEntry = ExAllocatePoolWithTag(PagedPool, sizeof(FONT_CACHE_ENTRY), TAG_FONT);
    if (!NewEntry)
    {
        DPRINT1("Alloc failure caching glyph.\n");
        FT_Done_Glyph(GlyphCopy);
        return NULL;
    }

    BitmapGlyph = (FT_BitmapGlyph)GlyphCopy;
    FT_Bitmap_New(&AlignedBitmap);
    if(FT_Bitmap_Convert(GlyphSlot->library, &BitmapGlyph->bitmap, &AlignedBitmap, 4))
    {
        DPRINT1("Conversion failed\n");
        ExFreePoolWithTag(NewEntry, TAG_FONT);
        FT_Bitmap_Done(GlyphSlot->library, &AlignedBitmap);
        FT_Done_Glyph((FT_Glyph)BitmapGlyph);
        return NULL;
    }

    FT_Bitmap_Done(GlyphSlot->library, &BitmapGlyph->bitmap);
    BitmapGlyph->bitmap = AlignedBitmap;

    NewEntry->GlyphIndex = GlyphIndex;
    NewEntry->Face = Face;
    NewEntry->BitmapGlyph = BitmapGlyph;
    NewEntry->Height = Height;
    NewEntry->mxWorldToDevice = *pmx;

    InsertHeadList(&FontCacheListHead, &NewEntry->ListEntry);
    if (++FontCacheNumEntries > MAX_FONT_CACHE)
    {
        NewEntry = CONTAINING_RECORD(FontCacheListHead.Blink, FONT_CACHE_ENTRY, ListEntry);
        RemoveCachedEntry(NewEntry);
    }

    return BitmapGlyph;
}


static void FTVectorToPOINTFX(FT_Vector *vec, POINTFX *pt)
{
    pt->x.value = vec->x >> 6;
    pt->x.fract = (vec->x & 0x3f) << 10;
    pt->x.fract |= ((pt->x.fract >> 6) | (pt->x.fract >> 12));
    pt->y.value = vec->y >> 6;
    pt->y.fract = (vec->y & 0x3f) << 10;
    pt->y.fract |= ((pt->y.fract >> 6) | (pt->y.fract >> 12));
}

/*
   This function builds an FT_Fixed from a float. It puts the integer part
   in the highest 16 bits and the decimal part in the lowest 16 bits of the FT_Fixed.
   It fails if the integer part of the float number is greater than SHORT_MAX.
*/
static __inline FT_Fixed FT_FixedFromFloat(float f)
{
    short value = f;
    unsigned short fract = (f - value) * 0xFFFF;
    return (FT_Fixed)((long)value << 16 | (unsigned long)fract);
}

/*
   This function builds an FT_Fixed from a FIXED. It simply put f.value
   in the highest 16 bits and f.fract in the lowest 16 bits of the FT_Fixed.
*/
static __inline FT_Fixed FT_FixedFromFIXED(FIXED f)
{
    return (FT_Fixed)((long)f.value << 16 | (unsigned long)f.fract);
}

static unsigned int get_native_glyph_outline(FT_Outline *outline, unsigned int buflen, char *buf)
{
    TTPOLYGONHEADER *pph;
    TTPOLYCURVE *ppc;
    int needed = 0, point = 0, contour, first_pt;
    unsigned int pph_start, cpfx;
    DWORD type;

    for (contour = 0; contour < outline->n_contours; contour++)
    {
        /* Ignore contours containing one point */
        if (point == outline->contours[contour])
        {
            point++;
            continue;
        }

        pph_start = needed;
        pph = (TTPOLYGONHEADER *)(buf + needed);
        first_pt = point;
        if (buf)
        {
            pph->dwType = TT_POLYGON_TYPE;
            FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
        }
        needed += sizeof(*pph);
        point++;
        while (point <= outline->contours[contour])
        {
            ppc = (TTPOLYCURVE *)(buf + needed);
            type = outline->tags[point] & FT_Curve_Tag_On ?
                TT_PRIM_LINE : TT_PRIM_QSPLINE;
            cpfx = 0;
            do
            {
                if (buf)
                    FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                cpfx++;
                point++;
            } while (point <= outline->contours[contour] &&
                    (outline->tags[point] & FT_Curve_Tag_On) ==
                    (outline->tags[point-1] & FT_Curve_Tag_On));
            /* At the end of a contour Windows adds the start point, but
               only for Beziers */
            if (point > outline->contours[contour] &&
               !(outline->tags[point-1] & FT_Curve_Tag_On))
            {
                if (buf)
                    FTVectorToPOINTFX(&outline->points[first_pt], &ppc->apfx[cpfx]);
                cpfx++;
            }
            else if (point <= outline->contours[contour] &&
                      outline->tags[point] & FT_Curve_Tag_On)
            {
                /* add closing pt for bezier */
                if (buf)
                    FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                cpfx++;
                point++;
            }
            if (buf)
            {
                ppc->wType = type;
                ppc->cpfx = cpfx;
            }
            needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
        }
        if (buf)
            pph->cb = needed - pph_start;
    }
    return needed;
}

static unsigned int get_bezier_glyph_outline(FT_Outline *outline, unsigned int buflen, char *buf)
{
    /* Convert the quadratic Beziers to cubic Beziers.
       The parametric eqn for a cubic Bezier is, from PLRM:
       r(t) = at^3 + bt^2 + ct + r0
       with the control points:
       r1 = r0 + c/3
       r2 = r1 + (c + b)/3
       r3 = r0 + c + b + a

       A quadratic Bezier has the form:
       p(t) = (1-t)^2 p0 + 2(1-t)t p1 + t^2 p2

       So equating powers of t leads to:
       r1 = 2/3 p1 + 1/3 p0
       r2 = 2/3 p1 + 1/3 p2
       and of course r0 = p0, r3 = p2
    */
    int contour, point = 0, first_pt;
    TTPOLYGONHEADER *pph;
    TTPOLYCURVE *ppc;
    DWORD pph_start, cpfx, type;
    FT_Vector cubic_control[4];
    unsigned int needed = 0;

    for (contour = 0; contour < outline->n_contours; contour++)
    {
        pph_start = needed;
        pph = (TTPOLYGONHEADER *)(buf + needed);
        first_pt = point;
        if (buf)
        {
            pph->dwType = TT_POLYGON_TYPE;
            FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
        }
        needed += sizeof(*pph);
        point++;
        while (point <= outline->contours[contour])
        {
            ppc = (TTPOLYCURVE *)(buf + needed);
            type = outline->tags[point] & FT_Curve_Tag_On ?
                TT_PRIM_LINE : TT_PRIM_CSPLINE;
            cpfx = 0;
            do
            {
                if (type == TT_PRIM_LINE)
                {
                    if (buf)
                        FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                    cpfx++;
                    point++;
                }
                else
                {
                    /* Unlike QSPLINEs, CSPLINEs always have their endpoint
                       so cpfx = 3n */

                    /* FIXME: Possible optimization in endpoint calculation
                       if there are two consecutive curves */
                    cubic_control[0] = outline->points[point-1];
                    if (!(outline->tags[point-1] & FT_Curve_Tag_On))
                    {
                        cubic_control[0].x += outline->points[point].x + 1;
                        cubic_control[0].y += outline->points[point].y + 1;
                        cubic_control[0].x >>= 1;
                        cubic_control[0].y >>= 1;
                    }
                    if (point+1 > outline->contours[contour])
                        cubic_control[3] = outline->points[first_pt];
                    else
                    {
                        cubic_control[3] = outline->points[point+1];
                        if (!(outline->tags[point+1] & FT_Curve_Tag_On))
                        {
                            cubic_control[3].x += outline->points[point].x + 1;
                            cubic_control[3].y += outline->points[point].y + 1;
                            cubic_control[3].x >>= 1;
                            cubic_control[3].y >>= 1;
                        }
                    }
                    /* r1 = 1/3 p0 + 2/3 p1
                       r2 = 1/3 p2 + 2/3 p1 */
                    cubic_control[1].x = (2 * outline->points[point].x + 1) / 3;
                    cubic_control[1].y = (2 * outline->points[point].y + 1) / 3;
                    cubic_control[2] = cubic_control[1];
                    cubic_control[1].x += (cubic_control[0].x + 1) / 3;
                    cubic_control[1].y += (cubic_control[0].y + 1) / 3;
                    cubic_control[2].x += (cubic_control[3].x + 1) / 3;
                    cubic_control[2].y += (cubic_control[3].y + 1) / 3;
                    if (buf)
                    {
                        FTVectorToPOINTFX(&cubic_control[1], &ppc->apfx[cpfx]);
                        FTVectorToPOINTFX(&cubic_control[2], &ppc->apfx[cpfx+1]);
                        FTVectorToPOINTFX(&cubic_control[3], &ppc->apfx[cpfx+2]);
                    }
                    cpfx += 3;
                    point++;
                }
            } while (point <= outline->contours[contour] &&
                    (outline->tags[point] & FT_Curve_Tag_On) ==
                    (outline->tags[point-1] & FT_Curve_Tag_On));
            /* At the end of a contour Windows adds the start point,
               but only for Beziers and we've already done that.
            */
            if (point <= outline->contours[contour] &&
               outline->tags[point] & FT_Curve_Tag_On)
            {
                /* This is the closing pt of a bezier, but we've already
                   added it, so just inc point and carry on */
                point++;
            }
            if (buf)
            {
                ppc->wType = type;
                ppc->cpfx = cpfx;
            }
            needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
        }
        if (buf)
            pph->cb = needed - pph_start;
    }
    return needed;
}

static INT
IntRequestFontSize(PDC dc, FT_Face face, LONG Width, LONG Height)
{
    FT_Size_RequestRec  req;

    if (Width < 0)
        Width = -Width;

    if (Height < 0)
    {
        Height = -Height;
    }
    if (Height == 0)
    {
        Height = dc->ppdev->devinfo.lfDefaultFont.lfHeight;
    }
    if (Height == 0)
    {
        Height = Width;
    }

    if (Height < 1)
        Height = 1;

    if (Width > 0xFFFFU)
        Width = 0xFFFFU;
    if (Height > 0xFFFFU)
        Height = 0xFFFFU;

    req.type           = FT_SIZE_REQUEST_TYPE_NOMINAL;
    req.width          = (FT_Long)(Width << 6);
    req.height         = (FT_Long)(Height << 6);
    req.horiResolution = 0;
    req.vertResolution = 0;
    return FT_Request_Size(face, &req);
}

BOOL
FASTCALL
TextIntUpdateSize(PDC dc,
                  PTEXTOBJ TextObj,
                  PFONTGDI FontGDI,
                  BOOL bDoLock)
{
    FT_Face face;
    INT error, n;
    FT_CharMap charmap, found;
    LOGFONTW *plf;

    if (bDoLock)
        IntLockFreeType;

    face = FontGDI->SharedFace->Face;
    if (face->charmap == NULL)
    {
        DPRINT("WARNING: No charmap selected!\n");
        DPRINT("This font face has %d charmaps\n", face->num_charmaps);

        found = NULL;
        for (n = 0; n < face->num_charmaps; n++)
        {
            charmap = face->charmaps[n];
            DPRINT("Found charmap encoding: %i\n", charmap->encoding);
            if (charmap->encoding != 0)
            {
                found = charmap;
                break;
            }
        }
        if (!found)
        {
            DPRINT1("WARNING: Could not find desired charmap!\n");
        }
        else
        {
            error = FT_Set_Charmap(face, found);
            if (error)
            {
                DPRINT1("WARNING: Could not set the charmap!\n");
            }
        }
    }

    plf = &TextObj->logfont.elfEnumLogfontEx.elfLogFont;

    error = IntRequestFontSize(dc, face, plf->lfWidth, plf->lfHeight);

    if (bDoLock)
        IntUnLockFreeType;

    if (error)
    {
        DPRINT1("Error in setting pixel sizes: %d\n", error);
        return FALSE;
    }

    return TRUE;
}


/*
 * Based on WineEngGetGlyphOutline
 *
 */
ULONG
FASTCALL
ftGdiGetGlyphOutline(
    PDC dc,
    WCHAR wch,
    UINT iFormat,
    LPGLYPHMETRICS pgm,
    ULONG cjBuf,
    PVOID pvBuf,
    LPMAT2 pmat2,
    BOOL bIgnoreRotation)
{
    static const FT_Matrix identityMat = {(1 << 16), 0, 0, (1 << 16)};
    PDC_ATTR pdcattr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    HFONT hFont = 0;
    GLYPHMETRICS gm;
    ULONG Size;
    FT_Face ft_face;
    FT_UInt glyph_index;
    DWORD width, height, pitch, needed = 0;
    FT_Bitmap ft_bitmap;
    FT_Error error;
    INT left, right, top = 0, bottom = 0;
    FT_Angle angle = 0;
    FT_Int load_flags = FT_LOAD_DEFAULT | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;
    FLOAT eM11, widthRatio = 1.0;
    FT_Matrix transMat = identityMat;
    BOOL needsTransform = FALSE;
    INT orientation;
    LONG aveWidth;
    INT adv, lsb, bbx; /* These three hold to widths of the unrotated chars */
    OUTLINETEXTMETRICW *potm;
    XFORM xForm;
    LOGFONTW *plf;

    DPRINT("%u, %08x, %p, %08lx, %p, %p\n", wch, iFormat, pgm,
           cjBuf, pvBuf, pmat2);

    pdcattr = dc->pdcattr;

    MatrixS2XForm(&xForm, &dc->pdcattr->mxWorldToDevice);
    eM11 = xForm.eM11;

    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);

    if (!TextObj)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    FontGDI = ObjToGDI(TextObj->Font, FONT);
    ft_face = FontGDI->SharedFace->Face;

    plf = &TextObj->logfont.elfEnumLogfontEx.elfLogFont;
    aveWidth = FT_IS_SCALABLE(ft_face) ? abs(plf->lfWidth) : 0;
    orientation = FT_IS_SCALABLE(ft_face) ? plf->lfOrientation : 0;

    Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
    potm = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
    if (!potm)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        TEXTOBJ_UnlockText(TextObj);
        return GDI_ERROR;
    }
    IntGetOutlineTextMetrics(FontGDI, Size, potm);

    IntLockFreeType;
    TextIntUpdateSize(dc, TextObj, FontGDI, FALSE);
    FtSetCoordinateTransform(ft_face, DC_pmxWorldToDevice(dc));

    TEXTOBJ_UnlockText(TextObj);

    if (iFormat & GGO_GLYPH_INDEX)
    {
        glyph_index = wch;
        iFormat &= ~GGO_GLYPH_INDEX;
    }
    else  glyph_index = FT_Get_Char_Index(ft_face, wch);

    if (orientation || (iFormat != GGO_METRICS && iFormat != GGO_BITMAP) || aveWidth || pmat2)
        load_flags |= FT_LOAD_NO_BITMAP;

    if (iFormat & GGO_UNHINTED)
    {
        load_flags |= FT_LOAD_NO_HINTING;
        iFormat &= ~GGO_UNHINTED;
    }

    error = FT_Load_Glyph(ft_face, glyph_index, load_flags);
    if (error)
    {
        DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
        IntUnLockFreeType;
        if (potm) ExFreePoolWithTag(potm, GDITAG_TEXT);
        return GDI_ERROR;
    }
    IntUnLockFreeType;

    if (aveWidth && potm)
    {
        widthRatio = (FLOAT)aveWidth * eM11 /
                     (FLOAT) potm->otmTextMetrics.tmAveCharWidth;
    }

    left = (INT)(ft_face->glyph->metrics.horiBearingX * widthRatio) & -64;
    right = (INT)((ft_face->glyph->metrics.horiBearingX +
                   ft_face->glyph->metrics.width) * widthRatio + 63) & -64;

    adv = (INT)((ft_face->glyph->metrics.horiAdvance * widthRatio) + 63) >> 6;
    lsb = left >> 6;
    bbx = (right - left) >> 6;

    DPRINT("Advance = %d, lsb = %d, bbx = %d\n",adv, lsb, bbx);

    IntLockFreeType;

    /* Scaling transform */
    /*if (aveWidth)*/
    {

        FT_Matrix ftmatrix;
        FLOATOBJ efTemp;

        PMATRIX pmx = DC_pmxWorldToDevice(dc);

        /* Create a freetype matrix, by converting to 16.16 fixpoint format */
        efTemp = pmx->efM11;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.xx = FLOATOBJ_GetLong(&efTemp);

        efTemp = pmx->efM12;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.xy = FLOATOBJ_GetLong(&efTemp);

        efTemp = pmx->efM21;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.yx = FLOATOBJ_GetLong(&efTemp);

        efTemp = pmx->efM22;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.yy = FLOATOBJ_GetLong(&efTemp);

        FT_Matrix_Multiply(&ftmatrix, &transMat);
        needsTransform = TRUE;
    }

    /* Rotation transform */
    if (orientation)
    {
        FT_Matrix rotationMat;
        FT_Vector vecAngle;
        DPRINT("Rotation Trans!\n");
        angle = FT_FixedFromFloat((float)orientation / 10.0);
        FT_Vector_Unit(&vecAngle, angle);
        rotationMat.xx = vecAngle.x;
        rotationMat.xy = -vecAngle.y;
        rotationMat.yx = -rotationMat.xy;
        rotationMat.yy = rotationMat.xx;
        FT_Matrix_Multiply(&rotationMat, &transMat);
        needsTransform = TRUE;
    }

    /* Extra transformation specified by caller */
    if (pmat2)
    {
        FT_Matrix extraMat;
        DPRINT("MAT2 Matrix Trans!\n");
        extraMat.xx = FT_FixedFromFIXED(pmat2->eM11);
        extraMat.xy = FT_FixedFromFIXED(pmat2->eM21);
        extraMat.yx = FT_FixedFromFIXED(pmat2->eM12);
        extraMat.yy = FT_FixedFromFIXED(pmat2->eM22);
        FT_Matrix_Multiply(&extraMat, &transMat);
        needsTransform = TRUE;
    }

    if (potm) ExFreePoolWithTag(potm, GDITAG_TEXT); /* It looks like we are finished with potm ATM. */

    if (!needsTransform)
    {
        DPRINT("No Need to be Transformed!\n");
        top = (ft_face->glyph->metrics.horiBearingY + 63) & -64;
        bottom = (ft_face->glyph->metrics.horiBearingY -
                  ft_face->glyph->metrics.height) & -64;
        gm.gmCellIncX = adv;
        gm.gmCellIncY = 0;
    }
    else
    {
        INT xc, yc;
        FT_Vector vec;
        for (xc = 0; xc < 2; xc++)
        {
            for (yc = 0; yc < 2; yc++)
            {
                vec.x = (ft_face->glyph->metrics.horiBearingX +
                         xc * ft_face->glyph->metrics.width);
                vec.y = ft_face->glyph->metrics.horiBearingY -
                        yc * ft_face->glyph->metrics.height;
                DPRINT("Vec %ld,%ld\n", vec.x, vec.y);
                FT_Vector_Transform(&vec, &transMat);
                if (xc == 0 && yc == 0)
                {
                    left = right = vec.x;
                    top = bottom = vec.y;
                }
                else
                {
                    if (vec.x < left) left = vec.x;
                    else if (vec.x > right) right = vec.x;
                    if (vec.y < bottom) bottom = vec.y;
                    else if (vec.y > top) top = vec.y;
                }
            }
        }
        left = left & -64;
        right = (right + 63) & -64;
        bottom = bottom & -64;
        top = (top + 63) & -64;

        DPRINT("Transformed box: (%d,%d - %d,%d)\n", left, top, right, bottom);
        vec.x = ft_face->glyph->metrics.horiAdvance;
        vec.y = 0;
        FT_Vector_Transform(&vec, &transMat);
        gm.gmCellIncX = (vec.x+63) >> 6;
        gm.gmCellIncY = -((vec.y+63) >> 6);
    }
    gm.gmBlackBoxX = (right - left) >> 6;
    gm.gmBlackBoxY = (top - bottom) >> 6;
    gm.gmptGlyphOrigin.x = left >> 6;
    gm.gmptGlyphOrigin.y = top >> 6;

    DPRINT("CX %d CY %d BBX %u BBY %u GOX %d GOY %d\n",
           gm.gmCellIncX, gm.gmCellIncY,
           gm.gmBlackBoxX, gm.gmBlackBoxY,
           gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);

    IntUnLockFreeType;


    if (iFormat == GGO_METRICS)
    {
        DPRINT("GGO_METRICS Exit!\n");
        *pgm = gm;
        return 1; /* FIXME */
    }

    if (ft_face->glyph->format != ft_glyph_format_outline && iFormat != GGO_BITMAP)
    {
        DPRINT1("Loaded a bitmap\n");
        return GDI_ERROR;
    }

    switch (iFormat)
    {
    case GGO_BITMAP:
        width = gm.gmBlackBoxX;
        height = gm.gmBlackBoxY;
        pitch = ((width + 31) >> 5) << 2;
        needed = pitch * height;

        if (!pvBuf || !cjBuf) break;
        if (!needed) return GDI_ERROR;  /* empty glyph */
        if (needed > cjBuf)
            return GDI_ERROR;

        switch (ft_face->glyph->format)
        {
        case ft_glyph_format_bitmap:
        {
            BYTE *src = ft_face->glyph->bitmap.buffer, *dst = pvBuf;
            INT w = min( pitch, (ft_face->glyph->bitmap.width + 7) >> 3 );
            INT h = min( height, ft_face->glyph->bitmap.rows );
            while (h--)
            {
                RtlCopyMemory(dst, src, w);
                src += ft_face->glyph->bitmap.pitch;
                dst += pitch;
            }
            break;
        }

        case ft_glyph_format_outline:
            ft_bitmap.width = width;
            ft_bitmap.rows = height;
            ft_bitmap.pitch = pitch;
            ft_bitmap.pixel_mode = FT_PIXEL_MODE_MONO;
            ft_bitmap.buffer = pvBuf;

            IntLockFreeType;
            if (needsTransform)
            {
                FT_Outline_Transform(&ft_face->glyph->outline, &transMat);
            }
            FT_Outline_Translate(&ft_face->glyph->outline, -left, -bottom );
            /* Note: FreeType will only set 'black' bits for us. */
            RtlZeroMemory(pvBuf, needed);
            FT_Outline_Get_Bitmap(library, &ft_face->glyph->outline, &ft_bitmap);
            IntUnLockFreeType;
            break;

        default:
            DPRINT1("Loaded glyph format %x\n", ft_face->glyph->format);
            return GDI_ERROR;
        }
        break;

    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
    {
        unsigned int mult, row, col;
        BYTE *start, *ptr;

        width = gm.gmBlackBoxX;
        height = gm.gmBlackBoxY;
        pitch = (width + 3) / 4 * 4;
        needed = pitch * height;

        if (!pvBuf || !cjBuf) break;
        if (!needed) return GDI_ERROR;  /* empty glyph */
        if (needed > cjBuf)
            return GDI_ERROR;

        switch (ft_face->glyph->format)
        {
        case ft_glyph_format_bitmap:
        {
            BYTE *src = ft_face->glyph->bitmap.buffer, *dst = pvBuf;
            INT h = min( height, ft_face->glyph->bitmap.rows );
            INT x;
            while (h--)
            {
                for (x = 0; (UINT)x < pitch; x++)
                {
                    if (x < ft_face->glyph->bitmap.width)
                        dst[x] = (src[x / 8] & (1 << ( (7 - (x % 8))))) ? 0xff : 0;
                    else
                        dst[x] = 0;
                }
                src += ft_face->glyph->bitmap.pitch;
                dst += pitch;
            }
            break;
        }
        case ft_glyph_format_outline:
        {
            ft_bitmap.width = width;
            ft_bitmap.rows = height;
            ft_bitmap.pitch = pitch;
            ft_bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
            ft_bitmap.buffer = pvBuf;

            IntLockFreeType;
            if (needsTransform)
            {
                FT_Outline_Transform(&ft_face->glyph->outline, &transMat);
            }
            FT_Outline_Translate(&ft_face->glyph->outline, -left, -bottom );
            RtlZeroMemory(ft_bitmap.buffer, cjBuf);
            FT_Outline_Get_Bitmap(library, &ft_face->glyph->outline, &ft_bitmap);
            IntUnLockFreeType;

            if (iFormat == GGO_GRAY2_BITMAP)
                mult = 4;
            else if (iFormat == GGO_GRAY4_BITMAP)
                mult = 16;
            else if (iFormat == GGO_GRAY8_BITMAP)
                mult = 64;
            else
            {
                return GDI_ERROR;
            }

            start = pvBuf;
            for (row = 0; row < height; row++)
            {
                ptr = start;
                for (col = 0; col < width; col++, ptr++)
                {
                    *ptr = (((int)*ptr) * mult + 128) / 256;
                }
                start += pitch;
            }

            break;
        }
        default:
            DPRINT1("Loaded glyph format %x\n", ft_face->glyph->format);
            return GDI_ERROR;
        }
    }

    case GGO_NATIVE:
    {
        FT_Outline *outline = &ft_face->glyph->outline;

        if (cjBuf == 0) pvBuf = NULL; /* This is okay, need cjBuf to allocate. */

        IntLockFreeType;
        if (needsTransform && pvBuf) FT_Outline_Transform(outline, &transMat);

        needed = get_native_glyph_outline(outline, cjBuf, NULL);

        if (!pvBuf || !cjBuf)
        {
            IntUnLockFreeType;
            break;
        }
        if (needed > cjBuf)
        {
            IntUnLockFreeType;
            return GDI_ERROR;
        }
        get_native_glyph_outline(outline, cjBuf, pvBuf);
        IntUnLockFreeType;
        break;
    }
    case GGO_BEZIER:
    {
        FT_Outline *outline = &ft_face->glyph->outline;
        if (cjBuf == 0) pvBuf = NULL;

        if (needsTransform && pvBuf)
        {
            IntLockFreeType;
            FT_Outline_Transform(outline, &transMat);
            IntUnLockFreeType;
        }
        needed = get_bezier_glyph_outline(outline, cjBuf, NULL);

        if (!pvBuf || !cjBuf)
            break;
        if (needed > cjBuf)
            return GDI_ERROR;

        get_bezier_glyph_outline(outline, cjBuf, pvBuf);
        break;
    }

    default:
        DPRINT1("Unsupported format %u\n", iFormat);
        return GDI_ERROR;
    }

    DPRINT("ftGdiGetGlyphOutline END and needed %lu\n", needed);
    *pgm = gm;
    return needed;
}

BOOL
FASTCALL
TextIntGetTextExtentPoint(PDC dc,
                          PTEXTOBJ TextObj,
                          LPCWSTR String,
                          INT Count,
                          ULONG MaxExtent,
                          LPINT Fit,
                          LPINT Dx,
                          LPSIZE Size,
                          FLONG fl)
{
    PFONTGDI FontGDI;
    FT_Face face;
    FT_GlyphSlot glyph;
    FT_BitmapGlyph realglyph;
    INT error, glyph_index, i, previous;
    ULONGLONG TotalWidth = 0;
    BOOL use_kerning;
    FT_Render_Mode RenderMode;
    BOOLEAN Render;
    PMATRIX pmxWorldToDevice;
    LOGFONTW *plf;
    BOOL EmuBold, EmuItalic;

    FontGDI = ObjToGDI(TextObj->Font, FONT);

    face = FontGDI->SharedFace->Face;
    if (NULL != Fit)
    {
        *Fit = 0;
    }

    IntLockFreeType;

    TextIntUpdateSize(dc, TextObj, FontGDI, FALSE);

    plf = &TextObj->logfont.elfEnumLogfontEx.elfLogFont;
    EmuBold = (plf->lfWeight >= FW_BOLD && FontGDI->OriginalWeight <= FW_NORMAL);
    EmuItalic = (plf->lfItalic && !FontGDI->OriginalItalic);

    Render = IntIsFontRenderingEnabled();
    if (Render)
        RenderMode = IntGetFontRenderMode(plf);
    else
        RenderMode = FT_RENDER_MODE_MONO;


    /* Get the DC's world-to-device transformation matrix */
    pmxWorldToDevice = DC_pmxWorldToDevice(dc);
    FtSetCoordinateTransform(face, pmxWorldToDevice);

    use_kerning = FT_HAS_KERNING(face);
    previous = 0;

    for (i = 0; i < Count; i++)
    {
        if (fl & GTEF_INDICES)
            glyph_index = *String;
        else
            glyph_index = FT_Get_Char_Index(face, *String);

        if (EmuBold || EmuItalic)
            realglyph = NULL;
        else
            realglyph = ftGdiGlyphCacheGet(face, glyph_index,
                                           plf->lfHeight, pmxWorldToDevice);

        if (EmuBold || EmuItalic || !realglyph)
        {
            error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            if (error)
            {
                DPRINT1("WARNING: Failed to load and render glyph! [index: %d]\n", glyph_index);
                break;
            }

            glyph = face->glyph;
            if (EmuBold || EmuItalic)
            {
                if (EmuBold)
                    FT_GlyphSlot_Embolden(glyph);
                if (EmuItalic)
                    FT_GlyphSlot_Oblique(glyph);
                realglyph = ftGdiGlyphSet(face, glyph, RenderMode);
            }
            else
            {
                realglyph = ftGdiGlyphCacheSet(face,
                                               glyph_index,
                                               plf->lfHeight,
                                               pmxWorldToDevice,
                                               glyph,
                                               RenderMode);
            }

            if (!realglyph)
            {
                DPRINT1("Failed to render glyph! [index: %d]\n", glyph_index);
                break;
            }
        }

        /* Retrieve kerning distance */
        if (use_kerning && previous && glyph_index)
        {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
            TotalWidth += delta.x;
        }

        TotalWidth += realglyph->root.advance.x >> 10;

        if (((TotalWidth + 32) >> 6) <= MaxExtent && NULL != Fit)
        {
            *Fit = i + 1;
        }
        if (NULL != Dx)
        {
            Dx[i] = (TotalWidth + 32) >> 6;
        }

        if (EmuBold || EmuItalic)
        {
            FT_Done_Glyph((FT_Glyph)realglyph);
            realglyph = NULL;
        }

        previous = glyph_index;
        String++;
    }
    IntUnLockFreeType;

    Size->cx = (TotalWidth + 32) >> 6;
    Size->cy = (plf->lfHeight == 0 ?
                dc->ppdev->devinfo.lfDefaultFont.lfHeight :
                abs(plf->lfHeight));
    Size->cy = EngMulDiv(Size->cy, dc->ppdev->gdiinfo.ulLogPixelsY, 72);

    return TRUE;
}


INT
FASTCALL
ftGdiGetTextCharsetInfo(
    PDC Dc,
    LPFONTSIGNATURE lpSig,
    DWORD dwFlags)
{
    PDC_ATTR pdcattr;
    UINT Ret = DEFAULT_CHARSET;
    INT i;
    HFONT hFont;
    PTEXTOBJ TextObj;
    PFONTGDI FontGdi;
    FONTSIGNATURE fs;
    TT_OS2 *pOS2;
    FT_Face Face;
    CHARSETINFO csi;
    DWORD cp, fs0;
    USHORT usACP, usOEM;

    pdcattr = Dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);

    if (!TextObj)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return Ret;
    }
    FontGdi = ObjToGDI(TextObj->Font, FONT);
    Face = FontGdi->SharedFace->Face;
    TEXTOBJ_UnlockText(TextObj);

    IntLockFreeType;
    pOS2 = FT_Get_Sfnt_Table(Face, ft_sfnt_os2);
    IntUnLockFreeType;
    memset(&fs, 0, sizeof(FONTSIGNATURE));
    if (NULL != pOS2)
    {
        fs.fsCsb[0] = pOS2->ulCodePageRange1;
        fs.fsCsb[1] = pOS2->ulCodePageRange2;
        fs.fsUsb[0] = pOS2->ulUnicodeRange1;
        fs.fsUsb[1] = pOS2->ulUnicodeRange2;
        fs.fsUsb[2] = pOS2->ulUnicodeRange3;
        fs.fsUsb[3] = pOS2->ulUnicodeRange4;
        if (pOS2->version == 0)
        {
            FT_UInt dummy;

            if (FT_Get_First_Char( Face, &dummy ) < 0x100)
                fs.fsCsb[0] |= FS_LATIN1;
            else
                fs.fsCsb[0] |= FS_SYMBOL;
        }
    }
    DPRINT("Csb 1=%x  0=%x\n", fs.fsCsb[1],fs.fsCsb[0]);
    if (fs.fsCsb[0] == 0)
    { /* Let's see if we can find any interesting cmaps */
        for (i = 0; i < Face->num_charmaps; i++)
        {
            switch (Face->charmaps[i]->encoding)
            {
            case FT_ENCODING_UNICODE:
            case FT_ENCODING_APPLE_ROMAN:
                fs.fsCsb[0] |= FS_LATIN1;
                break;
            case FT_ENCODING_MS_SYMBOL:
                fs.fsCsb[0] |= FS_SYMBOL;
                break;
            default:
                break;
            }
        }
    }
    if (lpSig)
    {
        RtlCopyMemory(lpSig, &fs, sizeof(FONTSIGNATURE));
    }

    RtlGetDefaultCodePage(&usACP, &usOEM);
    cp = usACP;

    if (IntTranslateCharsetInfo(&cp, &csi, TCI_SRCCODEPAGE))
        if (csi.fs.fsCsb[0] & fs.fsCsb[0])
        {
            DPRINT("Hit 1\n");
            Ret = csi.ciCharset;
            goto Exit;
        }

    for (i = 0; i < MAXTCIINDEX; i++)
    {
        fs0 = 1L << i;
        if (fs.fsCsb[0] & fs0)
        {
            if (IntTranslateCharsetInfo(&fs0, &csi, TCI_SRCFONTSIG))
            {
                // *cp = csi.ciACP;
                DPRINT("Hit 2\n");
                Ret = csi.ciCharset;
                goto Exit;
            }
            else
                DPRINT1("TCI failing on %x\n", fs0);
        }
    }
Exit:
    DPRINT("CharSet %u CodePage %u\n", csi.ciCharset, csi.ciACP);
    return (MAKELONG(csi.ciACP, csi.ciCharset));
}


DWORD
FASTCALL
ftGetFontUnicodeRanges(PFONTGDI Font, PGLYPHSET glyphset)
{
    DWORD size = 0;
    DWORD num_ranges = 0;
    FT_Face face = Font->SharedFace->Face;

    if (face->charmap->encoding == FT_ENCODING_UNICODE)
    {
        FT_UInt glyph_code = 0;
        FT_ULong char_code, char_code_prev;

        char_code_prev = char_code = FT_Get_First_Char(face, &glyph_code);

        DPRINT("Face encoding FT_ENCODING_UNICODE, number of glyphs %ld, first glyph %u, first char %04lx\n",
               face->num_glyphs, glyph_code, char_code);

        if (!glyph_code) return 0;

        if (glyphset)
        {
            glyphset->ranges[0].wcLow = (USHORT)char_code;
            glyphset->ranges[0].cGlyphs = 0;
            glyphset->cGlyphsSupported = 0;
        }

        num_ranges = 1;
        while (glyph_code)
        {
            if (char_code < char_code_prev)
            {
                DPRINT1("Expected increasing char code from FT_Get_Next_Char\n");
                return 0;
            }
            if (char_code - char_code_prev > 1)
            {
                num_ranges++;
                if (glyphset)
                {
                    glyphset->ranges[num_ranges - 1].wcLow = (USHORT)char_code;
                    glyphset->ranges[num_ranges - 1].cGlyphs = 1;
                    glyphset->cGlyphsSupported++;
                }
            }
            else if (glyphset)
            {
                glyphset->ranges[num_ranges - 1].cGlyphs++;
                glyphset->cGlyphsSupported++;
            }
            char_code_prev = char_code;
            char_code = FT_Get_Next_Char(face, char_code, &glyph_code);
        }
    }
    else
        DPRINT1("Encoding %i not supported\n", face->charmap->encoding);

    size = sizeof(GLYPHSET) + sizeof(WCRANGE) * (num_ranges - 1);
    if (glyphset)
    {
        glyphset->cbThis = size;
        glyphset->cRanges = num_ranges;
        glyphset->flAccel = 0;
    }
    return size;
}


BOOL
FASTCALL
ftGdiGetTextMetricsW(
    HDC hDC,
    PTMW_INTERNAL ptmwi)
{
    PDC dc;
    PDC_ATTR pdcattr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    FT_Face Face;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    FT_WinFNT_HeaderRec Win;
    ULONG Error;
    NTSTATUS Status = STATUS_SUCCESS;
    LOGFONTW *plf;

    if (!ptmwi)
    {
        EngSetLastError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;
    TextObj = RealizeFontInit(pdcattr->hlfntNew);
    if (NULL != TextObj)
    {
        plf = &TextObj->logfont.elfEnumLogfontEx.elfLogFont;
        FontGDI = ObjToGDI(TextObj->Font, FONT);

        Face = FontGDI->SharedFace->Face;
        IntLockFreeType;
        Error = IntRequestFontSize(dc, Face, plf->lfWidth, plf->lfHeight);
        FtSetCoordinateTransform(Face, DC_pmxWorldToDevice(dc));
        IntUnLockFreeType;
        if (0 != Error)
        {
            DPRINT1("Error in setting pixel sizes: %u\n", Error);
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            FT_Face Face = FontGDI->SharedFace->Face;
            Status = STATUS_SUCCESS;

            IntLockFreeType;
            pOS2 = FT_Get_Sfnt_Table(Face, ft_sfnt_os2);
            if (NULL == pOS2)
            {
                DPRINT1("Can't find OS/2 table - not TT font?\n");
                Status = STATUS_INTERNAL_ERROR;
            }

            pHori = FT_Get_Sfnt_Table(Face, ft_sfnt_hhea);
            if (NULL == pHori)
            {
                DPRINT1("Can't find HHEA table - not TT font?\n");
                Status = STATUS_INTERNAL_ERROR;
            }

            Error = FT_Get_WinFNT_Header(Face, &Win);

            IntUnLockFreeType;

            if (NT_SUCCESS(Status))
            {
                FillTM(&ptmwi->TextMetric, FontGDI, pOS2, pHori, !Error ? &Win : 0);

                /* FIXME: Fill Diff member */
                RtlZeroMemory(&ptmwi->Diff, sizeof(ptmwi->Diff));
            }
        }
        TEXTOBJ_UnlockText(TextObj);
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }
    DC_UnlockDc(dc);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }
    return TRUE;
}

DWORD
FASTCALL
ftGdiGetFontData(
    PFONTGDI FontGdi,
    DWORD Table,
    DWORD Offset,
    PVOID Buffer,
    DWORD Size)
{
    DWORD Result = GDI_ERROR;
    FT_Face Face = FontGdi->SharedFace->Face;

    IntLockFreeType;

    if (FT_IS_SFNT(Face))
    {
        if (Table)
            Table = Table >> 24 | Table << 24 | (Table >> 8 & 0xFF00) |
                    (Table << 8 & 0xFF0000);

        if (!Buffer) Size = 0;

        if (Buffer && Size)
        {
            FT_Error Error;
            FT_ULong Needed = 0;

            Error = FT_Load_Sfnt_Table(Face, Table, Offset, NULL, &Needed);

            if ( !Error && Needed < Size) Size = Needed;
        }
        if (!FT_Load_Sfnt_Table(Face, Table, Offset, Buffer, &Size))
            Result = Size;
    }

    IntUnLockFreeType;

    return Result;
}

// NOTE: See Table 1. of https://msdn.microsoft.com/en-us/library/ms969909.aspx
static UINT FASTCALL
GetFontPenalty(LOGFONTW *               LogFont,
               PUNICODE_STRING          RequestedNameW,
               PUNICODE_STRING          ActualNameW,
               PUNICODE_STRING          FullFaceNameW,
               BYTE                     RequestedCharSet,
               PFONTGDI                 FontGDI,
               OUTLINETEXTMETRICW *     Otm,
               TEXTMETRICW *            TM,
               const char *             style_name)
{
    ULONG   Penalty = 0;
    BYTE    Byte;
    LONG    Long;
    BOOL    fFixedSys = FALSE, fNeedScaling = FALSE;
    const BYTE UserCharSet = CharSetFromLangID(gusLanguageID);
    NTSTATUS Status;

    /* FIXME: Aspect Penalty 30 */
    /* FIXME: IntSizeSynth Penalty 20 */
    /* FIXME: SmallPenalty Penalty 1 */
    /* FIXME: FaceNameSubst Penalty 500 */

    if (RtlEqualUnicodeString(RequestedNameW, &SystemW, TRUE))
    {
        /* "System" font */
        if (TM->tmCharSet != UserCharSet)
        {
            /* CharSet Penalty 65000 */
            /* Requested charset does not match the candidate's. */
            Penalty += 65000;
        }
    }
    else if (RtlEqualUnicodeString(RequestedNameW, &FixedSysW, TRUE))
    {
        /* "FixedSys" font */
        if (TM->tmCharSet != UserCharSet)
        {
            /* CharSet Penalty 65000 */
            /* Requested charset does not match the candidate's. */
            Penalty += 65000;
        }
        fFixedSys = TRUE;
    }
    else    /* Request is non-"System" font */
    {
        Byte = RequestedCharSet;
        if (Byte == DEFAULT_CHARSET)
        {
            if (RtlEqualUnicodeString(RequestedNameW, &MarlettW, TRUE))
            {
                if (Byte == ANSI_CHARSET)
                {
                    DPRINT("Warning: FIXME: It's Marlett but ANSI_CHARSET.\n");
                }
                /* We assume SYMBOL_CHARSET for "Marlett" font */
                Byte = SYMBOL_CHARSET;
            }
        }

        if (Byte != TM->tmCharSet)
        {
            if (Byte != DEFAULT_CHARSET && Byte != ANSI_CHARSET)
            {
                /* CharSet Penalty 65000 */
                /* Requested charset does not match the candidate's. */
                Penalty += 65000;
            }
            else
            {
                if (UserCharSet != TM->tmCharSet)
                {
                    /* UNDOCUMENTED */
                    Penalty += 100;
                    if (ANSI_CHARSET != TM->tmCharSet)
                    {
                        /* UNDOCUMENTED */
                        Penalty += 100;
                    }
                }
            }
        }
    }

    Byte = LogFont->lfOutPrecision;
    if (Byte == OUT_DEFAULT_PRECIS)
        Byte = OUT_OUTLINE_PRECIS;  /* Is it OK? */
    switch (Byte)
    {
        case OUT_DEVICE_PRECIS:
            if (!(TM->tmPitchAndFamily & TMPF_DEVICE) ||
                !(TM->tmPitchAndFamily & (TMPF_VECTOR | TMPF_TRUETYPE)))
            {
                /* OutputPrecision Penalty 19000 */
                /* Requested OUT_STROKE_PRECIS, but the device can't do it
                   or the candidate is not a vector font. */
                Penalty += 19000;
            }
            break;
        default:
            if (TM->tmPitchAndFamily & (TMPF_VECTOR | TMPF_TRUETYPE))
            {
                /* OutputPrecision Penalty 19000 */
                /* Or OUT_STROKE_PRECIS not requested, and the candidate
                   is a vector font that requires GDI support. */
                Penalty += 19000;
            }
            break;
    }

    Byte = (LogFont->lfPitchAndFamily & 0x0F);
    if (Byte == DEFAULT_PITCH)
        Byte = VARIABLE_PITCH;
    if (fFixedSys)
    {
        /* "FixedSys" font should be fixed-pitch */
        Byte = FIXED_PITCH;
    }
    if (Byte == FIXED_PITCH)
    {
        if (TM->tmPitchAndFamily & _TMPF_VARIABLE_PITCH)
        {
            /* FixedPitch Penalty 15000 */
            /* Requested a fixed pitch font, but the candidate is a
               variable pitch font. */
            Penalty += 15000;
        }
    }
    if (Byte == VARIABLE_PITCH)
    {
        if (!(TM->tmPitchAndFamily & _TMPF_VARIABLE_PITCH))
        {
            /* PitchVariable Penalty 350 */
            /* Requested a variable pitch font, but the candidate is not a
               variable pitch font. */
            Penalty += 350;
        }
    }

    Byte = (LogFont->lfPitchAndFamily & 0x0F);
    if (Byte == DEFAULT_PITCH)
    {
        if (!(TM->tmPitchAndFamily & _TMPF_VARIABLE_PITCH))
        {
            /* DefaultPitchFixed Penalty 1 */
            /* Requested DEFAULT_PITCH, but the candidate is fixed pitch. */
            Penalty += 1;
        }
    }

    if (RequestedNameW->Buffer[0])
    {
        BOOL Found = FALSE;
        FT_Face Face = FontGDI->SharedFace->Face;

        /* localized family name */
        if (!Found)
        {
            Status = IntGetFontLocalizedName(ActualNameW, Face, TT_NAME_ID_FONT_FAMILY,
                                             gusLanguageID);
            if (NT_SUCCESS(Status))
            {
                Found = RtlEqualUnicodeString(RequestedNameW, ActualNameW, TRUE);
            }
        }
        /* localized full name */
        if (!Found)
        {
            Status = IntGetFontLocalizedName(ActualNameW, Face, TT_NAME_ID_FULL_NAME,
                                             gusLanguageID);
            if (NT_SUCCESS(Status))
            {
                Found = RtlEqualUnicodeString(RequestedNameW, ActualNameW, TRUE);
            }
        }
        if (gusLanguageID != gusEnglishUS)
        {
            /* English family name */
            if (!Found)
            {
                Status = IntGetFontLocalizedName(ActualNameW, Face, TT_NAME_ID_FONT_FAMILY,
                                                 gusEnglishUS);
                if (NT_SUCCESS(Status))
                {
                    Found = RtlEqualUnicodeString(RequestedNameW, ActualNameW, TRUE);
                }
            }
            /* English full name */
            if (!Found)
            {
                Status = IntGetFontLocalizedName(ActualNameW, Face, TT_NAME_ID_FULL_NAME,
                                                 gusEnglishUS);
                if (NT_SUCCESS(Status))
                {
                    Found = RtlEqualUnicodeString(RequestedNameW, ActualNameW, TRUE);
                }
            }
        }
        if (!Found)
        {
            /* FaceName Penalty 10000 */
            /* Requested a face name, but the candidate's face name
               does not match. */
            Penalty += 10000;
        }
    }

    Byte = (LogFont->lfPitchAndFamily & 0xF0);
    if (Byte != FF_DONTCARE)
    {
        if (Byte != (TM->tmPitchAndFamily & 0xF0))
        {
            /* Family Penalty 9000 */
            /* Requested a family, but the candidate's family is different. */
            Penalty += 9000;
        }
        if ((TM->tmPitchAndFamily & 0xF0) == FF_DONTCARE)
        {
            /* FamilyUnknown Penalty 8000 */
            /* Requested a family, but the candidate has no family. */
            Penalty += 8000;
        }
    }

    /* Is the candidate a non-vector font? */
    if (!(TM->tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_VECTOR)))
    {
        /* Is lfHeight specified? */
        if (LogFont->lfHeight != 0)
        {
            if (labs(LogFont->lfHeight) < TM->tmHeight)
            {
                /* HeightBigger Penalty 600 */
                /* The candidate is a nonvector font and is bigger than the
                   requested height. */
                Penalty += 600;
                /* HeightBiggerDifference Penalty 150 */
                /* The candidate is a raster font and is larger than the
                   requested height. Penalty * height difference */
                Penalty += 150 * labs(TM->tmHeight - labs(LogFont->lfHeight));

                fNeedScaling = TRUE;
            }
            if (TM->tmHeight < labs(LogFont->lfHeight))
            {
                /* HeightSmaller Penalty 150 */
                /* The candidate is a raster font and is smaller than the
                   requested height. Penalty * height difference */
                Penalty += 150 * labs(TM->tmHeight - labs(LogFont->lfHeight));

                fNeedScaling = TRUE;
            }
        }
    }

    switch (LogFont->lfPitchAndFamily & 0xF0)
    {
        case FF_ROMAN: case FF_MODERN: case FF_SWISS:
            switch (TM->tmPitchAndFamily & 0xF0)
            {
                case FF_DECORATIVE: case FF_SCRIPT:
                    /* FamilyUnlikely Penalty 50 */
                    /* Requested a roman/modern/swiss family, but the
                       candidate is decorative/script. */
                    Penalty += 50;
                    break;
                default:
                    break;
            }
            break;
        case FF_DECORATIVE: case FF_SCRIPT:
            switch (TM->tmPitchAndFamily & 0xF0)
            {
                case FF_ROMAN: case FF_MODERN: case FF_SWISS:
                    /* FamilyUnlikely Penalty 50 */
                    /* Or requested decorative/script, and the candidate is
                       roman/modern/swiss. */
                    Penalty += 50;
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    if (LogFont->lfWidth != 0)
    {
        if (LogFont->lfWidth != TM->tmAveCharWidth)
        {
            /* Width Penalty 50 */
            /* Requested a nonzero width, but the candidate's width
               doesn't match. Penalty * width difference */
            Penalty += 50 * labs(LogFont->lfWidth - TM->tmAveCharWidth);

            if (!(TM->tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_VECTOR)))
                fNeedScaling = TRUE;
        }
    }

    if (fNeedScaling)
    {
        /* SizeSynth Penalty 50 */
        /* The candidate is a raster font that needs scaling by GDI. */
        Penalty += 50;
    }

    if (!!LogFont->lfItalic != !!TM->tmItalic)
    {
        if (!LogFont->lfItalic && ItalicFromStyle(style_name))
        {
            /* Italic Penalty 4 */
            /* Requested font and candidate font do not agree on italic status,
               and the desired result cannot be simulated. */
            /* Adjusted to 40 to satisfy (Oblique Penalty > Book Penalty). */
            Penalty += 40;
        }
        else if (LogFont->lfItalic && !ItalicFromStyle(style_name))
        {
            /* ItalicSim Penalty 1 */
            /* Requested italic font but the candidate is not italic,
               although italics can be simulated. */
            Penalty += 1;
        }
    }

    if (LogFont->lfOutPrecision == OUT_TT_PRECIS)
    {
        if (!(TM->tmPitchAndFamily & TMPF_TRUETYPE))
        {
            /* NotTrueType Penalty 4 */
            /* Requested OUT_TT_PRECIS, but the candidate is not a
               TrueType font. */
            Penalty += 4;
        }
    }

    Long = LogFont->lfWeight;
    if (LogFont->lfWeight == FW_DONTCARE)
        Long = FW_NORMAL;
    if (Long != TM->tmWeight)
    {
        /* Weight Penalty 3 */
        /* The candidate's weight does not match the requested weight. 
           Penalty * (weight difference/10) */
        Penalty += 3 * (labs(Long - TM->tmWeight) / 10);
    }

    if (!LogFont->lfUnderline && TM->tmUnderlined)
    {
        /* Underline Penalty 3 */
        /* Requested font has no underline, but the candidate is
           underlined. */
        Penalty += 3;
    }

    if (!LogFont->lfStrikeOut && TM->tmStruckOut)
    {
        /* StrikeOut Penalty 3 */
        /* Requested font has no strike-out, but the candidate is
           struck out. */
        Penalty += 3;
    }

    /* Is the candidate a non-vector font? */
    if (!(TM->tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_VECTOR)))
    {
        if (LogFont->lfHeight != 0 && TM->tmHeight < LogFont->lfHeight)
        {
            /* VectorHeightSmaller Penalty 2 */
            /* Candidate is a vector font that is smaller than the
               requested height. Penalty * height difference */
            Penalty += 2 * labs(TM->tmHeight - LogFont->lfHeight);
        }
        if (LogFont->lfHeight != 0 && TM->tmHeight > LogFont->lfHeight)
        {
            /* VectorHeightBigger Penalty 1 */
            /* Candidate is a vector font that is bigger than the
               requested height. Penalty * height difference */
            Penalty += 1 * labs(TM->tmHeight - LogFont->lfHeight);
        }
    }

    if (!(TM->tmPitchAndFamily & TMPF_DEVICE))
    {
        /* DeviceFavor Penalty 2 */
        /* Extra penalty for all nondevice fonts. */
        Penalty += 2;
    }

    if (Penalty < 200)
    {
        DPRINT("WARNING: Penalty:%ld < 200: RequestedNameW:%ls, "
            "ActualNameW:%ls, lfCharSet:%d, lfWeight:%ld, "
            "tmCharSet:%d, tmWeight:%ld\n",
            Penalty, RequestedNameW->Buffer, ActualNameW->Buffer,
            LogFont->lfCharSet, LogFont->lfWeight,
            TM->tmCharSet, TM->tmWeight);
    }

    return Penalty;     /* success */
}

static __inline VOID
FindBestFontFromList(FONTOBJ **FontObj, ULONG *MatchPenalty, LOGFONTW *LogFont,
                     PUNICODE_STRING pRequestedNameW,
                     PUNICODE_STRING pActualNameW, BYTE RequestedCharSet,
                     PLIST_ENTRY Head)
{
    ULONG Penalty;
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PFONT_ENTRY CurrentEntry;
    FONTGDI *FontGDI;
    ANSI_STRING ActualNameA;
    UNICODE_STRING ActualNameW, FullFaceNameW;
    OUTLINETEXTMETRICW *Otm = NULL;
    UINT OtmSize, OldOtmSize = 0;
    TEXTMETRICW *TM;
    FT_Face Face;
    LPBYTE pb;

    ASSERT(FontObj);
    ASSERT(MatchPenalty);
    ASSERT(LogFont);
    ASSERT(pRequestedNameW);
    ASSERT(Head);

    /* get the FontObj of lowest penalty */
    Entry = Head->Flink;
    while (Entry != Head)
    {
        CurrentEntry = CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);
        FontGDI = CurrentEntry->Font;
        ASSERT(FontGDI);
        Face = FontGDI->SharedFace->Face;

        /* create actual name */
        RtlInitAnsiString(&ActualNameA, Face->family_name);
        Status = RtlAnsiStringToUnicodeString(&ActualNameW, &ActualNameA, TRUE);
        if (!NT_SUCCESS(Status))
        {
            /* next entry */
            Entry = Entry->Flink;
            continue;
        }

        /* get text metrics */
        OtmSize = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
        if (OtmSize > OldOtmSize)
        {
            if (Otm)
                ExFreePoolWithTag(Otm, GDITAG_TEXT);
            Otm = ExAllocatePoolWithTag(PagedPool, OtmSize, GDITAG_TEXT);
        }

        /* update FontObj if lowest penalty */
        if (Otm)
        {
            IntGetOutlineTextMetrics(FontGDI, OtmSize, Otm);
            TM = &Otm->otmTextMetrics;
            OldOtmSize = OtmSize;

            /* create full name */
            pb = (LPBYTE)Otm + (WORD)(DWORD_PTR)Otm->otmpFullName;
            Status = RtlCreateUnicodeString(&FullFaceNameW, (LPWSTR)pb);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeUnicodeString(&ActualNameW);
                RtlFreeUnicodeString(&FullFaceNameW);
                /* next entry */
                Entry = Entry->Flink;
                continue;
            }

            Penalty = GetFontPenalty(LogFont, pRequestedNameW, &ActualNameW,
                                     &FullFaceNameW, RequestedCharSet,
                                     FontGDI, Otm, TM, Face->style_name);
            if (*MatchPenalty == 0xFFFFFFFF || Penalty < *MatchPenalty)
            {
                DPRINT("%ls Penalty: %lu\n", FullFaceNameW.Buffer, Penalty);
                RtlFreeUnicodeString(pActualNameW);
                RtlCreateUnicodeString(pActualNameW, ActualNameW.Buffer);

                *FontObj = GDIToObj(FontGDI, FONT);
                *MatchPenalty = Penalty;
            }

            RtlFreeUnicodeString(&FullFaceNameW);
        }

        /* free strings */
        RtlFreeUnicodeString(&ActualNameW);

        /* next entry */
        Entry = Entry->Flink;
    }

    if (Otm)
        ExFreePoolWithTag(Otm, GDITAG_TEXT);
}

static
VOID
FASTCALL
IntFontType(PFONTGDI Font)
{
    PS_FontInfoRec psfInfo;
    FT_ULong tmp_size = 0;
    FT_Face Face = Font->SharedFace->Face;

    if (FT_HAS_MULTIPLE_MASTERS(Face))
        Font->FontObj.flFontType |= FO_MULTIPLEMASTER;
    if (FT_HAS_VERTICAL(Face))
        Font->FontObj.flFontType |= FO_VERT_FACE;
    if (!FT_IS_SCALABLE(Face))
        Font->FontObj.flFontType |= FO_TYPE_RASTER;
    if (FT_IS_SFNT(Face))
    {
        Font->FontObj.flFontType |= FO_TYPE_TRUETYPE;
        if (FT_Get_Sfnt_Table(Face, ft_sfnt_post))
            Font->FontObj.flFontType |= FO_POSTSCRIPT;
    }
    if (!FT_Get_PS_Font_Info(Face, &psfInfo ))
    {
        Font->FontObj.flFontType |= FO_POSTSCRIPT;
    }
    /* Check for the presence of the 'CFF ' table to check if the font is Type1 */
    if (!FT_Load_Sfnt_Table(Face, TTAG_CFF, 0, NULL, &tmp_size))
    {
        Font->FontObj.flFontType |= (FO_CFF|FO_POSTSCRIPT);
    }
}

NTSTATUS
FASTCALL
TextIntRealizeFont(HFONT FontHandle, PTEXTOBJ pTextObj)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTEXTOBJ TextObj;
    UNICODE_STRING ActualNameW, RequestedNameW;
    PPROCESSINFO Win32Process;
    ULONG MatchPenalty;
    LOGFONTW *pLogFont;
    FT_Face Face;
    BYTE RequestedCharSet;

    if (!pTextObj)
    {
        TextObj = TEXTOBJ_LockText(FontHandle);
        if (NULL == TextObj)
        {
            return STATUS_INVALID_HANDLE;
        }

        if (TextObj->fl & TEXTOBJECT_INIT)
        {
            TEXTOBJ_UnlockText(TextObj);
            return STATUS_SUCCESS;
        }
    }
    else
    {
        TextObj = pTextObj;
    }

    RtlInitUnicodeString(&ActualNameW, NULL);

    pLogFont = &TextObj->logfont.elfEnumLogfontEx.elfLogFont;
    if (!RtlCreateUnicodeString(&RequestedNameW, pLogFont->lfFaceName))
    {
        if (!pTextObj) TEXTOBJ_UnlockText(TextObj);
        return STATUS_NO_MEMORY;
    }

    /* substitute */
    RequestedCharSet = pLogFont->lfCharSet;
    DPRINT("Font '%ls,%u' is substituted by: ",
           RequestedNameW.Buffer, RequestedCharSet);
    SubstituteFontRecurse(&RequestedNameW, &RequestedCharSet);
    DPRINT("'%ls,%u'.\n", RequestedNameW.Buffer, RequestedCharSet);

    MatchPenalty = 0xFFFFFFFF;
    TextObj->Font = NULL;

    Win32Process = PsGetCurrentProcessWin32Process();

    /* Search private fonts */
    IntLockProcessPrivateFonts(Win32Process);
    FindBestFontFromList(&TextObj->Font, &MatchPenalty, pLogFont,
                         &RequestedNameW, &ActualNameW, RequestedCharSet,
                         &Win32Process->PrivateFontListHead);
    IntUnLockProcessPrivateFonts(Win32Process);

    /* Search system fonts */
    IntLockGlobalFonts;
    FindBestFontFromList(&TextObj->Font, &MatchPenalty, pLogFont,
                         &RequestedNameW, &ActualNameW, RequestedCharSet,
                         &FontListHead);
    IntUnLockGlobalFonts;

    if (NULL == TextObj->Font)
    {
        DPRINT1("Request font %S not found, no fonts loaded at all\n",
                pLogFont->lfFaceName);
        Status = STATUS_NOT_FOUND;
    }
    else
    {
        PFONTGDI FontGdi = ObjToGDI(TextObj->Font, FONT);
        // Need hdev, when freetype is loaded need to create DEVOBJ for
        // Consumer and Producer.
        TextObj->Font->iUniq = 1; // Now it can be cached.
        IntFontType(FontGdi);
        FontGdi->flType = TextObj->Font->flFontType;
        FontGdi->RequestUnderline = pLogFont->lfUnderline ? 0xFF : 0;
        FontGdi->RequestStrikeOut = pLogFont->lfStrikeOut ? 0xFF : 0;
        FontGdi->RequestItalic = pLogFont->lfItalic ? 0xFF : 0;
        if (pLogFont->lfWeight != FW_DONTCARE)
            FontGdi->RequestWeight = pLogFont->lfWeight;
        else
            FontGdi->RequestWeight = FW_NORMAL;

        Face = FontGdi->SharedFace->Face;

        //FontGdi->OriginalWeight = WeightFromStyle(Face->style_name);

        if (!FontGdi->OriginalItalic)
            FontGdi->OriginalItalic = ItalicFromStyle(Face->style_name);

        TextObj->fl |= TEXTOBJECT_INIT;
        Status = STATUS_SUCCESS;

        DPRINT("RequestedNameW: %ls (CharSet: %d) -> ActualNameW: %ls (CharSet: %d)\n",
            RequestedNameW.Buffer, pLogFont->lfCharSet,
            ActualNameW.Buffer, FontGdi->CharSet);
    }

    RtlFreeUnicodeString(&RequestedNameW);
    RtlFreeUnicodeString(&ActualNameW);
    if (!pTextObj) TEXTOBJ_UnlockText(TextObj);

    ASSERT((NT_SUCCESS(Status) ^ (NULL == TextObj->Font)) != 0);

    return Status;
}


static
BOOL
FASTCALL
IntGetFullFileName(
    POBJECT_NAME_INFORMATION NameInfo,
    ULONG Size,
    PUNICODE_STRING FileName)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hFile;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG Desired;

    InitializeObjectAttributes(&ObjectAttributes,
                               FileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenFile(
                 &hFile,
                 0, // FILE_READ_ATTRIBUTES,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 0);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwOpenFile() failed (Status = 0x%lx)\n", Status);
        return FALSE;
    }

    Status = ZwQueryObject(hFile, ObjectNameInformation, NameInfo, Size, &Desired);
    ZwClose(hFile);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwQueryObject() failed (Status = %lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}

static BOOL
EqualFamilyInfo(FONTFAMILYINFO *pInfo1, FONTFAMILYINFO *pInfo2)
{
    UNICODE_STRING Str1, Str2;
    ENUMLOGFONTEXW *pLog1 = &pInfo1->EnumLogFontEx;
    ENUMLOGFONTEXW *pLog2 = &pInfo2->EnumLogFontEx;
    RtlInitUnicodeString(&Str1, pLog1->elfLogFont.lfFaceName);
    RtlInitUnicodeString(&Str2, pLog2->elfLogFont.lfFaceName);
    if (!RtlEqualUnicodeString(&Str1, &Str2, TRUE))
    {
        return FALSE;
    }
    if ((pLog1->elfStyle != NULL) != (pLog2->elfStyle != NULL))
        return FALSE;
    if (pLog1->elfStyle != NULL)
    {
        RtlInitUnicodeString(&Str1, pLog1->elfStyle);
        RtlInitUnicodeString(&Str2, pLog2->elfStyle);
        if (!RtlEqualUnicodeString(&Str1, &Str2, TRUE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static VOID
IntAddNameFromFamInfo(LPWSTR psz, FONTFAMILYINFO *FamInfo)
{
    wcscat(psz, FamInfo->EnumLogFontEx.elfLogFont.lfFaceName);
    if (FamInfo->EnumLogFontEx.elfStyle[0] &&
        _wcsicmp(FamInfo->EnumLogFontEx.elfStyle, L"Regular") != 0)
    {
        wcscat(psz, L" ");
        wcscat(psz, FamInfo->EnumLogFontEx.elfStyle);
    }
}

BOOL
FASTCALL
IntGdiGetFontResourceInfo(
    PUNICODE_STRING FileName,
    PVOID pBuffer,
    DWORD *pdwBytes,
    DWORD dwType)
{
    UNICODE_STRING EntryFileName;
    POBJECT_NAME_INFORMATION NameInfo1, NameInfo2;
    PLIST_ENTRY ListEntry;
    PFONT_ENTRY FontEntry;
    ULONG Size, i, Count;
    LPBYTE pbBuffer;
    BOOL IsEqual;
    FONTFAMILYINFO *FamInfo;
    const ULONG MaxFamInfo = 64;
    BOOL bSuccess;

    DPRINT("IntGdiGetFontResourceInfo: dwType == %lu\n", dwType);

    /* Create buffer for full path name */
    Size = sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR);
    NameInfo1 = ExAllocatePoolWithTag(PagedPool, Size, TAG_FINF);
    if (!NameInfo1)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Get the full path name */
    if (!IntGetFullFileName(NameInfo1, Size, FileName))
    {
        ExFreePoolWithTag(NameInfo1, TAG_FINF);
        return FALSE;
    }

    /* Create a buffer for the entries' names */
    NameInfo2 = ExAllocatePoolWithTag(PagedPool, Size, TAG_FINF);
    if (!NameInfo2)
    {
        ExFreePoolWithTag(NameInfo1, TAG_FINF);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    FamInfo = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(FONTFAMILYINFO) * MaxFamInfo,
                                    TAG_FINF);
    if (!FamInfo)
    {
        ExFreePoolWithTag(NameInfo2, TAG_FINF);
        ExFreePoolWithTag(NameInfo1, TAG_FINF);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    /* Try to find the pathname in the global font list */
    Count = 0;
    IntLockGlobalFonts;
    for (ListEntry = FontListHead.Flink; ListEntry != &FontListHead;
         ListEntry = ListEntry->Flink)
    {
        FontEntry = CONTAINING_RECORD(ListEntry, FONT_ENTRY, ListEntry);
        if (FontEntry->Font->Filename == NULL)
            continue;

        RtlInitUnicodeString(&EntryFileName , FontEntry->Font->Filename);
        if (!IntGetFullFileName(NameInfo2, Size, &EntryFileName))
            continue;

        if (!RtlEqualUnicodeString(&NameInfo1->Name, &NameInfo2->Name, FALSE))
            continue;

        IsEqual = FALSE;
        FontFamilyFillInfo(&FamInfo[Count], FontEntry->FaceName.Buffer,
                           NULL, FontEntry->Font);
        for (i = 0; i < Count; ++i)
        {
            if (EqualFamilyInfo(&FamInfo[i], &FamInfo[Count]))
            {
                IsEqual = TRUE;
                break;
            }
        }
        if (!IsEqual)
        {
            /* Found */
            ++Count;
            if (Count >= MaxFamInfo)
                break;
        }
    }
    IntUnLockGlobalFonts;

    /* Free the buffers */
    ExFreePoolWithTag(NameInfo1, TAG_FINF);
    ExFreePool(NameInfo2);

    if (Count == 0 && dwType != 5)
    {
        /* Font could not be found in system table
           dwType == 5 will still handle this */
        ExFreePoolWithTag(FamInfo, TAG_FINF);
        return FALSE;
    }

    bSuccess = FALSE;
    switch (dwType)
    {
    case 0: /* FIXME: Returns 1 or 2, don't know what this is atm */
        Size = sizeof(DWORD);
        if (*pdwBytes == 0)
        {
            *pdwBytes = Size;
            bSuccess = TRUE;
        }
        else if (pBuffer)
        {
            if (*pdwBytes >= Size)
            {
                *(DWORD*)pBuffer = Count;
            }
            *pdwBytes = Size;
            bSuccess = TRUE;
        }
        break;

    case 1: /* copy the font title */
        /* calculate the required size */
        Size = 0;
        Size += wcslen(FamInfo[0].EnumLogFontEx.elfLogFont.lfFaceName);
        if (FamInfo[0].EnumLogFontEx.elfStyle[0] &&
            _wcsicmp(FamInfo[0].EnumLogFontEx.elfStyle, L"Regular") != 0)
        {
            Size += 1 + wcslen(FamInfo[0].EnumLogFontEx.elfStyle);
        }
        for (i = 1; i < Count; ++i)
        {
            Size += 3;  /* " & " */
            Size += wcslen(FamInfo[i].EnumLogFontEx.elfLogFont.lfFaceName);
            if (FamInfo[i].EnumLogFontEx.elfStyle[0] &&
                _wcsicmp(FamInfo[i].EnumLogFontEx.elfStyle, L"Regular") != 0)
            {
                Size += 1 + wcslen(FamInfo[i].EnumLogFontEx.elfStyle);
            }
        }
        Size += 2;  /* "\0\0" */
        Size *= sizeof(WCHAR);

        if (*pdwBytes == 0)
        {
            *pdwBytes = Size;
            bSuccess = TRUE;
        }
        else if (pBuffer)
        {
            if (*pdwBytes >= Size)
            {
                /* store font title to buffer */
                WCHAR *psz = pBuffer;
                *psz = 0;
                IntAddNameFromFamInfo(psz, &FamInfo[0]);
                for (i = 1; i < Count; ++i)
                {
                    wcscat(psz, L" & ");
                    IntAddNameFromFamInfo(psz, &FamInfo[i]);
                }
                psz[wcslen(psz) + 1] = UNICODE_NULL;
                *pdwBytes = Size;
                bSuccess = TRUE;
            }
            else
            {
                *pdwBytes = 1024;       /* this is confirmed value */
            }
        }
        break;

    case 2: /* Copy an array of LOGFONTW */
        Size = Count * sizeof(LOGFONTW);
        if (*pdwBytes == 0)
        {
            *pdwBytes = Size;
            bSuccess = TRUE;
        }
        else if (pBuffer)
        {
            if (*pdwBytes >= Size)
            {
                pbBuffer = (LPBYTE)pBuffer;
                for (i = 0; i < Count; ++i)
                {
                    FamInfo[i].EnumLogFontEx.elfLogFont.lfWidth = 0;
                    RtlCopyMemory(pbBuffer, &FamInfo[i].EnumLogFontEx.elfLogFont, sizeof(LOGFONTW));
                    pbBuffer += sizeof(LOGFONTW);
                }
            }
            *pdwBytes = Size;
            bSuccess = TRUE;
        }
        else
        {
            *pdwBytes = 1024;       /* this is confirmed value */
        }
        break;

    case 3:
        Size = sizeof(DWORD);
        if (*pdwBytes == 0)
        {
            *pdwBytes = Size;
            bSuccess = TRUE;
        }
        else if (pBuffer)
        {
            if (*pdwBytes >= Size)
            {
                /* FIXME: What exactly is copied here? */
                *(DWORD*)pBuffer = 1;
            }
            *pdwBytes = Size;
            bSuccess = TRUE;
        }
        break;

    case 4:     /* full file path */
        if (FileName->Length >= 4 * sizeof(WCHAR))
        {
            /* The beginning of FileName is \??\ */
            LPWSTR pch = FileName->Buffer + 4;
            DWORD Length = FileName->Length - 4 * sizeof(WCHAR);

            Size = Length + sizeof(WCHAR);
            if (*pdwBytes == 0)
            {
                *pdwBytes = Size;
                bSuccess = TRUE;
            }
            else if (pBuffer)
            {
                if (*pdwBytes >= Size)
                {
                    RtlCopyMemory(pBuffer, pch, Size);
                }
                *pdwBytes = Size;
                bSuccess = TRUE;
            }
        }
        break;

    case 5: /* Looks like a BOOL that is copied, TRUE, if the font was not found */
        Size = sizeof(BOOL);
        if (*pdwBytes == 0)
        {
            *pdwBytes = Size;
            bSuccess = TRUE;
        }
        else if (pBuffer)
        {
            if (*pdwBytes >= Size)
            {
                *(BOOL*)pBuffer = Count == 0;
            }
            *pdwBytes = Size;
            bSuccess = TRUE;
        }
        break;
    }
    ExFreePoolWithTag(FamInfo, TAG_FINF);

    return bSuccess;
}


BOOL
FASTCALL
ftGdiRealizationInfo(PFONTGDI Font, PREALIZATION_INFO Info)
{
    if (FT_HAS_FIXED_SIZES(Font->SharedFace->Face))
        Info->iTechnology = RI_TECH_BITMAP;
    else
    {
        if (FT_IS_SCALABLE(Font->SharedFace->Face))
            Info->iTechnology = RI_TECH_SCALABLE;
        else
            Info->iTechnology = RI_TECH_FIXED;
    }
    Info->iUniq = Font->FontObj.iUniq;
    Info->dwUnknown = -1;
    return TRUE;
}


DWORD
FASTCALL
ftGdiGetKerningPairs( PFONTGDI Font,
                      DWORD cPairs,
                      LPKERNINGPAIR pKerningPair)
{
    DWORD Count = 0;
    INT i = 0;
    FT_Face face = Font->SharedFace->Face;

    if (FT_HAS_KERNING(face) && face->charmap->encoding == FT_ENCODING_UNICODE)
    {
        FT_UInt previous_index = 0, glyph_index = 0;
        FT_ULong char_code, char_previous;
        FT_Vector delta;

        char_previous = char_code = FT_Get_First_Char(face, &glyph_index);

        IntLockFreeType;

        while (glyph_index)
        {
            if (previous_index && glyph_index)
            {
                FT_Get_Kerning(face, previous_index, glyph_index, FT_KERNING_DEFAULT, &delta);

                if (pKerningPair && cPairs)
                {
                    pKerningPair[i].wFirst      = char_previous;
                    pKerningPair[i].wSecond     = char_code;
                    pKerningPair[i].iKernAmount = delta.x;
                    i++;
                    if (i == cPairs) break;
                }
                Count++;
            }
            previous_index = glyph_index;
            char_previous = char_code;
            char_code = FT_Get_Next_Char(face, char_code, &glyph_index);
        }
        IntUnLockFreeType;
    }
    return Count;
}


///////////////////////////////////////////////////////////////////////////
//
// Functions needing sorting.
//
///////////////////////////////////////////////////////////////////////////
int APIENTRY
NtGdiGetFontFamilyInfo(HDC Dc,
                       LPLOGFONTW UnsafeLogFont,
                       PFONTFAMILYINFO UnsafeInfo,
                       DWORD Size)
{
    NTSTATUS Status;
    LOGFONTW LogFont;
    PFONTFAMILYINFO Info;
    DWORD Count;
    PPROCESSINFO Win32Process;

    /* Make a safe copy */
    Status = MmCopyFromCaller(&LogFont, UnsafeLogFont, sizeof(LOGFONTW));
    if (! NT_SUCCESS(Status))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* Allocate space for a safe copy */
    Info = ExAllocatePoolWithTag(PagedPool, Size * sizeof(FONTFAMILYINFO), GDITAG_TEXT);
    if (NULL == Info)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    /* Enumerate font families in the global list */
    IntLockGlobalFonts;
    Count = 0;
    if (! GetFontFamilyInfoForList(&LogFont, Info, &Count, Size, &FontListHead) )
    {
        IntUnLockGlobalFonts;
        ExFreePoolWithTag(Info, GDITAG_TEXT);
        return -1;
    }
    IntUnLockGlobalFonts;

    /* Enumerate font families in the process local list */
    Win32Process = PsGetCurrentProcessWin32Process();
    IntLockProcessPrivateFonts(Win32Process);
    if (! GetFontFamilyInfoForList(&LogFont, Info, &Count, Size,
                                   &Win32Process->PrivateFontListHead))
    {
        IntUnLockProcessPrivateFonts(Win32Process);
        ExFreePoolWithTag(Info, GDITAG_TEXT);
        return -1;
    }
    IntUnLockProcessPrivateFonts(Win32Process);

    /* Enumerate font families in the registry */
    if (! GetFontFamilyInfoForSubstitutes(&LogFont, Info, &Count, Size))
    {
        ExFreePoolWithTag(Info, GDITAG_TEXT);
        return -1;
    }

    /* Return data to caller */
    if (0 != Count)
    {
        Status = MmCopyToCaller(UnsafeInfo, Info,
                                (Count < Size ? Count : Size) * sizeof(FONTFAMILYINFO));
        if (! NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(Info, GDITAG_TEXT);
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return -1;
        }
    }

    ExFreePoolWithTag(Info, GDITAG_TEXT);

    return Count;
}

FORCEINLINE
LONG
ScaleLong(LONG lValue, PFLOATOBJ pef)
{
    FLOATOBJ efTemp;

    /* Check if we have scaling different from 1 */
    if (!FLOATOBJ_Equal(pef, (PFLOATOBJ)&gef1))
    {
        /* Need to multiply */
        FLOATOBJ_SetLong(&efTemp, lValue);
        FLOATOBJ_Mul(&efTemp, pef);
        lValue = FLOATOBJ_GetLong(&efTemp);
    }

    return lValue;
}

BOOL
APIENTRY
GreExtTextOutW(
    IN HDC hDC,
    IN INT XStart,
    IN INT YStart,
    IN UINT fuOptions,
    IN OPTIONAL PRECTL lprc,
    IN LPCWSTR String,
    IN INT Count,
    IN OPTIONAL LPINT Dx,
    IN DWORD dwCodePage)
{
    /*
     * FIXME:
     * Call EngTextOut, which does the real work (calling DrvTextOut where
     * appropriate)
     */

    DC *dc;
    PDC_ATTR pdcattr;
    SURFOBJ *SurfObj;
    SURFACE *psurf = NULL;
    int error, glyph_index, i;
    FT_Face face;
    FT_GlyphSlot glyph;
    FT_BitmapGlyph realglyph;
    LONGLONG TextLeft, RealXStart;
    ULONG TextTop, previous, BackgroundLeft;
    FT_Bool use_kerning;
    RECTL DestRect, MaskRect;
    POINTL SourcePoint, BrushOrigin;
    HBITMAP HSourceGlyph;
    SURFOBJ *SourceGlyphSurf;
    SIZEL bitSize;
    INT yoff;
    FONTOBJ *FontObj;
    PFONTGDI FontGDI;
    PTEXTOBJ TextObj = NULL;
    EXLATEOBJ exloRGB2Dst, exloDst2RGB;
    FT_Render_Mode RenderMode;
    BOOLEAN Render;
    POINT Start;
    BOOL DoBreak = FALSE;
    USHORT DxShift;
    PMATRIX pmxWorldToDevice;
    LONG fixAscender, fixDescender;
    FLOATOBJ Scale;
    LOGFONTW *plf;
    BOOL EmuBold, EmuItalic;
    int thickness;

    // TODO: Write test-cases to exactly match real Windows in different
    // bad parameters (e.g. does Windows check the DC or the RECT first?).
    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    pdcattr = dc->pdcattr;

    if ((fuOptions & ETO_OPAQUE) || pdcattr->jBkMode == OPAQUE)
    {
        if (pdcattr->ulDirty_ & DIRTY_BACKGROUND)
            DC_vUpdateBackgroundBrush(dc);
    }

    /* Check if String is valid */
    if ((Count > 0xFFFF) || (Count > 0 && String == NULL))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto fail;
    }

    DxShift = fuOptions & ETO_PDY ? 1 : 0;

    if (PATH_IsPathOpen(dc->dclevel))
    {
        if (!PATH_ExtTextOut( dc,
                              XStart,
                              YStart,
                              fuOptions,
                              (const RECTL *)lprc,
                              String,
                              Count,
                              (const INT *)Dx)) goto fail;
        goto good;
    }

    if (lprc && (fuOptions & (ETO_OPAQUE | ETO_CLIPPED)))
    {
        IntLPtoDP(dc, (POINT *)lprc, 2);
    }

    if (pdcattr->lTextAlign & TA_UPDATECP)
    {
        Start.x = pdcattr->ptlCurrent.x;
        Start.y = pdcattr->ptlCurrent.y;
    } else {
        Start.x = XStart;
        Start.y = YStart;
    }

    IntLPtoDP(dc, &Start, 1);
    RealXStart = ((LONGLONG)Start.x + dc->ptlDCOrig.x) << 6;
    YStart = Start.y + dc->ptlDCOrig.y;

    SourcePoint.x = 0;
    SourcePoint.y = 0;
    MaskRect.left = 0;
    MaskRect.top = 0;
    BrushOrigin.x = 0;
    BrushOrigin.y = 0;

    if (!dc->dclevel.pSurface)
    {
        goto fail;
    }

    if ((fuOptions & ETO_OPAQUE) && lprc)
    {
        DestRect.left   = lprc->left;
        DestRect.top    = lprc->top;
        DestRect.right  = lprc->right;
        DestRect.bottom = lprc->bottom;

        DestRect.left   += dc->ptlDCOrig.x;
        DestRect.top    += dc->ptlDCOrig.y;
        DestRect.right  += dc->ptlDCOrig.x;
        DestRect.bottom += dc->ptlDCOrig.y;

        if (dc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
        {
           IntUpdateBoundsRect(dc, &DestRect);
        }

        DC_vPrepareDCsForBlit(dc, &DestRect, NULL, NULL);

        if (pdcattr->ulDirty_ & DIRTY_BACKGROUND)
            DC_vUpdateBackgroundBrush(dc);

        psurf = dc->dclevel.pSurface;
        IntEngBitBlt(
            &psurf->SurfObj,
            NULL,
            NULL,
            &dc->co.ClipObj,
            NULL,
            &DestRect,
            &SourcePoint,
            &SourcePoint,
            &dc->eboBackground.BrushObject,
            &BrushOrigin,
            ROP4_FROM_INDEX(R3_OPINDEX_PATCOPY));
        fuOptions &= ~ETO_OPAQUE;
        DC_vFinishBlit(dc, NULL);
    }
    else
    {
        if (pdcattr->jBkMode == OPAQUE)
        {
            fuOptions |= ETO_OPAQUE;
        }
    }

    TextObj = RealizeFontInit(pdcattr->hlfntNew);
    if (TextObj == NULL)
    {
        goto fail;
    }

    FontObj = TextObj->Font;
    ASSERT(FontObj);
    FontGDI = ObjToGDI(FontObj, FONT);
    ASSERT(FontGDI);

    IntLockFreeType;
    face = FontGDI->SharedFace->Face;

    plf = &TextObj->logfont.elfEnumLogfontEx.elfLogFont;
    EmuBold = (plf->lfWeight >= FW_BOLD && FontGDI->OriginalWeight <= FW_NORMAL);
    EmuItalic = (plf->lfItalic && !FontGDI->OriginalItalic);

    Render = IntIsFontRenderingEnabled();
    if (Render)
        RenderMode = IntGetFontRenderMode(plf);
    else
        RenderMode = FT_RENDER_MODE_MONO;

    if (!TextIntUpdateSize(dc, TextObj, FontGDI, FALSE))
    {
        IntUnLockFreeType;
        goto fail;
    }

    if (dc->pdcattr->iGraphicsMode == GM_ADVANCED)
    {
        pmxWorldToDevice = DC_pmxWorldToDevice(dc);
        FtSetCoordinateTransform(face, pmxWorldToDevice);

        fixAscender = ScaleLong(face->size->metrics.ascender, &pmxWorldToDevice->efM22);
        fixDescender = ScaleLong(face->size->metrics.descender, &pmxWorldToDevice->efM22);
    }
    else
    {
        pmxWorldToDevice = (PMATRIX)&gmxWorldToDeviceDefault;
        FtSetCoordinateTransform(face, pmxWorldToDevice);

        fixAscender = face->size->metrics.ascender;
        fixDescender = face->size->metrics.descender;
    }

    /*
     * Process the vertical alignment and determine the yoff.
     */

    if (pdcattr->lTextAlign & TA_BASELINE)
        yoff = 0;
    else if (pdcattr->lTextAlign & TA_BOTTOM)
        yoff = -fixDescender >> 6;
    else /* TA_TOP */
        yoff = fixAscender >> 6;

    use_kerning = FT_HAS_KERNING(face);
    previous = 0;

    /*
     * Process the horizontal alignment and modify XStart accordingly.
     */

    if (pdcattr->lTextAlign & (TA_RIGHT | TA_CENTER))
    {
        ULONGLONG TextWidth = 0;
        LPCWSTR TempText = String;
        int iStart;

        /*
         * Calculate width of the text.
         */

        if (NULL != Dx)
        {
            iStart = Count < 2 ? 0 : Count - 2;
            TextWidth = Count < 2 ? 0 : (Dx[(Count-2)<<DxShift] << 6);
        }
        else
        {
            iStart = 0;
        }
        TempText = String + iStart;

        for (i = iStart; i < Count; i++)
        {
            if (fuOptions & ETO_GLYPH_INDEX)
                glyph_index = *TempText;
            else
                glyph_index = FT_Get_Char_Index(face, *TempText);

            if (EmuBold || EmuItalic)
                realglyph = NULL;
            else
                realglyph = ftGdiGlyphCacheGet(face, glyph_index,
                                               plf->lfHeight, pmxWorldToDevice);
            if (!realglyph)
            {
                error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
                if (error)
                {
                    DPRINT1("WARNING: Failed to load and render glyph! [index: %d]\n", glyph_index);
                }

                glyph = face->glyph;
                if (EmuBold || EmuItalic)
                {
                    if (EmuBold)
                        FT_GlyphSlot_Embolden(glyph);
                    if (EmuItalic)
                        FT_GlyphSlot_Oblique(glyph);
                    realglyph = ftGdiGlyphSet(face, glyph, RenderMode);
                }
                else
                {
                    realglyph = ftGdiGlyphCacheSet(face,
                                                   glyph_index,
                                                   plf->lfHeight,
                                                   pmxWorldToDevice,
                                                   glyph,
                                                   RenderMode);
                }
                if (!realglyph)
                {
                    DPRINT1("Failed to render glyph! [index: %d]\n", glyph_index);
                    IntUnLockFreeType;
                    goto fail;
                }

            }
            /* Retrieve kerning distance */
            if (use_kerning && previous && glyph_index)
            {
                FT_Vector delta;
                FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
                TextWidth += delta.x;
            }

            TextWidth += realglyph->root.advance.x >> 10;

            if (EmuBold || EmuItalic)
            {
                FT_Done_Glyph((FT_Glyph)realglyph);
                realglyph = NULL;
            }

            previous = glyph_index;
            TempText++;
        }

        previous = 0;

        if ((pdcattr->lTextAlign & TA_CENTER) == TA_CENTER)
        {
            RealXStart -= TextWidth / 2;
        }
        else
        {
            RealXStart -= TextWidth;
        }
    }

    /* Lock blit with a dummy rect */
    DC_vPrepareDCsForBlit(dc, NULL, NULL, NULL);

    psurf = dc->dclevel.pSurface;
    SurfObj = &psurf->SurfObj ;

    EXLATEOBJ_vInitialize(&exloRGB2Dst, &gpalRGB, psurf->ppal, 0, 0, 0);
    EXLATEOBJ_vInitialize(&exloDst2RGB, psurf->ppal, &gpalRGB, 0, 0, 0);

    if ((fuOptions & ETO_OPAQUE) && (dc->pdcattr->ulDirty_ & DIRTY_BACKGROUND))
        DC_vUpdateBackgroundBrush(dc) ;

    if(dc->pdcattr->ulDirty_ & DIRTY_TEXT)
        DC_vUpdateTextBrush(dc) ;

    if (!face->units_per_EM)
    {
        thickness = 1;
    }
    else
    {
        thickness = face->underline_thickness *
            face->size->metrics.y_ppem / face->units_per_EM;
        if (thickness <= 0)
            thickness = 1;
    }

    if ((fuOptions & ETO_OPAQUE) && plf->lfItalic)
    {
        /* Draw background */
        TextLeft = RealXStart;
        TextTop = YStart;
        BackgroundLeft = (RealXStart + 32) >> 6;
        for (i = 0; i < Count; ++i)
        {
            if (fuOptions & ETO_GLYPH_INDEX)
                glyph_index = String[i];
            else
                glyph_index = FT_Get_Char_Index(face, String[i]);

            error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            if (error)
            {
                DPRINT1("Failed to load and render glyph! [index: %d]\n", glyph_index);
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }

            glyph = face->glyph;
            if (EmuBold)
                FT_GlyphSlot_Embolden(glyph);
            if (EmuItalic)
                FT_GlyphSlot_Oblique(glyph);
            realglyph = ftGdiGlyphSet(face, glyph, RenderMode);
            if (!realglyph)
            {
                DPRINT1("Failed to render glyph! [index: %d]\n", glyph_index);
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }

            /* retrieve kerning distance and move pen position */
            if (use_kerning && previous && glyph_index && NULL == Dx)
            {
                FT_Vector delta;
                FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
                TextLeft += delta.x;
            }
            DPRINT("TextLeft: %I64d\n", TextLeft);
            DPRINT("TextTop: %lu\n", TextTop);
            DPRINT("Advance: %d\n", realglyph->root.advance.x);

            DestRect.left = BackgroundLeft;
            DestRect.right = (TextLeft + (realglyph->root.advance.x >> 10) + 32) >> 6;
            DestRect.top = TextTop + yoff - ((fixAscender + 32) >> 6);
            DestRect.bottom = TextTop + yoff + ((32 - fixDescender) >> 6);
            MouseSafetyOnDrawStart(dc->ppdev, DestRect.left, DestRect.top, DestRect.right, DestRect.bottom);
            if (dc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
            {
               IntUpdateBoundsRect(dc, &DestRect);
            }
            IntEngBitBlt(
                &psurf->SurfObj,
                NULL,
                NULL,
                &dc->co.ClipObj,
                NULL,
                &DestRect,
                &SourcePoint,
                &SourcePoint,
                &dc->eboBackground.BrushObject,
                &BrushOrigin,
                ROP4_FROM_INDEX(R3_OPINDEX_PATCOPY));
            MouseSafetyOnDrawEnd(dc->ppdev);
            BackgroundLeft = DestRect.right;

            DestRect.left = ((TextLeft + 32) >> 6) + realglyph->left;
            DestRect.right = DestRect.left + realglyph->bitmap.width;
            DestRect.top = TextTop + yoff - realglyph->top;
            DestRect.bottom = DestRect.top + realglyph->bitmap.rows;

            bitSize.cx = realglyph->bitmap.width;
            bitSize.cy = realglyph->bitmap.rows;
            MaskRect.right = realglyph->bitmap.width;
            MaskRect.bottom = realglyph->bitmap.rows;

            if (NULL == Dx)
            {
                TextLeft += realglyph->root.advance.x >> 10;
                DPRINT("New TextLeft: %I64d\n", TextLeft);
            }
            else
            {
                // FIXME this should probably be a matrix transform with TextTop as well.
                Scale = pdcattr->mxWorldToDevice.efM11;
                if (FLOATOBJ_Equal0(&Scale))
                    FLOATOBJ_Set1(&Scale);

                /* do the shift before multiplying to preserve precision */
                FLOATOBJ_MulLong(&Scale, Dx[i<<DxShift] << 6); 
                TextLeft += FLOATOBJ_GetLong(&Scale);
                DPRINT("New TextLeft2: %I64d\n", TextLeft);
            }

            if (DxShift)
            {
                TextTop -= Dx[2 * i + 1] << 6;
            }

            previous = glyph_index;

            if (EmuBold || EmuItalic)
            {
                FT_Done_Glyph((FT_Glyph)realglyph);
                realglyph = NULL;
            }
        }
    }

    /*
     * The main rendering loop.
     */
    TextLeft = RealXStart;
    TextTop = YStart;
    BackgroundLeft = (RealXStart + 32) >> 6;
    for (i = 0; i < Count; ++i)
    {
        if (fuOptions & ETO_GLYPH_INDEX)
            glyph_index = String[i];
        else
            glyph_index = FT_Get_Char_Index(face, String[i]);

        if (EmuBold || EmuItalic)
            realglyph = NULL;
        else
            realglyph = ftGdiGlyphCacheGet(face, glyph_index,
                                           plf->lfHeight, pmxWorldToDevice);
        if (!realglyph)
        {
            error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            if (error)
            {
                DPRINT1("Failed to load and render glyph! [index: %d]\n", glyph_index);
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }

            glyph = face->glyph;
            if (EmuBold || EmuItalic)
            {
                if (EmuBold)
                    FT_GlyphSlot_Embolden(glyph);
                if (EmuItalic)
                    FT_GlyphSlot_Oblique(glyph);
                realglyph = ftGdiGlyphSet(face, glyph, RenderMode);
            }
            else
            {
                realglyph = ftGdiGlyphCacheSet(face,
                                               glyph_index,
                                               plf->lfHeight,
                                               pmxWorldToDevice,
                                               glyph,
                                               RenderMode);
            }
            if (!realglyph)
            {
                DPRINT1("Failed to render glyph! [index: %d]\n", glyph_index);
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }
        }

        /* retrieve kerning distance and move pen position */
        if (use_kerning && previous && glyph_index && NULL == Dx)
        {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
            TextLeft += delta.x;
        }
        DPRINT("TextLeft: %I64d\n", TextLeft);
        DPRINT("TextTop: %lu\n", TextTop);
        DPRINT("Advance: %d\n", realglyph->root.advance.x);

        if ((fuOptions & ETO_OPAQUE) && !plf->lfItalic)
        {
            DestRect.left = BackgroundLeft;
            DestRect.right = (TextLeft + (realglyph->root.advance.x >> 10) + 32) >> 6;
            DestRect.top = TextTop + yoff - ((fixAscender + 32) >> 6);
            DestRect.bottom = TextTop + yoff + ((32 - fixDescender) >> 6);
            MouseSafetyOnDrawStart(dc->ppdev, DestRect.left, DestRect.top, DestRect.right, DestRect.bottom);
            if (dc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
            {
               IntUpdateBoundsRect(dc, &DestRect);
            }
            IntEngBitBlt(
                &psurf->SurfObj,
                NULL,
                NULL,
                &dc->co.ClipObj,
                NULL,
                &DestRect,
                &SourcePoint,
                &SourcePoint,
                &dc->eboBackground.BrushObject,
                &BrushOrigin,
                ROP4_FROM_INDEX(R3_OPINDEX_PATCOPY));
            MouseSafetyOnDrawEnd(dc->ppdev);
            BackgroundLeft = DestRect.right;
        }

        DestRect.left = ((TextLeft + 32) >> 6) + realglyph->left;
        DestRect.right = DestRect.left + realglyph->bitmap.width;
        DestRect.top = TextTop + yoff - realglyph->top;
        DestRect.bottom = DestRect.top + realglyph->bitmap.rows;

        bitSize.cx = realglyph->bitmap.width;
        bitSize.cy = realglyph->bitmap.rows;
        MaskRect.right = realglyph->bitmap.width;
        MaskRect.bottom = realglyph->bitmap.rows;

        /* Check if the bitmap has any pixels */
        if ((bitSize.cx != 0) && (bitSize.cy != 0))
        {
            /*
             * We should create the bitmap out of the loop at the biggest possible
             * glyph size. Then use memset with 0 to clear it and sourcerect to
             * limit the work of the transbitblt.
             */
            HSourceGlyph = EngCreateBitmap(bitSize, realglyph->bitmap.pitch,
                                           BMF_8BPP, BMF_TOPDOWN,
                                           realglyph->bitmap.buffer);
            if ( !HSourceGlyph )
            {
                DPRINT1("WARNING: EngCreateBitmap() failed!\n");
                // FT_Done_Glyph(realglyph);
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }
            SourceGlyphSurf = EngLockSurface((HSURF)HSourceGlyph);
            if ( !SourceGlyphSurf )
            {
                EngDeleteSurface((HSURF)HSourceGlyph);
                DPRINT1("WARNING: EngLockSurface() failed!\n");
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }

            /*
             * Use the font data as a mask to paint onto the DCs surface using a
             * brush.
             */
            if (lprc && (fuOptions & ETO_CLIPPED) &&
                    DestRect.right >= lprc->right + dc->ptlDCOrig.x)
            {
                // We do the check '>=' instead of '>' to possibly save an iteration
                // through this loop, since it's breaking after the drawing is done,
                // and x is always incremented.
                DestRect.right = lprc->right + dc->ptlDCOrig.x;
                DoBreak = TRUE;
            }
            if (lprc && (fuOptions & ETO_CLIPPED) &&
                    DestRect.bottom >= lprc->bottom + dc->ptlDCOrig.y)
            {
                DestRect.bottom = lprc->bottom + dc->ptlDCOrig.y;
            }
            MouseSafetyOnDrawStart(dc->ppdev, DestRect.left, DestRect.top, DestRect.right, DestRect.bottom);
            if (!IntEngMaskBlt(
                SurfObj,
                SourceGlyphSurf,
                &dc->co.ClipObj,
                &exloRGB2Dst.xlo,
                &exloDst2RGB.xlo,
                &DestRect,
                (PPOINTL)&MaskRect,
                &dc->eboText.BrushObject,
                &BrushOrigin))
            {
                DPRINT1("Failed to MaskBlt a glyph!\n");
            }

            MouseSafetyOnDrawEnd(dc->ppdev) ;

            EngUnlockSurface(SourceGlyphSurf);
            EngDeleteSurface((HSURF)HSourceGlyph);
        }

        if (DoBreak)
        {
            break;
        }

        if (plf->lfUnderline)
        {
            int i, position;
            if (!face->units_per_EM)
            {
                position = 0;
            }
            else
            {
                position = face->underline_position *
                    face->size->metrics.y_ppem / face->units_per_EM;
            }
            for (i = -thickness / 2; i < -thickness / 2 + thickness; ++i)
            {
                EngLineTo(SurfObj,
                          &dc->co.ClipObj,
                          &dc->eboText.BrushObject,
                          (TextLeft >> 6),
                          TextTop + yoff - position + i,
                          ((TextLeft + (realglyph->root.advance.x >> 10)) >> 6),
                          TextTop + yoff - position + i,
                          NULL,
                          ROP2_TO_MIX(R2_COPYPEN));
            }
        }
        if (plf->lfStrikeOut)
        {
            int i;
            for (i = -thickness / 2; i < -thickness / 2 + thickness; ++i)
            {
                EngLineTo(SurfObj,
                          &dc->co.ClipObj,
                          &dc->eboText.BrushObject,
                          (TextLeft >> 6),
                          TextTop + yoff - (fixAscender >> 6) / 3 + i,
                          ((TextLeft + (realglyph->root.advance.x >> 10)) >> 6),
                          TextTop + yoff - (fixAscender >> 6) / 3 + i,
                          NULL,
                          ROP2_TO_MIX(R2_COPYPEN));
            }
        }

        if (NULL == Dx)
        {
            TextLeft += realglyph->root.advance.x >> 10;
            DPRINT("New TextLeft: %I64d\n", TextLeft);
        }
        else
        {
            // FIXME this should probably be a matrix transform with TextTop as well.
            Scale = pdcattr->mxWorldToDevice.efM11;
            if (FLOATOBJ_Equal0(&Scale))
                FLOATOBJ_Set1(&Scale);

            /* do the shift before multiplying to preserve precision */
            FLOATOBJ_MulLong(&Scale, Dx[i<<DxShift] << 6); 
            TextLeft += FLOATOBJ_GetLong(&Scale);
            DPRINT("New TextLeft2: %I64d\n", TextLeft);
        }

        if (DxShift)
        {
            TextTop -= Dx[2 * i + 1] << 6;
        }

        previous = glyph_index;

        if (EmuBold || EmuItalic)
        {
            FT_Done_Glyph((FT_Glyph)realglyph);
            realglyph = NULL;
        }
    }

    if (pdcattr->lTextAlign & TA_UPDATECP) {
        pdcattr->ptlCurrent.x = DestRect.right - dc->ptlDCOrig.x;
    }

    IntUnLockFreeType;

    DC_vFinishBlit(dc, NULL) ;

    EXLATEOBJ_vCleanup(&exloRGB2Dst);
    EXLATEOBJ_vCleanup(&exloDst2RGB);
    if (TextObj != NULL)
        TEXTOBJ_UnlockText(TextObj);
good:
    DC_UnlockDc( dc );

    return TRUE;

fail2:
    EXLATEOBJ_vCleanup(&exloRGB2Dst);
    EXLATEOBJ_vCleanup(&exloDst2RGB);
fail:
    if (TextObj != NULL)
        TEXTOBJ_UnlockText(TextObj);

    DC_UnlockDc(dc);

    return FALSE;
}

#define STACK_TEXT_BUFFER_SIZE 100
BOOL
APIENTRY
NtGdiExtTextOutW(
    IN HDC hDC,
    IN INT XStart,
    IN INT YStart,
    IN UINT fuOptions,
    IN OPTIONAL LPRECT UnsafeRect,
    IN LPWSTR UnsafeString,
    IN INT Count,
    IN OPTIONAL LPINT UnsafeDx,
    IN DWORD dwCodePage)
{
    BOOL Result = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    RECTL SafeRect;
    BYTE LocalBuffer[STACK_TEXT_BUFFER_SIZE];
    PVOID Buffer = LocalBuffer;
    LPCWSTR SafeString = NULL;
    LPINT SafeDx = NULL;
    ULONG BufSize, StringSize, DxSize = 0;

    /* Check if String is valid */
    if ((Count > 0xFFFF) || (Count > 0 && UnsafeString == NULL))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Count > 0)
    {
        /* Calculate buffer size for string and Dx values */
        BufSize = StringSize = Count * sizeof(WCHAR);
        if (UnsafeDx)
        {
            /* If ETO_PDY is specified, we have pairs of INTs */
            DxSize = (Count * sizeof(INT)) * (fuOptions & ETO_PDY ? 2 : 1);
            BufSize += DxSize;
        }

        /* Check if our local buffer is large enough */
        if (BufSize > STACK_TEXT_BUFFER_SIZE)
        {
            /* It's not, allocate a temp buffer */
            Buffer = ExAllocatePoolWithTag(PagedPool, BufSize, GDITAG_TEXT);
            if (!Buffer)
            {
                return FALSE;
            }
        }

        /* Probe and copy user mode data to the buffer */
        _SEH2_TRY
        {
            /* Put the Dx before the String to assure alignment of 4 */
            SafeString = (LPCWSTR)(((ULONG_PTR)Buffer) + DxSize);

            /* Probe and copy the string */
            ProbeForRead(UnsafeString, StringSize, 1);
            memcpy((PVOID)SafeString, UnsafeString, StringSize);

            /* If we have Dx values... */
            if (UnsafeDx)
            {
                /* ... probe and copy them */
                SafeDx = Buffer;
                ProbeForRead(UnsafeDx, DxSize, 1);
                memcpy(SafeDx, UnsafeDx, DxSize);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
        if (!NT_SUCCESS(Status))
        {
            goto cleanup;
        }
    }

    /* If we have a rect, copy it */
    if (UnsafeRect)
    {
        _SEH2_TRY
        {
            ProbeForRead(UnsafeRect, sizeof(RECT), 1);
            SafeRect = *UnsafeRect;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
        if (!NT_SUCCESS(Status))
        {
            goto cleanup;
        }
    }

    /* Finally call the internal routine */
    Result = GreExtTextOutW(hDC,
                            XStart,
                            YStart,
                            fuOptions,
                            &SafeRect,
                            SafeString,
                            Count,
                            SafeDx,
                            dwCodePage);

cleanup:
    /* If we allocated a buffer, free it */
    if (Buffer != LocalBuffer)
    {
        ExFreePoolWithTag(Buffer, GDITAG_TEXT);
    }

    return Result;
}


/*
* @implemented
*/
BOOL
APIENTRY
NtGdiGetCharABCWidthsW(
    IN HDC hDC,
    IN UINT FirstChar,
    IN ULONG Count,
    IN OPTIONAL PWCHAR UnSafepwch,
    IN FLONG fl,
    OUT PVOID Buffer)
{
    LPABC SafeBuff;
    LPABCFLOAT SafeBuffF = NULL;
    PDC dc;
    PDC_ATTR pdcattr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    FT_Face face;
    FT_CharMap charmap, found = NULL;
    UINT i, glyph_index, BufferSize;
    HFONT hFont = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PMATRIX pmxWorldToDevice;
    PWCHAR Safepwch = NULL;
    LOGFONTW *plf;

    if (!Buffer)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (UnSafepwch)
    {
        UINT pwchSize = Count * sizeof(WCHAR);
        Safepwch = ExAllocatePoolWithTag(PagedPool, pwchSize, GDITAG_TEXT);

        if(!Safepwch)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        _SEH2_TRY
        {
            ProbeForRead(UnSafepwch, pwchSize, 1);
            RtlCopyMemory(Safepwch, UnSafepwch, pwchSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    if (!NT_SUCCESS(Status))
    {
        if(Safepwch)
            ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

        EngSetLastError(Status);
        return FALSE;
    }

    BufferSize = Count * sizeof(ABC); // Same size!
    SafeBuff = ExAllocatePoolWithTag(PagedPool, BufferSize, GDITAG_TEXT);
    if (!fl) SafeBuffF = (LPABCFLOAT) SafeBuff;
    if (SafeBuff == NULL)
    {

        if(Safepwch)
            ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);

        if(Safepwch)
            ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);

    /* Get the DC's world-to-device transformation matrix */
    pmxWorldToDevice = DC_pmxWorldToDevice(dc);
    DC_UnlockDc(dc);

    if (TextObj == NULL)
    {
        ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);

        if(Safepwch)
            ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    FontGDI = ObjToGDI(TextObj->Font, FONT);

    face = FontGDI->SharedFace->Face;
    if (face->charmap == NULL)
    {
        for (i = 0; i < (UINT)face->num_charmaps; i++)
        {
            charmap = face->charmaps[i];
            if (charmap->encoding != 0)
            {
                found = charmap;
                break;
            }
        }

        if (!found)
        {
            DPRINT1("WARNING: Could not find desired charmap!\n");
            ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);

            if(Safepwch)
                ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

            EngSetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        IntLockFreeType;
        FT_Set_Charmap(face, found);
        IntUnLockFreeType;
    }

    plf = &TextObj->logfont.elfEnumLogfontEx.elfLogFont;
    IntLockFreeType;
    IntRequestFontSize(dc, face, plf->lfWidth, plf->lfHeight);
    FtSetCoordinateTransform(face, pmxWorldToDevice);

    for (i = FirstChar; i < FirstChar+Count; i++)
    {
        int adv, lsb, bbx, left, right;

        if (Safepwch)
        {
            if (fl & GCABCW_INDICES)
                glyph_index = Safepwch[i - FirstChar];
            else
                glyph_index = FT_Get_Char_Index(face, Safepwch[i - FirstChar]);
        }
        else
        {
            if (fl & GCABCW_INDICES)
                glyph_index = i;
            else
                glyph_index = FT_Get_Char_Index(face, i);
        }
        FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);

        left = (INT)face->glyph->metrics.horiBearingX  & -64;
        right = (INT)((face->glyph->metrics.horiBearingX + face->glyph->metrics.width) + 63) & -64;
        adv  = (face->glyph->advance.x + 32) >> 6;

//      int test = (INT)(face->glyph->metrics.horiAdvance + 63) >> 6;
//      DPRINT1("Advance Wine %d and Advance Ros %d\n",test, adv ); /* It's the same! */

        lsb = left >> 6;
        bbx = (right - left) >> 6;
        /*
              DPRINT1("lsb %d and bbx %d\n", lsb, bbx );
         */
        if (!fl)
        {
            SafeBuffF[i - FirstChar].abcfA = (FLOAT) lsb;
            SafeBuffF[i - FirstChar].abcfB = (FLOAT) bbx;
            SafeBuffF[i - FirstChar].abcfC = (FLOAT) (adv - lsb - bbx);
        }
        else
        {
            SafeBuff[i - FirstChar].abcA = lsb;
            SafeBuff[i - FirstChar].abcB = bbx;
            SafeBuff[i - FirstChar].abcC = adv - lsb - bbx;
        }
    }
    IntUnLockFreeType;
    TEXTOBJ_UnlockText(TextObj);
    Status = MmCopyToCaller(Buffer, SafeBuff, BufferSize);

    ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);

    if(Safepwch)
        ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

    if (! NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

    DPRINT("NtGdiGetCharABCWidths Worked!\n");
    return TRUE;
}

/*
* @implemented
*/
BOOL
APIENTRY
NtGdiGetCharWidthW(
    IN HDC hDC,
    IN UINT FirstChar,
    IN UINT Count,
    IN OPTIONAL PWCHAR UnSafepwc,
    IN FLONG fl,
    OUT PVOID Buffer)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LPINT SafeBuff;
    PFLOAT SafeBuffF = NULL;
    PDC dc;
    PDC_ATTR pdcattr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    FT_Face face;
    FT_CharMap charmap, found = NULL;
    UINT i, glyph_index, BufferSize;
    HFONT hFont = 0;
    PMATRIX pmxWorldToDevice;
    PWCHAR Safepwc = NULL;
    LOGFONTW *plf;

    if (UnSafepwc)
    {
        UINT pwcSize = Count * sizeof(WCHAR);
        Safepwc = ExAllocatePoolWithTag(PagedPool, pwcSize, GDITAG_TEXT);

        if(!Safepwc)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        _SEH2_TRY
        {
            ProbeForRead(UnSafepwc, pwcSize, 1);
            RtlCopyMemory(Safepwc, UnSafepwc, pwcSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    if (!NT_SUCCESS(Status))
    {
        EngSetLastError(Status);
        return FALSE;
    }

    BufferSize = Count * sizeof(INT); // Same size!
    SafeBuff = ExAllocatePoolWithTag(PagedPool, BufferSize, GDITAG_TEXT);
    if (!fl) SafeBuffF = (PFLOAT) SafeBuff;
    if (SafeBuff == NULL)
    {
        if(Safepwc)
            ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        if(Safepwc)
            ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

        ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);
    /* Get the DC's world-to-device transformation matrix */
    pmxWorldToDevice = DC_pmxWorldToDevice(dc);
    DC_UnlockDc(dc);

    if (TextObj == NULL)
    {
        if(Safepwc)
            ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

        ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    FontGDI = ObjToGDI(TextObj->Font, FONT);

    face = FontGDI->SharedFace->Face;
    if (face->charmap == NULL)
    {
        for (i = 0; i < (UINT)face->num_charmaps; i++)
        {
            charmap = face->charmaps[i];
            if (charmap->encoding != 0)
            {
                found = charmap;
                break;
            }
        }

        if (!found)
        {
            DPRINT1("WARNING: Could not find desired charmap!\n");

            if(Safepwc)
                ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

            ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);
            EngSetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        IntLockFreeType;
        FT_Set_Charmap(face, found);
        IntUnLockFreeType;
    }

    plf = &TextObj->logfont.elfEnumLogfontEx.elfLogFont;
    IntLockFreeType;
    IntRequestFontSize(dc, face, plf->lfWidth, plf->lfHeight);
    FtSetCoordinateTransform(face, pmxWorldToDevice);

    for (i = FirstChar; i < FirstChar+Count; i++)
    {
        if (Safepwc)
        {
            if (fl & GCW_INDICES)
                glyph_index = Safepwc[i - FirstChar];
            else
                glyph_index = FT_Get_Char_Index(face, Safepwc[i - FirstChar]);
        }
        else
        {
            if (fl & GCW_INDICES)
                glyph_index = i;
            else
                glyph_index = FT_Get_Char_Index(face, i);
        }
        FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        if (!fl)
            SafeBuffF[i - FirstChar] = (FLOAT) ((face->glyph->advance.x + 32) >> 6);
        else
            SafeBuff[i - FirstChar] = (face->glyph->advance.x + 32) >> 6;
    }
    IntUnLockFreeType;
    TEXTOBJ_UnlockText(TextObj);
    MmCopyToCaller(Buffer, SafeBuff, BufferSize);

    if(Safepwc)
        ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

    ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);
    return TRUE;
}


/*
* @implemented
*/
// TODO: Move this code into NtGdiGetGlyphIndicesWInternal and wrap
// NtGdiGetGlyphIndicesW around NtGdiGetGlyphIndicesWInternal instead.
// NOTE: See also GreGetGlyphIndicesW.
__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesW(
    _In_ HDC hdc,
    _In_reads_opt_(cwc) LPCWSTR pwc,
    _In_ INT cwc,
    _Out_writes_opt_(cwc) LPWORD pgi,
    _In_ DWORD iMode)
{
    PDC dc;
    PDC_ATTR pdcattr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    HFONT hFont = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    OUTLINETEXTMETRICW *potm;
    INT i;
    WCHAR DefChar = 0xffff;
    PWSTR Buffer = NULL;
    ULONG Size, pwcSize;
    PWSTR Safepwc = NULL;
    LPCWSTR UnSafepwc = pwc;
    LPWORD UnSafepgi = pgi;

    /* Check for integer overflow */
    if (cwc & 0x80000000) // (INT_MAX + 1) == INT_MIN 
        return GDI_ERROR;

    if (!UnSafepwc && !UnSafepgi)
        return cwc;

    if (!UnSafepwc || !UnSafepgi)
    {
        DPRINT1("UnSafepwc == %p, UnSafepgi = %p\n", UnSafepwc, UnSafepgi);
        return GDI_ERROR;
    }

    // TODO: Special undocumented case!
    if (!pwc && !pgi && (cwc == 0))
    {
        DPRINT1("ERR: NtGdiGetGlyphIndicesW with (!pwc && !pgi && (cwc == 0)) is UNIMPLEMENTED!\n");
        return 0;
    }

    // FIXME: This is a hack!! (triggered by e.g. Word 2010). See CORE-12825
    if (cwc == 0)
    {
        DPRINT1("ERR: NtGdiGetGlyphIndicesW with (cwc == 0) is UNIMPLEMENTED!\n");
        return GDI_ERROR;
    }

    dc = DC_LockDc(hdc);
    if (!dc)
    {
        return GDI_ERROR;
    }
    pdcattr = dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);
    DC_UnlockDc(dc);
    if (!TextObj)
    {
        return GDI_ERROR;
    }

    FontGDI = ObjToGDI(TextObj->Font, FONT);
    TEXTOBJ_UnlockText(TextObj);

    Buffer = ExAllocatePoolWithTag(PagedPool, cwc*sizeof(WORD), GDITAG_TEXT);
    if (!Buffer)
    {
        return GDI_ERROR;
    }

    if (iMode & GGI_MARK_NONEXISTING_GLYPHS)
    {
        DefChar = 0xffff;
    }
    else
    {
        FT_Face Face = FontGDI->SharedFace->Face;
        if (FT_IS_SFNT(Face))
        {
            TT_OS2 *pOS2 = FT_Get_Sfnt_Table(Face, ft_sfnt_os2);
            DefChar = (pOS2->usDefaultChar ? FT_Get_Char_Index(Face, pOS2->usDefaultChar) : 0);
        }
        else
        {
            Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
            potm = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
            if (!potm)
            {
                cwc = GDI_ERROR;
                goto ErrorRet;
            }
            IntGetOutlineTextMetrics(FontGDI, Size, potm);
            DefChar = potm->otmTextMetrics.tmDefaultChar;
            ExFreePoolWithTag(potm, GDITAG_TEXT);
        }
    }

    pwcSize = cwc * sizeof(WCHAR);
    Safepwc = ExAllocatePoolWithTag(PagedPool, pwcSize, GDITAG_TEXT);

    if (!Safepwc)
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorRet;
    }

    _SEH2_TRY
    {
        ProbeForRead(UnSafepwc, pwcSize, 1);
        RtlCopyMemory(Safepwc, UnSafepwc, pwcSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status)) goto ErrorRet;

    IntLockFreeType;

    for (i = 0; i < cwc; i++)
    {
        Buffer[i] = FT_Get_Char_Index(FontGDI->SharedFace->Face, Safepwc[i]);
        if (Buffer[i] == 0)
        {
            Buffer[i] = DefChar;
        }
    }

    IntUnLockFreeType;

    _SEH2_TRY
    {
        ProbeForWrite(UnSafepgi, cwc * sizeof(WORD), 1);
        RtlCopyMemory(UnSafepgi, Buffer, cwc * sizeof(WORD));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

ErrorRet:
    ExFreePoolWithTag(Buffer, GDITAG_TEXT);
    if (Safepwc != NULL)
    {
        ExFreePoolWithTag(Safepwc, GDITAG_TEXT);
    }
    if (NT_SUCCESS(Status)) return cwc;
    return GDI_ERROR;
}

/* EOF */
