/*
 *  MEMORY.C - internal command.
 *
 *
 *  History:
 *
 *    01-Sep-1999 (Eric Kohl)
 *        Started.
 */

#include "config.h"

#ifdef INCLUDE_CMD_MEMORY

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"


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
		ConOutPuts (_T("Displays the amount of system memory.\n"
		               "\n"
		               "MEMORY"));
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

	ConOutPrintf (_T("\n"
	                 "  %12s%% memory load.\n"
	                 "\n"
	                 "  %13s bytes total physical RAM.\n"
	                 "  %13s bytes available physical RAM.\n"
	                 "\n"
	                 "  %13s bytes total page file.\n"
	                 "  %13s bytes available page file.\n"
	                 "\n"
	                 "  %13s bytes total virtual memory.\n"
	                 "  %13s bytes available virtual memory.\n"),
	              szMemoryLoad, szTotalPhys, szAvailPhys, szTotalPageFile,
	              szAvailPageFile, szTotalVirtual, szAvailVirtual);

	return 0;
}

#endif /* INCLUDE_CMD_MEMORY */

/* EOF */