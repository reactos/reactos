/*
 *  GOTO.C - goto internal batch command.
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
 *    28 Jul 1998 (Hans B Pufal) [HBP_003]
 *        Terminate label on first space character, use only first 8 chars of
 *        label string
 *
 *    24-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 *
 *    27-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("/?").
 *
 *    28-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc  
 */

#include "precomp.h"
#include "resource.h"


/*
 * Perform GOTO command.
 *
 * Only valid if batch file current.
 *
 */

INT cmd_goto (LPTSTR cmd, LPTSTR param)
{
	LPTSTR tmp;
	LONG   lNewPosHigh;
	WCHAR szMsg[RC_STRING_MAX_SIZE];

#ifdef _DEBUG
	DebugPrintf (_T("cmd_goto (\'%s\', \'%s\'\n"), cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		LoadString( GetModuleHandle(NULL), STRING_GOTO_HELP1, (LPTSTR) szMsg,sizeof(szMsg));
        ConOutPuts ((LPTSTR)szMsg);	

		return 0;
	}

	/* if not in batch -- error!! */
	if (bc == NULL)
	{
		return 1;
	}

	if (*param == _T('\0'))
	{
		
		LoadString( GetModuleHandle(NULL), STRING_GOTO_ERROR1, (LPTSTR) szMsg,sizeof(szMsg));
		ExitBatch ((LPTSTR)szMsg);
		return 1;
	}

	/* terminate label at first space char */
	tmp = param;
	while (*tmp && !_istspace (*tmp))
		tmp++;
	*tmp = _T('\0');

	/* set file pointer to the beginning of the batch file */
	lNewPosHigh = 0;
	SetFilePointer (bc->hBatchFile, 0, &lNewPosHigh, FILE_BEGIN);

	while (FileGetString (bc->hBatchFile, textline, sizeof(textline)))
	{
		/* Strip out any trailing spaces or control chars */
		tmp = textline + _tcslen (textline) - 1;
		while (_istcntrl (*tmp) || _istspace (*tmp))
			tmp--;
		*(tmp + 1) = _T('\0');

		/* Then leading spaces... */
		tmp = textline;
		while (_istspace (*tmp))
			tmp++;

		/* use only 1st 8 chars of label */
		if ((*tmp == _T(':')) && (_tcsncmp (++tmp, param, 8) == 0))
			return 0;
	}

	LoadString( GetModuleHandle(NULL), STRING_GOTO_ERROR2, (LPTSTR) szMsg,sizeof(szMsg));
    ConErrPrintf ((LPTSTR)szMsg,param);		
	ExitBatch (NULL);

	return 1;
}
