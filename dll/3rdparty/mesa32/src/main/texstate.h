/**
 * \file texstate.h
 * Texture state management.
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef TEXSTATE_H
#define TEXSTATE_H


#include "mtypes.h"


extern void
_mesa_copy_texture_state( const GLcontext *src, GLcontext *dst );

extern void
_mesa_print_texunit_state( GLcontext *ctx, GLuint unit );



/**
 * \name Called from API
 */
/*@{*/


/*
 * GL_ARB_multitexture
 */
extern void GLAPIENTRY
_mesa_ActiveTextureARB( GLenum target );

extern void GLAPIENTRY
_mesa_ClientActiveTextureARB( GLenum target );


/**
 * \name Initialization, state maintenance
 */
/*@{*/

extern void 
_mesa_update_texture( GLcontext *ctx, GLuint new_state );

extern GLboolean
_mesa_init_texture( GLcontext *ctx );

extern void 
_mesa_free_texture_data( GLcontext *ctx );

extern void
_mesa_update_default_objects_texture(GLcontext *ctx);

/*@}*/

#endif
