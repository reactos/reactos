/*
 *  TYPE.C - type internal command.
 *
 *  History:
 *
 *    07/08/1998 (John P. Price)
 *        started.
 *
 *    07/12/98 (Rob Lake)
 *        Changed error messages
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    07-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added support for quoted arguments (type "test file.dat").
 *        Cleaned up.
 *
 *    19-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection ready!
 */

#include "config.h"

#ifdef INCLUDE_CMD_TYPE

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"


INT cmd_type (LPTSTR cmd, LPTSTR param)
{
	TCHAR  szBuffer[256];
	HANDLE hFile;
	DWORD  dwBytesRead;
	DWORD  dwBytesWritten;
	BOOL   bResult;
	INT    args;
	LPTSTR *arg;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Displays the contents of text files.\n\n"
					   "TYPE [drive:][path]filename"));
		return 0;
	}

	if (!*param)
	{
		error_req_param_missing ();
		return 1;
	}

	arg = split (param, &args);

	if (args > 1)
	{
		error_too_many_parameters (_T("\b \b"));
		freep (arg);
		return 1;
	}

	hFile = CreateFile (arg[0], GENERIC_READ, FILE_SHARE_READ,
						NULL, OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
						NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		error_sfile_not_found (param);
		freep (arg);
		return 1;
	}

	do
	{
		bResult = ReadFile (hFile, szBuffer, sizeof(szBuffer),
							&dwBytesRead, NULL);
		if (dwBytesRead)
			WriteFile (GetStdHandle (STD_OUTPUT_HANDLE), szBuffer, dwBytesRead,
					   &dwBytesWritten, NULL);
	}
	while (bResult && dwBytesRead > 0);

	CloseHandle (hFile);
	freep (arg);

	return 0;
}

#endif
