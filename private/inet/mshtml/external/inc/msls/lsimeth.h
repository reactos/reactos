#ifndef LSIMETH_DEFINED
#define LSIMETH_DEFINED

#include "lsdefs.h"

#include "plscbk.h"
#include "plsrun.h"
#include "pilsobj.h"
#include "plnobj.h"
#include "pdobj.h"
#include "pfmti.h"
#include "pbrko.h"
#include "pobjdim.h"
#include "pdispi.h"
#include "plsdocin.h"
#include "pposichn.h"
#include "plocchnk.h"
#include "plsfgi.h"
#include "pheights.h"
#include "plsqin.h"
#include "plsqout.h"
#include "plssubl.h"
#include "plschp.h"
#include "lstflow.h"
#include "lskjust.h"
#include "breakrec.h"
#include "brkcond.h"
#include "brkkind.h"
#include "fmtres.h"
#include "mwcls.h"

typedef struct
{
	LSERR (WINAPI* pfnCreateILSObj)(POLS, PLSC,  PCLSCBK, DWORD, PILSOBJ*);
	/* CreateILSObj
	 *  pols (IN):
	 *  plsc (IN): LS context
	 *  plscbk (IN): callbacks
	 *  idObj (IN): id of the object
	 *  &pilsobj (OUT): object ilsobj
	*/

	LSERR (WINAPI* pfnDestroyILSObj)(PILSOBJ);
	/* DestroyILSObj
	 *  pilsobj (IN): object ilsobj
	*/

	LSERR (WINAPI* pfnSetDoc)(PILSOBJ, PCLSDOCINF);
	/* SetDoc
	 *  pilsobj (IN): object ilsobj
	 *  lsdocinf (IN): initialization data at document level
	*/

	LSERR (WINAPI* pfnCreateLNObj)(PCILSOBJ, PLNOBJ*);
	/* CreateLNObj
	 *  pilsobj (IN): object ilsobj
	 *  &plnobj (OUT): object lnobj
	*/

	LSERR (WINAPI* pfnDestroyLNObj)(PLNOBJ);
	/* DestroyLNObj
	 *  plnobj (OUT): object lnobj
	*/

	LSERR (WINAPI* pfnFmt)(PLNOBJ, PCFMTIN, FMTRES*);
	/* Fmt
	 *  plnobj (IN): object lnobj
	 *  pfmtin (IN): formatting input
	 *  &fmtres (OUT): formatting result
	*/

	LSERR (WINAPI* pfnFmtResume)(PLNOBJ, const BREAKREC*, DWORD, PCFMTIN, FMTRES*);
	/* FmtResume
	 *  plnobj (IN): object lnobj
	 *  rgBreakRecord (IN): array of break records
	 *	nBreakRecord (IN): size of the break records array
	 *  pfmtin (IN): formatting input
	 *  &fmtres (OUT): formatting result
	*/

	LSERR (WINAPI* pfnGetModWidthPrecedingChar)(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
	/* GetModWidthPrecedingChar
	 *  pdobj (IN): dobj
     *  plsrun (IN): plsrun of the object
     *  plsrunText (IN): plsrun of the preceding char
     *  heightsRef (IN): height info about character
	 *  wchar (IN): preceding character
	 *  mwcls (IN): ModWidth class of preceding character
	 *  &durChange (OUT): amount by which width of the preceding char is to be changed
	*/

	LSERR (WINAPI* pfnGetModWidthFollowingChar)(PDOBJ, PLSRUN, PLSRUN, PCHEIGHTS, WCHAR, MWCLS, long*);
	/* GetModWidthPrecedingChar
	 *  pdobj (IN): dobj
     *  plsrun (IN): plsrun of the object
     *  plsrunText (IN): plsrun of the following char
     *  heightsRef (IN): height info about character
	 *  wchar (IN): following character
	 *  mwcls (IN): ModWidth class of the following character
	 *  &durChange (OUT): amount by which width of the following char is to be changed
	*/

	LSERR (WINAPI* pfnTruncateChunk)(PCLOCCHNK, PPOSICHNK);
	/* Truncate
	 *  plocchnk (IN): locchnk to truncate
	 *  posichnk (OUT): truncation point
	*/

	LSERR (WINAPI* pfnFindPrevBreakChunk)(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
	/* FindPrevBreakChunk
	 *  plocchnk (IN): locchnk to break
	 *  pposichnk (IN): place to start looking for break
	 *  brkcond (IN): recommmendation about the break after chunk
	 *  &brkout (OUT): results of breaking
	*/

	LSERR (WINAPI* pfnFindNextBreakChunk)(PCLOCCHNK, PCPOSICHNK, BRKCOND, PBRKOUT);
	/* FindNextBreakChunk
	 *  plocchnk (IN): locchnk to break
	 *  pposichnk (IN): place to start looking for break
	 *  brkcond (IN): recommmendation about the break before chunk
	 *  &brkout (OUT): results of breaking
	*/

	LSERR (WINAPI* pfnForceBreakChunk)(PCLOCCHNK, PCPOSICHNK, PBRKOUT);
	/* ForceBreakChunk
	 *  plocchnk (IN): locchnk to break
	 *  pposichnk (IN): place to start looking for break
	 *  &brkout (OUT): results of breaking
	*/

	LSERR (WINAPI* pfnSetBreak)(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
	/* SetBreak
	 *  pdobj (IN): dobj which is broken
	 *  brkkind (IN): Previous/Next/Force/Imposed was chosen
	 *	nBreakRecord (IN): size of array
	 *  rgBreakRecord (OUT): array of break records
	 *	nActualBreakRecord (OUT): actual number of used elements in array
	*/

	LSERR (WINAPI* pfnGetSpecialEffectsInside)(PDOBJ, UINT*);
	/* GetSpecialEffects
	 *  pdobj (IN): dobj
	 *  &EffectsFlags (OUT): Special effects inside of this object
	*/

	LSERR (WINAPI* pfnFExpandWithPrecedingChar)(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
	/* FExpandWithPrecedingChar
	 *  pdobj (IN): dobj
     *  plsrun (IN): plsrun of the object
     *  plsrunText (IN): plsrun of the preceding char
	 *  wchar (IN): preceding character
	 *  mwcls (IN): ModWidth class of preceding character
	 *  &fExpand (OUT): expand preceding character?
	*/

	LSERR (WINAPI* pfnFExpandWithFollowingChar)(PDOBJ, PLSRUN, PLSRUN, WCHAR, MWCLS, BOOL*);
	/* FExpandWithFollowingChar
	 *  pdobj (IN): dobj
     *  plsrun (IN): plsrun of the object
     *  plsrunText (IN): plsrun of the following char
	 *  wchar (IN): following character
	 *  mwcls (IN): ModWidth class of the following character
	 *  &fExpand (OUT): expand object?
	*/
	LSERR (WINAPI* pfnCalcPresentation)(PDOBJ, long, LSKJUST, BOOL);
	/* CalcPresentation
	 *  pdobj (IN): dobj
	 *  dup (IN): dup of dobj
	 *  lskj (IN): current justification mode
	 *  fLastVisibleOnLine (IN): this object is last visible object on line
	*/

	LSERR (WINAPI* pfnQueryPointPcp)(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
	/* QueryPointPcp
	 *  pdobj (IN): dobj to query
	 * 	ppointuvQuery (IN): query point (uQuery,vQuery)
     *	plsqin (IN): query input
     *	plsqout (OUT): query output
	*/
	
	LSERR (WINAPI* pfnQueryCpPpoint)(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
	/* QueryCpPpoint
	 *  pdobj (IN): dobj to query
	 *  dcp (IN):  dcp for the query
     *	plsqin (IN): query input
     *	plsqout (OUT): query output
	*/

	LSERR (WINAPI* pfnEnum)(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW, BOOL,
												BOOL, const POINT*, PCHEIGHTS, long);
	/* Enum object
	 *  pdobj (IN): dobj to enumerate
	 *  plsrun (IN): from DNODE
	 *  plschp (IN): from DNODE
	 *  cpFirst (IN): from DNODE
	 *  dcp (IN): from DNODE
	 *  lstflow (IN): text flow
	 *  fReverseOrder (IN): enumerate in reverse order
	 *  fGeometryNeeded (IN):
	 *  pptStart (IN): starting position, iff fGeometryNeeded
	 *  pheightsPres(IN): from DNODE, relevant iff fGeometryNeeded
	 *  dupRun(IN): from DNODE, relevant iff fGeometryNeeded
	*/

	LSERR (WINAPI* pfnDisplay)(PDOBJ, PCDISPIN);
	/* Display
	 *  pdobj (IN): dobj to display
	 *  pdispin (IN): input display info
	*/

	LSERR (WINAPI* pfnDestroyDObj)(PDOBJ);
	/* DestroyDObj
	 *  pdobj (IN): dobj to destroy
	*/

} LSIMETHODS;

#endif /* LSIMETH_DEFINED */
