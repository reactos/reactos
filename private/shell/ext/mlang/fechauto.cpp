/*----------------------------------------------------------------------------
	%%File: fechauto.c
	%%Unit: fechmap
	%%Contact: jpick

	Module that attempts to auto-detect encoding for a given stream.
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stddef.h>

#include "private.h"
#include "fechmap_.h"
#include "lexint_.h"

// Code marked by these #defines will be deleted eventually ...
// (It prints out useful information and statistics about how
// auto-detect is doing and what it's finding in the input).
//
#define JPDEBUG			0
#define JPDEBUG2		0
#define JPDEBUG3		0

#define NEED_NAMES		0

#if JPDEBUG || JPDEBUG2 || JPDEBUG3
#undef NEED_NAMES
#define NEED_NAMES		1
#endif

#if NEED_NAMES
static char *rgszIcetNames[icetCount] =
{
	"icetEucCn",
	"icetEucJp",
	"icetEucKr",
	"icetEucTw",
	"icetIso2022Cn",
	"icetIso2022Jp",
	"icetIso2022Kr",
	"icetIso2022Tw",
	"icetBig5",
	"icetGbk",
	"icetHz",
	"icetShiftJis",
	"icetWansung",
	"icetUtf7",
	"icetUtf8",
};
#endif

// Characters we care about
//
#define chSo		(UCHAR) 0x0e
#define chSi		(UCHAR) 0x0f
#define chEsc		(UCHAR) 0x1b

// Minimum Sample Size
//
#define	cchMinSample		64

// High-ASCII character threshold.  If this routine is unable
// to absolutely determine the encoding of this file, it will
// need to guess.  Files that are ASCII, but contain high-ASCII
// characters (e.g., a file with some Cyrillic characters) may
// confuse us.  If the number of high-ASCII characters falls
// below this threshold, return the encoding we guessed but 
// also return a special rc that says the file "might be ASCII."
//
// 5%, for now.
//
// 40%, for now, of the high-ascii characters must be in high-
// ascii pairs.  (Pulled down because of Big5 and the other
// DBCS encodings that can have trail bytes in the low range).
//
#define nHighCharThreshold		 5		// %
#define nHighPairThreshold		40		// %

// Used by CceDetermineInputTypeReturnAll() to determine whether any icet has
// high enough count to rule out all other icets.
//
#define CchCountThreshold(icet)	(((icet) == icetHz || (icet) == icetUtf7) ? 5 : 10)



// Tokens
//
// Stop tokens (negative) imply special handling and will cause
// the processing loop to stop (eof, err, si, so and esc are
// stop tokens).
//
#define xmn			  0
#define esc			(-1)
#define  so			(-2)
#define  si			(-3)
#define eof			(-4)
#define err			(-5)

#define _FStopToken(tk)		((tk) < 0)


// Masks used in _CBitsOnFromUlong()
//
#define lMaskBitCount1	(LONG) 0x55555555
#define lMaskBitCount2	(LONG) 0x33333333
#define lMaskBitCount3	(LONG) 0x0F0F0F0F
#define lMaskBitCount4	(LONG) 0x00FF00FF
#define lMaskBitCount5	(LONG) 0x0000FFFF

/* _  C  B I T S  O N  F R O M  U L O N G */
/*----------------------------------------------------------------------------
	%%Function: _CBitsOnFromUlong
	%%Contact: jpick

	(adapted from code in convio.c)
----------------------------------------------------------------------------*/
int __inline _CBitsOnFromUlong(ULONG ulBits)
{
	ulBits = (ulBits & lMaskBitCount1) + ((ulBits & ~lMaskBitCount1) >> 1);
	ulBits = (ulBits & lMaskBitCount2) + ((ulBits & ~lMaskBitCount2) >> 2);
	ulBits = (ulBits & lMaskBitCount3) + ((ulBits & ~lMaskBitCount3) >> 4);
	ulBits = (ulBits & lMaskBitCount4) + ((ulBits & ~lMaskBitCount4) >> 8);
	ulBits = (ulBits & lMaskBitCount5) + ((ulBits & ~lMaskBitCount5) >> 16);
	
	return (int)ulBits;
}
	
// Masks for the encodings
//
#define grfEucCn        (ULONG) 0x0001
#define grfEucJp        (ULONG) 0x0002
#define grfEucKr        (ULONG) 0x0004
#define grfEucTw        (ULONG) 0x0008
#define grfIso2022Cn    (ULONG) 0x0010
#define grfIso2022Jp    (ULONG) 0x0020
#define grfIso2022Kr    (ULONG) 0x0040
#define grfIso2022Tw    (ULONG) 0x0080
#define grfBig5         (ULONG) 0x0100
#define grfGbk          (ULONG) 0x0200
#define grfHz           (ULONG) 0x0400 
#define grfShiftJis     (ULONG) 0x0800
#define grfWansung      (ULONG) 0x1000
#define grfUtf7         (ULONG) 0x2000 	
#define grfUtf8         (ULONG) 0x4000

// grfAll assumes that the tests for Euc-Kr fall within those
// for Wansung (as far as I can tell from reading, Euc-Kr is a
// strict subset of Wansung).  The same for Euc-Cn and Gbk.  No
// need to test for both the subset and the whole.
//
#define grfAll              (ULONG) 0x7FFA
#define grfAllButIso2022    (ULONG) 0x7F0A
#define cAll                13				// == number bits set in grfAll
#define cAllButIso2022      9				// == number bits set in grfAllButIso2022

// Array that maps an encoding to its mask
//
static ULONG _mpicetgrf[icetCount] =
{
	grfEucCn,
	grfEucJp,
	grfEucKr,
	grfEucTw,
	grfIso2022Cn,
	grfIso2022Jp,
	grfIso2022Kr,
	grfIso2022Tw,
	grfBig5,
	grfGbk,
	grfHz,
	grfShiftJis,
	grfWansung,
	grfUtf7,
	grfUtf8,
};

// Prototypes
//
static int  _NGetNextUch(IStream *pstmIn, unsigned char *c, BOOL *lpfIsHigh);
static ICET _IcetFromIcetMask(ULONG ulMask);
static ICET _IcetDefaultFromIcetMask(ULONG ulMask);
static CCE  _CceResolveAmbiguity(ULONG grfIcet, ICET *lpicet, int nPrefCp, EFam efPref);
static CCE  _CceReadEscSeq(IStream *pstmIn, int nPrefCp, ICET *lpicet, BOOL *lpfGuess);


/* C C E  D E T E R M I N E  I N P U T  T Y P E */
/*----------------------------------------------------------------------------
	%%Function: CceDetermineInputType
	%%Contact: jpick

	Attempt to determine the appropriate ICET type for the given 
	stream.  Caller-supplied get/unget routines used for data access.
----------------------------------------------------------------------------*/
CCE CceDetermineInputType(
    IStream   *pstmIn,           // input stream
	DWORD     dwFlags,			// configuration flags
	EFam      efPref,			// optional: preferred encoding family
	int       nPrefCp,			// optional: preferred code page
	ICET     *lpicet,			// set to detected encoding
	BOOL     *lpfGuess			// set to fTrue if function "guessed"
)
{
	unsigned char uch;
	int nToken;
	CCE cceRet;
	BOOL fGuess;
	ICET icet;
	int cIcetActive;
	ULONG grfIcetActive;	// Bitarray tracks which encodings are still active candidates.
	ICET icetSeq;
	int i, nCount, nCountCurr;
	DWORD dwValFlags;
	BOOL fIsHigh;
	int cchHigh = 0;
	int cchHighPairs = 0;
	int cchTotal = 0;
	BOOL fLastHigh = fFalse;
	
#if JPDEBUG3
	ULONG grfIcetNoCommonChars;
#endif

#if JPDEBUG
	printf("flags: %d\n", dwFlags);
#endif
	
	// Initialize parsers
	//
	dwValFlags = grfCountCommonChars;
	if (dwFlags & grfDetectUseCharMapping)
		dwValFlags |= grfValidateCharMapping;
	ValidateInitAll(dwValFlags);
	
	// Initialize locals -- be optimistic
	//
	cceRet = cceSuccess;
	fGuess = fFalse;
	grfIcetActive = grfAllButIso2022;
	cIcetActive = cAllButIso2022;
	
#if JPDEBUG3
	grfIcetNoCommonChars = grfAllButIso2022;
#endif
	
	while (fTrue)
		{
		nToken = _NGetNextUch(pstmIn, &uch, &fIsHigh);
		if (_FStopToken(nToken))
			break;
			
		// Update (admittedly dumb) statistics -- really counts high
		// ascii characters in runs (not really pairs).  But threshold
		// constants (defined, above) were determined by calculating
        // exactly these numbers for ~25 files, so it should be ok (?).
		//
		++cchTotal;
		if (fIsHigh)
			{
			++cchHigh;
			if (fLastHigh)
				++cchHighPairs;
			}
		fLastHigh = fIsHigh;
			
		for (i = 0; i < icetCount; i++)
			{
			if (!(grfIcetActive & _mpicetgrf[i]) || (NValidateUch((ICET)i, uch, fFalse) != 0))
				continue;
				
			grfIcetActive &= ~_mpicetgrf[i];
			--cIcetActive;
#if JPDEBUG
			printf("Log:  Lost %s at offset 0x%.4x (%d), char 0x%.2x\n", rgszIcetNames[i], (cchTotal-1), (cchTotal-1), uch);
#endif
			}
			
#if JPDEBUG3
		for (i = 0; i < icetCount; i++)
			{
			if (!(grfIcetActive & _mpicetgrf[i]) || !(grfIcetNoCommonChars & _mpicetgrf[i]))
				continue;
				
			if (!FValidateCharCount(i, &nCount) || (nCount == 0))
				continue;
				
			grfIcetNoCommonChars &= ~_mpicetgrf[i];
			printf("Log:  Found first common seq for %s at offset 0x%.4x (%d)\n", rgszIcetNames[i], (cchTotal-1), (cchTotal-1));
			}
#endif
			
		if ((cIcetActive == 0) || ((cIcetActive == 1) && (cchTotal > cchMinSample)))
			break;
		}
		
	// Figure out why we exited the loop.
	//
	if (nToken == err)
		{
		cceRet = cceRead;
		goto _LRet;
		}
		
	// Process escapes separately.  Interpret the escape sequence
	// to determine for real which ISO7 flavor we have found.
	//
	if ((nToken == esc) || (nToken == so) || (nToken == si))
		{
        LARGE_INTEGER   li;
        HRESULT hr;

        LISet32(li, -1 );
        hr = pstmIn->Seek(li,STREAM_SEEK_CUR, NULL);

//		if (!pfnUnget(uch, lpvPrivate))
//			{
//			cceRet = cceUnget;
//			goto _LRet;
//			}
		cceRet = _CceReadEscSeq(pstmIn, nPrefCp, &icet, &fGuess);
#if JPDEBUG
		if (cceRet == cceSuccess)
			printf("Log:  Found encoding %s at offset 0x%.4x (%d)\n", rgszIcetNames[icet], cchTotal, cchTotal);
#endif
		// ISO is a special case -- no need to check statistics.
		//
		goto _LRet;
		}
		
#if JPDEBUG2
	printf("Counts:  %d total chars, %d high chars, %d high pairs\n", cchTotal, cchHigh, cchHighPairs); 
#endif
			
	// If the token was eof, and we're not ignoring eof, transition
	// the remaining active sets on eof.
	//
	if ((nToken == eof) && !(dwFlags & grfDetectIgnoreEof))
		{
		for (i = 0; i < icetCount; i++)
			{
			if (!(grfIcetActive & _mpicetgrf[i]) || (NValidateUch((ICET)i, 0, fTrue) != 0))
				continue;
#if JPDEBUG
			printf("Log:  Lost %s at EOF\n", rgszIcetNames[i]);
#endif
			grfIcetActive &= ~_mpicetgrf[i];
			--cIcetActive;
			}
		}
		
	Assert(cIcetActive >= 0);	// better *not* be less than 0

	// See how we've narrowed our field of choices and set the 
	// return status accordingly.
	//
	if (cIcetActive <= 0)
		{
#if JPDEBUG
		printf("Log:  Bailed out entirely at offset 0x%.4x (%d)\n", cchTotal, cchTotal);
#endif
		cceRet = cceUnknownInput;
		goto _LRet;
		}
	else if (cIcetActive == 1)
		{
		icet = _IcetFromIcetMask(grfIcetActive);
#if JPDEBUG
		printf("Log:  Found encoding %s at offset 0x%.4x (%d)\n", rgszIcetNames[icet], cchTotal, cchTotal);
#endif
		// If we matched an encoding type and also found matching 
		// common character runs, skip statistics (see comment,
		// below).
		//
		if (FValidateCharCount(icet, &nCount) && (nCount > 0))
			{
#if JPDEBUG3
			printf("Log:  %d common sequences for %s\n", nCount, rgszIcetNames[icet]);
#endif
			goto _LRet;
			}
		else
			{
			goto _LStats;
			}
		}
		
	// Did we learn anything from counting characters?
	//
	icetSeq = (ICET)-1;
	nCountCurr = 0;
	for (i = 0; i < icetCount; i++)
		{
		if (!(grfIcetActive & _mpicetgrf[i]) || !FValidateCharCount((ICET)i, &nCount))
			continue;
			
		if (nCount > nCountCurr)
			{
			icetSeq = (ICET)i;
			nCountCurr = nCount;
			}
			
#if JPDEBUG3
		printf("Log:  %d common sequences for %s\n", nCount, rgszIcetNames[i]);
#endif
		}
			
	// Any luck?  If so, return.  Don't bother checking statistics.
	// We just proved that we found at least one common run of 
	// characters in this input.  The odds against this for just a
	// plain ASCII file with some high characters seem pretty high.
	// Ignore the statistics and just return the encoding type we
	// found.
	//
	if (icetSeq != -1)
		{
		icet = icetSeq;
		goto _LRet;
		}
		
#if JPDEBUG
	printf("Log:  Active Icet Mask 0x%.8x, %d left\n", grfIcetActive, cIcetActive);
	printf("Log:  Icet's left -- ");
	for (i = 0; i < icetCount; i++)
		{
		if (grfIcetActive & _mpicetgrf[i])
            printf("%s, ", rgszIcetNames[i]);
		}
    printf("\n");
#endif

	// If caller did not want us to try to guess at the encoding
	// in the absence of definitive data, bail out.
	//
	if (!(dwFlags & grfDetectResolveAmbiguity))
		{
		cceRet = cceAmbiguousInput;
		goto _LRet;
		}
		
	// We're guessing -- note it.
	//
	fGuess = fTrue;
		
	// More than one active encoding.  Attempt to resolve ambiguity.
	//
	cceRet = _CceResolveAmbiguity(grfIcetActive, &icet, nPrefCp, efPref);
	if (cceRet != cceSuccess)
		return cceRet;
		
_LStats:
		
	// Adjust the return code based on the "statistics" we gathered,
	// above.
	//
	if (cchHigh > 0)
		{
		if ((cchTotal < cchMinSample) ||
			(((cchHigh * 100) / cchTotal) < nHighCharThreshold) ||
			(((cchHighPairs * 100) / cchHigh) < nHighPairThreshold))
			{
			cceRet = cceMayBeAscii;
			}
		}
	else
		{
		cceRet = cceMayBeAscii;		// no high-ascii characters?  definitely maybe!
		}

#if JPDEBUG2
	if (cchHigh > 0)
		{
		int nPercent1 = ((cchHigh * 100) / cchTotal);
		int nPercent2 = ((cchHighPairs * 100) / cchHigh);
		printf("Ratios -- high/total: %d%%, runs/high: %d%%\n", nPercent1, nPercent2);
		}
#endif
		
_LRet:

	// Set the return variables, if successful.
	//
	if ((cceRet == cceSuccess) || (cceRet == cceMayBeAscii))
		{
		*lpicet = icet;
		*lpfGuess = fGuess;
		}
		
#if JPDEBUG
		if (cceRet == cceSuccess)
			{
			printf("Log:  Returning %s, fGuess = %s\n", rgszIcetNames[icet], (fGuess ? "fTrue" : "fFalse"));
			}
		else if (cceRet == cceMayBeAscii)
			{
			printf("Log:  Returning %s, fGuess = %s, may-be-ASCII\n", rgszIcetNames[icet], (fGuess ? "fTrue" : "fFalse"));
			}
#endif
		
	return cceRet;
}


/* _ N  G E T  N E X T  U C H */
/*----------------------------------------------------------------------------
	%%Function: _NGetNextUch
	%%Contact: jpick

	Get the next character from the input stream.  Classify the character.
----------------------------------------------------------------------------*/
static int _NGetNextUch(IStream *pstmIn, unsigned char *c, BOOL *lpfIsHigh)
{
	ULONG rc;
	unsigned char uch;
    HRESULT hr;
		  
    hr = pstmIn->Read(&uch, 1, &rc);
	
    if (rc == 0)
		return eof;
	else if (hr != S_OK )
		return err;
		
	*lpfIsHigh = (uch >= 0x80);
	*c = uch;
		
	switch (uch)
		{
		case chEsc:
			return esc;
		case chSo:
			return so;
		case chSi:
			return si;
		default:
			return xmn;
		}
}


// Masks for _CceResolveAmbiguity() -- only externally supported character
// sets are used in ambiguity resolution.  Don't include Euc-Tw here.
//
#define grfJapan			(ULONG) (grfShiftJis | grfEucJp)
#define grfChina			(ULONG) (grfEucCn | grfGbk)
#define grfKorea			(ULONG) (grfEucKr | grfWansung)
#define grfTaiwan			(ULONG) (grfBig5)
#define grfDbcs				(ULONG) (grfShiftJis | grfGbk | grfWansung | grfBig5)
#define grfEuc				(ULONG) (grfEucJp | grfEucKr | grfEucCn)


/* _ C E  F R O M  C E  M A S K */
/*----------------------------------------------------------------------------
	%%Function: _IcetFromIcetMask
	%%Contact: jpick
----------------------------------------------------------------------------*/
static ICET _IcetFromIcetMask(ULONG ulMask)
{
	switch (ulMask)
	{
	case grfEucCn:
		return icetEucCn;
	case grfEucJp:
		return icetEucJp;
	case grfEucKr:
		return icetEucKr;
	case grfEucTw:
		return icetEucTw;
	case grfIso2022Cn:
		return icetIso2022Cn;
	case grfIso2022Jp:
		return icetIso2022Jp;
	case grfIso2022Kr:
		return icetIso2022Kr;
	case grfIso2022Tw:
		return icetIso2022Tw;
	case grfBig5:
		return icetBig5;
	case grfGbk:
		return icetGbk;
	case grfHz:
		return icetHz;
	case grfShiftJis:
		return icetShiftJis;
	case grfWansung:
		return icetWansung;
	case grfUtf7:
		return icetUtf7;
	case grfUtf8:
		return icetUtf8;
	default:
		break;
	}
	
	// Should never get here ...
	//
//	NotReached();
	
	// Can't return a bogus value, here.
	//
	return icetShiftJis;
}

/* _ C E  D E F A U L T  F R O M  C E  M A S K */
/*----------------------------------------------------------------------------
	%%Function: _IcetDefaultFromIcetMask
	%%Contact: jpick
----------------------------------------------------------------------------*/
static ICET _IcetDefaultFromIcetMask(ULONG ulMask)
{
	// Priorities -- DBCS, EUC, Japan, Taiwan, China and Korea (???).
	//
	if (ulMask & grfDbcs)
		{
		if (ulMask & grfJapan)
			return icetShiftJis;
		if (ulMask & grfChina)
			return icetGbk;
		if (ulMask & grfTaiwan)
			return icetBig5;
		if (ulMask & grfKorea)
			return icetWansung;
		}
	else // EUC
		{
		if (ulMask & grfJapan)
			return icetEucJp;
		if (ulMask & grfChina)
			return icetEucCn;
		if (ulMask & grfKorea)
			return icetEucKr;			// may be able to return icetWansung, here
		}
		
	// (Assert);
	return icetShiftJis;  // ???
}

/* _ U L  C E  M A S K  F R O M  C P  E T P */
/*----------------------------------------------------------------------------
	%%Function: _UlIcetMaskFromCpEf
	%%Contact: jpick
----------------------------------------------------------------------------*/
static ULONG _UlIcetMaskFromCpEf(int nCp, EFam ef)
{
	ULONG grf = grfAll;
	
	switch (nCp)
	{
	case nCpJapan:
		grf &= grfJapan;
		break;
	case nCpChina:
		grf &= grfChina;
		break;
	case nCpKorea:
		grf &= grfKorea;
		break;
	case nCpTaiwan:
		grf &= grfTaiwan;
		break;
	default:
		break;
	}
	
	switch (ef)
	{
	case efDbcs:
		grf &= grfDbcs;
		break;
	case efEuc:
		grf &= grfEuc;
		break;
	default:
		break;
	}
	return grf;
}


/* _ C C E  R E S O L V E  A M B I G U I T Y */
/*----------------------------------------------------------------------------
	%%Function: _CceResolveAmbiguity
	%%Contact: jpick

	Attempt to resolve ambiguous input encoding based on user
	preferences, if set, and system code page.  grfIcet contains a
	bitmask representing the encodings that are still possible after
	examining the input sample.
----------------------------------------------------------------------------*/
static CCE _CceResolveAmbiguity(ULONG grfIcet, ICET *lpicet, int nPrefCp, EFam efPref)
{
    ULONG grfIcetOrig = grfIcet;
	ULONG grfPref;
	ULONG grfSys;
	ULONG grfResult;
	UINT  cpSys;
	int cIcet;
	
	// Build "list" of encodings based on user-prefs.
	//
	grfPref = _UlIcetMaskFromCpEf(nPrefCp, efPref);
	
	// See if the user's preferences make any difference.
	//
	grfResult = grfIcet & grfPref;
	
	if (grfResult)
		{
		cIcet = _CBitsOnFromUlong(grfResult);
		if (cIcet == 1)
			{
			*lpicet = _IcetFromIcetMask(grfResult);
			return cceSuccess;
			}
		else
			grfIcet = grfResult;			// see comment, below
		}
		
	// Now look to the system code page for help.  Look at
	// the set of encodings as modified by the user
	// preferences (??? do we want to do this ???).
	//
	cpSys = GetACP();
	if (!FIsFeCp(cpSys) || (grfIcetOrig & grfUtf8))
		goto _LDefault;
		
	// Build "list" of encodings based on system cp.
	//
	grfSys = _UlIcetMaskFromCpEf(cpSys, (EFam) 0);
	
	// See if the system cp makes any difference.
	//
	grfResult = grfIcet & grfSys;
	
	if (grfResult)
		{
		cIcet = _CBitsOnFromUlong(grfResult);
		if (cIcet == 1)
			{
			*lpicet = _IcetFromIcetMask(grfResult);
			return cceSuccess;
			}
		}
			
_LDefault:

    // Special case -- pick UTF-8 if it's legal and the prefs
    // don't help us.
    //
	*lpicet =
        (grfIcetOrig & grfUtf8) ? icetUtf8 : _IcetDefaultFromIcetMask(grfIcet);
	return cceSuccess;
}


/* _ C C E  R E A D  E S C  S E Q */
/*----------------------------------------------------------------------------
	%%Function: _CceReadEscSeq
	%%Contact: jpick

	We've read (and put back) an escape character.  Call the ISO-2022
	escape sequence converter to have it map the escape sequence to the
	appropriate character set.  We may be looking at the escape sequence
	for ASCII, so be prepared to read ahead to the next one.
----------------------------------------------------------------------------*/
static CCE _CceReadEscSeq(
    IStream   *pstmIn,           // input stream
	int       nPrefCp,
	ICET     *lpicet,
	BOOL     *lpfGuess
)
{
	unsigned char uch;
	CCE cceRet;
	int nToken;
	BOOL fDummy;
	
	do
		{
		cceRet = CceReadEscSeq(pstmIn, lpicet); 
		
		if ((cceRet == cceSuccess) || (cceRet != cceMayBeAscii))
			break;
		
		while (fTrue)
			{
			nToken = _NGetNextUch(pstmIn, &uch, &fDummy);
			if (_FStopToken(nToken))
				break;
			}
			
		// Why did we stop?
		//
		if (nToken == err)
			{
			cceRet = cceRead;
			break;
			}
		else if (nToken == eof)
			{
			// Means this is legal ISO-2022 input, but we've seen nothing
			// but non-flavor-specific escape sequences (e.g., only ASCII
			// or shift sequences).  Choose the encoding type based on
			// preferences (only pick from those currently supported
			// externally).
			//
			switch (nPrefCp)
				{
				case nCpKorea:
					*lpicet = icetIso2022Kr;
					break;
				case nCpJapan:
				default:						// Right ??? (gotta pick something ...)
					*lpicet = icetIso2022Jp;
					break;
				}
			*lpfGuess = fTrue;					// not *really* guessing, but ... (???)
			cceRet = cceSuccess;
			break;
			}
			
		Assert((nToken == esc) || (nToken == so) || (nToken == si));
		{
        LARGE_INTEGER   li;
        HRESULT hr;

        LISet32(li, -1 );

        hr = pstmIn->Seek(li,STREAM_SEEK_CUR, NULL);
        }
		// Put it back for CceReadEscSeq() to process.
		//
//		if (!pfnUnget(uch, lpvPrivate))
//			{
//			cceRet = cceUnget;
//			break;
//			}
			
		} while (fTrue);
	
	return cceRet;
}
