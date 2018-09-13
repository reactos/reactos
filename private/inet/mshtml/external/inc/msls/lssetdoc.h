#ifndef LSSETDOC_DEFINED
#define LSSETDOC_DEFINED

#include "lsdefs.h"
#include "lsdevres.h"
#include "lspract.h"
#include "lspairac.h"
#include "lsexpan.h"
#include "lsbrk.h"

LSERR WINAPI LsSetDoc(PLSC,				/* IN: ptr to line services context */
					  BOOL,				/* IN: Intend to display? 			*/
					  BOOL,				/* IN: Ref & Pres Devices are equal?*/
					  const LSDEVRES*); /* IN: device resolutions 			*/

LSERR WINAPI LsSetModWidthPairs(
					  PLSC,				/* IN: ptr to line services context */
					  DWORD,			/* IN: Number of mod pairs info units*/ 
					  const LSPAIRACT*,	/* IN: Mod pairs info units array  */
					  DWORD,			/* IN: Number of Mod Width classes	*/
					  const BYTE*);		/* IN: Mod width information(square):
											  indexes in the LSPAIRACT array */
LSERR WINAPI LsSetCompression(
					  PLSC,				/* IN: ptr to line services context */
					  DWORD,			/* IN: Number of compression priorities*/
					  DWORD,			/* IN: Number of compression info units*/
					  const LSPRACT*,	/* IN: Compession info units array 	*/
					  DWORD,			/* IN: Number of Mod Width classes	*/
					  const BYTE*);		/* IN: Compression information:
											  indexes in the LSPRACT array  */
LSERR WINAPI LsSetExpansion(
					  PLSC,				/* IN: ptr to line services context */
					  DWORD,			/* IN: Number of expansion info units*/
					  const LSEXPAN*,	/* IN: Expansion info units array	*/
					  DWORD,			/* IN: Number of Mod Width classes	*/
					  const BYTE*);		/* IN: Expansion information(square):
											  indexes in the LSEXPAN array  */
LSERR WINAPI LsSetBreaking(
					  PLSC,				/* IN: ptr to line services context */
					  DWORD,			/* IN: Number of breaking info units*/
					  const LSBRK*,		/* IN: Breaking info units array	*/
					  DWORD,			/* IN: Number of breaking classes	*/
					  const BYTE*);		/* IN: Breaking information(square):
											  indexes in the LSBRK array  */


#endif /* !LSSETDOC_DEFINED */

