/* File: C:\WACKER\xfer\cmprs1.c (Created: 20-Jan-1994)
 * created from HAWIN source file
 * cmprs1.c -- Routines to implement data compression
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
#define	BYTE unsigned char
#endif

#include "cmprs.h"
#include "cmprs.hh"

#if SHOW
// #include <stdio.h>
#endif

unsigned int usPrefixCode = 0;	   /* code representing pattern matched so far */
int mcK;					  /* character to be appended to prefix for
									next match */

int (**ppfCmprsGetfunc)(void *) = NULL;
										/* pointer to the
										pointer to a function used by calling
										routine */

int (*pfCmprsGetChar)(void *);
										/* pointer to the function used
										internally to get data to compress */
void *pPsave;

long *plCmprsLoadcnt;
long lCmprsBegcnt;
long lCmprsLimitcnt = 1L;	   // Initializing to one disables compression
							   //  shut-down unless changed
struct s_cmprs_node *pstCmprsTbl;  /* pointer to compression lookup table */

#define NODE_CAST struct s_cmprs_node *

int lookup_code(void);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * compress_start
 *
 * DESCRIPTION
 *	This function is called to begin data compression. The calling routine
 *	should set up a pointer to a function through which it will make calls
 *	to get characters of data. The pointer should be initialized to point
 *	to the function that the compression routines should use to get raw
 *	data for compression. The pointer is then modified by the compression
 *	routines to point to the compressor. After compression is complete or
 *	abandoned, the pointer is restored to its original value.
 *	Example of calling sequence:
 *		int (*xgetc)();
 *		int fgetc();
 *
 *		xgetc = fgetc;
 *		if (compress_start(&xgetc))
 *			;
 *	If fPauses is TRUE, the compressor will flush existing data through when
 *	the input function returns an EOF but will not shutdown. Whenever the
 *	next non-EOF is retrieved, compression will resume where it left off will
 *	the pattern table still intact. The fPauses flag must be used by both
 *	the compression and decompression routines to work. If fPauses is used,
 *	the cmprs_stop() function must be used to shut compression down before
 *	compress_disable() is called.
 *
 * RETURN VALUE
 *	Returns TRUE if memory is available for table storage and at least one
 *	character is available from input; FALSE otherwise.
 */
int compress_start(int (**getfunc)(void *),
					void *pP,
					long *loadcnt,
					int fPauses)
	{
#if FALSE
#if !defined(LZTEST)
	long x;
#endif
#endif

	if (!compress_enable())
		return(FALSE);

	fFlushable = fPauses;

	fxLastBuildGood = FALSE;	 /* By setting this FALSE, we will cause
								 * compression to shut down if the very first
								 * table build indicates that compression is
								 * not effective. Thereafter, it will take two
								 * consecutive bad builds to shut it down.
								 */

	if ((plCmprsLoadcnt = loadcnt) != NULL && !fFlushable)
		{
		lCmprsBegcnt = *plCmprsLoadcnt;
		/*
		 *	Compressability of files can be roughly measured by how many input
		 *	characters must be read before the pattern table fills up. The
		 *	lower the number, the less efficient compression is. This
		 *	calculation determines a cutoff point for any combination of
		 *	machine speed and transfer rate based on experimental trials.
		 *
		 *	Note that this mechanism should not be used when the fPauses
		 *	parameter is TRUE because the decompressor would misinterpret
		 *	the data following the STOPCODE after compression shut down
		 */
#if FALSE
#if !defined(LZTEST)
		if ((x = (cnfg.bit_rate / cpu_speed())) == 0L)
			lCmprsLimitcnt = 4300L;
		else
			lCmprsLimitcnt = max(x * 774L - 500L, 4300L);
#else
		lCmprsLimitcnt = 4300L;
#endif
#endif
		lCmprsLimitcnt = 4300L;
		}
	pPsave = pP;
	ppfCmprsGetfunc = getfunc;
	pfCmprsGetChar = *ppfCmprsGetfunc;
	if ((mcK = (*pfCmprsGetChar)(pPsave)) != EOF)
		{

		*ppfCmprsGetfunc = cmprs_getc;
		cmprs_inittbl();
		ulHoldReg = 0L;
		ulHoldReg |= CLEARCODE;
		sBitsLeft = sCodeBits;
		usxCmprsStatus = COMPRESS_ACTIVE;

		#if SHOW
			printf("C %02X                      (starting, emit CLEARCODE)\n",
					mcK);
			printf("C -> %03X     %08lX,%2d\n", CLEARCODE,
					ulHoldReg, sBitsLeft);
		#endif

		return(TRUE);
		}
	else
		{
		ppfCmprsGetfunc = NULL;
		return(FALSE);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * compress_stop
 *
 * DESCRIPTION
 *	If compression has been started, it is turned off.
 */
void compress_stop(void)
	{
	#if SHOW
		printf("C Compress_stop\n");
	#endif

	if (ppfCmprsGetfunc != NULL)
		{
		*ppfCmprsGetfunc = pfCmprsGetChar;
		ppfCmprsGetfunc = NULL;
		}
	usxCmprsStatus = COMPRESS_IDLE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * cmprs_inittbl
 *
 * DESCRIPTION
 *	Used to initialize the lookup table used for compressing data.
 */
void cmprs_inittbl(void)
	{
	register INT iCount;

	sCodeBits = 9;
	usMaxCode = 512;
	usFreeCode = FIRSTFREE;

	// pstCmprsTbl = (struct s_cmprs_node *)(OFFSETOF(compress_tblspace));
	pstCmprsTbl = (struct s_cmprs_node *)(compress_tblspace);

	for (iCount = 0; iCount < FIRSTFREE; ++iCount)
		pstCmprsTbl[iCount].first = pstCmprsTbl[iCount].next = NULL;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * cmprs_shutdown
 *
 * DESCRIPTION
 *	This is the function that is installed by cmprs_getc when compression
 *	is ending. It is installed after cmprs_getc encounters the end of the input
 *	data. This function returns any remaining bytes, then returns EOF and
 *	restores the original getc function
 *
 * RETURN VALUE
 *	Returns the next code to be sent or EOF.
 */
int cmprs_shutdown(void *pX)
	{
	int mcRetCode;

	// If we haven't sent all the data yet, do so
	if (sBitsLeft > 0)
		{
		mcRetCode = (int)(ulHoldReg & 0x00FF);
		ulHoldReg >>= 8;
		sBitsLeft -= 8;

		#if SHOW
			printf("C        %02X  %08lX,%2d  Draining ulHoldReg\n",
					mcRetCode, ulHoldReg, sBitsLeft);
		#endif
		}
	else
		{
		// No more data waiting.
		mcRetCode = EOF;
		sBitsLeft = 0;

		if (!fFlushable)
			{
			// Not flushable, get compression out of the chain
			*ppfCmprsGetfunc = pfCmprsGetChar;
			ppfCmprsGetfunc = NULL;
			#if SHOW
				printf("                          !fFlushable, outta here\n");
			#endif
			}
		else
			{
			// Flushable, see whether we should resume compression
			if ((mcK = (*pfCmprsGetChar)(pPsave)) != EOF)
				{
				#if SHOW
					printf("C %02X                      fFlushable TRUE, restarting\n",
							mcK);
				#endif
				*ppfCmprsGetfunc = cmprs_getc;
				mcRetCode = cmprs_getc(pPsave);
				}
			}
		}
	return(mcRetCode);
	}


#if !USE_ASM
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * cmprs_getc
 *
 * DESCRIPTION
 *	This is the function installed by compress_start to be used by any routine
 *	that needs compressed data. It delivers bytes to the calling routine,
 *	but may read several characters from the input to do so.
 *
 * RETURN VALUE
 *	Returns next 8-bits of compressed data or EOF if no more is available.
 */
int cmprs_getc(void *pX)
	{
	int mcRetCode;
	int fBuildGood;

	if (sBitsLeft < 8)
		{
		usPrefixCode = (unsigned int)mcK;
		do
			{
			if ((mcK = (*pfCmprsGetChar)(pPsave)) == EOF)
				{
				/* at end of file, send last code followed by STOPCODE */
				/*	to stop decompression. Note that ulHoldReg may overflow */
				/*	if the maximum code size is greater than 12 bits */
				ulHoldReg |= ((unsigned long)usPrefixCode << sBitsLeft);
				sBitsLeft += sCodeBits;

				#if SHOW
					printf("C -1                      Shutdown,"
							" emit prefix and STOPCODE\n");
					printf("C -> %03X     %08lX,%2d  Codebits=%d\n",
							usPrefixCode, ulHoldReg, sBitsLeft, sCodeBits);
				#endif

				// If we're poised to switch to the next larger code size,
				// the decompressor will do so after the prior code, so
				// we should switch now too.
				if (usFreeCode >= usMaxCode && sCodeBits < MAXCODEBITS)
					{
					++sCodeBits;
					usMaxCode *= 2;
					#if SHOW
						printf("C                         "
								"New sCodeBits = %d (anticipating)\n",
								sCodeBits);
					#endif
					}

				usPrefixCode = STOPCODE;
				*ppfCmprsGetfunc = cmprs_shutdown;
				usxCmprsStatus = COMPRESS_IDLE;

				break;	/* let last code go out */
				}
			} while (lookup_code());
		ulHoldReg |= ((unsigned long)usPrefixCode << sBitsLeft);
		sBitsLeft += sCodeBits;
		#if SHOW
			printf("C -> %03X     %08lX,%2d  Codebits=%d\n", usPrefixCode,
					ulHoldReg, sBitsLeft, sCodeBits);
		#endif
		}
	mcRetCode = (int)(ulHoldReg & 0x00FF);
	ulHoldReg >>= 8;
	sBitsLeft -= 8;

	#if SHOW
		printf("C        %02X  %08lX,%2d\n", mcRetCode, ulHoldReg, sBitsLeft);
	#endif

	if (usFreeCode > usMaxCode)
		{
		/* We've used up all available codes at the current codesize */

		if (sCodeBits >= MAXCODEBITS)
			{
			/* We've filled the pattern table, either shut down or clear the
			 *	table and build a new one.
			 */

			fBuildGood = TRUE;
			if (plCmprsLoadcnt &&
					(*plCmprsLoadcnt - lCmprsBegcnt) < lCmprsLimitcnt)
				fBuildGood = FALSE;

			#if SHOW
			printf("C                         Table full, fBuildGood = %d\n",
					fBuildGood);
			#endif
			/* if two ineffective builds in a row (or if the very first build
			 *	is ineffective, shut compression down.
			 */

			if (!fBuildGood && !fxLastBuildGood)
				{
				/* compression is not effective, shut it down */

				ulHoldReg |= ((unsigned long)STOPCODE << sBitsLeft);
				sBitsLeft += sCodeBits;
				#if SHOW
					printf("C -> %03X     %08lX,%2d  Ineffective, emitting STOPCODE\n",
							STOPCODE, ulHoldReg, sBitsLeft);
				#endif
				*ppfCmprsGetfunc = cmprs_shutdown;
				usxCmprsStatus = COMPRESS_SHUTDOWN;
				}
			else
				{
				/* clear the table and build a new one in case the nature of
				 *	the data changes.
				 */
				ulHoldReg |= ((unsigned long)CLEARCODE << sBitsLeft);
				sBitsLeft += sCodeBits;
				#if SHOW
					printf("C -> %03X     %08lX,%2d  New table, emiting CLEARCODE\n",
							CLEARCODE, ulHoldReg, sBitsLeft);
				#endif
				cmprs_inittbl();
				lCmprsBegcnt = *plCmprsLoadcnt;
				}
			fxLastBuildGood = fBuildGood;
			}
		else
			{
			/* code size hasn't maxed out yet, bump to next larger code size */

			++sCodeBits;
			usMaxCode *= 2;
			#if SHOW
				printf("C                         New sCodeBits = %d, usMaxCode = %03X\n",
						sCodeBits, usMaxCode);
			#endif
			}
		}
	return(mcRetCode);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * lookup_code
 *
 * DESCRIPTION
 *	This is a 'C' language version of the table lookup routine. It is used
 *	when an internal lookup table is being used. An assembly language version
 *	is used if an external lookup table is being used.
 *	Given a current usPrefixCode and input character, this function
 *	attempts to find a new usPrefixCode for the combined pattern in the table.
 *	If so, it updates the usPrefixCode and returns TRUE. If the pattern is
 *	not found, it adds the combination to the table and returns FALSE.
 *
 * RETURN VALUE
 *	TRUE if usPrefixCode:mcK is found in the table. FALSE if not.
 */
int lookup_code(void)
	{
	int firstflag;
	struct s_cmprs_node *tptr = (NODE_CAST)&pstCmprsTbl[usPrefixCode];
	struct s_cmprs_node *newptr;


	firstflag = TRUE;
	if (tptr->first != NULL)
		{
		firstflag = FALSE;
		tptr = tptr->first;
		for (;;)
			{
			if (tptr->cchar == (BYTE)mcK)
				{
				usPrefixCode = (unsigned int)(tptr - (NODE_CAST)(&pstCmprsTbl[0]));

				#if SHOW
					printf("C %02X                      ->(%03X)\n",
							mcK, usPrefixCode);
				#endif

				return(TRUE);
				}
			if (tptr->next == NULL)
				break;
			else
				tptr = tptr->next;
			}
		}
	if (usFreeCode < MAXNODES)
		{
		#if SHOW
			printf("C %02X                      Added %03X = %03X + %02X\n",
					mcK, usFreeCode, usPrefixCode, mcK);
		#endif
		newptr = (NODE_CAST)&pstCmprsTbl[usFreeCode++];
		if (firstflag)
			tptr->first = newptr;
		else
			tptr->next = newptr;
		newptr->first = newptr->next = NULL;
		newptr->cchar = (BYTE)mcK;
		}
	else
		++usFreeCode;	 /* triggers clearing and rebuilding of table */
	return(FALSE);
	}

#endif

/* end of cmprs1.c */
