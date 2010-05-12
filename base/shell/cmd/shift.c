/*
 *  SHIFT.C - shift internal batch command
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        started.
 *
 *    16 Jul 1998 (John P Price)
 *        Separated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    07-Jan-1999 (Eric Kohl)
 *        Added help text ("shift /?") and cleaned up.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>


/*
 *  Perform the SHIFT command.
 *
 * Only valid inside batch files.
 *
 * FREEDOS extension : optional parameter DOWN to allow shifting
 *   parameters backwards.
 *
 */

INT cmd_shift (LPTSTR param)
{
	INT i = 0;
	TRACE ("cmd_shift: (\'%s\')\n", debugstr_aw(param));

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_SHIFT_HELP);
		return 0;
	}

	nErrorLevel = 0;

	if (bc == NULL)
	{
		/* not in batch - error!! */
		nErrorLevel = 1;
		return 1;
	}

	if (!_tcsicmp (param, _T("down")))
	{
		if (bc->shiftlevel[0])
			for (; i <= 9; i++)
				bc->shiftlevel[i]--;
	}
	else /* shift up */
	{
		if (*param == _T('/'))
		{
			if (param[1] < '0' || param[1] > '9')
			{
				error_invalid_switch(param[1]);
				return 1;
			}
			i = param[1] - '0';
		}

		for (; i < 9; i++)
			bc->shiftlevel[i] = bc->shiftlevel[i + 1];
		bc->shiftlevel[i]++;
	}

	return 0;
}

/* EOF */
