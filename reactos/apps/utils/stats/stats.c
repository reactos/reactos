/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS project statistics
 * FILE:        stats.c
 * PURPOSE:     Main program file
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/01-2002 Created
 */
#include <windows.h>
#include <tchar.h>
#include <stdio.h>     

typedef struct _EXTENSION_INFO
{
  struct _EXTENSION_INFO * Next;
  struct _FILE_INFO * StatInfoList;
  TCHAR ExtName[16];
  TCHAR ExpandedExtName[128];
  DWORD nExtensions;
  TCHAR Description[256];
  DWORD FileCount;
  DWORD LineCount;
  DWORD EmptyLines;
} EXTENSION_INFO, *PEXTENSION_INFO;

typedef struct _FILE_INFO
{
  struct _FILE_INFO * Next;
  struct _FILE_INFO * StatInfoListNext;
  PEXTENSION_INFO ExtInfo;
  TCHAR FileName[MAX_PATH];
  DWORD LineCount;
  DWORD EmptyLines;
  DWORD FunctionCount;
} FILE_INFO, *PFILE_INFO;

HANDLE FileHandle;
PEXTENSION_INFO ExtInfoList;
PFILE_INFO StatInfoList;
BOOLEAN SkipEmptyLines, BeSilent;

#define MAX_OPTIONS	2
TCHAR *Options[MAX_OPTIONS];


VOID
Initialize(VOID)
{
  ExtInfoList = NULL;
  StatInfoList = NULL;
}


VOID
Cleanup(VOID)
{
  PEXTENSION_INFO ExtInfo;
  PEXTENSION_INFO NextExtInfo;

  ExtInfo = ExtInfoList;
  while (ExtInfo != NULL)
  {
    NextExtInfo = ExtInfo->Next;
    HeapFree (GetProcessHeap(), 0, ExtInfo);
    ExtInfo = NextExtInfo;
  }
}


PEXTENSION_INFO
AddExtension(LPTSTR ExtName,
  LPTSTR Description)
{
  PEXTENSION_INFO ExtInfo;
  PEXTENSION_INFO Info;
  TCHAR *t;
  DWORD ln;

  ExtInfo = (PEXTENSION_INFO) HeapAlloc (GetProcessHeap(), 0, sizeof (EXTENSION_INFO));
  if (!ExtInfo)
    return NULL;
  
  for(t = ExtName; *t != _T('\0'); t += _tcslen(t) + 1);
  ln = (DWORD)t - (DWORD)ExtName;
  
  ZeroMemory (ExtInfo, sizeof (EXTENSION_INFO));
  memcpy (ExtInfo->ExtName, ExtName, ln);
  _tcscpy (ExtInfo->Description, Description);
  
  for(t = ExtInfo->ExtName; *t != _T('\0'); t += _tcslen(t) + 1)
  {
    if(ExtInfo->nExtensions++ != 0)
      _tcscat (ExtInfo->ExpandedExtName, _T(";"));
    _tcscat (ExtInfo->ExpandedExtName, _T("*."));
    _tcscat (ExtInfo->ExpandedExtName, t);
  }
  
  if (ExtInfoList)
  {
    Info = ExtInfoList;
    while (Info->Next != NULL)
    {
      Info = Info->Next;
    }
    Info->Next = ExtInfo;
  }
  else
  {
    ExtInfoList = ExtInfo;
  }

  return ExtInfo;
}


PFILE_INFO
AddFile(LPTSTR FileName,
  PEXTENSION_INFO ExtInfo)
{
  PFILE_INFO StatInfo;
  PFILE_INFO Info;

  StatInfo = (PFILE_INFO) HeapAlloc (GetProcessHeap(), 0, sizeof (FILE_INFO));
  if (!StatInfo)
    return NULL;
  ZeroMemory (StatInfo, sizeof (FILE_INFO));
  _tcscpy (StatInfo->FileName, FileName);
  StatInfo->ExtInfo = ExtInfo;

  if (ExtInfo->StatInfoList)
  {
    Info = ExtInfo->StatInfoList;
    while (Info->StatInfoListNext != NULL)
    {
      Info = Info->StatInfoListNext;
    }
    Info->StatInfoListNext = StatInfo;
  }
  else
  {
    ExtInfo->StatInfoList = StatInfo;
  }

  if (StatInfoList)
  {
    Info = StatInfoList;
    while (Info->Next != NULL)
    {
      Info = Info->Next;
    }
    Info->Next = StatInfo;
  }
  else
  {
    StatInfoList = StatInfo;
  }

  return StatInfo;
}


VOID
CleanupAfterFile(VOID)
{
  if(FileHandle != INVALID_HANDLE_VALUE)
    CloseHandle (FileHandle);
}


BOOL
LoadFile(LPTSTR FileName)
{
  LONG FileSize;

  FileHandle = CreateFile (FileName, // Create this file
    GENERIC_READ,                    // Open for reading
    0,                               // No sharing
    NULL,                            // No security
    OPEN_EXISTING,                   // Open the file
    FILE_ATTRIBUTE_NORMAL,           // Normal file
    NULL);                           // No attribute template
  if (FileHandle == INVALID_HANDLE_VALUE)
    return FALSE;

  FileSize = GetFileSize (FileHandle, NULL);
  if (FileSize <= 0)
  {
    CloseHandle (FileHandle);
    return FALSE;
  }

  return TRUE;
}


VOID
ReadLines(PFILE_INFO StatInfo)
{
  DWORD ReadBytes, LineLen;
  static char FileBuffer[1024];
  char LastChar = '\0';
  char *Current;
  
  LineLen = 0;
  while(ReadFile (FileHandle, FileBuffer, sizeof(FileBuffer), &ReadBytes, NULL) && ReadBytes >= sizeof(char))
  {
    for(Current = FileBuffer; ReadBytes > 0; ReadBytes -= sizeof(char), Current++)
    {
      if(*Current == '\n' && LastChar == '\r')
      {
        LastChar = '\0';
        if(!SkipEmptyLines || (LineLen > 0))
          StatInfo->LineCount++;
        if(LineLen == 0)
          StatInfo->EmptyLines++;
        LineLen = 0;
        continue;
      }
      LastChar = *Current;
      if(SkipEmptyLines && (*Current == ' ' || *Current == '\t'))
      {
        continue;
      }
      if(*Current != '\r')
        LineLen++;
    }
  }
  
  StatInfo->LineCount += (LineLen > 0);
  StatInfo->EmptyLines += ((LastChar != '\0') && (LineLen == 0));
}


VOID
PrintStatistics(VOID)
{
  PEXTENSION_INFO Info;
  DWORD TotalFileCount;
  DWORD TotalLineCount;
  DWORD TotalAvgLF;
  DWORD TotalEmptyLines;

  TotalFileCount = 0;
  TotalLineCount = 0;
  TotalEmptyLines = 0;
  Info = ExtInfoList;
  
  for (Info = ExtInfoList; Info != NULL; Info = Info->Next)
  {
    TotalFileCount += Info->FileCount;
    TotalLineCount += Info->LineCount;
    TotalEmptyLines += Info->EmptyLines;
  }
  
  TotalAvgLF = (TotalFileCount ? TotalLineCount / TotalFileCount : 0);
  
  for (Info = ExtInfoList; Info != NULL; Info = Info->Next)
  {
    DWORD AvgLF;

    if (Info->FileCount != 0)
    {
      AvgLF = (Info->FileCount ? Info->LineCount / Info->FileCount : 0);
    }
    else
    {
      AvgLF = 0;
    }

    _tprintf (_T("\n"));
    _tprintf (_T("File extension%c             : %s\n"), ((Info->nExtensions > 1) ? _T('s') : _T(' ')), Info->ExpandedExtName);
    _tprintf (_T("File ext. description       : %s\n"), Info->Description);
    _tprintf (_T("Number of files             : %lu\n"), Info->FileCount);
    _tprintf (_T("Number of lines             : %lu\n"), Info->LineCount);
    if(SkipEmptyLines)
      _tprintf (_T("Number of empty lines       : %lu\n"), Info->EmptyLines);
    _tprintf (_T("Proportion of lines         : %.2f %%\n"), (float)(TotalLineCount ? (((float)Info->LineCount * 100) / (float)TotalLineCount) : 0));
    _tprintf (_T("Average no. lines/file      : %lu\n"), AvgLF);
  }

  _tprintf (_T("\n"));
  _tprintf (_T("Total number of files       : %lu\n"), TotalFileCount);
  _tprintf (_T("Total number of lines       : %lu\n"), TotalLineCount);
  if(SkipEmptyLines)
    _tprintf (_T("Total number of empty lines : %lu\n"), TotalEmptyLines);
  _tprintf (_T("Average no. lines/file      : %lu\n"), TotalAvgLF);
}


BOOL
ProcessFiles(LPTSTR Path)
{
  WIN32_FIND_DATA FindFile;
  PEXTENSION_INFO Info;
  TCHAR SearchPath[256];
  TCHAR FileName[256];
  TCHAR *Ext;
  HANDLE SearchHandle;
  BOOL More;

  Info = ExtInfoList;
  while (Info != NULL)
  {
   Ext = Info->ExtName;
   do
   {
    ZeroMemory (&FindFile, sizeof (FindFile));
    _tcscpy (SearchPath, Path);
    _tcscat (SearchPath, _T("\\*."));
    _tcscat (SearchPath, Ext);
    _tcscpy (FindFile.cFileName, SearchPath);
    SearchHandle = FindFirstFile (SearchPath, &FindFile);
    if (SearchHandle != INVALID_HANDLE_VALUE)
    {
      More = TRUE;
      while (More)
      {
	      if (!(FindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
  	    {
          _tcscpy (FileName, Path);
          _tcscat (FileName, _T("\\"));
          _tcscat (FileName, FindFile.cFileName);

				  if (LoadFile (FileName))
				  {
	          PFILE_INFO StatInfo;
	
	          StatInfo = AddFile (FindFile.cFileName, Info);
	          if (!StatInfo)
						{
	  			    _tprintf (_T("Not enough free memory.\n"));
	            return FALSE;
						}
	
				    ReadLines (StatInfo);
	
            Info->FileCount++;
	          Info->LineCount += StatInfo->LineCount;
	          Info->EmptyLines += StatInfo->EmptyLines;

	          CleanupAfterFile();
				  }
        }
        More = FindNextFile (SearchHandle, &FindFile);
      }
      FindClose (SearchHandle);
    }
    Ext += _tcslen(Ext) + 1;
   } while(*Ext != _T('\0'));
    Info = Info->Next;
  }
  return TRUE;
}


BOOL
ProcessDirectories(LPTSTR Path)
{
  WIN32_FIND_DATA FindFile;
  TCHAR SearchPath[MAX_PATH];
  HANDLE SearchHandle;
  BOOL More;

  if(!BeSilent)
  {
    _tprintf (_T("Processing %s ...\n"), Path);
  }

  _tcscpy (SearchPath, Path);
  _tcscat (SearchPath, _T("\\*.*"));

  SearchHandle = FindFirstFileEx (SearchPath,
    FindExInfoStandard,
    &FindFile,
    FindExSearchLimitToDirectories,
    NULL,
    0);
  if (SearchHandle != INVALID_HANDLE_VALUE)
  {
    More = TRUE;
    while (More)
    {
	    if ((FindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      && (_tcscmp (FindFile.cFileName, _T(".")) != 0)
      && (_tcscmp (FindFile.cFileName, _T("..")) != 0)
      && (_tcscmp (FindFile.cFileName, _T("CVS")) != 0))
			{
			  _tcscpy (SearchPath, Path);
			  _tcscat (SearchPath, _T("\\"));
			  _tcscat (SearchPath, FindFile.cFileName);
	      if (!ProcessDirectories (SearchPath))
          return FALSE;
	      if (!ProcessFiles (SearchPath))
          return FALSE;
			}
      More = FindNextFile (SearchHandle, &FindFile);
    }
    FindClose (SearchHandle);
  }
  return TRUE;
}


VOID
Execute(LPTSTR Path)
{
  if (!ExtInfoList)
  {
	  _tprintf (_T("No extensions specified.\n"));
    return;
  }

  if (!ProcessDirectories (Path))
  {
	  _tprintf (_T("Failed to process directories.\n"));
    return;
  }

  if (!ProcessFiles (Path))
  {
	  _tprintf (_T("Failed to process files.\n"));
    return;
  }

  PrintStatistics();
}

BOOLEAN
IsOptionSet(TCHAR *Option)
{
  int i;
  for(i = 0; i < MAX_OPTIONS; i++)
  {
    if(!Options[i])
      continue;
    
    if(!_tcscmp(Options[i], Option))
      return TRUE;
  }
  return FALSE;
}


int
main (int argc, char * argv [])
{
  int a;
#if UNICODE
  TCHAR Path[MAX_PATH + 1];
#endif

  _tprintf (_T("ReactOS Project Statistics\n"));
  _tprintf (_T("==========================\n\n"));

  if (argc < 2 || argc > 2 + MAX_OPTIONS)
  {
    _tprintf(_T("Usage: stats [-e] [-s] directory\n"));
    _tprintf(_T("  -e: don't count empty lines\n"));
    _tprintf(_T("  -s: be silent, don't print directories while processing\n"));
    return 1;
  }

  Initialize();
  AddExtension (_T("c\0\0"), _T("Ansi C Source files"));
  AddExtension (_T("cpp\0cxx\0\0"), _T("C++ Source files"));
  AddExtension (_T("h\0\0"), _T("Header files"));
  
  for(a = 1; a < argc - 1; a++)
  {
#if UNICODE
    int len = lstrlenA(argv[a]);
    TCHAR *str = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(TCHAR));
    if(MultiByteToWideChar(CP_ACP, 0, argv[a], -1, str, len + 1) > 0)
      Options[a - 1] = str;
    else
      Options[a - 1] = NULL;
#else
    Options[a - 1] = argv[a];
#endif
  }
  
  SkipEmptyLines = IsOptionSet(_T("-e"));
  BeSilent = IsOptionSet(_T("-s"));
  
#if UNICODE
  ZeroMemory(Path, sizeof(Path));
  if(MultiByteToWideChar(CP_ACP, 0, argv[argc - 1], -1, Path, MAX_PATH) > 0)
    Execute (Path);
#else
  Execute (argv[argc - 1]);
#endif
  Cleanup();

  return 0;
}
