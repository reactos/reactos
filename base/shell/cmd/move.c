/*
*  MOVE.C - move internal command.
*
*
*  History:
*
*    14-Dec-1998 (Eric Kohl)
*        Started.
*
*    18-Jan-1999 (Eric Kohl)
*        Unicode safe!
*        Preliminary version!!!
*
*    20-Jan-1999 (Eric Kohl)
*        Redirection safe!
*
*    27-Jan-1999 (Eric Kohl)
*        Added help text ("/?").
*        Added more error checks.
*
*    03-Feb-1999 (Eric Kohl)
*        Added "/N" option.
*
*    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
*        Remove all hardcode string to En.rc
*
*    24-Jun-2005 (Brandon Turner) <turnerb7@msu.edu>)
*        Fixed bug to allow MS style wildcards + code clean up
*        added /y and /-y
*/

#include <precomp.h>

#ifdef INCLUDE_CMD_MOVE

enum
{
	MOVE_NOTHING  = 0x001,   /* /N  */
	MOVE_OVER_YES = 0x002,   /* /Y  */
	MOVE_OVER_NO  = 0x004,   /* /-Y */
};

enum
{	/* Move status flags */
	MOVE_SOURCE_IS_DIR = 0x001,
	MOVE_SOURCE_IS_FILE = 0x002,
	MOVE_DEST_IS_DIR = 0x004,
	MOVE_DEST_IS_FILE = 0x008,
	MOVE_SOURCE_HAS_WILD = 0x010, /*  source has wildcard */
	MOVE_SRC_CURRENT_IS_DIR = 0x020, /* source is file but at the current round we found a directory */
	MOVE_DEST_EXISTS = 0x040,
	MOVE_PATHS_ON_DIF_VOL = 0x080 /* source and destination paths are on different volume */
};

static INT MoveOverwrite (LPTSTR fn)
{
	/*ask the user if they want to override*/
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	INT res;
	LoadString(CMD_ModuleHandle, STRING_MOVE_HELP1, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg,fn);
	res = FilePromptYNA (_T(""));
	return res;
}

void GetDirectory (LPTSTR wholepath, LPTSTR directory, BOOL CheckExisting)
{
	/* returns only directory part of path with backslash */
	/* TODO: make code unc aware */
	/* Is there a better alternative to this? */
	LPTSTR last;
	if (CheckExisting && IsExistingDirectory(wholepath))
	{
		_tcscpy(directory, wholepath);
	}
	else if ((last = _tcsrchr(wholepath,_T('\\'))) != NULL)
	{
		_tcsncpy(directory, wholepath, last - wholepath + 1);
		directory[last - wholepath + 1] = 0;
	}
	else
	{
		GetRootPath(wholepath,directory, MAX_PATH);
	}
}


INT cmd_move (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	LPTSTR *arg;
	INT argc, i, nFiles;
	TCHAR szDestPath[MAX_PATH];
	TCHAR szFullDestPath[MAX_PATH];
	TCHAR szSrcDirPath[MAX_PATH];
	TCHAR szSrcPath[MAX_PATH];
	TCHAR szFullSrcPath[MAX_PATH];
	DWORD dwFlags = 0;
	INT nOverwrite = 0;
	WIN32_FIND_DATA findBuffer;
	HANDLE hFile;
	
	/* used only when source and destination  directories are on different volume*/
	HANDLE hDestFile;
	WIN32_FIND_DATA findDestBuffer;
	TCHAR szMoveDest[MAX_PATH];
	TCHAR szMoveSrc[MAX_PATH];
	LPTSTR pszDestDirPointer;
	LPTSTR pszSrcDirPointer;
	INT nDirLevel = 0;
	
	LPTSTR pszFile;
	BOOL OnlyOneFile;
	BOOL FoundFile;
	BOOL MoveStatus;
	DWORD dwMoveFlags = 0;
	DWORD dwMoveStatusFlags = 0;


	if (!_tcsncmp (param, _T("/?"), 2))
	{
#if 0
		ConOutPuts (_T("Moves files and renames files and directories.\n\n"
			"To move one or more files:\n"
			"MOVE [/N][/Y|/-Y][drive:][path]filename1[,...] destination\n"
			"\n"
			"To rename a directory:\n"
			"MOVE [/N][/Y|/-Y][drive:][path]dirname1 dirname2\n"
			"\n"
			"  [drive:][path]filename1  Specifies the location and name of the file\n"
			"                           or files you want to move.\n"
			"  /N                       Nothing. Don everthing but move files or direcories.\n"
			"  /Y\n"
			"  /-Y\n"
			"..."));
#else
		ConOutResPaging(TRUE,STRING_MOVE_HELP2);
#endif
		return 0;
	}

	nErrorLevel = 0;
	arg = split (param, &argc, FALSE);
	nFiles = argc;

	/* read options */
	for (i = 0; i < argc; i++)
	{
		if (*arg[i] == _T('/'))
		{
			if (_tcslen(arg[i]) >= 2)
			{
				switch (_totupper(arg[i][1]))
				{
				case _T('N'):
					dwFlags |= MOVE_NOTHING;
					break;
				
				case _T('Y'):
					dwFlags |= MOVE_OVER_YES;
					break;
				
				case _T('-'):
					dwFlags |= MOVE_OVER_NO;
					break;
				}
			}
			nFiles--;
		}
	}
	
	if (nFiles < 2)
	{
		/* there must be at least two pathspecs */
		error_req_param_missing ();
		return 1;
	}
	
	/* check for wildcards in source and destination */
	if (_tcschr (arg[argc - 1], _T('*')) != NULL || _tcschr (arg[argc - 1], _T('?')) != NULL)
	{
		/* '*'/'?' in dest, this doesnt happen.  give folder name instead*/
		error_invalid_parameter_format(arg[argc - 1]);
		return 1;
	}
	if (_tcschr (arg[argc - 2], _T('*')) != NULL || _tcschr (arg[argc - 2], _T('?')) != NULL)
	{
		dwMoveStatusFlags |= MOVE_SOURCE_HAS_WILD;
	}
	
	
	/* get destination */
	GetFullPathName (arg[argc - 1], MAX_PATH, szDestPath, NULL);
#ifdef _DEBUG
	DebugPrintf (_T("Destination: %s\n"), szDestPath);
#endif
	
	/* get source folder */
	GetDirectory(arg[argc - 2], szSrcDirPath, 1);
	GetFullPathName(szSrcDirPath, MAX_PATH, szSrcPath, &pszFile);
	_tcscpy(szSrcDirPath,szSrcPath);
	/* we need following check to see if source happens to be directly given directory
	and if it is then rip off last directory part so that there won't be any clashes with codes after this point */
	GetFullPathName(arg[argc - 2], MAX_PATH, szSrcPath, &pszFile);
	if (_tcscmp(szSrcDirPath,szSrcPath) == 0)
		szSrcDirPath[pszFile - szSrcPath] = _T('\0');
#ifdef _DEBUG
	DebugPrintf (_T("Source Folder: %s\n"), szSrcDirPath);
#endif
	
	hFile = FindFirstFile (arg[argc - 2], &findBuffer);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		ErrorMessage (GetLastError (), arg[argc - 2]);
		freep (arg);
		return 1;
		
	}

	/* check for special cases "." and ".." and if found skip them */
	FoundFile = TRUE;
	while(FoundFile &&
		  (_tcscmp(findBuffer.cFileName,_T(".")) == 0 ||
		   _tcscmp(findBuffer.cFileName,_T("..")) == 0))
		FoundFile = FindNextFile (hFile, &findBuffer);
	
	if (!FoundFile)
	{
		/* what? we don't have anything to move? */
		error_file_not_found();
		FindClose(hFile);
		freep(arg);
		return 1;
	}
	
	OnlyOneFile = TRUE;
	_tcscpy(szSrcPath,szSrcDirPath);
	/*check to see if there is an ending slash, if not add one*/
	if(szSrcPath[_tcslen(szSrcPath) -  1] != _T('\\'))
		_tcscat (szSrcPath, _T("\\"));
	_tcscat(szSrcPath,findBuffer.cFileName);
#ifdef _DEBUG
	DebugPrintf (_T("Source Path: %s\n"), szSrcPath);
#endif
	/* check if there can be found files as files have first priority */
	if (IsExistingFile(szSrcPath)) dwMoveStatusFlags |= MOVE_SOURCE_IS_FILE;
	else dwMoveStatusFlags |= MOVE_SOURCE_IS_DIR;
	while(OnlyOneFile && FindNextFile(hFile,&findBuffer))
	{
		_tcscpy(szSrcPath,szSrcDirPath);
		if(szSrcPath[_tcslen(szSrcPath) -  1] != _T('\\'))
			_tcscat (szSrcPath, _T("\\"));
		_tcscat(szSrcPath,findBuffer.cFileName);
		if (IsExistingFile(szSrcPath))
		{
			ConOutPrintf(_T(""));
			if (dwMoveStatusFlags & MOVE_SOURCE_IS_FILE) OnlyOneFile = FALSE;
			else
			{	/* this has been done this way so that we don't disturb other settings if they have been set before this */
				dwMoveStatusFlags |= MOVE_SOURCE_IS_FILE;
				dwMoveStatusFlags &= ~MOVE_SOURCE_IS_DIR;
			}
		}
	}
	FindClose(hFile);

#ifdef _DEBUG
	DebugPrintf(_T("Do we have only one file: %s\n"), OnlyOneFile ? _T("TRUE") : _T("FALSE"));
#endif
	
	/* we have to start again to be sure we don't miss any files or folders*/
	hFile = FindFirstFile (arg[argc - 2], &findBuffer);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		ErrorMessage (GetLastError (), arg[argc - 2]);
		freep (arg);
		return 1;
		
	}
	
	/* check for special cases "." and ".." and if found skip them */
	FoundFile = TRUE;
	while(FoundFile &&
		  (_tcscmp(findBuffer.cFileName,_T(".")) == 0 ||
		   _tcscmp(findBuffer.cFileName,_T("..")) == 0))
		FoundFile = FindNextFile (hFile, &findBuffer);
	
	if (!FoundFile)
	{
		/* huh? somebody removed files and/or folders which were there */
		error_file_not_found();
		FindClose(hFile);
		freep(arg);
		return 1;
	}
	
	/* check if source and destination paths are on different volumes */
	if (szSrcPath[0] != szDestPath[0])
		dwMoveStatusFlags |= MOVE_PATHS_ON_DIF_VOL;
	
	/* move it */
	do
	{
#ifdef _DEBUG
		DebugPrintf (_T("Found file/directory: %s\n"), findBuffer.cFileName);
#endif
		nOverwrite = 1;
		dwMoveFlags = 0;
		dwMoveStatusFlags &= ~MOVE_DEST_IS_FILE &
							~MOVE_DEST_IS_DIR &
							~MOVE_SRC_CURRENT_IS_DIR &
							~MOVE_DEST_EXISTS;
		_tcscpy(szFullSrcPath,szSrcDirPath);
		if(szFullSrcPath[_tcslen(szFullSrcPath) -  1] != _T('\\'))
			_tcscat (szFullSrcPath, _T("\\"));
		_tcscat(szFullSrcPath,findBuffer.cFileName);
		_tcscpy(szSrcPath, szFullSrcPath);
		
		if (IsExistingDirectory(szSrcPath))
		{
			/* source is directory */
			
			if (dwMoveStatusFlags & MOVE_SOURCE_IS_FILE)
			{
				dwMoveStatusFlags |= MOVE_SRC_CURRENT_IS_DIR; /* source is file but at the current round we found a directory */
				continue;
			}
#ifdef _DEBUG
			DebugPrintf (_T("Source is dir: %s\n"), szSrcPath);
#endif
			dwMoveFlags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED;
		}
		
		/* if source is file we don't need to do anything special */
		
		if (IsExistingDirectory(szDestPath))
		{
			/* destination is existing directory */
#ifdef _DEBUG
			DebugPrintf (_T("Destination is directory: %s\n"), szDestPath);
#endif
			
			dwMoveStatusFlags |= MOVE_DEST_IS_DIR;
			
			/*build the dest string(accounts for *)*/
			_tcscpy (szFullDestPath, szDestPath);
			/*check to see if there is an ending slash, if not add one*/
			if(szFullDestPath[_tcslen(szFullDestPath) -  1] != _T('\\'))
				_tcscat (szFullDestPath, _T("\\"));
			_tcscat (szFullDestPath, findBuffer.cFileName);
			
			if (IsExistingFile(szFullDestPath) || IsExistingDirectory(szFullDestPath))
				dwMoveStatusFlags |= MOVE_DEST_EXISTS;
			
			dwMoveFlags |= MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED;
			
		}
		if (IsExistingFile(szDestPath))
		{
			/* destination is a file */
#ifdef _DEBUG
			DebugPrintf (_T("Destination is file: %s\n"), szDestPath);
#endif
			
			dwMoveStatusFlags |= MOVE_DEST_IS_FILE | MOVE_DEST_EXISTS;
			_tcscpy (szFullDestPath, szDestPath);
			
			dwMoveFlags |= MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED;
			
		}
		
#ifdef _DEBUG
		DebugPrintf(_T("Move Status Flags: 0x%X\n"),dwMoveStatusFlags);
#endif
		
		if (dwMoveStatusFlags & MOVE_SOURCE_IS_DIR &&
			dwMoveStatusFlags & MOVE_DEST_IS_DIR &&
			dwMoveStatusFlags & MOVE_SOURCE_HAS_WILD)
		{
			/* We are not allowed to have existing source and destination dir when there is wildcard in source */
			error_syntax(NULL);
			FindClose(hFile);
			freep(arg);
			return 1;
		}
			
		if (!(dwMoveStatusFlags & (MOVE_DEST_IS_FILE | MOVE_DEST_IS_DIR)))
		{
			/* destination doesn't exist */
			_tcscpy (szFullDestPath, szDestPath);
			if (dwMoveStatusFlags & MOVE_SOURCE_IS_FILE) dwMoveStatusFlags |= MOVE_DEST_IS_FILE;
			if (dwMoveStatusFlags & MOVE_SOURCE_IS_DIR) dwMoveStatusFlags |= MOVE_DEST_IS_DIR;
			
			dwMoveFlags |= MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED;
		}
		
		if (dwMoveStatusFlags & MOVE_SOURCE_IS_FILE &&
			dwMoveStatusFlags & MOVE_DEST_IS_FILE &&
			!OnlyOneFile)
		{
			/*source has many files but there is only one destination file*/
			error_invalid_parameter_format(arg[argc - 1]);
			FindClose(hFile);
			freep (arg);
			return 1;
		}
		
		/*checks to make sure user wanted/wants the override*/
		if((dwFlags & MOVE_OVER_NO) &&
		   (dwMoveStatusFlags & MOVE_DEST_EXISTS))
			continue;
		if(!(dwFlags & MOVE_OVER_YES) &&
		    (dwMoveStatusFlags & MOVE_DEST_EXISTS))
			nOverwrite = MoveOverwrite (szFullDestPath);
		if (nOverwrite == PROMPT_NO || nOverwrite == PROMPT_BREAK)
			continue;
		if (nOverwrite == PROMPT_ALL)
			dwFlags |= MOVE_OVER_YES;
		
			
		ConOutPrintf (_T("%s => %s "), szSrcPath, szFullDestPath);
		
		/* are we really supposed to do something */
		if (dwFlags & MOVE_NOTHING)
			continue;
		
		/*move the file*/
		if (!(dwMoveStatusFlags & MOVE_SOURCE_IS_DIR &&
			dwMoveStatusFlags & MOVE_PATHS_ON_DIF_VOL))
			/* we aren't moving source folder to different drive */
			MoveStatus = MoveFileEx (szSrcPath, szFullDestPath, dwMoveFlags);
		else
		{	/* we are moving source folder to different drive */
			_tcscpy(szMoveDest, szFullDestPath);
			_tcscpy(szMoveSrc, szSrcPath);
			DeleteFile(szMoveDest);
			MoveStatus = CreateDirectory(szMoveDest, NULL); /* we use default security settings */
			if (MoveStatus)
			{
				_tcscat(szMoveDest,_T("\\"));
				_tcscat(szMoveSrc,_T("\\"));
				nDirLevel = 0;
				pszDestDirPointer = szMoveDest + _tcslen(szMoveDest);
				pszSrcDirPointer = szMoveSrc + _tcslen(szMoveSrc);
				_tcscpy(pszSrcDirPointer,_T("*.*"));
				hDestFile = FindFirstFile(szMoveSrc, &findDestBuffer);
				if (hDestFile == INVALID_HANDLE_VALUE)
					MoveStatus = FALSE;
				else
				{
					BOOL FirstTime = TRUE;
					FoundFile = TRUE;
					MoveStatus = FALSE;
					while(FoundFile)
					{
						if (FirstTime)
							FirstTime = FALSE;
						else
							FoundFile = FindNextFile (hDestFile, &findDestBuffer);
						
						if (!FoundFile)
						{	/* Nothing to do in this folder so we stop working on it */
							FindClose(hDestFile);
							(pszSrcDirPointer)--;
							(pszDestDirPointer)--;
							_tcscpy(pszSrcDirPointer,_T(""));
							_tcscpy(pszDestDirPointer,_T(""));
							if (nDirLevel > 0)
							{
								TCHAR szTempPath[MAX_PATH];
								INT nDiff;

								FoundFile = TRUE; /* we need to continue our seek for files */
								nDirLevel--;
								RemoveDirectory(szMoveSrc);
								GetDirectory(szMoveSrc,szTempPath,0);
								nDiff = _tcslen(szMoveSrc) - _tcslen(szTempPath);
								pszSrcDirPointer = pszSrcDirPointer - nDiff;
								_tcscpy(pszSrcDirPointer,_T(""));
								GetDirectory(szMoveDest,szTempPath,0);
								nDiff = _tcslen(szMoveDest) - _tcslen(szTempPath);
								pszDestDirPointer = pszDestDirPointer - nDiff;
								_tcscpy(pszDestDirPointer,_T(""));
								if(szMoveSrc[_tcslen(szMoveSrc) -  1] != _T('\\'))
									_tcscat (szMoveSrc, _T("\\"));
								if(szMoveDest[_tcslen(szMoveDest) -  1] != _T('\\'))
									_tcscat (szMoveDest, _T("\\"));
								pszDestDirPointer = szMoveDest + _tcslen(szMoveDest);
								pszSrcDirPointer = szMoveSrc + _tcslen(szMoveSrc);
								_tcscpy(pszSrcDirPointer,_T("*.*"));
								hDestFile = FindFirstFile(szMoveSrc, &findDestBuffer);
								if (hDestFile == INVALID_HANDLE_VALUE)
									continue;
								FirstTime = TRUE;
							}
							else
							{
								MoveStatus = TRUE; /* we moved everything so lets tell user about it */
								RemoveDirectory(szMoveSrc);
							}
							continue;
						}
						
						/* if we find "." or ".." we'll skip them */
						if (_tcscmp(findDestBuffer.cFileName,_T(".")) == 0 ||
							_tcscmp(findDestBuffer.cFileName,_T("..")) == 0)
							continue;
					
						_tcscpy(pszSrcDirPointer, findDestBuffer.cFileName);
						_tcscpy(pszDestDirPointer, findDestBuffer.cFileName);
						if (IsExistingFile(szMoveSrc))
						{
							FoundFile = CopyFile(szMoveSrc, szMoveDest, FALSE);
							if (!FoundFile) continue;
							DeleteFile(szMoveSrc);
						}
						else
						{
							FindClose(hDestFile);
							CreateDirectory(szMoveDest, NULL);
							_tcscat(szMoveDest,_T("\\"));
							_tcscat(szMoveSrc,_T("\\"));
							nDirLevel++;
							pszDestDirPointer = szMoveDest + _tcslen(szMoveDest);
							pszSrcDirPointer = szMoveSrc + _tcslen(szMoveSrc);
							_tcscpy(pszSrcDirPointer,_T("*.*"));
							hDestFile = FindFirstFile(szMoveSrc, &findDestBuffer);
							if (hDestFile == INVALID_HANDLE_VALUE)
							{
								FoundFile = FALSE;
								continue;
							}
							FirstTime = TRUE;
						}
					}
				}
			}
		}
		if (MoveStatus)
			LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		else
			LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR2, szMsg, RC_STRING_MAX_SIZE);
		
		ConOutPrintf(szMsg);
	}
	while ((!OnlyOneFile || dwMoveStatusFlags & MOVE_SRC_CURRENT_IS_DIR ) &&
			!(dwMoveStatusFlags & MOVE_SOURCE_IS_DIR) &&
			FindNextFile (hFile, &findBuffer));
	FindClose (hFile);
	
	freep (arg);
	return 0;
}

#endif /* INCLUDE_CMD_MOVE */
