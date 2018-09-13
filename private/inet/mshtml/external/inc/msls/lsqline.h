#ifndef LSQLINE_DEFINED
#define LSQLINE_DEFINED

#include "lsdefs.h"
#include "plsline.h"
#include "plsqsinf.h"
#include "plscell.h"
#include "pcelldet.h"
#include "gprop.h"

LSERR WINAPI  LsQueryLineCpPpoint(
							PLSLINE,	/* IN: pointer to line info -- opaque to client	*/
							LSCP,		/* IN: cpQuery									*/
							DWORD,      /* IN: nDepthQueryMax							*/
							PLSQSUBINFO,/* OUT: array[nDepthQueryMax] of LSQSUBINFO		*/
							DWORD*,		 /* OUT: nActualDepth							*/
							PLSTEXTCELL);/* OUT: Text cell info							*/


LSERR WINAPI LsQueryLinePointPcp(
							PLSLINE,	/* IN: pointer to line -- opaque to client			*/
						 	PCPOINTUV,	/* IN: query point (uQuery,vQuery) (line text flow)	*/
							DWORD,      /* IN: nDepthQueryMax								*/
							PLSQSUBINFO,/* OUT: array[nDepthQueryMax] of LSQSUBINFO			*/
							DWORD*,      /* OUT: nActualDepth	*/
							PLSTEXTCELL);/* OUT: Text cell info */

LSERR WINAPI LsQueryTextCellDetails(
							PLSLINE,	/* IN: pointer to line -- opaque to client				*/
						 	PCELLDETAILS,/* IN: query point (uQuery,vQuery) (line text flow)	*/
							LSCP,		/* IN: cpStartCell										*/
							DWORD,		/* IN: nCharsInContext									*/
							DWORD,		/* IN: nGlyphsInContext									*/
							WCHAR*,		/* OUT: pointer array[nCharsInContext] of char codes	*/
							PGINDEX,	/* OUT: pointer array[nGlyphsInContext] of glyph indices*/
							long*,		/* OUT: pointer array[nGlyphsInContext] of glyph widths	*/
							PGOFFSET,	/* OUT: pointer array[nGlyphsInContext] of glyph offsets*/
							PGPROP);	/* OUT: pointer array[nGlyphsInContext] of glyph handles*/

/*
 *	Query point and output point are in the coordinate system of the line.
 *	Text flow is the text flow of the line, zero point is at the starting point of the line. 
 */


LSERR WINAPI LsQueryLineDup(PLSLINE,	/* IN: pointer to line -- opaque to client	*/
							long*,		/* OUT: upStartAutonumberingText			*/
							long*,		/* OUT: upLimAutonumberingText				*/
							long*,		/* OUT: upStartMainText						*/
							long*,		/* OUT: upStartTrailing						*/
							long*);		/* OUT: upLimLine							*/

LSERR WINAPI LsQueryFLineEmpty(
							PLSLINE,	/* IN: pointer to line -- opaque to client 	*/
							BOOL*);		/* OUT: Is line empty? 						*/

#endif /* !LSQLINE_DEFINED */
