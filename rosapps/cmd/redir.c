/*
 *  REDIR.C - redirection handling.
 *
 *
 *  History:
 *
 *    12/15/95 (Tim Norman)
 *        started.
 *
 *    12 Jul 98 (Hans B Pufal)
 *        Rewrote to make more efficient and to conform to new command.c
 *        and batch.c processing.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Added config.h include
 *
 *    22-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode safe!
 *        Added new error redirection "2>" and "2>>".
 *
 *    26-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added new error AND output redirection "&>" and "&>>".
 */

#include "config.h"

#ifdef FEATURE_REDIRECTION

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"


static BOOL
IsRedirection (TCHAR c)
{
	return (c == _T('<')) || (c == _T('>')) || (c == _T('|'));
}


/*
 * Gets the redirection info from the command line and copies the
 * file names into ifn, ofn and efn removing them from the command
 * line.
 *
 * Converts remaining command line into a series of null terminated
 * strings defined by the pipe char '|'. Each string corresponds
 * to a single executable command. A double null terminates the
 * command strings.
 *
 * Return number of command strings found.
 *
 */

INT GetRedirection (LPTSTR s, LPTSTR ifn, LPTSTR ofn, LPTSTR efn, LPINT lpnFlags)
{
	INT num = 1;
	LPTSTR dp = s;
	LPTSTR sp = s;

	/* find and remove all the redirections first */
	while (*sp)
	{
		if ((*sp == _T('"')) || (*sp == _T('\'')))
		{
			/* No redirects inside quotes */
			TCHAR qc = *sp;

			do
				*dp++ = *sp++;
			while (*sp && *sp != qc);

			*dp++ = *sp++;
		}
		else if ((*sp == _T('<')) || (*sp == _T('>')) ||
				 (*sp == _T('2')) || (*sp == _T('&')))
		{
			/* MS-DOS ignores multiple redirection symbols and uses the last */
			/* redirection, so we'll emulate that and not check */

			if (*sp == _T('<'))
			{
				/* input redirection */
				*lpnFlags |= INPUT_REDIRECTION;
				while (_istspace (*sp))
					sp++;

				/* copy file name */
				while (*sp && !IsRedirection (*sp) && !_istspace (*sp))
					*ifn++ = *sp++;
				*ifn = _T('\0');
			}
			else if (*sp == _T('>'))
			{
				/* output redirection */
				*lpnFlags |= OUTPUT_REDIRECTION;
				sp++;

				/* append request ? */
				if (*sp == _T('>'))
				{
					*lpnFlags |= OUTPUT_APPEND;
					sp++;
				}

				while (_istspace (*sp))
					sp++;

				/* copy file name */
				while (*sp && !IsRedirection (*sp) && !_istspace (*sp))
					*ofn++ = *sp++;
				*ofn = _T('\0');
			}
			else if (*sp == _T('2'))
			{
				/* error redirection */
				sp++;

				if (*sp == _T('>'))
				{
					*lpnFlags |= ERROR_REDIRECTION;
					sp++;

					/* append request ? */
					if (*sp == _T('>'))
					{
						*lpnFlags |= ERROR_APPEND;
						sp++;
					}
				}
				else
				{
					/* no redirection!! copy the '2' character! */
					sp--;
					*dp++ = *sp++;
					continue;
				}

				while (_istspace (*sp))
					sp++;

				/* copy file name */
				while (*sp && !IsRedirection (*sp) && !_istspace (*sp))
					*efn++ = *sp++;
				*efn = _T('\0');
			}
			else if (*sp == _T('&'))
			{
				/* output AND error redirection */
				sp++;

				if (*sp == _T('>'))
				{
					*lpnFlags |= (ERROR_REDIRECTION | OUTPUT_REDIRECTION);
					sp++;

					/* append request ? */
					if (*sp == _T('>'))
					{
						*lpnFlags |= (ERROR_APPEND | OUTPUT_APPEND);
						sp++;
					}
				}
				else
				{
					/* no redirection!! copy the '&' character! */
					sp--;
					*dp++ = *sp++;
					continue;
				}

				while (_istspace (*sp))
					sp++;

				/* copy file name */
				while (*sp && !IsRedirection (*sp) && !_istspace (*sp))
					*ofn++ = *efn++ = *sp++;
				*ofn = *efn = _T('\0');
			}
		}
		else
			*dp++ = *sp++;
	}
	*dp++ = _T('\0');
	*dp = _T('\0');

	/* now go for the pipes */
	sp = s;
	while (*sp)
	{
		if ((*sp == _T('"')) || (*sp == _T('\'')))
		{
			TCHAR qc = *sp;

			do
				sp++;
			while (*sp && *sp != qc);

			sp++;
		}
		else if (*sp == _T('|'))
		{
			*sp++ = _T('\0');
			num++;
		}
		else
			sp++;
	}

	return num;
}

#endif /* FEATURE_REDIRECTION */
