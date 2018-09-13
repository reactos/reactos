#ifndef HIH_DEFINED
#define HIH_DEFINED

#include "lsimeth.h"

/*
 *	H(orizontal)I(n)H(orizontal)
 *
 *	This object is designed to help client implementations which use 
 *	Tatenakayoko and wish to be able to convert the display from vertical
 *	to horizontal and then to display the Tatenakayoko text as horizontal.
 *	To do this the client application simply changes the object handler
 *	from the Tatenakayoko handler to this object handler and the text
 *	will be displayed horizontally.
 *	
 */

/* typedef for callback to client for enumeration */
typedef LSERR(WINAPI * PFNHIHENUM)(
	POLS pols,				/*(IN): client context */
	PLSRUN plsrun,			/*(IN): from DNODE */
	PCLSCHP plschp,			/*(IN): from DNODE */
	LSCP cp,				/*(IN): from DNODE */
	LSDCP dcp,				/*(IN): from DNODE */
	LSTFLOW lstflow,		/*(IN): text flow*/
	BOOL fReverse,			/*(IN): enumerate in reverse order */
	BOOL fGeometryNeeded,	/*(IN): */
	const POINT* pt,		/*(IN): starting position (top left), iff fGeometryNeeded */
	PCHEIGHTS pcheights,	/*(IN): from DNODE, relevant iff fGeometryNeeded */
	long dupRun,			/*(IN): from DNODE, relevant iff fGeometryNeeded*/
	PLSSUBL plssubl);		/*(IN): subline in hih object. */

/*
 *
 *	HIH object initialization data that the client application must return
 *	when the HIH object handler calls the GetObjectHandlerInfo callback.
 */

#define HIH_VERSION 0x300

typedef struct HIHINIT
{
	DWORD				dwVersion;		/* Version. Must be HIH_VERSION */
	WCHAR				wchEndHih;		/* Escape for end of HIH object */
	WCHAR				wchUnused1;
	WCHAR				wchUnused2;
	WCHAR				wchUnused3;
	PFNHIHENUM			pfnEnum;		/* Enumeration callback */
} HIHINIT, *PHIHINIT;

LSERR WINAPI LsGetHihLsimethods(
	LSIMETHODS *plsim);

/* GetHihLsimethods
 *	
 *	plsim (OUT): Hih object methods for Line Services
 *
 */

#endif /* HIH_DEFINED */

