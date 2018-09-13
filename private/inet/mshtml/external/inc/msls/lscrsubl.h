#ifndef LSCRSUBL_DEFINED
#define LSCRSUBL_DEFINED

/* Line services formatter fetch/dispatcher interface (to LsCreateLine())
 */

#include "lsdefs.h"
#include "lsfrun.h"
#include "lsesc.h"
#include "plssubl.h"
#include "plsdnode.h"
#include "fmtres.h"
#include "objdim.h"
#include "lstflow.h"
#include "lskjust.h"
#include "breakrec.h"
#include "brkkind.h"
#include "brkpos.h"

LSERR WINAPI LsCreateSubline(
							PLSC,			/* IN: LS context						*/
							LSCP,			/* IN: cpFirst							*/
						    long,			/* IN: urColumnMax						*/
							LSTFLOW,		/* IN: text flow						*/
							BOOL);			/* IN: fContiguous						*/

LSERR WINAPI LsFetchAppendToCurrentSubline(
							PLSC,			/* IN: LS context						*/
							LSDCP,			/* IN:Increase cp before fetching		*/
						    const LSESC*,	/* IN: escape characters				*/
						    DWORD,			/* IN: # of escape characters			*/
							BOOL*,			/* OUT: Successful?---if not, finish 
												subline, destroy it and start anew	*/
						    FMTRES*,		/* OUT: result of last formatter		*/
						    LSCP*,			/* OUT: cpLim							*/
						    PLSDNODE*,		/* OUT: First DNODE created				*/
						 	PLSDNODE*);		/* OUT: Last DNODE created				*/

LSERR WINAPI LsFetchAppendToCurrentSublineResume(
							PLSC,			/* IN: LS context						*/
							const BREAKREC*,/* IN: array of break records			*/
							DWORD,			/* IN: number of records in array		*/
							LSDCP,			/* IN:Increase cp before fetching		*/
						    const LSESC*,	/* IN: escape characters				*/
						    DWORD,			/* IN: # of escape characters			*/
							BOOL*,			/* OUT: Successful?---if not, finish 
												subline, destroy it and start anew	*/
						    FMTRES*,		/* OUT: result of last formatter		*/
						    LSCP*,			/* OUT: cpLim							*/
						    PLSDNODE*,		/* OUT: First DNODE created				*/
						 	PLSDNODE*);		/* OUT: Last DNODE created				*/

LSERR WINAPI LsAppendRunToCurrentSubline(		/* Simple runs only	*/
							PLSC,			/* IN: LS context						*/
						    const LSFRUN*,	/* IN: given run						*/
							BOOL*,			/* OUT: Successful?---if not, finish 
												subline, destroy it and start anew	*/
						    FMTRES*,		/* OUT: result of last formatter		*/
						    LSCP*,			/* OUT: cpLim							*/
						    PLSDNODE*);		/* OUT: DNODE created					*/

LSERR WINAPI LsResetRMInCurrentSubline(
							PLSC,			/* IN: LS context						*/
						    long);			/* IN: urColumnMax						*/

LSERR WINAPI LsFinishCurrentSubline(
							PLSC,			/* IN: LS context						*/
							PLSSUBL*);		/* OUT: subline context					*/


LSERR WINAPI LsTruncateSubline(
							PLSSUBL,		/* IN: subline context					*/
							long,			/* IN: urColumnMax						*/
							LSCP*);			/* OUT: cpTruncate 						*/

LSERR WINAPI LsFindPrevBreakSubline(
							PLSSUBL,		/* IN: subline context					*/
							BOOL,			/* IN: fFirstSubline					*/
							LSCP,			/* IN: truncation cp					*/
						    long,			/* IN: urColumnMax						*/
							BOOL*,			/* OUT: fSuccessful?					*/
							LSCP*,			/* OUT: cpBreak							*/
							POBJDIM,		/* OUT: objdimSub up to break			*/
							BRKPOS*);		/* OUT: Before/Inside/After				*/

LSERR WINAPI LsFindNextBreakSubline(
							PLSSUBL,		/* IN: subline context					*/
							BOOL,			/* IN: fFirstSubline					*/
							LSCP,			/* IN: truncation cp					*/
						    long,			/* IN: urColumnMax						*/
							BOOL*,			/* OUT: fSuccessful?					*/
							LSCP*,			/* OUT: cpBreak							*/
							POBJDIM,		/* OUT: objdimSub up to break			*/			
							BRKPOS*);		/* OUT: Before/Inside/After				*/

LSERR WINAPI LsForceBreakSubline(
							PLSSUBL,		/* IN: subline context					*/
							BOOL,			/* IN: fFirstSubline					*/
							LSCP,			/* IN: truncation cp					*/
						    long,			/* IN: urColumnMax						*/
							LSCP*,			/* OUT: cpBreak							*/
							POBJDIM,		/* OUT: objdimSub up to break			*/			
							BRKPOS*);		/* OUT: Before/Inside/After				*/

LSERR WINAPI LsSetBreakSubline(
							PLSSUBL,		/* IN: subline context					*/
							BRKKIND,		/* IN: Prev/Next/Force/Imposed			*/			
							DWORD,			/* IN: size of array					*/
							BREAKREC*, 		/* OUT: array of break records			*/
							DWORD*);		/* OUT: number of used elements of the array*/

LSERR WINAPI LsDestroySubline(PLSSUBL);

LSERR WINAPI LsMatchPresSubline(
							  PLSSUBL);		/* IN: subline context		*/

LSERR WINAPI LsExpandSubline(
							  PLSSUBL,		/* IN: subline context		*/
							  LSKJUST,		/* IN: justification type	*/
							  long);		/* IN: dup					*/

LSERR WINAPI LsCompressSubline(
							  PLSSUBL,		/* IN: subline context		*/
							  LSKJUST,		/* IN: justification type	*/
							  long);		/* IN: dup					*/

LSERR WINAPI LsSqueezeSubline(
							  PLSSUBL,		/* IN: subline context		*/
							  long,			/* IN: durTarget			*/
							  BOOL*,		/* OUT: fSuccessful?		*/
							  long*);		/* OUT: if nof successful, 
													extra dur 			*/

LSERR WINAPI LsGetSpecialEffectsSubline(
							  PLSSUBL,		/* IN: subline context		*/
							  UINT*);		/* OUT: special effects		*/

#endif /* !LSCRSUBL_DEFINED */

