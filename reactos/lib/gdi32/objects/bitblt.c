/* $Id: bitblt.c,v 1.25 2004/12/30 02:32:24 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/gdi32/object/bitblt.c
 * PURPOSE:         
 * PROGRAMMER:
 */

#include "precomp.h"

#include <debug.h>


/*
 * @implemented
 */
BOOL
STDCALL
StretchBlt(
           HDC hdcDest,      // handle to destination DC
           int nXOriginDest, // x-coord of destination upper-left corner
           int nYOriginDest, // y-coord of destination upper-left corner
           int nWidthDest,   // width of destination rectangle
           int nHeightDest,  // height of destination rectangle
           HDC hdcSrc,       // handle to source DC
           int nXOriginSrc,  // x-coord of source upper-left corner
           int nYOriginSrc,  // y-coord of source upper-left corner
           int nWidthSrc,    // width of source rectangle
           int nHeightSrc,   // height of source rectangle
           DWORD dwRop       // raster operation code
	)
{
  if ((nWidthDest != nWidthSrc) || (nHeightDest != nHeightSrc))
  {
    return NtGdiStretchBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
                           hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, dwRop);
  }
  
  return NtGdiBitBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, hdcSrc,
                     nXOriginSrc, nYOriginSrc, dwRop);
}
