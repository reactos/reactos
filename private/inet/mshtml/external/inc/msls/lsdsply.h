#ifndef LSDSPLY_DEFINED
#define LSDSPLY_DEFINED

#include "lsdefs.h"
#include "plsline.h"

LSERR WINAPI LsDisplayLine(PLSLINE, const POINT*, UINT, const RECT*);
/* LsDisplayLine
 *  pline (IN)
 *  ppt (IN)
 *  kDisp (IN): transparent or opaque
 *  &rcClip (IN): clipping rect
 */

#endif /* !LSDSPLY_DEFINED */
