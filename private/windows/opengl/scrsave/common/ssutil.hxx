/******************************Module*Header*******************************\
* Module Name: ssutil.hxx
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __ssutil_hxx__
#define __ssutil_hxx__

#include "sscommon.h"

extern BOOL SSU_SetupPixelFormat( HDC hdc, int flags, PIXELFORMATDESCRIPTOR *ppfd );
extern BOOL SSU_bNeedPalette( PIXELFORMATDESCRIPTOR *ppfd );
extern int SSU_PixelFormatDescriptorFromDc( HDC Dc, PIXELFORMATDESCRIPTOR *Pfd );

#endif // __ssutil_hxx__
