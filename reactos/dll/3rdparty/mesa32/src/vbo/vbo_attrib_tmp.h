/**************************************************************************

Copyright 2002 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#define ATTR1FV( A, V ) ATTR( A, 1, (V)[0], 0, 0, 1 )
#define ATTR2FV( A, V ) ATTR( A, 2, (V)[0], (V)[1], 0, 1 )
#define ATTR3FV( A, V ) ATTR( A, 3, (V)[0], (V)[1], (V)[2], 1 )
#define ATTR4FV( A, V ) ATTR( A, 4, (V)[0], (V)[1], (V)[2], (V)[3] )

#define ATTR1F( A, X )          ATTR( A, 1, X, 0, 0, 1 )
#define ATTR2F( A, X, Y )       ATTR( A, 2, X, Y, 0, 1 )
#define ATTR3F( A, X, Y, Z )    ATTR( A, 3, X, Y, Z, 1 )
#define ATTR4F( A, X, Y, Z, W ) ATTR( A, 4, X, Y, Z, W )

#define MAT_ATTR( A, N, V ) ATTR( A, N, (V)[0], (V)[1], (V)[2], (V)[3] )

static void GLAPIENTRY TAG(Vertex2f)( GLfloat x, GLfloat y )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR2F( VBO_ATTRIB_POS, x, y );
}

static void GLAPIENTRY TAG(Vertex2fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR2FV( VBO_ATTRIB_POS, v );
}

static void GLAPIENTRY TAG(Vertex3f)( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3F( VBO_ATTRIB_POS, x, y, z );
}

static void GLAPIENTRY TAG(Vertex3fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3FV( VBO_ATTRIB_POS, v );
}

static void GLAPIENTRY TAG(Vertex4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR4F( VBO_ATTRIB_POS, x, y, z, w );
}

static void GLAPIENTRY TAG(Vertex4fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR4FV( VBO_ATTRIB_POS, v );
}

static void GLAPIENTRY TAG(TexCoord1f)( GLfloat x )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR1F( VBO_ATTRIB_TEX0, x );
}

static void GLAPIENTRY TAG(TexCoord1fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR1FV( VBO_ATTRIB_TEX0, v );
}

static void GLAPIENTRY TAG(TexCoord2f)( GLfloat x, GLfloat y )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR2F( VBO_ATTRIB_TEX0, x, y );
}

static void GLAPIENTRY TAG(TexCoord2fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR2FV( VBO_ATTRIB_TEX0, v );
}

static void GLAPIENTRY TAG(TexCoord3f)( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3F( VBO_ATTRIB_TEX0, x, y, z );
}

static void GLAPIENTRY TAG(TexCoord3fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3FV( VBO_ATTRIB_TEX0, v );
}

static void GLAPIENTRY TAG(TexCoord4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR4F( VBO_ATTRIB_TEX0, x, y, z, w );
}

static void GLAPIENTRY TAG(TexCoord4fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR4FV( VBO_ATTRIB_TEX0, v );
}

static void GLAPIENTRY TAG(Normal3f)( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3F( VBO_ATTRIB_NORMAL, x, y, z );
}

static void GLAPIENTRY TAG(Normal3fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3FV( VBO_ATTRIB_NORMAL, v );
}

static void GLAPIENTRY TAG(FogCoordfEXT)( GLfloat x )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR1F( VBO_ATTRIB_FOG, x );
}

static void GLAPIENTRY TAG(FogCoordfvEXT)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR1FV( VBO_ATTRIB_FOG, v );
}

static void GLAPIENTRY TAG(Color3f)( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3F( VBO_ATTRIB_COLOR0, x, y, z );
}

static void GLAPIENTRY TAG(Color3fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3FV( VBO_ATTRIB_COLOR0, v );
}

static void GLAPIENTRY TAG(Color4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR4F( VBO_ATTRIB_COLOR0, x, y, z, w );
}

static void GLAPIENTRY TAG(Color4fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR4FV( VBO_ATTRIB_COLOR0, v );
}

static void GLAPIENTRY TAG(SecondaryColor3fEXT)( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3F( VBO_ATTRIB_COLOR1, x, y, z );
}

static void GLAPIENTRY TAG(SecondaryColor3fvEXT)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR3FV( VBO_ATTRIB_COLOR1, v );
}


static void GLAPIENTRY TAG(EdgeFlag)( GLboolean b )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR1F( VBO_ATTRIB_EDGEFLAG, (GLfloat)b );
}

static void GLAPIENTRY TAG(Indexf)( GLfloat f )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR1F( VBO_ATTRIB_INDEX, f );
}

static void GLAPIENTRY TAG(Indexfv)( const GLfloat *f )
{
   GET_CURRENT_CONTEXT( ctx );
   ATTR1FV( VBO_ATTRIB_INDEX, f );
}


static void GLAPIENTRY TAG(MultiTexCoord1f)( GLenum target, GLfloat x  )
{
   GET_CURRENT_CONTEXT( ctx );
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR1F( attr, x );
}

static void GLAPIENTRY TAG(MultiTexCoord1fv)( GLenum target, const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR1FV( attr, v );
}

static void GLAPIENTRY TAG(MultiTexCoord2f)( GLenum target, GLfloat x, GLfloat y )
{
   GET_CURRENT_CONTEXT( ctx );
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR2F( attr, x, y );
}

static void GLAPIENTRY TAG(MultiTexCoord2fv)( GLenum target, const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR2FV( attr, v );
}

static void GLAPIENTRY TAG(MultiTexCoord3f)( GLenum target, GLfloat x, GLfloat y,
				    GLfloat z)
{
   GET_CURRENT_CONTEXT( ctx );
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR3F( attr, x, y, z );
}

static void GLAPIENTRY TAG(MultiTexCoord3fv)( GLenum target, const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR3FV( attr, v );
}

static void GLAPIENTRY TAG(MultiTexCoord4f)( GLenum target, GLfloat x, GLfloat y,
				    GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT( ctx );
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR4F( attr, x, y, z, w );
}

static void GLAPIENTRY TAG(MultiTexCoord4fv)( GLenum target, const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR4FV( attr, v );
}


static void GLAPIENTRY TAG(VertexAttrib1fARB)( GLuint index, GLfloat x )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index == 0) 
      ATTR1F(0, x);
   else if (index < MAX_VERTEX_ATTRIBS)
      ATTR1F(VBO_ATTRIB_GENERIC0 + index, x);
   else
      ERROR();
}

static void GLAPIENTRY TAG(VertexAttrib1fvARB)( GLuint index, 
					       const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index == 0) 
      ATTR1FV(0, v);
   else if (index < MAX_VERTEX_ATTRIBS)
      ATTR1FV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR();
}

static void GLAPIENTRY TAG(VertexAttrib2fARB)( GLuint index, GLfloat x, 
					      GLfloat y )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index == 0) 
      ATTR2F(0, x, y);
   else if (index < MAX_VERTEX_ATTRIBS)
      ATTR2F(VBO_ATTRIB_GENERIC0 + index, x, y);
   else
      ERROR();
}

static void GLAPIENTRY TAG(VertexAttrib2fvARB)( GLuint index,
					       const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index == 0) 
      ATTR2FV(0, v);
   else if (index < MAX_VERTEX_ATTRIBS)
      ATTR2FV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR();
}

static void GLAPIENTRY TAG(VertexAttrib3fARB)( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index == 0) 
      ATTR3F(0, x, y, z);
   else if (index < MAX_VERTEX_ATTRIBS)
      ATTR3F(VBO_ATTRIB_GENERIC0 + index, x, y, z);
   else
      ERROR();
}

static void GLAPIENTRY TAG(VertexAttrib3fvARB)( GLuint index,
					       const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index == 0) 
      ATTR3FV(0, v);
   else if (index < MAX_VERTEX_ATTRIBS)
      ATTR3FV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR();
}

static void GLAPIENTRY TAG(VertexAttrib4fARB)( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z,
					      GLfloat w )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index == 0) 
      ATTR4F(0, x, y, z, w);
   else if (index < MAX_VERTEX_ATTRIBS)
      ATTR4F(VBO_ATTRIB_GENERIC0 + index, x, y, z, w);
   else
      ERROR();
}

static void GLAPIENTRY TAG(VertexAttrib4fvARB)( GLuint index, 
					       const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index == 0) 
      ATTR4FV(0, v);
   else if (index < MAX_VERTEX_ATTRIBS)
      ATTR4FV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR();
}


/* In addition to supporting NV_vertex_program, these entrypoints are
 * used by the display list and other code specifically because of
 * their property of aliasing with other attributes.  (See
 * vbo_save_loopback.c)
 */
static void GLAPIENTRY TAG(VertexAttrib1fNV)( GLuint index, GLfloat x )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index < VBO_ATTRIB_MAX)
      ATTR1F(index, x);
}

static void GLAPIENTRY TAG(VertexAttrib1fvNV)( GLuint index, 
					       const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index < VBO_ATTRIB_MAX) 
      ATTR1FV(index, v);
}

static void GLAPIENTRY TAG(VertexAttrib2fNV)( GLuint index, GLfloat x, 
					      GLfloat y )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index < VBO_ATTRIB_MAX) 
      ATTR2F(index, x, y);
}

static void GLAPIENTRY TAG(VertexAttrib2fvNV)( GLuint index,
					       const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index < VBO_ATTRIB_MAX) 
      ATTR2FV(index, v);
}

static void GLAPIENTRY TAG(VertexAttrib3fNV)( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index < VBO_ATTRIB_MAX) 
      ATTR3F(index, x, y, z);
}

static void GLAPIENTRY TAG(VertexAttrib3fvNV)( GLuint index,
					       const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index < VBO_ATTRIB_MAX) 
      ATTR3FV(index, v);
}

static void GLAPIENTRY TAG(VertexAttrib4fNV)( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z,
					      GLfloat w )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index < VBO_ATTRIB_MAX) 
      ATTR4F(index, x, y, z, w);
}

static void GLAPIENTRY TAG(VertexAttrib4fvNV)( GLuint index, 
					       const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   if (index < VBO_ATTRIB_MAX) 
      ATTR4FV(index, v);
}


#define MAT( ATTR, N, face, params )			\
do {							\
   if (face != GL_BACK)					\
      MAT_ATTR( ATTR, N, params ); /* front */		\
   if (face != GL_FRONT)				\
      MAT_ATTR( ATTR + 1, N, params ); /* back */	\
} while (0)


/* Colormaterial conflicts are dealt with later.
 */
static void GLAPIENTRY TAG(Materialfv)( GLenum face, GLenum pname, 
			       const GLfloat *params )
{
   GET_CURRENT_CONTEXT( ctx );
   switch (pname) {
   case GL_EMISSION:
      MAT( VBO_ATTRIB_MAT_FRONT_EMISSION, 4, face, params );
      break;
   case GL_AMBIENT:
      MAT( VBO_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params );
      break;
   case GL_DIFFUSE:
      MAT( VBO_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params );
      break;
   case GL_SPECULAR:
      MAT( VBO_ATTRIB_MAT_FRONT_SPECULAR, 4, face, params );
      break;
   case GL_SHININESS:
      MAT( VBO_ATTRIB_MAT_FRONT_SHININESS, 1, face, params );
      break;
   case GL_COLOR_INDEXES:
      MAT( VBO_ATTRIB_MAT_FRONT_INDEXES, 3, face, params );
      break;
   case GL_AMBIENT_AND_DIFFUSE:
      MAT( VBO_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params );
      MAT( VBO_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params );
      break;
   default:
      ERROR();
      return;
   }
}


#undef ATTR1FV
#undef ATTR2FV
#undef ATTR3FV
#undef ATTR4FV

#undef ATTR1F
#undef ATTR2F
#undef ATTR3F
#undef ATTR4F

#undef MAT
#undef MAT_ATTR
