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
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "cmd.h"


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

/*
 * CD / CHDIR
 *
 */
INT cmd_chdir (LPTSTR cmd, LPTSTR param)
{
	LPTSTR dir;		/* pointer to the directory to change to */
	LPTSTR lpOldPath;
	LPTSTR endofstring; /* pointer to the null character in the directory to change to */
	LPTSTR lastquote; /* pointer to the last quotation mark in the directory to change to */

	/*Should we better declare a variable containing _tsclen(dir) ? It's used a few times,
	  but on the other hand paths are generally not very long*/

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Changes the current directory or displays it's name\n\n"
					   "CHDIR [drive:][path]\n"
					   "CHDIR[..|-]\n"
					   "CD [drive:][path]\n"
					   "CD[..|-]\n\n"
					   "  ..   parent directory\n"
					   "  -    previous directory\n\n"
					   "Type CD drive: to display the current directory on the specified drive.\n"
					   "Type CD without a parameter to display the current drive and directory."));
		return 0;
	}

	/* The whole param string is our parameter these days. The only thing we do is eliminating every quotation mark */
	/* Is it safe to change the characters param is pointing to? I presume it is, as there doesn't seem to be any
	post-processing of it after the function call (what would that accomplish?) */

	dir=param;
	endofstring=dir+_tcslen(dir);

	while ((lastquote = _tcsrchr(dir,'\"')))
	{
		endofstring--;
		memmove(lastquote,lastquote+1,endofstring-lastquote);
		*endofstring=_T('\0');
	}

	/* if doing a CD and no parameters given, print out current directory */
	if (!dir || !dir[0])
	{
		TCHAR szPath[MAX_PATH];

		GetCurrentDirectory (MAX_PATH, szPath);

		ConOutPuts (szPath);


		return 0;
	}

	if (dir && _tcslen (dir) == 1 && *dir == _T('-'))
	{
		if (lpLastPath)
			dir = lpLastPath;
		else
			return 0;
	}
	else if (dir && _tcslen (dir)==2 && dir[1] == _T(':'))
	{
		TCHAR szRoot[3] = _T("A:");
		TCHAR szPath[MAX_PATH];

		szRoot[0] = _totupper (dir[0]);
		GetFullPathName (szRoot, MAX_PATH, szPath, NULL);

		/* PathRemoveBackslash */
		if (_tcslen (szPath) > 3)
		{
			LPTSTR p = _tcsrchr (szPath, _T('\\'));
			*p = _T('\0');
		}

		ConOutPuts (szPath);


		return 0;
	}

	/* remove trailing \ if any, but ONLY if dir is not the root dir */
	if (_tcslen (dir) > 3 && dir[_tcslen (dir) - 1] == _T('\\'))
		dir[_tcslen(dir) - 1] = _T('\0');


	/* store current directory */
	lpOldPath = (LPTSTR)malloc (MAX_PATH * sizeof(TCHAR));
	GetCurrentDirectory (MAX_PATH, lpOldPath);

	if (!SetCurrentDirectory (dir))
	{
		ErrorMessage (GetLastError(), _T("CD"));

		/* throw away current directory */
		free (lpOldPath);
		lpOldPath = NULL;

		return 1;
	}
	else
	{
		GetCurrentDirectory(MAX_PATH, dir);
		if (dir[0]!=lpOldPath[0])
		{
			SetCurrentDirectory(lpOldPath);
			free(lpOldPath);
		}
		else
		{
			if (lpLastPath)
				free (lpLastPath);
			lpLastPath = lpOldPath;
		}
	}


	return 0;
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
		ConOutPuts (_T("Creates a directory.\n\n"
					   "MKDIR [drive:]path\nMD [drive:]path"));
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
		p = split (param, &argc);
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
		ConErrPrintf (_T("Required parameter missing\n"));
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
		ConOutPuts (_T("Removes a directory.\n\n"
					   "RMDIR [drive:]path\nRD [drive:]path"));
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
		p = split (param, &argc);
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
		ConErrPrintf (_T("Required parameter missing\n"));
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
	{
		ConOutPuts (_T("Exits the command line interpreter.\n\nEXIT"));
		return 0;
	}

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
		ConOutPuts (_T("Starts a comment line in a batch file.\n\n"
		               "REM [Comment]"));
	}

	return 0;
}
#endif /* INCLUDE_CMD_REM */


INT CommandShowCommands (LPTSTR cmd, LPTSTR param)
{
	PrintCommandList ();
	return 0;
}

/* EOF */