#ifndef WARICHU_DEFINED
#define WARICHU_DEFINED

#include	"lsimeth.h"

/* Character location for pfnFetchWarichuWidthAdjust callback */
enum warichucharloc {
	warichuBegin,				/* Character preceeds Warichu object */
	warichuEnd					/* Character follows Warichu object */
};

/*
 *
 * Warichu Object Callbacks to Client
 *
 */
typedef struct WARICHUCBK
{
	LSERR (WINAPI* pfnGetWarichuInfo)(
		POLS pols,
		LSCP cp,
		LSTFLOW lstflow,
		PCOBJDIM pcobjdimFirst,
		PCOBJDIM pcobjdimSecond,
		PHEIGHTS pheightsRef,
		PHEIGHTS pheightsPres,
		long *pdvpDescentReservedForClient);

	/* GetWarichuInfo
	 *  pols (IN): The client context for the request.
	 *
	 *  cp (IN): the cp of the Warichu object.
	 *
     *  lstflow (IN): the lstflow of Warichu parent subline
	 *
	 *	pcobjdimFirst (IN): dimensions of first line of Warichu.
	 *
	 *	pcobjdimSecond (IN): dimensions of second line of Warichu.
	 *
	 *	pheightsRef (OUT): specifies heights for Warichu object in reference
	 *			device units.
	 *
	 *	pheightsPres (OUT): specifies heights for Warichu object in presentation
	 *			device units.
	 *
	 *	pdvpDescentReservedForClient (OUT): specifies the part of the descent area 
	 *			that the client is reserving for its own use (usually for the purpose 
	 *			of underlining) in presentation device units. The object will begin its 
	 *			display area below the baseline at the difference between 
	 *			pheightsRef->dvDescent and *pdvpDescentReservedForClient. 
	 *
	 */

	LSERR (WINAPI* pfnFetchWarichuWidthAdjust)(
		POLS pols,
		LSCP cp,
		enum warichucharloc wcl,
		PLSRUN plsrunForChar,
		WCHAR wch,
		MWCLS mwclsForChar,
		PLSRUN plsrunWarichuBracket,
		long *pdurAdjustChar,
		long *pdurAdjustBracket);

	/* FetchWarichuWidthAdjust
	 *  pols (IN): The client context for the request.
	 *
	 *  cp (IN): the cp of the Warichu object.
	 *
	 *  wcl (IN): specifies the location of the character and bracket.
	 *
	 *  plsrunForChar (IN): the run of the character that is either previous or 
	 *		following the Warichu object. Whether preceeding or following is 
	 *		determined by value of the wcl parameter above.
	 *
	 *	wch (IN): character that is either preceeding or following the Warichu 
	 *		object.
	 *
	 *	mwclsForChar (IN): mod width class for the wch parameter.
	 *
	 *	plsrunWarichuBracket (IN): plsrun for leading or following bracket of 
	 *		the Warichu.
	 *
	 *	pdurAdjustChar (OUT): the amount that the width of the input character 
	 *		should be adjusted. A negative value means the width of the input 
	 *		character should be made smaller.
	 *
	 *	pdurAdjustBracket (OUT): the amount that the width of the Warichu bracket
	 *		should be adjusted.  A negative value means the width of the Warichu 
	 *		bracket should be made smaller.
	 */

	LSERR (WINAPI* pfnWarichuEnum)(
		POLS pols,
		PLSRUN plsrun,		
		PCLSCHP plschp,	
		LSCP cp,		
		LSDCP dcp,		
		LSTFLOW lstflow,	
		BOOL fReverse,		
		BOOL fGeometryNeeded,	
		const POINT* pt,		
		PCHEIGHTS pcheights,	
		long dupRun,		
		const POINT *ptLeadBracket,	
		PCHEIGHTS pcheightsLeadBracket,
		long dupLeadBracket,		
		const POINT *ptTrailBracket,	
		PCHEIGHTS pcheightsTrailBracket,
		long dupTrailBracket,
		const POINT *ptFirst,	
		PCHEIGHTS pcheightsFirst,
		long dupFirst,		
		const POINT *ptSecond,	
		PCHEIGHTS pcheightsSecond,
		long dupSecond,
		PLSSUBL plssublLeadBracket,
		PLSSUBL plssublTrailBracket,
		PLSSUBL plssublFirst,	
		PLSSUBL plssublSecond);	

	/* WarichuEnum
	 * 
	 *	pols (IN): client context.
	 *
	 *  plsrun (IN): plsrun for the entire Warichu Object.
	 *
	 *	plschp (IN): is lschp for lead character of Warichu Object.
	 *
	 *	cp (IN): is cp of first character of Warichu Object.
	 *
	 *	dcp (IN): is number of characters in Warichu Object
	 *
	 *	lstflow (IN): is text flow at Warichu Object.
	 *
	 *	fReverse (IN): is whether text should be reversed for visual order.
	 *
	 *	fGeometryNeeded (IN): is whether Geometry should be returned.
	 *
	 *	pt (IN): is starting position , iff fGeometryNeeded .
	 *
	 *	pcheights (IN):	is height of Warichu object, iff fGeometryNeeded.
	 *
	 *	dupRun (IN): is length of Warichu Object, iff fGeometryNeeded.
	 *
	 *	ptLeadBracket (IN):	is point for second line iff fGeometryNeeded and 
	 *		plssublLeadBracket not NULL.
	 *
	 *	pcheightsLeadBracket (IN): is height for Warichu line iff fGeometryNeeded 
	 *		and plssublLeadBracket not NULL.
	 *
	 *	dupLeadBracket (IN): is length of Warichu line iff fGeometryNeeded and 
	 *		plssublLeadBracket not NULL.
	 *
	 *	ptTrailBracket (IN): is point for second line iff fGeometryNeeded and 
	 *		plssublLeadBracket not NULL.
	 *
	 *	pcheightsTrailBracket (IN):	is  height for Warichu  line iff fGeometryNeeded 
	 *		and plssublTrailBracket not NULL.
	 *
	 *	dupTrailBracket (IN): is length of Warichu line iff fGeometryNeeded and 
	 *		plssublTrailBracket not NULL.
	 *
	 *	ptFirst (IN): is starting point for main line iff fGeometryNeeded
	 *
	 *	pcheightsFirst (IN): is height of main line iff fGeometryNeeded
	 *
	 *	dupFirst (IN): is length of main line iff fGeometryNeeded
	 *
	 *	ptSecond (IN): is point for second line iff fGeometryNeeded and 
	 *		plssublSecond not NULL.
	 *
	 *	pcheightsSecond (IN): is height for Warichu line iff fGeometryNeeded 
	 *		and plssublSecond not NULL.
	 *
	 *	dupSecond (IN):	is length of Warichu line iff fGeometryNeeded and 
	 *		plssublSecond not NULL.
	 *
	 *	plssublLeadBracket (IN): is subline for lead bracket.
	 *
	 *	plssublTrailBracket (IN): is subline for trail bracket.
	 *
	 *	plssublFirst (IN): is first subline in Warichu object.
	 *
	 *	plssublSecond (IN):	is second subline in Warichu object.
	 *
	 */

} WARICHUCBK;

#define WARICHU_VERSION 0x300

/*
 * 
 *	Warichi object initialization data that the client application must return
 *	when the Warichu object handler calls the GetObjectHandlerInfo callback.
 */
typedef struct WARICHUINIT
{
	DWORD				dwVersion;			/* Version must be WARICHU_VERSION */
	WCHAR				wchEndFirstBracket;	/* Escape char to end first bracket */
	WCHAR				wchEndText;			/* Escape char to end text */
	WCHAR				wchEndWarichu;		/* Escape char to end object */
	WCHAR				wchUnused;			/* For alignment */
	WARICHUCBK			warichcbk;			/* Callbacks */
	BOOL				fContiguousFetch;	/* Always refetch whole subline & closing brace
											   after reformatting inside warichu */
} WARICHUINIT;

LSERR WINAPI LsGetWarichuLsimethods(
	LSIMETHODS *plsim);

/* GetWarichuLsimethods
 *
 *	plsim (OUT): Warichu object callbacks.
 */

#endif /* WARICHU_DEFINED */

