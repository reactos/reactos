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
 *
 *    19-Jan-1999 (Paolo Pantaleo <paolopan@freemail.it>)
 *        Added multiple file support (copied from y.c)
 */

#include "config.h"

#ifdef INCLUDE_CMD_TYPE

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"


INT cmd_type (LPTSTR cmd, LPTSTR param)
{
	TCHAR  buff[256];
	HANDLE hFile, hConsoleOut;
	DWORD  dwRead;
	DWORD  dwWritten;
	BOOL   bRet;
	INT    argc,i;
	LPTSTR *argv;
	LPTSTR errmsg;
	
	hConsoleOut=GetStdHandle (STD_OUTPUT_HANDLE);

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

	argv = split (param, &argc, TRUE);
	
	for (i = 0; i < argc; i++)
	{
		if (_T('/') == argv[i][0])
		{
			ConErrPrintf("Invalid option \"%s\"\n", argv[i] + 1);
			continue;
		}
		hFile = CreateFile(argv[i],
			GENERIC_READ,
			FILE_SHARE_READ,NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,NULL);

		if(hFile == INVALID_HANDLE_VALUE)
		{
			FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
			               FORMAT_MESSAGE_IGNORE_INSERTS |
			               FORMAT_MESSAGE_FROM_SYSTEM,
			               NULL,
			               GetLastError(),
			               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			               (LPTSTR) &errmsg,
			               0,
			               NULL);
			ConErrPrintf ("%s - %s", argv[i], errmsg);
			LocalFree (errmsg);
			continue;
		}

		do
		{
			bRet = ReadFile(hFile,buff,sizeof(buff),&dwRead,NULL);

			if (dwRead>0 && bRet)
				WriteFile(hConsoleOut,buff,dwRead,&dwWritten,NULL);
			
		} while(dwRead>0 && bRet);

		CloseHandle(hFile);
	}	
	
	freep (argv);

	return 0;
}

#endif
