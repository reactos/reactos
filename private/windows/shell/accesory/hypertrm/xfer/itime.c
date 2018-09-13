/* itime.c -- functions to handle time in our program
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
#include <windows.h>
#pragma hdrstop

#include <time.h>
#include <memory.h>
#include <tdll\stdtyp.h>
#include <tdll\assert.h>

#include "itime.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *                                                                            *
 *                             R E A D    M E                                 *
 *                                                                            *
 * Everybody keeps changing the time standard to whatever they feel might be  *
 * a little bit better for them.  So far I have found 3 different standards   *
 * in Microsoft functions.  This does not even count the fact that HyperP     *
 * uses its own format for time.                                              *
 *                                                                            *
 * Henceforth, all time values that are passed around in the program will be  *
 * based on the old UCT format of the number of seconds since Jan 1, 1970.    *
 *                                                                            *
 * Please use an unsigned long for these values.                              *
 *                                                                            *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

unsigned long itimeGetBasetime(void);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	itimeSetFileTime
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to set the date/time of a
 *	file.
 *
 * PARAMETERS:
 *	pszName -- pointer to a file name
 *	ulTime  -- our internal standard time format
 *
 * RETURNS:
 *	Nothing.
 *
 */
void itimeSetFileTime(LPCTSTR pszName, unsigned long ulTime)
	{
	time_t base_time;
	struct tm *pstT;
	WORD wDOSDate;
	WORD wDOSTime;
	HANDLE hFile;
	FILETIME stFileTime;

	/* Yes, we need to open the file */
	hFile = CreateFile(pszName,
						GENERIC_READ,
						FILE_SHARE_READ,
						0,
						OPEN_EXISTING,
						0,
						0);
	if (hFile == INVALID_HANDLE_VALUE)
		return;								/* No such file */

	base_time = itimeGetBasetime();
	if ((long)base_time == (-1))
		goto SFTexit;

	base_time += ulTime;	/* Convert to 1990 base */

	pstT = localtime(&base_time);
	assert(pstT);
	/* For some reason, this sometimes returns a NULL */
	if (pstT)
		{
		/* Build the "DOS" formats */
		wDOSDate = ((pstT->tm_year - 80) << 9) |
					(pstT->tm_mon << 5) |
					pstT->tm_mday;
		DbgOutStr("Date %d %d %d 0x%x\r\n",
					pstT->tm_year, pstT->tm_mon, pstT->tm_mday, wDOSDate, 0);

		wDOSTime = ((pstT->tm_hour - 1) << 11) |
					(pstT->tm_min << 5) |
					(pstT->tm_sec / 2);
		DbgOutStr("Time %d %d %d 0x%x\r\n",
					pstT->tm_hour, pstT->tm_min, pstT->tm_sec, wDOSTime, 0);

		/* Convert to CHICAGO format */
		/* TODO: as of 14-Mar-94, this doesn't work. Check later */
		if (!DosDateTimeToFileTime(wDOSDate, wDOSTime, &stFileTime))
			goto SFTexit;

		/* Set the time */
		SetFileTime(hFile, &stFileTime, &stFileTime, &stFileTime);
		}

SFTexit:
	/* Close the handle */
	CloseHandle(hFile);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	itimeGetFileTime
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to get the date/time of a
 *	file.
 *
 * PARAMETERS:
 *	pszName -- pointer to a file name
 *
 * RETURNS:
 *	The file date/time in our internal standard time format.
 *
 */
unsigned long itimeGetFileTime(LPCTSTR pszName)
	{
	unsigned long ulTime = 0;
	struct tm stT;
	WORD wDOSDate;
	WORD wDOSTime;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	FILETIME stFileTime;

	/* Yes, we need to open the file */
	hFile = CreateFile(pszName,
						GENERIC_READ,
						FILE_SHARE_READ,
						0,
						OPEN_EXISTING,
						0,
						0);
	if (hFile == INVALID_HANDLE_VALUE)
		goto GFTexit;

	if (!GetFileTime(hFile, NULL, NULL, &stFileTime))
		goto GFTexit;

	if (!FileTimeToDosDateTime(&stFileTime, &wDOSDate, &wDOSTime))
		goto GFTexit;

	memset(&stT, 0, sizeof(struct tm));
	stT.tm_mday = (wDOSDate & 0x1F);
	stT.tm_mon = ((wDOSDate >> 5) & 0xF);
	stT.tm_year = ((wDOSDate >> 9) & 0x7F);
	stT.tm_sec = (wDOSTime & 0x1F) * 2;
	stT.tm_min = ((wDOSTime >> 5) & 0x3F);
	stT.tm_hour = ((wDOSTime >> 11) & 0x1F);

	stT.tm_year += 80;

	ulTime = (unsigned long) mktime(&stT);
	if ((long)ulTime == (-1))
		ulTime = 0;
	else
		ulTime -= itimeGetBasetime();


GFTexit:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return ulTime;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *	This function converts the "new" internal MICROSOFT time format (based at
 *	1900) to the "old" format (based at 1970).
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
unsigned long itimeGetBasetime()
	{
	unsigned long ulBaseTime = 0;
	struct tm stT;

	memset(&stT, 0, sizeof(struct tm));

	/* Get our base time */
	stT.tm_mday = 1;		/* Jan 1, 1970 */
	stT.tm_mon = 1;
	stT.tm_year = 70;

	ulBaseTime = (unsigned long) mktime(&stT);

	return ulBaseTime;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
