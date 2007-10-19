/* DO NOT EDIT - This file generated automatically by gl_table.py (from Mesa) script */

/*
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined( _GLAPI_TABLE_H_ )
#  define _GLAPI_TABLE_H_

#ifndef GLAPIENTRYP
#define GLAPIENTRYP
#endif

typedef void (*_glapi_proc)(void); /* generic function pointer */

struct _glapi_table
{
   void (GLAPIENTRYP NewList)(GLuint list, GLenum mode); /* 0 */
   void (GLAPIENTRYP EndList)(void); /* 1 */
   void (GLAPIENTRYP CallList)(GLuint list); /* 2 */
   void (GLAPIENTRYP CallLists)(GLsizei n, GLenum type, const GLvoid * lists); /* 3 */
   void (GLAPIENTRYP DeleteLists)(GLuint list, GLsizei range); /* 4 */
   GLuint (GLAPIENTRYP GenLists)(GLsizei range); /* 5 */
   void (GLAPIENTRYP ListBase)(GLuint base); /* 6 */
   void (GLAPIENTRYP Begin)(GLenum mode); /* 7 */
   void (GLAPIENTRYP Bitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap); /* 8 */
   void (GLAPIENTRYP Color3b)(GLbyte red, GLbyte green, GLbyte blue); /* 9 */
   void (GLAPIENTRYP Color3bv)(const GLbyte * v); /* 10 */
   void (GLAPIENTRYP Color3d)(GLdouble red, GLdouble green, GLdouble blue); /* 11 */
   void (GLAPIENTRYP Color3dv)(const GLdouble * v); /* 12 */
   void (GLAPIENTRYP Color3f)(GLfloat red, GLfloat green, GLfloat blue); /* 13 */
   void (GLAPIENTRYP Color3fv)(const GLfloat * v); /* 14 */
   void (GLAPIENTRYP Color3i)(GLint red, GLint green, GLint blue); /* 15 */
   void (GLAPIENTRYP Color3iv)(const GLint * v); /* 16 */
   void (GLAPIENTRYP Color3s)(GLshort red, GLshort green, GLshort blue); /* 17 */
   void (GLAPIENTRYP Color3sv)(const GLshort * v); /* 18 */
   void (GLAPIENTRYP Color3ub)(GLubyte red, GLubyte green, GLubyte blue); /* 19 */
   void (GLAPIENTRYP Color3ubv)(const GLubyte * v); /* 20 */
   void (GLAPIENTRYP Color3ui)(GLuint red, GLuint green, GLuint blue); /* 21 */
   void (GLAPIENTRYP Color3uiv)(const GLuint * v); /* 22 */
   void (GLAPIENTRYP Color3us)(GLushort red, GLushort green, GLushort blue); /* 23 */
   void (GLAPIENTRYP Color3usv)(const GLushort * v); /* 24 */
   void (GLAPIENTRYP Color4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha); /* 25 */
   void (GLAPIENTRYP Color4bv)(const GLbyte * v); /* 26 */
   void (GLAPIENTRYP Color4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha); /* 27 */
   void (GLAPIENTRYP Color4dv)(const GLdouble * v); /* 28 */
   void (GLAPIENTRYP Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); /* 29 */
   void (GLAPIENTRYP Color4fv)(const GLfloat * v); /* 30 */
   void (GLAPIENTRYP Color4i)(GLint red, GLint green, GLint blue, GLint alpha); /* 31 */
   void (GLAPIENTRYP Color4iv)(const GLint * v); /* 32 */
   void (GLAPIENTRYP Color4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha); /* 33 */
   void (GLAPIENTRYP Color4sv)(const GLshort * v); /* 34 */
   void (GLAPIENTRYP Color4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha); /* 35 */
   void (GLAPIENTRYP Color4ubv)(const GLubyte * v); /* 36 */
   void (GLAPIENTRYP Color4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha); /* 37 */
   void (GLAPIENTRYP Color4uiv)(const GLuint * v); /* 38 */
   void (GLAPIENTRYP Color4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha); /* 39 */
   void (GLAPIENTRYP Color4usv)(const GLushort * v); /* 40 */
   void (GLAPIENTRYP EdgeFlag)(GLboolean flag); /* 41 */
   void (GLAPIENTRYP EdgeFlagv)(const GLboolean * flag); /* 42 */
   void (GLAPIENTRYP End)(void); /* 43 */
   void (GLAPIENTRYP Indexd)(GLdouble c); /* 44 */
   void (GLAPIENTRYP Indexdv)(const GLdouble * c); /* 45 */
   void (GLAPIENTRYP Indexf)(GLfloat c); /* 46 */
   void (GLAPIENTRYP Indexfv)(const GLfloat * c); /* 47 */
   void (GLAPIENTRYP Indexi)(GLint c); /* 48 */
   void (GLAPIENTRYP Indexiv)(const GLint * c); /* 49 */
   void (GLAPIENTRYP Indexs)(GLshort c); /* 50 */
   void (GLAPIENTRYP Indexsv)(const GLshort * c); /* 51 */
   void (GLAPIENTRYP Normal3b)(GLbyte nx, GLbyte ny, GLbyte nz); /* 52 */
   void (GLAPIENTRYP Normal3bv)(const GLbyte * v); /* 53 */
   void (GLAPIENTRYP Normal3d)(GLdouble nx, GLdouble ny, GLdouble nz); /* 54 */
   void (GLAPIENTRYP Normal3dv)(const GLdouble * v); /* 55 */
   void (GLAPIENTRYP Normal3f)(GLfloat nx, GLfloat ny, GLfloat nz); /* 56 */
   void (GLAPIENTRYP Normal3fv)(const GLfloat * v); /* 57 */
   void (GLAPIENTRYP Normal3i)(GLint nx, GLint ny, GLint nz); /* 58 */
   void (GLAPIENTRYP Normal3iv)(const GLint * v); /* 59 */
   void (GLAPIENTRYP Normal3s)(GLshort nx, GLshort ny, GLshort nz); /* 60 */
   void (GLAPIENTRYP Normal3sv)(const GLshort * v); /* 61 */
   void (GLAPIENTRYP RasterPos2d)(GLdouble x, GLdouble y); /* 62 */
   void (GLAPIENTRYP RasterPos2dv)(const GLdouble * v); /* 63 */
   void (GLAPIENTRYP RasterPos2f)(GLfloat x, GLfloat y); /* 64 */
   void (GLAPIENTRYP RasterPos2fv)(const GLfloat * v); /* 65 */
   void (GLAPIENTRYP RasterPos2i)(GLint x, GLint y); /* 66 */
   void (GLAPIENTRYP RasterPos2iv)(const GLint * v); /* 67 */
   void (GLAPIENTRYP RasterPos2s)(GLshort x, GLshort y); /* 68 */
   void (GLAPIENTRYP RasterPos2sv)(const GLshort * v); /* 69 */
   void (GLAPIENTRYP RasterPos3d)(GLdouble x, GLdouble y, GLdouble z); /* 70 */
   void (GLAPIENTRYP RasterPos3dv)(const GLdouble * v); /* 71 */
   void (GLAPIENTRYP RasterPos3f)(GLfloat x, GLfloat y, GLfloat z); /* 72 */
   void (GLAPIENTRYP RasterPos3fv)(const GLfloat * v); /* 73 */
   void (GLAPIENTRYP RasterPos3i)(GLint x, GLint y, GLint z); /* 74 */
   void (GLAPIENTRYP RasterPos3iv)(const GLint * v); /* 75 */
   void (GLAPIENTRYP RasterPos3s)(GLshort x, GLshort y, GLshort z); /* 76 */
   void (GLAPIENTRYP RasterPos3sv)(const GLshort * v); /* 77 */
   void (GLAPIENTRYP RasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 78 */
   void (GLAPIENTRYP RasterPos4dv)(const GLdouble * v); /* 79 */
   void (GLAPIENTRYP RasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 80 */
   void (GLAPIENTRYP RasterPos4fv)(const GLfloat * v); /* 81 */
   void (GLAPIENTRYP RasterPos4i)(GLint x, GLint y, GLint z, GLint w); /* 82 */
   void (GLAPIENTRYP RasterPos4iv)(const GLint * v); /* 83 */
   void (GLAPIENTRYP RasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w); /* 84 */
   void (GLAPIENTRYP RasterPos4sv)(const GLshort * v); /* 85 */
   void (GLAPIENTRYP Rectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2); /* 86 */
   void (GLAPIENTRYP Rectdv)(const GLdouble * v1, const GLdouble * v2); /* 87 */
   void (GLAPIENTRYP Rectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2); /* 88 */
   void (GLAPIENTRYP Rectfv)(const GLfloat * v1, const GLfloat * v2); /* 89 */
   void (GLAPIENTRYP Recti)(GLint x1, GLint y1, GLint x2, GLint y2); /* 90 */
   void (GLAPIENTRYP Rectiv)(const GLint * v1, const GLint * v2); /* 91 */
   void (GLAPIENTRYP Rects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2); /* 92 */
   void (GLAPIENTRYP Rectsv)(const GLshort * v1, const GLshort * v2); /* 93 */
   void (GLAPIENTRYP TexCoord1d)(GLdouble s); /* 94 */
   void (GLAPIENTRYP TexCoord1dv)(const GLdouble * v); /* 95 */
   void (GLAPIENTRYP TexCoord1f)(GLfloat s); /* 96 */
   void (GLAPIENTRYP TexCoord1fv)(const GLfloat * v); /* 97 */
   void (GLAPIENTRYP TexCoord1i)(GLint s); /* 98 */
   void (GLAPIENTRYP TexCoord1iv)(const GLint * v); /* 99 */
   void (GLAPIENTRYP TexCoord1s)(GLshort s); /* 100 */
   void (GLAPIENTRYP TexCoord1sv)(const GLshort * v); /* 101 */
   void (GLAPIENTRYP TexCoord2d)(GLdouble s, GLdouble t); /* 102 */
   void (GLAPIENTRYP TexCoord2dv)(const GLdouble * v); /* 103 */
   void (GLAPIENTRYP TexCoord2f)(GLfloat s, GLfloat t); /* 104 */
   void (GLAPIENTRYP TexCoord2fv)(const GLfloat * v); /* 105 */
   void (GLAPIENTRYP TexCoord2i)(GLint s, GLint t); /* 106 */
   void (GLAPIENTRYP TexCoord2iv)(const GLint * v); /* 107 */
   void (GLAPIENTRYP TexCoord2s)(GLshort s, GLshort t); /* 108 */
   void (GLAPIENTRYP TexCoord2sv)(const GLshort * v); /* 109 */
   void (GLAPIENTRYP TexCoord3d)(GLdouble s, GLdouble t, GLdouble r); /* 110 */
   void (GLAPIENTRYP TexCoord3dv)(const GLdouble * v); /* 111 */
   void (GLAPIENTRYP TexCoord3f)(GLfloat s, GLfloat t, GLfloat r); /* 112 */
   void (GLAPIENTRYP TexCoord3fv)(const GLfloat * v); /* 113 */
   void (GLAPIENTRYP TexCoord3i)(GLint s, GLint t, GLint r); /* 114 */
   void (GLAPIENTRYP TexCoord3iv)(const GLint * v); /* 115 */
   void (GLAPIENTRYP TexCoord3s)(GLshort s, GLshort t, GLshort r); /* 116 */
   void (GLAPIENTRYP TexCoord3sv)(const GLshort * v); /* 117 */
   void (GLAPIENTRYP TexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q); /* 118 */
   void (GLAPIENTRYP TexCoord4dv)(const GLdouble * v); /* 119 */
   void (GLAPIENTRYP TexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q); /* 120 */
   void (GLAPIENTRYP TexCoord4fv)(const GLfloat * v); /* 121 */
   void (GLAPIENTRYP TexCoord4i)(GLint s, GLint t, GLint r, GLint q); /* 122 */
   void (GLAPIENTRYP TexCoord4iv)(const GLint * v); /* 123 */
   void (GLAPIENTRYP TexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q); /* 124 */
   void (GLAPIENTRYP TexCoord4sv)(const GLshort * v); /* 125 */
   void (GLAPIENTRYP Vertex2d)(GLdouble x, GLdouble y); /* 126 */
   void (GLAPIENTRYP Vertex2dv)(const GLdouble * v); /* 127 */
   void (GLAPIENTRYP Vertex2f)(GLfloat x, GLfloat y); /* 128 */
   void (GLAPIENTRYP Vertex2fv)(const GLfloat * v); /* 129 */
   void (GLAPIENTRYP Vertex2i)(GLint x, GLint y); /* 130 */
   void (GLAPIENTRYP Vertex2iv)(const GLint * v); /* 131 */
   void (GLAPIENTRYP Vertex2s)(GLshort x, GLshort y); /* 132 */
   void (GLAPIENTRYP Vertex2sv)(const GLshort * v); /* 133 */
   void (GLAPIENTRYP Vertex3d)(GLdouble x, GLdouble y, GLdouble z); /* 134 */
   void (GLAPIENTRYP Vertex3dv)(const GLdouble * v); /* 135 */
   void (GLAPIENTRYP Vertex3f)(GLfloat x, GLfloat y, GLfloat z); /* 136 */
   void (GLAPIENTRYP Vertex3fv)(const GLfloat * v); /* 137 */
   void (GLAPIENTRYP Vertex3i)(GLint x, GLint y, GLint z); /* 138 */
   void (GLAPIENTRYP Vertex3iv)(const GLint * v); /* 139 */
   void (GLAPIENTRYP Vertex3s)(GLshort x, GLshort y, GLshort z); /* 140 */
   void (GLAPIENTRYP Vertex3sv)(const GLshort * v); /* 141 */
   void (GLAPIENTRYP Vertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 142 */
   void (GLAPIENTRYP Vertex4dv)(const GLdouble * v); /* 143 */
   void (GLAPIENTRYP Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 144 */
   void (GLAPIENTRYP Vertex4fv)(const GLfloat * v); /* 145 */
   void (GLAPIENTRYP Vertex4i)(GLint x, GLint y, GLint z, GLint w); /* 146 */
   void (GLAPIENTRYP Vertex4iv)(const GLint * v); /* 147 */
   void (GLAPIENTRYP Vertex4s)(GLshort x, GLshort y, GLshort z, GLshort w); /* 148 */
   void (GLAPIENTRYP Vertex4sv)(const GLshort * v); /* 149 */
   void (GLAPIENTRYP ClipPlane)(GLenum plane, const GLdouble * equation); /* 150 */
   void (GLAPIENTRYP ColorMaterial)(GLenum face, GLenum mode); /* 151 */
   void (GLAPIENTRYP CullFace)(GLenum mode); /* 152 */
   void (GLAPIENTRYP Fogf)(GLenum pname, GLfloat param); /* 153 */
   void (GLAPIENTRYP Fogfv)(GLenum pname, const GLfloat * params); /* 154 */
   void (GLAPIENTRYP Fogi)(GLenum pname, GLint param); /* 155 */
   void (GLAPIENTRYP Fogiv)(GLenum pname, const GLint * params); /* 156 */
   void (GLAPIENTRYP FrontFace)(GLenum mode); /* 157 */
   void (GLAPIENTRYP Hint)(GLenum target, GLenum mode); /* 158 */
   void (GLAPIENTRYP Lightf)(GLenum light, GLenum pname, GLfloat param); /* 159 */
   void (GLAPIENTRYP Lightfv)(GLenum light, GLenum pname, const GLfloat * params); /* 160 */
   void (GLAPIENTRYP Lighti)(GLenum light, GLenum pname, GLint param); /* 161 */
   void (GLAPIENTRYP Lightiv)(GLenum light, GLenum pname, const GLint * params); /* 162 */
   void (GLAPIENTRYP LightModelf)(GLenum pname, GLfloat param); /* 163 */
   void (GLAPIENTRYP LightModelfv)(GLenum pname, const GLfloat * params); /* 164 */
   void (GLAPIENTRYP LightModeli)(GLenum pname, GLint param); /* 165 */
   void (GLAPIENTRYP LightModeliv)(GLenum pname, const GLint * params); /* 166 */
   void (GLAPIENTRYP LineStipple)(GLint factor, GLushort pattern); /* 167 */
   void (GLAPIENTRYP LineWidth)(GLfloat width); /* 168 */
   void (GLAPIENTRYP Materialf)(GLenum face, GLenum pname, GLfloat param); /* 169 */
   void (GLAPIENTRYP Materialfv)(GLenum face, GLenum pname, const GLfloat * params); /* 170 */
   void (GLAPIENTRYP Materiali)(GLenum face, GLenum pname, GLint param); /* 171 */
   void (GLAPIENTRYP Materialiv)(GLenum face, GLenum pname, const GLint * params); /* 172 */
   void (GLAPIENTRYP PointSize)(GLfloat size); /* 173 */
   void (GLAPIENTRYP PolygonMode)(GLenum face, GLenum mode); /* 174 */
   void (GLAPIENTRYP PolygonStipple)(const GLubyte * mask); /* 175 */
   void (GLAPIENTRYP Scissor)(GLint x, GLint y, GLsizei width, GLsizei height); /* 176 */
   void (GLAPIENTRYP ShadeModel)(GLenum mode); /* 177 */
   void (GLAPIENTRYP TexParameterf)(GLenum target, GLenum pname, GLfloat param); /* 178 */
   void (GLAPIENTRYP TexParameterfv)(GLenum target, GLenum pname, const GLfloat * params); /* 179 */
   void (GLAPIENTRYP TexParameteri)(GLenum target, GLenum pname, GLint param); /* 180 */
   void (GLAPIENTRYP TexParameteriv)(GLenum target, GLenum pname, const GLint * params); /* 181 */
   void (GLAPIENTRYP TexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels); /* 182 */
   void (GLAPIENTRYP TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels); /* 183 */
   void (GLAPIENTRYP TexEnvf)(GLenum target, GLenum pname, GLfloat param); /* 184 */
   void (GLAPIENTRYP TexEnvfv)(GLenum target, GLenum pname, const GLfloat * params); /* 185 */
   void (GLAPIENTRYP TexEnvi)(GLenum target, GLenum pname, GLint param); /* 186 */
   void (GLAPIENTRYP TexEnviv)(GLenum target, GLenum pname, const GLint * params); /* 187 */
   void (GLAPIENTRYP TexGend)(GLenum coord, GLenum pname, GLdouble param); /* 188 */
   void (GLAPIENTRYP TexGendv)(GLenum coord, GLenum pname, const GLdouble * params); /* 189 */
   void (GLAPIENTRYP TexGenf)(GLenum coord, GLenum pname, GLfloat param); /* 190 */
   void (GLAPIENTRYP TexGenfv)(GLenum coord, GLenum pname, const GLfloat * params); /* 191 */
   void (GLAPIENTRYP TexGeni)(GLenum coord, GLenum pname, GLint param); /* 192 */
   void (GLAPIENTRYP TexGeniv)(GLenum coord, GLenum pname, const GLint * params); /* 193 */
   void (GLAPIENTRYP FeedbackBuffer)(GLsizei size, GLenum type, GLfloat * buffer); /* 194 */
   void (GLAPIENTRYP SelectBuffer)(GLsizei size, GLuint * buffer); /* 195 */
   GLint (GLAPIENTRYP RenderMode)(GLenum mode); /* 196 */
   void (GLAPIENTRYP InitNames)(void); /* 197 */
   void (GLAPIENTRYP LoadName)(GLuint name); /* 198 */
   void (GLAPIENTRYP PassThrough)(GLfloat token); /* 199 */
   void (GLAPIENTRYP PopName)(void); /* 200 */
   void (GLAPIENTRYP PushName)(GLuint name); /* 201 */
   void (GLAPIENTRYP DrawBuffer)(GLenum mode); /* 202 */
   void (GLAPIENTRYP Clear)(GLbitfield mask); /* 203 */
   void (GLAPIENTRYP ClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); /* 204 */
   void (GLAPIENTRYP ClearIndex)(GLfloat c); /* 205 */
   void (GLAPIENTRYP ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha); /* 206 */
   void (GLAPIENTRYP ClearStencil)(GLint s); /* 207 */
   void (GLAPIENTRYP ClearDepth)(GLclampd depth); /* 208 */
   void (GLAPIENTRYP StencilMask)(GLuint mask); /* 209 */
   void (GLAPIENTRYP ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha); /* 210 */
   void (GLAPIENTRYP DepthMask)(GLboolean flag); /* 211 */
   void (GLAPIENTRYP IndexMask)(GLuint mask); /* 212 */
   void (GLAPIENTRYP Accum)(GLenum op, GLfloat value); /* 213 */
   void (GLAPIENTRYP Disable)(GLenum cap); /* 214 */
   void (GLAPIENTRYP Enable)(GLenum cap); /* 215 */
   void (GLAPIENTRYP Finish)(void); /* 216 */
   void (GLAPIENTRYP Flush)(void); /* 217 */
   void (GLAPIENTRYP PopAttrib)(void); /* 218 */
   void (GLAPIENTRYP PushAttrib)(GLbitfield mask); /* 219 */
   void (GLAPIENTRYP Map1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble * points); /* 220 */
   void (GLAPIENTRYP Map1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat * points); /* 221 */
   void (GLAPIENTRYP Map2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble * points); /* 222 */
   void (GLAPIENTRYP Map2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat * points); /* 223 */
   void (GLAPIENTRYP MapGrid1d)(GLint un, GLdouble u1, GLdouble u2); /* 224 */
   void (GLAPIENTRYP MapGrid1f)(GLint un, GLfloat u1, GLfloat u2); /* 225 */
   void (GLAPIENTRYP MapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2); /* 226 */
   void (GLAPIENTRYP MapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2); /* 227 */
   void (GLAPIENTRYP EvalCoord1d)(GLdouble u); /* 228 */
   void (GLAPIENTRYP EvalCoord1dv)(const GLdouble * u); /* 229 */
   void (GLAPIENTRYP EvalCoord1f)(GLfloat u); /* 230 */
   void (GLAPIENTRYP EvalCoord1fv)(const GLfloat * u); /* 231 */
   void (GLAPIENTRYP EvalCoord2d)(GLdouble u, GLdouble v); /* 232 */
   void (GLAPIENTRYP EvalCoord2dv)(const GLdouble * u); /* 233 */
   void (GLAPIENTRYP EvalCoord2f)(GLfloat u, GLfloat v); /* 234 */
   void (GLAPIENTRYP EvalCoord2fv)(const GLfloat * u); /* 235 */
   void (GLAPIENTRYP EvalMesh1)(GLenum mode, GLint i1, GLint i2); /* 236 */
   void (GLAPIENTRYP EvalPoint1)(GLint i); /* 237 */
   void (GLAPIENTRYP EvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2); /* 238 */
   void (GLAPIENTRYP EvalPoint2)(GLint i, GLint j); /* 239 */
   void (GLAPIENTRYP AlphaFunc)(GLenum func, GLclampf ref); /* 240 */
   void (GLAPIENTRYP BlendFunc)(GLenum sfactor, GLenum dfactor); /* 241 */
   void (GLAPIENTRYP LogicOp)(GLenum opcode); /* 242 */
   void (GLAPIENTRYP StencilFunc)(GLenum func, GLint ref, GLuint mask); /* 243 */
   void (GLAPIENTRYP StencilOp)(GLenum fail, GLenum zfail, GLenum zpass); /* 244 */
   void (GLAPIENTRYP DepthFunc)(GLenum func); /* 245 */
   void (GLAPIENTRYP PixelZoom)(GLfloat xfactor, GLfloat yfactor); /* 246 */
   void (GLAPIENTRYP PixelTransferf)(GLenum pname, GLfloat param); /* 247 */
   void (GLAPIENTRYP PixelTransferi)(GLenum pname, GLint param); /* 248 */
   void (GLAPIENTRYP PixelStoref)(GLenum pname, GLfloat param); /* 249 */
   void (GLAPIENTRYP PixelStorei)(GLenum pname, GLint param); /* 250 */
   void (GLAPIENTRYP PixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat * values); /* 251 */
   void (GLAPIENTRYP PixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint * values); /* 252 */
   void (GLAPIENTRYP PixelMapusv)(GLenum map, GLsizei mapsize, const GLushort * values); /* 253 */
   void (GLAPIENTRYP ReadBuffer)(GLenum mode); /* 254 */
   void (GLAPIENTRYP CopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type); /* 255 */
   void (GLAPIENTRYP ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels); /* 256 */
   void (GLAPIENTRYP DrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels); /* 257 */
   void (GLAPIENTRYP GetBooleanv)(GLenum pname, GLboolean * params); /* 258 */
   void (GLAPIENTRYP GetClipPlane)(GLenum plane, GLdouble * equation); /* 259 */
   void (GLAPIENTRYP GetDoublev)(GLenum pname, GLdouble * params); /* 260 */
   GLenum (GLAPIENTRYP GetError)(void); /* 261 */
   void (GLAPIENTRYP GetFloatv)(GLenum pname, GLfloat * params); /* 262 */
   void (GLAPIENTRYP GetIntegerv)(GLenum pname, GLint * params); /* 263 */
   void (GLAPIENTRYP GetLightfv)(GLenum light, GLenum pname, GLfloat * params); /* 264 */
   void (GLAPIENTRYP GetLightiv)(GLenum light, GLenum pname, GLint * params); /* 265 */
   void (GLAPIENTRYP GetMapdv)(GLenum target, GLenum query, GLdouble * v); /* 266 */
   void (GLAPIENTRYP GetMapfv)(GLenum target, GLenum query, GLfloat * v); /* 267 */
   void (GLAPIENTRYP GetMapiv)(GLenum target, GLenum query, GLint * v); /* 268 */
   void (GLAPIENTRYP GetMaterialfv)(GLenum face, GLenum pname, GLfloat * params); /* 269 */
   void (GLAPIENTRYP GetMaterialiv)(GLenum face, GLenum pname, GLint * params); /* 270 */
   void (GLAPIENTRYP GetPixelMapfv)(GLenum map, GLfloat * values); /* 271 */
   void (GLAPIENTRYP GetPixelMapuiv)(GLenum map, GLuint * values); /* 272 */
   void (GLAPIENTRYP GetPixelMapusv)(GLenum map, GLushort * values); /* 273 */
   void (GLAPIENTRYP GetPolygonStipple)(GLubyte * mask); /* 274 */
   const GLubyte * (GLAPIENTRYP GetString)(GLenum name); /* 275 */
   void (GLAPIENTRYP GetTexEnvfv)(GLenum target, GLenum pname, GLfloat * params); /* 276 */
   void (GLAPIENTRYP GetTexEnviv)(GLenum target, GLenum pname, GLint * params); /* 277 */
   void (GLAPIENTRYP GetTexGendv)(GLenum coord, GLenum pname, GLdouble * params); /* 278 */
   void (GLAPIENTRYP GetTexGenfv)(GLenum coord, GLenum pname, GLfloat * params); /* 279 */
   void (GLAPIENTRYP GetTexGeniv)(GLenum coord, GLenum pname, GLint * params); /* 280 */
   void (GLAPIENTRYP GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels); /* 281 */
   void (GLAPIENTRYP GetTexParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 282 */
   void (GLAPIENTRYP GetTexParameteriv)(GLenum target, GLenum pname, GLint * params); /* 283 */
   void (GLAPIENTRYP GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat * params); /* 284 */
   void (GLAPIENTRYP GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint * params); /* 285 */
   GLboolean (GLAPIENTRYP IsEnabled)(GLenum cap); /* 286 */
   GLboolean (GLAPIENTRYP IsList)(GLuint list); /* 287 */
   void (GLAPIENTRYP DepthRange)(GLclampd zNear, GLclampd zFar); /* 288 */
   void (GLAPIENTRYP Frustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar); /* 289 */
   void (GLAPIENTRYP LoadIdentity)(void); /* 290 */
   void (GLAPIENTRYP LoadMatrixf)(const GLfloat * m); /* 291 */
   void (GLAPIENTRYP LoadMatrixd)(const GLdouble * m); /* 292 */
   void (GLAPIENTRYP MatrixMode)(GLenum mode); /* 293 */
   void (GLAPIENTRYP MultMatrixf)(const GLfloat * m); /* 294 */
   void (GLAPIENTRYP MultMatrixd)(const GLdouble * m); /* 295 */
   void (GLAPIENTRYP Ortho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar); /* 296 */
   void (GLAPIENTRYP PopMatrix)(void); /* 297 */
   void (GLAPIENTRYP PushMatrix)(void); /* 298 */
   void (GLAPIENTRYP Rotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z); /* 299 */
   void (GLAPIENTRYP Rotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z); /* 300 */
   void (GLAPIENTRYP Scaled)(GLdouble x, GLdouble y, GLdouble z); /* 301 */
   void (GLAPIENTRYP Scalef)(GLfloat x, GLfloat y, GLfloat z); /* 302 */
   void (GLAPIENTRYP Translated)(GLdouble x, GLdouble y, GLdouble z); /* 303 */
   void (GLAPIENTRYP Translatef)(GLfloat x, GLfloat y, GLfloat z); /* 304 */
   void (GLAPIENTRYP Viewport)(GLint x, GLint y, GLsizei width, GLsizei height); /* 305 */
   void (GLAPIENTRYP ArrayElement)(GLint i); /* 306 */
   void (GLAPIENTRYP BindTexture)(GLenum target, GLuint texture); /* 307 */
   void (GLAPIENTRYP ColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 308 */
   void (GLAPIENTRYP DisableClientState)(GLenum array); /* 309 */
   void (GLAPIENTRYP DrawArrays)(GLenum mode, GLint first, GLsizei count); /* 310 */
   void (GLAPIENTRYP DrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices); /* 311 */
   void (GLAPIENTRYP EdgeFlagPointer)(GLsizei stride, const GLvoid * pointer); /* 312 */
   void (GLAPIENTRYP EnableClientState)(GLenum array); /* 313 */
   void (GLAPIENTRYP IndexPointer)(GLenum type, GLsizei stride, const GLvoid * pointer); /* 314 */
   void (GLAPIENTRYP Indexub)(GLubyte c); /* 315 */
   void (GLAPIENTRYP Indexubv)(const GLubyte * c); /* 316 */
   void (GLAPIENTRYP InterleavedArrays)(GLenum format, GLsizei stride, const GLvoid * pointer); /* 317 */
   void (GLAPIENTRYP NormalPointer)(GLenum type, GLsizei stride, const GLvoid * pointer); /* 318 */
   void (GLAPIENTRYP PolygonOffset)(GLfloat factor, GLfloat units); /* 319 */
   void (GLAPIENTRYP TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 320 */
   void (GLAPIENTRYP VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 321 */
   GLboolean (GLAPIENTRYP AreTexturesResident)(GLsizei n, const GLuint * textures, GLboolean * residences); /* 322 */
   void (GLAPIENTRYP CopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border); /* 323 */
   void (GLAPIENTRYP CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border); /* 324 */
   void (GLAPIENTRYP CopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width); /* 325 */
   void (GLAPIENTRYP CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height); /* 326 */
   void (GLAPIENTRYP DeleteTextures)(GLsizei n, const GLuint * textures); /* 327 */
   void (GLAPIENTRYP GenTextures)(GLsizei n, GLuint * textures); /* 328 */
   void (GLAPIENTRYP GetPointerv)(GLenum pname, GLvoid ** params); /* 329 */
   GLboolean (GLAPIENTRYP IsTexture)(GLuint texture); /* 330 */
   void (GLAPIENTRYP PrioritizeTextures)(GLsizei n, const GLuint * textures, const GLclampf * priorities); /* 331 */
   void (GLAPIENTRYP TexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels); /* 332 */
   void (GLAPIENTRYP TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels); /* 333 */
   void (GLAPIENTRYP PopClientAttrib)(void); /* 334 */
   void (GLAPIENTRYP PushClientAttrib)(GLbitfield mask); /* 335 */
   void (GLAPIENTRYP BlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha); /* 336 */
   void (GLAPIENTRYP BlendEquation)(GLenum mode); /* 337 */
   void (GLAPIENTRYP DrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices); /* 338 */
   void (GLAPIENTRYP ColorTable)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table); /* 339 */
   void (GLAPIENTRYP ColorTableParameterfv)(GLenum target, GLenum pname, const GLfloat * params); /* 340 */
   void (GLAPIENTRYP ColorTableParameteriv)(GLenum target, GLenum pname, const GLint * params); /* 341 */
   void (GLAPIENTRYP CopyColorTable)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width); /* 342 */
   void (GLAPIENTRYP GetColorTable)(GLenum target, GLenum format, GLenum type, GLvoid * table); /* 343 */
   void (GLAPIENTRYP GetColorTableParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 344 */
   void (GLAPIENTRYP GetColorTableParameteriv)(GLenum target, GLenum pname, GLint * params); /* 345 */
   void (GLAPIENTRYP ColorSubTable)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data); /* 346 */
   void (GLAPIENTRYP CopyColorSubTable)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width); /* 347 */
   void (GLAPIENTRYP ConvolutionFilter1D)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image); /* 348 */
   void (GLAPIENTRYP ConvolutionFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image); /* 349 */
   void (GLAPIENTRYP ConvolutionParameterf)(GLenum target, GLenum pname, GLfloat params); /* 350 */
   void (GLAPIENTRYP ConvolutionParameterfv)(GLenum target, GLenum pname, const GLfloat * params); /* 351 */
   void (GLAPIENTRYP ConvolutionParameteri)(GLenum target, GLenum pname, GLint params); /* 352 */
   void (GLAPIENTRYP ConvolutionParameteriv)(GLenum target, GLenum pname, const GLint * params); /* 353 */
   void (GLAPIENTRYP CopyConvolutionFilter1D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width); /* 354 */
   void (GLAPIENTRYP CopyConvolutionFilter2D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height); /* 355 */
   void (GLAPIENTRYP GetConvolutionFilter)(GLenum target, GLenum format, GLenum type, GLvoid * image); /* 356 */
   void (GLAPIENTRYP GetConvolutionParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 357 */
   void (GLAPIENTRYP GetConvolutionParameteriv)(GLenum target, GLenum pname, GLint * params); /* 358 */
   void (GLAPIENTRYP GetSeparableFilter)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span); /* 359 */
   void (GLAPIENTRYP SeparableFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column); /* 360 */
   void (GLAPIENTRYP GetHistogram)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values); /* 361 */
   void (GLAPIENTRYP GetHistogramParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 362 */
   void (GLAPIENTRYP GetHistogramParameteriv)(GLenum target, GLenum pname, GLint * params); /* 363 */
   void (GLAPIENTRYP GetMinmax)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values); /* 364 */
   void (GLAPIENTRYP GetMinmaxParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 365 */
   void (GLAPIENTRYP GetMinmaxParameteriv)(GLenum target, GLenum pname, GLint * params); /* 366 */
   void (GLAPIENTRYP Histogram)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink); /* 367 */
   void (GLAPIENTRYP Minmax)(GLenum target, GLenum internalformat, GLboolean sink); /* 368 */
   void (GLAPIENTRYP ResetHistogram)(GLenum target); /* 369 */
   void (GLAPIENTRYP ResetMinmax)(GLenum target); /* 370 */
   void (GLAPIENTRYP TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels); /* 371 */
   void (GLAPIENTRYP TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels); /* 372 */
   void (GLAPIENTRYP CopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height); /* 373 */
   void (GLAPIENTRYP ActiveTextureARB)(GLenum texture); /* 374 */
   void (GLAPIENTRYP ClientActiveTextureARB)(GLenum texture); /* 375 */
   void (GLAPIENTRYP MultiTexCoord1dARB)(GLenum target, GLdouble s); /* 376 */
   void (GLAPIENTRYP MultiTexCoord1dvARB)(GLenum target, const GLdouble * v); /* 377 */
   void (GLAPIENTRYP MultiTexCoord1fARB)(GLenum target, GLfloat s); /* 378 */
   void (GLAPIENTRYP MultiTexCoord1fvARB)(GLenum target, const GLfloat * v); /* 379 */
   void (GLAPIENTRYP MultiTexCoord1iARB)(GLenum target, GLint s); /* 380 */
   void (GLAPIENTRYP MultiTexCoord1ivARB)(GLenum target, const GLint * v); /* 381 */
   void (GLAPIENTRYP MultiTexCoord1sARB)(GLenum target, GLshort s); /* 382 */
   void (GLAPIENTRYP MultiTexCoord1svARB)(GLenum target, const GLshort * v); /* 383 */
   void (GLAPIENTRYP MultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t); /* 384 */
   void (GLAPIENTRYP MultiTexCoord2dvARB)(GLenum target, const GLdouble * v); /* 385 */
   void (GLAPIENTRYP MultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t); /* 386 */
   void (GLAPIENTRYP MultiTexCoord2fvARB)(GLenum target, const GLfloat * v); /* 387 */
   void (GLAPIENTRYP MultiTexCoord2iARB)(GLenum target, GLint s, GLint t); /* 388 */
   void (GLAPIENTRYP MultiTexCoord2ivARB)(GLenum target, const GLint * v); /* 389 */
   void (GLAPIENTRYP MultiTexCoord2sARB)(GLenum target, GLshort s, GLshort t); /* 390 */
   void (GLAPIENTRYP MultiTexCoord2svARB)(GLenum target, const GLshort * v); /* 391 */
   void (GLAPIENTRYP MultiTexCoord3dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r); /* 392 */
   void (GLAPIENTRYP MultiTexCoord3dvARB)(GLenum target, const GLdouble * v); /* 393 */
   void (GLAPIENTRYP MultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r); /* 394 */
   void (GLAPIENTRYP MultiTexCoord3fvARB)(GLenum target, const GLfloat * v); /* 395 */
   void (GLAPIENTRYP MultiTexCoord3iARB)(GLenum target, GLint s, GLint t, GLint r); /* 396 */
   void (GLAPIENTRYP MultiTexCoord3ivARB)(GLenum target, const GLint * v); /* 397 */
   void (GLAPIENTRYP MultiTexCoord3sARB)(GLenum target, GLshort s, GLshort t, GLshort r); /* 398 */
   void (GLAPIENTRYP MultiTexCoord3svARB)(GLenum target, const GLshort * v); /* 399 */
   void (GLAPIENTRYP MultiTexCoord4dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q); /* 400 */
   void (GLAPIENTRYP MultiTexCoord4dvARB)(GLenum target, const GLdouble * v); /* 401 */
   void (GLAPIENTRYP MultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q); /* 402 */
   void (GLAPIENTRYP MultiTexCoord4fvARB)(GLenum target, const GLfloat * v); /* 403 */
   void (GLAPIENTRYP MultiTexCoord4iARB)(GLenum target, GLint s, GLint t, GLint r, GLint q); /* 404 */
   void (GLAPIENTRYP MultiTexCoord4ivARB)(GLenum target, const GLint * v); /* 405 */
   void (GLAPIENTRYP MultiTexCoord4sARB)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q); /* 406 */
   void (GLAPIENTRYP MultiTexCoord4svARB)(GLenum target, const GLshort * v); /* 407 */
   void (GLAPIENTRYP LoadTransposeMatrixfARB)(const GLfloat * m); /* 408 */
   void (GLAPIENTRYP LoadTransposeMatrixdARB)(const GLdouble * m); /* 409 */
   void (GLAPIENTRYP MultTransposeMatrixfARB)(const GLfloat * m); /* 410 */
   void (GLAPIENTRYP MultTransposeMatrixdARB)(const GLdouble * m); /* 411 */
   void (GLAPIENTRYP SampleCoverageARB)(GLclampf value, GLboolean invert); /* 412 */
   void (GLAPIENTRYP DrawBuffersARB)(GLsizei n, const GLenum * bufs); /* 413 */
   void (GLAPIENTRYP PolygonOffsetEXT)(GLfloat factor, GLfloat bias); /* 414 */
   void (GLAPIENTRYP GetTexFilterFuncSGIS)(GLenum target, GLenum filter, GLfloat * weights); /* 415 */
   void (GLAPIENTRYP TexFilterFuncSGIS)(GLenum target, GLenum filter, GLsizei n, const GLfloat * weights); /* 416 */
   void (GLAPIENTRYP GetHistogramEXT)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values); /* 417 */
   void (GLAPIENTRYP GetHistogramParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params); /* 418 */
   void (GLAPIENTRYP GetHistogramParameterivEXT)(GLenum target, GLenum pname, GLint * params); /* 419 */
   void (GLAPIENTRYP GetMinmaxEXT)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values); /* 420 */
   void (GLAPIENTRYP GetMinmaxParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params); /* 421 */
   void (GLAPIENTRYP GetMinmaxParameterivEXT)(GLenum target, GLenum pname, GLint * params); /* 422 */
   void (GLAPIENTRYP GetConvolutionFilterEXT)(GLenum target, GLenum format, GLenum type, GLvoid * image); /* 423 */
   void (GLAPIENTRYP GetConvolutionParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params); /* 424 */
   void (GLAPIENTRYP GetConvolutionParameterivEXT)(GLenum target, GLenum pname, GLint * params); /* 425 */
   void (GLAPIENTRYP GetSeparableFilterEXT)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span); /* 426 */
   void (GLAPIENTRYP GetColorTableSGI)(GLenum target, GLenum format, GLenum type, GLvoid * table); /* 427 */
   void (GLAPIENTRYP GetColorTableParameterfvSGI)(GLenum target, GLenum pname, GLfloat * params); /* 428 */
   void (GLAPIENTRYP GetColorTableParameterivSGI)(GLenum target, GLenum pname, GLint * params); /* 429 */
   void (GLAPIENTRYP PixelTexGenSGIX)(GLenum mode); /* 430 */
   void (GLAPIENTRYP PixelTexGenParameteriSGIS)(GLenum pname, GLint param); /* 431 */
   void (GLAPIENTRYP PixelTexGenParameterivSGIS)(GLenum pname, const GLint * params); /* 432 */
   void (GLAPIENTRYP PixelTexGenParameterfSGIS)(GLenum pname, GLfloat param); /* 433 */
   void (GLAPIENTRYP PixelTexGenParameterfvSGIS)(GLenum pname, const GLfloat * params); /* 434 */
   void (GLAPIENTRYP GetPixelTexGenParameterivSGIS)(GLenum pname, GLint * params); /* 435 */
   void (GLAPIENTRYP GetPixelTexGenParameterfvSGIS)(GLenum pname, GLfloat * params); /* 436 */
   void (GLAPIENTRYP TexImage4DSGIS)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid * pixels); /* 437 */
   void (GLAPIENTRYP TexSubImage4DSGIS)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid * pixels); /* 438 */
   GLboolean (GLAPIENTRYP AreTexturesResidentEXT)(GLsizei n, const GLuint * textures, GLboolean * residences); /* 439 */
   void (GLAPIENTRYP GenTexturesEXT)(GLsizei n, GLuint * textures); /* 440 */
   GLboolean (GLAPIENTRYP IsTextureEXT)(GLuint texture); /* 441 */
   void (GLAPIENTRYP DetailTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat * points); /* 442 */
   void (GLAPIENTRYP GetDetailTexFuncSGIS)(GLenum target, GLfloat * points); /* 443 */
   void (GLAPIENTRYP SharpenTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat * points); /* 444 */
   void (GLAPIENTRYP GetSharpenTexFuncSGIS)(GLenum target, GLfloat * points); /* 445 */
   void (GLAPIENTRYP SampleMaskSGIS)(GLclampf value, GLboolean invert); /* 446 */
   void (GLAPIENTRYP SamplePatternSGIS)(GLenum pattern); /* 447 */
   void (GLAPIENTRYP ColorPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 448 */
   void (GLAPIENTRYP EdgeFlagPointerEXT)(GLsizei stride, GLsizei count, const GLboolean * pointer); /* 449 */
   void (GLAPIENTRYP IndexPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 450 */
   void (GLAPIENTRYP NormalPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 451 */
   void (GLAPIENTRYP TexCoordPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 452 */
   void (GLAPIENTRYP VertexPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 453 */
   void (GLAPIENTRYP SpriteParameterfSGIX)(GLenum pname, GLfloat param); /* 454 */
   void (GLAPIENTRYP SpriteParameterfvSGIX)(GLenum pname, const GLfloat * params); /* 455 */
   void (GLAPIENTRYP SpriteParameteriSGIX)(GLenum pname, GLint param); /* 456 */
   void (GLAPIENTRYP SpriteParameterivSGIX)(GLenum pname, const GLint * params); /* 457 */
   void (GLAPIENTRYP PointParameterfEXT)(GLenum pname, GLfloat param); /* 458 */
   void (GLAPIENTRYP PointParameterfvEXT)(GLenum pname, const GLfloat * params); /* 459 */
   GLint (GLAPIENTRYP GetInstrumentsSGIX)(void); /* 460 */
   void (GLAPIENTRYP InstrumentsBufferSGIX)(GLsizei size, GLint * buffer); /* 461 */
   GLint (GLAPIENTRYP PollInstrumentsSGIX)(GLint * marker_p); /* 462 */
   void (GLAPIENTRYP ReadInstrumentsSGIX)(GLint marker); /* 463 */
   void (GLAPIENTRYP StartInstrumentsSGIX)(void); /* 464 */
   void (GLAPIENTRYP StopInstrumentsSGIX)(GLint marker); /* 465 */
   void (GLAPIENTRYP FrameZoomSGIX)(GLint factor); /* 466 */
   void (GLAPIENTRYP TagSampleBufferSGIX)(void); /* 467 */
   void (GLAPIENTRYP ReferencePlaneSGIX)(const GLdouble * equation); /* 468 */
   void (GLAPIENTRYP FlushRasterSGIX)(void); /* 469 */
   void (GLAPIENTRYP GetListParameterfvSGIX)(GLuint list, GLenum pname, GLfloat * params); /* 470 */
   void (GLAPIENTRYP GetListParameterivSGIX)(GLuint list, GLenum pname, GLint * params); /* 471 */
   void (GLAPIENTRYP ListParameterfSGIX)(GLuint list, GLenum pname, GLfloat param); /* 472 */
   void (GLAPIENTRYP ListParameterfvSGIX)(GLuint list, GLenum pname, const GLfloat * params); /* 473 */
   void (GLAPIENTRYP ListParameteriSGIX)(GLuint list, GLenum pname, GLint param); /* 474 */
   void (GLAPIENTRYP ListParameterivSGIX)(GLuint list, GLenum pname, const GLint * params); /* 475 */
   void (GLAPIENTRYP FragmentColorMaterialSGIX)(GLenum face, GLenum mode); /* 476 */
   void (GLAPIENTRYP FragmentLightfSGIX)(GLenum light, GLenum pname, GLfloat param); /* 477 */
   void (GLAPIENTRYP FragmentLightfvSGIX)(GLenum light, GLenum pname, const GLfloat * params); /* 478 */
   void (GLAPIENTRYP FragmentLightiSGIX)(GLenum light, GLenum pname, GLint param); /* 479 */
   void (GLAPIENTRYP FragmentLightivSGIX)(GLenum light, GLenum pname, const GLint * params); /* 480 */
   void (GLAPIENTRYP FragmentLightModelfSGIX)(GLenum pname, GLfloat param); /* 481 */
   void (GLAPIENTRYP FragmentLightModelfvSGIX)(GLenum pname, const GLfloat * params); /* 482 */
   void (GLAPIENTRYP FragmentLightModeliSGIX)(GLenum pname, GLint param); /* 483 */
   void (GLAPIENTRYP FragmentLightModelivSGIX)(GLenum pname, const GLint * params); /* 484 */
   void (GLAPIENTRYP FragmentMaterialfSGIX)(GLenum face, GLenum pname, GLfloat param); /* 485 */
   void (GLAPIENTRYP FragmentMaterialfvSGIX)(GLenum face, GLenum pname, const GLfloat * params); /* 486 */
   void (GLAPIENTRYP FragmentMaterialiSGIX)(GLenum face, GLenum pname, GLint param); /* 487 */
   void (GLAPIENTRYP FragmentMaterialivSGIX)(GLenum face, GLenum pname, const GLint * params); /* 488 */
   void (GLAPIENTRYP GetFragmentLightfvSGIX)(GLenum light, GLenum pname, GLfloat * params); /* 489 */
   void (GLAPIENTRYP GetFragmentLightivSGIX)(GLenum light, GLenum pname, GLint * params); /* 490 */
   void (GLAPIENTRYP GetFragmentMaterialfvSGIX)(GLenum face, GLenum pname, GLfloat * params); /* 491 */
   void (GLAPIENTRYP GetFragmentMaterialivSGIX)(GLenum face, GLenum pname, GLint * params); /* 492 */
   void (GLAPIENTRYP LightEnviSGIX)(GLenum pname, GLint param); /* 493 */
   void (GLAPIENTRYP VertexWeightfEXT)(GLfloat weight); /* 494 */
   void (GLAPIENTRYP VertexWeightfvEXT)(const GLfloat * weight); /* 495 */
   void (GLAPIENTRYP VertexWeightPointerEXT)(GLsizei size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 496 */
   void (GLAPIENTRYP FlushVertexArrayRangeNV)(void); /* 497 */
   void (GLAPIENTRYP VertexArrayRangeNV)(GLsizei length, const GLvoid * pointer); /* 498 */
   void (GLAPIENTRYP CombinerParameterfvNV)(GLenum pname, const GLfloat * params); /* 499 */
   void (GLAPIENTRYP CombinerParameterfNV)(GLenum pname, GLfloat param); /* 500 */
   void (GLAPIENTRYP CombinerParameterivNV)(GLenum pname, const GLint * params); /* 501 */
   void (GLAPIENTRYP CombinerParameteriNV)(GLenum pname, GLint param); /* 502 */
   void (GLAPIENTRYP CombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage); /* 503 */
   void (GLAPIENTRYP CombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum); /* 504 */
   void (GLAPIENTRYP FinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage); /* 505 */
   void (GLAPIENTRYP GetCombinerInputParameterfvNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat * params); /* 506 */
   void (GLAPIENTRYP GetCombinerInputParameterivNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint * params); /* 507 */
   void (GLAPIENTRYP GetCombinerOutputParameterfvNV)(GLenum stage, GLenum portion, GLenum pname, GLfloat * params); /* 508 */
   void (GLAPIENTRYP GetCombinerOutputParameterivNV)(GLenum stage, GLenum portion, GLenum pname, GLint * params); /* 509 */
   void (GLAPIENTRYP GetFinalCombinerInputParameterfvNV)(GLenum variable, GLenum pname, GLfloat * params); /* 510 */
   void (GLAPIENTRYP GetFinalCombinerInputParameterivNV)(GLenum variable, GLenum pname, GLint * params); /* 511 */
   void (GLAPIENTRYP ResizeBuffersMESA)(void); /* 512 */
   void (GLAPIENTRYP WindowPos2dMESA)(GLdouble x, GLdouble y); /* 513 */
   void (GLAPIENTRYP WindowPos2dvMESA)(const GLdouble * v); /* 514 */
   void (GLAPIENTRYP WindowPos2fMESA)(GLfloat x, GLfloat y); /* 515 */
   void (GLAPIENTRYP WindowPos2fvMESA)(const GLfloat * v); /* 516 */
   void (GLAPIENTRYP WindowPos2iMESA)(GLint x, GLint y); /* 517 */
   void (GLAPIENTRYP WindowPos2ivMESA)(const GLint * v); /* 518 */
   void (GLAPIENTRYP WindowPos2sMESA)(GLshort x, GLshort y); /* 519 */
   void (GLAPIENTRYP WindowPos2svMESA)(const GLshort * v); /* 520 */
   void (GLAPIENTRYP WindowPos3dMESA)(GLdouble x, GLdouble y, GLdouble z); /* 521 */
   void (GLAPIENTRYP WindowPos3dvMESA)(const GLdouble * v); /* 522 */
   void (GLAPIENTRYP WindowPos3fMESA)(GLfloat x, GLfloat y, GLfloat z); /* 523 */
   void (GLAPIENTRYP WindowPos3fvMESA)(const GLfloat * v); /* 524 */
   void (GLAPIENTRYP WindowPos3iMESA)(GLint x, GLint y, GLint z); /* 525 */
   void (GLAPIENTRYP WindowPos3ivMESA)(const GLint * v); /* 526 */
   void (GLAPIENTRYP WindowPos3sMESA)(GLshort x, GLshort y, GLshort z); /* 527 */
   void (GLAPIENTRYP WindowPos3svMESA)(const GLshort * v); /* 528 */
   void (GLAPIENTRYP WindowPos4dMESA)(GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 529 */
   void (GLAPIENTRYP WindowPos4dvMESA)(const GLdouble * v); /* 530 */
   void (GLAPIENTRYP WindowPos4fMESA)(GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 531 */
   void (GLAPIENTRYP WindowPos4fvMESA)(const GLfloat * v); /* 532 */
   void (GLAPIENTRYP WindowPos4iMESA)(GLint x, GLint y, GLint z, GLint w); /* 533 */
   void (GLAPIENTRYP WindowPos4ivMESA)(const GLint * v); /* 534 */
   void (GLAPIENTRYP WindowPos4sMESA)(GLshort x, GLshort y, GLshort z, GLshort w); /* 535 */
   void (GLAPIENTRYP WindowPos4svMESA)(const GLshort * v); /* 536 */
   void (GLAPIENTRYP BlendFuncSeparateEXT)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha); /* 537 */
   void (GLAPIENTRYP IndexMaterialEXT)(GLenum face, GLenum mode); /* 538 */
   void (GLAPIENTRYP IndexFuncEXT)(GLenum func, GLclampf ref); /* 539 */
   void (GLAPIENTRYP LockArraysEXT)(GLint first, GLsizei count); /* 540 */
   void (GLAPIENTRYP UnlockArraysEXT)(void); /* 541 */
   void (GLAPIENTRYP CullParameterdvEXT)(GLenum pname, GLdouble * params); /* 542 */
   void (GLAPIENTRYP CullParameterfvEXT)(GLenum pname, GLfloat * params); /* 543 */
   void (GLAPIENTRYP HintPGI)(GLenum target, GLint mode); /* 544 */
   void (GLAPIENTRYP FogCoordfEXT)(GLfloat coord); /* 545 */
   void (GLAPIENTRYP FogCoordfvEXT)(const GLfloat * coord); /* 546 */
   void (GLAPIENTRYP FogCoorddEXT)(GLdouble coord); /* 547 */
   void (GLAPIENTRYP FogCoorddvEXT)(const GLdouble * coord); /* 548 */
   void (GLAPIENTRYP FogCoordPointerEXT)(GLenum type, GLsizei stride, const GLvoid * pointer); /* 549 */
   void (GLAPIENTRYP GetColorTableEXT)(GLenum target, GLenum format, GLenum type, GLvoid * data); /* 550 */
   void (GLAPIENTRYP GetColorTableParameterivEXT)(GLenum target, GLenum pname, GLint * params); /* 551 */
   void (GLAPIENTRYP GetColorTableParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params); /* 552 */
   void (GLAPIENTRYP TbufferMask3DFX)(GLuint mask); /* 553 */
   void (GLAPIENTRYP CompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data); /* 554 */
   void (GLAPIENTRYP CompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data); /* 555 */
   void (GLAPIENTRYP CompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data); /* 556 */
   void (GLAPIENTRYP CompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data); /* 557 */
   void (GLAPIENTRYP CompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data); /* 558 */
   void (GLAPIENTRYP CompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data); /* 559 */
   void (GLAPIENTRYP GetCompressedTexImageARB)(GLenum target, GLint level, GLvoid * img); /* 560 */
   void (GLAPIENTRYP SecondaryColor3bEXT)(GLbyte red, GLbyte green, GLbyte blue); /* 561 */
   void (GLAPIENTRYP SecondaryColor3bvEXT)(const GLbyte * v); /* 562 */
   void (GLAPIENTRYP SecondaryColor3dEXT)(GLdouble red, GLdouble green, GLdouble blue); /* 563 */
   void (GLAPIENTRYP SecondaryColor3dvEXT)(const GLdouble * v); /* 564 */
   void (GLAPIENTRYP SecondaryColor3fEXT)(GLfloat red, GLfloat green, GLfloat blue); /* 565 */
   void (GLAPIENTRYP SecondaryColor3fvEXT)(const GLfloat * v); /* 566 */
   void (GLAPIENTRYP SecondaryColor3iEXT)(GLint red, GLint green, GLint blue); /* 567 */
   void (GLAPIENTRYP SecondaryColor3ivEXT)(const GLint * v); /* 568 */
   void (GLAPIENTRYP SecondaryColor3sEXT)(GLshort red, GLshort green, GLshort blue); /* 569 */
   void (GLAPIENTRYP SecondaryColor3svEXT)(const GLshort * v); /* 570 */
   void (GLAPIENTRYP SecondaryColor3ubEXT)(GLubyte red, GLubyte green, GLubyte blue); /* 571 */
   void (GLAPIENTRYP SecondaryColor3ubvEXT)(const GLubyte * v); /* 572 */
   void (GLAPIENTRYP SecondaryColor3uiEXT)(GLuint red, GLuint green, GLuint blue); /* 573 */
   void (GLAPIENTRYP SecondaryColor3uivEXT)(const GLuint * v); /* 574 */
   void (GLAPIENTRYP SecondaryColor3usEXT)(GLushort red, GLushort green, GLushort blue); /* 575 */
   void (GLAPIENTRYP SecondaryColor3usvEXT)(const GLushort * v); /* 576 */
   void (GLAPIENTRYP SecondaryColorPointerEXT)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 577 */
   GLboolean (GLAPIENTRYP AreProgramsResidentNV)(GLsizei n, const GLuint * ids, GLboolean * residences); /* 578 */
   void (GLAPIENTRYP BindProgramNV)(GLenum target, GLuint program); /* 579 */
   void (GLAPIENTRYP DeleteProgramsNV)(GLsizei n, const GLuint * programs); /* 580 */
   void (GLAPIENTRYP ExecuteProgramNV)(GLenum target, GLuint id, const GLfloat * params); /* 581 */
   void (GLAPIENTRYP GenProgramsNV)(GLsizei n, GLuint * programs); /* 582 */
   void (GLAPIENTRYP GetProgramParameterdvNV)(GLenum target, GLuint index, GLenum pname, GLdouble * params); /* 583 */
   void (GLAPIENTRYP GetProgramParameterfvNV)(GLenum target, GLuint index, GLenum pname, GLfloat * params); /* 584 */
   void (GLAPIENTRYP GetProgramivNV)(GLuint id, GLenum pname, GLint * params); /* 585 */
   void (GLAPIENTRYP GetProgramStringNV)(GLuint id, GLenum pname, GLubyte * program); /* 586 */
   void (GLAPIENTRYP GetTrackMatrixivNV)(GLenum target, GLuint address, GLenum pname, GLint * params); /* 587 */
   void (GLAPIENTRYP GetVertexAttribdvARB)(GLuint index, GLenum pname, GLdouble * params); /* 588 */
   void (GLAPIENTRYP GetVertexAttribfvARB)(GLuint index, GLenum pname, GLfloat * params); /* 589 */
   void (GLAPIENTRYP GetVertexAttribivARB)(GLuint index, GLenum pname, GLint * params); /* 590 */
   void (GLAPIENTRYP GetVertexAttribPointervNV)(GLuint index, GLenum pname, GLvoid ** params); /* 591 */
   GLboolean (GLAPIENTRYP IsProgramNV)(GLuint program); /* 592 */
   void (GLAPIENTRYP LoadProgramNV)(GLenum target, GLuint id, GLsizei len, const GLubyte * program); /* 593 */
   void (GLAPIENTRYP ProgramParameter4dNV)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 594 */
   void (GLAPIENTRYP ProgramParameter4dvNV)(GLenum target, GLuint index, const GLdouble * params); /* 595 */
   void (GLAPIENTRYP ProgramParameter4fNV)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 596 */
   void (GLAPIENTRYP ProgramParameter4fvNV)(GLenum target, GLuint index, const GLfloat * params); /* 597 */
   void (GLAPIENTRYP ProgramParameters4dvNV)(GLenum target, GLuint index, GLuint num, const GLdouble * params); /* 598 */
   void (GLAPIENTRYP ProgramParameters4fvNV)(GLenum target, GLuint index, GLuint num, const GLfloat * params); /* 599 */
   void (GLAPIENTRYP RequestResidentProgramsNV)(GLsizei n, const GLuint * ids); /* 600 */
   void (GLAPIENTRYP TrackMatrixNV)(GLenum target, GLuint address, GLenum matrix, GLenum transform); /* 601 */
   void (GLAPIENTRYP VertexAttribPointerNV)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 602 */
   void (GLAPIENTRYP VertexAttrib1dARB)(GLuint index, GLdouble x); /* 603 */
   void (GLAPIENTRYP VertexAttrib1dvARB)(GLuint index, const GLdouble * v); /* 604 */
   void (GLAPIENTRYP VertexAttrib1fARB)(GLuint index, GLfloat x); /* 605 */
   void (GLAPIENTRYP VertexAttrib1fvARB)(GLuint index, const GLfloat * v); /* 606 */
   void (GLAPIENTRYP VertexAttrib1sARB)(GLuint index, GLshort x); /* 607 */
   void (GLAPIENTRYP VertexAttrib1svARB)(GLuint index, const GLshort * v); /* 608 */
   void (GLAPIENTRYP VertexAttrib2dARB)(GLuint index, GLdouble x, GLdouble y); /* 609 */
   void (GLAPIENTRYP VertexAttrib2dvARB)(GLuint index, const GLdouble * v); /* 610 */
   void (GLAPIENTRYP VertexAttrib2fARB)(GLuint index, GLfloat x, GLfloat y); /* 611 */
   void (GLAPIENTRYP VertexAttrib2fvARB)(GLuint index, const GLfloat * v); /* 612 */
   void (GLAPIENTRYP VertexAttrib2sARB)(GLuint index, GLshort x, GLshort y); /* 613 */
   void (GLAPIENTRYP VertexAttrib2svARB)(GLuint index, const GLshort * v); /* 614 */
   void (GLAPIENTRYP VertexAttrib3dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z); /* 615 */
   void (GLAPIENTRYP VertexAttrib3dvARB)(GLuint index, const GLdouble * v); /* 616 */
   void (GLAPIENTRYP VertexAttrib3fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z); /* 617 */
   void (GLAPIENTRYP VertexAttrib3fvARB)(GLuint index, const GLfloat * v); /* 618 */
   void (GLAPIENTRYP VertexAttrib3sARB)(GLuint index, GLshort x, GLshort y, GLshort z); /* 619 */
   void (GLAPIENTRYP VertexAttrib3svARB)(GLuint index, const GLshort * v); /* 620 */
   void (GLAPIENTRYP VertexAttrib4dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 621 */
   void (GLAPIENTRYP VertexAttrib4dvARB)(GLuint index, const GLdouble * v); /* 622 */
   void (GLAPIENTRYP VertexAttrib4fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 623 */
   void (GLAPIENTRYP VertexAttrib4fvARB)(GLuint index, const GLfloat * v); /* 624 */
   void (GLAPIENTRYP VertexAttrib4sARB)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w); /* 625 */
   void (GLAPIENTRYP VertexAttrib4svARB)(GLuint index, const GLshort * v); /* 626 */
   void (GLAPIENTRYP VertexAttrib4NubARB)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w); /* 627 */
   void (GLAPIENTRYP VertexAttrib4NubvARB)(GLuint index, const GLubyte * v); /* 628 */
   void (GLAPIENTRYP VertexAttribs1dvNV)(GLuint index, GLsizei n, const GLdouble * v); /* 629 */
   void (GLAPIENTRYP VertexAttribs1fvNV)(GLuint index, GLsizei n, const GLfloat * v); /* 630 */
   void (GLAPIENTRYP VertexAttribs1svNV)(GLuint index, GLsizei n, const GLshort * v); /* 631 */
   void (GLAPIENTRYP VertexAttribs2dvNV)(GLuint index, GLsizei n, const GLdouble * v); /* 632 */
   void (GLAPIENTRYP VertexAttribs2fvNV)(GLuint index, GLsizei n, const GLfloat * v); /* 633 */
   void (GLAPIENTRYP VertexAttribs2svNV)(GLuint index, GLsizei n, const GLshort * v); /* 634 */
   void (GLAPIENTRYP VertexAttribs3dvNV)(GLuint index, GLsizei n, const GLdouble * v); /* 635 */
   void (GLAPIENTRYP VertexAttribs3fvNV)(GLuint index, GLsizei n, const GLfloat * v); /* 636 */
   void (GLAPIENTRYP VertexAttribs3svNV)(GLuint index, GLsizei n, const GLshort * v); /* 637 */
   void (GLAPIENTRYP VertexAttribs4dvNV)(GLuint index, GLsizei n, const GLdouble * v); /* 638 */
   void (GLAPIENTRYP VertexAttribs4fvNV)(GLuint index, GLsizei n, const GLfloat * v); /* 639 */
   void (GLAPIENTRYP VertexAttribs4svNV)(GLuint index, GLsizei n, const GLshort * v); /* 640 */
   void (GLAPIENTRYP VertexAttribs4ubvNV)(GLuint index, GLsizei n, const GLubyte * v); /* 641 */
   void (GLAPIENTRYP PointParameteriNV)(GLenum pname, GLint param); /* 642 */
   void (GLAPIENTRYP PointParameterivNV)(GLenum pname, const GLint * params); /* 643 */
   void (GLAPIENTRYP MultiDrawArraysEXT)(GLenum mode, GLint * first, GLsizei * count, GLsizei primcount); /* 644 */
   void (GLAPIENTRYP MultiDrawElementsEXT)(GLenum mode, const GLsizei * count, GLenum type, const GLvoid ** indices, GLsizei primcount); /* 645 */
   void (GLAPIENTRYP ActiveStencilFaceEXT)(GLenum face); /* 646 */
   void (GLAPIENTRYP DeleteFencesNV)(GLsizei n, const GLuint * fences); /* 647 */
   void (GLAPIENTRYP GenFencesNV)(GLsizei n, GLuint * fences); /* 648 */
   GLboolean (GLAPIENTRYP IsFenceNV)(GLuint fence); /* 649 */
   GLboolean (GLAPIENTRYP TestFenceNV)(GLuint fence); /* 650 */
   void (GLAPIENTRYP GetFenceivNV)(GLuint fence, GLenum pname, GLint * params); /* 651 */
   void (GLAPIENTRYP FinishFenceNV)(GLuint fence); /* 652 */
   void (GLAPIENTRYP SetFenceNV)(GLuint fence, GLenum condition); /* 653 */
   void (GLAPIENTRYP VertexAttrib4bvARB)(GLuint index, const GLbyte * v); /* 654 */
   void (GLAPIENTRYP VertexAttrib4ivARB)(GLuint index, const GLint * v); /* 655 */
   void (GLAPIENTRYP VertexAttrib4ubvARB)(GLuint index, const GLubyte * v); /* 656 */
   void (GLAPIENTRYP VertexAttrib4usvARB)(GLuint index, const GLushort * v); /* 657 */
   void (GLAPIENTRYP VertexAttrib4uivARB)(GLuint index, const GLuint * v); /* 658 */
   void (GLAPIENTRYP VertexAttrib4NbvARB)(GLuint index, const GLbyte * v); /* 659 */
   void (GLAPIENTRYP VertexAttrib4NsvARB)(GLuint index, const GLshort * v); /* 660 */
   void (GLAPIENTRYP VertexAttrib4NivARB)(GLuint index, const GLint * v); /* 661 */
   void (GLAPIENTRYP VertexAttrib4NusvARB)(GLuint index, const GLushort * v); /* 662 */
   void (GLAPIENTRYP VertexAttrib4NuivARB)(GLuint index, const GLuint * v); /* 663 */
   void (GLAPIENTRYP VertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer); /* 664 */
   void (GLAPIENTRYP EnableVertexAttribArrayARB)(GLuint index); /* 665 */
   void (GLAPIENTRYP DisableVertexAttribArrayARB)(GLuint index); /* 666 */
   void (GLAPIENTRYP ProgramStringARB)(GLenum target, GLenum format, GLsizei len, const GLvoid * string); /* 667 */
   void (GLAPIENTRYP ProgramEnvParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 668 */
   void (GLAPIENTRYP ProgramEnvParameter4dvARB)(GLenum target, GLuint index, const GLdouble * params); /* 669 */
   void (GLAPIENTRYP ProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 670 */
   void (GLAPIENTRYP ProgramEnvParameter4fvARB)(GLenum target, GLuint index, const GLfloat * params); /* 671 */
   void (GLAPIENTRYP ProgramLocalParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 672 */
   void (GLAPIENTRYP ProgramLocalParameter4dvARB)(GLenum target, GLuint index, const GLdouble * params); /* 673 */
   void (GLAPIENTRYP ProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 674 */
   void (GLAPIENTRYP ProgramLocalParameter4fvARB)(GLenum target, GLuint index, const GLfloat * params); /* 675 */
   void (GLAPIENTRYP GetProgramEnvParameterdvARB)(GLenum target, GLuint index, GLdouble * params); /* 676 */
   void (GLAPIENTRYP GetProgramEnvParameterfvARB)(GLenum target, GLuint index, GLfloat * params); /* 677 */
   void (GLAPIENTRYP GetProgramLocalParameterdvARB)(GLenum target, GLuint index, GLdouble * params); /* 678 */
   void (GLAPIENTRYP GetProgramLocalParameterfvARB)(GLenum target, GLuint index, GLfloat * params); /* 679 */
   void (GLAPIENTRYP GetProgramivARB)(GLenum target, GLenum pname, GLint * params); /* 680 */
   void (GLAPIENTRYP GetProgramStringARB)(GLenum target, GLenum pname, GLvoid * string); /* 681 */
   void (GLAPIENTRYP ProgramNamedParameter4fNV)(GLuint id, GLsizei len, const GLubyte * name, GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 682 */
   void (GLAPIENTRYP ProgramNamedParameter4dNV)(GLuint id, GLsizei len, const GLubyte * name, GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 683 */
   void (GLAPIENTRYP ProgramNamedParameter4fvNV)(GLuint id, GLsizei len, const GLubyte * name, const GLfloat * v); /* 684 */
   void (GLAPIENTRYP ProgramNamedParameter4dvNV)(GLuint id, GLsizei len, const GLubyte * name, const GLdouble * v); /* 685 */
   void (GLAPIENTRYP GetProgramNamedParameterfvNV)(GLuint id, GLsizei len, const GLubyte * name, GLfloat * params); /* 686 */
   void (GLAPIENTRYP GetProgramNamedParameterdvNV)(GLuint id, GLsizei len, const GLubyte * name, GLdouble * params); /* 687 */
   void (GLAPIENTRYP BindBufferARB)(GLenum target, GLuint buffer); /* 688 */
   void (GLAPIENTRYP BufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage); /* 689 */
   void (GLAPIENTRYP BufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid * data); /* 690 */
   void (GLAPIENTRYP DeleteBuffersARB)(GLsizei n, const GLuint * buffer); /* 691 */
   void (GLAPIENTRYP GenBuffersARB)(GLsizei n, GLuint * buffer); /* 692 */
   void (GLAPIENTRYP GetBufferParameterivARB)(GLenum target, GLenum pname, GLint * params); /* 693 */
   void (GLAPIENTRYP GetBufferPointervARB)(GLenum target, GLenum pname, GLvoid ** params); /* 694 */
   void (GLAPIENTRYP GetBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid * data); /* 695 */
   GLboolean (GLAPIENTRYP IsBufferARB)(GLuint buffer); /* 696 */
   GLvoid * (GLAPIENTRYP MapBufferARB)(GLenum target, GLenum access); /* 697 */
   GLboolean (GLAPIENTRYP UnmapBufferARB)(GLenum target); /* 698 */
   void (GLAPIENTRYP DepthBoundsEXT)(GLclampd zmin, GLclampd zmax); /* 699 */
   void (GLAPIENTRYP GenQueriesARB)(GLsizei n, GLuint * ids); /* 700 */
   void (GLAPIENTRYP DeleteQueriesARB)(GLsizei n, const GLuint * ids); /* 701 */
   GLboolean (GLAPIENTRYP IsQueryARB)(GLuint id); /* 702 */
   void (GLAPIENTRYP BeginQueryARB)(GLenum target, GLuint id); /* 703 */
   void (GLAPIENTRYP EndQueryARB)(GLenum target); /* 704 */
   void (GLAPIENTRYP GetQueryivARB)(GLenum target, GLenum pname, GLint * params); /* 705 */
   void (GLAPIENTRYP GetQueryObjectivARB)(GLuint id, GLenum pname, GLint * params); /* 706 */
   void (GLAPIENTRYP GetQueryObjectuivARB)(GLuint id, GLenum pname, GLuint * params); /* 707 */
   void (GLAPIENTRYP MultiModeDrawArraysIBM)(const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride); /* 708 */
   void (GLAPIENTRYP MultiModeDrawElementsIBM)(const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride); /* 709 */
   void (GLAPIENTRYP BlendEquationSeparateEXT)(GLenum modeRGB, GLenum modeA); /* 710 */
   void (GLAPIENTRYP DeleteObjectARB)(GLhandleARB obj); /* 711 */
   GLhandleARB (GLAPIENTRYP GetHandleARB)(GLenum pname); /* 712 */
   void (GLAPIENTRYP DetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj); /* 713 */
   GLhandleARB (GLAPIENTRYP CreateShaderObjectARB)(GLenum shaderType); /* 714 */
   void (GLAPIENTRYP ShaderSourceARB)(GLhandleARB shaderObj, GLsizei count, const GLcharARB ** string, const GLint * length); /* 715 */
   void (GLAPIENTRYP CompileShaderARB)(GLhandleARB shaderObj); /* 716 */
   GLhandleARB (GLAPIENTRYP CreateProgramObjectARB)(void); /* 717 */
   void (GLAPIENTRYP AttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj); /* 718 */
   void (GLAPIENTRYP LinkProgramARB)(GLhandleARB programObj); /* 719 */
   void (GLAPIENTRYP UseProgramObjectARB)(GLhandleARB programObj); /* 720 */
   void (GLAPIENTRYP ValidateProgramARB)(GLhandleARB programObj); /* 721 */
   void (GLAPIENTRYP Uniform1fARB)(GLint location, GLfloat v0); /* 722 */
   void (GLAPIENTRYP Uniform2fARB)(GLint location, GLfloat v0, GLfloat v1); /* 723 */
   void (GLAPIENTRYP Uniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2); /* 724 */
   void (GLAPIENTRYP Uniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3); /* 725 */
   void (GLAPIENTRYP Uniform1iARB)(GLint location, GLint v0); /* 726 */
   void (GLAPIENTRYP Uniform2iARB)(GLint location, GLint v0, GLint v1); /* 727 */
   void (GLAPIENTRYP Uniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2); /* 728 */
   void (GLAPIENTRYP Uniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3); /* 729 */
   void (GLAPIENTRYP Uniform1fvARB)(GLint location, GLsizei count, const GLfloat * value); /* 730 */
   void (GLAPIENTRYP Uniform2fvARB)(GLint location, GLsizei count, const GLfloat * value); /* 731 */
   void (GLAPIENTRYP Uniform3fvARB)(GLint location, GLsizei count, const GLfloat * value); /* 732 */
   void (GLAPIENTRYP Uniform4fvARB)(GLint location, GLsizei count, const GLfloat * value); /* 733 */
   void (GLAPIENTRYP Uniform1ivARB)(GLint location, GLsizei count, const GLint * value); /* 734 */
   void (GLAPIENTRYP Uniform2ivARB)(GLint location, GLsizei count, const GLint * value); /* 735 */
   void (GLAPIENTRYP Uniform3ivARB)(GLint location, GLsizei count, const GLint * value); /* 736 */
   void (GLAPIENTRYP Uniform4ivARB)(GLint location, GLsizei count, const GLint * value); /* 737 */
   void (GLAPIENTRYP UniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value); /* 738 */
   void (GLAPIENTRYP UniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value); /* 739 */
   void (GLAPIENTRYP UniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value); /* 740 */
   void (GLAPIENTRYP GetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat * params); /* 741 */
   void (GLAPIENTRYP GetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint * params); /* 742 */
   void (GLAPIENTRYP GetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * infoLog); /* 743 */
   void (GLAPIENTRYP GetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxLength, GLsizei * length, GLhandleARB * infoLog); /* 744 */
   GLint (GLAPIENTRYP GetUniformLocationARB)(GLhandleARB programObj, const GLcharARB * name); /* 745 */
   void (GLAPIENTRYP GetActiveUniformARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name); /* 746 */
   void (GLAPIENTRYP GetUniformfvARB)(GLhandleARB programObj, GLint location, GLfloat * params); /* 747 */
   void (GLAPIENTRYP GetUniformivARB)(GLhandleARB programObj, GLint location, GLint * params); /* 748 */
   void (GLAPIENTRYP GetShaderSourceARB)(GLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * source); /* 749 */
   void (GLAPIENTRYP BindAttribLocationARB)(GLhandleARB programObj, GLuint index, const GLcharARB * name); /* 750 */
   void (GLAPIENTRYP GetActiveAttribARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name); /* 751 */
   GLint (GLAPIENTRYP GetAttribLocationARB)(GLhandleARB programObj, const GLcharARB * name); /* 752 */
   void (GLAPIENTRYP GetVertexAttribdvNV)(GLuint index, GLenum pname, GLdouble * params); /* 753 */
   void (GLAPIENTRYP GetVertexAttribfvNV)(GLuint index, GLenum pname, GLfloat * params); /* 754 */
   void (GLAPIENTRYP GetVertexAttribivNV)(GLuint index, GLenum pname, GLint * params); /* 755 */
   void (GLAPIENTRYP VertexAttrib1dNV)(GLuint index, GLdouble x); /* 756 */
   void (GLAPIENTRYP VertexAttrib1dvNV)(GLuint index, const GLdouble * v); /* 757 */
   void (GLAPIENTRYP VertexAttrib1fNV)(GLuint index, GLfloat x); /* 758 */
   void (GLAPIENTRYP VertexAttrib1fvNV)(GLuint index, const GLfloat * v); /* 759 */
   void (GLAPIENTRYP VertexAttrib1sNV)(GLuint index, GLshort x); /* 760 */
   void (GLAPIENTRYP VertexAttrib1svNV)(GLuint index, const GLshort * v); /* 761 */
   void (GLAPIENTRYP VertexAttrib2dNV)(GLuint index, GLdouble x, GLdouble y); /* 762 */
   void (GLAPIENTRYP VertexAttrib2dvNV)(GLuint index, const GLdouble * v); /* 763 */
   void (GLAPIENTRYP VertexAttrib2fNV)(GLuint index, GLfloat x, GLfloat y); /* 764 */
   void (GLAPIENTRYP VertexAttrib2fvNV)(GLuint index, const GLfloat * v); /* 765 */
   void (GLAPIENTRYP VertexAttrib2sNV)(GLuint index, GLshort x, GLshort y); /* 766 */
   void (GLAPIENTRYP VertexAttrib2svNV)(GLuint index, const GLshort * v); /* 767 */
   void (GLAPIENTRYP VertexAttrib3dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z); /* 768 */
   void (GLAPIENTRYP VertexAttrib3dvNV)(GLuint index, const GLdouble * v); /* 769 */
   void (GLAPIENTRYP VertexAttrib3fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z); /* 770 */
   void (GLAPIENTRYP VertexAttrib3fvNV)(GLuint index, const GLfloat * v); /* 771 */
   void (GLAPIENTRYP VertexAttrib3sNV)(GLuint index, GLshort x, GLshort y, GLshort z); /* 772 */
   void (GLAPIENTRYP VertexAttrib3svNV)(GLuint index, const GLshort * v); /* 773 */
   void (GLAPIENTRYP VertexAttrib4dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 774 */
   void (GLAPIENTRYP VertexAttrib4dvNV)(GLuint index, const GLdouble * v); /* 775 */
   void (GLAPIENTRYP VertexAttrib4fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 776 */
   void (GLAPIENTRYP VertexAttrib4fvNV)(GLuint index, const GLfloat * v); /* 777 */
   void (GLAPIENTRYP VertexAttrib4sNV)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w); /* 778 */
   void (GLAPIENTRYP VertexAttrib4svNV)(GLuint index, const GLshort * v); /* 779 */
   void (GLAPIENTRYP VertexAttrib4ubNV)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w); /* 780 */
   void (GLAPIENTRYP VertexAttrib4ubvNV)(GLuint index, const GLubyte * v); /* 781 */
   GLuint (GLAPIENTRYP GenFragmentShadersATI)(GLuint range); /* 782 */
   void (GLAPIENTRYP BindFragmentShaderATI)(GLuint id); /* 783 */
   void (GLAPIENTRYP DeleteFragmentShaderATI)(GLuint id); /* 784 */
   void (GLAPIENTRYP BeginFragmentShaderATI)(void); /* 785 */
   void (GLAPIENTRYP EndFragmentShaderATI)(void); /* 786 */
   void (GLAPIENTRYP PassTexCoordATI)(GLuint dst, GLuint coord, GLenum swizzle); /* 787 */
   void (GLAPIENTRYP SampleMapATI)(GLuint dst, GLuint interp, GLenum swizzle); /* 788 */
   void (GLAPIENTRYP ColorFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod); /* 789 */
   void (GLAPIENTRYP ColorFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod); /* 790 */
   void (GLAPIENTRYP ColorFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod); /* 791 */
   void (GLAPIENTRYP AlphaFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod); /* 792 */
   void (GLAPIENTRYP AlphaFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod); /* 793 */
   void (GLAPIENTRYP AlphaFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod); /* 794 */
   void (GLAPIENTRYP SetFragmentShaderConstantATI)(GLuint dst, const GLfloat * value); /* 795 */
   GLboolean (GLAPIENTRYP IsRenderbufferEXT)(GLuint renderbuffer); /* 796 */
   void (GLAPIENTRYP BindRenderbufferEXT)(GLenum target, GLuint renderbuffer); /* 797 */
   void (GLAPIENTRYP DeleteRenderbuffersEXT)(GLsizei n, const GLuint * renderbuffers); /* 798 */
   void (GLAPIENTRYP GenRenderbuffersEXT)(GLsizei n, GLuint * renderbuffers); /* 799 */
   void (GLAPIENTRYP RenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height); /* 800 */
   void (GLAPIENTRYP GetRenderbufferParameterivEXT)(GLenum target, GLenum pname, GLint * params); /* 801 */
   GLboolean (GLAPIENTRYP IsFramebufferEXT)(GLuint framebuffer); /* 802 */
   void (GLAPIENTRYP BindFramebufferEXT)(GLenum target, GLuint framebuffer); /* 803 */
   void (GLAPIENTRYP DeleteFramebuffersEXT)(GLsizei n, const GLuint * framebuffers); /* 804 */
   void (GLAPIENTRYP GenFramebuffersEXT)(GLsizei n, GLuint * framebuffers); /* 805 */
   GLenum (GLAPIENTRYP CheckFramebufferStatusEXT)(GLenum target); /* 806 */
   void (GLAPIENTRYP FramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level); /* 807 */
   void (GLAPIENTRYP FramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level); /* 808 */
   void (GLAPIENTRYP FramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset); /* 809 */
   void (GLAPIENTRYP FramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer); /* 810 */
   void (GLAPIENTRYP GetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, GLint * params); /* 811 */
   void (GLAPIENTRYP GenerateMipmapEXT)(GLenum target); /* 812 */
   void (GLAPIENTRYP StencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask); /* 813 */
   void (GLAPIENTRYP StencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass); /* 814 */
   void (GLAPIENTRYP StencilMaskSeparate)(GLenum face, GLuint mask); /* 815 */
};

#endif /* !defined( _GLAPI_TABLE_H_ ) */
