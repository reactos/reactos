/*
 *  FREE.C - internal command.
 *
 *
 *  History:
 *
 *    01-Sep-1999 (Eric Kohl)
 *        Started.
 */

#include "config.h"

#ifdef INCLUDE_CMD_FREE

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "cmd.h"


/*
 * convert
 *
 * insert commas into a number
 */

static INT
ConvertULargeInteger (ULARGE_INTEGER num, LPTSTR des, INT len)
{
	TCHAR temp[32];
	INT c = 0;
	INT n = 0;

	if (num.QuadPart == 0)
	{
		des[0] = _T('0');
		des[1] = _T('\0');
		n = 1;
	}
	else
	{
		temp[31] = 0;
		while (num.QuadPart > 0)
		{
			if (((c + 1) % (nNumberGroups + 1)) == 0)
				temp[30 - c++] = cThousandSeparator;
			temp[30 - c++] = (TCHAR)(num.QuadPart % 10) + _T('0');
			num.QuadPart /= 10;
		}

		for (n = 0; n <= c; n++)
			des[n] = temp[31 - c + n];
	}

	return n;
}


static VOID
PrintDiskInfo (LPTSTR szDisk)
{
	TCHAR szRootPath[4] = "A:\\";
	TCHAR szDrive[2] = "A";
	TCHAR szVolume[64];
	TCHAR szSerial[10];
	TCHAR szTotal[40];
	TCHAR szUsed[40];
	TCHAR szFree[40];
	DWORD dwSerial;
	ULARGE_INTEGER uliSize;
	DWORD dwSecPerCl;
	DWORD dwBytPerSec;
	DWORD dwFreeCl;
	DWORD dwTotCl;

	if (_tcslen (szDisk) < 2 || szDisk[1] != _T(':'))
	{
		ConErrPrintf (_T("Invalid drive %s\n"), szDisk);
		return;
	}

	szRootPath[0] = szDisk[0];
	szDrive[0] = _totupper (szRootPath[0]);

	if (!GetVolumeInformation (szRootPath, szVolume, 64, &dwSerial,
	                           NULL, NULL, NULL, 0))
	{
		ConErrPrintf (_T("Invalid drive %s:\n"), szDrive);
		return;
	}

	if (szVolume[0] == _T('\0'))
		_tcscpy (szVolume, _T("unlabeled"));

	_stprintf (szSerial,
	           _T("%04X-%04X"),
	           HIWORD(dwSerial),
	           LOWORD(dwSerial));

	if (!GetDiskFreeSpace (szRootPath, &dwSecPerCl,
	                       &dwBytPerSec, &dwFreeCl, &dwTotCl))
	{
		ConErrPrintf (_T("Invalid drive %s:\n"), szDrive);
		return;
	}

	uliSize.QuadPart = dwSecPerCl * dwBytPerSec * dwTotCl;
	ConvertULargeInteger (uliSize, szTotal, 40);

	uliSize.QuadPart = dwSecPerCl * dwBytPerSec * (dwTotCl - dwFreeCl);
	ConvertULargeInteger (uliSize, szUsed, 40);

	uliSize.QuadPart = dwSecPerCl * dwBytPerSec * dwFreeCl;
	ConvertULargeInteger (uliSize, szFree, 40);

	ConOutPrintf (_T("\n"
	                 " Volume in drive %s is %-11s   Serial number is %s\n"
	                 "  %16s bytes total disk space\n"
	                 "  %16s bytes used\n"
	                 "  %16s bytes free\n"),
	              szDrive, szVolume, szSerial,
	              szTotal, szUsed, szFree);
}


INT CommandFree (LPTSTR cmd, LPTSTR param)
{
	LPTSTR szParam;
	TCHAR  szDefPath[MAX_PATH];
	INT argc, i;
	LPTSTR *arg;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Displays drive information.\n"
		               "\n"
		               "FREE [drive: ...]"));
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

	arg = split (szParam, &argc);

	for (i = 0; i < argc; i++)
		PrintDiskInfo (arg[i]);

	freep (arg);

	return 0;
}

#endif /* INCLUDE_CMD_FREE */

/* EOF */