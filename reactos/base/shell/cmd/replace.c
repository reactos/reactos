/*
 * PROJECT:         ReactOS Command shell
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/shell/cmd/replace.c
 * PURPOSE:         Implements 'replace' cmd command
 * PROGRAMMERS:     Samuel Erdtman (samuel@erdtman.se)
 */

/* INCLUDES ******************************************************************/

#include <precomp.h>

#ifdef INCLUDE_CMD_REPLACE

/* GLOBALS *******************************************************************/

enum
{
	REPLACE_ADD       = 0x001,   /* /A  */
	REPLACE_CONFIRM   = 0x002,   /* /P  */
	REPLACE_READ_ONLY = 0x004,   /* /R */
	REPLACE_SUBDIR    = 0x008,   /* /S  */
	REPLACE_DISK      = 0x010,   /* /W  */
	REPLACE_UPDATE    = 0x020,   /* /U */
};

/* FUNCTIONS *****************************************************************/

/*just makes a print out if there is a problem with the switches*/
void invalid_switch(LPTSTR is)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LoadString(CMD_ModuleHandle, STRING_REPLACE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg,is);
	ConOutResPaging(TRUE,STRING_REPLACE_HELP3);
}

/*retrives the pathe dependen om the input file name*/
void getPath(TCHAR* out, LPTSTR in)
{
	if(_tcslen(in) == 2 && in[1] == _T(':'))
		GetRootPath(in,out,MAX_PATH);
	else
		GetFullPathName (in, MAX_PATH, out, NULL);
}


/*makes the replace*/
INT replace(TCHAR source[MAX_PATH], TCHAR dest[MAX_PATH], DWORD dwFlags, BOOL *doMore)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR d[MAX_PATH];
	TCHAR s[MAX_PATH];
	HANDLE hFileSrc, hFileDest;
	DWORD  dwAttrib, dwRead, dwWritten;
	LPBYTE buffer;
	BOOL   bEof = FALSE;
	FILETIME srcCreationTime, destCreationTime, srcLastAccessTime, destLastAccessTime;
	FILETIME srcLastWriteTime, destLastWriteTime;
	GetPathCase(source, s);
	GetPathCase(dest, d);
	s[0] = _totupper(s[0]);
	d[0] = _totupper(d[0]);
// 	ConOutPrintf(_T("old-src:  %s\n"), s);
// 	ConOutPrintf(_T("old-dest: %s\n"), d);
// 	ConOutPrintf(_T("src:  %s\n"), source);
// 	ConOutPrintf(_T("dest: %s\n"), dest);

	/* Open up the sourcefile */
	hFileSrc = CreateFile (source, GENERIC_READ, FILE_SHARE_READ,NULL, OPEN_EXISTING, 0, NULL);
	if (hFileSrc == INVALID_HANDLE_VALUE)
	{
		LoadString(CMD_ModuleHandle, STRING_COPY_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		ConOutPrintf(szMsg, source);
		return 0;
	}

	/* Get the time from source file to be used in the comparison with
	   dest time if update switch is set */
	GetFileTime (hFileSrc, &srcCreationTime, &srcLastAccessTime, &srcLastWriteTime);

	/* Retrieve the source attributes so that they later on can be
	   inserted in to the destination */
	dwAttrib = GetFileAttributes (source);

	if(IsExistingFile (dest))
	{
		/* Resets the attributes to avoid probles with read only files,
		   checks for read only has been made earlier */
		SetFileAttributes(dest,FILE_ATTRIBUTE_NORMAL);
		/* Is the update flas set? The time has to be controled so that
		   only older files are replaced */
		if(dwFlags & REPLACE_UPDATE)
		{
			/* Read destination time */
			hFileDest = CreateFile(dest, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
				0, NULL);

			if (hFileSrc == INVALID_HANDLE_VALUE)
			{
				LoadString(CMD_ModuleHandle, STRING_COPY_ERROR1, szMsg, RC_STRING_MAX_SIZE);
				ConOutPrintf(szMsg, dest);
				return 0;
			}

			/* Compare time */
			GetFileTime (hFileDest, &destCreationTime, &destLastAccessTime, &destLastWriteTime);
			if(!((srcLastWriteTime.dwHighDateTime > destLastWriteTime.dwHighDateTime) ||
					(	srcLastWriteTime.dwHighDateTime == destLastWriteTime.dwHighDateTime &&
						srcLastWriteTime.dwLowDateTime > destLastWriteTime.dwLowDateTime)))
			{
				CloseHandle (hFileSrc);
				CloseHandle (hFileDest);
				return 0;
			}
			CloseHandle (hFileDest);
		}
		/* Delete the old file */
		DeleteFile (dest);
	}

	/* Check confirm flag, and take appropriate action */
	if(dwFlags & REPLACE_CONFIRM)
	{
		/* Output depending on add flag */
		if(dwFlags & REPLACE_ADD)
			LoadString(CMD_ModuleHandle, STRING_REPLACE_HELP9, szMsg, RC_STRING_MAX_SIZE);
		else
			LoadString(CMD_ModuleHandle, STRING_REPLACE_HELP10, szMsg, RC_STRING_MAX_SIZE);
		ConOutPrintf(szMsg, dest);
		if( !FilePromptYNA (_T("")))
			return 0;
	}

	/* Output depending on add flag */
	if(dwFlags & REPLACE_ADD)
		LoadString(CMD_ModuleHandle, STRING_REPLACE_HELP11, szMsg, RC_STRING_MAX_SIZE);
	else
		LoadString(CMD_ModuleHandle, STRING_REPLACE_HELP5, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg,dest);

	/* Make sure source and destination is not the same */
	if(!_tcscmp(s, d))
	{
		ConOutResPaging(TRUE, STRING_REPLACE_ERROR7);
		CloseHandle (hFileSrc);
		*doMore = FALSE;
		return 0;
	}

	/* Open destination file to write to */
	hFileDest = CreateFile (dest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFileDest == INVALID_HANDLE_VALUE)
	{
		CloseHandle (hFileSrc);
		ConOutResPaging(TRUE, STRING_REPLACE_ERROR7);
		*doMore = FALSE;
		return 0;
	}

	/* Get buffer for the copy process */
	buffer = (LPBYTE)VirtualAlloc(NULL, BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);
	if (buffer == NULL)
	{
		CloseHandle (hFileDest);
		CloseHandle (hFileSrc);
		ConOutResPaging(TRUE, STRING_ERROR_OUT_OF_MEMORY);
		return 0;
	}

	/* Put attribute and time to the new destination file */
	SetFileAttributes (dest, dwAttrib);
	SetFileTime (hFileDest, &srcCreationTime, &srcLastAccessTime, &srcLastWriteTime);
	do
	{
		/* Read data from source */
		ReadFile (hFileSrc, buffer, BUFF_SIZE, &dwRead, NULL);

		/* Done? */
		if (dwRead == 0)
			break;

		/* Write to destination file */
		WriteFile (hFileDest, buffer, dwRead, &dwWritten, NULL);

		/* Done! or ctrl break! */
		if (dwWritten != dwRead || CheckCtrlBreak(BREAK_INPUT))
		{
			ConOutResPuts(STRING_COPY_ERROR3);
			cmd_free (buffer);
			CloseHandle (hFileDest);
			CloseHandle (hFileSrc);
			nErrorLevel = 1;
			return 0;
		}
	}
	while (!bEof);

	/* Return memory and close files */
	VirtualFree (buffer, 0, MEM_RELEASE);
	CloseHandle (hFileDest);
	CloseHandle (hFileSrc);

	/* Return one file replaced */
	return 1;
}


/* Function to iterate over source files and call replace for each of them */
INT recReplace(DWORD dwFlags, TCHAR szSrcPath[MAX_PATH], TCHAR szDestPath[MAX_PATH], BOOL *doMore)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE], tmpDestPath[MAX_PATH], tmpSrcPath[MAX_PATH];
	INT filesReplaced=0, i;
	DWORD dwAttrib = 0;
	HANDLE hFile;
	WIN32_FIND_DATA findBuffer;

	/* Get file handel to the sourcefile(s) */
	hFile = FindFirstFile (szSrcPath, &findBuffer);

	/* Strip the paths back to the folder they are in, so that the diffrent
	   filenames can be added if more than one */
	for(i = (_tcslen(szSrcPath) -  1); i > -1; i--)
		if(szSrcPath[i] != _T('\\'))
			szSrcPath[i] = _T('\0');
		else
			break;

	/* Go through all the soursfiles and copy/replace them */
	do
	{
		if(CheckCtrlBreak(BREAK_INPUT))
		{
			return filesReplaced;
		}

		/* Problem with file handler */
		if(hFile == INVALID_HANDLE_VALUE)
			return filesReplaced;

		/* We do not want to replace any .. . ocr directory */
		if(!_tcscmp (findBuffer.cFileName, _T("."))  ||
				!_tcscmp (findBuffer.cFileName, _T(".."))||
				findBuffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

		/* Add filename to destpath */
		_tcscpy(tmpDestPath,szDestPath);
		_tcscat (tmpDestPath, findBuffer.cFileName);

		dwAttrib = GetFileAttributes(tmpDestPath);
		/* Check add flag */
		if(dwFlags & REPLACE_ADD)
		{
			if(IsExistingFile(tmpDestPath))
				continue;
			else
				dwAttrib = 0;
		}
		else
		{
			if(!IsExistingFile(tmpDestPath))
				continue;
		}

		/* Check if file is read only, if so check if that should be ignored */
		if(dwAttrib & FILE_ATTRIBUTE_READONLY)
		{
			if(!(dwFlags & REPLACE_READ_ONLY))
			{
				LoadString(CMD_ModuleHandle, STRING_REPLACE_ERROR5, szMsg, RC_STRING_MAX_SIZE);
				ConOutPrintf(szMsg, tmpDestPath);
				*doMore = FALSE;
				break;
			}
		}

		/* Add filename to sourcepath, insted of wildcards */
		_tcscpy(tmpSrcPath,szSrcPath);
		_tcscat (tmpSrcPath, findBuffer.cFileName);

		/* Make the replace */
		if(replace(tmpSrcPath,tmpDestPath, dwFlags, doMore))
		{
			filesReplaced++;
		}
		else if (!*doMore)
		{
			/* The file to be replaced was the same as the source */
			filesReplaced = -1;
			break;
		}

	/* Take next sourcefile if any */
	}while(FindNextFile (hFile, &findBuffer));

	return filesReplaced;
}

/* If /s switch is specifyed all subdirs has to be considered */
INT recFindSubDirs(DWORD dwFlags,
				   TCHAR szSrcPath[MAX_PATH],
				   TCHAR szDestPath[MAX_PATH],
				   BOOL *doMore)
{
	HANDLE hFile;
	WIN32_FIND_DATA findBuffer;
	TCHAR tmpDestPath[MAX_PATH], tmpSrcPath[MAX_PATH];
	INT filesReplaced = 0, i;

	/* Add a wildcard to dest end so the it will be easy to itterate
	   over all the files and directorys in the dest directory */
	_tcscat(szDestPath, _T("*"));

	/* Get the first file in the directory */
	hFile = FindFirstFile (szDestPath, &findBuffer);

	/* Remove the star added earlyer to dest path */
	for(i = (_tcslen(szDestPath) -  1); i > -1; i--)
		if(szDestPath[i] != _T('\\'))
			szDestPath[i] = _T('\0');
		else
			break;

	/* Iterate over all filed directories in the dest dir */
	do
	{
		/* Save the source path so that it will not be wrecked */
		_tcscpy(tmpSrcPath,szSrcPath);
		/* Check for reading problems */
		if(hFile == INVALID_HANDLE_VALUE)
		{
			ConOutFormatMessage (GetLastError(), tmpSrcPath);
			return filesReplaced;
		}

		/* Check if the we should enter the dir or if it is a file
		   or . or .. if so thake the next object to process */
		if(!_tcscmp (findBuffer.cFileName, _T("."))  ||
			!_tcscmp (findBuffer.cFileName, _T(".."))||
			IsExistingFile(findBuffer.cFileName))
			continue;
		/* Add the destpath and the new dir path to tempDestPath */
		_tcscpy(tmpDestPath,szDestPath);
		_tcscat (tmpDestPath, findBuffer.cFileName);
		/* Make sure that we have a directory */
		if(IsExistingDirectory(tmpDestPath))
		{
			/* Add a \ to the end or the path */
			if(szDestPath[_tcslen(tmpDestPath) -  1] != _T('\\'))
				_tcscat(tmpDestPath, _T("\\"));
			/* Call the function to replace files in the new directory */
			filesReplaced += recReplace(dwFlags, tmpSrcPath, tmpDestPath, doMore);
			/* If there were problems break e.g. read-only file */
			if(!*doMore)
				break;
			_tcscpy(tmpSrcPath,szSrcPath);
			/* Controle the next level of subdirs */
			filesReplaced += recFindSubDirs(dwFlags,tmpSrcPath,tmpDestPath, doMore);
			if(!*doMore)
				break;
		}
		/* Get the next handle */
	} while(FindNextFile (hFile, &findBuffer));

	return filesReplaced;
}

INT cmd_replace (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	LPTSTR *arg;
	INT argc, i,filesReplaced = 0, nFiles, srcIndex = -1, destIndex = -1;
	DWORD dwFlags = 0;
	TCHAR szDestPath[MAX_PATH], szSrcPath[MAX_PATH], tmpSrcPath[MAX_PATH];
	BOOL doMore = TRUE;

	/* Help wanted? */
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_REPLACE_HELP1);
		return 0;
	}

	/* Divide the argument in to an array of c-strings */
	arg = split (param, &argc, FALSE);
	nFiles = argc;

	/* Read options */
	for (i = 0; i < argc; i++)
	{
		if (arg[i][0] == _T('/'))
		{
			if (_tcslen(arg[i]) == 2)
			{
				switch (_totupper(arg[i][1]))
				{
				case _T('A'):
					dwFlags |= REPLACE_ADD;
					break;
				case _T('P'):
					dwFlags |= REPLACE_CONFIRM;
					break;
				case _T('R'):
					dwFlags |= REPLACE_READ_ONLY;
					break;
				case _T('S'):
					dwFlags |= REPLACE_SUBDIR;
					break;
				case _T('W'):
					dwFlags |= REPLACE_DISK;
					break;
				case _T('U'):
					dwFlags |= REPLACE_UPDATE;
					break;
				default:
					invalid_switch(arg[i]);
					return 0;
				}
			}
			else
			{
				invalid_switch(arg[i]);
				freep(arg);
				return 0;
			}
			nFiles--;
		}
		else
		{
			if(srcIndex == -1)
			{
				srcIndex = i;
			}
			else if(destIndex == -1)
			{
				destIndex = i;
			}
			else
			{
				invalid_switch(arg[i]);
				freep(arg);
				return 0;
			}
		}
	}

	/* See so that at least source is there */
	if (nFiles < 1)
	{
		ConOutResPaging(TRUE,STRING_REPLACE_HELP2);
		ConOutResPaging(TRUE,STRING_REPLACE_HELP3);
		freep(arg);
		return 1;
	}
	/* Check so that not both update and add switch is added and subdir */
	if((dwFlags & REPLACE_UPDATE || dwFlags & REPLACE_SUBDIR) && (dwFlags & REPLACE_ADD))
	{
		ConOutResPaging(TRUE,STRING_REPLACE_ERROR4);
		ConOutResPaging(TRUE,STRING_REPLACE_HELP7);
		freep(arg);
		return 1;
	}

	/* If we have a destination get the full path */
	if(destIndex != -1)
	{
		if(_tcslen(arg[destIndex]) == 2 && arg[destIndex][1] == ':')
			GetRootPath(arg[destIndex],szDestPath,MAX_PATH);
		else
		{
			/* Check for wildcards in destination directory */
			if (_tcschr (arg[destIndex], _T('*')) != NULL ||
				_tcschr (arg[destIndex], _T('?')) != NULL)
			{
				LoadString(CMD_ModuleHandle, STRING_REPLACE_ERROR2, szMsg, RC_STRING_MAX_SIZE);
				ConOutPrintf(szMsg,arg[destIndex]);
				ConOutResPaging(TRUE,STRING_REPLACE_HELP3);
				freep(arg);
				return 1;
			}
			getPath(szDestPath, arg[destIndex]);
			/* Make sure that destination exists */
			if(!IsExistingDirectory(szDestPath))
			{
				LoadString(CMD_ModuleHandle, STRING_REPLACE_ERROR2, szMsg, RC_STRING_MAX_SIZE);
				ConOutPrintf(szMsg, szDestPath);
				ConOutResPaging(TRUE,STRING_REPLACE_HELP3);
				freep(arg);
				return 1;
			}
		}
	}
	else
	{
		/* Dest is current dir */
		GetCurrentDirectory(MAX_PATH,szDestPath);
	}

	/* Get the full source path */
	if(!(_tcslen(arg[srcIndex]) == 2 && arg[srcIndex][1] == ':'))
		getPath(szSrcPath, arg[srcIndex]);
	else
 		_tcscpy(szSrcPath,arg[srcIndex]);

	/* Source does not have wildcards */
	if (_tcschr (arg[srcIndex], _T('*')) == NULL &&
		_tcschr (arg[srcIndex], _T('?')) == NULL)
	{
		/* Check so that source is not a directory, because that is not allowed */
		if(IsExistingDirectory(szSrcPath))
		{
			LoadString(CMD_ModuleHandle, STRING_REPLACE_ERROR6, szMsg, RC_STRING_MAX_SIZE);
			ConOutPrintf(szMsg, szSrcPath);
			ConOutResPaging(TRUE,STRING_REPLACE_HELP3);
			freep(arg);
			return 1;
		}
		/* Check if the file exists */
		if(!IsExistingFile(szSrcPath))
		{
			ConOutResPaging(TRUE,STRING_REPLACE_HELP3);
			freep(arg);
			return 1;
		}
	}
	/* /w switch is set so wait for any key to be pressed */
	if(dwFlags & REPLACE_DISK)
	{
		msg_pause();
		cgetchar();
	}

	/* Add an extra \ to the destination path if needed */
	if(szDestPath[_tcslen(szDestPath) -  1] != _T('\\'))
		_tcscat(szDestPath, _T("\\"));

	/* Save source path */
	_tcscpy(tmpSrcPath,szSrcPath);
	/* Replace in dest dir */
	filesReplaced += recReplace(dwFlags, tmpSrcPath, szDestPath, &doMore);
	/* If subdir switch is set replace in the subdirs to */
	if(dwFlags & REPLACE_SUBDIR && doMore)
	{
		filesReplaced += recFindSubDirs(dwFlags, szSrcPath,  szDestPath, &doMore);
	}

	/* If source == dest write no more */
	if(filesReplaced != -1)
	{
		/* No files replaced */
		if(filesReplaced==0)
		{
			/* Add switch dependent output */
			if(dwFlags & REPLACE_ADD)
				ConOutResPaging(TRUE,STRING_REPLACE_HELP7);
			else
				ConOutResPaging(TRUE,STRING_REPLACE_HELP3);
		}
		/* Some files replaced */
		else
		{
			/* Add switch dependent output */
			if(dwFlags & REPLACE_ADD)
				LoadString(CMD_ModuleHandle, STRING_REPLACE_HELP8, szMsg, RC_STRING_MAX_SIZE);
			else
				LoadString(CMD_ModuleHandle, STRING_REPLACE_HELP4, szMsg, RC_STRING_MAX_SIZE);
			ConOutPrintf(szMsg, filesReplaced);
		}
	}
	/* Return memory */
	freep(arg);
	return 1;
}
#endif /* INCLUDE_CMD_REPLACE */
