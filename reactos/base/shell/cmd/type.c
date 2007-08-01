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
 *    07-Jan-1999 (Eric Kohl)
 *        Added support for quoted arguments (type "test file.dat").
 *        Cleaned up.
 *
 *    19-Jan-1999 (Eric Kohl)
 *        Unicode and redirection ready!
 *
 *    19-Jan-1999 (Paolo Pantaleo <paolopan@freemail.it>)
 *        Added multiple file support (copied from y.c)
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_TYPE


INT cmd_type (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR  buff[256];
	HANDLE hFile, hConsoleOut;
	BOOL   bRet;
	INT    argc,i;
	LPTSTR *argv;
	LPTSTR errmsg;
	BOOL bPaging = FALSE;
	BOOL bFirstTime = TRUE;

	hConsoleOut=GetStdHandle (STD_OUTPUT_HANDLE);

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_TYPE_HELP1);
		return 0;
	}	

	if (!*param)
	{
		error_req_param_missing ();
		return 1;
	}

	argv = split (param, &argc, TRUE);

	for(i = 0; i < argc; i++)
	{
		if(*argv[i] == _T('/') && _tcslen(argv[i]) >= 2 && _totupper(argv[i][1]) == _T('P'))
		{
			bPaging = TRUE;
		}
	}

	for (i = 0; i < argc; i++)
	{
		if (_T('/') == argv[i][0] && _totupper(argv[i][1]) != _T('P'))
		{
			LoadString(CMD_ModuleHandle, STRING_TYPE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, argv[i] + 1);
			continue;
		}

    	nErrorLevel = 0;

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
			ConErrPrintf (_T("%s - %s"), argv[i], errmsg);
			LocalFree (errmsg);
			nErrorLevel = 1;
			continue;
		}
		
		do
		{
			bRet = FileGetString (hFile, buff, sizeof(buff) / sizeof(TCHAR));
			if(bPaging)
			{
				if(bRet)
				{
					if (ConOutPrintfPaging(bFirstTime, buff) == 1)
					{
						bCtrlBreak = FALSE;
						return 0;
					}
				}
			}
			else
			{				
				if(bRet)
					ConOutPrintf(buff);
			}
			bFirstTime = FALSE;

		} while(bRet);

		CloseHandle(hFile);
	}

	freep (argv);

	return 0;
}

#endif
