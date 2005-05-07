/*
 *  CHCP.C - chcp internal command.
 *
 *
 *  History:
 *
 *    23-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 *
 *    02-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc  
 */

#include "precomp.h"
#include "resource.h"



#ifdef INCLUDE_CMD_CHCP

INT CommandChcp (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	LPTSTR *arg;
	INT    args;
	UINT uOldCodePage;
	UINT uNewCodePage;

	/* print help */
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		LoadString(GetModuleHandle(NULL), STRING_CHCP_HELP, szMsg, RC_STRING_MAX_SIZE);
		ConOutPuts(szMsg);
		return 0;
	}

	/* get parameters */
	arg = split (param, &args, FALSE);

	if (args == 0)
	{
		/* display active code page number */
		LoadString(GetModuleHandle(NULL), STRING_CHCP_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(szMsg, GetCodePage);
		return 0;
	}

	if (args >= 2)
	{
		/* too many parameters */
		LoadString(GetModuleHandle(NULL), STRING_CHCP_ERROR2, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(szMsg, param);
		return 1;
	}


	/* save old code page */
	uOldCodePage = GetCodePage;

	uNewCodePage = (UINT)_ttoi(arg[0]);

	if (uNewCodePage == 0)
	{
		LoadString(GetModuleHandle(NULL), STRING_CHCP_ERROR3, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(szMsg, arg[0]);
		freep (arg);
		return 1;
	}

	if (!SetConsoleCP(uNewCodePage))
	{
		LoadString(GetModuleHandle(NULL), STRING_CHCP_ERROR4, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(szMsg);
	}
	else
	{
		
		SetConsoleOutputCP (uNewCodePage);
		InitLocale ();
		InputCodePage= GetConsoleCP();
	}

	freep (arg);

	return 0;
}

#endif /* INCLUDE_CMD_CHCP */
