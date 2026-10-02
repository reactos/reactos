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

NTSTATUS
WINAPI
CreateNlsSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SIZE_T DescriptorSize,
    _In_ ULONG AccessMask);

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Creates a security descriptor for the NLS object directory.
 *
 * @param[out]  SecurityDescriptor
 * @param[in]   DescriptorSize
 * Same parameters as for CreateNlsSecurityDescriptor().
 *
 * @remark
 * Everyone (World SID) is given read access to the NLS directory,
 * whereas Admins are given full access.
 */
static NTSTATUS
CreateNlsDirectorySecurity(
    _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SIZE_T DescriptorSize)
{
    static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    NTSTATUS Status;
    PSID AdminsSid;
    PACL Dacl;
    BOOLEAN DaclPresent, DaclDefaulted;

    /* Give everyone basic directory access */
    Status = CreateNlsSecurityDescriptor(SecurityDescriptor,
                                         DescriptorSize,
                                         DIRECTORY_TRAVERSE | DIRECTORY_CREATE_OBJECT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create basic NLS SD (Status 0x%08x)\n", Status);
        return Status;
    }

    /* Create the Admins SID */
    // NOTE: Win <= 2k3 uses SYSTEM instead (SECURITY_LOCAL_SYSTEM_RID with one SubAuthority)
    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &AdminsSid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create Admins SID (Status 0x%08x)\n", Status);
        goto Quit;
    }

    /* Retrieve the DACL from the descriptor */
    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                          &DaclPresent,
                                          &Dacl,
                                          &DaclDefaulted);
    if (!NT_SUCCESS(Status) || !DaclPresent || !Dacl)
    {
        DPRINT1("Failed to get DACL from descriptor (Status 0x%08x)\n", Status);
        goto Quit;
    }

    /* Add an allowed access ACE to the Admins SID with full access.
     * The function verifies the DACL is large enough to accommodate it. */
    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    DIRECTORY_ALL_ACCESS,
                                    AdminsSid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to add allowed access ACE for Admins SID (Status 0x%08x)\n", Status);
        goto Quit;
    }

Quit:
    RtlFreeSid(AdminsSid);
    return Status;
}


extern void init_locale(HMODULE module);

/**
 * @name NlsInit
 *
 * Internal NLS related stuff initialization.
 */

BOOL
FASTCALL
NlsInit(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING DirName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    UCHAR SecurityDescriptor[NLS_SECTION_SECURITY_DESCRIPTOR_SIZE +
                             NLS_SIZEOF_ACE_AND_SIDS(2)];

    InitializeListHead(&CodePageListHead);
    RtlInitializeCriticalSection(&CodePageListLock);

    /*
     * FIXME: Eventually this should be done only for the NLS Server
     * process, but since we don't have anything like that (yet?) we
     * always try to create the "\NLS" directory here.
     */
    RtlInitUnicodeString(&DirName, L"\\NLS");

    /* Create a security descriptor for the NLS directory */
    Status = CreateNlsDirectorySecurity(&SecurityDescriptor,
                                        sizeof(SecurityDescriptor));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create NLS directory security (Status 0x%08x)\n", Status);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               &SecurityDescriptor);

    Status = NtCreateDirectoryObject(&Handle,
                                     DIRECTORY_TRAVERSE | DIRECTORY_CREATE_OBJECT,
                                     &ObjectAttributes);
    if (NT_SUCCESS(Status))
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
    init_locale(hCurrentModule);

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
    NTSTATUS Status;
    CHAR SectionName[40];
    HANDLE SectionHandle = INVALID_HANDLE_VALUE, FileHandle;
    PBYTE SectionMapping;
    OBJECT_ATTRIBUTES ObjectAttributes;
    union
    {
        SECURITY_DESCRIPTOR AlignedSd;
        UCHAR Buffer[NLS_SECTION_SECURITY_DESCRIPTOR_SIZE];
    } SecurityDescriptor;
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

    /*
     * FIXME: IntGetCodePageEntry should not create any security
     * descriptor here but instead this responsibility should be
     * assigned to Base Server API (aka basesrv.dll). That is,
     * kernel32 must instruct basesrv.dll on creating NLS section
     * names that do not exist through API message communication.
     * However since we do not do that, let the kernel32 do the job
     * by assigning security to NLS section names for the time being...
     */
    Status = CreateNlsSecurityDescriptor(&SecurityDescriptor,
                                         sizeof(SecurityDescriptor),
                                         SECTION_MAP_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateNlsSecurityDescriptor FAILED! (Status 0x%08x)\n", Status);
        RtlLeaveCriticalSection(&CodePageListLock);
        return NULL;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               &SecurityDescriptor);

    /* Try to open the section first */
    Status = NtOpenSection(&SectionHandle,
                           SECTION_MAP_READ,
                           &ObjectAttributes);

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


/**
 * @brief
 * Creates a security descriptor for each NLS section. Typically used by
 * BASESRV to give Everyone (World SID) read access to the sections.
 *
 * @param[out]  SecurityDescriptor
 * A pointer to a correctly sized user-allocated buffer, that receives
 * a security descriptor containing one ACL with one World SID.
 * Its size should be at least equal to NLS_SECTION_SECURITY_DESCRIPTOR_SIZE.
 *
 * @param[in]   DescriptorSize
 * Size (in bytes) of the user-provided SecurityDescriptor buffer.
 *
 * @param[in]   AccessMask
 * An access mask that grants Everyone an access specific to that mask.
 *
 * @return
 * STATUS_SUCCESS is returned if the function has successfully
 * created a security descriptor for a NLS section name. Otherwise
 * a NTSTATUS failure code is returned.
 *
 * @remark
 * This implementation has to be made compatible with NT <= 5.2 in order
 * to inter-operate with BASESRV. In particular, the security descriptor
 * is a user-provided buffer correctly sized. The caller is responsible
 * to submit the exact size of the descriptor.
 **/
NTSTATUS
WINAPI
CreateNlsSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SIZE_T DescriptorSize,
    _In_ ULONG AccessMask)
{
    static SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    NTSTATUS Status;
    PSID WorldSid;
    PACL Dacl;
    ULONG DaclSize;

    if (DescriptorSize < NLS_SECTION_SECURITY_DESCRIPTOR_SIZE)
    {
        DPRINT1("Security descriptor size too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Create the World SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
                                         1,
                                         SECURITY_WORLD_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &WorldSid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create World SID (Status 0x%08x)\n", Status);
        return Status;
    }

    /* Initialize the security descriptor */
    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create security descriptor (Status 0x%08x)\n", Status);
        goto Quit;
    }

    /* The DACL follows the security descriptor, and includes the World SID */
    Dacl = (PACL)((ULONG_PTR)SecurityDescriptor + sizeof(SECURITY_DESCRIPTOR));
    DaclSize = DescriptorSize - sizeof(SECURITY_DESCRIPTOR);

    /* Create the DACL */
    Status = RtlCreateAcl(Dacl, DaclSize, ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create DACL (Status 0x%08x)\n", Status);
        goto Quit;
    }

    /* Add an allowed access ACE to the World SID */
    Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, AccessMask, WorldSid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to add allowed access ACE for World SID (Status 0x%08x)\n", Status);
        goto Quit;
    }

    /* Set the DACL to the descriptor */
    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE, Dacl, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to set DACL into descriptor (Status 0x%08x)\n", Status);
        goto Quit;
    }

Quit:
    RtlFreeSid(WorldSid);
    return Status;
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

/* EOF */
