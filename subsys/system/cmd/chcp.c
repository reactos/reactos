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
	LPTSTR *arg;
	INT    args;
	UINT uOldCodePage;
	UINT uNewCodePage;
	WCHAR szMsg[RC_STRING_MAX_SIZE];

	/* print help */
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		LoadString( GetModuleHandle(NULL), STRING_CHCP_HELP, (LPTSTR) szMsg,sizeof(szMsg));
        ConOutPuts (_T((LPTSTR)szMsg));
		return 0;
	}

	/* get parameters */
	arg = split (param, &args, FALSE);

	if (args == 0)
	{
		/* display active code page number */
		LoadString( GetModuleHandle(NULL), STRING_CHCP_ERROR1, (LPTSTR) szMsg,sizeof(szMsg));
        ConErrPrintf (_T((LPTSTR)szMsg), GetConsoleCP ());

		return 0;
	}

	if (args >= 2)
	{
		/* too many parameters */
		LoadString( GetModuleHandle(NULL), STRING_CHCP_ERROR2, (LPTSTR) szMsg,sizeof(szMsg));
        ConErrPrintf (_T((LPTSTR)szMsg), param);

		return 1;
	}


	/* save old code page */
	uOldCodePage = GetConsoleCP ();

	uNewCodePage = (UINT)_ttoi (arg[0]);

	if (uNewCodePage == 0)
	{
		LoadString( GetModuleHandle(NULL), STRING_CHCP_ERROR3, (LPTSTR) szMsg,sizeof(szMsg));
        ConErrPrintf (_T((LPTSTR)szMsg), arg[0]);
		freep (arg);
		return 1;
	}

	if (!SetConsoleCP (uNewCodePage))
	{
		LoadString( GetModuleHandle(NULL), STRING_CHCP_ERROR4, (LPTSTR) szMsg,sizeof(szMsg));
        ConErrPrintf (_T((LPTSTR)szMsg));		
	}
	else
	{
		SetConsoleOutputCP (uNewCodePage);
		InitLocale ();
	}

	freep (arg);

	return 0;
}

#endif /* INCLUDE_CMD_CHCP */
