/*      File: D:\WACKER\tdll\printhdl.c (Created: 10-Dec-1993)
 *
 *      Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *      All rights reserved
 *
 *      $Revision: 2 $
 *      $Date: 2/05/99 3:21p $
 */

#include <windows.h>
#pragma hdrstop

#include <term\res.h>

#include "stdtyp.h"
#include "mc.h"
#include "assert.h"
#include "print.h"
#include "print.hh"
#include "sf.h"
#include "tdll.h"
#include "tchar.h"
#include "term.h"
#include "session.h"
#include "sess_ids.h"
#include "statusbr.h"
#include "globals.h"
#include "errorbox.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      printCreateHdl
 *
 * DESCRIPTION:
 *      Creates a print handle.
 *
 *
 * ARGUMENTS:
 *      hSession        - Exteranl session handle
 *
 * RETURNS:
 *      Returns an External Print Handle, or 0 if an error.
 *
 */
HPRINT printCreateHdl(const HSESSION hSession)
	{
	HHPRINT hhPrint = 0;

	hhPrint = malloc(sizeof(*hhPrint));

	if (hhPrint == 0)
		{
		assert(FALSE);
		return 0;
		}

	memset(hhPrint, 0, sizeof(*hhPrint));

	hhPrint->hSession = hSession;

	InitializeCriticalSection(&hhPrint->csPrint);

	if (printInitializeHdl((HPRINT)hhPrint) != 0)
		{
		printDestroyHdl((HPRINT)hhPrint);
		hhPrint = NULL;
		return 0;
		}

	return (HPRINT)hhPrint;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      printInitializeHdl
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *      hPrint - External print handle.
 *
 * RETURNS:
 *      0 if successful, otherwise -1
 *
 */
int printInitializeHdl(const HPRINT hPrint)
	{
	unsigned long  lSize;
	const HHPRINT hhPrint = (HHPRINT)hPrint;
	TCHAR *tmp = 0;
	TCHAR achBuf[256];
	TCHAR *pszString;
	int		nCharSet;

	if (hhPrint == 0)
		return -1;

	hhPrint->nLnIdx = 0;

	// Initialize the printer name to the default.
	//
	if (GetProfileString("Windows", "Device", ",,,", achBuf,
					sizeof(achBuf)) && (pszString = strtok(achBuf, ",")))
		{
		StrCharCopy(hhPrint->achPrinterName, pszString);
		}
	else
		{
		// Just to let you know, there is no printer.
		//
		assert(FALSE);
		hhPrint->achPrinterName[0] = TEXT('\0');
		}

	if (hhPrint->pstDevMode)
		{
		free(hhPrint->pstDevMode);
		hhPrint->pstDevMode = 0;
		}

	if (hhPrint->pstDevNames)
		{
		free(hhPrint->pstDevNames);
		hhPrint->pstDevNames = 0;
		}

	lSize = sizeof(hhPrint->achPrinterName);

	sfGetSessionItem(sessQuerySysFileHdl(hhPrint->hSession),
						SFID_PRINTSET_NAME,
						&lSize,
						hhPrint->achPrinterName);


	lSize = 0;
	if (sfGetSessionItem(sessQuerySysFileHdl(hhPrint->hSession),
						SFID_PRINTSET_DEVMODE,
						&lSize,
						0) == 0 && lSize)
		{
		if ((hhPrint->pstDevMode = malloc(lSize)))
			{
			sfGetSessionItem(sessQuerySysFileHdl(hhPrint->hSession),
						SFID_PRINTSET_DEVMODE,
						&lSize,
						hhPrint->pstDevMode);
			}
		}

	lSize = 0;
	if (sfGetSessionItem(sessQuerySysFileHdl(hhPrint->hSession),
						SFID_PRINTSET_DEVNAMES,
						&lSize,
						0) == 0 && lSize)
		{
		if ((hhPrint->pstDevNames = malloc(lSize)))
			{
			sfGetSessionItem(sessQuerySysFileHdl(hhPrint->hSession),
						SFID_PRINTSET_DEVNAMES,
						&lSize,
						hhPrint->pstDevNames);
			}
		}

    //
    // get the font and margin settings
    //

    memset(&hhPrint->lf, 0, sizeof(LOGFONT));
    memset(&hhPrint->margins, 0, sizeof(RECT));
    hhPrint->hFont = NULL;

	lSize = sizeof(hhPrint->margins);
	sfGetSessionItem( sessQuerySysFileHdl(hhPrint->hSession),
		      SFID_PRINTSET_MARGINS,
					  &lSize, &hhPrint->margins );

	lSize = sizeof(hhPrint->lf);
	sfGetSessionItem( sessQuerySysFileHdl(hhPrint->hSession),
		      SFID_PRINTSET_FONT,
					  &lSize, &hhPrint->lf );

	lSize = sizeof(hhPrint->iFontPointSize);
	sfGetSessionItem( sessQuerySysFileHdl(hhPrint->hSession),
		      SFID_PRINTSET_FONT_HEIGHT,
					  &lSize, &hhPrint->iFontPointSize );


    //
    // use default if we have no value stored
    //

    if (hhPrint->lf.lfHeight == 0)
		{
		TCHAR faceName[100];

		if ( LoadString(glblQueryDllHinst(), IDS_PRINT_DEF_FONT,
			faceName, sizeof (hhPrint->lf.lfFaceName)) )
			{
			strncpy( hhPrint->lf.lfFaceName, faceName, sizeof (faceName) );
			hhPrint->lf.lfFaceName[sizeof(hhPrint->lf.lfFaceName)-1] = TEXT('\0');
			}

		hhPrint->lf.lfHeight    = -17;
		hhPrint->iFontPointSize = 100;
		
		//mpt:2-4-98 changed to use resources so that dbcs fonts print correctly
		if (LoadString(glblQueryDllHinst(), IDS_PRINT_DEF_CHARSET,
			achBuf, sizeof(achBuf)))
			{
			nCharSet = atoi(achBuf);
			hhPrint->lf.lfCharSet = (BYTE)nCharSet;
			}
		
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      printSaveHdl
 *
 * DESCRIPTION:
 *      Saves the name of the selected printer in the session file.
 *
 * ARGUMENTS:
 *      hPrint   -       The external printer handle.
 *
 * RETURNS:
 *      void
 *
 */
void printSaveHdl(const HPRINT hPrint)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;
	unsigned long ulSize;
	TCHAR *sz;

	sfPutSessionItem(sessQuerySysFileHdl(hhPrint->hSession),
						SFID_PRINTSET_NAME,
						StrCharGetByteCount(hhPrint->achPrinterName) +
							sizeof(TCHAR),
						hhPrint->achPrinterName);

	if (hhPrint->pstDevMode)
		{
		ulSize = hhPrint->pstDevMode->dmSize +
			hhPrint->pstDevMode->dmDriverExtra;

		sfPutSessionItem(sessQuerySysFileHdl(hhPrint->hSession),
						SFID_PRINTSET_DEVMODE,
						ulSize,
						hhPrint->pstDevMode);
		}

	if (hhPrint->pstDevNames)
		{
		// Getting the size of a DEVNAMES structure is harder.
		//
		sz = (TCHAR *)hhPrint->pstDevNames +
			hhPrint->pstDevNames->wOutputOffset;

		sz += StrCharGetByteCount((LPCSTR)sz) + sizeof(TCHAR);
		ulSize = (unsigned long)(sz - (TCHAR *)hhPrint->pstDevNames);

		sfPutSessionItem(sessQuerySysFileHdl(hhPrint->hSession),
						SFID_PRINTSET_DEVNAMES,
						ulSize,
						hhPrint->pstDevNames);
		}

    //
    // save the font and margin settings
    //

	sfPutSessionItem( sessQuerySysFileHdl(hhPrint->hSession),
		      SFID_PRINTSET_MARGINS,
					  sizeof(hhPrint->margins), &hhPrint->margins );

	sfPutSessionItem( sessQuerySysFileHdl(hhPrint->hSession),
		      SFID_PRINTSET_FONT,
					  sizeof(hhPrint->lf), &hhPrint->lf );

	sfPutSessionItem( sessQuerySysFileHdl(hhPrint->hSession),
		      SFID_PRINTSET_FONT_HEIGHT,
					  sizeof(hhPrint->iFontPointSize),
		      &hhPrint->iFontPointSize );


	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      printDestroyHdl
 *
 * DESCRIPTION:
 *      Destroys a valid print handle.
 *
 * ARGUMENTS:
 *      hPrint   - AN External Print Handle.
 *
 * RETURNS:
 *      void
 *
 */
void printDestroyHdl(const HPRINT hPrint)
	{
	const HHPRINT hhPrint = (HHPRINT)hPrint;

	if (hhPrint == 0)
		return;

	if (hhPrint->hFont)
	{
		DeleteObject(hhPrint->hFont);
	}

	printEchoClose(hPrint);

	DeleteCriticalSection(&hhPrint->csPrint);

	free(hhPrint);
	return;
	}

