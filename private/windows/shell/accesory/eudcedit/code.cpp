//
// Copyright (c) 1997-1999 Microsoft Corporation.
//

#include	"stdafx.h"
#include	"eudcedit.h"
#ifdef BUILD_ON_WINNT
#include    "extfunc.h"
#endif // BUILD_ON_WINNT
#pragma		pack(2)


#define		SEGMAX	256
#define		EUDCCODEBASE	((unsigned short)0xe000)


static int init = 0;
static unsigned short	segStart[SEGMAX];
static unsigned short	segEnd[SEGMAX];
static unsigned short	segUni[SEGMAX];
static int	segCnt = 0;
static int	recCnt = 0;
static void
setseg( unsigned short segH, unsigned short segLS, unsigned short segLE)
{
	unsigned short	cCnt;

	if ( segCnt >= SEGMAX)
		return;
	cCnt = segLE - segLS + 1;
	segStart[segCnt] = (segH<<8)+segLS;
	segEnd[segCnt] = (segH<<8)+segLE;
	segUni[segCnt]  = EUDCCODEBASE	+recCnt;
	recCnt += cCnt;
	segCnt++;
}
void
makeUniCodeTbl ( )
{
	int	base;
	unsigned short	slow, elow;
	int	n;
	int	nlow;
	unsigned short high;
	COUNTRYINFO	*cInfo;

    //we don't need an unicode table if we only have unicode
    if (CountryInfo.bOnlyUnicode) 
        return;

	if ( init)	return ;
	cInfo = &CountryInfo;
	base = 0;
	segCnt = recCnt = 0;
	for ( n=0; n < cInfo->nRange - 1; n++) {

#ifdef BUILD_ON_WINNT
        /* CHS needs to dynamically calculate trailbyte range for each 
         * EUDC select range.
         */
        if (cInfo->LangID == EUDC_CHS)
            CorrectTrailByteRange(n);
#endif // BUILD_ON_WINNT

		for ( high = cInfo->sLeadByte[n]; high <=cInfo->eLeadByte[n]; 
                                                        high++){
		    if ( high == cInfo->sLeadByte[n])
			    slow = cInfo->sRange[n] & 0xff;
		    else
			    slow = cInfo->sTralByte[0];
		    if ( high ==cInfo->eLeadByte[n])
			    elow = cInfo->eRange[n] & 0xff;
		    else
			    elow = cInfo->eTralByte[cInfo->nTralByte-1];

		    for ( nlow = 0; nlow < cInfo->nTralByte; nlow++) {
			if (  slow >= cInfo->sTralByte[nlow]
					&& slow <= cInfo->eTralByte[nlow]) {
				if ( elow <=  cInfo->eTralByte[nlow] )
					setseg( high, slow, elow);
				else
				    setseg( high, slow, cInfo->eTralByte[nlow]);
			}
			else if ( slow < cInfo->sTralByte[nlow]
				&& elow >= cInfo->sTralByte[nlow]) {
				if ( elow <=  cInfo->eTralByte[nlow] )
				    setseg( high, cInfo->sTralByte[nlow], elow);
				else
				    setseg( high, cInfo->sTralByte[nlow],
						cInfo->sTralByte[nlow]);
			}
		    }
		}
	}
	init = 1;
}
unsigned short
sjisToUniEUDC( unsigned short code)
{
	int	seg;
	unsigned short	ofs;

	for ( seg = 0; seg < segCnt; seg++) {
		if ( code <= segEnd[seg]) {
			if ( segStart[seg] <= code) {
				ofs = code - segStart[seg];
				return segUni[seg]+ofs;
			}
		}
	}
	return (unsigned short)0xffff;
}
unsigned short
getMaxUniCode( )
{
		USHORT ansiMax;
		if (CountryInfo.bOnlyUnicode)
			ansiMax = 0;
		else
			ansiMax = segUni[segCnt-1] + (segEnd[segCnt-1] - segStart[segCnt-1]);
        return max(ansiMax, CountryInfo.eRange[CountryInfo.nRange-1]);
}


/* EOF */
