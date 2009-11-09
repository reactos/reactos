/*
 *  FREE.C - internal command.
 *
 *
 *  History:
 *
 *    01-Sep-1999 (Eric Kohl)
 *        Started.
 *
 *    28-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_FREE

static VOID
PrintDiskInfo (LPTSTR szDisk)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR szRootPath[4] = _T("A:\\");
	TCHAR szDrive[2] = _T("A");
	TCHAR szVolume[64];
	TCHAR szSerial[10];
	TCHAR szTotal[40];
	TCHAR szUsed[40];
	TCHAR szFree[40];
	DWORD dwSerial;
	ULONGLONG uliSize;
	DWORD dwSecPerCl;
	DWORD dwBytPerSec;
	DWORD dwFreeCl;
	DWORD dwTotCl;

	if (_tcslen (szDisk) < 2 || szDisk[1] != _T(':'))
	{
		ConErrResPrintf(STRING_FREE_ERROR1);
		return;
	}

	szRootPath[0] = szDisk[0];
	szDrive[0] = _totupper (szRootPath[0]);

	if (!GetVolumeInformation (szRootPath, szVolume, 64, &dwSerial,
	                           NULL, NULL, NULL, 0))
	{
		LoadString(CMD_ModuleHandle, STRING_FREE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(_T("%s %s:\n"), szMsg, szDrive);
		return;
	}

	if (szVolume[0] == _T('\0'))
	{

		LoadString(CMD_ModuleHandle, STRING_FREE_ERROR2, szMsg, RC_STRING_MAX_SIZE);
		_tcscpy (szVolume, szMsg);
	}

	_stprintf (szSerial,
	           _T("%04X-%04X"),
	           HIWORD(dwSerial),
	           LOWORD(dwSerial));

	if (!GetDiskFreeSpace (szRootPath, &dwSecPerCl,
	                       &dwBytPerSec, &dwFreeCl, &dwTotCl))
	{
		LoadString(CMD_ModuleHandle, STRING_FREE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf (_T("%s %s:\n"), szMsg, szDrive);
		return;
	}

	uliSize = dwSecPerCl * dwBytPerSec * (ULONGLONG)dwTotCl;
	ConvertULargeInteger(uliSize, szTotal, 40, TRUE);

	uliSize = dwSecPerCl * dwBytPerSec * (ULONGLONG)(dwTotCl - dwFreeCl);
	ConvertULargeInteger(uliSize, szUsed, 40, TRUE);

	uliSize = dwSecPerCl * dwBytPerSec * (ULONGLONG)dwFreeCl;
	ConvertULargeInteger(uliSize, szFree, 40, TRUE);


	ConOutResPrintf(STRING_FREE_HELP1, szDrive, szVolume, szSerial, szTotal, szUsed, szFree);
}


INT CommandFree (LPTSTR param)
{
	LPTSTR szParam;
	TCHAR  szDefPath[MAX_PATH];
	INT argc, i;
	LPTSTR *arg;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_FREE_HELP2);
		return 0;
	}

	if (!param || *param == _T('\0'))
	{
		GetCurrentDirectory (MAX_PATH, szDefPath);
		szDefPath[2] = _T('\0');
		szParam = szDefPath;
	}
	else
		szParam = param;

	arg = split (szParam, &argc, FALSE);

	for (i = 0; i < argc; i++)
		PrintDiskInfo (arg[i]);

	freep (arg);

	return 0;
}

#endif /* INCLUDE_CMD_FREE */

/* EOF */
