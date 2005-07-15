/*
 *  COPY.C -- copy internal command.
 *
 *
 *  History:
 *
 *    01-Aug-98 (Rob Lake z63rrl@morgan.ucs.mun.ca)
 *        started
 *
 *    13-Aug-1998 (John P. Price)
 *        fixed memory leak problem in copy function.
 *        fixed copy function so it would work with wildcards in the source
 *
 *    13-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added COPY command to CMD.
 *
 *    26-Jan-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Replaced CRT io functions by Win32 io functions.
 *
 *    27-Oct-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Disabled prompting when used in batch mode.
 *
 *    03-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *
 *    13-Jul-2005 (Brandon Turner) <turnerb7@msu.edu>)
 *        Rewrite to clean up copy and support wildcard.
 */
 
#include <precomp.h>
#include "resource.h"
 
#ifdef INCLUDE_CMD_COPY
 
enum
{
	COPY_ASCII   = 0x001,   /* /A  */
	COPY_DECRYPT     = 0x004,   /* /D  : Not Impleneted */
	COPY_VERIFY      = 0x008,   /* /V  : Dummy, Never will be Impleneted */
	COPY_SHORTNAME   = 0x010,   /* /N  : Not Impleneted */
	COPY_NO_PROMPT   = 0x020,   /* /Y  */
	COPY_PROMPT      = 0x040,   /* /-Y */
	COPY_RESTART     = 0x080,   /* /Z  : Not Impleneted */
	COPY_BINARY     = 0x100,    /* /B  */
};
 
#define BUFF_SIZE 16384         /* 16k = max buffer size */
 
 
int copy (LPTSTR source, LPTSTR dest, int append, DWORD lpdwFlags)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	FILETIME srctime;
	HANDLE hFileSrc;
	HANDLE hFileDest;
	LPBYTE buffer;
	DWORD  dwAttrib;
	DWORD  dwRead;
	DWORD  dwWritten;
	DWORD  i;
	BOOL   bEof = FALSE;
 
#ifdef _DEBUG
	DebugPrintf (_T("checking mode\n"));
#endif
 
	dwAttrib = GetFileAttributes (source);
 
	hFileSrc = CreateFile (source, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, 0, NULL);
	if (hFileSrc == INVALID_HANDLE_VALUE)
	{
		LoadString(CMD_ModuleHandle, STRING_COPY_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		ConOutPrintf(szMsg, source);
    nErrorLevel = 1;
		return 0;
	}
 
#ifdef _DEBUG
	DebugPrintf (_T("getting time\n"));
#endif
 
	GetFileTime (hFileSrc, &srctime, NULL, NULL);
 
#ifdef _DEBUG
	DebugPrintf (_T("copy: flags has %s\n"),
		*lpdwFlags & ASCII ? "ASCII" : "BINARY");
#endif
 
	if (!IsExistingFile (dest))
	{
#ifdef _DEBUG
		DebugPrintf (_T("opening/creating\n"));
#endif
		hFileDest =
			CreateFile (dest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	}
	else if (!append)
	{
		if (!_tcscmp (dest, source))
		{
			LoadString(CMD_ModuleHandle, STRING_COPY_ERROR2, szMsg, RC_STRING_MAX_SIZE);
			ConOutPrintf(szMsg, source);
 
			CloseHandle (hFileSrc);
      nErrorLevel = 1;
			return 0;
		}
 
#ifdef _DEBUG
		DebugPrintf (_T("SetFileAttributes (%s, FILE_ATTRIBUTE_NORMAL);\n"), dest);
#endif
		SetFileAttributes (dest, FILE_ATTRIBUTE_NORMAL);
 
#ifdef _DEBUG
		DebugPrintf (_T("DeleteFile (%s);\n"), dest);
#endif
		DeleteFile (dest);
 
		hFileDest =	CreateFile (dest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	}
	else
	{
		LONG lFilePosHigh = 0;
 
		if (!_tcscmp (dest, source))
		{
			CloseHandle (hFileSrc);
			return 0;
		}
 
#ifdef _DEBUG
		DebugPrintf (_T("opening/appending\n"));
#endif
		SetFileAttributes (dest, FILE_ATTRIBUTE_NORMAL);
 
		hFileDest =
			CreateFile (dest, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
 
		/* Move to end of file to start writing */
		SetFilePointer (hFileDest, 0, &lFilePosHigh,FILE_END);
	}
 
 
		if (hFileDest == INVALID_HANDLE_VALUE)
	{
		CloseHandle (hFileSrc);
		error_path_not_found ();
    nErrorLevel = 1;
		return 0;
	}
	buffer = (LPBYTE)malloc (BUFF_SIZE);
	if (buffer == NULL)
	{
		CloseHandle (hFileDest);
		CloseHandle (hFileSrc);
		error_out_of_memory ();
    nErrorLevel = 1;
		return 0;
	}
 
	do
	{
		ReadFile (hFileSrc, buffer, BUFF_SIZE, &dwRead, NULL);
		if (lpdwFlags & COPY_ASCII)
		{
			for (i = 0; i < dwRead; i++)
			{
				if (((LPTSTR)buffer)[i] == 0x1A)
				{
					bEof = TRUE;
					break;
				}
			}
			dwRead = i;
		}
 
		if (dwRead == 0)
			break;
 
		WriteFile (hFileDest, buffer, dwRead, &dwWritten, NULL);
		if (dwWritten != dwRead)
		{
			ConOutResPuts(STRING_COPY_ERROR3);
 
			free (buffer);
			CloseHandle (hFileDest);
			CloseHandle (hFileSrc);
      nErrorLevel = 1;
			return 0;
		}
	}
	while (dwRead && !bEof);
 
#ifdef _DEBUG
	DebugPrintf (_T("setting time\n"));
#endif
	SetFileTime (hFileDest, &srctime, NULL, NULL);
 
	if (lpdwFlags & COPY_ASCII)
	{
		((LPTSTR)buffer)[0] = 0x1A;
		((LPTSTR)buffer)[1] = _T('\0');
#ifdef _DEBUG
		DebugPrintf (_T("appending ^Z\n"));
#endif
		WriteFile (hFileDest, buffer, sizeof(TCHAR), &dwWritten, NULL);
	}
 
	free (buffer);
	CloseHandle (hFileDest);
	CloseHandle (hFileSrc);
 
#ifdef _DEBUG
	DebugPrintf (_T("setting mode\n"));
#endif
	SetFileAttributes (dest, dwAttrib);
 
	return 1;
}
 
 
static INT Overwrite (LPTSTR fn)
{
	/*ask the user if they want to override*/
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	INT res;
	LoadString(CMD_ModuleHandle, STRING_COPY_HELP1, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg,fn);
	res = FilePromptYNA ("");
	return res;
}
 
 
INT cmd_copy (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	LPTSTR *arg;
	INT argc, i, nFiles, nOverwrite = 0, nSrc = -1, nDes = -1;
	/* this is the path up to the folder of the src and dest ie C:\windows\ */
	TCHAR szDestPath[MAX_PATH];
	TCHAR szSrcPath[MAX_PATH];
	DWORD dwFlags = 0;
	/* If this is the type of copy where we are adding files */
	BOOL bAppend = FALSE;
	WIN32_FIND_DATA findBuffer;
	HANDLE hFile;
	/* Used when something like "copy c*.exe d*.exe" during the process of
	   figuring out the new name */
	TCHAR tmpName[MAX_PATH] = _T("");
	/* Pointer to keep track of how far through the append input(file1+file2+file3) we are */
	TCHAR  * appendPointer = _T("\0");
	/* The full path to src and dest.  This has drive letter, folders, and filename */
	TCHAR tmpDestPath[MAX_PATH];
	TCHAR tmpSrcPath[MAX_PATH];
	/* A bool on weather or not the destination name will be taking from the input */
	BOOL bSrcName = FALSE;
	/* Seems like a waste but it is a pointer used to copy from input to PreserveName */
	TCHAR * UseThisName;
	/* Stores the name( i.e. blah.txt or blah*.txt) which later we might need */
	TCHAR PreserveName[MAX_PATH];
  /* for CMDCOPY env */
  TCHAR *evar;
  int size;
 
	
  /*Show help/usage info*/
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE, STRING_COPY_HELP2);
		return 0;
	}
 
  nErrorLevel = 0;

  /* Get the envor value if it exists */
  evar = malloc(512);
  size = GetEnvironmentVariable (_T("COPYCMD"), evar, 512);
  if (size > 512)
  {
    evar = realloc(evar,size * sizeof(TCHAR) );
    if (evar!=NULL)
    {             
      size = GetEnvironmentVariable (_T("COPYCMD"), evar, size);
      }
  }
  /* check see if we did get any env variable */
  if (size !=0)
  {
    int t=0;
    /* scan and set the flags */
    for (t=0;t<size;t++)
    {
      if (_tcsncicmp(_T("/A"),&evar[t],2)==0) 
      {
        evar[t]=_T(' ');
        evar[t+1]=_T(' ');
      }

      else if (_tcsncicmp(_T("/B"),&evar[t],2)==0) 
      {
        dwFlags |= COPY_BINARY;
        evar[t]=_T(' ');
        evar[t+1]=_T(' ');
      }
      else if (_tcsncicmp(_T("/D"),&evar[t],2)==0) 
      {
        dwFlags |= COPY_DECRYPT;
        evar[t]=_T(' ');
        evar[t+1]=_T(' ');
      }

			else if (_tcsncicmp(_T("/V"),&evar[t],2)==0) 
      {
        dwFlags |= COPY_VERIFY;
        evar[t]=_T(' ');
        evar[t+1]=_T(' ');
      }

			else if (_tcsncicmp(_T("/N"),&evar[t],2)==0) 
      {
        dwFlags |= COPY_SHORTNAME;
        evar[t]=_T(' ');
        evar[t+1]=_T(' ');
      }
 
      else if (_tcsncicmp(_T("/Y"),&evar[t],2)==0) 
      {
        dwFlags |= COPY_NO_PROMPT;
        evar[t]=_T(' ');
        evar[t+1]=_T(' ');
      }

      else if (_tcsncicmp(_T("/-Y"),&evar[t],3)==0) 
      {
        dwFlags |= COPY_PROMPT;
        evar[t]=_T(' ');
        evar[t+1]=_T(' ');
        evar[t+2]=_T(' ');
      }

      else if (_tcsncicmp(_T("/Z"),&evar[t],2)==0) 
      {
        dwFlags |= COPY_PROMPT;
        evar[t]=_T(' ');
        evar[t+1]=_T(' ');
      }
    }
  }
  free(evar);


  /*Split the user input into array*/
	arg = split (param, &argc, FALSE);
	nFiles = argc;
 
 
	/*Read switches and count files*/
	for (i = 0; i < argc; i++)
	{
		if (*arg[i] == _T('/'))
		{
			if (_tcslen(arg[i]) >= 2)
			{
				switch (_totupper(arg[i][1]))
				{
 
				case _T('A'):
					dwFlags |= COPY_ASCII;
					break;
 
				case _T('B'):
					dwFlags |= COPY_BINARY;
					break;
 
				case _T('D'):
					dwFlags |= COPY_DECRYPT;
					break;
 
				case _T('V'):
					dwFlags |= COPY_VERIFY;
					break;
 
				case _T('N'):
					dwFlags |= COPY_SHORTNAME;
					break;
 
				case _T('Y'):
					dwFlags |= COPY_NO_PROMPT;
					dwFlags &= ~COPY_PROMPT;
					break;
 
				case _T('-'):
					if(_tcslen(arg[i]) >= 3)
						if(_totupper(arg[i][2]) == _T('Y'))
						{
							dwFlags &= ~COPY_NO_PROMPT;
							dwFlags |= COPY_PROMPT;
						}
 
						break;
 
				case _T('Z'):
					dwFlags |= COPY_RESTART;
					break;
 
				default:
					/* invaild switch */
					error_invalid_switch(_totupper(arg[i][1]));
					return 1;
					break;
				}
			}
			/*If it was a switch, subtract from total arguments*/
			nFiles--;
		}
		else
		{
			/*if it isnt a switch then it is the source or destination*/
			if(nSrc == -1)
				nSrc = i;
			else if(nDes == -1)
				nDes = i;
 
		}
	}
 
	if(nFiles < 1)
	{
		/* There is not enough files, there has to be at least 1 */
		error_req_param_missing();
		return 1;
	}
 
	if(nFiles > 2)
	{
		/* there is too many file names in command */
		error_too_many_parameters("");
    nErrorLevel = 1;
		return 1;
	}
 
	if((nDes != -1) &&
		((_tcschr (arg[nSrc], _T('+')) != NULL) ||
		(_tcschr (arg[nSrc], _T('*')) != NULL && _tcschr (arg[nDes], _T('*')) == NULL) ||
		(IsExistingDirectory (arg[nSrc]) && (_tcschr (arg[nDes], _T('*')) == NULL && !IsExistingDirectory (arg[nDes])))
		))
	{
		/* There is a + in the source filename, this means
		that there is more then one file being put into
		one file. */
		bAppend = TRUE;
		if(_tcschr (arg[nSrc], _T('+')) != NULL)
		   appendPointer = arg[nSrc];
	}
 
	/* Reusing the number of files variable */
	nFiles = 0;
 
	do
	{
 
	/* Set up the string that is the path to the destination */
	if(nDes != -1)
	{
		/* Check to make sure if they entered c:, if they do then GFPN 
		return current directory even though msdn says it will return c:\ */
		if(_tcslen(arg[nDes]) == 2)
		{
			if(arg[nDes][1] == _T(':'))
			{
				_tcscpy (szDestPath, arg[nDes]);
				_tcscat (szDestPath, _T("\\"));
			}
		}
		else
		/* If the user entered two file names then form the full string path*/
		GetFullPathName (arg[nDes], MAX_PATH, szDestPath, NULL);
 
	}
	else
	{
		/* If no destination was entered then just use 
		the current directory as the destination */
		GetCurrentDirectory (MAX_PATH, szDestPath);
	}
 
 
	/* Get the full string of the path to the source file*/
	if(_tcschr (arg[nSrc], _T('+')) != NULL)
	{
 
		_tcscpy(tmpName,_T("\0"));
		/* Loop through the source file name and copy all
		the chars one at a time until it gets too + */
		while(TRUE)
		{
 
			if(!_tcsncmp (appendPointer,_T("+"),1) || !_tcsncmp (appendPointer,_T("\0"),1))
			{
				/* Now that the pointer is on the + we 
				   need to go to the start of the next filename */
				if(!_tcsncmp (appendPointer,_T("+"),1))
				   appendPointer++;
				break;
			}
			_tcsncat(tmpName,appendPointer,1);
			appendPointer++;
		}
		/* Finish the string off with a null char */
		_tcsncat(tmpName,_T("\0"),1);
		/* Check to make sure if they entered c:, if they do then GFPN 
		return current directory even though msdn says it will return c:\ */
		if(_tcslen(tmpName) == 2)
		{
			if(tmpName[1] == _T(':'))
			{
				_tcscpy (szSrcPath, tmpName);
				_tcscat (szSrcPath, _T("\\"));
			}
		}
		else
		/* Get the full path to first file in the string of file names */
		GetFullPathName (tmpName, MAX_PATH, szSrcPath, NULL);	
	}
	else
	{
		/* Check to make sure if they entered c:, if they do then GFPN 
		return current directory even though msdn says it will return c:\ */
		if(_tcslen(arg[nSrc]) == 2)
		{
			if(arg[nSrc][1] == _T(':'))
			{
				_tcscpy (szSrcPath, arg[nSrc]);
				_tcscat (szSrcPath, _T("\\"));
			}
		}
		else
		/* Get the full path of the source file */
		GetFullPathName (arg[nSrc], MAX_PATH, szSrcPath, NULL);
 
	}
 
	/* From this point on, we can assume that the shortest path is 3 letters long
	and that would be [DriveLetter]:\ */
 
	/* If there is no * in the path name and it is a folder
	then we will need to add a wildcard to the pathname
	so FindFirstFile comes up with all the files in that
	folder */
	if(_tcschr (szSrcPath, _T('*')) == NULL && 
		IsExistingDirectory (szSrcPath))
	{
		/* If it doesnt have a \ at the end already then on needs to be added */
		if(szSrcPath[_tcslen(szSrcPath) -  1] != _T('\\'))
			_tcscat (szSrcPath, _T("\\"));
		/* Add a wildcard after the \ */
		_tcscat (szSrcPath, _T("*"));
	}
	/* Make sure there is an ending slash to the path if the dest is a folder */
	if(_tcschr (szDestPath, _T('*')) == NULL && 
		IsExistingDirectory(szDestPath))
	{
		if(szDestPath[_tcslen(szDestPath) -  1] != _T('\\'))
			_tcscat (szDestPath, _T("\\"));
	}
 
 
	/* Get a list of all the files */
	hFile = FindFirstFile (szSrcPath, &findBuffer);
 
 
	/* We need to figure out what the name of the file in the is going to be */
	if((szDestPath[_tcslen(szDestPath) -  1] == _T('*') && szDestPath[_tcslen(szDestPath) -  2] == _T('\\')) ||
		szDestPath[_tcslen(szDestPath) -  1] == _T('\\'))
	{
		/* In this case we will be using the same name as the source file
		for the destination file because destination is a folder */
		bSrcName = TRUE;
	}
	else
	{
		/* Save the name the user entered */
		UseThisName = _tcsrchr(szDestPath,_T('\\'));
		UseThisName++;
		_tcscpy(PreserveName,UseThisName);
	}
 
	/* Strip the paths back to the folder they are in */
	for(i = (_tcslen(szSrcPath) -  1); i > -1; i--)
		if(szSrcPath[i] != _T('\\'))
			szSrcPath[i] = _T('\0');
		else
			break;
 
	for(i = (_tcslen(szDestPath) -  1); i > -1; i--)
		if(szDestPath[i] != _T('\\'))
			szDestPath[i] = _T('\0');
		else
			break;
 
 
		do
		{
			/* Set the override to yes each new file */
			nOverwrite = 1;
 
			/* If it couldnt open the file handle, print out the error */
			if(hFile == INVALID_HANDLE_VALUE)
			{			
				ConOutFormatMessage (GetLastError(), szSrcPath);			
				freep (arg);
        nErrorLevel = 1;
				return 1;
			}
 
			/* Ignore the . and .. files */
			if(!_tcscmp (findBuffer.cFileName, _T("."))  ||
				!_tcscmp (findBuffer.cFileName, _T(".."))||
				findBuffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
 
			/* Copy the base folder over to a tmp string */
			_tcscpy(tmpDestPath,szDestPath);
 
			/* Can't put a file into a folder that isnt there */
			if(!IsExistingDirectory(szDestPath))
			{
				ConOutFormatMessage (GetLastError (), szSrcPath);			
				freep (arg);
        nErrorLevel = 1;
				return 1;
			}
			/* Copy over the destination path name */
			if(bSrcName)
				_tcscat (tmpDestPath, findBuffer.cFileName);
			else
			{
				/* If there is no wildcard you can use the name the user entered */
				if(_tcschr (PreserveName, _T('*')) == NULL)
				{
					_tcscat (tmpDestPath, PreserveName);
				}
				else
				{
					/* The following lines of copy were written by someone else
					(most likely Eric Khoul) and it was taken from ren.c */
					LPTSTR p,q,r;
					TCHAR DoneFile[MAX_PATH];
					/* build destination file name */
					p = findBuffer.cFileName;
					q = PreserveName;
					r = DoneFile;
					while(*q != 0)
					{
						if (*q == '*')
						{
							q++;
							while (*p != 0 && *p != *q)
							{
								*r = *p;
								p++;
								r++;
							}
						}
						else if (*q == '?')
						{
							q++;
							if (*p != 0)
							{
								*r = *p;
								p++;
								r++;
							}
						}
						else
						{
							*r = *q;
							if (*p != 0)
								p++;
							q++;
							r++;
						}
					}
					*r = 0;
					/* Add the filename to the tmp string path */
					_tcscat (tmpDestPath, DoneFile);
 
				}
			}
 
 
			/* Build the string path to the source file */
			_tcscpy(tmpSrcPath,szSrcPath);
			_tcscat (tmpSrcPath, findBuffer.cFileName);
 
			/* Check to see if the file is the same file */
			if(!_tcscmp (tmpSrcPath, tmpDestPath))
				continue;
 
			/* Handle any overriding / prompting that needs to be done */
			if((!(dwFlags & COPY_NO_PROMPT) && IsExistingFile (tmpDestPath)) || dwFlags & COPY_PROMPT)
				nOverwrite = Overwrite(tmpDestPath);
			if(nOverwrite == PROMPT_NO || nOverwrite == PROMPT_BREAK)
				continue;
			if(nOverwrite == PROMPT_ALL || (nOverwrite == PROMPT_YES && bAppend))
				dwFlags |= COPY_NO_PROMPT;
 
			/* Tell weather the copy was successful or not */
			if(copy(tmpSrcPath,tmpDestPath, bAppend, dwFlags))
			{
				nFiles++;
				/* only print source name when more then one file */
				if(_tcschr (arg[nSrc], _T('+')) != NULL || _tcschr (arg[nSrc], _T('*')) != NULL)
					ConOutPrintf("%s\n",findBuffer.cFileName);
				//LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
			}
			else
			{
				/* print out the error message */
				LoadString(CMD_ModuleHandle, STRING_COPY_ERROR3, szMsg, RC_STRING_MAX_SIZE);
				ConOutPrintf(szMsg);
        nErrorLevel = 1;
			}
 
		/* Loop through all wildcard files */
		}while(FindNextFile (hFile, &findBuffer));
	/* Loop through all files in src string with a + */
	}while(_tcsncmp (appendPointer,_T("\0"),1));
 
	/* print out the number of files copied */
	LoadString(CMD_ModuleHandle, STRING_COPY_FILE, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg, nFiles);
 
	CloseHandle(hFile);
	freep (arg);
	return 0;
}
 
 
#endif /* INCLUDE_CMD_COPY */
