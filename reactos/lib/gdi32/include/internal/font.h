/* $Id: font.h,v 1.1 2004/03/23 00:18:54 gvg Exp $
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

#include <windows.h>

BOOL FASTCALL TextMetricW2A(TEXTMETRICA *tma, TEXTMETRICW *tmw);
BOOL FASTCALL NewTextMetricW2A(NEWTEXTMETRICA *tma, NEWTEXTMETRICW *tmw);
BOOL FASTCALL NewTextMetricExW2A(NEWTEXTMETRICEXA *tma, NEWTEXTMETRICEXW *tmw);

#endif

