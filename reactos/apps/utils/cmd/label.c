/*
 *  LABEL.C - label internal command.
 *
 *
 *  History:
 *
 *    10-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 *
 *    11-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Finished.
 *
 *    19-Jan-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 */

#include "config.h"

#ifdef INCLUDE_CMD_LABEL

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"


INT cmd_label (LPTSTR cmd, LPTSTR param)
{
	TCHAR  szRootPath[] = _T("A:\\");
	TCHAR  szLabel[80];
	TCHAR  szOldLabel[80];
	DWORD  dwSerialNr;
	LPTSTR *arg;
	INT    args;

	/* set empty label string */
	szLabel[0] = _T('\0');

	/* print help */
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Displays or changes drive label.\n\n"
					   "LABEL [drive:][label]"));
		return 0;
	}

	/* get parameters */
	arg = split (param, &args);

	if (args > 2)
	{
		/* too many parameters */
		error_too_many_parameters (arg[args - 1]);
		freep (arg);
		return 1;
	}

	if (args == 0)
	{
		/* get label of current drive */
		TCHAR szCurPath[MAX_PATH];
		GetCurrentDirectory (MAX_PATH, szCurPath);
		szRootPath[0] = szCurPath[0];
	}
	else
	{
		if ((_tcslen (arg[0]) >= 2) && (arg[0][1] == _T(':')))
		{
			szRootPath[0] = toupper (*arg[0]);
			if (args == 2)
				_tcsncpy (szLabel, arg[1], 12);
		}
		else
		{
			TCHAR szCurPath[MAX_PATH];
			GetCurrentDirectory (MAX_PATH, szCurPath);
			szRootPath[0] = szCurPath[0];
			_tcsncpy (szLabel, arg[0], 12);
		}
	}

	/* check root path */
	if (!IsValidPathName (szRootPath))
	{
		error_invalid_drive ();
		freep (arg);
		return 1;
	}

	GetVolumeInformation (szRootPath, szOldLabel, 80, &dwSerialNr,
						  NULL, NULL, NULL, 0);

	/* print drive info */
	ConOutPrintf (_T("Volume in drive %c:"), _totupper (szRootPath[0]));

	if (szOldLabel[0] != _T('\0'))
		ConOutPrintf (_T(" is %s\n"), szOldLabel);
	else
		ConOutPrintf (_T(" has no label\n"));

	/* print the volume serial number */
	ConOutPrintf (_T("Volume Serial Number is %04X-%04X\n"),
				  HIWORD(dwSerialNr), LOWORD(dwSerialNr));

	if (szLabel[0] == _T('\0'))
	{
		ConOutPrintf (_T("Drive label (11 Characters, ENTER if none)? "));
		ConInString (szLabel, 80);
	}

	SetVolumeLabel (szRootPath, szLabel);

	freep (arg);

	return 0;
}

#endif /* INCLUDE_CMD_LABEL */
