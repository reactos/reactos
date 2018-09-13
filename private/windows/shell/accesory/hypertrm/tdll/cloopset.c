/*	File: cloopset.c (created 02/19/94, JKH)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:34p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\assert.h>
#include <tdll\session.h>
#include <tdll\timers.h>
#include <tdll\sf.h>
#include "tchar.h"
#include "cloop.h"
#include "cloop.hh"

/* --- Session file access functions --- */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CLoopInitHdl
 *
 * DESCRIPTION:
 *	Set all cloop user settings to valid, default values.
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int CLoopInitHdl(const HCLOOP hCLoop)
	{
	ST_CLOOP *pstCLoop = (ST_CLOOP *)hCLoop;

	pstCLoop->stWorkSettings.fSendCRLF		   = FALSE;
	pstCLoop->stWorkSettings.fExpandBlankLines = FALSE;
	pstCLoop->stWorkSettings.fLocalEcho 	   = FALSE;
	pstCLoop->stWorkSettings.fLineWait		   = FALSE;
	pstCLoop->stWorkSettings.chWaitChar 	   = TEXT('\r');
	pstCLoop->stWorkSettings.fExpandTabsOut    = FALSE;
	pstCLoop->stWorkSettings.nTabSizeOut	   = 8;
	pstCLoop->stWorkSettings.nLineDelay 	   = 0;
	pstCLoop->stWorkSettings.nCharDelay 	   = 0;

	pstCLoop->stWorkSettings.fAddLF 		   = FALSE;
	pstCLoop->stWorkSettings.fASCII7		   = FALSE;
	pstCLoop->stWorkSettings.fEchoplex		   = FALSE;
	pstCLoop->stWorkSettings.fShowHex		   = FALSE;
	pstCLoop->stWorkSettings.nTabSizeIn 	   = 8;

	// Original settings and working settings must match initially
	// If they no longer match at save time, then changes occurred.
	pstCLoop->stOrigSettings = pstCLoop->stWorkSettings;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CLoopLoadHdl
 *
 * DESCRIPTION:
 *	Loads CLoop user settings from the session file
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int CLoopLoadHdl(const HCLOOP hCLoop)
	{
	unsigned long	   ul;
	ST_CLOOP		  *pstCLoop = (ST_CLOOP *)hCLoop;
	ST_CLOOP_SETTINGS *pstSettings;
	const SF_HANDLE    sfhdl = sessQuerySysFileHdl(pstCLoop->hSession);

	// Start off with default settings
	CLoopInitHdl(hCLoop);

	pstSettings = &pstCLoop->stOrigSettings;

	ul = sizeof(pstSettings->fSendCRLF);
	sfGetSessionItem(sfhdl, SF_CLOOP_SENDCRLF, &ul, &pstSettings->fSendCRLF);

	ul = sizeof(pstSettings->fExpandBlankLines);
	sfGetSessionItem(sfhdl, SF_CLOOP_EXPAND, &ul, &pstSettings->fExpandBlankLines);

	ul = sizeof(pstSettings->fLocalEcho);
	sfGetSessionItem(sfhdl, SF_CLOOP_LOCALECHO, &ul, &pstSettings->fLocalEcho);

	ul = sizeof(pstSettings->fLineWait);
	sfGetSessionItem(sfhdl, SF_CLOOP_LINEWAIT, &ul, &pstSettings->fLineWait);

	ul = sizeof(pstSettings->chWaitChar);
	sfGetSessionItem(sfhdl, SF_CLOOP_WAITCHAR, &ul, &pstSettings->chWaitChar);

	ul = sizeof(pstSettings->fExpandTabsOut);
	sfGetSessionItem(sfhdl, SF_CLOOP_EXTABSOUT, &ul, &pstSettings->fExpandTabsOut);

	ul = sizeof(pstSettings->nTabSizeOut);
	sfGetSessionItem(sfhdl, SF_CLOOP_TABSIZEOUT, &ul, &pstSettings->nTabSizeOut);

	ul = sizeof(pstSettings->nLineDelay);
	sfGetSessionItem(sfhdl, SF_CLOOP_LINEDELAY, &ul, &pstSettings->nLineDelay);

	ul = sizeof(pstSettings->nCharDelay);
	sfGetSessionItem(sfhdl, SF_CLOOP_CHARDELAY, &ul, &pstSettings->nCharDelay);

	ul = sizeof(pstSettings->fAddLF);
	sfGetSessionItem(sfhdl, SF_CLOOP_ADDLF, &ul, &pstSettings->fAddLF);

	ul = sizeof(pstSettings->fASCII7);
	sfGetSessionItem(sfhdl, SF_CLOOP_ASCII7, &ul, &pstSettings->fASCII7);

	ul = sizeof(pstSettings->fEchoplex);
	sfGetSessionItem(sfhdl, SF_CLOOP_ECHOPLEX, &ul, &pstSettings->fEchoplex);

	ul = sizeof(pstSettings->fShowHex);
	sfGetSessionItem(sfhdl, SF_CLOOP_SHOWHEX, &ul, &pstSettings->fShowHex);

	ul = sizeof(pstSettings->nTabSizeIn);
	sfGetSessionItem(sfhdl, SF_CLOOP_TABSIZEIN, &ul, &pstSettings->nTabSizeIn);

	ul = sizeof(pstCLoop->fDoMBCS);
	sfGetSessionItem(sfhdl, SF_CLOOP_DBCS, &ul, &pstCLoop->fDoMBCS);

	pstCLoop->stWorkSettings = pstCLoop->stOrigSettings;
	return 0;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CLoopSaveHdl
 *
 * DESCRIPTION:
 *	Saves CLoop user settings in the session file if any changes have
 *	been made.
 *
 * ARGUMENTS:
 *	hCLoop -- The CLoop handle created by CLoopCreateHandle
 *
 * RETURNS:
 *	nothing
 */
int CLoopSaveHdl(const HCLOOP hCLoop)
	{
	ST_CLOOP *pstCLoop = (ST_CLOOP *)hCLoop;
	ST_CLOOP_SETTINGS *pstSettings;
	SF_HANDLE hSF;
	#define PutItem(v,i) sfPutSessionItem(hSF, i, sizeof(v), &(v))


	hSF = sessQuerySysFileHdl(pstCLoop->hSession);

	EnterCriticalSection(&pstCLoop->csect);

	// If any settings have changed, write all settings out to session file
	// Outgoing line endings
#if 1
	// Decided on 3/23 to modify design so handles will write out all
	// settings and session file routiines will decide what has changed
	if (TRUE)
#else
	if (memcmp(&pstCLoop->stWorkSettings, &pstCLoop->stOrigSettings,
		   sizeof(pstCLoop->stWorkSettings)) != 0)
#endif
		{
		pstSettings = &pstCLoop->stWorkSettings;

		PutItem(pstSettings->fSendCRLF, SF_CLOOP_SENDCRLF);
		PutItem(pstSettings->fExpandBlankLines, SF_CLOOP_EXPAND);
		PutItem(pstSettings->fLocalEcho, SF_CLOOP_LOCALECHO);
		PutItem(pstSettings->fLineWait, SF_CLOOP_LINEWAIT);
		PutItem(pstSettings->chWaitChar, SF_CLOOP_WAITCHAR);
		PutItem(pstSettings->fExpandTabsOut, SF_CLOOP_EXTABSOUT);
		PutItem(pstSettings->nTabSizeOut, SF_CLOOP_TABSIZEOUT);
		PutItem(pstSettings->nLineDelay, SF_CLOOP_LINEDELAY);
		PutItem(pstSettings->nCharDelay, SF_CLOOP_CHARDELAY);
		PutItem(pstSettings->fAddLF, SF_CLOOP_ADDLF);
		PutItem(pstSettings->fASCII7, SF_CLOOP_ASCII7);
		PutItem(pstSettings->fEchoplex, SF_CLOOP_ECHOPLEX);
		PutItem(pstSettings->fShowHex, SF_CLOOP_SHOWHEX);
		PutItem(pstSettings->nTabSizeIn, SF_CLOOP_TABSIZEIN);
		PutItem(pstCLoop->fDoMBCS, SF_CLOOP_DBCS);

		pstCLoop->stOrigSettings = pstCLoop->stWorkSettings;
		}

	LeaveCriticalSection(&pstCLoop->csect);
	return SF_OK;
	}

/* --- Access functions for user settings --- */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopGetXXXXX (several functions)
 *
 * DESCRIPTION:
 *	Retrieves the named user setting.
 *
 * ARGUMENTS:
 *	hCLoop -- Value returned from CLoopCreateHandle
 *
 * RETURNS:
 *	The requested user setting.
 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopSetXXXXX (several functions)
 *
 * DESCRIPTION:
 *	Sets the named user setting
 *
 * ARGUMENTS:
 *	hCLoop -- Value returned from CLoopCreateHandle
 *	<varname> -- The value to be set
 *
 * RETURNS:
 *	nothing
 */

/* ------------------------------------------------------------*/

int CLoopGetSendCRLF(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal =	((ST_CLOOP *)hCLoop)->stWorkSettings.fSendCRLF;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetSendCRLF(const HCLOOP hCLoop, const int fSendCRLF)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fSendCRLF = fSendCRLF;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetExpandBlankLines(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.fExpandBlankLines;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetExpandBlankLines(const HCLOOP hCLoop, const int fExpandBlankLines)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fExpandBlankLines = fExpandBlankLines;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetLocalEcho(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.fLocalEcho;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetLocalEcho(const HCLOOP hCLoop, const int fLocalEcho)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fLocalEcho = fLocalEcho;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetLineWait(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.fLineWait;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetLineWait(const HCLOOP hCLoop, const int fLineWait)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fLineWait = fLineWait;

	// Clear line wait flag in case option is turned off while waiting
	CLoopSndControl(hCLoop, CLOOP_RESUME, CLOOP_SB_LINEWAIT);
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

TCHAR CLoopGetWaitChar(const HCLOOP hCLoop)
	{
	TCHAR chVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	chVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.chWaitChar;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return chVal;
	}

void CLoopSetWaitChar(const HCLOOP hCLoop, TCHAR chWaitChar)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.chWaitChar = chWaitChar;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetExpandTabsOut(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.fExpandTabsOut;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetExpandTabsOut(const HCLOOP hCLoop, const int fExpandTabsOut)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fExpandTabsOut = fExpandTabsOut;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetTabSizeOut(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.nTabSizeOut;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetTabSizeOut(const HCLOOP hCLoop, const int nTabSizeOut)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.nTabSizeOut = nTabSizeOut;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetLineDelay(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal =	((ST_CLOOP *)hCLoop)->stWorkSettings.nLineDelay;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetLineDelay(const HCLOOP hCLoop, const int nLineDelay)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.nLineDelay = nLineDelay;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetCharDelay(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.nCharDelay;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetCharDelay(const HCLOOP hCLoop, const int nCharDelay)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.nCharDelay = nCharDelay;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetAddLF(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.fAddLF;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetAddLF(const HCLOOP hCLoop, const int fAddLF)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fAddLF = fAddLF;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetASCII7(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.fASCII7;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetASCII7(const HCLOOP hCLoop, const int fASCII7)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fASCII7 = fASCII7;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetEchoplex(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.fEchoplex;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetEchoplex(const HCLOOP hCLoop, const int fEchoplex)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fEchoplex = fEchoplex;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

#if 0
int CLoopGetWrapLines(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.fWrapLines;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetWrapLines(const HCLOOP hCLoop, const int fWrapLines)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fWrapLines = fWrapLines;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}
#endif

/* ------------------------------------------------------------*/

int CLoopGetShowHex(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.fShowHex;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetShowHex(const HCLOOP hCLoop, const int fShowHex)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.fShowHex = fShowHex;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/

int CLoopGetTabSizeIn(const HCLOOP hCLoop)
	{
	int iVal;

	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	iVal = ((ST_CLOOP *)hCLoop)->stWorkSettings.nTabSizeIn;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return iVal;
	}

void CLoopSetTabSizeIn(const HCLOOP hCLoop, const int nTabSizeIn)
	{
	assert(hCLoop);
	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	((ST_CLOOP *)hCLoop)->stWorkSettings.nTabSizeIn = nTabSizeIn;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	}

/* ------------------------------------------------------------*/
