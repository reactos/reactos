/* $Id: asm-386.h,v 1.1 1997/12/15 03:39:14 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
 * Copyright (C) 1995-1997  Brian Paul
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
 */


/*
 * $Log: asm-386.h,v $
 * Revision 1.1  1997/12/15 03:39:14  brianp
 * Initial revision
 *
 */


#ifndef ASM_386_H
#define ASM_386_H


#include "GL/gl.h"


/*
 * Prototypes for assembly functions.
 */


extern void asm_transform_points3_general( GLuint n, GLfloat d[][4],
                                           GLfloat m[16], GLfloat s[][4] );

extern void asm_transform_points3_identity( GLuint n, GLfloat d[][4],
                                            GLfloat s[][4] );

extern void asm_transform_points3_2d( GLuint n, GLfloat d[][4],
                                      GLfloat m[16], GLfloat s[][4] );

extern void asm_transform_points3_2d_no_rot( GLuint n, GLfloat d[][4],
                                             GLfloat m[16], GLfloat s[][4] );

extern void asm_transform_points3_3d( GLuint n, GLfloat d[][4], GLfloat m[16],
                                      GLfloat s[][4] );

extern void asm_transform_points4_general( GLuint n, GLfloat d[][4],
                                           GLfloat m[16], GLfloat s[][4] );

extern void asm_transform_points4_identity( GLuint n, GLfloat d[][4],
                                            GLfloat s[][4] );

extern void asm_transform_points4_2d( GLuint n, GLfloat d[][4], GLfloat m[16],
                                      GLfloat s[][4] );

extern void asm_transform_points4_2d_no_rot( GLuint n, GLfloat d[][4],
                                             GLfloat m[16], GLfloat s[][4] );

extern void asm_transform_points4_3d( GLuint n, GLfloat d[][4], GLfloat m[16],
                                      GLfloat s[][4] );

extern void asm_transform_points4_ortho( GLuint n, GLfloat d[][4],
                                         GLfloat m[16], GLfloat s[][4] );

extern void asm_transform_points4_perspective( GLuint n, GLfloat d[][4],
                                               GLfloat m[16], GLfloat s[][4] );

extern void asm_project_and_cliptest_general( GLuint n, GLfloat d[][4],
                                          GLfloat m[16],
                                          GLfloat s[][4], GLubyte clipmask[],
                                          GLubyte *ormask, GLubyte *andmask );

extern void asm_project_and_cliptest_identity( GLuint n, GLfloat d[][4],
                                          GLfloat s[][4], GLubyte clipmask[],
                                          GLubyte *ormask, GLubyte *andmask );

extern void asm_project_and_cliptest_ortho( GLuint n, GLfloat d[][4],
                                          GLfloat m[16],
                                          GLfloat s[][4], GLubyte clipmask[],
                                          GLubyte *ormask, GLubyte *andmask );

extern void asm_project_and_cliptest_perspective( GLuint n, GLfloat d[][4],
                                          GLfloat m[16],
                                          GLfloat s[][4], GLubyte clipmask[],
                                          GLubyte *ormask, GLubyte *andmask );

#endif
