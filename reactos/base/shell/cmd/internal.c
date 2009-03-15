/*
 *  INTERNAL.C - command.com internal commands.
 *
 *
 *  History:
 *
 *  17/08/94 (Tim Norman)
 *    started.
 *
 *  08/08/95 (Matt Rains)
 *    i have cleaned up the source code. changes now bring this source into
 *    guidelines for recommended programming practice.
 *
 *  cd()
 *    started.
 *
 *  dir()
 *    i have added support for file attributes to the DIR() function. the
 *    routine adds "d" (directory) and "r" (read only) output. files with the
 *    system attribute have the filename converted to lowercase. files with
 *    the hidden attribute are not displayed.
 *
 *    i have added support for directorys. now if the directory attribute is
 *    detected the file size if replaced with the string "<dir>".
 *
 *  ver()
 *    started.
 *
 *  md()
 *    started.
 *
 *  rd()
 *    started.
 *
 *  del()
 *    started.
 *
 *  does not support wildcard selection.
 *
 *  todo: add delete directory support.
 *        add recursive directory delete support.
 *
 *  ren()
 *    started.
 *
 *  does not support wildcard selection.
 *
 *    todo: add rename directory support.
 *
 *  a general structure has been used for the cd, rd and md commands. this
 *  will be better in the long run. it is too hard to maintain such diverse
 *  functions when you are involved in a group project like this.
 *
 *  12/14/95 (Tim Norman)
 *    fixed DIR so that it will stick \*.* if a directory is specified and
 *    that it will stick on .* if a file with no extension is specified or
 *    *.* if it ends in a \
 *
 *  1/6/96 (Tim Norman)
 *    added an isatty call to DIR so it won't prompt for keypresses unless
 *    stdin and stdout are the console.
 *
 *    changed parameters to be mutually consistent to make calling the
 *    functions easier
 *
 *  rem()
 *    started.
 *
 *  doskey()
 *    started.
 *
 *    01/22/96 (Oliver Mueller)
 *        error messages are now handled by perror.
 *
 *    02/05/96 (Tim Norman)
 *        converted all functions to accept first/rest parameters
 *
 *    07/26/96 (Tim Norman)
 *        changed return values to int instead of void
 *
 *        path() started.
 *
 *    12/23/96 (Aaron Kaufman)
 *        rewrote dir() to mimic MS-DOS's dir
 *
 *    01/28/97 (Tim Norman)
 *        cleaned up Aaron's DIR code
 *
 *    06/13/97 (Tim Norman)
 *        moved DIR code to dir.c
 *        re-implemented Aaron's DIR code
 *
 *    06/14/97 (Steffan Kaiser)
 *        ctrl-break handling
 *        bug fixes
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    03-Dec-1998 (Eric Kohl)
 *        Replaced DOS calls by Win32 calls.
 *
 *    08-Dec-1998 (Eric Kohl)
 *        Added help texts ("/?").
 *
 *    18-Dec-1998 (Eric Kohl)
 *        Added support for quoted arguments (cd "program files").
 *
 *    07-Jan-1999 (Eric Kohl)
 *        Clean up.
 *
 *    26-Jan-1999 (Eric Kohl)
 *        Replaced remaining CRT io functions by Win32 io functions.
 *        Unicode safe!
 *
 *    30-Jan-1999 (Eric Kohl)
 *        Added "cd -" feature. Changes to the previous directory.
 *
 *    15-Mar-1999 (Eric Kohl)
 *        Fixed bug in "cd -" feature. If the previous directory was a root
 *        directory, it was ignored.
 *
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.se>)
 *        Improved chdir/cd command.
 *
 *    02-Apr-2004 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hard code string so they can be
 *		  translate to other langues.
 *
 *    19-Jul-2005 (Brandon Turner <turnerb7@msu.edu>)
 *        Rewrite the CD, it working as Windows 2000 CMD
 *
 *    19-Jul-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Add SetRootPath and GetRootPath
 *
 *    14-Jul-2007 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *        Added commands help display to help command (ex. : "help cmd")
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_CHDIR

static LPTSTR lpLastPath;


VOID InitLastPath (VOID)
{
	lpLastPath = NULL;
}


VOID FreeLastPath (VOID)
{
	if (lpLastPath)
		cmd_free (lpLastPath);
}

/* help functions for getting current path from drive
   without changing drive. Return code 0 = ok, 1 = fail.
   INT GetRootPath("C:",outbuffer,chater size of outbuffer);
   the first param can have any size, if the the two frist
   letter are not a drive with : it will get Currentpath on
   current drive exacly as GetCurrentDirectory does.
   */

INT GetRootPath(TCHAR *InPath,TCHAR *OutPath,INT size)
{
  INT retcode = 1;

  if (_tcslen(InPath)>1)
  {
    if (InPath[1]==_T(':'))
    {
      INT t=0;

      if ((InPath[0] >= _T('0')) && (InPath[0] <= _T('9')))
      {
          t = (InPath[0] - _T('0')) +28;
      }

      if ((InPath[0] >= _T('a')) && (InPath[0] <= _T('z')))
      {
          t = (InPath[0] - _T('a')) +1;
          InPath[0] = t + _T('A') - 1;
      }

       if ((InPath[0] >= _T('A')) && (InPath[0] <= _T('Z')))
      {
          t = (InPath[0] - _T('A')) +1;
      }

      if (_tgetdcwd(t,OutPath,size) != NULL)
      {
        return 0;
      }
     }
    }

  /* fail */
  if (_tcslen(InPath)>1)
  {
    if (InPath[1]==_T(':'))
       return 1;
  }

  /* Get current directory */
  retcode = GetCurrentDirectory(size,OutPath);
  if (retcode==0)
      return 1;

  return 0;
}


BOOL SetRootPath(TCHAR *InPath)
{
  TCHAR oldpath[MAX_PATH];
  TCHAR OutPath[MAX_PATH];
  TCHAR OutPathTemp[MAX_PATH];
  TCHAR OutPathTemp2[MAX_PATH];
  BOOL fail;


  /* Get The current directory path and save it */
  fail = GetCurrentDirectory(MAX_PATH,oldpath);
  if (!fail)
      return 1;

  /* Get current drive directory path if C: was only pass down*/

  if (_tcsncicmp(&InPath[1],_T(":\\"),2)!=0)
  {
      if (!GetRootPath(InPath,OutPathTemp,MAX_PATH))
         _tcscpy(OutPathTemp,InPath);
  }
  else
  {
    _tcscpy(OutPathTemp,InPath);
  }

   _tcsupr(OutPathTemp);
  /* The use of both of these together will correct the case of a path
     where as one alone or GetFullPath will not.  Exameple:
	  c:\windows\SYSTEM32 => C:\WINDOWS\system32 */
  GetFullPathName(OutPathTemp, MAX_PATH, OutPathTemp2, NULL);
  GetPathCase(OutPathTemp2, OutPath);

  fail = SetCurrentDirectory(OutPath);
  if (!fail)
      return 1;



  SetCurrentDirectory(OutPath);
  GetCurrentDirectory(MAX_PATH,OutPath);
  _tchdir(OutPath);

  if (_tcsncicmp(OutPath,oldpath,2)!=0)
      SetCurrentDirectory(oldpath);

 return 0;
}


/*
 * CD / CHDIR
 *
 */
INT cmd_chdir (LPTSTR param)
{

	WIN32_FIND_DATA f;
	HANDLE hFile;
	BOOL bChangeDrive = FALSE;
	TCHAR szPath[MAX_PATH];
	TCHAR szFinalPath[MAX_PATH];
	TCHAR * tmpPath;
	TCHAR szCurrent[MAX_PATH];
	INT i;


	/* Filter out special cases first */

	/* Print Help */
	if (!_tcsncmp(param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_CD_HELP);
		return 0;
	}

  /* Set Error Level to Success */
	nErrorLevel = 0;

	/* Input String Contains /D Switch */
	if (!_tcsncicmp(param, _T("/D"), 2))
	{
		bChangeDrive = TRUE;
		tmpPath = _tcsstr(param,_T(" "));
		if(!tmpPath)
		{
			/* Didnt find an directories */
			ConErrResPrintf(STRING_ERROR_PATH_NOT_FOUND);
			nErrorLevel = 1;
			return 1;
		}
		tmpPath++;
		_tcscpy(szPath,tmpPath);
	}
	else
	{
		_tcscpy(szPath,param);
	}

	/* Print Current Directory on a disk */
	if (_tcslen(szPath) == 2 && szPath[1] == _T(':'))
	{
		if(GetRootPath(szPath,szCurrent,MAX_PATH))
		{
			nErrorLevel = 1;
			return 1;
		}
		ConOutPuts(szCurrent);
		return 0;
	}

	/* Get Current Directory */
	GetRootPath(_T("."),szCurrent,MAX_PATH);

   /* Remove " */
	i = 0;
	while(i < (INT)_tcslen(szPath))
	{
		if(szPath[i] == _T('\"'))
			memmove(&szPath[i],&szPath[i + 1], _tcslen(&szPath[i]) * sizeof(TCHAR));
		else
			i++;
	}

	tmpPath = szPath;
	while (_istspace (*tmpPath))
			tmpPath++;
	_tcscpy(szPath,tmpPath);

	if (szPath[0] == _T('\0'))
	{
		ConOutPuts(szCurrent);
		return 0;
	}


	/* change to full path if relative path was given */
	GetFullPathName(szPath,MAX_PATH,szFinalPath,NULL);

	if(szFinalPath[_tcslen(szFinalPath) - 1] == _T('\\') && _tcslen(szFinalPath) > 3)
		szFinalPath[_tcslen(szFinalPath) - 1] = _T('\0');

	/* Handle Root Directory Alone*/
	if (_tcslen(szFinalPath) == 3 && szFinalPath[1] == _T(':'))
	{
		if(!SetRootPath(szFinalPath))
		{
			/* Change prompt if it is one the same drive or /D */
			if(bChangeDrive || !_tcsncicmp(szFinalPath,szCurrent,1))
				SetCurrentDirectory(szFinalPath);
			return 0;
		}
		/* Didnt find an directories */
		ConErrResPrintf(STRING_ERROR_PATH_NOT_FOUND);
		nErrorLevel = 1;
		return 1;

	}

	/* Get a list of all the files */
	hFile = FindFirstFile (szFinalPath, &f);

	do
	{
		if(hFile == INVALID_HANDLE_VALUE)
		{
			ConErrFormatMessage (GetLastError(), szFinalPath);
			nErrorLevel = 1;
			return 1;
		}

		/* Strip the paths back to the folder they are in */
		for(i = (_tcslen(szFinalPath) -  1); i > -1; i--)
			if(szFinalPath[i] != _T('\\'))
				szFinalPath[i] = _T('\0');
			else
				break;

		_tcscat(szFinalPath,f.cFileName);

		if ((f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==  FILE_ATTRIBUTE_DIRECTORY)
		{
			if(!SetRootPath(szFinalPath))
			{
				/* Change for /D */
				if(bChangeDrive)
				{
					_tcsupr(szFinalPath);
					GetPathCase(szFinalPath, szPath);
					SetCurrentDirectory(szPath);
				}
				return 0;
			}

		}
	}while(FindNextFile (hFile, &f));

	/* Didnt find an directories */
	ConErrResPrintf(STRING_ERROR_PATH_NOT_FOUND);
	nErrorLevel = 1;
	return 1;
}

#endif



#ifdef INCLUDE_CMD_MKDIR

/* Helper funtion for mkdir to make directories in a path.
Dont use the api to decrease depence on libs */
BOOL
MakeFullPath(TCHAR * DirPath)
{
    TCHAR path[MAX_PATH];
    TCHAR *p = DirPath;
    INT  n;

    if (CreateDirectory(DirPath, NULL))
        return TRUE;
    else if (GetLastError() != ERROR_PATH_NOT_FOUND)
        return FALSE;

    /* got ERROR_PATH_NOT_FOUND, so try building it up one component at a time */
    if (p[0] && p[1] == _T(':'))
        p += 2;
    while (*p == _T('\\'))
        p++; /* skip drive root */
    do
    {
       p = _tcschr(p, _T('\\'));
       n = p ? p++ - DirPath : _tcslen(DirPath);
       _tcsncpy(path, DirPath, n);
       path[n] = _T('\0');
       if( !CreateDirectory(path, NULL) &&
           (GetLastError() != ERROR_ALREADY_EXISTS))
           return FALSE;
    } while (p != NULL);

    return TRUE;
}

/*
 * MD / MKDIR
 *
 */
INT cmd_mkdir (LPTSTR param)
{
	LPTSTR *p;
	INT argc, i;
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_MKDIR_HELP);
		return 0;
	}

	p = split (param, &argc, FALSE);
	if (argc == 0)
	{
		ConErrResPuts(STRING_ERROR_REQ_PARAM_MISSING);
		nErrorLevel = 1;
		freep(p);
		return 1;
	}

	nErrorLevel = 0;
	for (i = 0; i < argc; i++)
	{
		if (!MakeFullPath(p[i]))
		{
			if(GetLastError() == ERROR_PATH_NOT_FOUND)
			{
				ConErrResPuts(STRING_MD_ERROR2);
			}
			else
			{
				ErrorMessage (GetLastError(), _T("MD"));
			}
			nErrorLevel = 1;
		}
	}

	freep (p);
	return nErrorLevel;
}
#endif


#ifdef INCLUDE_CMD_RMDIR
/*
 * RD / RMDIR
 *
 */
BOOL DeleteFolder(LPTSTR FileName)
{
	TCHAR Base[MAX_PATH];
	TCHAR TempFileName[MAX_PATH];
	HANDLE hFile;
    WIN32_FIND_DATA f;
	_tcscpy(Base,FileName);
	_tcscat(Base,_T("\\*"));
	hFile = FindFirstFile(Base, &f);
	Base[_tcslen(Base) - 1] = _T('\0');
    if (hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
       		if (!_tcscmp(f.cFileName, _T(".")) ||
                !_tcscmp(f.cFileName, _T("..")))
		        continue;
			_tcscpy(TempFileName,Base);
			_tcscat(TempFileName,f.cFileName);

			if(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				DeleteFolder(TempFileName);
			else
			{
				SetFileAttributes(TempFileName,FILE_ATTRIBUTE_NORMAL);
				if(!DeleteFile(TempFileName))
					return 0;
			}

        }while (FindNextFile (hFile, &f));
	    FindClose (hFile);
    }
	return RemoveDirectory(FileName);
}
INT cmd_rmdir (LPTSTR param)
{
	TCHAR ch;
	INT args;
	INT dirCount;
	LPTSTR *arg;
	INT i;
	BOOL RD_SUB = FALSE;
	BOOL RD_QUIET = FALSE;
	INT res;
	INT nError = 0;
	TCHAR szFullPath[MAX_PATH];

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_RMDIR_HELP);
		return 0;
	}

	arg = split (param, &args, FALSE);
	dirCount = 0;

	/* check for options anywhere in command line */
	for (i = 0; i < args; i++)
	{
		if (*arg[i] == _T('/'))
		{
			/*found a command, but check to make sure it has something after it*/
			if (_tcslen (arg[i]) == 2)
			{
				ch = _totupper (arg[i][1]);

				if (ch == _T('S'))
				{
					RD_SUB = TRUE;
				}
				else if (ch == _T('Q'))
				{
					RD_QUIET = TRUE;
				}
			}
		}
		else
		{
			dirCount++;
		}
	}

	if (dirCount == 0)
	{
		/* No folder to remove */
		error_req_param_missing();
		freep(arg);
		return 1;
	}

	for (i = 0; i < args; i++)
	{
		if (*arg[i] == _T('/'))
			continue;

		if (RD_SUB)
		{
			/* ask if they want to delete evrything in the folder */
			if (!RD_QUIET)
			{
				res = FilePromptYNA (STRING_DEL_HELP2);
				if (res == PROMPT_NO || res == PROMPT_BREAK)
				{
					nError = 1;
					continue;
				}
				if (res == PROMPT_ALL)
					RD_QUIET = TRUE;
			}
			/* get the folder name */
			GetFullPathName(arg[i],MAX_PATH,szFullPath,NULL);

			/* remove trailing \ if any, but ONLY if dir is not the root dir */
			if (_tcslen (szFullPath) >= 2 && szFullPath[_tcslen (szFullPath) - 1] == _T('\\'))
				szFullPath[_tcslen(szFullPath) - 1] = _T('\0');

			res = DeleteFolder(szFullPath);
		}
		else
		{
			res = RemoveDirectory(arg[i]);
		}

		if (!res)
		{
			/* Couldn't delete the folder, print out the error */
			nError = GetLastError();
			ErrorMessage(nError, _T("RD"));
		}
	}

	freep (arg);
	return nError;
}
#endif


/*
 * set the exitflag to true
 *
 */
INT CommandExit (LPTSTR param)
{
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_EXIT_HELP);
		/* Just make sure */
		bExit = FALSE;
		/* Dont exit */
		return 0;
	}

	if (bc != NULL && _tcsnicmp(param,_T("/b"),2) == 0)
	{
		param += 2;
		while (_istspace (*param))
			param++;
		if (_istdigit(*param))
			nErrorLevel = _ttoi(param);
		ExitBatch();
	}

	else
		bExit = TRUE;


	return 0;

}

#ifdef INCLUDE_CMD_REM
/*
 * does nothing
 *
 */
INT CommandRem (LPTSTR param)
{
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_REM_HELP);
	}

	return 0;
}
#endif /* INCLUDE_CMD_REM */


INT CommandShowCommands (LPTSTR param)
{
	PrintCommandList ();
	return 0;
}

INT CommandShowCommandsDetail (LPTSTR param)
{
	/* If a param was send, display help of correspondent command */
	if (_tcslen(param))
	{
		LPTSTR NewCommand = cmd_alloc((_tcslen(param)+4)*sizeof(TCHAR));
		_tcscpy(NewCommand, param);
		_tcscat(NewCommand, _T(" /?"));
		DoCommand(NewCommand, NULL);
		cmd_free(NewCommand);
	}
	/* Else, display detailed commands list */
	else
	{
		PrintCommandListDetail ();
	}
	return 0;
}

/* EOF */
