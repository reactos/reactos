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


typedef struct _EXTENSION_INFO
{
  struct _EXTENSION_INFO * Next;
  struct _FILE_INFO * StatInfoList;
  TCHAR ExtName[16];
  TCHAR Description[256];
  DWORD FileCount;
  DWORD LineCount;
} EXTENSION_INFO, *PEXTENSION_INFO;

typedef struct _FILE_INFO
{
  struct _FILE_INFO * Next;
  struct _FILE_INFO * StatInfoListNext;
  PEXTENSION_INFO ExtInfo;
  TCHAR FileName[256];
  DWORD LineCount;
  DWORD FunctionCount;
} FILE_INFO, *PFILE_INFO;


DWORD TotalLineCount;
PCHAR FileBuffer;
DWORD FileBufferSize;
CHAR Line[256];
DWORD CurrentOffset;
DWORD CurrentChar;
DWORD CurrentLine;
DWORD LineLength;
PEXTENSION_INFO ExtInfoList;
PFILE_INFO StatInfoList;


VOID
Initialize()
{
  TotalLineCount = 0;
  FileBuffer = NULL;
  FileBufferSize = 0;
  CurrentOffset = 0;
  CurrentLine = 0;
  LineLength = 0;
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

  ExtInfo = (PEXTENSION_INFO) HeapAlloc (GetProcessHeap(), 0, sizeof (EXTENSION_INFO));
  if (!ExtInfo)
    return NULL;
  ZeroMemory (ExtInfo, sizeof (EXTENSION_INFO));
  _tcscpy (ExtInfo->ExtName, ExtName);
  _tcscpy (ExtInfo->Description, Description);

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
  if (FileBuffer)
  {
    HeapFree (GetProcessHeap(), 0, FileBuffer);
    FileBuffer = NULL;
  }
}


BOOL
LoadFile(LPTSTR FileName)
{
  HANDLE FileHandle;
  DWORD BytesRead;
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
  if (FileSize < 0)
  {
    CloseHandle (FileHandle);
    return FALSE;
  }

  FileBufferSize = (DWORD) FileSize;

  FileBuffer = (PCHAR) HeapAlloc (GetProcessHeap(), 0, FileBufferSize);
  if (!FileBuffer)
  {
    CloseHandle (FileHandle);
    return FALSE;
  }

  if (!ReadFile (FileHandle, FileBuffer, FileBufferSize, &BytesRead, NULL))
  {
    CloseHandle(FileHandle);
    HeapFree (GetProcessHeap(), 0, FileBuffer);
    FileBuffer = NULL;
    return FALSE;
  }

  CloseHandle (FileHandle);

  CurrentOffset = 0;
  CurrentLine = 0;
  CurrentChar = 0;

  return TRUE;
}


BOOL
ReadLine()
/*
 * FUNCTION: Reads the next line into the line buffer
 * RETURNS:
 *     TRUE if there is a new line, FALSE if not
 */
{
  ULONG i, j;
  TCHAR ch;

  if (CurrentOffset >= FileBufferSize)
    return FALSE;

  i = 0;
  while (((j = CurrentOffset + i) < FileBufferSize) && (i < sizeof (Line)) &&
    ((ch = FileBuffer[j]) != 0x0D))
  {
    Line[i] = ch;
    i++;
  }

  Line[i]    = '\0';
  LineLength = i;
  
  if (FileBuffer[CurrentOffset + i + 1] == 0x0A)
    CurrentOffset++;

  CurrentOffset += i + 1;

  CurrentChar = 0;

  CurrentLine++;

  return TRUE;
}


VOID
DoStatisticsForFile(PFILE_INFO StatInfo)
{
  while (ReadLine())
  {
  }
  StatInfo->LineCount = CurrentLine;
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

  while (Info != NULL)
  {
    DWORD AvgLF;

    if (Info->FileCount != 0)
    {
      AvgLF = Info->LineCount / Info->FileCount;
    }
    else
    {
      AvgLF = 0;
    }

    _tprintf (_T("\n"));
    _tprintf (_T("File extension         : %s\n"), Info->ExtName);
    _tprintf (_T("File ext. description  : %s\n"), Info->Description);
    _tprintf (_T("Number of files        : %d\n"), Info->FileCount);
    _tprintf (_T("Number of lines        : %d\n"), Info->LineCount);
    _tprintf (_T("Average no. lines/file : %d\n"), AvgLF);

    TotalFileCount += Info->FileCount;
    TotalLineCount += Info->LineCount;

    Info = Info->Next;
  }

  TotalAvgLF = TotalLineCount / TotalFileCount;

  _tprintf (_T("\n"));
  _tprintf (_T("Total number of files  : %d\n"), TotalFileCount);
  _tprintf (_T("Total number of lines  : %d\n"), TotalLineCount);
  _tprintf (_T("Average no. lines/file : %d\n"), TotalAvgLF);
}


BOOL
ProcessFiles(LPTSTR Path)
{
  WIN32_FIND_DATA FindFile;
  PEXTENSION_INFO Info;
  TCHAR SearchPath[256];
  TCHAR FileName[256];
  HANDLE SearchHandle;
  BOOL More;

  Info = ExtInfoList;
  while (Info != NULL)
  {
    ZeroMemory (&FindFile, sizeof (FindFile));
    _tcscpy (SearchPath, Path);
    _tcscat (SearchPath, _T("\\*."));
    _tcscat (SearchPath, Info->ExtName);
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

	_tprintf (_T("Processing directory %s\n"), Path);

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
  _tprintf (_T("\nReactOS project statistics generator.\n\n"));

  if (argc < 2)
  {
    _tprintf(_T("Usage: stats directory"));
    return 1;
  }

  Initialize();
  AddExtension (_T("c"), _T("Source files"));
  AddExtension (_T("h"), _T("Header files"));
  Execute (argv[1]);
  Cleanup();

  return 0;
}
