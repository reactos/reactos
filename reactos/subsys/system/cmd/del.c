/*
*  DEL.C - del internal command.
*
*
*  History:
*
*    06/29/98 (Rob Lake rlake@cs.mun.ca)
*        rewrote del to support wildcards
*        added my name to the contributors
*
*    07/13/98 (Rob Lake)
*        fixed bug that caused del not to delete file with out
*        attribute. moved set, del, ren, and ver to there own files
*
*    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
*        added config.h include
*
*    09-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
*        Fixed command line parsing bugs.
*
*    21-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
*        Started major rewrite using a new structure.
*
*    03-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
*        First working version.
*
*    30-Mar-1999 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
*        Added quiet ("/Q"), wipe ("/W") and zap ("/Z") option.
*
*    06-Nov-1999 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
*        Little fix to keep DEL quiet inside batch files.
*
*    28-Jan-2004 (Michael Fritscher <michael@fritscher.net>)
*        Added prompt ("/P"), yes ("/Y") and wipe("/W") option.
*
*    22-Jun-2005 (Brandon Turner <turnerb7@msu.edu>)
*        Added exclusive deletion "del * -abc.txt -text*.txt"
*
*    22-Jun-2005 (Brandon Turner <turnerb7@msu.edu>)
*        Implemented /A   example "del /A:H /A:-R *.exe -ping.exe"
*/

#include <precomp.h>
#include "resource.h"

#ifdef INCLUDE_CMD_DEL


enum
{
	DEL_ATTRIBUTES = 0x001,   /* /A */
	DEL_NOTHING    = 0x004,   /* /N */
	DEL_PROMPT     = 0x008,   /* /P */
	DEL_QUIET      = 0x010,   /* /Q */
	DEL_SUBDIR     = 0x020,   /* /S */
	DEL_TOTAL      = 0x040,   /* /T */
	DEL_WIPE       = 0x080,   /* /W */
	DEL_EMPTYDIR   = 0x100,   /* /X : not implemented */
	DEL_YES        = 0x200,   /* /Y */
	DEL_FORCE      = 0x800    /* /F */
};

enum
{
	ATTR_ARCHIVE     = 0x001,   /* /A:A */
	ATTR_HIDDEN      = 0x002,   /* /A:H */
	ATTR_SYSTEM      = 0x004,   /* /A:S */
	ATTR_READ_ONLY   = 0x008,   /* /A:R */
	ATTR_N_ARCHIVE   = 0x010,   /* /A:-A */
	ATTR_N_HIDDEN    = 0x020,   /* /A:-H */
	ATTR_N_SYSTEM    = 0x040,   /* /A:-S */
	ATTR_N_READ_ONLY = 0x080    /* /A:-R */
};



static BOOL
RemoveFile (LPTSTR lpFileName, DWORD dwFlags)
{
	/*This function is called by CommandDelete and
	does the actual process of deleting the single 
	file*/

	if (dwFlags & DEL_WIPE)
	{

		HANDLE file;
		DWORD temp;
		LONG BufferSize = 65536;
		BYTE buffer[BufferSize];
		LONG i;
		HANDLE fh;
		WIN32_FIND_DATA f;
		LONGLONG FileSize;
		TCHAR szMsg[RC_STRING_MAX_SIZE];

		LoadString( CMD_ModuleHandle, STRING_DELETE_WIPE, szMsg, RC_STRING_MAX_SIZE);

		fh = FindFirstFile(lpFileName, &f);
		FileSize = ((LONGLONG)f.nFileSizeHigh * ((LONGLONG)MAXDWORD+1)) + (LONGLONG)f.nFileSizeLow;

		for(i = 0; i < BufferSize; i++)
		{
			buffer[i]=rand() % 256;
		}
		file = CreateFile (lpFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,  FILE_FLAG_WRITE_THROUGH, NULL);
		for(i = 0; i < (FileSize - BufferSize); i += BufferSize)
		{
			WriteFile (file, buffer, BufferSize, &temp, NULL);
			ConOutPrintf (_T("%I64d%% %s\r"),(i * (LONGLONG)100)/FileSize,szMsg);
		}
		WriteFile (file, buffer, FileSize - i, &temp, NULL);
		ConOutPrintf (_T("100%% %s\n"),szMsg);
		CloseHandle (file);
	}
	/*check to see if it is read only and if this is done based on /A
	if it is done by file name, access is denied. However, if it is done 
	using the /A switch you must un-read only the file and allow it to be
	deleted*/
	if((dwFlags & DEL_ATTRIBUTES) || (dwFlags & DEL_FORCE))
	{
		HANDLE hFile;
		WIN32_FIND_DATA f2;
		hFile = FindFirstFile(lpFileName, &f2);
		if(f2.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		{
			/*setting file to normal, not saving old attrs first
			because the file is going to be deleted anyways
			so the only thing that matters is that it isnt
			read only.*/
			SetFileAttributes(lpFileName,FILE_ATTRIBUTE_NORMAL);
		}

	}
	return DeleteFile (lpFileName);
}

BOOL _SearchPathex(TCHAR *lpPath,TCHAR *lpFileName,TCHAR *lpBuffer, DWORD dwFlags)
{
	TCHAR szFullPath[MAX_PATH];
	WIN32_FIND_DATA f;
	HANDLE hFile;
	TCHAR CMDPath[MAX_PATH];


	/* Make the Full Path Name */
	_tcscpy(szFullPath,lpPath);
	_tcscat(szFullPath,lpFileName);

	/* Open up the handle to the file(s) */
	hFile = FindFirstFile(szFullPath, &f);
	do
	{
		/* Build the full name again, this time using the handle
		   as it might have had a * in the name before */
		_tcscpy(szFullPath,lpPath);
		_tcscat(szFullPath,f.cFileName);

		/* We cant delete . or .. */
		if (!_tcscmp (f.cFileName, _T(".")) ||
			!_tcscmp (f.cFileName, _T("..")))
			continue;

		/* We cant delete ourselves */
		GetModuleFileName(NULL, CMDPath, MAX_PATH);
		if(!_tcscmp (CMDPath,szFullPath))
			continue;

		/* If there is a read only / system file and they are forcing, then we can delete it
		   But we must first set it to normal.  Otherwise we cant delete it */
		if(((dwFlags & DEL_FORCE) || (dwFlags & DEL_ATTRIBUTES)) && ((f.dwFileAttributes & FILE_ATTRIBUTE_READONLY) || (f.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)))
		{

			SetFileAttributes(szFullPath,FILE_ATTRIBUTE_NORMAL);

		}
		else if(!((dwFlags & DEL_FORCE) || (dwFlags & DEL_ATTRIBUTES)) && ((f.dwFileAttributes & FILE_ATTRIBUTE_READONLY) || (f.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)))
			continue;

		/* If we find a file, we can return and continue to delete it */
		if(IsExistingFile(szFullPath))
		{
			_tcscpy(lpBuffer,szFullPath);
			CloseHandle(hFile);
			return TRUE;
		}
	}while(FindNextFile (hFile, &f));

	CloseHandle(hFile);

	/* None in this folder, so lets drop down to another folder */
	_tcscat(lpPath,_T("*"));
	hFile = FindFirstFile(lpPath, &f);
	lpPath[_tcslen(lpPath) - 1] = _T('\0');

	do
	{
		/* dont count this folder or the folder above */
		if (!_tcscmp (f.cFileName, _T(".")) ||
			!_tcscmp (f.cFileName, _T("..")))
			continue;

		/* Assemble full path to file */
		_tcscpy(szFullPath,lpPath);
		_tcscat(szFullPath,f.cFileName);
		if(IsExistingDirectory(szFullPath))
		{
			_tcscat(szFullPath,_T("\\"));
			/* If it is a directory, move into(Recurvsily) */
			if(_SearchPathex(szFullPath,lpFileName,lpBuffer,dwFlags))
			{
				return TRUE;
			}

		}
	}while(FindNextFile (hFile, &f));
	return FALSE;
}


INT CommandDelete (LPTSTR cmd, LPTSTR param)
{
	/*cmd is the command that was given, in this case it will always be "del" or "delete"
	param is whatever is given after the command*/

	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR szFullPath[MAX_PATH];
	LPTSTR pFilePart;
	LPTSTR *arg = NULL;
	TCHAR exfileName[MAX_PATH];
	TCHAR * szFileName;
	INT args;
	INT i;
	INT ii;
	INT res;
	INT   nEvalArgs = 0; /* nunber of evaluated arguments */
	DWORD dwFlags = 0;
	DWORD dwAttrFlags = 0;
	DWORD dwFiles = 0;
	HANDLE hFile;
	HANDLE hFileExcl;
	WIN32_FIND_DATA f;
	WIN32_FIND_DATA f2;
	LONG ch;
	BOOL bSubFileFound = FALSE;
	BOOL bExclusion;
	TCHAR szOrginalArg[MAX_PATH];
	TCHAR AltArg[MAX_PATH];

	/*checks the first two chars of param to see if it is /?
	this however allows the following command to not show help
	"del frog.txt /?" */

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_DEL_HELP1);
		return 0;
	}

	arg = split (param, &args, FALSE);

	if (args == 0)
	{
		/* only command given */
		error_req_param_missing ();
		freep (arg);
		return 1;
	}
	/* check for options anywhere in command line */
	for (i = 0; i < args; i++)
	{
		if (*arg[i] == _T('/'))
		{
			/*found a command, but check to make sure it has something after it*/
			if (_tcslen (arg[i]) >= 2)
			{
				ch = _totupper (arg[i][1]);
				if (ch == _T('N'))
				{
					dwFlags |= DEL_NOTHING;
				}
				else if (ch == _T('P'))
				{
					dwFlags |= DEL_PROMPT;
				}
				else if (ch == _T('Q'))
				{
					dwFlags |= DEL_QUIET;
				}
				else if (ch == _T('F'))
				{
					dwFlags |= DEL_FORCE;
				}
				else if (ch == _T('S'))
				{
					dwFlags |= DEL_SUBDIR;
				}
				else if (ch == _T('T'))
				{
					dwFlags |= DEL_TOTAL;
				}
				else if (ch == _T('W'))
				{
					dwFlags |= DEL_WIPE;
				}
				else if (ch == _T('Y'))
				{
					dwFlags |= DEL_YES;
				}
				else if (ch == _T('A'))
				{

					dwFlags |= DEL_ATTRIBUTES;
					/*the proper syntax for /A has a min of 4 chars
					i.e. /A:R or /A:-H */
					if (_tcslen (arg[i]) < 4)
					{
						error_invalid_parameter_format(arg[i]);
						return 0;
					}
					ch = _totupper (arg[i][3]);
					if (_tcslen (arg[i]) == 4)
					{						
						if(ch == _T('A'))
						{
							dwAttrFlags |= ATTR_ARCHIVE;
						}
						if(ch == _T('H'))
						{
							dwAttrFlags |= ATTR_HIDDEN;
						}
						if(ch == _T('S'))
						{
							dwAttrFlags |= ATTR_SYSTEM;
						}
						if(ch == _T('R'))
						{
							dwAttrFlags |= ATTR_READ_ONLY;
						}
					}
					if (_tcslen (arg[i]) == 5)
					{
						if(ch == _T('-'))
						{
							ch = _totupper (arg[i][4]);
							if(ch == _T('A'))
							{
								dwAttrFlags |= ATTR_N_ARCHIVE;
							}
							if(ch == _T('H'))
							{
								dwAttrFlags |= ATTR_N_HIDDEN;
							}
							if(ch == _T('S'))
							{
								dwAttrFlags |= ATTR_N_SYSTEM;
							}
							if(ch == _T('R'))
							{
								dwAttrFlags |= ATTR_N_READ_ONLY;
							}
						}
					}
				}
			}

			nEvalArgs++;
		}
	}

	/* there are only options on the command line --> error!!!
	there is the same number of args as there is flags, so none of the args were filenames*/
	if (args == nEvalArgs)
	{
		error_req_param_missing ();
		freep (arg);
		return 1;
	}

	/* keep quiet within batch files */
	if (bc != NULL)
		dwFlags |= DEL_QUIET;

	/* check for filenames anywhere in command line */
	for (i = 0; i < args; i++)
	{

      /*this checks to see if it isnt a flag, if it isnt, we assume it is a file name*/
		if((*arg[i] == _T('/')) || (*arg[i] == _T('-')))
			continue;
		bSubFileFound = FALSE;

		/* We want to make 2 copies of the argument,
		   1. copy will stay the same as the argument 
			2. will be edited and striped down to the bad folder */
		if(_tcslen(arg[i]) == 2 && arg[i][1] == _T(':'))
		{
			/* Check for C: D: ... */
			GetRootPath(arg[i],szOrginalArg,MAX_PATH);
			GetRootPath(arg[i],AltArg,MAX_PATH);
		}
		else
		{
			_tcscpy(szOrginalArg,arg[i]);
			_tcscpy(AltArg,arg[i]);
		}

		do
		{
			

			if(dwFlags & DEL_SUBDIR)
			{
				
				TCHAR FoundPath[MAX_PATH];				
				TCHAR * PathFileName;
				TCHAR PathBaseFolder[MAX_PATH];

				/* Get just the file name */
				PathFileName = _tcsrchr(szOrginalArg,_T('\\'));
				if(PathFileName != NULL)
					PathFileName++;
				else
					PathFileName = szOrginalArg;

				/* Get the full path to the file */
				GetFullPathName (szOrginalArg,MAX_PATH,PathBaseFolder,NULL);

				/* strip the filename off of it */
				for(ii = (_tcslen(PathBaseFolder) -  1); ii > -1; ii--)
				{
					if(PathBaseFolder[ii] != _T('\\'))
						PathBaseFolder[ii] = _T('\0');
					else
					{
						break;
					}
					
				}
			
				/* Start looking for it, once it cant find anymore files on that name
				   it returns false and that is how we know we have found all files in sub directories. */
				if(_SearchPathex(PathBaseFolder,PathFileName,FoundPath,dwFlags))
				{
					bSubFileFound = TRUE;
					_tcscpy(AltArg,FoundPath);
				}
				else
				{
					bSubFileFound = FALSE;
					continue;
				}
				
			}

		
		if(_tcschr (AltArg, _T('*')) == NULL && 
			IsExistingDirectory (AltArg))
		{
		/* If it doesnt have a \ at the end already then on needs to be added */
		if(AltArg[_tcslen(AltArg) -  1] != _T('\\'))
			_tcscat (AltArg, _T("\\"));
		/* Add a wildcard after the \ */
		_tcscat (AltArg, _T("*"));
		}

		if(!_tcscmp (AltArg, _T("*")) ||
			!_tcscmp (AltArg, _T("*.*"))||
			(AltArg[_tcslen(AltArg) -  2] == _T('\\') && AltArg[_tcslen(AltArg) -  1] == _T('*')))
		{
			/*well, the user wants to delete everything but if they didnt yes DEL_YES, DEL_QUIET, or DEL_PROMPT
			then we are going to want to make sure that in fact they want to do that.  */

			if (!((dwFlags & DEL_YES) || (dwFlags & DEL_QUIET) || (dwFlags & DEL_PROMPT)))
			{
				LoadString( CMD_ModuleHandle, STRING_DEL_HELP2, szMsg, RC_STRING_MAX_SIZE);

				res = FilePromptYNA (szMsg);
				if ((res == PROMPT_NO) || (res == PROMPT_BREAK))
					break;
				if(res == PROMPT_ALL)
					dwFlags |= DEL_YES;
			}
		}



#ifdef _DEBUG
		ConErrPrintf (_T("File: %s\n"), arg[i]);
#endif

#ifdef _DEBUG
		ConErrPrintf(_T("Wildcards!\n\n"));
#endif
		
		GetFullPathName (AltArg,
			MAX_PATH,
			szFullPath,
			&pFilePart);

#ifdef _DEBUG
		ConErrPrintf(_T("Full path: %s\n"), szFullPath);
		ConErrPrintf(_T("File part: %s\n"), pFilePart);
#endif

		hFile = FindFirstFile (szFullPath, &f);
		
		do
		{


			if (hFile == INVALID_HANDLE_VALUE)
			{
				error_file_not_found ();
				freep (arg);
				return 0;
			}

			/*bExclusion is the check varible to see if it has a match
			and it needs to be set to false before each loop, as it hasnt been matched yet*/						
			bExclusion = 0;
			
			/*loop through each of the arguments*/
			for (ii = 0; ii < args; ii++)
			{
				/*check to see if it is a exclusion tag but not a ':' (used in ATTR)*/
				if(_tcschr (arg[ii], _T('-')) && _tcschr (arg[ii], _T(':')) == NULL)
				{
					/*remove the - from the front to get the real name*/
					_tcscpy (exfileName , arg[ii]);								
					szFileName = strtok (exfileName,"-");								
					GetFullPathName (szFileName,
						MAX_PATH,
						szFullPath,
						&pFilePart);
					hFileExcl = FindFirstFile (szFullPath, &f2);								
					do
					{
						/*check to see if the filenames match*/
						if(!_tcscmp (f.cFileName, f2.cFileName))
							bExclusion = 1;	
					}
					while (FindNextFile (hFileExcl, &f2));
				}
			}
			
			/*if it is going to be excluded by - no need to check attrs*/
			if(dwFlags & DEL_ATTRIBUTES && bExclusion == 0)
			{

				/*save if file attr check if user doesnt care about that attr anyways*/
				if(dwAttrFlags & ATTR_ARCHIVE && !(f.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE))
					bExclusion = 1;
				if(dwAttrFlags & ATTR_HIDDEN && !(f.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
					bExclusion = 1;
				if(dwAttrFlags & ATTR_SYSTEM && !(f.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
					bExclusion = 1;
				if(dwAttrFlags & ATTR_READ_ONLY && !(f.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
					bExclusion = 1;
				if(dwAttrFlags & ATTR_N_ARCHIVE && (f.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE))
					bExclusion = 1;
				if(dwAttrFlags & ATTR_N_HIDDEN && (f.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
					bExclusion = 1;
				if(dwAttrFlags & ATTR_N_SYSTEM && (f.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
					bExclusion = 1;
				if(dwAttrFlags & ATTR_N_READ_ONLY && (f.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
					bExclusion = 1;
			}

			if(bExclusion)
				continue;

			/* ignore ".", ".." and directories */
			if (!_tcscmp (f.cFileName, _T(".")) ||
				!_tcscmp (f.cFileName, _T("..")) ||
				f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			_tcscpy (pFilePart, f.cFileName);

#ifdef _DEBUG
			ConErrPrintf(_T("Full filename: %s\n"), szFullPath);
#endif

			/* ask for deleting */
			if (dwFlags & DEL_PROMPT)
			{
				LoadString(CMD_ModuleHandle, STRING_DEL_ERROR5, szMsg, RC_STRING_MAX_SIZE);
				ConErrPrintf(szMsg, szFullPath);

				LoadString(CMD_ModuleHandle, STRING_DEL_ERROR6, szMsg, RC_STRING_MAX_SIZE);
				res = FilePromptYN (szMsg);

				if ((res == PROMPT_NO) || (res == PROMPT_BREAK))
				{
					continue;  //FIXME: Errorcode?
				}
			}

			/*user cant ask it to be quiet and tell you what it did*/
			if (!(dwFlags & DEL_QUIET) && !(dwFlags & DEL_TOTAL))
			{
				LoadString(CMD_ModuleHandle, STRING_DEL_ERROR7, szMsg, RC_STRING_MAX_SIZE);
				ConErrPrintf(szMsg, szFullPath);
			}

			/* delete the file */
			if(dwFlags & DEL_NOTHING)
				continue;


			if(RemoveFile (szFullPath, dwFlags))
				dwFiles++;
			else
			{
				bSubFileFound = FALSE;
				ErrorMessage (GetLastError(), _T(""));
				break;
			}
		}
		while (FindNextFile (hFile, &f));
		}
		while(bSubFileFound);
	}
	FindClose (hFile);
	
	freep (arg);

	/*Based on MS cmd, we only tell what files are being deleted when /S is used */
	if (dwFlags & DEL_TOTAL)
	{
		if (dwFiles < 2)
		{
			LoadString(CMD_ModuleHandle, STRING_DEL_HELP3, szMsg, RC_STRING_MAX_SIZE);
		}
		else
		{
			LoadString(CMD_ModuleHandle, STRING_DEL_HELP4, szMsg, RC_STRING_MAX_SIZE);
		}

		ConOutPrintf(szMsg, dwFiles);
	}

	return 0;
}


#endif
