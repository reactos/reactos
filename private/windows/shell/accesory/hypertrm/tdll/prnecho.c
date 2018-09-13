/*	File: D:\wacker\tdll\prnecho.c (Created: 24-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */
//#define DEBUGSTR 1

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "session.h"
#include "assert.h"
#include "print.h"
#include "print.hh"
#include "tdll.h"
#include "tchar.h"
#include "mc.h"

//#define DEBUGSTR

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printEchoChar
 *
 * DESCRIPTION:
 *	Prints a single character by formating it for printString().
 *
 * ARGUMENTS:
 *	HPRINT		hPrint	- The external print handle.
 *	ECHAR		tChar	- The character to print.
 *
 * RETURNS:
 *	TRUE = OK, FALSE = error
 *
 */
int printEchoChar(const HPRINT hPrint, const ECHAR tChar)
	{
	HHPRINT hhPrint = (HHPRINT)hPrint;
	TCHAR *pszTemp;
	int	nByteCnt;

	if ((hhPrint->nFlags & PRNECHO_IS_ON) == 0 ||
			(hhPrint->nFlags & PRNECHO_PAUSE))
		return FALSE;

/*
	if ((hhPrint->nFlags & PRNECHO_CHARS) == 0)
		return FALSE;
*/

	// A bit of history.  Originally I just sent the character along
	// to the PrintString function.  However, this generated a large
	// metafile that caused Windows to crash and burn.	Microsoft
	// suggested banding but that was more work than I wanted.	So the
	// kludge fix is to gather them-there characters into an array and
	// flush-um out when the time comes.

	hhPrint->achPrnEchoLine[hhPrint->nLnIdx++] = tChar;

	if (tChar == ETEXT('\n') ||
		hhPrint->nLnIdx >= (int)((sizeof(hhPrint->achPrnEchoLine) / sizeof(ECHAR)) - 1))
		{
		// Force LF.
		//
		hhPrint->achPrnEchoLine[hhPrint->nLnIdx-1] = ETEXT('\n');

		// Convert over to a MBCS string for printing
		pszTemp = malloc(sizeof(hhPrint->achPrnEchoLine)+sizeof(ECHAR));

		nByteCnt = CnvrtECHARtoMBCS(pszTemp, sizeof(hhPrint->achPrnEchoLine),
									hhPrint->achPrnEchoLine,
									hhPrint->nLnIdx * sizeof(ECHAR));

		// Make sure that the string is NULL terminated.
		pszTemp[nByteCnt] = '\0';
		DbgOutStr("%s",pszTemp,0,0,0,0);
		printString(hhPrint, pszTemp, StrCharGetByteCount(pszTemp));
		free(pszTemp);
		pszTemp = NULL;

		hhPrint->nLnIdx = 0;									

		ECHAR_Fill(hhPrint->achPrnEchoLine,
					ETEXT('\0'),
					sizeof(hhPrint->achPrnEchoLine)/sizeof(ECHAR));
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printEchoString
 *
 * DESCRIPTION:
 *	External interface to print a string.
 *
 * ARGUMENTS:
 *	HPRINT		hPrint		- The external print handle.
 *	ECHAR 	   *achPrintStr - A pointer to an array of characters string.
 *							  Include "\r\n" to finish a line.
 *	int 		iLen		- The number of characters to print.
 *
 * RETURNS:
 *	TRUE on success.
 *
 */
int printEchoString(HPRINT hPrint, ECHAR *achPrintStr, int iLen)
	{
	HHPRINT hhPrint = (HHPRINT)hPrint;
	TCHAR *pszTemp;
	int nByteCnt;
	int nRet = 0;

	if ((hhPrint->nFlags & PRNECHO_IS_ON) == 0 ||
			(hhPrint->nFlags & PRNECHO_PAUSE))
		return FALSE;

	if ((hhPrint->nFlags & PRNECHO_LINES) == 0)
		return FALSE;

	pszTemp = malloc((unsigned int)iLen * sizeof(ECHAR));
	nByteCnt = CnvrtECHARtoMBCS(pszTemp, (unsigned int)iLen * sizeof(ECHAR),
						achPrintStr, (unsigned int)iLen * sizeof(ECHAR));

	// Make sure that the string is NULL terminated.
	pszTemp[nByteCnt] = '\0';
	DbgOutStr("%s",pszTemp,0,0,0,0);
	nRet = printString(hhPrint, pszTemp, nByteCnt);
	free(pszTemp);
	pszTemp = NULL;

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printEchoScreen
 *
 * DESCRIPTION:
 *	Really the same func as printEchoString but checks if we are
 *	in screen mode.  This keeps the PrintEchoString from intermixing
 *	lines in the output.
 *
 * ARGUMENTS:
 *
 *	HPRINT		hPrint		- The external print handle.
 *	ECHAR 	   *pszPrintStr - A pointer to NULL terminated string.
 *							  Include "\r\n" to finish a line.
 * RETURNS:
 *	TRUE = OK, FALSE = error.
 *
 */
int printEchoScreen(HPRINT hPrint, ECHAR *achPrintStr, int iLen)
	{
	HHPRINT hhPrint = (HHPRINT)hPrint;
	TCHAR *pszTemp;
	int nByteCnt;
	int nRet;

	if ((hhPrint->nFlags & PRNECHO_IS_ON) == 0 ||
			(hhPrint->nFlags & PRNECHO_PAUSE))
		return FALSE;

	if ((hhPrint->nFlags & PRNECHO_SCREENS) == 0)
		return FALSE;

	pszTemp = malloc((unsigned int)iLen * sizeof(ECHAR));
	nByteCnt = CnvrtECHARtoMBCS(pszTemp, (unsigned int)iLen * sizeof(ECHAR),
					achPrintStr, (unsigned int)iLen * sizeof(ECHAR));

	// Make sure that the string is NULL terminated.
	pszTemp[nByteCnt] = '\0';
	DbgOutStr("%s",pszTemp,0,0,0,0);
	nRet = printString(hhPrint, pszTemp, (unsigned int)nByteCnt);
	free(pszTemp);
	pszTemp = NULL;

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printEchoStart
 *
 * DESCRIPTION:
 *	The print echo functions are general purpose and not limited to the
 *	print echo function.  However, we still need to set flags so that
 *	printing takes place.  This function was added so that host directed
 *	printing which uses the print echo functions could get started.
 *
 * ARGUMENTS:
 *	HPRINT		hPrint	- The external Print handle.
 *
 * RETURNS:
 *	TRUE		If successful,
 *	FALSE		If the external print handle is bad.
 *
 */
int printEchoStart(HPRINT hPrint)
	{
	HHPRINT hhPrint = (HHPRINT)hPrint;

	if (hPrint == 0)
		return FALSE;

	hhPrint->nFlags |= PRNECHO_IS_ON;
	hhPrint->nFlags &= ~PRNECHO_PAUSE;

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	printEchoClose
 *
 * DESCRIPTION:
 *	This function cleans up a print operation by closing the printer DC
 *	and forcing the remaining pages out to the print spooler.
 *
 * ARGUMENTS:
 *	HPRINT		hPrint	- The external Print handle.
 *
 * RETURNS:
 *	TRUE
 *
 */
int printEchoClose(HPRINT hPrint)
	{
	HHPRINT hhPrint = (HHPRINT)hPrint;
	TCHAR *pszTemp;
	int	nByteCnt;

    //
    // if there are chars in the buffer that have never been sent to the printer
    // flush them now.
    //

	if (hhPrint->nLnIdx > 0)
		{
		// Convert over to a MBCS string for printing
		pszTemp = malloc(sizeof(hhPrint->achPrnEchoLine)+sizeof(ECHAR));

		nByteCnt = CnvrtECHARtoMBCS(pszTemp, sizeof(hhPrint->achPrnEchoLine),
									hhPrint->achPrnEchoLine,
									hhPrint->nLnIdx * sizeof(ECHAR));

		// Make sure that the string is NULL terminated.
		pszTemp[nByteCnt] = '\0';
		DbgOutStr("%s",pszTemp,0,0,0,0);
		printString(hhPrint, pszTemp, StrCharGetByteCount(pszTemp));
		free(pszTemp);
		pszTemp = NULL;

		hhPrint->nLnIdx = 0;									

		ECHAR_Fill(hhPrint->achPrnEchoLine,
					ETEXT('\0'),
					sizeof(hhPrint->achPrnEchoLine)/sizeof(ECHAR));
		}

	if (hhPrint->hDC)
		{
		if (hhPrint->nStatus >= 0)
			{
			hhPrint->nStatus = EndPage(hhPrint->hDC);

			if (hhPrint->nStatus >= 0)
				hhPrint->nStatus = EndDoc(hhPrint->hDC);

			DbgOutStr("EndPage/EndDoc\r\n", 0, 0, 0, 0, 0);
			}

		printCtrlDeleteDC(hPrint);
		}

//	  hhPrint->nFlags &= ~(PRNECHO_IS_ON | PRNECHO_PAUSE);

	if (hhPrint->nStatus < 0)
		NotifyClient(hhPrint->hSession,
						EVENT_PRINT_ERROR,
						(WORD)hhPrint->nStatus);

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	PrintEchoRaw
 *
 * DESCRIPTION:
 *	Fires data directly at the printing avoiding the print driver.
 *	Certain modes of the DEC emulators use this.
 *
 * ARGUMENTS:
 *	HPRINT		hPrint		- The external print handle.
 *	ECHAR 		*pszPrintStr - The null terminated string to print.
 *
 * RETURNS:
 *	TRUE		if the string was printed successfully, otherwise
 *	FALSE
 *
 */
int printEchoRaw(HPRINT hPrint, ECHAR *pszPrintStr, int nLen)
	{
	HHPRINT hhPrint = (HHPRINT)hPrint;
	char ach[1024];
    assert( nLen <= 1024 );

	if (hhPrint->hDC == 0)
		{
		if (printOpenDC(hhPrint) == FALSE)
			{
			printEchoClose((HPRINT)hhPrint);
			return FALSE;
			}
		}

	if (hhPrint->nStatus > 0)
		{
        char * pBuffer;
        short sLength;

		CnvrtECHARtoMBCS(ach, sizeof(ach), pszPrintStr, (unsigned int)nLen);

        pBuffer = (char*) malloc( nLen + sizeof(short) + 1);
        sLength = (short)nLen;

        MemCopy( pBuffer, &sLength, sizeof( short ));
        if (nLen)
            MemCopy( pBuffer + sizeof(short), &ach, nLen );
        pBuffer[nLen + sizeof( short )] = '\0';

        hhPrint->nStatus = Escape( hhPrint->hDC, PASSTHROUGH, 0, pBuffer, NULL );
        free( pBuffer );
		pBuffer = NULL;

        //
        // if pasthrough fails then send the data through the print driver
        //

        if ( hhPrint->nStatus < 0 )
            {
            printEchoString(hPrint, pszPrintStr, nLen);
            }
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * printEchoLine
 *
 * DESCRIPTION:
 *	Adds CR LF to specified line of text and prints the line.
 *
 *
 * ARGUMENTS:
 *	hPrint		-	The external print handle.
 *	pachLine	-	A pointer to the text to print.
 *	iLen		-	The number of characters pointed to by pachLine.
 *
 * RETURNS:
 *	nothing
 */
void printEchoLine(const HPRINT hPrint, ECHAR *pachLine, int iLen)
	{
	ECHAR aech[256];
	printEchoString(hPrint, pachLine, iLen);

 	CnvrtMBCStoECHAR(aech, sizeof(aech), "\r\n",
 		(int)StrCharGetByteCount("\r\n"));

	printEchoString(hPrint, aech, sizeof(ECHAR) * 2);
	return;
	}
