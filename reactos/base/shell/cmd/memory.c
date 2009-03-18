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

INT CommandMemory (LPTSTR param)
{
	MEMORYSTATUSEX msex;
	TCHAR szMemoryLoad[20];
	TCHAR szTotalPhys[40];
	TCHAR szAvailPhys[40];
	TCHAR szTotalPageFile[40];
	TCHAR szAvailPageFile[40];
	TCHAR szTotalVirtual[40];
	TCHAR szAvailVirtual[40];

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_MEMMORY_HELP1);
		return 0;
	}

	BOOL (WINAPI *GlobalMemoryStatusEx)(LPMEMORYSTATUSEX)
		= GetProcAddress(GetModuleHandle(_T("KERNEL32")), "GlobalMemoryStatusEx");
	if (GlobalMemoryStatusEx)
	{
		msex.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&msex);
	}
	else
	{
		MEMORYSTATUS ms;
		ms.dwLength = sizeof(MEMORYSTATUS);
		GlobalMemoryStatus(&ms);
		msex.dwMemoryLoad = ms.dwMemoryLoad;
		msex.ullTotalPhys = ms.dwTotalPhys;
		msex.ullAvailPhys = ms.dwAvailPhys;
		msex.ullTotalPageFile = ms.dwTotalPageFile;
		msex.ullAvailPageFile = ms.dwAvailPageFile;
		msex.ullTotalVirtual = ms.dwTotalVirtual;
		msex.ullAvailVirtual = ms.dwAvailVirtual;
	}

	ConvertULargeInteger(msex.dwMemoryLoad, szMemoryLoad, 20, FALSE);
	ConvertULargeInteger(msex.ullTotalPhys, szTotalPhys, 40, TRUE);
	ConvertULargeInteger(msex.ullAvailPhys, szAvailPhys, 40, TRUE);
	ConvertULargeInteger(msex.ullTotalPageFile, szTotalPageFile, 40, TRUE);
	ConvertULargeInteger(msex.ullAvailPageFile, szAvailPageFile, 40, TRUE);
	ConvertULargeInteger(msex.ullTotalVirtual, szTotalVirtual, 40, TRUE);
	ConvertULargeInteger(msex.ullAvailVirtual, szAvailVirtual, 40, TRUE);

	ConOutResPrintf(STRING_MEMMORY_HELP2,
	                szMemoryLoad, szTotalPhys, szAvailPhys, szTotalPageFile,
	                szAvailPageFile, szTotalVirtual, szAvailVirtual);

	return 0;
}

#endif /* INCLUDE_CMD_MEMORY */

/* EOF */
