/*
 *  LABEL.C - label internal command.
 *
 *
 *  History:
 *
 *    10-Dec-1998 (Eric Kohl)
 *        Started.
 *
 *    11-Dec-1998 (Eric Kohl)
 *        Finished.
 *
 *    19-Jan-1998 (Eric Kohl)
 *        Unicode ready!
 *
 *    28-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_LABEL


INT cmd_label (LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
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
		ConOutResPaging(TRUE,STRING_LABEL_HELP1);
		return 0;
	}

  nErrorLevel = 0;

	/* get parameters */
	arg = split (param, &args, FALSE);

	if (args > 2)
	{
		/* too many parameters */
		error_too_many_parameters (arg[args - 1]);
		freep (arg);
    nErrorLevel = 1;
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

	_tcsncat(szLabel, param, 79);

	/* check root path */
	if (!IsValidPathName (szRootPath))
	{
		error_invalid_drive ();
		freep (arg);
    nErrorLevel = 1;
		return 1;
	}

	if (szLabel[0] == _T('\0'))
	{
		GetVolumeInformation(szRootPath, szOldLabel, 80, &dwSerialNr,
		                     NULL, NULL, NULL, 0);

		/* print drive info */
		if (szOldLabel[0] != _T('\0'))
		{
			ConOutResPrintf(STRING_LABEL_HELP2, _totupper(szRootPath[0]), szOldLabel);
		}
		else
		{
			ConOutResPrintf(STRING_LABEL_HELP3, _totupper(szRootPath[0]));
		}

		/* print the volume serial number */
		ConOutResPrintf(STRING_LABEL_HELP4, HIWORD(dwSerialNr), LOWORD(dwSerialNr));

		LoadString(CMD_ModuleHandle, STRING_LABEL_HELP5, szMsg, RC_STRING_MAX_SIZE);
		ConOutResPuts(STRING_LABEL_HELP5);

		ConInString(szLabel, 80);
	}

	if (!SetVolumeLabel(szRootPath, szLabel))
	{
		ConOutFormatMessage(GetLastError());
		nErrorLevel = 1;
		return 1;
	}

	freep(arg);

	return 0;
}

#endif /* INCLUDE_CMD_LABEL */

/* EOF */
