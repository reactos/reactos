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
} EXTENSION_INFO, *PEXTENSION_INFO;

typedef struct _FILE_INFO
{
  struct _FILE_INFO * Next;
  struct _FILE_INFO * StatInfoListNext;
  PEXTENSION_INFO ExtInfo;
  TCHAR FileName[MAX_PATH];
  DWORD LineCount;
  DWORD FunctionCount;
} FILE_INFO, *PFILE_INFO;

HANDLE FileHandle;
DWORD TotalLineCount;
PEXTENSION_INFO ExtInfoList;
PFILE_INFO StatInfoList;


VOID
Initialize()
{
  TotalLineCount = 0;
  ExtInfoList = NULL;
  StatInfoList = NULL;
}


VOID
Cleanup()
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
CleanupAfterFile()
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


DWORD
ReadLines()
{
  DWORD ReadBytes, LineLen, Lines = 0;
  static char FileBuffer[1024];
  char LastChar = '\0';
  char *Current;
  
  LineLen = 0;
  while(ReadFile (FileHandle, FileBuffer, sizeof(FileBuffer), &ReadBytes, NULL) && ReadBytes >= sizeof(char))
  {
    if(ReadBytes & 0x1)
      ReadBytes--;
    
    for(Current = FileBuffer; ReadBytes > 0; ReadBytes -= sizeof(char), Current++)
    {
      if(*Current == '\n' && LastChar == '\r')
      {
        LastChar = '\0';
        if(LineLen > 0)
          Lines++;
        LineLen = 0;
      }
      LineLen++;
      LastChar = *Current;
    }
  }
  
  Lines += (LineLen > 0);
  return Lines;
}


VOID
DoStatisticsForFile(PFILE_INFO StatInfo)
{
  StatInfo->LineCount = ReadLines();
}


VOID
PrintStatistics()
{
  PEXTENSION_INFO Info;
  DWORD TotalFileCount;
  DWORD TotalLineCount;
  DWORD TotalAvgLF;

  TotalFileCount = 0;
  TotalLineCount = 0;
  Info = ExtInfoList;
  
  for (Info = ExtInfoList; Info != NULL; Info = Info->Next)
  {
    TotalFileCount += Info->FileCount;
    TotalLineCount += Info->LineCount;
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
    _tprintf (_T("File extension%c        : %s\n"), ((Info->nExtensions > 1) ? _T('s') : _T(' ')), Info->ExpandedExtName);
    _tprintf (_T("File ext. description  : %s\n"), Info->Description);
    _tprintf (_T("Number of files        : %lu\n"), Info->FileCount);
    _tprintf (_T("Number of lines        : %lu\n"), Info->LineCount);
    _tprintf (_T("Proportion of lines    : %.2f %%\n"), (float)(TotalLineCount ? (((float)Info->LineCount * 100) / (float)TotalLineCount) : 0));
    _tprintf (_T("Average no. lines/file : %lu\n"), AvgLF);
  }

  _tprintf (_T("\n"));
  _tprintf (_T("Total number of files  : %lu\n"), TotalFileCount);
  _tprintf (_T("Total number of lines  : %lu\n"), TotalLineCount);
  _tprintf (_T("Average no. lines/file : %lu\n"), TotalAvgLF);
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
	
				    DoStatisticsForFile (StatInfo);
	
            Info->FileCount++;
	          Info->LineCount += StatInfo->LineCount;

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

	_tprintf (_T("Processing %s ...\n"), Path);

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


int
main (int argc, char * argv [])
{
#if UNICODE
  TCHAR Path[MAX_PATH + 1];
#endif

  _tprintf (_T("\nReactOS project statistics generator.\n\n"));

  if (argc < 2)
  {
    _tprintf(_T("Usage: stats directory"));
    return 1;
  }

  Initialize();
  AddExtension (_T("c\0\0"), _T("Ansi C Source files"));
  AddExtension (_T("cpp\0cxx\0\0"), _T("C++ Source files"));
  AddExtension (_T("h\0\0"), _T("Header files"));
#if UNICODE
  ZeroMemory(Path, sizeof(Path));
  if(MultiByteToWideChar(CP_ACP, 0, argv[1], -1, Path, MAX_PATH) > 0)
    Execute (Path);
#else
  Execute (argv[1]);
#endif
  Cleanup();

  return 0;
}
