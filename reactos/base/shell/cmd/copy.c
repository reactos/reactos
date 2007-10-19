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
 *    13-Dec-1998 (Eric Kohl)
 *        Added COPY command to CMD.
 *
 *    26-Jan-1998 (Eric Kohl)
 *        Replaced CRT io functions by Win32 io functions.
 *
 *    27-Oct-1998 (Eric Kohl)
 *        Disabled prompting when used in batch mode.
 *
 *    03-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *
 *    13-Jul-2005 (Brandon Turner) <turnerb7@msu.edu>)
 *        Rewrite to clean up copy and support wildcard.
 *
 *    20-Jul-2005 (Brandon Turner) <turnerb7@msu.edu>)
 *        Add touch syntax.  "copy arp.exe+,,"
 *        Copy command is now completed.
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_COPY

enum
{
	COPY_ASCII       = 0x001,   /* /A  */
	COPY_DECRYPT     = 0x004,   /* /D  */
	COPY_VERIFY      = 0x008,   /* /V  : Dummy, Never will be Impleneted */
	COPY_SHORTNAME   = 0x010,   /* /N  : Dummy, Never will be Impleneted */
	COPY_NO_PROMPT   = 0x020,   /* /Y  */
	COPY_PROMPT      = 0x040,   /* /-Y */
	COPY_RESTART     = 0x080,   /* /Z  */
	COPY_BINARY      = 0x100,   /* /B  */
};

INT
copy (TCHAR source[MAX_PATH],
	  TCHAR dest[MAX_PATH],
	  INT append,
	  DWORD lpdwFlags,
	  BOOL bTouch)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	FILETIME srctime,NewFileTime;
	HANDLE hFileSrc;
	HANDLE hFileDest;
	LPBYTE buffer;
	DWORD  dwAttrib;
	DWORD  dwRead;
	DWORD  dwWritten;
	BOOL   bEof = FALSE;
	TCHAR TrueDest[MAX_PATH];
	TCHAR TempSrc[MAX_PATH];
	TCHAR * FileName;
	SYSTEMTIME CurrentTime;

	/* Check Breaker */
	if(CheckCtrlBreak(BREAK_INPUT))
		return 0;

#ifdef _DEBUG
	DebugPrintf (_T("checking mode\n"));
#endif

	if(bTouch)
	{
	hFileSrc = CreateFile (source, GENERIC_WRITE, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, 0, NULL);
	if (hFileSrc == INVALID_HANDLE_VALUE)
	{
		LoadString(CMD_ModuleHandle, STRING_COPY_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		ConOutPrintf(szMsg, source);
        nErrorLevel = 1;
		return 0;
	}

		GetSystemTime(&CurrentTime);
		SystemTimeToFileTime(&CurrentTime, &NewFileTime);
		if(SetFileTime(hFileSrc,(LPFILETIME) NULL, (LPFILETIME) NULL, &NewFileTime))
		{
			CloseHandle(hFileSrc);
			nErrorLevel = 1;
			return 1;

		}
		else
		{
			CloseHandle(hFileSrc);
			return 0;
		}
	}

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
		lpdwFlags & COPY_ASCII ? "ASCII" : "BINARY");
#endif

	/* Check to see if /D or /Z are true, if so we need a middle
	   man to copy the file too to allow us to use CopyFileEx later */
	if(lpdwFlags & COPY_DECRYPT)
	{
		GetEnvironmentVariable(_T("TEMP"),TempSrc,MAX_PATH);
		_tcscat(TempSrc,_T("\\"));
		FileName = _tcsrchr(source,_T('\\'));
		FileName++;
		_tcscat(TempSrc,FileName);
		/* This is needed to be on the end to prevent an error
		   if the user did "copy /D /Z foo bar then it would be copied
		   too %TEMP%\foo here and when %TEMP%\foo when it sets it up
		   for COPY_RESTART, this would mean it is copying to itself
		   which would error when it tried to open the handles for ReadFile
		   and WriteFile */
		_tcscat(TempSrc,_T(".decrypt"));
		if(!CopyFileEx(source, TempSrc, NULL, NULL, FALSE, COPY_FILE_ALLOW_DECRYPTED_DESTINATION))
		{
		   nErrorLevel = 1;
		   return 0;
		}
		_tcscpy(source, TempSrc);
	}


	if(lpdwFlags & COPY_RESTART)
	{
		_tcscpy(TrueDest, dest);
		GetEnvironmentVariable(_T("TEMP"),dest,MAX_PATH);
		_tcscat(dest,_T("\\"));
		FileName = _tcsrchr(TrueDest,_T('\\'));
		FileName++;
		_tcscat(dest,FileName);
	}


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
		ConOutResPuts(STRING_ERROR_PATH_NOT_FOUND);
        nErrorLevel = 1;
		return 0;
	}

	/* A page-aligned buffer usually give more speed */
	buffer = (LPBYTE)VirtualAlloc(NULL, BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);
	if (buffer == NULL)
	{
		CloseHandle (hFileDest);
		CloseHandle (hFileSrc);
		ConOutResPuts(STRING_ERROR_OUT_OF_MEMORY);
        nErrorLevel = 1;
		return 0;
	}

	do
	{

		ReadFile (hFileSrc, buffer, BUFF_SIZE, &dwRead, NULL);
		if (lpdwFlags & COPY_ASCII)
		{
			LPBYTE pEof = memchr(buffer, 0x1A, dwRead);
			if (pEof != NULL)
			{
				bEof = TRUE;
				dwRead = pEof-buffer+1;
				break;
			}
		}

		if (dwRead == 0)
			break;

		WriteFile (hFileDest, buffer, dwRead, &dwWritten, NULL);
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

#ifdef _DEBUG
	DebugPrintf (_T("setting time\n"));
#endif
	SetFileTime (hFileDest, &srctime, NULL, NULL);

	if ((lpdwFlags & COPY_ASCII) && !bEof)
	{
		/* we're dealing with ASCII files! */
		buffer[0] = 0x1A;
#ifdef _DEBUG
		DebugPrintf (_T("appending ^Z\n"));
#endif
		WriteFile (hFileDest, buffer, sizeof(CHAR), &dwWritten, NULL);
	}

	VirtualFree (buffer, 0, MEM_RELEASE);
	CloseHandle (hFileDest);
	CloseHandle (hFileSrc);

#ifdef _DEBUG
	DebugPrintf (_T("setting mode\n"));
#endif
	SetFileAttributes (dest, dwAttrib);

	/* Now finish off the copy if needed with CopyFileEx */
	if(lpdwFlags & COPY_RESTART)
	{
		if(!CopyFileEx(dest, TrueDest, NULL, NULL, FALSE, COPY_FILE_RESTARTABLE))
		{
		   nErrorLevel = 1;
		   DeleteFile(dest);
           return 0;
		}
		/* Take care of file in the temp folder */
		DeleteFile(dest);

	}

	if(lpdwFlags & COPY_DECRYPT)
	   DeleteFile(TempSrc);

	return 1;
}


static INT CopyOverwrite (LPTSTR fn)
{
	/*ask the user if they want to override*/
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	INT res;
	LoadString(CMD_ModuleHandle, STRING_COPY_HELP1, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg,fn);
	res = FilePromptYNA (_T(""));
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
	BOOL bTouch = FALSE;
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
	TCHAR * szTouch;
	BOOL bDone = FALSE;


  /*Show help/usage info*/
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE, STRING_COPY_HELP2);
		return 0;
	}

  nErrorLevel = 0;

  /* Get the envor value if it exists */
  evar = cmd_alloc(512 * sizeof(TCHAR));
  if (evar==NULL) size = 0;
  else
  {
   size = GetEnvironmentVariable (_T("COPYCMD"), evar, 512);
  }
  if (size > 512)
  {
    evar = cmd_realloc(evar,size * sizeof(TCHAR) );
    if (evar!=NULL)
    {
      size = GetEnvironmentVariable (_T("COPYCMD"), evar, size);
      }
    else
    {
      size=0;
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
        dwFlags |=COPY_ASCII;
        t++;
      }

      else if (_tcsncicmp(_T("/B"),&evar[t],2)==0)
      {
        dwFlags |= COPY_BINARY;
        t++;
      }
      else if (_tcsncicmp(_T("/D"),&evar[t],2)==0)
      {
        dwFlags |= COPY_DECRYPT;
        t++;
      }

			else if (_tcsncicmp(_T("/V"),&evar[t],2)==0)
      {
        dwFlags |= COPY_VERIFY;
        t++;
      }

			else if (_tcsncicmp(_T("/N"),&evar[t],2)==0)
      {
        dwFlags |= COPY_SHORTNAME;
        t++;
      }

      else if (_tcsncicmp(_T("/Y"),&evar[t],2)==0)
      {
        dwFlags |= COPY_NO_PROMPT;
        t++;
      }

      else if (_tcsncicmp(_T("/-Y"),&evar[t],3)==0)
      {
        dwFlags |= COPY_PROMPT;
        t+=2;
      }

      else if (_tcsncicmp(_T("/Z"),&evar[t],2)==0)
      {
        dwFlags |= COPY_PROMPT;
        t++;
      }
    }
  }
  cmd_free(evar);


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
                    LoadString(CMD_ModuleHandle, STRING_ERROR_INVALID_SWITCH, szMsg, RC_STRING_MAX_SIZE);
	                ConOutPrintf(szMsg, _totupper(arg[i][1]));
					nErrorLevel = 1;
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
			{
				nSrc = i;
			}
			else if(*arg[i] == _T('+') || *arg[i] == _T(','))
			{
				/* Add these onto the source string
				   this way we can do all checks
					directly on source string later on */
				_tcscat(arg[nSrc],arg[i]);
				nFiles--;
			}
			else if(nDes == -1)
			{
				nDes = i;
			}

		}
	}

	/* keep quiet within batch files */
	if (bc != NULL)
        {
		dwFlags |= COPY_NO_PROMPT;
		dwFlags &= ~COPY_PROMPT;
        }

	if(nFiles < 1)
	{
		/* There is not enough files, there has to be at least 1 */
		ConOutResPuts(STRING_ERROR_REQ_PARAM_MISSING);
		freep (arg);
		return 1;
	}

	if(nFiles > 2)
	{
		/* there is too many file names in command */
        LoadString(CMD_ModuleHandle, STRING_ERROR_TOO_MANY_PARAMETERS, szMsg, RC_STRING_MAX_SIZE);
	    ConErrPrintf(szMsg,_T(""));
        nErrorLevel = 1;
		freep (arg);
		return 1;
	}

	if(((_tcschr (arg[nSrc], _T('+')) != NULL) ||
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
		if(_tcslen(arg[nDes]) == 2 && arg[nDes][1] == _T(':'))
		{
			GetRootPath(arg[nDes],szDestPath,MAX_PATH);
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
				else
					bDone = TRUE;
				break;

			}

			_tcsncat(tmpName,appendPointer,1);
			appendPointer++;

		}
		/* Finish the string off with a null char */
		_tcsncat(tmpName,_T("\0"),1);

		if(_tcschr (arg[nSrc], _T(',')) != NULL)
			{
				/* Only time there is a , in the source is when they are using touch
				   Cant have a destination and can only have on ,, at the end of the string
					Cant have more then one file name */
				szTouch = _tcsstr (arg[nSrc], _T("+"));
				if(_tcsncmp (szTouch,_T("+,,\0"),4) || nDes != -1)
				{
					LoadString(CMD_ModuleHandle, STRING_ERROR_INVALID_PARAM_FORMAT, szMsg, RC_STRING_MAX_SIZE);
					ConErrPrintf(szMsg,arg[nSrc]);
					nErrorLevel = 1;
					freep (arg);
					return 1;
				}
				bTouch = TRUE;
				bDone = TRUE;
			}

		if(_tcslen(tmpName) == 2)
		{
			if(tmpName[1] == _T(':'))
			{

				GetRootPath(tmpName,szSrcPath,MAX_PATH);
			}
		}
		else
		/* Get the full path to first file in the string of file names */
		GetFullPathName (tmpName, MAX_PATH, szSrcPath, NULL);
	}
	else
	{
		bDone = TRUE;
		if(_tcslen(arg[nSrc]) == 2 && arg[nSrc][1] == _T(':'))
		{
			GetRootPath(arg[nSrc],szSrcPath,MAX_PATH);
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
			/* Check Breaker */
			if(CheckCtrlBreak(BREAK_INPUT))
			{
				freep(arg);
				return 1;
			}
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
			if(!bTouch && !_tcscmp (tmpSrcPath, tmpDestPath))
				continue;

			/* Handle any overriding / prompting that needs to be done */
			if(((!(dwFlags & COPY_NO_PROMPT) && IsExistingFile (tmpDestPath)) || dwFlags & COPY_PROMPT) && !bTouch)
				nOverwrite = CopyOverwrite(tmpDestPath);
			if(nOverwrite == PROMPT_NO || nOverwrite == PROMPT_BREAK)
				continue;
			if(nOverwrite == PROMPT_ALL || (nOverwrite == PROMPT_YES && bAppend))
				dwFlags |= COPY_NO_PROMPT;

			/* Tell weather the copy was successful or not */
			if(copy(tmpSrcPath,tmpDestPath, bAppend, dwFlags, bTouch))
			{
				nFiles++;
				/* only print source name when more then one file */
				if(_tcschr (arg[nSrc], _T('+')) != NULL || _tcschr (arg[nSrc], _T('*')) != NULL)
					ConOutPrintf(_T("%s\n"),findBuffer.cFileName);
				//LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
			}
			else
			{
				/* print out the error message */
				LoadString(CMD_ModuleHandle, STRING_COPY_ERROR3, szMsg, RC_STRING_MAX_SIZE);
				ConOutPrintf(szMsg);
				ConOutFormatMessage (GetLastError(), szSrcPath);
				nErrorLevel = 1;
			}

		/* Loop through all wildcard files */
		}while(FindNextFile (hFile, &findBuffer));
	/* Loop through all files in src string with a + */
	}while(!bDone);

	/* print out the number of files copied */
	LoadString(CMD_ModuleHandle, STRING_COPY_FILE, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg, nFiles);

	FindClose(hFile);
  if (arg!=NULL)
      freep(arg);

	return 0;
}


#endif /* INCLUDE_CMD_COPY */
