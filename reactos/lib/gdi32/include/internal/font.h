/* $Id: font.h,v 1.2 2004/08/15 18:40:07 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/gdi32/include/internal/font.h
 * PURPOSE:         
 * PROGRAMMER:
 *
 */

#ifndef GDI32_FONT_H_INCLUDED
#define GDI32_FONT_H_INCLUDED

BOOL FASTCALL TextMetricW2A(TEXTMETRICA *tma, TEXTMETRICW *tmw);
BOOL FASTCALL NewTextMetricW2A(NEWTEXTMETRICA *tma, NEWTEXTMETRICW *tmw);
BOOL FASTCALL NewTextMetricExW2A(NEWTEXTMETRICEXA *tma, NEWTEXTMETRICEXW *tmw);

#endif

