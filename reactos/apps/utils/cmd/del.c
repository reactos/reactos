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
 *    03-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
 *        First working version.
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#ifdef INCLUDE_CMD_DEL

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


#define PROMPT_NO		0
#define PROMPT_YES		1
#define PROMPT_ALL		2
#define PROMPT_BREAK	3


static BOOL ConfirmDeleteAll (VOID)
{
	TCHAR inp[10];
	LPTSTR p;

	ConOutPrintf ("All files in directory will be deleted!\n"
				  "Are you sure (Y/N)? ");
	ConInString (inp, 10);

	_tcsupr (inp);
	for (p = inp; _istspace (*p); p++)
		;

	if (*p == _T('Y'))
		return PROMPT_YES;
	else if (*p == _T('N'))
		return PROMPT_NO;

#if 0
	if (*p == _T('A'))
		return PROMPT_ALL;
	else if (*p == _T('\03'))
		return PROMPT_BREAK;
#endif

	return PROMPT_NO;
}


static INT Prompt (LPTSTR str)
{
	TCHAR inp[10];
	LPTSTR p;

	ConOutPrintf (_T("Delete %s (Yes/No)? "), str);
	ConInString (inp, 10);

	_tcsupr (inp);
	for (p = inp; _istspace (*p); p++)
		;

	if (*p == _T('Y'))
		return PROMPT_YES;
	else if (*p == _T('N'))
		return PROMPT_NO;

#if 0
	if (*p == _T('A'))
		return PROMPT_ALL;
	else if (*p == _T('\03'))
		return PROMPT_BREAK;
#endif
}



INT cmd_del (LPTSTR cmd, LPTSTR param)
{
	LPTSTR *arg = NULL;
	INT args;
	INT i;
	INT  nEvalArgs = 0; /* nunber of evaluated arguments */
	BOOL bNothing = FALSE;
	BOOL bQuiet   = FALSE;
	BOOL bPrompt  = FALSE;

	HANDLE hFile;
	WIN32_FIND_DATA f;
//	DWORD dwAttributes;


	if (!_tcsncmp (param, _T("/?"), 2))
	{
/*
		ConOutPuts (_T("Deletes one or more files.\n\n"
			"DEL [drive:][path]filename [/P]\n"
			"DELETE [drive:][path]filename [/P]\n"
			"ERASE [drive:][path]filename [/P]\n\n"
			"  [drive:][path]filename  Specifies the file(s) to delete.  Specify multiple\n"
			"                          files by using wildcards.\n"
			"  /P        Prompts for confirmation before deleting each file."));
*/

		ConOutPuts (_T("Deletes one or more files.\n"
					   "\n"
					   "DEL [/N /P /Q] file ...\n"
					   "DELETE [/N /P /Q] file ...\n"
					   "ERASE [/N /P /Q] file ...\n"
					   "\n"
					   "  file  Specifies the file(s) to delete.\n"
					   "\n"
					   "  /N    Nothing.\n"
					   "  /P    Prompts for confirmation before deleting each file.\n"
					   "        (Not implemented yet!)\n"
					   "  /Q    Quiet."
					   ));

		return 0;
	}

	arg = split (param, &args);

	if (args > 0)
	{
		/* check for options anywhere in command line */
		for (i = 0; i < args; i++)
		{
			if (*arg[i] == _T('/'))
			{
				if (_tcslen (arg[i]) >= 2)
				{
					switch (_totupper (arg[i][1]))
					{
						case _T('N'):
							bNothing = TRUE;
							break;

						case _T('P'):
							bPrompt = TRUE;
							break;

						case _T('Q'):
							bQuiet = TRUE;
							break;
					}

				}

				nEvalArgs++;
			}
		}

		/* there are only options on the command line --> error!!! */
		if (args == nEvalArgs)
		{
			error_req_param_missing ();
			freep (arg);
			return 1;
		}

		/* check for filenames anywhere in command line */
		for (i = 0; i < args; i++)
		{
			if (!_tcscmp (arg[i], _T("*")) ||
				!_tcscmp (arg[i], _T("*.*")))
			{
				if (!ConfirmDeleteAll ())
					break;

			}

			if (*arg[i] != _T('/'))
			{
#ifdef _DEBUG
				ConErrPrintf ("File: %s\n", arg[i]);
#endif

				if (_tcschr (arg[i], _T('*')) || _tcschr (arg[i], _T('?')))
				{
					/* wildcards in filespec */
#ifdef _DEBUG
					ConErrPrintf ("Wildcards!\n\n");
#endif

					hFile = FindFirstFile (arg[i], &f);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						error_file_not_found ();
						freep (arg);
						return 0;
					}

					do
					{
						if (!_tcscmp (f.cFileName, _T(".")) ||
							!_tcscmp (f.cFileName, _T("..")))
							continue;

#ifdef _DEBUG
						ConErrPrintf ("Delete file: %s\n", f.cFileName);
#endif

						if (!bNothing)
						{
							if (!DeleteFile (f.cFileName))
								ErrorMessage (GetLastError(), _T(""));
						}


					}
					while (FindNextFile (hFile, &f));
					FindClose (hFile);
				}
				else
				{
					/* no wildcards in filespec */
#ifdef _DEBUG
					ConErrPrintf ("No Wildcards!\n");
					ConErrPrintf ("Delete file: %s\n", arg[i]);
#endif

					if (!bNothing)
					{
						if (!DeleteFile (arg[i]))
							ErrorMessage (GetLastError(), _T(""));
					}
				}
			}
		}
	}
	else
	{
		/* only command given */
		error_req_param_missing ();
		freep (arg);
		return 1;
	}

	freep (arg);

	return 0;
}

#endif
