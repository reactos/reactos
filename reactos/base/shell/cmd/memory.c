/*
 *  MEMORY.C - internal command.
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

#ifdef INCLUDE_CMD_MEMORY


/*
 * convert
 *
 * insert commas into a number
 */
static INT
ConvertDWord (DWORD num, LPTSTR des, INT len, BOOL bSeparator)
{
	TCHAR temp[32];
	INT c = 0;
	INT n = 0;

	if (num == 0)
	{
		des[0] = _T('0');
		des[1] = _T('\0');
		n = 1;
	}
	else
	{
		temp[31] = 0;
		while (num > 0)
		{
			if (bSeparator && (((c + 1) % (nNumberGroups + 1)) == 0))
				temp[30 - c++] = cThousandSeparator;
			temp[30 - c++] = (TCHAR)(num % 10) + _T('0');
			num /= 10;
		}

		for (n = 0; n <= c; n++)
			des[n] = temp[31 - c + n];
	}

	return n;
}


INT CommandMemory (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	MEMORYSTATUS ms;
	TCHAR szMemoryLoad[20];
	TCHAR szTotalPhys[20];
	TCHAR szAvailPhys[20];
	TCHAR szTotalPageFile[20];
	TCHAR szAvailPageFile[20];
	TCHAR szTotalVirtual[20];
	TCHAR szAvailVirtual[20];

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_MEMMORY_HELP1);
		return 0;
	}

	ms.dwLength = sizeof(MEMORYSTATUS);

	GlobalMemoryStatus (&ms);

	ConvertDWord (ms.dwMemoryLoad, szMemoryLoad, 20, FALSE);
	ConvertDWord (ms.dwTotalPhys, szTotalPhys, 20, TRUE);
	ConvertDWord (ms.dwAvailPhys, szAvailPhys, 20, TRUE);
	ConvertDWord (ms.dwTotalPageFile, szTotalPageFile, 20, TRUE);
	ConvertDWord (ms.dwAvailPageFile, szAvailPageFile, 20, TRUE);
	ConvertDWord (ms.dwTotalVirtual, szTotalVirtual, 20, TRUE);
	ConvertDWord (ms.dwAvailVirtual, szAvailVirtual, 20, TRUE);

	LoadString(CMD_ModuleHandle, STRING_MEMMORY_HELP2, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg,
	             szMemoryLoad, szTotalPhys, szAvailPhys, szTotalPageFile,
	             szAvailPageFile, szTotalVirtual, szAvailVirtual);

	return 0;
}

#endif /* INCLUDE_CMD_MEMORY */

/* EOF */
