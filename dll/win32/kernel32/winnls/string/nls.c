/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/winnls/string/nls.c
 * PURPOSE:         National Language Support
 * PROGRAMMER:      Filip Navara
 *                  Hartmut Birr
 *                  Gunnar Andre Dalsnes
 *                  Thomas Weidenmueller
 *                  Katayama Hirofumi MZ
 * UPDATE HISTORY:
 *                  Created 24/08/2004
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* GLOBAL VARIABLES ***********************************************************/

/* Sequence length based on the first character. */
static const char UTF8Length[128] =
{
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x80 - 0x8F */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x90 - 0x9F */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xA0 - 0xAF */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xB0 - 0xBF */
   0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0xC0 - 0xCF */
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0xD0 - 0xDF */
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 0xE0 - 0xEF */
   3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0  /* 0xF0 - 0xFF */
};

/* First byte mask depending on UTF-8 sequence length. */
static const unsigned char UTF8Mask[6] = {0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

/* UTF-8 length to lower bound */
static const unsigned long UTF8LBound[] =
    {0, 0x80, 0x800, 0x10000, 0x200000, 0x2000000, 0xFFFFFFFF};

/* FIXME: Change to HASH table or linear array. */
static LIST_ENTRY CodePageListHead;
static CODEPAGE_ENTRY AnsiCodePage;
static CODEPAGE_ENTRY OemCodePage;
static RTL_CRITICAL_SECTION CodePageListLock;

/* FORWARD DECLARATIONS *******************************************************/

BOOL WINAPI
GetNlsSectionName(UINT CodePage, UINT Base, ULONG Unknown,
                  LPSTR BaseName, LPSTR Result, ULONG ResultSize);

BOOL WINAPI
GetCPFileNameFromRegistry(UINT CodePage, LPWSTR FileName, ULONG FileNameSize);

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @name NlsInit
 *
 * Internal NLS related stuff initialization.
 */

BOOL
FASTCALL
NlsInit(VOID)
{
    UNICODE_STRING DirName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;

    InitializeListHead(&CodePageListHead);
    RtlInitializeCriticalSection(&CodePageListLock);

    /*
     * FIXME: Eventually this should be done only for the NLS Server
     * process, but since we don't have anything like that (yet?) we
     * always try to create the "\Nls" directory here.
     */
    RtlInitUnicodeString(&DirName, L"\\Nls");

    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);

    if (NT_SUCCESS(NtCreateDirectoryObject(&Handle, DIRECTORY_ALL_ACCESS, &ObjectAttributes)))
    {
        NtClose(Handle);
    }

    /* Setup ANSI code page. */
    AnsiCodePage.SectionHandle = NULL;
    AnsiCodePage.SectionMapping = NtCurrentTeb()->ProcessEnvironmentBlock->AnsiCodePageData;

    RtlInitCodePageTable((PUSHORT)AnsiCodePage.SectionMapping,
                         &AnsiCodePage.CodePageTable);
    AnsiCodePage.CodePage = AnsiCodePage.CodePageTable.CodePage;

    InsertTailList(&CodePageListHead, &AnsiCodePage.Entry);

    /* Setup OEM code page. */
    OemCodePage.SectionHandle = NULL;
    OemCodePage.SectionMapping = NtCurrentTeb()->ProcessEnvironmentBlock->OemCodePageData;

    RtlInitCodePageTable((PUSHORT)OemCodePage.SectionMapping,
                         &OemCodePage.CodePageTable);
    OemCodePage.CodePage = OemCodePage.CodePageTable.CodePage;
    InsertTailList(&CodePageListHead, &OemCodePage.Entry);

    return TRUE;
}

/**
 * @name NlsUninit
 *
 * Internal NLS related stuff uninitialization.
 */

VOID
FASTCALL
NlsUninit(VOID)
{
    PCODEPAGE_ENTRY Current;

    /* Delete the code page list. */
    while (!IsListEmpty(&CodePageListHead))
    {
        Current = CONTAINING_RECORD(CodePageListHead.Flink, CODEPAGE_ENTRY, Entry);
        if (Current->SectionHandle != NULL)
        {
            UnmapViewOfFile(Current->SectionMapping);
            NtClose(Current->SectionHandle);
        }
        RemoveHeadList(&CodePageListHead);
    }
    RtlDeleteCriticalSection(&CodePageListLock);
}

/**
 * @name IntGetLoadedCodePageEntry
 *
 * Internal function to get structure containing a code page information
 * of code page that is already loaded.
 *
 * @param CodePage
 *        Number of the code page. Special values like CP_OEMCP, CP_ACP
 *        or CP_UTF8 aren't allowed.
 *
 * @return Code page entry or NULL if the specified code page hasn't
 *         been loaded yet.
 */

PCODEPAGE_ENTRY
FASTCALL
IntGetLoadedCodePageEntry(UINT CodePage)
{
    LIST_ENTRY *CurrentEntry;
    PCODEPAGE_ENTRY Current;

    RtlEnterCriticalSection(&CodePageListLock);
    for (CurrentEntry = CodePageListHead.Flink;
         CurrentEntry != &CodePageListHead;
         CurrentEntry = CurrentEntry->Flink)
    {
        Current = CONTAINING_RECORD(CurrentEntry, CODEPAGE_ENTRY, Entry);
        if (Current->CodePage == CodePage)
        {
            RtlLeaveCriticalSection(&CodePageListLock);
            return Current;
        }
    }
    RtlLeaveCriticalSection(&CodePageListLock);

    return NULL;
}

/**
 * @name IntGetCodePageEntry
 *
 * Internal function to get structure containing a code page information.
 *
 * @param CodePage
 *        Number of the code page. Special values like CP_OEMCP, CP_ACP
 *        or CP_THREAD_ACP are allowed, but CP_UTF[7/8] isn't.
 *
 * @return Code page entry.
 */

PCODEPAGE_ENTRY
FASTCALL
IntGetCodePageEntry(UINT CodePage)
{
    CHAR SectionName[40];
    NTSTATUS Status;
    HANDLE SectionHandle = INVALID_HANDLE_VALUE, FileHandle;
    PBYTE SectionMapping;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ANSI_STRING AnsiName;
    UNICODE_STRING UnicodeName;
    WCHAR FileName[MAX_PATH + 1];
    UINT FileNamePos;
    PCODEPAGE_ENTRY CodePageEntry;
    if (CodePage == CP_ACP)
    {
        return &AnsiCodePage;
    }
    else if (CodePage == CP_OEMCP)
    {
        return &OemCodePage;
    }
    else if (CodePage == CP_THREAD_ACP)
    {
        if (!GetLocaleInfoW(GetThreadLocale(),
                            LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                            (WCHAR *)&CodePage,
                            sizeof(CodePage) / sizeof(WCHAR)))
        {
            /* Last error is set by GetLocaleInfoW. */
            return NULL;
        }
        if (CodePage == 0)
            return &AnsiCodePage;
    }
    else if (CodePage == CP_MACCP)
    {
        if (!GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT,
                            LOCALE_IDEFAULTMACCODEPAGE | LOCALE_RETURN_NUMBER,
                            (WCHAR *)&CodePage,
                            sizeof(CodePage) / sizeof(WCHAR)))
        {
            /* Last error is set by GetLocaleInfoW. */
            return NULL;
        }
    }

    /* Try searching for loaded page first. */
    CodePageEntry = IntGetLoadedCodePageEntry(CodePage);
    if (CodePageEntry != NULL)
    {
        return CodePageEntry;
    }

    /*
     * Yes, we really want to lock here. Otherwise it can happen that
     * two parallel requests will try to get the entry for the same
     * code page and we would load it twice.
     */
    RtlEnterCriticalSection(&CodePageListLock);

    /* Generate the section name. */
    if (!GetNlsSectionName(CodePage,
                           10,
                           0,
                           "\\Nls\\NlsSectionCP",
                           SectionName,
                           sizeof(SectionName)))
    {
        RtlLeaveCriticalSection(&CodePageListLock);
        return NULL;
    }

    RtlInitAnsiString(&AnsiName, SectionName);
    RtlAnsiStringToUnicodeString(&UnicodeName, &AnsiName, TRUE);

    InitializeObjectAttributes(&ObjectAttributes, &UnicodeName, 0, NULL, NULL);

    /* Try to open the section first */
    Status = NtOpenSection(&SectionHandle, SECTION_MAP_READ, &ObjectAttributes);

    /* If the section doesn't exist, try to create it. */
    if (Status == STATUS_UNSUCCESSFUL ||
        Status == STATUS_OBJECT_NAME_NOT_FOUND ||
        Status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        FileNamePos = GetSystemDirectoryW(FileName, MAX_PATH);
        if (GetCPFileNameFromRegistry(CodePage,
                                      FileName + FileNamePos + 1,
                                      MAX_PATH - FileNamePos - 1))
        {
            FileName[FileNamePos] = L'\\';
            FileName[MAX_PATH] = 0;
            FileHandle = CreateFileW(FileName,
                                     FILE_GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL);

            Status = NtCreateSection(&SectionHandle,
                                     SECTION_MAP_READ,
                                     &ObjectAttributes,
                                     NULL,
                                     PAGE_READONLY,
                                     SEC_COMMIT,
                                     FileHandle);

            /* HACK: Check if another process was faster
             * and already created this section. See bug 3626 for details */
            if (Status == STATUS_OBJECT_NAME_COLLISION)
            {
                /* Close the file then */
                NtClose(FileHandle);

                /* And open the section */
                Status = NtOpenSection(&SectionHandle,
                                       SECTION_MAP_READ,
                                       &ObjectAttributes);
            }
        }
    }
    RtlFreeUnicodeString(&UnicodeName);

    if (!NT_SUCCESS(Status))
    {
        RtlLeaveCriticalSection(&CodePageListLock);
        return NULL;
    }

    SectionMapping = MapViewOfFile(SectionHandle, FILE_MAP_READ, 0, 0, 0);
    if (SectionMapping == NULL)
    {
        NtClose(SectionHandle);
        RtlLeaveCriticalSection(&CodePageListLock);
        return NULL;
    }

    CodePageEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(CODEPAGE_ENTRY));
    if (CodePageEntry == NULL)
    {
        NtClose(SectionHandle);
        RtlLeaveCriticalSection(&CodePageListLock);
        return NULL;
    }

    CodePageEntry->CodePage = CodePage;
    CodePageEntry->SectionHandle = SectionHandle;
    CodePageEntry->SectionMapping = SectionMapping;

    RtlInitCodePageTable((PUSHORT)SectionMapping, &CodePageEntry->CodePageTable);

    /* Insert the new entry to list and unlock. Uff. */
    InsertTailList(&CodePageListHead, &CodePageEntry->Entry);
    RtlLeaveCriticalSection(&CodePageListLock);

    return CodePageEntry;
}

/**
 * @name IntMultiByteToWideCharUTF8
 *
 * Internal version of MultiByteToWideChar for UTF8.
 *
 * @note We use Win10's behaviour due to security reason.
 *
 * @see MultiByteToWideChar
 */
static
INT
WINAPI
IntMultiByteToWideCharUTF8(DWORD Flags,
                           LPCSTR MultiByteString,
                           INT MultiByteCount,
                           LPWSTR WideCharString,
                           INT WideCharCount)
{
    LPCSTR MbsEnd, MbsPtrSave;
    UCHAR Char, TrailLength;
    WCHAR WideChar;
    LONG Count;
    BOOL CharIsValid, StringIsValid = TRUE;
    const WCHAR InvalidChar = 0xFFFD;

    if (Flags != 0 && Flags != MB_ERR_INVALID_CHARS)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    /* Does caller query for output buffer size? */
    if (WideCharCount == 0)
    {
        /* validate and count the wide characters */
        MbsEnd = MultiByteString + MultiByteCount;
        for (; MultiByteString < MbsEnd; WideCharCount++)
        {
            Char = *MultiByteString++;
            if (Char < 0x80)
            {
                TrailLength = 0;
                continue;
            }
            if ((Char & 0xC0) == 0x80)
            {
                TrailLength = 0;
                StringIsValid = FALSE;
                continue;
            }

            TrailLength = UTF8Length[Char - 0x80];
            if (TrailLength == 0)
            {
                StringIsValid = FALSE;
                continue;
            }

            CharIsValid = TRUE;
            MbsPtrSave = MultiByteString;
            WideChar = Char & UTF8Mask[TrailLength];

            while (TrailLength && MultiByteString < MbsEnd)
            {
                if ((*MultiByteString & 0xC0) != 0x80)
                {
                    CharIsValid = StringIsValid = FALSE;
                    break;
                }

                WideChar = (WideChar << 6) | (*MultiByteString++ & 0x7f);
                TrailLength--;
            }

            if (!CharIsValid || WideChar < UTF8LBound[UTF8Length[Char - 0x80]])
            {
                MultiByteString = MbsPtrSave;
            }
        }

        if (TrailLength)
        {
            WideCharCount++;
            StringIsValid = FALSE;
        }

        if (Flags == MB_ERR_INVALID_CHARS && !StringIsValid)
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }

        return WideCharCount;
    }

    /* convert */
    MbsEnd = MultiByteString + MultiByteCount;
    for (Count = 0; Count < WideCharCount && MultiByteString < MbsEnd; Count++)
    {
        Char = *MultiByteString++;
        if (Char < 0x80)
        {
            *WideCharString++ = Char;
            TrailLength = 0;
            continue;
        }
        if ((Char & 0xC0) == 0x80)
        {
            *WideCharString++ = InvalidChar;
            TrailLength = 0;
            StringIsValid = FALSE;
            continue;
        }

        TrailLength = UTF8Length[Char - 0x80];
        if (TrailLength == 0)
        {
            *WideCharString++ = InvalidChar;
            StringIsValid = FALSE;
            continue;
        }

        CharIsValid = TRUE;
        MbsPtrSave = MultiByteString;
        WideChar = Char & UTF8Mask[TrailLength];

        while (TrailLength && MultiByteString < MbsEnd)
        {
            if ((*MultiByteString & 0xC0) != 0x80)
            {
                CharIsValid = StringIsValid = FALSE;
                break;
            }

            WideChar = (WideChar << 6) | (*MultiByteString++ & 0x7f);
            TrailLength--;
        }

        if (CharIsValid && UTF8LBound[UTF8Length[Char - 0x80]] <= WideChar)
        {
            *WideCharString++ = WideChar;
        }
        else
        {
            *WideCharString++ = InvalidChar;
            MultiByteString = MbsPtrSave;
            StringIsValid = FALSE;
        }
    }

    if (TrailLength && Count < WideCharCount && MultiByteString < MbsEnd)
    {
        *WideCharString = InvalidChar;
        WideCharCount++;
    }

    if (MultiByteString < MbsEnd)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    if (Flags == MB_ERR_INVALID_CHARS && (!StringIsValid || TrailLength))
    {
        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
        return 0;
    }

    return Count;
}

/**
 * @name IntMultiByteToWideCharCP
 *
 * Internal version of MultiByteToWideChar for code page tables.
 *
 * @see MultiByteToWideChar
 * @todo Handle MB_PRECOMPOSED, MB_COMPOSITE, MB_USEGLYPHCHARS and
 *       DBCS codepages.
 */

static
INT
WINAPI
IntMultiByteToWideCharCP(UINT CodePage,
                         DWORD Flags,
                         LPCSTR MultiByteString,
                         INT MultiByteCount,
                         LPWSTR WideCharString,
                         INT WideCharCount)
{
    PCODEPAGE_ENTRY CodePageEntry;
    PCPTABLEINFO CodePageTable;
    PUSHORT MultiByteTable;
    LPCSTR TempString;
    INT TempLength;
    USHORT WideChar;

    /* Get code page table. */
    CodePageEntry = IntGetCodePageEntry(CodePage);
    if (CodePageEntry == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    CodePageTable = &CodePageEntry->CodePageTable;

    /* If MB_USEGLYPHCHARS flag present and glyph table present */
    if ((Flags & MB_USEGLYPHCHARS) && CodePageTable->MultiByteTable[256])
    {
        /* Use glyph table */
        MultiByteTable = CodePageTable->MultiByteTable + 256 + 1;
    }
    else
    {
        MultiByteTable = CodePageTable->MultiByteTable;
    }

    /* Different handling for DBCS code pages. */
    if (CodePageTable->DBCSCodePage)
    {
        UCHAR Char;
        USHORT DBCSOffset;
        LPCSTR MbsEnd = MultiByteString + MultiByteCount;
        INT Count;

        if (Flags & MB_ERR_INVALID_CHARS)
        {
            TempString = MultiByteString;

            while (TempString < MbsEnd)
            {
                DBCSOffset = CodePageTable->DBCSOffsets[(UCHAR)*TempString];

                if (DBCSOffset)
                {
                    /* If lead byte is presented, but behind it there is no symbol */
                    if (((TempString + 1) == MbsEnd) || (*(TempString + 1) == 0))
                    {
                        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                        return 0;
                    }

                    WideChar = CodePageTable->DBCSOffsets[DBCSOffset + *(TempString + 1)];

                    if (WideChar == CodePageTable->UniDefaultChar &&
                        MAKEWORD(*(TempString + 1), *TempString) != CodePageTable->TransUniDefaultChar)
                    {
                        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                        return 0;
                    }

                    TempString++;
                }
                else
                {
                    WideChar = MultiByteTable[(UCHAR)*TempString];

                    if ((WideChar == CodePageTable->UniDefaultChar &&
                        *TempString != CodePageTable->TransUniDefaultChar) ||
                        /* "Private Use" characters */
                        (WideChar >= 0xE000 && WideChar <= 0xF8FF))
                    {
                        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                        return 0;
                    }
                }

                TempString++;
            }
        }

        /* Does caller query for output buffer size? */
        if (WideCharCount == 0)
        {
            for (; MultiByteString < MbsEnd; WideCharCount++)
            {
                Char = *MultiByteString++;

                DBCSOffset = CodePageTable->DBCSOffsets[Char];

                if (!DBCSOffset)
                    continue;

                if (MultiByteString < MbsEnd)
                    MultiByteString++;
            }

            return WideCharCount;
        }

        for (Count = 0; Count < WideCharCount && MultiByteString < MbsEnd; Count++)
        {
            Char = *MultiByteString++;

            DBCSOffset = CodePageTable->DBCSOffsets[Char];

            if (!DBCSOffset)
            {
                *WideCharString++ = MultiByteTable[Char];
                continue;
            }

            if (MultiByteString == MbsEnd || *MultiByteString == 0)
            {
                *WideCharString++ = CodePageTable->UniDefaultChar;
            }
            else
            {
                *WideCharString++ = CodePageTable->DBCSOffsets[DBCSOffset + (UCHAR)*MultiByteString++];
            }
        }

        if (MultiByteString < MbsEnd)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }

        return Count;
    }
    else /* SBCS code page */
    {
        /* Check for invalid characters. */
        if (Flags & MB_ERR_INVALID_CHARS)
        {
            for (TempString = MultiByteString, TempLength = MultiByteCount;
                 TempLength > 0;
                 TempString++, TempLength--)
            {
                WideChar = MultiByteTable[(UCHAR)*TempString];

                if ((WideChar == CodePageTable->UniDefaultChar &&
                    *TempString != CodePageTable->TransUniDefaultChar) ||
                    /* "Private Use" characters */
                    (WideChar >= 0xE000 && WideChar <= 0xF8FF))
                {
                    SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                    return 0;
                }
            }
        }

        /* Does caller query for output buffer size? */
        if (WideCharCount == 0)
            return MultiByteCount;

        /* Fill the WideCharString buffer with what will fit: Verified on WinXP */
        for (TempLength = (WideCharCount < MultiByteCount) ? WideCharCount : MultiByteCount;
            TempLength > 0;
            MultiByteString++, TempLength--)
        {
            *WideCharString++ = MultiByteTable[(UCHAR)*MultiByteString];
        }

        /* Adjust buffer size. Wine trick ;-) */
        if (WideCharCount < MultiByteCount)
        {
            MultiByteCount = WideCharCount;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        return MultiByteCount;
    }
}

/**
 * @name IntMultiByteToWideCharSYMBOL
 *
 * Internal version of MultiByteToWideChar for SYMBOL.
 *
 * @see MultiByteToWideChar
 */

static
INT
WINAPI
IntMultiByteToWideCharSYMBOL(DWORD Flags,
                             LPCSTR MultiByteString,
                             INT MultiByteCount,
                             LPWSTR WideCharString,
                             INT WideCharCount)
{
    LONG Count;
    UCHAR Char;
    INT WideCharMaxLen;


    if (Flags != 0)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    if (WideCharCount == 0)
    {
        return MultiByteCount;
    }

    WideCharMaxLen = WideCharCount > MultiByteCount ? MultiByteCount : WideCharCount;

    for (Count = 0; Count < WideCharMaxLen; Count++)
    {
        Char = MultiByteString[Count];
        if ( Char < 0x20 )
        {
            WideCharString[Count] = Char;
        }
        else
        {
            WideCharString[Count] = Char + 0xf000;
        }
    }
    if (MultiByteCount > WideCharMaxLen)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    return WideCharMaxLen;
}

/**
 * @name IntWideCharToMultiByteSYMBOL
 *
 * Internal version of WideCharToMultiByte for SYMBOL.
 *
 * @see WideCharToMultiByte
 */

static INT
WINAPI
IntWideCharToMultiByteSYMBOL(DWORD Flags,
                             LPCWSTR WideCharString,
                             INT WideCharCount,
                             LPSTR MultiByteString,
                             INT MultiByteCount)
{
    LONG Count;
    INT MaxLen;
    WCHAR Char;

    if (Flags!=0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }


    if (MultiByteCount == 0)
    {
        return WideCharCount;
    }

    MaxLen = MultiByteCount > WideCharCount ? WideCharCount : MultiByteCount;
    for (Count = 0; Count < MaxLen; Count++)
    {
        Char = WideCharString[Count];
        if (Char < 0x20)
        {
            MultiByteString[Count] = (CHAR)Char;
        }
        else
        {
            if ((Char >= 0xf020) && (Char < 0xf100))
            {
                MultiByteString[Count] = Char - 0xf000;
            }
            else
            {
                SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                return 0;
            }
        }
    }

    if (WideCharCount > MaxLen)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }
    return MaxLen;
}

/**
 * @name IntWideCharToMultiByteUTF8
 *
 * Internal version of WideCharToMultiByte for UTF8.
 *
 * @see WideCharToMultiByte
 */

static INT
WINAPI
IntWideCharToMultiByteUTF8(UINT CodePage,
                           DWORD Flags,
                           LPCWSTR WideCharString,
                           INT WideCharCount,
                           LPSTR MultiByteString,
                           INT MultiByteCount,
                           LPCSTR DefaultChar,
                           LPBOOL UsedDefaultChar)
{
    INT TempLength;
    DWORD Char;

    if (Flags)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    /* Does caller query for output buffer size? */
    if (MultiByteCount == 0)
    {
        for (TempLength = 0; WideCharCount;
            WideCharCount--, WideCharString++)
        {
            TempLength++;
            if (*WideCharString >= 0x80)
            {
                TempLength++;
                if (*WideCharString >= 0x800)
                {
                    TempLength++;
                    if (*WideCharString >= 0xd800 && *WideCharString < 0xdc00 &&
                        WideCharCount >= 1 &&
                        WideCharString[1] >= 0xdc00 && WideCharString[1] <= 0xe000)
                    {
                        WideCharCount--;
                        WideCharString++;
                        TempLength++;
                    }
                }
            }
        }
        return TempLength;
    }

    for (TempLength = MultiByteCount; WideCharCount; WideCharCount--, WideCharString++)
    {
        Char = *WideCharString;
        if (Char < 0x80)
        {
            if (!TempLength)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                break;
            }
            TempLength--;
            *MultiByteString++ = (CHAR)Char;
            continue;
        }

        if (Char < 0x800)  /* 0x80-0x7ff: 2 bytes */
        {
            if (TempLength < 2)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                break;
            }
            MultiByteString[1] = 0x80 | (Char & 0x3f); Char >>= 6;
            MultiByteString[0] = 0xc0 | Char;
            MultiByteString += 2;
            TempLength -= 2;
            continue;
        }

        /* surrogate pair 0x10000-0x10ffff: 4 bytes */
        if (Char >= 0xd800 && Char < 0xdc00 &&
            WideCharCount >= 1 &&
            WideCharString[1] >= 0xdc00 && WideCharString[1] < 0xe000)
        {
            WideCharCount--;
            WideCharString++;

            if (TempLength < 4)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                break;
            }

            Char = (Char - 0xd800) << 10;
            Char |= *WideCharString - 0xdc00;
            ASSERT(Char <= 0xfffff);
            Char += 0x10000;
            ASSERT(Char <= 0x10ffff);

            MultiByteString[3] = 0x80 | (Char & 0x3f); Char >>= 6;
            MultiByteString[2] = 0x80 | (Char & 0x3f); Char >>= 6;
            MultiByteString[1] = 0x80 | (Char & 0x3f); Char >>= 6;
            MultiByteString[0] = 0xf0 | Char;
            MultiByteString += 4;
            TempLength -= 4;
            continue;
        }

        /* 0x800-0xffff: 3 bytes */
        if (TempLength < 3)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            break;
        }
        MultiByteString[2] = 0x80 | (Char & 0x3f); Char >>= 6;
        MultiByteString[1] = 0x80 | (Char & 0x3f); Char >>= 6;
        MultiByteString[0] = 0xe0 | Char;
        MultiByteString += 3;
        TempLength -= 3;
    }

    return MultiByteCount - TempLength;
}

/**
 * @name IsValidSBCSMapping
 *
 * Checks if ch (single-byte character) is a valid mapping for wch
 *
 * @see IntWideCharToMultiByteCP
 */
static
inline
BOOL
IntIsValidSBCSMapping(PCPTABLEINFO CodePageTable, DWORD Flags, WCHAR wch, UCHAR ch)
{
    /* If the WC_NO_BEST_FIT_CHARS flag has been specified, the characters need to match exactly. */
    if (Flags & WC_NO_BEST_FIT_CHARS)
        return (CodePageTable->MultiByteTable[ch] == wch);

    /* By default, all characters except TransDefaultChar apply as a valid mapping
       for ch (so also "nearest" characters) */
    if (ch != CodePageTable->TransDefaultChar)
        return TRUE;

    /* The only possible left valid mapping is the default character itself */
    return (wch == CodePageTable->TransUniDefaultChar);
}

/**
 * @name IsValidDBCSMapping
 *
 * Checks if ch (double-byte character) is a valid mapping for wch
 *
 * @see IntWideCharToMultiByteCP
 */
static inline BOOL
IntIsValidDBCSMapping(PCPTABLEINFO CodePageTable, DWORD Flags, WCHAR wch, USHORT ch)
{
    /* If ch is the default character, but the wch is not, it can't be a valid mapping */
    if (ch == CodePageTable->TransDefaultChar && wch != CodePageTable->TransUniDefaultChar)
        return FALSE;

    /* If the WC_NO_BEST_FIT_CHARS flag has been specified, the characters need to match exactly. */
    if (Flags & WC_NO_BEST_FIT_CHARS)
    {
        if(ch & 0xff00)
        {
            USHORT uOffset = CodePageTable->DBCSOffsets[ch >> 8];
            /* if (!uOffset) return (CodePageTable->MultiByteTable[ch] == wch); */
            return (CodePageTable->DBCSOffsets[uOffset + (ch & 0xff)] == wch);
        }

        return (CodePageTable->MultiByteTable[ch] == wch);
    }

    /* If we're still here, we have a valid mapping */
    return TRUE;
}

/**
 * @name IntWideCharToMultiByteCP
 *
 * Internal version of WideCharToMultiByte for code page tables.
 *
 * @see WideCharToMultiByte
 * @todo Handle WC_COMPOSITECHECK
 */
static
INT
WINAPI
IntWideCharToMultiByteCP(UINT CodePage,
                         DWORD Flags,
                         LPCWSTR WideCharString,
                         INT WideCharCount,
                         LPSTR MultiByteString,
                         INT MultiByteCount,
                         LPCSTR DefaultChar,
                         LPBOOL UsedDefaultChar)
{
    PCODEPAGE_ENTRY CodePageEntry;
    PCPTABLEINFO CodePageTable;
    INT TempLength;

    /* Get code page table. */
    CodePageEntry = IntGetCodePageEntry(CodePage);
    if (CodePageEntry == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    CodePageTable = &CodePageEntry->CodePageTable;


    /* Different handling for DBCS code pages. */
    if (CodePageTable->DBCSCodePage)
    {
        /* If Flags, DefaultChar or UsedDefaultChar were given, we have to do some more work */
        if (Flags || DefaultChar || UsedDefaultChar)
        {
            BOOL TempUsedDefaultChar;
            USHORT DefChar;

            /* If UsedDefaultChar is not set, set it to a temporary value, so we don't have
               to check on every character */
            if (!UsedDefaultChar)
                UsedDefaultChar = &TempUsedDefaultChar;

            *UsedDefaultChar = FALSE;

            /* Use the CodePage's TransDefaultChar if none was given. Don't modify the DefaultChar pointer here. */
            if (DefaultChar)
                DefChar = DefaultChar[1] ? ((DefaultChar[0] << 8) | DefaultChar[1]) : DefaultChar[0];
            else
                DefChar = CodePageTable->TransDefaultChar;

            /* Does caller query for output buffer size? */
            if (!MultiByteCount)
            {
                for (TempLength = 0; WideCharCount; WideCharCount--, WideCharString++, TempLength++)
                {
                    USHORT uChar;

                    if ((Flags & WC_COMPOSITECHECK) && WideCharCount > 1)
                    {
                        /* FIXME: Handle WC_COMPOSITECHECK */
                        DPRINT("WC_COMPOSITECHECK flag UNIMPLEMENTED\n");
                    }

                    uChar = ((PUSHORT) CodePageTable->WideCharTable)[*WideCharString];

                    /* Verify if the mapping is valid for handling DefaultChar and UsedDefaultChar */
                    if (!IntIsValidDBCSMapping(CodePageTable, Flags, *WideCharString, uChar))
                    {
                        uChar = DefChar;
                        *UsedDefaultChar = TRUE;
                    }

                    /* Increment TempLength again if this is a double-byte character */
                    if (uChar & 0xff00)
                        TempLength++;
                }

                return TempLength;
            }

            /* Convert the WideCharString to the MultiByteString and verify if the mapping is valid */
            for (TempLength = MultiByteCount;
                 WideCharCount && TempLength;
                 TempLength--, WideCharString++, WideCharCount--)
            {
                USHORT uChar;

                if ((Flags & WC_COMPOSITECHECK) && WideCharCount > 1)
                {
                    /* FIXME: Handle WC_COMPOSITECHECK */
                    DPRINT("WC_COMPOSITECHECK flag UNIMPLEMENTED\n");
                }

                uChar = ((PUSHORT)CodePageTable->WideCharTable)[*WideCharString];

                /* Verify if the mapping is valid for handling DefaultChar and UsedDefaultChar */
                if (!IntIsValidDBCSMapping(CodePageTable, Flags, *WideCharString, uChar))
                {
                    uChar = DefChar;
                    *UsedDefaultChar = TRUE;
                }

                /* Handle double-byte characters */
                if (uChar & 0xff00)
                {
                    /* Don't output a partial character */
                    if (TempLength == 1)
                        break;

                    TempLength--;
                    *MultiByteString++ = uChar >> 8;
                }

                *MultiByteString++ = (char)uChar;
            }

            /* WideCharCount should be 0 if all characters were converted */
            if (WideCharCount)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }

            return MultiByteCount - TempLength;
        }

        /* Does caller query for output buffer size? */
        if (!MultiByteCount)
        {
            for (TempLength = 0; WideCharCount; WideCharCount--, WideCharString++, TempLength++)
            {
                /* Increment TempLength again if this is a double-byte character */
                if (((PWCHAR)CodePageTable->WideCharTable)[*WideCharString] & 0xff00)
                    TempLength++;
            }

            return TempLength;
        }

        /* Convert the WideCharString to the MultiByteString */
        for (TempLength = MultiByteCount;
             WideCharCount && TempLength;
             TempLength--, WideCharString++, WideCharCount--)
        {
            USHORT uChar = ((PUSHORT) CodePageTable->WideCharTable)[*WideCharString];

            /* Is this a double-byte character? */
            if (uChar & 0xff00)
            {
                /* Don't output a partial character */
                if (TempLength == 1)
                    break;

                TempLength--;
                *MultiByteString++ = uChar >> 8;
            }

            *MultiByteString++ = (char)uChar;
        }

        /* WideCharCount should be 0 if all characters were converted */
        if (WideCharCount)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }

        return MultiByteCount - TempLength;
    }
    else /* SBCS code page */
    {
        INT nReturn;

        /* If Flags, DefaultChar or UsedDefaultChar were given, we have to do some more work */
        if (Flags || DefaultChar || UsedDefaultChar)
        {
            BOOL TempUsedDefaultChar;
            CHAR DefChar;

            /* If UsedDefaultChar is not set, set it to a temporary value, so we don't have
               to check on every character */
            if (!UsedDefaultChar)
                UsedDefaultChar = &TempUsedDefaultChar;

            *UsedDefaultChar = FALSE;

            /* Does caller query for output buffer size? */
            if (!MultiByteCount)
            {
                /* Loop through the whole WideCharString and check if we can get a valid mapping for each character */
                for (TempLength = 0; WideCharCount; TempLength++, WideCharString++, WideCharCount--)
                {
                    if ((Flags & WC_COMPOSITECHECK) && WideCharCount > 1)
                    {
                        /* FIXME: Handle WC_COMPOSITECHECK */
                        DPRINT("WC_COMPOSITECHECK flag UNIMPLEMENTED\n");
                    }

                    if (!*UsedDefaultChar)
                        *UsedDefaultChar = !IntIsValidSBCSMapping(CodePageTable,
                                                                  Flags,
                                                                  *WideCharString,
                                                                  ((PCHAR)CodePageTable->WideCharTable)[*WideCharString]);
                }

                return TempLength;
            }

            /* Use the CodePage's TransDefaultChar if none was given. Don't modify the DefaultChar pointer here. */
            if (DefaultChar)
                DefChar = *DefaultChar;
            else
                DefChar = (CHAR)CodePageTable->TransDefaultChar;

            /* Convert the WideCharString to the MultiByteString and verify if the mapping is valid */
            for (TempLength = MultiByteCount;
                 WideCharCount && TempLength;
                 MultiByteString++, TempLength--, WideCharString++, WideCharCount--)
            {
                if ((Flags & WC_COMPOSITECHECK) && WideCharCount > 1)
                {
                    /* FIXME: Handle WC_COMPOSITECHECK */
                    DPRINT("WC_COMPOSITECHECK flag UNIMPLEMENTED\n");
                }

                *MultiByteString = ((PCHAR)CodePageTable->WideCharTable)[*WideCharString];

                if (!IntIsValidSBCSMapping(CodePageTable, Flags, *WideCharString, *MultiByteString))
                {
                    *MultiByteString = DefChar;
                    *UsedDefaultChar = TRUE;
                }
            }

            /* WideCharCount should be 0 if all characters were converted */
            if (WideCharCount)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }

            return MultiByteCount - TempLength;
        }

        /* Does caller query for output buffer size? */
        if (!MultiByteCount)
            return WideCharCount;

        /* Is the buffer large enough? */
        if (MultiByteCount < WideCharCount)
        {
            /* Convert the string up to MultiByteCount and return 0 */
            WideCharCount = MultiByteCount;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            nReturn = 0;
        }
        else
        {
            /* Otherwise WideCharCount will be the number of converted characters */
            nReturn = WideCharCount;
        }

        /* Convert the WideCharString to the MultiByteString */
        for (TempLength = WideCharCount; --TempLength >= 0; WideCharString++, MultiByteString++)
        {
            *MultiByteString = ((PCHAR)CodePageTable->WideCharTable)[*WideCharString];
        }

        return nReturn;
    }
}

/**
 * @name IntIsLeadByte
 *
 * Internal function to detect if byte is lead byte in specific character
 * table.
 */

static BOOL
WINAPI
IntIsLeadByte(PCPTABLEINFO TableInfo, BYTE Byte)
{
    UINT i;

    if (TableInfo->MaximumCharacterSize == 2)
    {
        for (i = 0; i < MAXIMUM_LEADBYTES && TableInfo->LeadByte[i]; i += 2)
        {
            if (Byte >= TableInfo->LeadByte[i] && Byte <= TableInfo->LeadByte[i+1])
                return TRUE;
        }
    }

    return FALSE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @name GetNlsSectionName
 *
 * Construct a name of NLS section.
 *
 * @param CodePage
 *        Code page number.
 * @param Base
 *        Integer base used for converting to string. Usually set to 10.
 * @param Unknown
 *        As the name suggests the meaning of this parameter is unknown.
 *        The native version of Kernel32 passes it as the third parameter
 *        to NlsConvertIntegerToString function, which is used for the
 *        actual conversion of the code page number.
 * @param BaseName
 *        Base name of the section. (ex. "\\Nls\\NlsSectionCP")
 * @param Result
 *        Buffer that will hold the constructed name.
 * @param ResultSize
 *        Size of the buffer for the result.
 *
 * @return TRUE if the buffer was large enough and was filled with
 *         the requested information, FALSE otherwise.
 *
 * @implemented
 */

BOOL
WINAPI
GetNlsSectionName(UINT CodePage,
                  UINT Base,
                  ULONG Unknown,
                  LPSTR BaseName,
                  LPSTR Result,
                  ULONG ResultSize)
{
    CHAR Integer[11];

    if (!NT_SUCCESS(RtlIntegerToChar(CodePage, Base, sizeof(Integer), Integer)))
        return FALSE;

    /*
     * If the name including the terminating NULL character doesn't
     * fit in the output buffer then fail.
     */
    if (strlen(Integer) + strlen(BaseName) >= ResultSize)
        return FALSE;

    lstrcpyA(Result, BaseName);
    lstrcatA(Result, Integer);

    return TRUE;
}

/**
 * @name GetCPFileNameFromRegistry
 *
 * Get file name of code page definition file.
 *
 * @param CodePage
 *        Code page number to get file name of.
 * @param FileName
 *        Buffer that is filled with file name of successful return. Can
 *        be set to NULL.
 * @param FileNameSize
 *        Size of the buffer to hold file name in WCHARs.
 *
 * @return TRUE if the file name was retrieved, FALSE otherwise.
 *
 * @implemented
 */

BOOL
WINAPI
GetCPFileNameFromRegistry(UINT CodePage, LPWSTR FileName, ULONG FileNameSize)
{
    WCHAR ValueNameBuffer[11];
    UNICODE_STRING KeyName, ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION Kvpi;
    DWORD KvpiSize;
    BOOL bRetValue;

    bRetValue = FALSE;

    /* Convert the codepage number to string. */
    ValueName.Buffer = ValueNameBuffer;
    ValueName.MaximumLength = sizeof(ValueNameBuffer);

    if (!NT_SUCCESS(RtlIntegerToUnicodeString(CodePage, 10, &ValueName)))
        return bRetValue;

    /* Open the registry key containing file name mappings. */
    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\System\\"
                         L"CurrentControlSet\\Control\\Nls\\CodePage");
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE,
                               NULL, NULL);
    Status = NtOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return bRetValue;
    }

    /* Allocate buffer that will be used to query the value data. */
    KvpiSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + (MAX_PATH * sizeof(WCHAR));
    Kvpi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, KvpiSize);
    if (Kvpi == NULL)
    {
        NtClose(KeyHandle);
        return bRetValue;
    }

    /* Query the file name for our code page. */
    Status = NtQueryValueKey(KeyHandle, &ValueName, KeyValuePartialInformation,
                             Kvpi, KvpiSize, &KvpiSize);

    NtClose(KeyHandle);

    /* Check if we succeded and the value is non-empty string. */
    if (NT_SUCCESS(Status) && Kvpi->Type == REG_SZ &&
        Kvpi->DataLength > sizeof(WCHAR))
    {
        bRetValue = TRUE;
        if (FileName != NULL)
        {
            lstrcpynW(FileName, (WCHAR*)Kvpi->Data,
                      min(Kvpi->DataLength / sizeof(WCHAR), FileNameSize));
        }
    }

    /* free temporary buffer */
    HeapFree(GetProcessHeap(),0,Kvpi);
    return bRetValue;
}

/**
 * @name IsValidCodePage
 *
 * Detect if specified code page is valid and present in the system.
 *
 * @param CodePage
 *        Code page number to query.
 *
 * @return TRUE if code page is present.
 */

BOOL
WINAPI
IsValidCodePage(UINT CodePage)
{
    if (CodePage == 0) return FALSE;
    if (CodePage == CP_UTF8 || CodePage == CP_UTF7)
        return TRUE;
    if (IntGetLoadedCodePageEntry(CodePage))
        return TRUE;
    return GetCPFileNameFromRegistry(CodePage, NULL, 0);
}

static inline BOOL utf7_write_w(WCHAR *dst, int dstlen, int *index, WCHAR character)
{
    if (dstlen > 0)
    {
        if (*index >= dstlen)
            return FALSE;

        dst[*index] = character;
    }

    (*index)++;

    return TRUE;
}

static INT Utf7ToWideChar(const char *src, int srclen, WCHAR *dst, int dstlen)
{
    static const signed char base64_decoding_table[] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00-0x0F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10-0x1F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 0x20-0x2F */
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 0x30-0x3F */
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 0x40-0x4F */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 0x50-0x5F */
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 0x60-0x6F */
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  /* 0x70-0x7F */
    };

    const char *source_end = src + srclen;
    int dest_index = 0;

    DWORD byte_pair = 0;
    short offset = 0;

    while (src < source_end)
    {
        if (*src == '+')
        {
            src++;
            if (src >= source_end)
                break;

            if (*src == '-')
            {
                /* just a plus sign escaped as +- */
                if (!utf7_write_w(dst, dstlen, &dest_index, '+'))
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return 0;
                }
                src++;
                continue;
            }

            do
            {
                signed char sextet = *src;
                if (sextet == '-')
                {
                    /* skip over the dash and end base64 decoding
                     * the current, unfinished byte pair is discarded */
                    src++;
                    offset = 0;
                    break;
                }
                if (sextet < 0)
                {
                    /* the next character of src is < 0 and therefore not part of a base64 sequence
                     * the current, unfinished byte pair is NOT discarded in this case
                     * this is probably a bug in Windows */
                    break;
                }

                sextet = base64_decoding_table[sextet];
                if (sextet == -1)
                {
                    /* -1 means that the next character of src is not part of a base64 sequence
                     * in other words, all sextets in this base64 sequence have been processed
                     * the current, unfinished byte pair is discarded */
                    offset = 0;
                    break;
                }

                byte_pair = (byte_pair << 6) | sextet;
                offset += 6;

                if (offset >= 16)
                {
                    /* this byte pair is done */
                    if (!utf7_write_w(dst, dstlen, &dest_index, (byte_pair >> (offset - 16)) & 0xFFFF))
                    {
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return 0;
                    }
                    offset -= 16;
                }

                src++;
            }
            while (src < source_end);
        }
        else
        {
            /* we have to convert to unsigned char in case *src < 0 */
            if (!utf7_write_w(dst, dstlen, &dest_index, (unsigned char)*src))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            src++;
        }
    }

    return dest_index;
}

/**
 * @name MultiByteToWideChar
 *
 * Convert a multi-byte string to wide-charater equivalent.
 *
 * @param CodePage
 *        Code page to be used to perform the conversion. It can be also
 *        one of the special values (CP_ACP for ANSI code page, CP_MACCP
 *        for Macintosh code page, CP_OEMCP for OEM code page, CP_THREAD_ACP
 *        for thread active code page, CP_UTF7 or CP_UTF8).
 * @param Flags
 *        Additional conversion flags (MB_PRECOMPOSED, MB_COMPOSITE,
 *        MB_ERR_INVALID_CHARS, MB_USEGLYPHCHARS).
 * @param MultiByteString
 *        Input buffer.
 * @param MultiByteCount
 *        Size of MultiByteString, or -1 if MultiByteString is NULL
 *        terminated.
 * @param WideCharString
 *        Output buffer.
 * @param WideCharCount
 *        Size in WCHARs of WideCharString, or 0 if the caller just wants
 *        to know how large WideCharString should be for a successful
 *        conversion.
 *
 * @return Zero on error, otherwise the number of WCHARs written
 *         in the WideCharString buffer.
 *
 * @implemented
 */
INT
WINAPI
MultiByteToWideChar(UINT CodePage,
                    DWORD Flags,
                    LPCSTR MultiByteString,
                    INT MultiByteCount,
                    LPWSTR WideCharString,
                    INT WideCharCount)
{
    /* Check the parameters. */
    if (MultiByteString == NULL ||
        MultiByteCount == 0 || WideCharCount < 0 ||
        (WideCharCount && (WideCharString == NULL ||
        (PVOID)MultiByteString == (PVOID)WideCharString)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Determine the input string length. */
    if (MultiByteCount < 0)
    {
        MultiByteCount = lstrlenA(MultiByteString) + 1;
    }

    switch (CodePage)
    {
        case CP_UTF8:
            return IntMultiByteToWideCharUTF8(Flags,
                                              MultiByteString,
                                              MultiByteCount,
                                              WideCharString,
                                              WideCharCount);

        case CP_UTF7:
            if (Flags)
            {
                SetLastError(ERROR_INVALID_FLAGS);
                return 0;
            }
            return Utf7ToWideChar(MultiByteString, MultiByteCount,
                                  WideCharString, WideCharCount);

        case CP_SYMBOL:
            return IntMultiByteToWideCharSYMBOL(Flags,
                                                MultiByteString,
                                                MultiByteCount,
                                                WideCharString,
                                                WideCharCount);
        default:
            return IntMultiByteToWideCharCP(CodePage,
                                            Flags,
                                            MultiByteString,
                                            MultiByteCount,
                                            WideCharString,
                                            WideCharCount);
    }
}

static inline BOOL utf7_can_directly_encode(WCHAR codepoint)
{
    static const BOOL directly_encodable_table[] =
    {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, /* 0x00 - 0x0F */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 - 0x1F */
        1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, /* 0x20 - 0x2F */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 0x30 - 0x3F */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x40 - 0x4F */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 0x50 - 0x5F */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x60 - 0x6F */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1                 /* 0x70 - 0x7A */
    };

    return codepoint <= 0x7A ? directly_encodable_table[codepoint] : FALSE;
}

static inline BOOL utf7_write_c(char *dst, int dstlen, int *index, char character)
{
    if (dstlen > 0)
    {
        if (*index >= dstlen)
            return FALSE;

        dst[*index] = character;
    }

    (*index)++;

    return TRUE;
}

static INT WideCharToUtf7(const WCHAR *src, int srclen, char *dst, int dstlen)
{
    static const char base64_encoding_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const WCHAR *source_end = src + srclen;
    int dest_index = 0;

    while (src < source_end)
    {
        if (*src == '+')
        {
            if (!utf7_write_c(dst, dstlen, &dest_index, '+'))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            if (!utf7_write_c(dst, dstlen, &dest_index, '-'))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            src++;
        }
        else if (utf7_can_directly_encode(*src))
        {
            if (!utf7_write_c(dst, dstlen, &dest_index, *src))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            src++;
        }
        else
        {
            unsigned int offset = 0;
            DWORD byte_pair = 0;

            if (!utf7_write_c(dst, dstlen, &dest_index, '+'))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }

            while (src < source_end && !utf7_can_directly_encode(*src))
            {
                byte_pair = (byte_pair << 16) | *src;
                offset += 16;
                while (offset >= 6)
                {
                    if (!utf7_write_c(dst, dstlen, &dest_index, base64_encoding_table[(byte_pair >> (offset - 6)) & 0x3F]))
                    {
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return 0;
                    }
                    offset -= 6;
                }
                src++;
            }

            if (offset)
            {
                /* Windows won't create a padded base64 character if there's no room for the - sign
                 * as well ; this is probably a bug in Windows */
                if (dstlen > 0 && dest_index + 1 >= dstlen)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return 0;
                }

                byte_pair <<= (6 - offset);
                if (!utf7_write_c(dst, dstlen, &dest_index, base64_encoding_table[byte_pair & 0x3F]))
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return 0;
                }
            }

            /* Windows always explicitly terminates the base64 sequence
               even though RFC 2152 (page 3, rule 2) does not require this */
            if (!utf7_write_c(dst, dstlen, &dest_index, '-'))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
        }
    }

    return dest_index;
}

/*
 * A function similar to LoadStringW, but adapted for usage by GetCPInfoExW
 * and GetGeoInfoW. It uses the current user localization, otherwise falls back
 * to English (US). Contrary to LoadStringW which always saves the loaded string
 * into the user-given buffer, truncating the string if needed, this function
 * returns instead an ERROR_INSUFFICIENT_BUFFER error code if the user buffer
 * is not large enough.
 */
UINT
GetLocalisedText(
    IN UINT uID,
    IN LPWSTR lpszDest,
    IN UINT cchDest,
    IN LANGID lang)
{
    HRSRC hrsrc;
    HGLOBAL hmem;
    LCID lcid;
    LANGID langId;
    const WCHAR *p;
    UINT i;

    /* See HACK in winnls/lang/xx-XX.rc files */
    if (uID == 37)
        uID = uID * 100;

    lcid = ConvertDefaultLocale(lang);

    langId = LANGIDFROMLCID(lcid);

    if (PRIMARYLANGID(langId) == LANG_NEUTRAL)
        langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    hrsrc = FindResourceExW(hCurrentModule,
                            (LPWSTR)RT_STRING,
                            MAKEINTRESOURCEW((uID >> 4) + 1),
                            langId);

    /* English fallback */
    if (!hrsrc)
    {
        hrsrc = FindResourceExW(hCurrentModule,
                                (LPWSTR)RT_STRING,
                                MAKEINTRESOURCEW((uID >> 4) + 1),
                                MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
    }

    if (!hrsrc)
        goto NotFound;

    hmem = LoadResource(hCurrentModule, hrsrc);
    if (!hmem)
        goto NotFound;

    p = LockResource(hmem);

    for (i = 0; i < (uID & 0x0F); i++)
        p += *p + 1;

    /* Needed for GetGeoInfo(): return the needed string size including the NULL terminator */
    if (cchDest == 0)
        return *p + 1;
    /* Needed for GetGeoInfo(): bail out if the user buffer is not large enough */
    if (*p + 1 > cchDest)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    i = *p;
    if (i > 0)
    {
        memcpy(lpszDest, p + 1, i * sizeof(WCHAR));
        lpszDest[i] = L'\0';
        return i;
    }
#if 0
    else
    {
        if (cchDest >= 1)
            lpszDest[0] = L'\0';
        /* Fall-back */
    }
#endif

NotFound:
    DPRINT1("Resource not found: uID = %lu\n", uID);
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCPInfo(UINT CodePage,
          LPCPINFO CodePageInfo)
{
    PCODEPAGE_ENTRY CodePageEntry;

    if (!CodePageInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CodePageEntry = IntGetCodePageEntry(CodePage);
    if (CodePageEntry == NULL)
    {
        switch(CodePage)
        {
            case CP_UTF7:
            case CP_UTF8:
                CodePageInfo->DefaultChar[0] = 0x3f;
                CodePageInfo->DefaultChar[1] = 0;
                CodePageInfo->LeadByte[0] = CodePageInfo->LeadByte[1] = 0;
                CodePageInfo->MaxCharSize = (CodePage == CP_UTF7) ? 5 : 4;
                return TRUE;
        }

        DPRINT1("Invalid CP!: %lx\n", CodePage);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (CodePageEntry->CodePageTable.DefaultChar & 0xff00)
    {
        CodePageInfo->DefaultChar[0] = (CodePageEntry->CodePageTable.DefaultChar & 0xff00) >> 8;
        CodePageInfo->DefaultChar[1] = CodePageEntry->CodePageTable.DefaultChar & 0x00ff;
    }
    else
    {
        CodePageInfo->DefaultChar[0] = CodePageEntry->CodePageTable.DefaultChar & 0xff;
        CodePageInfo->DefaultChar[1] = 0;
    }

    if ((CodePageInfo->MaxCharSize = CodePageEntry->CodePageTable.MaximumCharacterSize) == 2)
        memcpy(CodePageInfo->LeadByte, CodePageEntry->CodePageTable.LeadByte, sizeof(CodePageInfo->LeadByte));
    else
        CodePageInfo->LeadByte[0] = CodePageInfo->LeadByte[1] = 0;

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCPInfoExW(UINT CodePage,
             DWORD dwFlags,
             LPCPINFOEXW lpCPInfoEx)
{
    if (!GetCPInfo(CodePage, (LPCPINFO)lpCPInfoEx))
        return FALSE;

    switch(CodePage)
    {
        case CP_UTF7:
        {
            lpCPInfoEx->CodePage = CP_UTF7;
            lpCPInfoEx->UnicodeDefaultChar = 0x3f;
            return GetLocalisedText(lpCPInfoEx->CodePage,
                                    lpCPInfoEx->CodePageName,
                                    ARRAYSIZE(lpCPInfoEx->CodePageName),
                                    GetThreadLocale()) != 0;
        }
        break;

        case CP_UTF8:
        {
            lpCPInfoEx->CodePage = CP_UTF8;
            lpCPInfoEx->UnicodeDefaultChar = 0x3f;
            return GetLocalisedText(lpCPInfoEx->CodePage,
                                    lpCPInfoEx->CodePageName,
                                    ARRAYSIZE(lpCPInfoEx->CodePageName),
                                    GetThreadLocale()) != 0;
        }

        default:
        {
            PCODEPAGE_ENTRY CodePageEntry;

            CodePageEntry = IntGetCodePageEntry(CodePage);
            if (CodePageEntry == NULL)
            {
                DPRINT1("Could not get CodePage Entry! CodePageEntry = NULL\n");
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            lpCPInfoEx->CodePage = CodePageEntry->CodePageTable.CodePage;
            lpCPInfoEx->UnicodeDefaultChar = CodePageEntry->CodePageTable.UniDefaultChar;
            return GetLocalisedText(lpCPInfoEx->CodePage,
                                    lpCPInfoEx->CodePageName,
                                    ARRAYSIZE(lpCPInfoEx->CodePageName),
                                    GetThreadLocale()) != 0;
        }
        break;
    }
}


/*
 * @implemented
 */
BOOL
WINAPI
GetCPInfoExA(UINT CodePage,
             DWORD dwFlags,
             LPCPINFOEXA lpCPInfoEx)
{
    CPINFOEXW CPInfo;

    if (!GetCPInfoExW(CodePage, dwFlags, &CPInfo))
        return FALSE;

    /* the layout is the same except for CodePageName */
    memcpy(lpCPInfoEx, &CPInfo, sizeof(CPINFOEXA));

    WideCharToMultiByte(CP_ACP,
                        0,
                        CPInfo.CodePageName,
                        -1,
                        lpCPInfoEx->CodePageName,
                        sizeof(lpCPInfoEx->CodePageName),
                        NULL,
                        NULL);
    return TRUE;
}

/**
 * @name WideCharToMultiByte
 *
 * Convert a wide-charater string to closest multi-byte equivalent.
 *
 * @param CodePage
 *        Code page to be used to perform the conversion. It can be also
 *        one of the special values (CP_ACP for ANSI code page, CP_MACCP
 *        for Macintosh code page, CP_OEMCP for OEM code page, CP_THREAD_ACP
 *        for thread active code page, CP_UTF7 or CP_UTF8).
 * @param Flags
 *        Additional conversion flags (WC_NO_BEST_FIT_CHARS, WC_COMPOSITECHECK,
 *        WC_DISCARDNS, WC_SEPCHARS, WC_DEFAULTCHAR).
 * @param WideCharString
 *        Points to the wide-character string to be converted.
 * @param WideCharCount
 *        Size in WCHARs of WideCharStr, or 0 if the caller just wants to
 *        know how large WideCharString should be for a successful conversion.
 * @param MultiByteString
 *        Points to the buffer to receive the translated string.
 * @param MultiByteCount
 *        Specifies the size in bytes of the buffer pointed to by the
 *        MultiByteString parameter. If this value is zero, the function
 *        returns the number of bytes required for the buffer.
 * @param DefaultChar
 *        Points to the character used if a wide character cannot be
 *        represented in the specified code page. If this parameter is
 *        NULL, a system default value is used.
 * @param UsedDefaultChar
 *        Points to a flag that indicates whether a default character was
 *        used. This parameter can be NULL.
 *
 * @return Zero on error, otherwise the number of bytes written in the
 *         MultiByteString buffer. Or the number of bytes needed for
 *         the MultiByteString buffer if MultiByteCount is zero.
 *
 * @implemented
 */

INT
WINAPI
WideCharToMultiByte(UINT CodePage,
                    DWORD Flags,
                    LPCWSTR WideCharString,
                    INT WideCharCount,
                    LPSTR MultiByteString,
                    INT MultiByteCount,
                    LPCSTR DefaultChar,
                    LPBOOL UsedDefaultChar)
{
    /* Check the parameters. */
    if (WideCharString == NULL ||
        WideCharCount == 0 ||
        (MultiByteString == NULL && MultiByteCount > 0) ||
        (PVOID)WideCharString == (PVOID)MultiByteString ||
        MultiByteCount < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Determine the input string length. */
    if (WideCharCount < 0)
    {
        WideCharCount = lstrlenW(WideCharString) + 1;
    }

    switch (CodePage)
    {
        case CP_UTF8:
            if (DefaultChar != NULL || UsedDefaultChar != NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
            return IntWideCharToMultiByteUTF8(CodePage,
                                              Flags,
                                              WideCharString,
                                              WideCharCount,
                                              MultiByteString,
                                              MultiByteCount,
                                              DefaultChar,
                                              UsedDefaultChar);

        case CP_UTF7:
            if (DefaultChar != NULL || UsedDefaultChar != NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
            if (Flags)
            {
                SetLastError(ERROR_INVALID_FLAGS);
                return 0;
            }
            return WideCharToUtf7(WideCharString, WideCharCount,
                                  MultiByteString, MultiByteCount);

        case CP_SYMBOL:
            if ((DefaultChar!=NULL) || (UsedDefaultChar!=NULL))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
            return IntWideCharToMultiByteSYMBOL(Flags,
                                                WideCharString,
                                                WideCharCount,
                                                MultiByteString,
                                                MultiByteCount);

        default:
            return IntWideCharToMultiByteCP(CodePage,
                                            Flags,
                                            WideCharString,
                                            WideCharCount,
                                            MultiByteString,
                                            MultiByteCount,
                                            DefaultChar,
                                            UsedDefaultChar);
   }
}

/**
 * @name GetACP
 *
 * Get active ANSI code page number.
 *
 * @implemented
 */

UINT
WINAPI
GetACP(VOID)
{
    return AnsiCodePage.CodePageTable.CodePage;
}

/**
 * @name GetOEMCP
 *
 * Get active OEM code page number.
 *
 * @implemented
 */

UINT
WINAPI
GetOEMCP(VOID)
{
    return OemCodePage.CodePageTable.CodePage;
}

/**
 * @name IsDBCSLeadByteEx
 *
 * Determine if passed byte is lead byte in specified code page.
 *
 * @implemented
 */

BOOL
WINAPI
IsDBCSLeadByteEx(UINT CodePage, BYTE TestByte)
{
    PCODEPAGE_ENTRY CodePageEntry;

    CodePageEntry = IntGetCodePageEntry(CodePage);
    if (CodePageEntry != NULL)
        return IntIsLeadByte(&CodePageEntry->CodePageTable, TestByte);

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/**
 * @name IsDBCSLeadByteEx
 *
 * Determine if passed byte is lead byte in current ANSI code page.
 *
 * @implemented
 */

BOOL
WINAPI
IsDBCSLeadByte(BYTE TestByte)
{
    return IntIsLeadByte(&AnsiCodePage.CodePageTable, TestByte);
}

/*
 * @unimplemented
 */
NTSTATUS WINAPI CreateNlsSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,ULONG Size,ULONG AccessMask)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI IsValidUILanguage(LANGID langid)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID WINAPI NlsConvertIntegerToString(ULONG Value,ULONG Base,ULONG strsize, LPWSTR str, ULONG strsize2)
{
    STUB;
}

/*
 * @unimplemented
 */
UINT WINAPI SetCPGlobal(UINT CodePage)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
ValidateLCType(int a1, unsigned int a2, int a3, int a4)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
NlsResetProcessLocale(VOID)
{
    STUB;
    return TRUE;
}

/*
 * @unimplemented
 */
VOID
WINAPI
GetDefaultSortkeySize(LPVOID lpUnknown)
{
    STUB;
    lpUnknown = NULL;
}

/*
 * @unimplemented
 */
VOID
WINAPI
GetLinguistLangSize(LPVOID lpUnknown)
{
    STUB;
    lpUnknown = NULL;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
ValidateLocale(IN ULONG LocaleId)
{
    STUB;
    return TRUE;
}

/*
 * @unimplemented
 */
ULONG
WINAPI
NlsGetCacheUpdateCount(VOID)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IsNLSDefinedString(IN NLS_FUNCTION Function,
                   IN DWORD dwFlags,
                   IN LPNLSVERSIONINFO lpVersionInformation,
                   IN LPCWSTR lpString,
                   IN INT cchStr)
{
    STUB;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetNLSVersion(IN NLS_FUNCTION Function,
              IN LCID Locale,
              IN OUT LPNLSVERSIONINFO lpVersionInformation)
{
    STUB;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetNLSVersionEx(IN NLS_FUNCTION function,
                IN LPCWSTR lpLocaleName,
                IN OUT LPNLSVERSIONINFOEX lpVersionInformation)
{
    STUB;
    return TRUE;
}

/* EOF */
