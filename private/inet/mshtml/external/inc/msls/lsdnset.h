#ifndef LSDNSET_DEFINED
#define LSDNSET_DEFINED

/* Access routines for contents of DNODES */

#include "lsdefs.h"
#include "plssubl.h"
#include "plsdnode.h"
#include "pobjdim.h"
#include "lsktab.h"
#include "lskeop.h"

LSERR WINAPI LsdnQueryObjDimRange(
								  PLSC,			/* IN: Pointer to LS Context */
							 	  PLSDNODE,		/* IN: plsdnFirst -- First DNODE in range */
								  PLSDNODE,		/* IN: plsdnLast -- Last DNODE in range */
							 	  POBJDIM);		/* OUT: dimensions of range */


LSERR WINAPI LsdnResetObjDim(
								 PLSC,			/* IN: Pointer to LS Context	*/
								 PLSDNODE,		/* IN: plsdnFirst 				*/
								 PCOBJDIM);		/* IN: dimensions of dnode 		*/


LSERR WINAPI LsdnQueryPenNode(
							  PLSC,				/* IN: Pointer to LS Context*/
						  	  PLSDNODE,			/* IN: DNODE queried		*/
						  	  long*,			/* OUT: &dvpPen				*/
						  	  long*,			/* OUT: &durPen				*/
						  	  long*);			/* OUT: &dvrPen				*/


LSERR WINAPI LsdnResetPenNode(
							  PLSC,				/* IN: Pointer to LS Context */
						  	  PLSDNODE,			/* IN: DNODE to be modified */
						  	  long,				/* IN: dvpPen */
						  	  long,				/* IN: durPen */
						  	  long);			/* IN: dvrPen */

LSERR WINAPI LsdnSetRigidDup(
							 PLSC,				/* IN: Pointer to LS Context */
							 PLSDNODE,			/* IN: DNODE to be modified	 */
							 long);				/* IN: dup					 */

LSERR WINAPI LsdnGetDup(
							 PLSC,				/* IN: Pointer to LS Context */
							 PLSDNODE,			/* IN: DNODE queried		 */
							 long*);			/* OUT: dup					 */

LSERR WINAPI LsdnSetAbsBaseLine(
								PLSC,			/* IN: Pointer to LS Context */
							  	long);    		/* IN: new vaBase            */

LSERR WINAPI LsdnModifyParaEnding(
								PLSC,			/* IN: Pointer to LS Context */
								LSKEOP);		/* IN: Kind of line ending			*/

LSERR WINAPI LsdnResolvePrevTab(PLSC);			/* IN: Pointer to LS Context */

LSERR WINAPI LsdnGetCurTabInfo(
							PLSC,				/* IN: Pointer to LS Context */
							LSKTAB*);			/* OUT: Type of current tab  */

LSERR WINAPI LsdnSkipCurTab(PLSC);					/* IN: Pointer to LS Context */

LSERR WINAPI LsdnDistribute(
							PLSC,				/* IN: Pointer to LS Context	*/
							PLSDNODE,			/* IN: First DNODE				*/
							PLSDNODE,			/* IN: Last DNODE				*/
							long);				/* IN: durToDistribute			*/

LSERR WINAPI LsdnSubmitSublines(
							PLSC,				/* IN: Pointer to LS Context	*/
							PLSDNODE,			/* IN: DNODE					*/
							DWORD,				/* IN: cSublinesSubmitted		*/
							PLSSUBL*,			/* IN: rgpsublSubmitted			*/
							BOOL,				/* IN: fUseForJustification		*/
							BOOL,				/* IN: fUseForCompression		*/
							BOOL,				/* IN: fUseForDisplay			*/
							BOOL,				/* IN: fUseForDecimalTab		*/
							BOOL				/* IN: fUseForTrailingArea		*/
							);											
LSERR WINAPI LsdnGetFormatDepth(
							PLSC,				/* IN: Pointer to LS Context	*/
							DWORD*);			/* OUT: nDepthFormatLineMax		*/

#endif /* !LSDNSET_DEFINED */

