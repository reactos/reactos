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
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"
#include "batch.h"


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

#ifdef _DEBUG
	DebugPrintf ("cmd_goto (\'%s\', \'%s\'\n", cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Directs CMD to a labeled line in a batch script.\n"
					   "\n"
					   "GOTO label\n"
					   "\n"
					   "  label  Specifies a text string used in a batch script as a label.\n"
					   "\n"
					   "You type a label on a line by itself, beginning with a colon."));
		return 0;
	}

	/* if not in batch -- error!! */
	if (bc == NULL)
	{
		return 1;
	}

	if (*param == _T('\0'))
	{
		ExitBatch (_T("No label specified for GOTO\n"));
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

	ConErrPrintf (_T("Label '%s' not found\n"), param);
	ExitBatch (NULL);

	return 1;
}
