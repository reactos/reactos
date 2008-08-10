/*
 * Mesa 3-D graphics library
 * Version:  3.0
 * Copyright (C) 1995-1998  Brian Paul
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
 */


/*
 * Windows driver by: Mark E. Peterson (markp@ic.mankato.mn.us)
 * Updated by Li Wei (liwei@aiar.xjtu.edu.cn)
 *
 *
 ***************************************************************
 *                     WMesa                                   *
 *                     version 2.3                             *	
 *                                                             *
 *                        By                                   *
 *                      Li Wei                                 *
 *       Institute of Artificial Intelligence & Robotics       *
 *       Xi'an Jiaotong University                             *
 *       Email: liwei@aiar.xjtu.edu.cn                         * 
 *       Web page: http://sun.aiar.xjtu.edu.cn                 *
 *                                                             *
 *	       July 7th, 1997				       *
 ***************************************************************
 */


#ifndef WMESA_H
#define WMESA_H


#ifdef __cplusplus
extern "C" {
#endif


#include "GL/gl.h"

#if defined(_MSV_VER) && !defined(__GNUC__)
#  pragma warning (disable:4273)
#  pragma warning( disable : 4244 ) /* '=' : conversion from 'const double ' to 'float ', possible loss of data */
#  pragma warning( disable : 4018 ) /* '<' : signed/unsigned mismatch */
#  pragma warning( disable : 4305 ) /* '=' : truncation from 'const double ' to 'float ' */
#  pragma warning( disable : 4013 ) /* 'function' undefined; assuming extern returning int */
#  pragma warning( disable : 4761 ) /* integral size mismatch in argument; conversion supplied */
#  pragma warning( disable : 4273 ) /* 'identifier' : inconsistent DLL linkage. dllexport assumed */
#  if (MESA_WARNQUIET>1)
#    pragma warning( disable : 4146 ) /* unary minus operator applied to unsigned type, result still unsigned */
#  endif
#endif

/*
 * This is the WMesa context 'handle':
 */
typedef struct wmesa_context *WMesaContext;



/*
 * Create a new WMesaContext for rendering into a window.  You must
 * have already created the window of correct visual type and with an
 * appropriate colormap.
 *
 * Input:
 *         hDC - Windows device or memory context
 *         Pal  - Palette to use
 *         rgb_flag - GL_TRUE = RGB mode,
 *                    GL_FALSE = color index mode
 *         db_flag - GL_TRUE = double-buffered,
 *                   GL_FALSE = single buffered
 *         alpha_flag - GL_TRUE = create software alpha buffer,
 *                      GL_FALSE = no software alpha buffer
 *
 * Note: Indexed mode requires double buffering under Windows.
 *
 * Return:  a WMesa_context or NULL if error.
 */
extern WMesaContext WMesaCreateContext(HDC hDC,HPALETTE* pPal,
                                       GLboolean rgb_flag,
                                       GLboolean db_flag,
                                       GLboolean alpha_flag);


/*
 * Destroy a rendering context as returned by WMesaCreateContext()
 */
extern void WMesaDestroyContext( WMesaContext ctx );



/*
 * Make the specified context the current one.
 */
extern void WMesaMakeCurrent( WMesaContext ctx, HDC hdc );


/*
 * Return a handle to the current context.
 */
extern WMesaContext WMesaGetCurrentContext( void );


/*
 * Swap the front and back buffers for the current context.  No action
 * taken if the context is not double buffered.
 */
extern void WMesaSwapBuffers(HDC hdc);


/*
 * In indexed color mode we need to know when the palette changes.
 */
extern void WMesaPaletteChange(HPALETTE Pal);

extern void WMesaMove(void);



#ifdef __cplusplus
}
#endif


#endif

