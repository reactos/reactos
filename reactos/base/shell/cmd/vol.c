/*
 *  VOL.C - vol internal command.
 *
 *
 *  History:
 *
 *    03-Dec-1998 (Eric Kohl)
 *        Replaced DOS calls by Win32 calls.
 *
 *    08-Dec-1998 (Eric Kohl)
 *        Added help text ("/?").
 *
 *    07-Jan-1999 (Eric Kohl)
 *        Cleanup.
 *
 *    18-Jan-1999 (Eric Kohl)
 *        Unicode ready!
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Redirection ready!
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_VOL


static INT
PrintVolumeHeader (LPTSTR pszRootPath)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR szVolName[80];
	DWORD dwSerialNr;

	/* get the volume information of the drive */
	if(!GetVolumeInformation (pszRootPath,
				  szVolName,
				  80,
				  &dwSerialNr,
				  NULL,
				  NULL,
				  NULL,
				  0))
	{
		ErrorMessage (GetLastError (), _T(""));
		return 1;
	}

	/* print drive info */
	if (szVolName[0] != '\0')
	{
		LoadString(CMD_ModuleHandle, STRING_VOL_HELP1, szMsg, RC_STRING_MAX_SIZE);
		ConOutPrintf(szMsg, pszRootPath[0],szVolName);
	}
	else
	{
		LoadString(CMD_ModuleHandle, STRING_VOL_HELP2, szMsg, RC_STRING_MAX_SIZE);
		ConOutPrintf(szMsg, pszRootPath[0]);
	}

	/* print the volume serial number */
	LoadString(CMD_ModuleHandle, STRING_VOL_HELP3, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg, HIWORD(dwSerialNr), LOWORD(dwSerialNr));
	return 0;
}


INT cmd_vol (LPTSTR cmd, LPTSTR param)
{
	TCHAR szRootPath[] = _T("A:\\");
	TCHAR szPath[MAX_PATH];

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_VOL_HELP4);
		return 0;
	}

  nErrorLevel = 0;

	if (param[0] == _T('\0'))
	{
		GetCurrentDirectory (MAX_PATH, szPath);
		szRootPath[0] = szPath[0];
	}
	else
	{
		_tcsupr (param);
		if (param[1] == _T(':'))
			szRootPath[0] = param[0];
		else
		{
			error_invalid_drive ();
      nErrorLevel = 1;
			return 1;
		}
	}

	if (!IsValidPathName (szRootPath))
	{
		error_invalid_drive ();
     nErrorLevel = 1;
		return 1;
	}

	/* print the header */
	if (!PrintVolumeHeader (szRootPath))
  {
	    nErrorLevel = 1;
		return 1;
  }

	return 0;
}

#endif
