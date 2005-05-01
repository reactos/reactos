/*
 *  VOL.C - vol internal command.
 *
 *
 *  History:
 *
 *    03-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Replaced DOS calls by Win32 calls.
 *
 *    08-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("/?").
 *
 *    07-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Cleanup.
 *
 *    18-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection ready!
 */

#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_VOL


static INT
PrintVolumeHeader (LPTSTR pszRootPath)
{
	TCHAR szVolName[80];
	DWORD dwSerialNr;
	TCHAR szMsg[RC_STRING_MAX_SIZE];


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
		LoadString( GetModuleHandle(NULL), STRING_VOL_HELP1, szMsg,sizeof(szMsg)/sizeof(TCHAR));
		ConOutPrintf (szMsg, pszRootPath[0],szVolName);
	}
	else
	{
		LoadString( GetModuleHandle(NULL), STRING_VOL_HELP2, szMsg,sizeof(szMsg)/sizeof(TCHAR));
		ConOutPrintf (szMsg, pszRootPath[0]);
	}

	
	

	/* print the volume serial number */
	LoadString( GetModuleHandle(NULL), STRING_VOL_HELP3, szMsg,sizeof(szMsg)/sizeof(TCHAR));
	ConOutPrintf (szMsg,
		      HIWORD(dwSerialNr),
		      LOWORD(dwSerialNr));
	return 0;
}


INT cmd_vol (LPTSTR cmd, LPTSTR param)
{
	TCHAR szRootPath[] = _T("A:\\");
	TCHAR szPath[MAX_PATH];
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		LoadString( GetModuleHandle(NULL), STRING_VOL_HELP4, szMsg,sizeof(szMsg)/sizeof(TCHAR));
		ConOutPuts ((LPTSTR)szMsg);	

		return 0;
	}

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
			return 1;
		}
	}

	if (!IsValidPathName (szRootPath))
	{
		error_invalid_drive ();
		return 1;
	}

	/* print the header */
	if (!PrintVolumeHeader (szRootPath))
		return 1;

	return 0;
}

#endif
