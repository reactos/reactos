/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#ifndef VBO_ATTRIB_H
#define VBO_ATTRIB_H


/*
 * Note: The first attributes match the VERT_ATTRIB_* definitions
 * in mtypes.h.  However, the tnl module has additional attributes
 * for materials, color indexes, edge flags, etc.
 */
/* Although it's nice to use these as bit indexes in a DWORD flag, we
 * could manage without if necessary.  Another limit currently is the
 * number of bits allocated for these numbers in places like vertex
 * program instruction formats and register layouts.
 */
enum {
	VBO_ATTRIB_POS = 0,
	VBO_ATTRIB_WEIGHT = 1,
	VBO_ATTRIB_NORMAL = 2,
	VBO_ATTRIB_COLOR = 3,
	VBO_ATTRIB_FOG = 4,
	VBO_ATTRIB_INDEX = 5,
	VBO_ATTRIB_EDGEFLAG = 6,
	VBO_ATTRIB_TEX = 7,
	VBO_ATTRIB_POINT_SIZE = 8,
    
	VBO_ATTRIB_MAT_FRONT_AMBIENT = 9,
	VBO_ATTRIB_MAT_BACK_AMBIENT = 10,
	VBO_ATTRIB_MAT_FRONT_DIFFUSE = 11,
	VBO_ATTRIB_MAT_BACK_DIFFUSE = 12,
	VBO_ATTRIB_MAT_FRONT_SPECULAR = 13,
	VBO_ATTRIB_MAT_BACK_SPECULAR = 14,
	VBO_ATTRIB_MAT_FRONT_EMISSION = 15,
	VBO_ATTRIB_MAT_BACK_EMISSION = 16,
	VBO_ATTRIB_MAT_FRONT_SHININESS = 17,
	VBO_ATTRIB_MAT_BACK_SHININESS = 18,
	VBO_ATTRIB_MAT_FRONT_INDEXES = 19,
	VBO_ATTRIB_MAT_BACK_INDEXES = 20,

	VBO_ATTRIB_MAX = 21
};

#define VBO_ATTRIB_FIRST_MATERIAL VBO_ATTRIB_MAT_FRONT_AMBIENT
#define VBO_ATTRIB_LAST_MATERIAL VBO_ATTRIB_MAT_BACK_INDEXES

#define VBO_MAX_COPIED_VERTS 3

#endif
