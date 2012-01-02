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
	VBO_ATTRIB_COLOR0 = 3,
	VBO_ATTRIB_COLOR1 = 4,
	VBO_ATTRIB_FOG = 5,
	VBO_ATTRIB_INDEX = 6,        
	VBO_ATTRIB_EDGEFLAG = 7,     
	VBO_ATTRIB_TEX0 = 8,
	VBO_ATTRIB_TEX1 = 9,
	VBO_ATTRIB_TEX2 = 10,
	VBO_ATTRIB_TEX3 = 11,
	VBO_ATTRIB_TEX4 = 12,
	VBO_ATTRIB_TEX5 = 13,
	VBO_ATTRIB_TEX6 = 14,
	VBO_ATTRIB_TEX7 = 15,

	VBO_ATTRIB_GENERIC0 = 16, /* Not used? */
	VBO_ATTRIB_GENERIC1 = 17,
	VBO_ATTRIB_GENERIC2 = 18,
	VBO_ATTRIB_GENERIC3 = 19,
	VBO_ATTRIB_GENERIC4 = 20,
	VBO_ATTRIB_GENERIC5 = 21,
	VBO_ATTRIB_GENERIC6 = 22,
	VBO_ATTRIB_GENERIC7 = 23,
	VBO_ATTRIB_GENERIC8 = 24,
	VBO_ATTRIB_GENERIC9 = 25,
	VBO_ATTRIB_GENERIC10 = 26,
	VBO_ATTRIB_GENERIC11 = 27,
	VBO_ATTRIB_GENERIC12 = 28,
	VBO_ATTRIB_GENERIC13 = 29,
	VBO_ATTRIB_GENERIC14 = 30,
	VBO_ATTRIB_GENERIC15 = 31,

	/* XXX: in the vertex program InputsRead flag, we alias
	 * materials and generics and use knowledge about the program
	 * (whether it is a fixed-function emulation) to
	 * differentiate.  Here we must keep them apart instead.
	 */
	VBO_ATTRIB_MAT_FRONT_AMBIENT = 32, 
	VBO_ATTRIB_MAT_BACK_AMBIENT = 33,
	VBO_ATTRIB_MAT_FRONT_DIFFUSE = 34,
	VBO_ATTRIB_MAT_BACK_DIFFUSE = 35,
	VBO_ATTRIB_MAT_FRONT_SPECULAR = 36,
	VBO_ATTRIB_MAT_BACK_SPECULAR = 37,
	VBO_ATTRIB_MAT_FRONT_EMISSION = 38,
	VBO_ATTRIB_MAT_BACK_EMISSION = 39,
	VBO_ATTRIB_MAT_FRONT_SHININESS = 40,
	VBO_ATTRIB_MAT_BACK_SHININESS = 41,
	VBO_ATTRIB_MAT_FRONT_INDEXES = 42,
	VBO_ATTRIB_MAT_BACK_INDEXES = 43, 

	VBO_ATTRIB_MAX = 44
};

#define VBO_ATTRIB_FIRST_MATERIAL VBO_ATTRIB_MAT_FRONT_AMBIENT
#define VBO_ATTRIB_LAST_MATERIAL VBO_ATTRIB_MAT_BACK_INDEXES

#define VBO_MAX_COPIED_VERTS 3

#endif
