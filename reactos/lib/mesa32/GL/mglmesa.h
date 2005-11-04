/****************************************************************************
*
*                      Mesa bindings for SciTech MGL
*
*                   Copyright (C) 1996 SciTech Software.
*                           All rights reserved.
*
* Filename:     mglmesa.h
* Version:      Revision: 1.1.1.1
*
* Language:     ANSI C
* Environment:  Any
*
* Description:  Header file for the Mesa/OpenGL interface bindings for the
*               SciTech MGL graphics library. Uses the MGL internal
*               device context structures to get direct access to the
*               high performance MGL rasterization functions for maximum
*               performance. Utilizes the VESA VBE/AF Accelerator Functions
*               via the MGL's accelerated device driver functions, as well
*               as basic DirectDraw accelerated functions provided by the
*               MGL.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the Free
* Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*
****************************************************************************/

#ifndef __MGLMESA_H
#define __MGLMESA_H

#include "mgraph.h"

/*------------------------- Function Prototypes ---------------------------*/

#ifdef  __cplusplus
extern "C" {            /* Use "C" linkage when in C++ mode */
#endif

#ifndef __WINDOWS__
#define GLAPIENTRY
#endif

#ifdef  __WINDOWS__
bool    GLAPIENTRY MGLMesaInitDLL(MGLCallbacks *cb,char *version);
#endif
void    GLAPIENTRY MGLMesaChooseVisual(MGLDC *dc,MGLVisual *visual);
bool    GLAPIENTRY MGLMesaSetVisual(MGLDC *dc,MGLVisual *visual);
bool    GLAPIENTRY MGLMesaCreateContext(MGLDC *dc,bool forceMemDC);
void    GLAPIENTRY MGLMesaDestroyContext(MGLDC *dc);
void    GLAPIENTRY MGLMesaMakeCurrent(MGLDC *dc);
void    GLAPIENTRY MGLMesaSwapBuffers(MGLDC *dc,bool waitVRT);

/* Palette manipulation support. The reason we provide palette manipulation
 * routines is so that when rendering in double buffered modes with a
 * software backbuffer, the palette for the backbuffer is kept consistent
 * with the hardware front buffer.
 */

void    GLAPIENTRY MGLMesaSetPaletteEntry(MGLDC *dc,int entry,uchar red,uchar green,uchar blue);
void    GLAPIENTRY MGLMesaSetPalette(MGLDC *dc,palette_t *pal,int numColors,int startIndex);
void    GLAPIENTRY MGLMesaRealizePalette(MGLDC *dc,int numColors,int startIndex,int waitVRT);

#ifdef  __cplusplus
}                       /* End of "C" linkage for C++   */
#endif  /* __cplusplus */

#endif  /* __MGLMESA_H */
