/*
 *  CHCP.C - chcp internal command.
 *
 *
 *  History:
 *
 *    23-Dec-1998 (Eric Kohl)
 *        Started.
 *
 *    02-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>



#ifdef INCLUDE_CMD_CHCP

INT CommandChcp (LPTSTR param)
{
	LPTSTR *arg;
	INT    args;
	UINT uNewCodePage;

	/* print help */
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_CHCP_HELP);
		return 0;
	}

	nErrorLevel = 0;

	/* get parameters */
	arg = split (param, &args, FALSE);

	if (args == 0)
	{
		/* display active code page number */
		ConErrResPrintf(STRING_CHCP_ERROR1, InputCodePage);
		freep (arg);
		return 0;
	}

	if (args >= 2)
	{
		/* too many parameters */
		ConErrResPrintf(STRING_ERROR_INVALID_PARAM_FORMAT, param);
		nErrorLevel = 1;
		freep (arg);
		return 1;
	}

	uNewCodePage = (UINT)_ttoi(arg[0]);

	if (uNewCodePage == 0)
	{
		ConErrResPrintf(STRING_ERROR_INVALID_PARAM_FORMAT, arg[0]);
		freep (arg);
		nErrorLevel = 1;
		return 1;
	}

	if (!SetConsoleCP(uNewCodePage))
	{
		ConErrResPuts(STRING_CHCP_ERROR4);
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
