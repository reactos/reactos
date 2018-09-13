/* File: C:\WACKER\xfer\cmprs2.c (Created: 20-Jan-1994)
 * created from HAWIN source file
 * cmprs2.c -- Routines to implement data decompression
 *
 *	Copyright 1989,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */
#include <windows.h>

#include <tdll\stdtyp.h>

#if !defined(BYTE)
#define	BYTE	unsigned char
#endif

#include "cmprs.h"
#include "cmprs.hh"

#if SHOW
//	#include <stdio.h>
#endif

/* * * * * * * * * * * * * *
 * Decompression routines  *
 * * * * * * * * * * * * * */

typedef struct s_dcmp_node DCMP_NODE;
struct s_dcmp_node
	{
	DCMP_NODE *pstLinkBack;
	DCMP_NODE *pstLinkFwd;
	BYTE ucChar;
	};

#define NODE_CAST DCMP_NODE *

DCMP_NODE *pstDcmpTbl;			// pointer to lookup table
DCMP_NODE *pstCode = NULL;		// used to scan table for output

int (**ppfDcmpPutfunc)(void *, int); /* ptr. to ptr. to function used
											   by calling func */
int (*pfDcmpPutChar)(void *, int);	/* ptr. to function used
											   internally to get data */

void *pPsave;

DCMP_NODE *pstTblLimit = NULL;	/* pointer to table beyond 1st 256 nodes */
DCMP_NODE *pstExtraNode = NULL;	/* pointer to additional node used in spec. case */
int	 fDcmpError;					/* set TRUE if illegal code is received */
int	 fStartFresh = FALSE;
unsigned int	 usCodeMask;					/* mask to isolate varible sized codes */
unsigned int	 usOldCode; 					/* last code received */
int mcFirstChar;					/* final character of pattern readout, actually,
										   the FIRST character of pattern (characters
										   are read out in reverse order */

// #pragma optimize("lgea",on)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: decompress_start
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int decompress_start(int (**put_func)(void *, int),
					void *pP,
					int fPauses)
	{
	unsigned int usCount;
	DCMP_NODE *pstTmp;

	if (!compress_enable())
		return(FALSE);

	pPsave = pP;

	fFlushable = fPauses;

	// Due to the use of based pointers, we must use compress_tblspace + 1
	//	in the following code. Otherwise, node 0 (which could have an offset
	//	of 0 looks like a NULL pointer.
	// pstDcmpTbl = (DCMP_NODE *)(OFFSETOF(compress_tblspace) + 1);
	pstDcmpTbl = (DCMP_NODE *)(compress_tblspace);

	pstCode = NULL;
	pstExtraNode = (NODE_CAST)&pstDcmpTbl[MAXNODES];   /* last node */
	pstExtraNode->pstLinkFwd = NULL;
	pstTblLimit = (NODE_CAST)&pstDcmpTbl[256];
	for (usCount = 0, pstTmp = pstDcmpTbl; usCount < 256; ++usCount)
		{
		pstTmp->ucChar = (BYTE)usCount;
		++pstTmp;
		}

	ulHoldReg = 0;
	sBitsLeft = 0;
	sCodeBits = 9;
	usMaxCode = 512;
	usCodeMask = (1 << sCodeBits) - 1;
	usFreeCode = FIRSTFREE;
	fDcmpError = FALSE;
	fStartFresh = FALSE;
	ppfDcmpPutfunc = put_func;
	pfDcmpPutChar = *ppfDcmpPutfunc;
	*ppfDcmpPutfunc = dcmp_putc;
	usxCmprsStatus = COMPRESS_ACTIVE;
	#if SHOW
		printf("D                         decompress_start, sCodeBits=%d\n",
				sCodeBits);
	#endif
	return(TRUE);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: decompress_error
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int decompress_error(void)
	{
	return(fDcmpError);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: dcmp_start
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int dcmp_start(void *pX, int mcStartChar)
	{
	unsigned int usCode;

	ulHoldReg |= ((unsigned long)mcStartChar << sBitsLeft);
	sBitsLeft += 8;
	#if SHOW
		printf("D %02X         %08lX,%2d  dcmp_start\n", mcStartChar,
				ulHoldReg, sBitsLeft);
	#endif
	if (sBitsLeft >= sCodeBits)
		{
		usCode = (unsigned int)ulHoldReg & usCodeMask;
		ulHoldReg >>= sCodeBits;
		sBitsLeft -= sCodeBits;
		#if SHOW
			printf("D >> %03X     %08lX,%2d  sCodeBits=%d dcmp_start\n",
					usCode, ulHoldReg, sBitsLeft, sCodeBits);
		#endif
		/* Table has just been cleared, code must be in range of 0 - 255 */
		if (!IN_RANGE((INT)usCode, 0, 255))
			{
			#if SHOW
				printf("D >> %03X                  ERROR: out of range\n",
						usCode);
			#endif
			return(dcmp_abort());
			}
		else
			{
			#if SHOW
				printf("D        %02X               dcmp_start\n", usCode);
			#endif
			mcStartChar = (*pfDcmpPutChar)(pPsave, mcFirstChar =
					(INT)(usOldCode = usCode));
			*ppfDcmpPutfunc = dcmp_putc;
			}
		}
	return(mcStartChar);
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: dcmp_putc
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int dcmp_putc(void *pX, int mcInput)
	{
	unsigned int usCode;
	unsigned int usInCode;
	int  mcPutResult;
	DCMP_NODE *pstTmp;

	ulHoldReg |= ((unsigned long)mcInput << sBitsLeft);
	sBitsLeft += 8;
	#if SHOW
		printf("D %02X         %08lX,%2d\n", mcInput, ulHoldReg, sBitsLeft);
	#endif
	if (sBitsLeft >= sCodeBits)
		{
		usCode = (unsigned int)ulHoldReg & usCodeMask;
		ulHoldReg >>= sCodeBits;
		sBitsLeft -= sCodeBits;
		#if SHOW
			printf("D >> %03X     %08lX,%2d  sCodeBits=%d\n",
					usCode, ulHoldReg, sBitsLeft, sCodeBits);
		#endif
		if (usCode == STOPCODE)
			{
			if (!fFlushable)
				decompress_stop();
			else
				{
				// Pause in the data, leave lookup table intact but start
				//	receiving a fresh stream.
				sBitsLeft = 0;
				ulHoldReg = 0L;
				fStartFresh = TRUE;
				#if SHOW
					printf("D            %08lX,%2d  setting fFreshStart\n",
							ulHoldReg, sBitsLeft);
				#endif
				}
			}
		else if (usCode == CLEARCODE)
			{
			sCodeBits = 9;
			usMaxCode = 512;
			usCodeMask = (1 << sCodeBits) - 1;
			usFreeCode = FIRSTFREE;
			*ppfDcmpPutfunc = dcmp_start;
			#if SHOW
				printf("D                         CLEARCODE, sCodeBits=%d\n",
						sCodeBits);
			#endif
			}
		else if (usCode > (unsigned int)usFreeCode)
			{
			#if SHOW
				printf("D                         ERROR: usCode > usFreeCode of %03X\n",
						usFreeCode);
			#endif
			return(dcmp_abort());
			}
		else
			{
			pstCode = (NODE_CAST)&pstDcmpTbl[usInCode = usCode];
			if (usCode == usFreeCode)  /* spec. case k<w>k<w>k */
				{
				pstCode = (NODE_CAST)&pstDcmpTbl[usCode = usOldCode];
				pstExtraNode->ucChar = (BYTE)mcFirstChar;
				pstCode->pstLinkFwd = pstExtraNode;
				#if SHOW
					printf("D                         Special case: k<w>k<w>k\n");
				#endif
				}
			else
				pstCode->pstLinkFwd = NULL;
			while(pstCode > pstTblLimit)
				{
				pstCode->pstLinkBack->pstLinkFwd = pstCode;
				pstCode = pstCode->pstLinkBack;
				}
			mcFirstChar = pstCode->ucChar;

			if (!fStartFresh)
				{
				#if SHOW
					printf("D                         D Added %03X = %03X + %02X\n",
							usFreeCode, usOldCode, mcFirstChar);
				#endif
				if (usFreeCode < MAXNODES)
					{
					pstTmp = (NODE_CAST)&pstDcmpTbl[usFreeCode++];
					pstTmp->ucChar = (BYTE)mcFirstChar;
					pstTmp->pstLinkBack = (NODE_CAST)&pstDcmpTbl[usOldCode];
					}
				}
			fStartFresh = FALSE;

			usOldCode = usInCode;
			if (usFreeCode >= usMaxCode && sCodeBits < MAXCODEBITS)
				{
				++sCodeBits;
				usCodeMask = (1 << sCodeBits) - 1;
				usMaxCode *= 2;
				#if SHOW
					printf("D                         D New sCodeBits = %d\n",
							sCodeBits);
				#endif
				}

			while (pstCode != NULL)
				{
				#if SHOW
					printf("D        %02X               ", pstCode->ucChar);
				#endif
				if ((mcPutResult = (*pfDcmpPutChar)(pPsave, pstCode->ucChar)) < 0)
					{
					if (mcPutResult == DCMP_UNFINISHED)
						{
						#if SHOW
							printf("Interrupted");
						#endif
						pstCode = pstCode->pstLinkFwd;	 //  to pick up later
						mcInput = DCMP_UNFINISHED;
						break;
						}
					else
						{
						#if SHOW
							printf("ERROR: putc returned -1");
						#endif
						pstCode = NULL;
						mcInput = ERROR;
						break;
						}
					}
				#if SHOW
					printf("\n");
				#endif
				pstCode = pstCode->pstLinkFwd;
				}
			}
		}
	return(mcInput);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: decompress_continue
 *
 * DESCRIPTION:
 *	Needed for compression in remote control. Picks up expansion of an
 *	output string after it has been interrupted.
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int decompress_continue(void)
	{
	int mcPutResult;
	int mcRetCode;

	// Deliver an initial unfinished code so routines downstream can pick
	//	up midstream if necessary
	if ((*pfDcmpPutChar)(pPsave, DCMP_UNFINISHED) == DCMP_UNFINISHED)
		return DCMP_UNFINISHED;

	// Now continue delivering any remaining expansion codes unless
	//	interrupted again
	while (pstCode != NULL)
		{
		if ((mcPutResult = (*pfDcmpPutChar)(pPsave, pstCode->ucChar)) < 0)
			{
			if (mcPutResult == DCMP_UNFINISHED)
				{
				pstCode = pstCode->pstLinkFwd;	 //  to pick up later
				mcRetCode = DCMP_UNFINISHED;
				break;
				}
			else
				{
				pstCode = NULL;
				mcRetCode = ERROR;
				break;
				}
			}
		pstCode = pstCode->pstLinkFwd;
		}

	return mcRetCode;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: dcmp_abort
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int dcmp_abort(void)
	{
	/* print error message or whatever */
	fDcmpError = TRUE;
	decompress_stop();
	return(ERROR);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: decompress_stop
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void decompress_stop(void)
	{
	#if SHOW
		printf("D                         Decompress_stop\n");
	#endif
	if (ppfDcmpPutfunc != NULL)
		{
		*ppfDcmpPutfunc = pfDcmpPutChar;
		ppfDcmpPutfunc = NULL;
		}
	usxCmprsStatus = COMPRESS_IDLE;
	}


/* end of cmprs2.c */
