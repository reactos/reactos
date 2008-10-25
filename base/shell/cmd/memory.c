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
	ULARGE_INTEGER ui;

	ui.u.LowPart = num;
	ui.u.HighPart = 0;
	return ConvertULargeInteger(ui, des, len, bSeparator);
}


INT CommandMemory (LPTSTR param)
{
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

	ConOutResPrintf(STRING_MEMMORY_HELP2,
	                szMemoryLoad, szTotalPhys, szAvailPhys, szTotalPageFile,
	                szAvailPageFile, szTotalVirtual, szAvailVirtual);

	return 0;
}

#endif /* INCLUDE_CMD_MEMORY */

/* EOF */
