/*
 *  HISTORY.C - command line history.
 *
 *
 *  History:
 *
 *    14/01/95 (Tim Norman)
 *        started.
 *
 *    08/08/95 (Matt Rains)
 *        i have cleaned up the source code. changes now bring this source
 *        into guidelines for recommended programming practice.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    25-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Cleanup!
 *        Unicode and redirection safe!
 */

#include "config.h"

#ifdef FEATURE_HISTORY

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"

#define MAXLINES 128

static INT history_size = 2048;    /* make this configurable later */


VOID History (INT dir, LPTSTR commandline)
{
	static LPTSTR history = NULL;
	static LPTSTR lines[MAXLINES];
	static INT curline = 0;
	static INT numlines = 0;
	static INT maxpos = 0;
	INT count;
	INT length;

	if (!history)
	{
		history = malloc (history_size * sizeof (TCHAR));
		lines[0] = history;
		history[0] = 0;
	}

	if (dir > 0)
	{
		/* next command */
		if (curline < numlines)
		{
			curline++;
		}

		if (curline == numlines)
		{
			commandline[0] = 0;
		}
		else
		{
			_tcscpy (commandline, lines[curline]);
		}
	}
	else if (dir < 0)
	{
		/* prev command */
		if (curline > 0)
		{
			curline--;
		}

		_tcscpy (commandline, lines[curline]);
	}
	else
	{
		/* add to history */
		/* remove oldest string until there's enough room for next one */
		/* strlen (commandline) must be less than history_size! */
		while ((maxpos + (INT)_tcslen (commandline) + 1 > history_size) || (numlines >= MAXLINES))
		{
			length = _tcslen (lines[0]) + 1;

			for (count = 0; count < maxpos && count + (lines[1] - lines[0]) < history_size; count++)
			{
				history[count] = history[count + length];
			}

			maxpos -= length;

			for (count = 0; count <= numlines && count < MAXLINES; count++)
			{
				lines[count] = lines[count + 1] - length;
			}

			numlines--;
#ifdef DEBUG
			ConOutPrintf (_T("Reduced size:  %ld lines\n"), numlines);

			for (count = 0; count < numlines; count++)
			{
				ConOutPrintf (_T("%d: %s\n"), count, lines[count]);
			}
#endif
		}

		_tcscpy (lines[numlines], commandline);
		numlines++;
		lines[numlines] = lines[numlines - 1] + _tcslen (commandline) + 1;
		maxpos += _tcslen (commandline) + 1;
		/* last line, empty */
		curline = numlines;
	}

	return;
}

#endif
