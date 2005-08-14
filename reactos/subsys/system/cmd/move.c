/*
*  MOVE.C - move internal command.
*
*
*  History:
*
*    14-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
*        Started.
*
*    18-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
*        Unicode safe!
*        Preliminary version!!!
*
*    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
*        Redirection safe!
*
*    27-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
*        Added help text ("/?").
*        Added more error checks.
*
*    03-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
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
#include "resource.h"

#ifdef INCLUDE_CMD_MOVE


enum
{
	MOVE_NOTHING  = 0x001,   /* /N  */
	MOVE_OVER_YES = 0x002,   /* /Y  */
	MOVE_OVER_NO  = 0x004,   /* /-Y */
};

static INT Overwrite (LPTSTR fn)
{
	/*ask the user if they want to override*/
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	INT res;
	LoadString(CMD_ModuleHandle, STRING_MOVE_HELP1, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg,fn);
	res = FilePromptYNA ("");
	return res;
}



INT cmd_move (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	LPTSTR *arg;
	INT argc, i, nFiles;
	TCHAR szDestPath[MAX_PATH];
	TCHAR szSrcPath[MAX_PATH];
	DWORD dwFlags = 0;
	INT nOverwrite = 0;
	WIN32_FIND_DATA findBuffer;
	HANDLE hFile;
	LPTSTR pszFile;


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

	if (_tcschr (arg[argc - 1], _T('*')) != NULL)
	{
		/*'*' in dest, this doesnt happen.  give folder name instead*/
		error_parameter_format('2');
		return 1;
	}

	/* get destination */
	GetFullPathName (arg[argc - 1], MAX_PATH, szDestPath, NULL);
#ifdef _DEBUG
	DebugPrintf (_T("Destination: %s\n"), szDestPath);
#endif

	/* move it */
	for (i = 0; i < argc - 1; i++)
	{
		if (*arg[i] == _T('/'))
			continue;

		hFile = FindFirstFile (arg[i], &findBuffer);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			ErrorMessage (GetLastError (), arg[i]);
			freep (arg);
			return 1;
		}

		do
		{

			nOverwrite = 1;
			GetFullPathName (findBuffer.cFileName, MAX_PATH, szSrcPath, &pszFile);

			if (GetFileAttributes (szSrcPath) & FILE_ATTRIBUTE_DIRECTORY)
			{
				/* source is directory */

#ifdef _DEBUG
				DebugPrintf (_T("Move directory \'%s\' to \'%s\'\n"),
					szSrcPath, szDestPath);
#endif
				if (!(dwFlags & MOVE_NOTHING))
					continue;
				MoveFileEx (szSrcPath, szDestPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED);
			}
			else
			{
				/* source is file */				
				if (GetFileAttributes (szDestPath) & FILE_ATTRIBUTE_DIRECTORY)
				{
					/* destination is existing directory */

					/*build the dest string(accounts for *)*/
					TCHAR szFullDestPath[MAX_PATH];
					_tcscpy (szFullDestPath, szDestPath);
					/*check to see if there is an ending slash, if not add one*/
					if(szFullDestPath[_tcslen(szFullDestPath) -  1] != _T('\\'))
					   _tcscat (szFullDestPath, _T("\\"));

					_tcscat (szFullDestPath, findBuffer.cFileName);

					/*checks to make sure user wanted/wants the override*/
					if((dwFlags & MOVE_OVER_NO) && IsExistingFile (szFullDestPath))
						continue;
					if(!(dwFlags & MOVE_OVER_YES) && IsExistingFile (szFullDestPath))
						nOverwrite = Overwrite (szFullDestPath);
					if (nOverwrite == PROMPT_NO || nOverwrite == PROMPT_BREAK)
						continue;
					if (nOverwrite == PROMPT_ALL)
						dwFlags |= MOVE_OVER_YES;

					/*delete the file that might be there first*/
					DeleteFile(szFullDestPath);
					ConOutPrintf (_T("%s => %s"), szSrcPath, szFullDestPath);

					if ((dwFlags & MOVE_NOTHING))
						continue;

					/*delete the file that might be there first*/
					DeleteFile(szFullDestPath);
					/*move the file*/
					if (MoveFileEx (szSrcPath, szFullDestPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED))
						LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
					else
						LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR2, szMsg, RC_STRING_MAX_SIZE);

					ConOutPrintf(szMsg);
				}
				else
				{
					/* destination is a file */

					if (_tcschr (arg[argc - 2], _T('*')) != NULL)
					{
						/*'*' in src but there should't be one in the dest*/
						error_parameter_format('1');
						return 1;
					}

					/*bunch of checks to see if we the user wanted/wants
					to really override the files*/
					if((dwFlags & MOVE_OVER_NO) && IsExistingFile (szDestPath))
						continue;
					if(!(dwFlags & MOVE_OVER_YES) && IsExistingFile (szDestPath))
						nOverwrite = Overwrite (szDestPath);
					if (nOverwrite == PROMPT_NO || nOverwrite == PROMPT_BREAK)
						continue;
					if (nOverwrite == PROMPT_ALL)
						dwFlags |= MOVE_OVER_YES;

					ConOutPrintf (_T("%s => %s"), szSrcPath, szDestPath);

					if ((dwFlags & MOVE_NOTHING))
						continue;

					/*delete the file first just to get ride of it
					if it was already there*/
					DeleteFile(szDestPath);
					/*do the moving*/
					if (MoveFileEx (szSrcPath, szDestPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED))
						LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
					else
						LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR2, szMsg, RC_STRING_MAX_SIZE);

					ConOutPrintf(szMsg);

				}
			}
		}
		while (FindNextFile (hFile, &findBuffer));
		FindClose (hFile);
	}

	freep (arg);
	return 0;
}

#endif /* INCLUDE_CMD_MOVE */
