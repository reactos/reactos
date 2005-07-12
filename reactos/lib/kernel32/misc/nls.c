/*
 * ReactOS Kernel
 * Copyright (C) 2004 ReactOS Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/**
 * @brief National Language Support.
 * @author Filip Navara
 */

/* INCLUDES *******************************************************************/

#include <k32.h>
#define NDEBUG
#include "../include/debug.h"

/* GLOBAL VARIABLES ***********************************************************/

typedef struct _CODEPAGE_ENTRY
{
   LIST_ENTRY Entry;
   UINT CodePage;
   HANDLE SectionHandle;
   PBYTE SectionMapping;
   CPTABLEINFO CodePageTable;
} CODEPAGE_ENTRY, *PCODEPAGE_ENTRY;

/* Sequence length based on the first character. */
static const char UTF8Length[128] =
{
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x80 - 0x8F */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x90 - 0x9F */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xA0 - 0xAF */
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xB0 - 0xBF */
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0xC0 - 0xCF */
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0xD0 - 0xDF */
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 0xE0 - 0xEF */
   3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 0, 0  /* 0xF0 - 0xFF */
};

/* First byte mask depending on UTF-8 sequence length. */
static const unsigned char UTF8Mask[6] = {0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

/* FIXME: Change to HASH table or linear array. */
static LIST_ENTRY CodePageListHead;
static CODEPAGE_ENTRY AnsiCodePage;
static CODEPAGE_ENTRY OemCodePage;
static RTL_CRITICAL_SECTION CodePageListLock;

/* FORWARD DECLARATIONS *******************************************************/

BOOL STDCALL
GetNlsSectionName(UINT CodePage, UINT Base, ULONG Unknown,
                  LPSTR BaseName, LPSTR Result, ULONG ResultSize);

BOOL STDCALL
GetCPFileNameFromRegistry(UINT CodePage, LPWSTR FileName, ULONG FileNameSize);

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * Internal NLS related stuff initialization.
 */
BOOL FASTCALL
NlsInit()
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
   InitializeObjectAttributes(&ObjectAttributes, &DirName,
                              OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                              NULL, NULL);
   if (NT_SUCCESS(NtCreateDirectoryObject(&Handle, DIRECTORY_ALL_ACCESS, &ObjectAttributes)))
   {
      NtClose(Handle);
   }

   /* Setup ANSI code page. */
   AnsiCodePage.CodePage = CP_ACP;
   AnsiCodePage.SectionHandle = NULL;
   AnsiCodePage.SectionMapping = NtCurrentTeb()->ProcessEnvironmentBlock->AnsiCodePageData;
   RtlInitCodePageTable((PUSHORT)AnsiCodePage.SectionMapping,
                        &AnsiCodePage.CodePageTable);
   InsertTailList(&CodePageListHead, &AnsiCodePage.Entry);

   /* Setup OEM code page. */
   OemCodePage.CodePage = CP_OEMCP;
   OemCodePage.SectionHandle = NULL;
   OemCodePage.SectionMapping = NtCurrentTeb()->ProcessEnvironmentBlock->OemCodePageData;
   RtlInitCodePageTable((PUSHORT)OemCodePage.SectionMapping,
                        &OemCodePage.CodePageTable);
   InsertTailList(&CodePageListHead, &OemCodePage.Entry);

   return TRUE;
}

/**
 * Internal NLS related stuff uninitialization.
 */
VOID FASTCALL
NlsUninit()
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
PCODEPAGE_ENTRY FASTCALL
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
 * Internal function to get structure containing a code page information.
 *
 * @param CodePage
 *        Number of the code page. Special values like CP_OEMCP, CP_ACP
 *        or CP_THREAD_ACP are allowed, but CP_UTF[7/8] isn't.
 *
 * @return Code page entry.
 */
PCODEPAGE_ENTRY FASTCALL
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

   if (CodePage == CP_THREAD_ACP)
   {
      if (!GetLocaleInfoW(GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE |
                          LOCALE_RETURN_NUMBER, (WCHAR *)&CodePage,
                          sizeof(CodePage) / sizeof(WCHAR)))
      {
         /* Last error is set by GetLocaleInfoW. */
         return 0;
      }
   }
   else if (CodePage == CP_MACCP)
   {
      if (!GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTMACCODEPAGE |
                          LOCALE_RETURN_NUMBER, (WCHAR *)&CodePage,
                          sizeof(CodePage) / sizeof(WCHAR)))
      {
         /* Last error is set by GetLocaleInfoW. */
         return 0;
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
   if (!GetNlsSectionName(CodePage, 10, 0, "\\Nls\\NlsSectionCP",
                          SectionName, sizeof(SectionName)))
   {
      RtlLeaveCriticalSection(&CodePageListLock);
      return NULL;
   }
   RtlInitAnsiString(&AnsiName, SectionName);
   RtlAnsiStringToUnicodeString(&UnicodeName, &AnsiName, TRUE);
   InitializeObjectAttributes(&ObjectAttributes, &UnicodeName, 0,
                              NULL, NULL);

   /* Try to open the section first */
   Status = NtOpenSection(&SectionHandle, SECTION_MAP_READ, &ObjectAttributes);

   /* If the section doesn't exist, try to create it. */
   if (Status == STATUS_UNSUCCESSFUL ||
       Status == STATUS_OBJECT_NAME_NOT_FOUND ||
       Status == STATUS_OBJECT_PATH_NOT_FOUND)
   {
      FileNamePos = GetSystemDirectoryW(FileName, MAX_PATH);
      if (GetCPFileNameFromRegistry(CodePage, FileName + FileNamePos + 1,
                                    MAX_PATH - FileNamePos - 1))
      {
         FileName[FileNamePos] = L'\\';
         FileName[MAX_PATH] = 0;
         FileHandle = CreateFileW(FileName, FILE_GENERIC_READ, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, 0, NULL);
         Status = NtCreateSection(&SectionHandle, SECTION_MAP_READ,
                                  &ObjectAttributes, NULL, PAGE_READONLY,
                                  SEC_FILE, FileHandle);
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
 * Internal version of MultiByteToWideChar for UTF8.
 *
 * @see MultiByteToWideChar
 * @todo Add UTF8 validity checks.
 */
INT STDCALL
IntMultiByteToWideCharUTF8(DWORD Flags,
                           LPCSTR MultiByteString, INT MultiByteCount,
                           LPWSTR WideCharString, INT WideCharCount)
{
   LPCSTR MbsEnd;
   UCHAR Char, Length;
   WCHAR WideChar;
   LONG Count;

   if (Flags != 0)
   {
      SetLastError(ERROR_INVALID_FLAGS);
      return 0;
   }

   /* Does caller query for output buffer size? */
   if (WideCharCount == 0)
   {
      MbsEnd = MultiByteString + MultiByteCount;
      for (; MultiByteString < MbsEnd; WideCharCount++)
      {
         Char = *MultiByteString++;
         if (Char < 0xC0)
            continue;
         MultiByteString += UTF8Length[Char - 0x80];
      }
      return WideCharCount;
   }

   MbsEnd = MultiByteString + MultiByteCount;
   for (Count = 0; Count < WideCharCount && MultiByteString < MbsEnd; Count++)
   {
      Char = *MultiByteString++;
      if (Char < 0x80)
      {
         *WideCharString++ = Char;
         continue;
      }
      Length = UTF8Length[Char - 0x80];
      WideChar = Char & UTF8Mask[Length];
      while (Length && MultiByteString < MbsEnd)
      {
         WideChar = (WideChar << 6) | *MultiByteString++;
         Length--;
      }
      *WideCharString++ = WideChar;
   }

   if (MultiByteString < MbsEnd)
      SetLastError(ERROR_INSUFFICIENT_BUFFER);

   return Count;
}

/**
 * Internal version of MultiByteToWideChar for code page tables.
 *
 * @see MultiByteToWideChar
 * @todo Handle MB_PRECOMPOSED, MB_COMPOSITE, MB_USEGLYPHCHARS and
 *       DBCS codepages.
 */
INT STDCALL
IntMultiByteToWideCharCP(UINT CodePage, DWORD Flags,
                         LPCSTR MultiByteString, INT MultiByteCount,
                         LPWSTR WideCharString, INT WideCharCount)
{
   PCODEPAGE_ENTRY CodePageEntry;
   PCPTABLEINFO CodePageTable;
   LPCSTR TempString;
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
   if (CodePageTable->MaximumCharacterSize > 1)
   {
      /* UNIMPLEMENTED */
      return 0;
   }
   else
   {
      /* Check for invalid characters. */
      if (Flags & MB_ERR_INVALID_CHARS)
      {
         for (TempString = MultiByteString, TempLength = MultiByteCount;
              TempLength > 0;
              TempString++, TempLength--)
         {
            if (CodePageTable->MultiByteTable[(UCHAR)*TempString] ==
                CodePageTable->UniDefaultChar &&
                *TempString != CodePageEntry->CodePageTable.DefaultChar)
            {
               SetLastError(ERROR_NO_UNICODE_TRANSLATION);
               return 0;
            }
         }
      }

      /* Does caller query for output buffer size? */
      if (WideCharCount == 0)
         return MultiByteCount;

      /* Adjust buffer size. Wine trick ;-) */
      if (WideCharCount < MultiByteCount)
      {
         MultiByteCount = WideCharCount;
         SetLastError(ERROR_INSUFFICIENT_BUFFER);
      }

      for (TempLength = MultiByteCount;
           TempLength > 0;
           MultiByteString++, TempLength--)
      {
         *WideCharString++ = CodePageTable->MultiByteTable[(UCHAR)*MultiByteString];
      }

      return MultiByteCount;
   }
}

/**
 * Internal version of WideCharToMultiByte for UTF8.
 *
 * @see WideCharToMultiByte
 */
INT STDCALL
IntWideCharToMultiByteUTF8(UINT CodePage, DWORD Flags,
                           LPCWSTR WideCharString, INT WideCharCount,
                           LPSTR MultiByteString, INT MultiByteCount,
                           LPCSTR DefaultChar, LPBOOL UsedDefaultChar)
{
   INT TempLength;
   WCHAR Char;

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
               TempLength++;
         }
      }
      return TempLength;
   }

   for (TempLength = MultiByteCount; WideCharCount;
        WideCharCount--, WideCharString++)
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
         *MultiByteString++ = Char;
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
 * Internal version of WideCharToMultiByte for code page tables.
 *
 * @see WideCharToMultiByte
 * @todo Handle default characters and flags.
 */
INT STDCALL
IntWideCharToMultiByteCP(UINT CodePage, DWORD Flags,
                         LPCWSTR WideCharString, INT WideCharCount,
                         LPSTR MultiByteString, INT MultiByteCount,
                         LPCSTR DefaultChar, LPBOOL UsedDefaultChar)
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
   if (CodePageTable->MaximumCharacterSize > 1)
   {
      DPRINT1("WideCharToMultiByte for DBCS codepages is not implemented!\n");
      return 0;
   }
   else
   {
      /* Does caller query for output buffer size? */
      if (MultiByteCount == 0)
         return WideCharCount;

      /* Adjust buffer size. Wine trick ;-) */
      if (MultiByteCount < WideCharCount)
      {
         WideCharCount = MultiByteCount;
         SetLastError(ERROR_INSUFFICIENT_BUFFER);
      }

      for (TempLength = WideCharCount;
           TempLength > 0;
           WideCharString++, TempLength--)
      {
         *MultiByteString++ = ((PCHAR)CodePageTable->WideCharTable)[*WideCharString];
      }

      /* FIXME */
      if (UsedDefaultChar != NULL)
         *UsedDefaultChar = FALSE;

      return WideCharCount;
   }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
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
BOOL STDCALL
GetNlsSectionName(UINT CodePage, UINT Base, ULONG Unknown,
                  LPSTR BaseName, LPSTR Result, ULONG ResultSize)
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
BOOL STDCALL
GetCPFileNameFromRegistry(UINT CodePage, LPWSTR FileName, ULONG FileNameSize)
{
   WCHAR ValueNameBuffer[11];
   UNICODE_STRING KeyName, ValueName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   HANDLE KeyHandle;
   PKEY_VALUE_PARTIAL_INFORMATION Kvpi;
   DWORD KvpiSize;

   /* Convert the codepage number to string. */
   ValueName.Buffer = ValueNameBuffer;
   ValueName.MaximumLength = sizeof(ValueNameBuffer);
   if (!NT_SUCCESS(RtlIntegerToUnicodeString(CodePage, 10, &ValueName)))
      return FALSE;

   /* Open the registry key containing file name mappings. */
   RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\System\\"
                        L"CurrentControlSet\\Control\\Nls\\CodePage");
   InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE,
			      NULL, NULL);
   Status = NtOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }

   /* Allocate buffer that will be used to query the value data. */
   KvpiSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
              (MAX_PATH * sizeof(WCHAR));
   Kvpi = HeapAlloc(GetProcessHeap(), 0, KvpiSize);
   if (Kvpi == NULL)
   {
      NtClose(KeyHandle);
      return FALSE;
   }

   /* Query the file name for our code page. */
   Status = NtQueryValueKey(KeyHandle, &ValueName, KeyValuePartialInformation,
                            Kvpi, KvpiSize, &KvpiSize);

   NtClose(KeyHandle);

   /* Check if we succeded and the value is non-empty string. */
   if (NT_SUCCESS(Status) && Kvpi->Type == REG_SZ &&
       Kvpi->DataLength > sizeof(WCHAR))
   {
      if (FileName != NULL)
      {
         lstrcpynW(FileName, (WCHAR*)Kvpi->Data,
                   min(Kvpi->DataLength / sizeof(WCHAR), FileNameSize));
      }
      return TRUE;
   }

   return FALSE;
}

/**
 * Detect if specified code page is valid and present in the system.
 *
 * @param CodePage
 *        Code page number to query.
 *
 * @return TRUE if code page is present.
 */
BOOL STDCALL
IsValidCodePage(UINT CodePage)
{
   if (CodePage == CP_UTF8 || CodePage == CP_UTF7)
      return TRUE;
   if (IntGetLoadedCodePageEntry(CodePage))
      return TRUE;
   return GetCPFileNameFromRegistry(CodePage, NULL, 0);
}

/**
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
INT STDCALL
MultiByteToWideChar(UINT CodePage, DWORD Flags,
                    LPCSTR MultiByteString, INT MultiByteCount,
                    LPWSTR WideCharString, INT WideCharCount)
{
   /* Check the parameters. */
   if (MultiByteString == NULL ||
       (WideCharString == NULL && WideCharCount > 0))
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
         return IntMultiByteToWideCharUTF8(
            Flags, MultiByteString, MultiByteCount,
            WideCharString, WideCharCount);

      case CP_UTF7:
         DPRINT1("MultiByteToWideChar for CP_UTF7 is not implemented!\n");
         return 0;

      case CP_SYMBOL:
         DPRINT1("MultiByteToWideChar for CP_SYMBOL is not implemented!\n");
         return 0;

      default:
         return IntMultiByteToWideCharCP(
            CodePage, Flags, MultiByteString, MultiByteCount,
            WideCharString, WideCharCount);
   }
}

/**
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
INT STDCALL
WideCharToMultiByte(UINT CodePage, DWORD Flags,
                    LPCWSTR WideCharString, INT WideCharCount,
                    LPSTR MultiByteString, INT MultiByteCount,
                    LPCSTR DefaultChar, LPBOOL UsedDefaultChar)
{
   /* Check the parameters. */
   if (WideCharString == NULL ||
       (MultiByteString == NULL && MultiByteCount > 0))
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
         return IntWideCharToMultiByteUTF8(
            CodePage, Flags, WideCharString, WideCharCount,
            MultiByteString, MultiByteCount, DefaultChar,
            UsedDefaultChar);

      case CP_UTF7:
         DPRINT1("WideCharToMultiByte for CP_UTF7 is not implemented!\n");
         return 0;

      case CP_SYMBOL:
         DPRINT1("WideCharToMultiByte for CP_SYMBOL is not implemented!\n");
         return 0;

      default:
         return IntWideCharToMultiByteCP(
            CodePage, Flags, WideCharString, WideCharCount,
            MultiByteString, MultiByteCount, DefaultChar,
            UsedDefaultChar);
   }
}

/*
 * @implemented
 */
UINT
STDCALL
GetACP (VOID)
{
    return AnsiCodePage.CodePageTable.CodePage;
}

/*
 * @implemented
 */
UINT
STDCALL
GetOEMCP (VOID)
{
    return OemCodePage.CodePageTable.CodePage;
}

static inline BOOL
IntIsLeadByte(PCPTABLEINFO TableInfo, UCHAR Ch)
{
  if(TableInfo->MaximumCharacterSize == 2)
  {
    UINT i;
    for(i = 0; i < MAXIMUM_LEADBYTES; i++)
    {
      if(TableInfo->LeadByte[i] == Ch)
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/*
 * @implemented
 */
BOOL
STDCALL
IsDBCSLeadByte (
    BYTE    TestChar
    )
{
   return IntIsLeadByte(&AnsiCodePage.CodePageTable, (UCHAR)TestChar);
}


/*
 * @implemented
 */
BOOL
STDCALL
IsDBCSLeadByteEx (
    UINT    CodePage,
    BYTE    TestChar
    )
{
    PCODEPAGE_ENTRY CodePageEntry;

    CodePageEntry = IntGetCodePageEntry(CodePage);
    if(CodePageEntry != NULL)
    {
      return IntIsLeadByte(&CodePageEntry->CodePageTable, (UCHAR)TestChar);
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/* EOF */
