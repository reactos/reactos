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
 */

#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_DEL


enum
{
	DEL_ATTRIBUTES = 0x001,   /* /A : not implemented */
	DEL_ERROR      = 0x002,   /* /E : not implemented */
	DEL_NOTHING    = 0x004,   /* /N */
	DEL_PROMPT     = 0x008,   /* /P */
	DEL_QUIET      = 0x010,   /* /Q */
	DEL_SUBDIR     = 0x020,   /* /S : not implemented */
	DEL_TOTAL      = 0x040,   /* /T */
	DEL_WIPE       = 0x080,   /* /W */
	DEL_EMPTYDIR   = 0x100,   /* /X : not implemented */
	DEL_YES        = 0x200,   /* /Y */
	DEL_ZAP        = 0x400    /* /Z */
};



static BOOL
RemoveFile (LPTSTR lpFileName, DWORD dwFlags)
{
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

		LoadString( CMD_ModuleHandle, STRING_DELETE_WIPE, (LPTSTR) szMsg,sizeof(szMsg));

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

	return DeleteFile (lpFileName);
}


INT CommandDelete (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR szFullPath[MAX_PATH];
	LPTSTR pFilePart;
	LPTSTR *arg = NULL;
	INT args;
	INT i;
	INT res;
	INT   nEvalArgs = 0; /* nunber of evaluated arguments */
	DWORD dwFlags = 0;
	DWORD dwFiles = 0;
	HANDLE hFile;
	WIN32_FIND_DATA f;
	LONG ch;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPuts(STRING_DEL_HELP1);
		return 0;
	}

	arg = split (param, &args, FALSE);

	if (args > 0)
	{
		/* check for options anywhere in command line */
		for (i = 0; i < args; i++)
		{
			if (*arg[i] == _T('/'))
			{
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
					else if (ch == _T('Z'))
					{
						dwFlags |= DEL_ZAP;
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

		/* keep quiet within batch files */
		if (bc != NULL)
			dwFlags |= DEL_QUIET;

		/* check for filenames anywhere in command line */
		for (i = 0; i < args; i++)
		{
			if (!_tcscmp (arg[i], _T("*")) ||
			    !_tcscmp (arg[i], _T("*.*")))
			{
				if (!((dwFlags & DEL_YES) || (dwFlags & DEL_QUIET) || (dwFlags & DEL_PROMPT)))
				{
					LoadString( CMD_ModuleHandle, STRING_DEL_HELP2, szMsg, RC_STRING_MAX_SIZE);

					res = FilePromptYN (szMsg);
					if ((res == PROMPT_NO) || (res == PROMPT_BREAK))
						break;
				}
			}

			if (*arg[i] != _T('/'))
			{
#ifdef _DEBUG
				ConErrPrintf (_T("File: %s\n"), arg[i]);
#endif

				if (_tcschr (arg[i], _T('*')) || _tcschr (arg[i], _T('?')))
				{
					/* wildcards in filespec */
#ifdef _DEBUG
					ConErrPrintf(_T("Wildcards!\n\n"));
#endif

					GetFullPathName (arg[i],
					                 MAX_PATH,
					                 szFullPath,
					                 &pFilePart);

#ifdef _DEBUG
					ConErrPrintf(_T("Full path: %s\n"), szFullPath);
					ConErrPrintf(_T("File part: %s\n"), pFilePart);
#endif

					hFile = FindFirstFile (szFullPath, &f);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						error_file_not_found ();
						freep (arg);
						return 0;
					}

					do
					{
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

						if (!(dwFlags & DEL_QUIET) && !(dwFlags & DEL_TOTAL))
						{
							LoadString(CMD_ModuleHandle, STRING_DEL_ERROR7, szMsg, RC_STRING_MAX_SIZE);
							ConErrPrintf(szMsg, szFullPath);
						}

						/* delete the file */
						if (!(dwFlags & DEL_NOTHING))
						{
							if (RemoveFile (szFullPath, dwFlags))
							{
								dwFiles++;
							}
							else
							{
								if (dwFlags & DEL_ZAP)
								{
									if (SetFileAttributes (szFullPath, 0))
									{
										if (RemoveFile (szFullPath, dwFlags))
										{
											dwFiles++;
										}
										else
										{
											ErrorMessage (GetLastError(), _T(""));
										}
									}
									else
									{
										ErrorMessage (GetLastError(), _T(""));
									}
								}
								else
								{
									ErrorMessage (GetLastError(), _T(""));
								}
							}
						}
					}
					while (FindNextFile (hFile, &f));
					FindClose (hFile);
				}
				else
				{
					/* no wildcards in filespec */
#ifdef _DEBUG
					ConErrPrintf(_T("No Wildcards!\n"));
#endif
					GetFullPathName (arg[i],
					                 MAX_PATH,
					                 szFullPath,
					                 &pFilePart);

					/* ask for deleting */
					// Don't ask if the file doesn't exist, the following code will make the error-msg
					if((dwFlags & DEL_PROMPT) && (FindFirstFile(szFullPath, &f) != INVALID_HANDLE_VALUE))
					{
						LoadString(CMD_ModuleHandle, STRING_DEL_ERROR5, szMsg, RC_STRING_MAX_SIZE);
						ConErrPrintf(szMsg, szFullPath);

						LoadString(CMD_ModuleHandle, STRING_DEL_ERROR6, szMsg, RC_STRING_MAX_SIZE);
						res = FilePromptYN (szMsg);

						if ((res == PROMPT_NO) || (res == PROMPT_BREAK))
						{
							break;   //FIXME: Errorcode?
						}
					}

#ifdef _DEBUG
					ConErrPrintf(_T("Full path: %s\n"), szFullPath);
#endif
					if (!(dwFlags & DEL_QUIET) && !(dwFlags & DEL_TOTAL))
					{
						LoadString(CMD_ModuleHandle, STRING_DEL_ERROR7, szMsg, RC_STRING_MAX_SIZE);
						ConErrPrintf(szMsg, szFullPath);
					}

					if (!(dwFlags & DEL_NOTHING))
					{
						if (RemoveFile (szFullPath, dwFlags))
						{
							dwFiles++;
						}
						else
						{
							if (dwFlags & DEL_ZAP)
							{
								if (SetFileAttributes (szFullPath, 0))
								{
									if (RemoveFile (szFullPath, dwFlags))
									{
										dwFiles++;
									}
									else
									{
										ErrorMessage (GetLastError(), _T(""));
									}
								}
								else
								{
									ErrorMessage (GetLastError(), _T(""));
								}
							}
							else
							{
								ErrorMessage (GetLastError(), _T(""));
							}
						}
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

	if (!(dwFlags & DEL_QUIET))
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
