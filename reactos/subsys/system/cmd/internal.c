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
 *    03-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Replaced DOS calls by Win32 calls.
 *
 *    08-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help texts ("/?").
 *
 *    18-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added support for quoted arguments (cd "program files").
 *
 *    07-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Clean up.
 *
 *    26-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Replaced remaining CRT io functions by Win32 io functions.
 *        Unicode safe!
 *
 *    30-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added "cd -" feature. Changes to the previous directory.
 *
 *    15-Mar-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
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
 *    19-jul-2005 (Brandon Turner <turnerb7@msu.edu)
 *        Rewrite the CD, it working as Windows 2000 CMD
 *		
 *    19-jul-2005 (Magnus Olsen <magnus@greatlord.com)
 *        Add SetRootPath and GetRootPath         
 */

#include <precomp.h>
#include "resource.h"

#ifdef INCLUDE_CMD_CHDIR

static LPTSTR lpLastPath;


VOID InitLastPath (VOID)
{
	lpLastPath = NULL;
}


VOID FreeLastPath (VOID)
{
	if (lpLastPath)
		free (lpLastPath);
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
      TCHAR num[2];
      INT t=0;

      num[1] = _T('\0');
      num[0] = InPath[0];
      _tcslwr(num);

      if ((InPath[0] >= _T('0')) && (InPath[0] <= _T('9')))
      {
          t = (InPath[0] - _T('0')) +28;
      }
      
      if ((InPath[0] >= _T('a')) && (InPath[0] <= _T('z')))
      {
          t = (InPath[0] - _T('a')) +1;
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
  BOOL fail;
  
  /* Get The current directory path and save it */
  fail = GetCurrentDirectory(MAX_PATH,oldpath);
  if (!fail)   
      return 1;
  
  /* Get current drive directory path if C: was only pass down*/
  
  if (_tcsncicmp(&InPath[1],_T(":\\"),2)!=0)
  {
      if (!GetRootPath(InPath,OutPath,MAX_PATH))
         _tcscpy(OutPath,InPath);
  }
  else 
  {
    _tcscpy(OutPath,InPath);
  }
  
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
INT cmd_chdir (LPTSTR cmd, LPTSTR param)
{
 
	WIN32_FIND_DATA f; 
	HANDLE hFile;
	BOOL bChangeDrive = FALSE;
	TCHAR szPath[MAX_PATH];
	TCHAR szFinalPath[MAX_PATH];
	TCHAR * tmpPath;
	TCHAR szCurrent[MAX_PATH];
	TCHAR szMsg[RC_STRING_MAX_SIZE];
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
	if(szPath[0] == _T('\"'))
	{
		tmpPath = _tcsstr(szPath,_T("\""));
		tmpPath++;
		_tcscpy(szPath,tmpPath);
	}
 
	if(szPath[_tcslen(szPath) - 1] == _T('\"'))
	{
		szPath[_tcslen(szPath) - 1] = _T('\0');
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
		LoadString(CMD_ModuleHandle, STRING_ERROR_PATH_NOT_FOUND, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(szMsg);
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
		
		if(IsExistingDirectory(szFinalPath))
		{
			if(!SetRootPath(szFinalPath))
			{
				/* Change for /D */
				if(bChangeDrive)
					SetCurrentDirectory(szFinalPath);
				return 0;
			}
		}
	}while(FindNextFile (hFile, &f));
 
	/* Didnt find an directories */
	LoadString(CMD_ModuleHandle, STRING_ERROR_PATH_NOT_FOUND, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg);
	nErrorLevel = 1;
	return 1;
}

#endif



#ifdef INCLUDE_CMD_MKDIR
/*
 * MD / MKDIR
 *
 */
INT cmd_mkdir (LPTSTR cmd, LPTSTR param)
{
	LPTSTR dir;		/* pointer to the directory to change to */
	LPTSTR place;	/* used to search for the \ when no space is used */
	LPTSTR *p = NULL;
	INT argc;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_MKDIR_HELP);
		return 0;
	}


	/* check if there is no space between the command and the path */
	if (param[0] == _T('\0'))
	{
		/* search for the \ or . so that both short & long names will work */
		for (place = cmd; *place; place++)
			if (*place == _T('.') || *place == _T('\\'))
				break;

		if (*place)
			dir = place;
		else
			/* signal that there are no parameters */
			dir = NULL;
	}
	else
	{
		p = split (param, &argc, FALSE);
		if (argc > 1)
		{
			/*JPP 20-Jul-1998 use standard error message */
			error_too_many_parameters (param);
			freep (p);
			return 1;
		}
		else
			dir = p[0];
	}

	if (!dir)
	{
		ConErrResPuts (STRING_ERROR_REQ_PARAM_MISSING);
		return 1;
	}

	/* remove trailing \ if any, but ONLY if dir is not the root dir */
	if (_tcslen (dir) >= 2 && dir[_tcslen (dir) - 1] == _T('\\'))
		dir[_tcslen(dir) - 1] = _T('\0');

	if (!CreateDirectory (dir, NULL))
	{
		ErrorMessage (GetLastError(), _T("MD"));

		freep (p);
		return 1;
	}

	freep (p);

	return 0;
}
#endif


#ifdef INCLUDE_CMD_RMDIR
/*
 * RD / RMDIR
 *
 */
INT cmd_rmdir (LPTSTR cmd, LPTSTR param)
{
	LPTSTR dir;		/* pointer to the directory to change to */
	LPTSTR place;	/* used to search for the \ when no space is used */

	LPTSTR *p = NULL;
	INT argc;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_RMDIR_HELP);
		return 0;
	}

	/* check if there is no space between the command and the path */
	if (param[0] == _T('\0'))
	{
		/* search for the \ or . so that both short & long names will work */
		for (place = cmd; *place; place++)
			if (*place == _T('.') || *place == _T('\\'))
				break;

		if (*place)
			dir = place;
		else
			/* signal that there are no parameters */
			dir = NULL;
	}
	else
	{
		p = split (param, &argc, FALSE);
		if (argc > 1)
		{
			/*JPP 20-Jul-1998 use standard error message */
			error_too_many_parameters (param);
			freep (p);
			return 1;
		}
		else
			dir = p[0];
	}

	if (!dir)
	{
		ConErrResPuts(STRING_ERROR_REQ_PARAM_MISSING);
		return 1;
	}

	/* remove trailing \ if any, but ONLY if dir is not the root dir */
	if (_tcslen (dir) >= 2 && dir[_tcslen (dir) - 1] == _T('\\'))
		dir[_tcslen(dir) - 1] = _T('\0');

	if (!RemoveDirectory (dir))
	{
		ErrorMessage (GetLastError(), _T("RD"));
		freep (p);

		return 1;
	}

	freep (p);

	return 0;
}
#endif


/*
 * set the exitflag to true
 *
 */
INT CommandExit (LPTSTR cmd, LPTSTR param)
{
	if (!_tcsncmp (param, _T("/?"), 2))
		ConOutResPaging(TRUE,STRING_EXIT_HELP);

	if (bc != NULL && _tcsnicmp(param,_T("/b"),2) == 0)
	{
		param += 2;
		while (_istspace (*param))
			param++;
		if (_istdigit(*param))
			nErrorLevel = _ttoi(param);
		ExitBatch (NULL);
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
INT CommandRem (LPTSTR cmd, LPTSTR param)
{
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_REM_HELP);
	}

	return 0;
}
#endif /* INCLUDE_CMD_REM */


INT CommandShowCommands (LPTSTR cmd, LPTSTR param)
{
	PrintCommandList ();
	return 0;
}

INT CommandShowCommandsDetail (LPTSTR cmd, LPTSTR param)
{
	PrintCommandListDetail ();
	return 0;
}

/* EOF */
