/*
 *  WHERE.C - file serch functions.
 *
 *
 *  History:
 *
 *  07/15/95 (Tim Norman)
 *    started.
 *
 *  08/08/95 (Matt Rains)
 *    i have cleaned up the source code. changes now bring this source into
 *    guidelines for recommended programming practice.
 *
 *  12/12/95 (Steffan Kaiser & Tim Norman)
 *    added some patches to fix some things and make more efficient
 *
 *  1/6/96 (Tim Norman)
 *    fixed a stupid pointer mistake...  Thanks to everyone who noticed it!
 *
 *  8/1/96 (Tim Norman)
 *    fixed a bug when getenv returns NULL
 *
 *  8/7/96 (Steffan Kaiser and Tim Norman)
 *    speed improvements and bug fixes
 *
 *  8/27/96 (Tim Norman)
 *    changed code to use pointers directly into PATH environment variable
 *    rather than making our own copy.  This saves some memory, but requires
 *    we write our own function to copy pathnames out of the variable.
 *
 *  12/23/96 (Aaron Kaufman)
 *    Fixed a bug in get_paths() that did not point to the first PATH in the
 *    environment variable.
 *
 *  7/12/97 (Tim Norman)
 *    Apparently, Aaron's bugfix got lost, so I fixed it again.
 *
 *  16 July 1998 (John P. Price)
 *    Added stand alone code.
 *
 *  17 July 1998 (John P. Price)
 *    Rewrote find_which to use searchpath function
 *
 * 24-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 * - fixed bug where didn't check all extensions when path was specified
 *
 * 27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 * - added config.h include
 *
 * 30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 * - fixed so that it find_which returns NULL if filename is not executable
 *   (does not have .bat, .com, or .exe extention). Before command would
 *   to execute any file with any extension (opps!)
 *
 *    03-Dec_1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Changed find_which().
 *
 *    07-Dec_1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added ".CMD" extension.
 *        Replaced numeric constant by _NR_OF_EXTENSIONS.
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#include <windows.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


static LPTSTR ext[]  = {".BAT", ".CMD", ".COM", ".EXE"};
static INT nExtCount = sizeof(ext) / sizeof(LPTSTR);


/* searches for file using path info. */

BOOL find_which (LPCTSTR fname, LPTSTR fullbuffer)
{
	static TCHAR temp[MAX_PATH];
	LPTSTR fullname;
	INT x;

	*fullbuffer = _T('\0');

	/* if there an extension and it is in the last path component, then
	 * don't test all the extensions. */
	if (!(fullname = _tcsrchr (fname, _T('.'))) ||
		_tcschr (fullname + 1, _T('\\')))
	{
#ifdef _DEBUG
		DebugPrintf ("No filename extension!\n");
#endif

		for (x = 0; x < nExtCount; x++)
		{
			_tcscpy (temp, fname);
			_tcscat (temp, ext[x]);
#ifdef _DEBUG
			DebugPrintf ("Checking for %s\n", temp);
#endif
			if (_tcschr (fname, _T('\\')))
			{
				if (IsValidFileName (temp))
				{
					_tcscpy (fullbuffer, temp);
					return TRUE;
				}
			}
			else
			{
				_searchenv (temp, _T("PATH"), fullbuffer);
				if (*fullbuffer != '\0')
					return TRUE;
			}
		}
	}
	else
	{
		/* there is an extension... don't test other extensions */
		/* make sure that the extention is one of the four */
#ifdef _DEBUG
		DebugPrintf ("No filename extension!\n");
#endif
		for (x = 0; x < nExtCount; x++)
		{
			if (!_tcsicmp (_tcsrchr (fname, _T('.')), ext[x]))
			{
				if (_tcschr (fname, _T('\\')))
				{
					if (IsValidFileName (fname))
					{
						_tcscpy (fullbuffer, fname);
#ifdef _DEBUG
						DebugPrintf ("Found: %s\n", fullbuffer);
#endif
						return TRUE;
					}
				}
				else
				{
#ifdef _DEBUG
					DebugPrintf ("Checking for %s\n", fname);
#endif
					_searchenv (fname, _T("PATH"), fullbuffer);
					if (*fullbuffer != _T('\0'))
					{
#ifdef _DEBUG
						DebugPrintf ("Found: %s\n", fullbuffer);
#endif
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}
