/*	File: D:\WACKER\htrn_jis\htrn_jis.c (Created: 28-Aug-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */
#include <windows.h>
#include <stdlib.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
// Smart Heap has a problem with manually loaded DLL's
// so we are not using it in here.
//#include <tdll\mc.h>

#include <tdll\features.h>
#include <tdll\translat.h>
#include <tdll\session.h>
#include <tdll\globals.h>
#include <tdll\hlptable.h>
#include <tdll\sf.h>
#include <tdll\sess_ids.h>
#if defined(CHARACTER_TRANSLATION)
#include <tdll\translat.hh>
#endif
#include "htrn_jis.h"
#include "htrn_jis.hh"

#define IDS_GNRL_HELPFILE  		102
#if defined(INCL_USE_HTML_HELP)
#define IDS_HTML_HELPFILE		114
#endif

static TCHAR szHelpFileName[FNAME_LEN];

BOOL WINAPI _CRT_INIT(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);

INT_PTR CALLBACK EncodeSelectDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	transJisEntry
 *
 * DESCRIPTION:
 *	Currently, just initializes the C-Runtime library but may be used
 *	for other things later.
 *
 * ARGUMENTS:
 *	hInstDll	- Instance of this DLL
 *	fdwReason	- Why this entry point is called
 *	lpReserved	- reserved
 *
 * RETURNS:
 *	BOOL
 *
 */

static HINSTANCE hInstanceDll;

BOOL WINAPI transJisEntry(HINSTANCE hInstDll,
						DWORD fdwReason,
						LPVOID lpReserved)
	{
	hInstanceDll = hInstDll;

	// You need to initialize the C runtime if you use any C-Runtime
	// functions.

	#if defined(NDEBUG)
	return TRUE;
	#else
	return _CRT_INIT(hInstDll, fdwReason, lpReserved);
	#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
VOID *transCreateHandle(HSESSION hSession)
	{
	pstICT pstI;

	pstI = malloc(sizeof(stICT));
	if (pstI)
		{
 		memset(pstI, 0, sizeof(stICT));
		pstI->hSession = hSession;
		transInitHandle(pstI);
		}
	return (VOID *)pstI;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int transInitHandle(VOID *pHdl)
	{
	pstICT pstI;

	pstI = (pstICT)pHdl;

	if (pstI)
		{
		pstI->nInputMode  = PASS_THRU_MODE;
		pstI->nOutputMode = PASS_THRU_MODE;
		}

	return TRANS_OK;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int transLoadHandle(VOID *pHdl)
	{
	pstICT pstI;
	SF_HANDLE hSF;
	long lSize;

	pstI = (pstICT)pHdl;

	if (pstI)
		{
		hSF = sessQuerySysFileHdl(pstI->hSession);
		if (hSF)
			{
			lSize = sizeof(int);
			sfGetSessionItem(hSF,
							SFID_TRANS_FIRST,
							&lSize,
							&pstI->nInputMode);
			lSize = sizeof(int);
			sfGetSessionItem(hSF,
							SFID_TRANS_FIRST + 1,
							&lSize,
							&pstI->nOutputMode);
			}
		}

	return TRANS_OK;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int transSaveHandle(VOID *pHdl)
	{
	pstICT pstI;
	SF_HANDLE hSF;

	pstI = (pstICT)pHdl;

	if (pstI)
		{
		hSF = sessQuerySysFileHdl(pstI->hSession);
		if (hSF)
			{
			sfPutSessionItem(hSF,
							SFID_TRANS_FIRST,
							sizeof(int),
							&pstI->nInputMode);
			sfPutSessionItem(hSF,
							SFID_TRANS_FIRST + 1,
							sizeof(int),
							&pstI->nOutputMode);
			}
		}

	return TRANS_OK;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int transDestroyHandle(VOID *pHdl)
	{
	pstICT pstI;

	pstI = (pstICT)pHdl;

	if (pstI)
		{
		free(pstI);
		pstI = NULL;
		}

	return TRANS_OK;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int transDoDialog(HWND hDlg, VOID *pHdl)
	{
	pstICT pstI;

	pstI = (pstICT)pHdl;

	if (pstI)
		{
		DialogBoxParam(
					hInstanceDll,
					"IDD_TRANSLATE",
					hDlg,
					EncodeSelectDlg,
					(LPARAM)pstI->hSession
					);
		}

	return TRANS_OK;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
static void transPumpOutString(TCHAR *cReady, int *nReady, TCHAR *cOut)
	{
	TCHAR *pszPtr;

	pszPtr = cReady + *nReady;

	while (*cOut != TEXT('\0'))
		{
		*pszPtr++ = *cOut++;
		*nReady += 1;
		}
	}
static void transPumpOutChar(TCHAR *cReady, int *nReady, TCHAR cOut)
	{
	TCHAR *pszPtr;

	pszPtr = cReady + *nReady;

	*pszPtr = cOut;
	*nReady += 1;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
static void transShiftToJisOut(struct stShiftToJis *pstS,
								TCHAR *cReady,
								int *nReady,
								TCHAR cOne,
								TCHAR cTwo)
	{
	unsigned int adjust;
	unsigned int rowOffset;
	unsigned int cellOffset;

	if (!pstS->nInTwoByteMode)
		{
		transPumpOutString(cReady,
							nReady,
							TEXT("\x1B$B"));
		pstS->nInTwoByteMode = TRUE;
		}

	adjust     = cTwo < 159;
	rowOffset  = (cOne < 160) ? 112 : 176;
	cellOffset = adjust ? ((cTwo > 127) ? 32 : 31) : 126;
	cOne = (TCHAR)(((cOne - rowOffset) << 1) - adjust);
	cTwo -= (TCHAR)cellOffset;
	transPumpOutChar(cReady, nReady, cOne);
	transPumpOutChar(cReady, nReady, cTwo);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
static void transShiftToEucOut(struct stShiftToJis *pstS,
								TCHAR *cReady,
								int *nReady,
								TCHAR cOne,
								TCHAR cTwo)
	{
	unsigned int adjust;
	unsigned int rowOffset;
	unsigned int cellOffset;

	adjust     = cTwo < 159;
	rowOffset  = (cOne < 160) ? 112 : 176;
	cellOffset = adjust ? ((cTwo > 127) ? 32 : 31) : 126;
	cOne = (TCHAR)(((cOne - rowOffset) << 1) - adjust);
	cOne |= 0x80;
	cTwo -= (TCHAR)cellOffset;
	cTwo |= 0x80;
	transPumpOutChar(cReady, nReady, cOne);
	transPumpOutChar(cReady, nReady, cTwo);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
static void transJisToShiftOut(struct stJisToShift *pstJ,
								TCHAR *cReady,
								int *nReady,
								TCHAR cOne,
								TCHAR cTwo)
	{
	unsigned int rowOffset;
	unsigned int cellOffset;

	rowOffset = (cOne < 95) ? 112 : 176;
	cellOffset = (cOne % 2) ? ((cTwo > 95) ? 32 : 31) : 126;

	cOne = (TCHAR)(((cOne + 1) >> 1) + rowOffset);
	cTwo += (TCHAR)cellOffset;
	transPumpOutChar(cReady, nReady, cOne);
	transPumpOutChar(cReady, nReady, cTwo);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
static void transEucToShiftOut(struct stJisToShift *pstJ,
								TCHAR *cReady,
								int *nReady,
								TCHAR cOne,
								TCHAR cTwo)
	{
	unsigned int rowOffset;
	unsigned int cellOffset;

	cOne &= 0x7F;
	cTwo &= 0x7F;

	rowOffset = (cOne < 95) ? 112 : 176;
	cellOffset = (cOne % 2) ? ((cTwo > 95) ? 32 : 31) : 126;

	cOne = (TCHAR)(((cOne + 1) >> 1) + rowOffset);
	cTwo += (TCHAR)cellOffset;
	transPumpOutChar(cReady, nReady, cOne);
	transPumpOutChar(cReady, nReady, cTwo);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *	cOne	-- the half width katakana character
 *	cTwo	-- the next character, maybe a MARU or NIGORI
 *
 * RETURNS:
 *	TRUE means that both characters were processed, no further action needed.
 *	FALSE means that the second character still needs to be processed.
 *
 */
static int transHalfToFullKatakana(struct stShiftToJis *pstS,
									TCHAR *cReady,
									int *nReady,
									int fJisOut,
									TCHAR cOne,
									TCHAR cTwo)
	{
	int nRet = TRUE;
	int nigori = FALSE;
	int maru = FALSE;
	int tmp;
	/*
	 * The data structure for mapping half width katakana characters to
	 * full width characters in the Shift-JIS encoding scheme.
	 */
	int mtable[][2] = {
		{129,66},{129,117},{129,118},{129,65},{129,69},{131,146},{131,64},
		{131,66},{131,68},{131,70},{131,72},{131,131},{131,133},{131,135},
		{131,98},{129,91},{131,65},{131,67},{131,69},{131,71},{131,73},
		{131,74},{131,76},{131,78},{131,80},{131,82},{131,84},{131,86},
		{131,88},{131,90},{131,92},{131,94},{131,96},{131,99},{131,101},
		{131,103},{131,105},{131,106},{131,107},{131,108},{131,109},
		{131,110},{131,113},{131,116},{131,119},{131,122},{131,125},
		{131,126},{131,128},{131,129},{131,130},{131,132},{131,134},
		{131,136},{131,137},{131,138},{131,139},{131,140},{131,141},
		{131,143},{131,147},{129,74},{129,75}
		};

	if (cTwo == 222)			/* Is it a nigori mark ? */
		{
		if (ISNIGORI(cOne))		/* Can it be modified with a NIGORI ? */
			nigori = TRUE;
		else
			nRet = FALSE;
		}
	else if (cTwo == 223)		/* Is it a maru mark ? */
		{
		if (ISMARU(cOne))		/* Can it be modified with a MARU ? */
			maru = TRUE;
		else
			nRet = FALSE;
		}
	else
		{
		/* Wasn't a nigori or a maru */
		nRet = FALSE;
		}

	tmp = cOne;

	cOne = (TCHAR)mtable[tmp - 161][0];
	cTwo = (TCHAR)mtable[tmp - 161][1];

	if (nigori)
		{
		/*
		 * Transform a kana into a kana with nigori
		 */
		if (((cTwo >= 74) && (cTwo <= 103)) ||
			((cTwo >= 110) && (cTwo <= 122)))
			{
			cTwo += 1;
			}
		else if ((cOne == 131) && (cTwo == 69))
			{
			cTwo = (TCHAR)148;
			}
		}
	if (maru)
		{
		/*
		 * Transform a kana into a kana with maru
		 */
		if ((cTwo >= 110) && (cTwo <= 122))
			{
			cTwo += 2;
			}
		}

	if (fJisOut)
		{
		transShiftToJisOut(pstS,
						cReady,
						nReady,
						cOne,
						cTwo);
		}
	else
		{
		transShiftToEucOut(pstS,
						cReady,
						nReady,
						cOne,
						cTwo);
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int transCharIn(VOID *pHdl,
				TCHAR cIn,
				int *nReady,
				int nSize,
				TCHAR *cReady)
	{
	int nOK;
	int nTemp;
	TCHAR cTest;
	pstICT pstI;
	struct stJisToShift *pstJ;

	*nReady = 0;
											
	pstI = (pstICT)pHdl;

	if (pstI)
		{
		switch(pstI->nInputMode)
			{
			case PASS_THRU_MODE:
				if (nSize > 0)
					{
					*cReady = cIn;
					*nReady = 1;
					}
				break;
			case JIS_TO_SHIFT_MODE:
				/*
				 * This code is a modified version of the code found in the
				 * book "Understanding Japanese Information Processing" by
				 * Ken Lunde.  See page 171, "Seven- to Eight-bit Conversion".
				 */
				pstJ = (struct stJisToShift *)&pstI->uIn;
				if (cIn == TEXT('\x1B'))
					{
					/* The magical escape sequence */
					nTemp = pstJ->nInTwoByteMode;
					memset(pstJ, 0, sizeof(struct stJisToShift));
					pstJ->nInTwoByteMode = nTemp;
					pstJ->nSeenEscape = TRUE;
					}
				else if (pstJ->nSeenEscape)
					{
					/*
					 * OK, this is the messy place.  Just hang on and we will
					 * get thru without too many injuries.
					 *
					 * This will need to be rewritten if the escape sequences
					 * that we look for are changed.
					 *
					 * Currently we accept:
					 *	<esc> $ B	to shift into 2 byte mode
					 *	<esc> ( J	to shift out of 2 byte mode
					 *
					 * added codes as of 18-Jan-95:
					 *	<esc> $ @	to shift into 2 byte mode
					 *	<esc> ( H	to shift out of 2 byte mode
					 *	<esc> ( B	to shift out of 2 byte mode
					 */
					pstJ->acBuffer[pstJ->nEscSeqCount] = cIn;
					pstJ->nEscSeqCount += 1;
					nOK = TRUE;
					switch(pstJ->nEscSeqCount)
						{
						case 1:
							cTest = pstJ->acBuffer[0];
							if ((cTest == TEXT('$')) || (cTest == TEXT('(')))
								break;	/* OK */
							nOK = FALSE;
							break;
						case 2:
							cTest = pstJ->acBuffer[0];
							switch (cTest)
								{
								case TEXT('$'):
									cTest = pstJ->acBuffer[1];
									switch (cTest)
										{
									case TEXT('B'):
									case TEXT('@'):
										/*
										 * Shift to two byte mode
										 */
										if (!pstJ->nInTwoByteMode)
											{
											memset(pstJ, 0,
												sizeof(struct stJisToShift));
											pstJ->nInTwoByteMode = TRUE;
											}
										break;
									default:
										nOK = FALSE;
										break;
										}
									break;
								case TEXT('('):
									cTest = pstJ->acBuffer[1];
									switch (cTest)
										{
									case TEXT('J'):
									case TEXT('H'):
									case TEXT('B'):
										/*
										 * Shift from two byte mode
										 */
										if (pstJ->nInTwoByteMode)
											{
											memset(pstJ, 0,
												sizeof(struct stJisToShift));
											pstJ->nInTwoByteMode = FALSE;
											}
										break;
									default:
										nOK = FALSE;
										break;
										}
									break;
								default:
									nOK = FALSE;
									break;
								}
							break;
						default:
							nOK = FALSE;
							break;
						}
					if (!nOK)
						{
						pstJ->acBuffer[pstJ->nEscSeqCount] = TEXT('\0');
						/*
						 * Dump out whatever it is we have seen
						 */
						if (pstJ->nSeenEscape)
							{
							transPumpOutChar(cReady, nReady, TEXT('\x1B'));
							}
						transPumpOutString(cReady,
											nReady,
											pstJ->acBuffer);
						/*
						 * For now, preserve the state of nInTwoByteMode
						 */
						nTemp = pstJ->nInTwoByteMode;
						memset(pstJ, 0, sizeof(struct stJisToShift));
						pstJ->nInTwoByteMode = nTemp;
						}
					}
				else if (pstJ->nSeenFirstCharacter)
					{
					/*
					 * Got two characters to convert and pump out
					 */
					transJisToShiftOut(pstJ,
										cReady,
										nReady,
										pstJ->chPrev,
										cIn);
					pstJ->nSeenFirstCharacter = FALSE;
					pstJ->chPrev = TEXT('\0');
					}
				else if ((cIn == TEXT('\n')) || (cIn == TEXT('\r')))
					{
					/*
					 * Switch out of two byte mode
					 */
					pstJ->nInTwoByteMode = FALSE;
					pstJ->nSeenFirstCharacter = FALSE;
					pstJ->chPrev = TEXT('\0');
					transPumpOutChar(cReady, nReady, cIn);
					}
				else
					{
					if (pstJ->nInTwoByteMode)
						{
						pstJ->nSeenFirstCharacter = TRUE;
						pstJ->chPrev = cIn;
						}
					else
						{
						/*
						 * Nothing going on, just pump out the character
						 */
						transPumpOutChar(cReady, nReady, cIn);
						}
					}
				break;
			case EUC_TO_SHIFT_MODE:
				pstJ = (struct stJisToShift *)&pstI->uOut;

				if (pstJ->nSeenFirstCharacter)
					{
					if (ISEUC(cIn))
						{
						transEucToShiftOut(pstJ,
											cReady,
											nReady,
											pstJ->chPrev,
											cIn);
						}
					else
						{
						transPumpOutChar(cReady, nReady, pstJ->chPrev);
						transPumpOutChar(cReady, nReady, cIn);
						}
					pstJ->nSeenFirstCharacter = FALSE;
					pstJ->chPrev = 0;
					}
				else if (pstJ->nHalfWidthKanaSeen)
					{
					/*
					 * Handle result of the previous case
					 */
					transPumpOutChar(cReady, nReady, cIn);
					pstJ->nHalfWidthKanaSeen = FALSE;
					pstJ->chPrev = TEXT('\0');
					}
				else if (cIn == 0x8E)
					{
					/*
					 * Set up to convert next character to half width katakana
					 */
					pstJ->nHalfWidthKanaSeen = TRUE;
					pstJ->chPrev = cIn;
					}
				else if (ISEUC(cIn))
					{
					pstJ->nSeenFirstCharacter = TRUE;
					pstJ->chPrev = cIn;
					}
				else
					{
					transPumpOutChar(cReady, nReady, cIn);
					}
				break;
			case SHIFT_TO_JIS_MODE:
			default:
				break;
			}
		}

	return TRANS_OK;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int transCharOut(VOID *pHdl,
				TCHAR cOut,
				int *nReady,
				int nSize,
				TCHAR *cReady)
	{
	pstICT pstI;
	struct stShiftToJis *pstS;
	int nFinished;

	*nReady = 0;

	pstI = (pstICT)pHdl;

restart:
	if (pstI)
		{
		switch(pstI->nOutputMode)
			{
			case PASS_THRU_MODE:
				if (nSize > 0)
					{
					*cReady = cOut;
					*nReady = 1;
					}
				break;
			case SHIFT_TO_JIS_MODE:
				/*
				 * This code is a modified version of the code found in the
				 * book "Understanding Japanese Information Processing" by
				 * Ken Lunde.  See page 170, "Eight- to Seven-bit Conversion".
				 */
				pstS = (struct stShiftToJis *)&pstI->uOut;
				if ((cOut == TEXT('\r')) || (cOut == TEXT('\n')))
					{
					if (pstS->nInTwoByteMode)
						{
						/* Output the escape to one bye sequence */
						transPumpOutString(cReady,
											nReady,
											TEXT("\x1B(J"));
					
						pstS->nInTwoByteMode = FALSE;
						}
					/* Output the end of line character */
					transPumpOutChar(cReady, nReady, cOut);
					}
				else if (pstS->nLeadByteSeen)
					{
					/* Already seen a lead byte last time thru */
					transShiftToJisOut(pstS,
									cReady,
									nReady,
									pstS->chPrev,
									cOut);

					pstS->nLeadByteSeen = FALSE;
					pstS->chPrev = TEXT('\0');
					}
				else if (pstS->nHalfWidthKanaSeen)
					{
					nFinished = transHalfToFullKatakana(pstS,
														cReady,
														nReady,
														TRUE,
														pstS->chPrev,
														cOut);
					pstS->nHalfWidthKanaSeen = FALSE;
					pstS->chPrev = TEXT('\0');
					if (!nFinished)
						goto restart;
					}
				else if (SJIS1(cOut))
					{
					/* If the character is a DBCS lead byte */
					pstS->nLeadByteSeen = TRUE;
					pstS->chPrev = cOut;
					}
				else if (HANKATA(cOut))
					{
					/* If the character is a half width katakana character */
					pstS->nHalfWidthKanaSeen = TRUE;
					pstS->chPrev = cOut;
					}
				else
					{
					if (pstS->nInTwoByteMode)
						{
						/* Output the escape to one bye sequence */
						transPumpOutString(cReady,
											nReady,
											TEXT("\x1B(J"));
						pstS->nInTwoByteMode = FALSE;
						}

					/* Output the character */
					transPumpOutChar(cReady, nReady, cOut);
					}
				break;
			case SHIFT_TO_EUC_MODE:
				/*
				 */
				pstS = (struct stShiftToJis *)&pstI->uOut;
				if (pstS->nLeadByteSeen)
					{
					/* Already seen a lead byte last time thru */
					transShiftToEucOut(pstS,
									cReady,
									nReady,
									pstS->chPrev,
									cOut);
					pstS->nLeadByteSeen = FALSE;
					pstS->chPrev = TEXT('\0');
					}
				else if (pstS->nHalfWidthKanaSeen)
					{
					nFinished = transHalfToFullKatakana(pstS,
														cReady,
														nReady,
														FALSE,
														pstS->chPrev,
														cOut);
					pstS->nHalfWidthKanaSeen = FALSE;
					pstS->chPrev = TEXT('\0');
					if (!nFinished)
						goto restart;
					}
				else if (SJIS1(cOut))
					{
					/* If the character is a DBCS lead byte */
					pstS->nLeadByteSeen = TRUE;
					pstS->chPrev = cOut;
					}
				else if (HANKATA(cOut))
					{
					/* If the character is a half width katakana character */
					transPumpOutChar(cReady, nReady, (TCHAR)0x8E);
					transPumpOutChar(cReady, nReady, cOut);
					}
				else
					{
					transPumpOutChar(cReady, nReady, cOut);
					}
				break;
			case JIS_TO_SHIFT_MODE:
			default:
				break;
			}
		}

	return TRANS_OK;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * DESCRIPTION:
 *	Header stuff for the dialog procedure that follows.
 *
 * NOTE:
 *	Most of this stuff was copied from TDLL\GENRCDLG.C along with the
 *	framework code for the dialog procedure.
 */

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
	{
	/*
	 * Put in whatever else you might need to access later
	 */
	HSESSION hSession;
	};

typedef	struct stSaveDlgStuff SDS;

// Dialog control defines...

#define	IDC_RB_SHIFT_JIS		102
#define	IDC_RB_STANDARD_JIS		103
#define IDC_PB_HELP				8

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:	EncodeSelectDlg
 *
 * DESCRIPTION: Dialog manager stub
 *
 * ARGUMENTS:	Standard Windows dialog manager
 *
 * RETURNS: 	Standard Windows dialog manager
 *
 */
INT_PTR CALLBACK EncodeSelectDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;
	static  aHlpTable[] = {IDC_PB_HELP,				IDH_HTRN_DIALOG,
							IDC_RB_SHIFT_JIS,		IDH_HTRN_SHIFTJIS,
							IDC_RB_STANDARD_JIS,	IDH_HTRN_JIS,
							0,						0};

#if defined(CHARACTER_TRANSLATION)
	HHTRANSLATE hTrans;
	pstICT pstI;
#endif

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));
		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			break;
			}

		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);

		// mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

		pS->hSession = (HSESSION)lPar;
#if defined(CHARACTER_TRANSLATION)
		hTrans = (HHTRANSLATE)sessQueryTranslateHdl(pS->hSession);
		pstI = (pstICT)hTrans->pDllHandle;

		if (pstI->nInputMode == PASS_THRU_MODE)
			{
			CheckDlgButton(hDlg, IDC_RB_SHIFT_JIS, TRUE);
			}
		else
			{
			CheckDlgButton(hDlg, IDC_RB_STANDARD_JIS, TRUE);
			}
#endif

		break;

	case WM_DESTROY:
		break;

	case WM_CONTEXTMENU:
#if defined(CHARACTER_TRANSLATION)
		LoadString(hInstanceDll, IDS_GNRL_HELPFILE, szHelpFileName,
					sizeof(szHelpFileName) / sizeof(TCHAR));

		WinHelp((HWND)wPar, szHelpFileName, HELP_CONTEXTMENU,
			 (DWORD_PTR)(LPTSTR)aHlpTable);
#endif
		break;

	case WM_HELP:
#if defined(CHARACTER_TRANSLATION)
		LoadString(hInstanceDll, IDS_GNRL_HELPFILE, szHelpFileName,
		sizeof(szHelpFileName) / sizeof(TCHAR));

		WinHelp(((LPHELPINFO)lPar)->hItemHandle, szHelpFileName,
			HELP_WM_HELP, (DWORD_PTR)(LPTSTR)aHlpTable);
#endif //CHARACTER_TRANSLATION
		break;

	case WM_COMMAND:

		/*
		 * Did we plan to put a macro in here to do the parsing ?
		 */
		DlgParseCmd(nId, nNtfy, hwndChild, wPar, lPar);

		switch (nId)
			{
		case IDOK:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			/*
			 * Do whatever saving is necessary
			 */
#if defined(CHARACTER_TRANSLATION)
			hTrans = (HHTRANSLATE)sessQueryTranslateHdl(pS->hSession);
			pstI = (pstICT)hTrans->pDllHandle;
			if (IsDlgButtonChecked(hDlg, IDC_RB_SHIFT_JIS))
				{
				pstI->nInputMode  = PASS_THRU_MODE;
				pstI->nOutputMode = PASS_THRU_MODE;
				}
			else
				{
				pstI->nInputMode  = JIS_TO_SHIFT_MODE;
				pstI->nOutputMode = SHIFT_TO_JIS_MODE;
				}
#endif

			/* Free the storeage */
			free(pS);
			pS = (SDS *)0;
			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			/* Free the storeage */
			free(pS);
			pS = (SDS *)0;
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: transQueryMode
 *
 * DESCRIPTION:
 *	
 * ARGUMENTS:
 *
 * RETURNS:
 */
int transQueryMode(VOID *pHdl)
	{
	pstICT pstI;
	int nReturn;

	pstI = (pstICT)pHdl;

	if(pstI->nInputMode	== PASS_THRU_MODE)
	   {
	   nReturn = 0;
	   }
	else
	   {
	   nReturn = 1;
	   }
	return(nReturn);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: transSetMode
 *
 * DESCRIPTION:
 *	
 * ARGUMENTS:
 *
 * RETURNS:
 */
int transSetMode(VOID *pHdl,
                 int   nMode)
   {

	pstICT pstI;


	pstI = (pstICT)pHdl;

	if (nMode == 0)
	   {
	   pstI->nInputMode  = PASS_THRU_MODE;
	   pstI->nOutputMode = PASS_THRU_MODE;
	   }
	else
	   {
	   pstI->nInputMode  = JIS_TO_SHIFT_MODE;
	   pstI->nOutputMode = SHIFT_TO_JIS_MODE;
	   }
	
	return(1);
	}



