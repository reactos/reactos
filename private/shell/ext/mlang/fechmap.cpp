/*----------------------------------------------------------------------------
	%%File: fechmap.c
	%%Unit: fechmap
	%%Contact: jpick

	DLL entry points for FarEast conversion module.
----------------------------------------------------------------------------*/

#include "private.h"
#include "fechmap_.h"

#include "codepage.h"

static CODEPAGE _mpicetce[icetCount] =
{
    CP_EUC_CH,  			// icetEucCn
    CP_EUC_JP, 				// icetEucJp
    CP_EUC_KR, 				// icetEucKr
    CP_UNDEFINED,			// icetEucTw		(Not externally supported)
    CP_UNDEFINED,			// icetIso2022Cn	(Not externally supported)
    CP_ISO_2022_JP,			// icetIso2022Jp
    CP_ISO_2022_KR,			// icetIso2022Kr
    CP_UNDEFINED,			// icetIso2022Tw	(Not externally supported)
    CP_TWN,   				// icetBig5
    CP_CHN_GB,				// icetGbk
    CP_CHN_HZ,				// icetHz
    CP_JPN_SJ,				// icetShiftJis
    CP_KOR_5601,			// icetWansung
    CP_UTF_7,				// icetUtf7
    CP_UTF_8, 				// icetUtf8
};

/* C C E  D E T E C T  I N P U T  C O D E */
/*----------------------------------------------------------------------------
	%%Function: CceDetectInputCode
	%%Contact: jpick

	Routine that will analyze contents of file to make a best guess
	as to what encoding method was used on it.  Caller-supplied get
	and unget routines used for data access.
----------------------------------------------------------------------------*/
EXPIMPL(CCE)
CceDetectInputCode(
    IStream   *pstmIn,           // input stream
	DWORD     dwFlags,			// configuration flags
	EFam      efPref,			// optional: preferred encoding family
	int       nPrefCp,			// optional: preferred code page
	UINT      *lpCe,				// set to detected encoding
	BOOL      *lpfGuess			// set to fTrue if function "guessed"
)
{
	CCE cceRet;
	ICET icet;
	
	if (!pstmIn || !lpCe || !lpfGuess)
		return cceInvalidParameter;
		
	// DEBUG, only.  Prepare the assert handler.  This macro will
	// return cceInternal to the calling app if an assert is hit
	// before the handler is cleared, below.
	//
	//	InitAndCatchAsserts();
		
	cceRet = CceDetermineInputType(pstmIn, dwFlags, efPref, 
					nPrefCp, &icet, lpfGuess);
	
	if ((cceRet == cceSuccess) || (cceRet == cceMayBeAscii))
		{
		if (_mpicetce[icet] != CP_UNDEFINED )
			*lpCe = (UINT) _mpicetce[icet];
		else
			cceRet = cceUnknownInput;
		}
		
	// Done with the assert handler.
	//
	//	ClearAsserts();

	return cceRet;
}

