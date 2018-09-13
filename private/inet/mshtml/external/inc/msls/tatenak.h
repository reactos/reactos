#ifndef TATENAKYOKO_DEFINED
#define TATENAKYOKO_DEFINED

#include "lsimeth.h"

/*
 *
 *	Tatenakayoko object callbacks to client application
 *
 */
typedef struct TATENAKYOKOCBK
{
	LSERR (WINAPI *pfnGetTatenakayokoLinePosition)(
		POLS pols,
		LSCP cp,
		LSTFLOW lstflow,
		PLSRUN plsrun,
		long dvr,
		PHEIGHTS pheightsRef,
		PHEIGHTS pheightsPres,
		long *pdvpDescentReservedForClient);

	/* GetTatenakayokoLinePosition
	 *  pols (IN): The client context for the request.
	 *
	 *  cp (IN): the cp of the Tatenakayoko object.
	 *
	 *  lstflow (IN): the lstflow of Tatenakayoko parent subline
	 *
	 *	plsrun (IN): the plsrun of the Tatenakayoko object.
	 *
	 *	dvr	(IN): the total height of the tatenakayoko object with respect to 
	 *			the current flow of the line in reference units.
	 *
	 *	pheightsRef	(OUT): specifies heights of Tatenakayoko object in reference
	 *			device units. 
	 *
	 *	pdvrDescentReservedForClient (OUT): specifies the part of the descent area 
	 *			that the client is reserving for its own use (usually for the purpose 
	 *			of underlining) in reference device units. The object will begin its 
	 *			display area below the baseline at the difference between *pdvrDescent 
	 *			and *pdvrDescentReservedForClient. 
	 *
	 *	pheightsPres (OUT): specifies heights of Tatenakayoko object in presenatation
	 *			device units. 
	 *
	 *	pdvpDescentReservedForClient (OUT): specifies the part of the descent area 
	 *			that the client is reserving for its own use (usually for the purpose 
	 *			of underlining) in presentation device units. The object will begin its 
	 *			display area below the baseline at the difference between *pdvpDescent 
	 *			and pheightsPres.dvDescent. 
	 *
	 */

	LSERR (WINAPI* pfnTatenakayokoEnum)(
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
		LSTFLOW lstflowT,
		PLSSUBL plssubl);

	/* TatenakayokoEnum
	 * 
	 *	pols (IN): client context.
	 *
	 *  plsrun (IN): plsrun for the entire Tatenakayoko Object.
	 *
	 *	plschp (IN): is lschp for lead character of Tatenakayoko Object.
	 *
	 *	cp (IN): is cp of first character of Tatenakayoko Object.
	 *
	 *	dcp (IN): is number of characters in Tatenakayoko Object
	 *
	 *	lstflow (IN): is text flow at Tatenakayoko Object.
	 *
	 *	fReverse (IN): is whether text should be reversed for visual order.
	 *
	 *	fGeometryNeeded (IN): is whether Geometry should be returned.
	 *
	 *	pt (IN): is starting position , iff fGeometryNeeded .
	 *
	 *	pcheights (IN):	is height of Tatenakayoko object, iff fGeometryNeeded.
	 *
	 *	dupRun (IN): is length of Tatenakayoko Object, iff fGeometryNeeded.
	 *
	 *	lstflowT (IN): is text flow for Tatenakayoko object.
	 *
	 *	plssubl (IN): is subline for Tatenakayoko object.
	 */

} TATENAKAYOKOCBK;

/*
 *
 *	Tatenakayoko object initialization data that the client application must return
 *	when the Tatenakayoko object handler calls the GetObjectHandlerInfo callback.
 */

#define TATENAKAYOKO_VERSION 0x300

typedef struct TATENAKAYOKOINIT
{
	DWORD				dwVersion;			/* Version. Only TATENAKAYOKO_VERSION is valid. */
	WCHAR				wchEndTatenakayoko;	/* Character marking end of Tatenakayoko object */
	WCHAR				wchUnused1;			/* For alignment */
	WCHAR				wchUnused2;			/* For alignment */
	WCHAR				wchUnused3;			/* For alignment */
	TATENAKAYOKOCBK		tatenakayokocbk;	/* Client application callbacks */
} TATENAKAYOKOINIT, *PTATENAKAYOKOINIT;

LSERR WINAPI LsGetTatenakayokoLsimethods(
	LSIMETHODS *plsim);

/* GetTatenakayokoLsimethods
 *	
 *	plsim (OUT): Tatenakayoko object methods for Line Services
 *
 */

#endif /* TATENAKYOKO_DEFINED */

