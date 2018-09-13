#ifndef LSDNFIN_DEFINED
#define LSDNFIN_DEFINED

/* Access routines for contents of DNODES */

#include "lsdefs.h"
#include "plsrun.h"
#include "plsfrun.h"
#include "plschp.h"
#include "pobjdim.h"
#include "pdobj.h"


LSERR WINAPI LsdnFinishRegular(
							  PLSC,				/* IN: Pointer to LS Context */
							  LSDCP,     		/* IN: dcp adopted           */
							  PLSRUN,   		/* IN: PLSRUN  		         */
							  PCLSCHP,  		/* IN: CHP          	     */
							  PDOBJ,    		/* IN: PDOBJ             	 */ 
							  PCOBJDIM);		/* IN: OBJDIM      		     */

LSERR WINAPI LsdnFinishRegularAddAdvancePen(
							  PLSC,				/* IN: Pointer to LS Context */
							  LSDCP,     		/* IN: dcp adopted           */
							  PLSRUN,   		/* IN: PLSRUN  		         */
							  PCLSCHP,  		/* IN: CHP          	     */
							  PDOBJ,    		/* IN: PDOBJ             	 */ 
							  PCOBJDIM,			/* IN: OBJDIM      		     */
							  long,				/* IN: durPen				 */
							  long,				/* IN: dvrPen				 */
							  long);			/* IN: dvpPen 				 */

LSERR WINAPI LsdnFinishByPen(PLSC,				/* IN: Pointer to LS Context */
						   LSDCP, 	    		/* IN: dcp	adopted          */
						   PLSRUN,		   		/* IN: PLSRUN  		         */
						   PDOBJ,	    		/* IN: PDOBJ             	 */ 
						   long,    	 		/* IN: dur         		     */
						   long,     			/* IN: dvr             		 */
						   long);   			/* IN: dvp          	     */

LSERR WINAPI LsdnFinishDeleteAll(PLSC,			/* IN: Pointer to LS Context */
					  			LSDCP);			/* IN: dcp adopted			 */

#endif /* !LSDNFIN_DEFINED */

