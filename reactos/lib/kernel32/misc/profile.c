/* $Id: profile.c,v 1.12 2004/06/13 20:04:56 navaraf Exp $
 *
 * Imported from Wine
 * Copyright 1993 Miguel de Icaza
 * Copyright 1996 Alexandre Julliard
 *
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/profile.c
 * PURPOSE:         Profiles functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

static CRITICAL_SECTION ProfileLock;

typedef struct tagPROFILEKEY
{
  WCHAR                 *Value;
  struct tagPROFILEKEY  *Next;
  WCHAR                  Name[1];
} PROFILEKEY;

typedef struct tagPROFILESECTION
{
  struct tagPROFILEKEY       *Key;
  struct tagPROFILESECTION   *Next;
  WCHAR                       Name[1];
} PROFILESECTION;


typedef struct
{
  BOOL             Changed;
  PROFILESECTION  *Section;
  WCHAR           *FullName;
  FILETIME         LastWriteTime;
} PROFILE;


#define N_CACHED_PROFILES 10

/* Cached profile files */
static PROFILE *MRUProfile[N_CACHED_PROFILES]={NULL};

#define CurProfile (MRUProfile[0])

#define PROFILE_MAX_LINE_LEN   1024

/* Check for comments in profile */
#define IS_ENTRY_COMMENT(str)  ((str)[0] == ';')

/* FUNCTIONS *****************************************************************/

/***********************************************************************
 *           PROFILE_CopyEntry
 *
 * Copy the content of an entry into a buffer, removing quotes, and possibly
 * translating environment variables.
 */
STATIC void FASTCALL
PROFILE_CopyEntry(LPWSTR Buffer, LPCWSTR Value, int Len, BOOL StripQuote)
{
  WCHAR Quote = L'\0';

  if (NULL == Buffer)
    {
      return;
    }

  if (StripQuote && (L'\'' == *Value || L'\"' == *Value))
    {
      if (L'\0' != Value[1] && (Value[wcslen(Value) - 1] == *Value))
        {
          Quote = *Value++;
        }
    }

  lstrcpynW(Buffer, Value, Len);
  if (L'\0' != Quote && wcslen(Value) <= Len)
    {
      Buffer[wcslen(Buffer) - 1] = L'\0';
    }
}


/***********************************************************************
 *           PROFILE_Save
 *
 * Save a profile tree to a file.
 */
STATIC void FASTCALL
PROFILE_Save(HANDLE File, PROFILESECTION *Section)
{
  PROFILEKEY *Key;
  char Buffer[PROFILE_MAX_LINE_LEN];
  DWORD BytesWritten;

  for ( ; NULL != Section; Section = Section->Next)
    {
      if (L'\0' != Section->Name[0])
        {
          strcpy(Buffer, "\r\n[");
          WideCharToMultiByte(CP_ACP, 0, Section->Name, -1, Buffer + 3, sizeof(Buffer) - 6, NULL, NULL);
          strcat(Buffer, "]\r\n");
          WriteFile(File, Buffer, strlen(Buffer), &BytesWritten, NULL);
        }
      for (Key = Section->Key; NULL != Key; Key = Key->Next)
        {
          WideCharToMultiByte(CP_ACP, 0, Key->Name, -1, Buffer, sizeof(Buffer) - 3, NULL, NULL);
          if (Key->Value)
            {
              strcat(Buffer, "=");
              WideCharToMultiByte(CP_ACP, 0, Key->Value, -1, Buffer + strlen(Buffer),
                                  sizeof(Buffer) - strlen(Buffer) - 2, NULL, NULL);
            }
          strcat(Buffer, "\r\n");
          WriteFile(File, Buffer, strlen(Buffer), &BytesWritten, NULL);
        }
    }
}


/***********************************************************************
 *           PROFILE_Free
 *
 * Free a profile tree.
 */
STATIC void FASTCALL
PROFILE_Free(PROFILESECTION *Section)
{
  PROFILESECTION *NextSection;
  PROFILEKEY *Key, *NextKey;

  for ( ; NULL != Section; Section = NextSection)
    {
      for (Key = Section->Key; NULL != Key; Key = NextKey)
        {
          NextKey = Key->Next;
          if (NULL != Key->Value)
            {
              HeapFree(GetProcessHeap(), 0, Key->Value);
            }
          HeapFree(GetProcessHeap(), 0, Key);
        }
      NextSection = Section->Next;
      HeapFree(GetProcessHeap(), 0, Section);
    }
}

STATIC inline int
PROFILE_isspace(char c)
{
  if (isspace(c))
    {
      return 1;
    }
  if ('\r' == c || 0x1a == c)
    {
      /* CR and ^Z (DOS EOF) are spaces too  (found on CD-ROMs) */
      return 1;
    }
  return 0;
}


/***********************************************************************
 *           PROFILE_Load
 *
 * Load a profile tree from a file.
 */
static PROFILESECTION * FASTCALL
PROFILE_Load(HANDLE File)
{
  char *p, *p2;
  int Len;
  PROFILESECTION *Section, *FirstSection;
  PROFILESECTION **NextSection;
  PROFILEKEY *Key, *PrevKey, **NextKey;
  LARGE_INTEGER FileSize;
  HANDLE Mapping;
  PVOID BaseAddress;
  char *Input, *End;

  FileSize.u.LowPart = GetFileSize(File, &FileSize.u.HighPart);
  if (INVALID_FILE_SIZE == FileSize.u.LowPart && 0 != GetLastError())
    {
      return NULL;
    }

  FirstSection = HeapAlloc(GetProcessHeap(), 0, sizeof(*Section));
  if (NULL == FirstSection)
    {
      return NULL;
    }
  FirstSection->Name[0] = L'\0';
  FirstSection->Key  = NULL;
  FirstSection->Next = NULL;
  NextSection = &FirstSection->Next;
  NextKey     = &FirstSection->Key;
  PrevKey     = NULL;

  if (0 == FileSize.QuadPart)
    {
      return FirstSection;
    }
  Mapping = CreateFileMappingW(File, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
  if (NULL == Mapping)
    {
      HeapFree(GetProcessHeap(), 0, FirstSection);
      return NULL;
    }
  BaseAddress = MapViewOfFile(Mapping, FILE_MAP_READ, 0, 0, 0);
  if (NULL == BaseAddress)
    {
      CloseHandle(Mapping);
      HeapFree(GetProcessHeap(), 0, FirstSection);
      return NULL;
    }

  Input = (char *) BaseAddress;
  End = Input + FileSize.QuadPart;
  while (Input < End)
    {
      while (Input < End && PROFILE_isspace(*Input))
        {
          Input++;
        }
      if (End <= Input)
        {
          break;
        }
      if ('[' == *Input)  /* section start */
        {
          p = ++Input;
          while (p < End && ']' != *p && '\n' != *p)
            {
              p++;
            }
          if (p < End && ']' == *p)
            {
              Len = p - Input;
              if (NULL == (Section = HeapAlloc(GetProcessHeap(), 0,
                                               sizeof(*Section) + Len * sizeof(WCHAR))))
                {
                  break;
                }
              MultiByteToWideChar(CP_ACP, 0, Input, Len, Section->Name, Len);
              Section->Name[Len] = L'\0';
              Section->Key  = NULL;
              Section->Next = NULL;
              *NextSection = Section;
              NextSection  = &Section->Next;
              NextKey      = &Section->Key;
              PrevKey      = NULL;

              DPRINT("New section: %S\n", Section->Name);

            }
          Input = p;
          while (Input < End && '\n' != *Input)
            {
              Input++;
            }
          Input++; /* Skip past the \n */
          continue;
        }

      p = Input;
      p2 = p;
      while (p < End && '=' != *p && '\n' != *p)
        {
          if (! PROFILE_isspace(*p))
            {
              p2 = p;
            }
          p++;
        }

      if (p < End && '=' == *p)
        {
          Len = p2 - Input + 1;
          if (NULL == (Key = HeapAlloc(GetProcessHeap(), 0,
                                       sizeof(*Key) + Len * sizeof(WCHAR))))
            {
              break;
            }
          MultiByteToWideChar(CP_ACP, 0, Input, Len, Key->Name, Len);
          Key->Name[Len] = L'\0';
          Input = p + 1;
          while (Input < End && '\n' != *Input && PROFILE_isspace(*Input))
            {
              Input++;
            }
          if (End <= Input || '\n' == *Input)
            {
              Key->Value = NULL;
            }
          else
            {
              p2 = Input;
              p = Input + 1;
              while (p < End && '\n' != *p)
                {
                  if (! PROFILE_isspace(*p))
                    {
                      p2 = p;
                    }
                  p++;
                }
              Len = p2 - Input + 1;
              if (NULL == (Key->Value = HeapAlloc(GetProcessHeap(), 0,
                                                  (Len + 1) * sizeof(WCHAR))))
                {
                  break;
                }
              MultiByteToWideChar(CP_ACP, 0, Input, Len, Key->Value, Len);
              Key->Value[Len] = L'\0';

              Key->Next  = NULL;
              *NextKey  = Key;
              NextKey   = &Key->Next;
              PrevKey   = Key;

              DPRINT("New key: name=%S, value=%S\n",
                     Key->Name, NULL != Key->Value ? Key->Value : L"(none)");

              Input = p;
            }
        }
      while (Input < End && '\n' != *Input)
        {
          Input++;
        }
      Input++; /* Skip past the \n */
    }

  UnmapViewOfFile(BaseAddress);
  CloseHandle(Mapping);

  return FirstSection;
}


/***********************************************************************
 *           PROFILE_Find
 *
 * Find a key in a profile tree, optionally creating it.
 */
static PROFILEKEY * FASTCALL
PROFILE_Find(PROFILESECTION **Section, LPCWSTR SectionName,
             LPCWSTR KeyName, BOOL Create, BOOL CreateAlways)
{
  LPCWSTR p;
  int SecLen, KeyLen;

  while (PROFILE_isspace(*SectionName))
    {
      SectionName++;
    }
  p = SectionName + wcslen(SectionName) - 1;
  while (SectionName < p && PROFILE_isspace(*p))
    {
      p--;
    }
  SecLen = p - SectionName + 1;

  while (PROFILE_isspace(*KeyName))
    {
      KeyName++;
    }
  p = KeyName + wcslen(KeyName) - 1;
  while (KeyName < p && PROFILE_isspace(*p))
    {
      p--;
    }
  KeyLen = p - KeyName + 1;

  while (NULL != *Section)
    {
      if (L'\0' != ((*Section)->Name[0])
          && 0 == _wcsnicmp((*Section)->Name, SectionName, SecLen)
          && L'\0' == ((*Section)->Name)[SecLen])
        {
          PROFILEKEY **Key = &(*Section)->Key;

          while (NULL != *Key)
            {
              /* If create_always is FALSE then we check if the keyname
               * already exists. Otherwise we add it regardless of its
               * existence, to allow keys to be added more than once in
               * some cases.
               */
              if (! CreateAlways)
                {
                  if (0 == _wcsnicmp((*Key)->Name, KeyName, KeyLen)
                      && L'\0' == ((*Key)->Name)[KeyLen])
                    {
                      return *Key;
                    }
                }
              Key = &(*Key)->Next;
            }
          if (! Create)
            {
              return NULL;
            }
          if (NULL == (*Key = HeapAlloc(GetProcessHeap(), 0,
                                        sizeof(PROFILEKEY) + wcslen(KeyName) * sizeof(WCHAR))))
            {
              return NULL;
            }
          wcscpy((*Key)->Name, KeyName);
          (*Key)->Value = NULL;
          (*Key)->Next  = NULL;
          return *Key;
        }
      Section = &(*Section)->Next;
    }

  if (! Create)
    {
      return NULL;
    }

  *Section = HeapAlloc(GetProcessHeap(), 0,
                       sizeof(PROFILESECTION) + wcslen(SectionName) * sizeof(WCHAR));
  if (NULL == *Section)
    {
      return NULL;
    }
  wcscpy((*Section)->Name, SectionName);
  (*Section)->Next = NULL;
  if (NULL == ((*Section)->Key = HeapAlloc(GetProcessHeap(), 0,
                                           sizeof(PROFILEKEY) + wcslen(KeyName) * sizeof(WCHAR))))
    {
      HeapFree(GetProcessHeap(), 0, *Section);
      return NULL;
    }
  wcscpy((*Section)->Key->Name, KeyName );
  (*Section)->Key->Value = NULL;
  (*Section)->Key->Next  = NULL;

  return (*Section)->Key;
}


/***********************************************************************
 *           PROFILE_FlushFile
 *
 * Flush the current profile to disk if changed.
 */
STATIC BOOL FASTCALL
PROFILE_FlushFile(void)
{
  HANDLE File = INVALID_HANDLE_VALUE;
  FILETIME LastWriteTime;

  if (NULL == CurProfile)
    {
      DPRINT("No current profile!\n");
      return FALSE;
    }

  if (! CurProfile->Changed || NULL == CurProfile->FullName)
    {
      return TRUE;
    }

  File = CreateFileW(CurProfile->FullName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
  if (INVALID_HANDLE_VALUE == File)
    {
      DPRINT1("could not save profile file %s\n", CurProfile->FullName);
      return FALSE;
    }

  DPRINT("Saving %S\n", CurProfile->FullName);
  PROFILE_Save(File, CurProfile->Section);
  if (GetFileTime(File, NULL, NULL, &LastWriteTime))
    {
      CurProfile->LastWriteTime.dwLowDateTime = LastWriteTime.dwLowDateTime;
      CurProfile->LastWriteTime.dwHighDateTime = LastWriteTime.dwHighDateTime;
    }
  CurProfile->Changed = FALSE;
  CloseHandle(File);

  return TRUE;
}


/***********************************************************************
 *           PROFILE_ReleaseFile
 *
 * Flush the current profile to disk and remove it from the cache.
 */
STATIC void FASTCALL
PROFILE_ReleaseFile(void)
{
  PROFILE_FlushFile();
  PROFILE_Free(CurProfile->Section);
  if (NULL != CurProfile->FullName)
    {
      HeapFree(GetProcessHeap(), 0, CurProfile->FullName);
    }
  CurProfile->Changed = FALSE;
  CurProfile->Section = NULL;
  CurProfile->FullName = NULL;
  CurProfile->LastWriteTime.dwLowDateTime = 0;
  CurProfile->LastWriteTime.dwHighDateTime = 0;
}


/***********************************************************************
 *           PROFILE_Open
 *
 * Open a profile file, checking the cached file first.
 */
STATIC BOOL FASTCALL
PROFILE_Open(LPCWSTR FileName)
{
  WCHAR FullName[MAX_PATH];
  HANDLE File = INVALID_HANDLE_VALUE;
  int i, j;
  PROFILE *TempProfile;
  DWORD FullLen;
  FILETIME LastWriteTime;

  /* Build full path name */

  if (wcschr(FileName, L'/') || wcschr(FileName, L'\\') ||
      wcschr(FileName, L':'))
    {
      FullLen = GetFullPathNameW(FileName, MAX_PATH, FullName, NULL);
      if (0 == FullLen || MAX_PATH < FullLen)
        {
          return FALSE;
        }
    }
  else
    {
      FullLen = GetWindowsDirectoryW(FullName, MAX_PATH);
      if (0 == FullLen || MAX_PATH < FullLen + 1 + wcslen(FileName))
        {
          return FALSE;
        }
      wcscat(FullName, L"\\");
      wcscat(FullName, FileName);
    }
#if 0 /* FIXME: Not yet implemented */
  FullLen = GetLongPathNameW(FullName, FullName, MAX_PATH);
  if (0 == FullLen || MAX_PATH < FullLen)
    {
      return FALSE;
    }
#endif

  /* Check for a match */

  for (i = 0; i < N_CACHED_PROFILES; i++)
    {
      if (NULL != MRUProfile[i]->FullName && 0 == wcscmp(FullName, MRUProfile[i]->FullName))
        {
          if (0 != i)
            {
              PROFILE_FlushFile();
              TempProfile = MRUProfile[i];
              for(j = i; j > 0; j--)
                {
                  MRUProfile[j] = MRUProfile[j - 1];
                }
              CurProfile = TempProfile;
            }
          File = CreateFileW(FullName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, 0, NULL);
          if (INVALID_HANDLE_VALUE == File)
            {
              return FALSE;
            }
          if (GetFileTime(File, NULL, NULL, &LastWriteTime) &&
              LastWriteTime.dwLowDateTime == CurProfile->LastWriteTime.dwLowDateTime &&
              LastWriteTime.dwHighDateTime == CurProfile->LastWriteTime.dwHighDateTime)
            {
              DPRINT("(%S): already opened (mru = %d)\n", FileName, i);
            }
          else
            {
              DPRINT("(%S): already opened, needs refreshing (mru = %d)\n", FileName, i);
            }
          CloseHandle(File);

	  return TRUE;
        }
    }

  /* Flush the old current profile */
  PROFILE_FlushFile();

  /* Make the oldest profile the current one only in order to get rid of it */
  if (N_CACHED_PROFILES == i)
    {
      TempProfile = MRUProfile[N_CACHED_PROFILES - 1];
      for(i = N_CACHED_PROFILES - 1; i > 0; i--)
        {
          MRUProfile[i] = MRUProfile[i - 1];
        }
      CurProfile = TempProfile;
    }
  if (NULL != CurProfile->FullName)
    {
      PROFILE_ReleaseFile();
    }

  /* OK, now that CurProfile is definitely free we assign it our new file */
  CurProfile->FullName  = HeapAlloc(GetProcessHeap(), 0, (wcslen(FullName) + 1) * sizeof(WCHAR));
  if (NULL != CurProfile->FullName)
    {
      wcscpy(CurProfile->FullName, FullName);
    }

  /* Try to open the profile file */
  File = CreateFileW(FullName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
  if (INVALID_HANDLE_VALUE != File)
    {
      CurProfile->Section = PROFILE_Load(File);
      if (GetFileTime(File, NULL, NULL, &LastWriteTime))
        {
          CurProfile->LastWriteTime.dwLowDateTime = LastWriteTime.dwLowDateTime;
          CurProfile->LastWriteTime.dwHighDateTime = LastWriteTime.dwHighDateTime;
        }
      CloseHandle(File);
    }
  else
    {
      /* Does not exist yet, we will create it in PROFILE_FlushFile */
      DPRINT("profile file %S not found\n", FileName);
    }

  return TRUE;
}


/***********************************************************************
 *           PROFILE_GetSection
 *
 * Returns all keys of a section.
 * If ReturnValues is TRUE, also include the corresponding values.
 */
static INT FASTCALL
PROFILE_GetSection(PROFILESECTION *Section, LPCWSTR SectionName,
                   LPWSTR Buffer, UINT Len, BOOL ReturnValues )
{
  PROFILEKEY *Key;

  if (NULL == Buffer)
    {
      return 0;
    }

  DPRINT("%S,%p,%u\n", SectionName, Buffer, Len);

  while (NULL != Section)
    {
      if (L'\0' != Section->Name[0] && 0 == _wcsicmp(Section->Name, SectionName))
        {
          UINT OldLen = Len;
          for (Key = Section->Key; NULL != Key; Key = Key->Next)
            {
              if (Len <= 2)
                {
                  break;
                }
              if (L'\0' == *Key->Name)
                {
                  continue;  /* Skip empty lines */
                }
              if (IS_ENTRY_COMMENT(Key->Name))
                {
                  continue;  /* Skip comments */
                }
              PROFILE_CopyEntry(Buffer, Key->Name, Len - 1, FALSE);
              Len -= wcslen(Buffer) + 1;
              Buffer += wcslen(Buffer) + 1;
              if (Len < 2)
                {
                  break;
                }
              if (ReturnValues && NULL != Key->Value)
                {
                  Buffer[-1] = L'=';
                  PROFILE_CopyEntry(Buffer, Key->Value, Len - 1, 0);
                  Len -= wcslen(Buffer) + 1;
                  Buffer += wcslen(Buffer) + 1;
                }
            }
          *Buffer = L'\0';
          if (Len <= 1)
            /*If either lpszSection or lpszKey is NULL and the supplied
              destination buffer is too small to hold all the strings,
              the last string is truncated and followed by two null characters.
              In this case, the return value is equal to cchReturnBuffer
              minus two. */
            {
              Buffer[-1] = L'\0';
              return OldLen - 2;
            }
          return OldLen - Len;
        }
      Section = Section->Next;
    }

  Buffer[0] = Buffer[1] = L'\0';

  return 0;
}

/* See GetPrivateProfileSectionNamesW for documentation */
STATIC INT FASTCALL
PROFILE_GetSectionNames(LPWSTR Buffer, UINT Len)
{
  LPWSTR Buf;
  UINT f,l;
  PROFILESECTION *Section;

  if (NULL == Buffer || 0 == Len)
    {
      return 0;
    }

  if (1 == Len)
    {
      *Buffer = L'\0';
      return 0;
    }

  f = Len - 1;
  Buf = Buffer;
  Section = CurProfile->Section;
  while (NULL != Section)
    {
      if (L'\0' != Section->Name[0])
        {
          l = wcslen(Section->Name) + 1;
          if (f < l)
            {
              if (0 < f)
                {
                  wcsncpy(Buf, Section->Name, f - 1);
                  Buf += f - 1;
                  *Buf++ = L'\0';
                }
              *Buf = L'\0';
              return Len-2;
            }
          wcscpy(Buf, Section->Name);
          Buf += l;
          f -= l;
        }
      Section = Section->Next;
    }

  *Buf='\0';
  return Buf - Buffer;
}


/***********************************************************************
 *           PROFILE_GetString
 *
 * Get a profile string.
 */
static INT FASTCALL
PROFILE_GetString(LPCWSTR Section, LPCWSTR KeyName,
                  LPCWSTR DefVal, LPWSTR Buffer, UINT Len)
{
  PROFILEKEY *Key = NULL;

  if (NULL == Buffer)
    {
      return 0;
    }

  if (NULL == DefVal)
    {
      DefVal = L"";
    }

  if (NULL != KeyName)
    {
      if (L'\0' == KeyName[0])
        {
          return 0;
        }
      Key = PROFILE_Find(&CurProfile->Section, Section, KeyName, FALSE, FALSE);
      PROFILE_CopyEntry(Buffer, (NULL != Key && NULL != Key->Value) ? Key->Value : DefVal,
                        Len, TRUE );
      DPRINT("(%S,%S,%S): returning %S\n", Section, KeyName, DefVal, Buffer);
      return wcslen(Buffer);
    }

  if (NULL != Section && L'\0' != Section[0])
    {
      INT Ret = PROFILE_GetSection(CurProfile->Section, Section, Buffer, Len, FALSE);
      if (L'\0' == Buffer[0]) /* no luck -> def_val */
        {
          PROFILE_CopyEntry(Buffer, DefVal, Len, TRUE);
          Ret = wcslen(Buffer);
        }
      return Ret;
    }

  Buffer[0] = '\0';

  return 0;
}


/***********************************************************************
 *           PROFILE_DeleteSection
 *
 * Delete a section from a profile tree.
 */
STATIC BOOL FASTCALL
PROFILE_DeleteSection(PROFILESECTION **Section, LPCWSTR Name)
{
  while (NULL != *Section)
    {
      if (L'\0' != (*Section)->Name[0] && 0 == _wcsicmp((*Section)->Name, Name))
        {
          PROFILESECTION *ToDel = *Section;
          *Section = ToDel->Next;
          ToDel->Next = NULL;
          PROFILE_Free(ToDel);
          return TRUE;
        }
      Section = &(*Section)->Next;
    }

  return FALSE;
}


/***********************************************************************
 *           PROFILE_DeleteKey
 *
 * Delete a key from a profile tree.
 */
STATIC BOOL FASTCALL
PROFILE_DeleteKey(PROFILESECTION **Section,
                  LPCWSTR SectionName, LPCWSTR KeyName)
{
  while (*Section)
    {
      if (L'\0' != (*Section)->Name[0] && 0 == _wcsicmp((*Section)->Name, SectionName))
        {
          PROFILEKEY **Key = &(*Section)->Key;
          while (NULL != *Key)
            {
              if (0 == _wcsicmp((*Key)->Name, KeyName))
                {
                  PROFILEKEY *ToDel = *Key;
                  *Key = ToDel->Next;
                  if (NULL != ToDel->Value)
                    {
                      HeapFree(GetProcessHeap(), 0, ToDel->Value);
                    }
                  HeapFree(GetProcessHeap(), 0, ToDel);
                  return TRUE;
                }
              Key = &(*Key)->Next;
            }
        }
      Section = &(*Section)->Next;
    }

  return FALSE;
}


/***********************************************************************
 *           PROFILE_SetString
 *
 * Set a profile string.
 */
STATIC BOOL FASTCALL
PROFILE_SetString(LPCWSTR SectionName, LPCWSTR KeyName,
                  LPCWSTR Value, BOOL CreateAlways )
{
  PROFILEKEY *Key;

  if (NULL == KeyName)  /* Delete a whole section */
    {
      DPRINT("(%S)\n", SectionName);
      CurProfile->Changed |= PROFILE_DeleteSection(&CurProfile->Section,
                                                   SectionName);
      return TRUE;         /* Even if PROFILE_DeleteSection() has failed,
                              this is not an error on application's level.*/
    }

  if (NULL == Value)  /* Delete a key */
    {
      DPRINT("(%S,%S)\n", SectionName, KeyName);
      CurProfile->Changed |= PROFILE_DeleteKey(&CurProfile->Section,
                                               SectionName, KeyName);
      return TRUE;          /* same error handling as above */
    }

  /* Set the key value */
  Key = PROFILE_Find(&CurProfile->Section, SectionName,
                     KeyName, TRUE, CreateAlways);
  DPRINT("(%S,%S,%S):\n", SectionName, KeyName, Value);
  if (NULL == Key)
    {
      return FALSE;
    }
  if (NULL != Key->Value)
    {
      /* strip the leading spaces. We can safely strip \n\r and
       * friends too, they should not happen here anyway. */
      while (PROFILE_isspace(*Value))
        {
          Value++;
        }

      if (0 == wcscmp(Key->Value, Value))
        {
          DPRINT("  no change needed\n");
          return TRUE;  /* No change needed */
        }
      DPRINT("  replacing %S\n", Key->Value);
      HeapFree(GetProcessHeap(), 0, Key->Value);
    }
  else
    {
      DPRINT("  creating key\n" );
    }
  Key->Value = HeapAlloc(GetProcessHeap(), 0, (wcslen(Value) + 1) * sizeof(WCHAR));
  wcscpy(Key->Value, Value);
  CurProfile->Changed = TRUE;

  return TRUE;
}

/*
 * if AllowSectionNameCopy is TRUE, allow the copying :
 *   - of Section names if 'section' is NULL
 *   - of Keys in a Section if 'entry' is NULL
 * (see MSDN doc for GetPrivateProfileString)
 */
STATIC int FASTCALL
PROFILE_GetPrivateProfileString(LPCWSTR Section, LPCWSTR Entry,
                                LPCWSTR DefVal, LPWSTR Buffer,
                                UINT Len, LPCWSTR Filename,
                                BOOL AllowSectionNameCopy)
{
  int Ret;
  LPWSTR pDefVal = NULL;

  if (NULL == Filename)
    {
      Filename = L"win.ini";
    }

  DPRINT("%S,%S,%S,%p,%u,%S\n", Section, Entry, DefVal, Buffer, Len, Filename);

  /* strip any trailing ' ' of DefVal. */
  if (NULL != DefVal)
    {
      LPCWSTR p = &DefVal[wcslen(DefVal)]; /* even "" works ! */

      while (DefVal < p)
        {
          p--;
          if (' ' != (*p))
            {
              break;
            }
        }
      if (' ' == *p) /* ouch, contained trailing ' ' */
        {
          int NewLen = (int)(p - DefVal);
          pDefVal = HeapAlloc(GetProcessHeap(), 0, (NewLen + 1) * sizeof(WCHAR));
          wcsncpy(pDefVal, DefVal, NewLen);
          pDefVal[Len] = '\0';
        }
    }

  if (NULL == pDefVal)
    {
      pDefVal = (LPWSTR) DefVal;
    }

  RtlEnterCriticalSection(&ProfileLock);

  if (PROFILE_Open(Filename))
    {
      if ((AllowSectionNameCopy) && (NULL == Section))
        {
          Ret = PROFILE_GetSectionNames(Buffer, Len);
        }
      else
        {
          /* PROFILE_GetString already handles the 'entry == NULL' case */
          Ret = PROFILE_GetString(Section, Entry, pDefVal, Buffer, Len);
        }
    }
  else
    {
      lstrcpynW(Buffer, pDefVal, Len);
      Ret = wcslen(Buffer);
    }

  RtlLeaveCriticalSection(&ProfileLock);

  if (pDefVal != DefVal) /* allocated */
    {
      HeapFree(GetProcessHeap(), 0, pDefVal);
    }

  DPRINT("returning %S, %d\n", Buffer, Ret);

  return Ret;
}


BOOL FASTCALL
PROFILE_Init()
{
  unsigned i;

  RtlInitializeCriticalSection(&ProfileLock);

  for (i = 0; i < N_CACHED_PROFILES; i++)
    {
      MRUProfile[i] = HeapAlloc(GetProcessHeap(), 0, sizeof(PROFILE));
      if (NULL == MRUProfile[i])
        {
          return FALSE;
        }
      MRUProfile[i]->Changed = FALSE;
      MRUProfile[i]->Section = NULL;
      MRUProfile[i]->FullName = NULL;
      MRUProfile[i]->LastWriteTime.dwLowDateTime = 0;
      MRUProfile[i]->LastWriteTime.dwHighDateTime = 0;
    }

  return TRUE;
}

/*
 * @unimplemented
 */
BOOL STDCALL
CloseProfileUserMapping(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @implemented
 */
UINT STDCALL
GetPrivateProfileIntW(
	LPCWSTR	AppName,
	LPCWSTR	KeyName,
	INT	Default,
	LPCWSTR	FileName
	)
{
  WCHAR Buffer[30];
  UNICODE_STRING BufferW;
  INT Len;
  ULONG Result;

  if (0 == (Len = GetPrivateProfileStringW(AppName, KeyName, L"",
                                           Buffer, sizeof(Buffer)/sizeof(WCHAR),
                                           FileName)))
    {
      return (UINT) Default;
    }

  if (Len + 1 == sizeof(Buffer) / sizeof(WCHAR))
    {
      DPRINT1("Result may be wrong!\n");
    }

  /* FIXME: if entry can be found but it's empty, then Win16 is
   * supposed to return 0 instead of def_val ! Difficult/problematic
   * to implement (every other failure also returns zero buffer),
   * thus wait until testing framework avail for making sure nothing
   * else gets broken that way. */
  if (L'\0' == Buffer[0])
    {
      return (UINT) Default;
    }

  RtlInitUnicodeString(&BufferW, Buffer);
  RtlUnicodeStringToInteger(&BufferW, 10, &Result);

  return Result;
}


/*
 * @implemented
 */
UINT STDCALL
GetPrivateProfileIntA(
	LPCSTR	AppName,
	LPCSTR	KeyName,
	INT	Default,
	LPCSTR	FileName
	)
{
  UNICODE_STRING KeyNameW, FileNameW, AppNameW;
  UINT Res;

  if (NULL != KeyName)
    {
      RtlCreateUnicodeStringFromAsciiz(&KeyNameW, (PCSZ) KeyName);
    }
  else
    {
      KeyNameW.Buffer = NULL;
    }
  if (NULL != FileName)
    {
      RtlCreateUnicodeStringFromAsciiz(&FileNameW, (PCSZ) FileName);
    }
  else
    {
      FileNameW.Buffer = NULL;
    }
  if (NULL != AppName)
    {
      RtlCreateUnicodeStringFromAsciiz(&AppNameW, (PCSZ) AppName);
    }
  else
    {
      AppNameW.Buffer = NULL;
    }

  Res = GetPrivateProfileIntW(AppNameW.Buffer, KeyNameW.Buffer, Default,
                              FileNameW.Buffer);

  RtlFreeUnicodeString(&AppNameW);
  RtlFreeUnicodeString(&FileNameW);
  RtlFreeUnicodeString(&KeyNameW);

  return Res;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetPrivateProfileSectionW (
	LPCWSTR	lpAppName,
	LPWSTR	lpReturnedString,
	DWORD	nSize,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetPrivateProfileSectionA (
	LPCSTR	lpAppName,
	LPSTR	lpReturnedString,
	DWORD	nSize,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/***********************************************************************
 *           GetPrivateProfileSectionNamesW  (KERNEL32.@)
 *
 * Returns the section names contained in the specified file.
 * The section names are returned as a list of strings with an extra
 * '\0' to mark the end of the list.
 *
 * - if the buffer is 0, 1 or 2 characters long then it is filled with
 *   '\0' and the return value is 0
 * - otherwise if the buffer is too small then the first section name that
 *   does not fit is truncated so that the string list can be terminated
 *   correctly (double '\0')
 * - the return value is the number of characters written in the buffer
 *   except for the trailing '\0'. If the buffer is too small, then the
 *   return value is len-2
 * - Win2000 has a bug that triggers when the section names and the
 *   trailing '\0' fit exactly in the buffer. In that case the trailing
 *   '\0' is missing.
 *
 * Note that when the buffer is big enough then the return value may be any
 * value between 1 and len-1 , including len-2.
 *
 * @implemented
 */
DWORD STDCALL
GetPrivateProfileSectionNamesW(
	LPWSTR  Buffer,
	DWORD   Size,
	LPCWSTR FileName
	)
{
  DWORD Ret = 0;

  RtlEnterCriticalSection(&ProfileLock);

  if (PROFILE_Open(FileName))
    {
      Ret = PROFILE_GetSectionNames(Buffer, Size);
    }

  RtlLeaveCriticalSection(&ProfileLock);

  return Ret;
}


/*
 * @implemented
 */
DWORD STDCALL
GetPrivateProfileSectionNamesA(
	LPSTR  Buffer,
	DWORD  Size,
	LPCSTR FileName
	)
{
  UNICODE_STRING FileNameW;
  LPWSTR BufferW;
  INT RetW, Ret = 0;

  BufferW = Buffer ? HeapAlloc(GetProcessHeap(), 0, Size * sizeof(WCHAR)) : NULL;
  if (NULL != FileName)
    {
      RtlCreateUnicodeStringFromAsciiz(&FileNameW, (PCSZ) FileName);
    }
  else
    {
      FileNameW.Buffer = NULL;
    }

  RetW = GetPrivateProfileSectionNamesW(BufferW, Size, FileNameW.Buffer);
  if (0 != RetW && 0 != Size)
    {
      Ret = WideCharToMultiByte(CP_ACP, 0, BufferW, RetW, Buffer, Size, NULL, NULL);
      if (0 == Ret)
        {
          Ret = Size;
          Buffer[Size - 1] = '\0';
        }
    }

  RtlFreeUnicodeString(&FileNameW);
  if (NULL != BufferW)
    {
      HeapFree(GetProcessHeap(), 0, BufferW);
    }

  return Ret;
}


/*
 * @implemented
 */
DWORD STDCALL
GetPrivateProfileStringW(
	LPCWSTR AppName,
	LPCWSTR KeyName,
	LPCWSTR Default,
	LPWSTR	ReturnedString,
	DWORD	Size,
	LPCWSTR	FileName
	)
{
  return PROFILE_GetPrivateProfileString(AppName, KeyName, Default,
                                         ReturnedString, Size, FileName, TRUE);
}


/*
 * @implemented
 */
DWORD STDCALL
GetPrivateProfileStringA(
	LPCSTR	AppName,
	LPCSTR	KeyName,
	LPCSTR	Default,
	LPSTR	ReturnedString,
	DWORD	Size,
	LPCSTR	FileName
	)
{
  UNICODE_STRING AppNameW, KeyNameW, DefaultW, FileNameW;
  LPWSTR ReturnedStringW;
  INT RetW, Ret = 0;

  ReturnedStringW = (NULL != ReturnedString
                     ? HeapAlloc(GetProcessHeap(), 0, Size * sizeof(WCHAR)) : NULL);
  if (NULL != AppName)
    {
      RtlCreateUnicodeStringFromAsciiz(&AppNameW, (PCSZ) AppName);
    }
  else
    {
      AppNameW.Buffer = NULL;
    }
  if (NULL != KeyName)
    {
      RtlCreateUnicodeStringFromAsciiz(&KeyNameW, (PCSZ) KeyName);
    }
  else
    {
      KeyNameW.Buffer = NULL;
    }
  if (NULL != Default)
    {
      RtlCreateUnicodeStringFromAsciiz(&DefaultW, (PCSZ) Default);
    }
  else
    {
      DefaultW.Buffer = NULL;
    }
  if (NULL != FileName)
    {
      RtlCreateUnicodeStringFromAsciiz(&FileNameW, (PCSZ) FileName);
    }
  else
    {
      FileNameW.Buffer = NULL;
    }

  RetW = GetPrivateProfileStringW(AppNameW.Buffer, KeyNameW.Buffer,
                                  DefaultW.Buffer, ReturnedStringW, Size,
                                  FileNameW.Buffer);
  if (0 != Size)
    {
      Ret = WideCharToMultiByte(CP_ACP, 0, ReturnedStringW, RetW + 1, ReturnedString, Size, NULL, NULL);
      if (0 == Ret)
        {
          Ret = Size - 1;
          ReturnedString[Ret] = 0;
        }
      else
        {
          Ret--; /* strip terminating 0 */
        }
    }

  RtlFreeUnicodeString(&AppNameW);
  RtlFreeUnicodeString(&KeyNameW);
  RtlFreeUnicodeString(&DefaultW);
  RtlFreeUnicodeString(&FileNameW);
  if (NULL != ReturnedStringW)
    {
      HeapFree(GetProcessHeap(), 0, ReturnedStringW);
    }

  return Ret;
}


/*
 * @unimplemented
 */
BOOL STDCALL
GetPrivateProfileStructW (
	IN LPCWSTR Section,
	IN LPCWSTR Key,
	OUT LPVOID Struct,
	IN UINT StructSize,
	IN LPCWSTR File
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL STDCALL
GetPrivateProfileStructA (
	IN LPCSTR Section,
	IN LPCSTR Key,
	OUT LPVOID Struct,
	IN UINT StructSize,
	IN LPCSTR File
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @implemented
 */
UINT STDCALL
GetProfileIntW(LPCWSTR lpAppName,
	       LPCWSTR lpKeyName,
	       INT nDefault)
{
  return GetPrivateProfileIntW(lpAppName,
                               lpKeyName,
                               nDefault,
                               L"win.ini");
}


/*
 * @implemented
 */
UINT STDCALL
GetProfileIntA(LPCSTR lpAppName,
	       LPCSTR lpKeyName,
	       INT nDefault)
{
  return GetPrivateProfileIntA(lpAppName,
                               lpKeyName,
                               nDefault,
                               "win.ini");
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetProfileSectionW(LPCWSTR lpAppName,
		   LPWSTR lpReturnedString,
		   DWORD nSize)
{
   return GetPrivateProfileSectionW(lpAppName,
				    lpReturnedString,
				    nSize,
				    NULL);
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetProfileSectionA(LPCSTR lpAppName,
		   LPSTR lpReturnedString,
		   DWORD nSize)
{
   return GetPrivateProfileSectionA(lpAppName,
				    lpReturnedString,
				    nSize,
				    NULL);
}


/*
 * @implemented
 */
DWORD STDCALL
GetProfileStringW(LPCWSTR lpAppName,
		  LPCWSTR lpKeyName,
		  LPCWSTR lpDefault,
		  LPWSTR lpReturnedString,
		  DWORD nSize)
{
   return GetPrivateProfileStringW(lpAppName,
				   lpKeyName,
				   lpDefault,
				   lpReturnedString,
				   nSize,
				   NULL);
}


/*
 * @implemented
 */
DWORD STDCALL
GetProfileStringA(LPCSTR lpAppName,
		  LPCSTR lpKeyName,
		  LPCSTR lpDefault,
		  LPSTR lpReturnedString,
		  DWORD nSize)
{
   return GetPrivateProfileStringA(lpAppName,
				   lpKeyName,
				   lpDefault,
				   lpReturnedString,
				   nSize,
				   NULL);
}


/*
 * @unimplemented
 */
BOOL STDCALL
OpenProfileUserMapping (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL STDCALL
QueryWin31IniFilesMappedToRegistry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
WritePrivateProfileSectionA (
	LPCSTR	lpAppName,
	LPCSTR	lpString,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
WritePrivateProfileSectionW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpString,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
WritePrivateProfileStringA(LPCSTR AppName,
			   LPCSTR KeyName,
			   LPCSTR String,
			   LPCSTR FileName)
{
  UNICODE_STRING AppNameW, KeyNameW, StringW, FileNameW;
  BOOL Ret;

  if (NULL != AppName)
    {
      RtlCreateUnicodeStringFromAsciiz(&AppNameW, (PCSZ) AppName);
    }
  else
    {
      AppNameW.Buffer = NULL;
    }
  if (NULL != KeyName)
    {
      RtlCreateUnicodeStringFromAsciiz(&KeyNameW, (PCSZ) KeyName);
    }
  else
    {
      KeyNameW.Buffer = NULL;
    }
  if (NULL != String)
    {
      RtlCreateUnicodeStringFromAsciiz(&StringW, (PCSZ) String);
    }
  else
    {
      StringW.Buffer = NULL;
    }
  if (NULL != FileName)
    {
      RtlCreateUnicodeStringFromAsciiz(&FileNameW, (PCSZ) FileName);
    }
  else
    {
      FileNameW.Buffer = NULL;
    }

  Ret = WritePrivateProfileStringW(AppNameW.Buffer, KeyNameW.Buffer,
                                   StringW.Buffer, FileNameW.Buffer);

  RtlFreeUnicodeString(&AppNameW);
  RtlFreeUnicodeString(&KeyNameW);
  RtlFreeUnicodeString(&StringW);
  RtlFreeUnicodeString(&FileNameW);

  return Ret;
}


/*
 * @implemented
 */
BOOL STDCALL
WritePrivateProfileStringW(LPCWSTR AppName,
			   LPCWSTR KeyName,
			   LPCWSTR String,
			   LPCWSTR FileName)
{
  BOOL Ret = FALSE;

  RtlEnterCriticalSection(&ProfileLock);

  if (PROFILE_Open(FileName))
    {
      if (NULL == AppName && NULL == KeyName && NULL == String) /* documented "file flush" case */
        {
          PROFILE_FlushFile();
          PROFILE_ReleaseFile();  /* always return FALSE in this case */
        }
      else
        {
          if (NULL == AppName)
            {
              DPRINT1("(NULL?,%s,%s,%s)?\n", KeyName, String, FileName);
            }
          else
            {
              Ret = PROFILE_SetString(AppName, KeyName, String, FALSE);
              PROFILE_FlushFile();
            }
        }
    }

  RtlLeaveCriticalSection(&ProfileLock);

  return Ret;
}


/*
 * @unimplemented
 */
BOOL STDCALL
WritePrivateProfileStructA (
	IN LPCSTR Section,
	IN LPCSTR Key,
	IN LPVOID Struct,
	IN UINT StructSize,
	IN LPCSTR File
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
WritePrivateProfileStructW (
	IN LPCWSTR Section,
	IN LPCWSTR Key,
	IN LPVOID Struct,
	IN UINT StructSize,
	IN LPCWSTR File
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
WriteProfileSectionA(LPCSTR lpAppName,
		     LPCSTR lpString)
{
   return WritePrivateProfileSectionA(lpAppName,
				      lpString,
				      NULL);
}


/*
 * @unimplemented
 */
BOOL STDCALL
WriteProfileSectionW(LPCWSTR lpAppName,
		     LPCWSTR lpString)
{
   return WritePrivateProfileSectionW(lpAppName,
				      lpString,
				      NULL);
}


/*
 * @implemented
 */
BOOL STDCALL
WriteProfileStringA(LPCSTR AppName,
		    LPCSTR KeyName,
		    LPCSTR String)
{
   return WritePrivateProfileStringA(AppName,
				     KeyName,
				     String,
				     NULL);
}


/*
 * @implemented
 */
BOOL STDCALL
WriteProfileStringW(LPCWSTR AppName,
		    LPCWSTR KeyName,
		    LPCWSTR String)
{
   return WritePrivateProfileStringW(AppName,
				     KeyName,
				     String,
				     NULL);
}

/* EOF */
