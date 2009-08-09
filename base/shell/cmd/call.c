/*
 *  CALL.C - call internal batch command.
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        started.
 *
 *    16 Jul 1998 (John P Price)
 *        Seperated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    04-Aug-1998 (Hans B Pufal)
 *        added lines to initialize for pointers (HBP004)  This fixed the
 *        lock-up that happened sometimes when calling a batch file from
 *        another batch file.
 *
 *    07-Jan-1999 (Eric Kohl)
 *        Added help text ("call /?") and cleaned up.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    02-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>


/*
 * Perform CALL command.
 */

INT cmd_call (LPTSTR param)
{
	TCHAR line[CMDLINE_LENGTH];

	TRACE ("cmd_call: (\'%s\')\n", debugstr_aw(param));
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_CALL_HELP);
		return 0;
	}

	/* Do a second round of %-variable substitutions */
	if (!SubstituteVars(param, line, _T('%')))
		return nErrorLevel = 1;

	param = line;
	while (_istspace(*param))
		param++;
	if (*param == _T(':') && (bc))
	{
		/* CALL :label - call a subroutine of the current batch file */
		TCHAR *first = param;
		while (*param && !_istspace(*param))
			param++;
		if (*param)
		{
			/* Separate label and arguments */
			*param++ = _T('\0');
			while (_istspace(*param))
				param++;
		}
		return !Batch(bc->BatchFilePath, first, param, NULL);
	}

	return !DoCommand(param, NULL);
}

/* EOF */
