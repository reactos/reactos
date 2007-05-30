
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 */

/* Template for immediate mode color functions.
 *
 * FIXME: Floating-point color versions of these...
 */


static void TAG(Color3f)( GLfloat r, GLfloat g, GLfloat b )
{
   GET_CURRENT;
#ifdef COLOR_IS_FLOAT
   CURRENT_COLOR( RCOMP ) = CLAMP(r, 0.0f, 1.0f);
   CURRENT_COLOR( GCOMP ) = CLAMP(g, 0.0f, 1.0f);
   CURRENT_COLOR( BCOMP ) = CLAMP(b, 0.0f, 1.0f);
   CURRENT_COLOR( ACOMP ) = 1.0f;
#else
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( RCOMP ), r );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( GCOMP ), g );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( BCOMP ), b );
   CURRENT_COLOR( ACOMP ) = 255;
#endif
}

static void TAG(Color3fv)( const GLfloat *v )
{
   GET_CURRENT;
#ifdef COLOR_IS_FLOAT
   CURRENT_COLOR( RCOMP ) = CLAMP(v[0], 0.0f, 1.0f);
   CURRENT_COLOR( GCOMP ) = CLAMP(v[1], 0.0f, 1.0f);
   CURRENT_COLOR( BCOMP ) = CLAMP(v[2], 0.0f, 1.0f);
   CURRENT_COLOR( ACOMP ) = 1.0f;
#else
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( RCOMP ), v[0] );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( GCOMP ), v[1] );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( BCOMP ), v[2] );
   CURRENT_COLOR( ACOMP ) = 255;
#endif
}

static void TAG(Color3ub)( GLubyte r, GLubyte g, GLubyte b )
{
   GET_CURRENT;
#ifdef COLOR_IS_FLOAT
   CURRENT_COLOR( RCOMP ) = UBYTE_TO_FLOAT( r );
   CURRENT_COLOR( GCOMP ) = UBYTE_TO_FLOAT( g );
   CURRENT_COLOR( BCOMP ) = UBYTE_TO_FLOAT( b );
   CURRENT_COLOR( ACOMP ) = 1.0f;
#else
   CURRENT_COLOR( RCOMP ) = r;
   CURRENT_COLOR( GCOMP ) = g;
   CURRENT_COLOR( BCOMP ) = b;
   CURRENT_COLOR( ACOMP ) = 255;
#endif
}

static void TAG(Color3ubv)( const GLubyte *v )
{
   GET_CURRENT;
#ifdef COLOR_IS_FLOAT
   CURRENT_COLOR( RCOMP ) = UBYTE_TO_FLOAT( v[0] );
   CURRENT_COLOR( GCOMP ) = UBYTE_TO_FLOAT( v[1] );
   CURRENT_COLOR( BCOMP ) = UBYTE_TO_FLOAT( v[2] );
   CURRENT_COLOR( ACOMP ) = 1.0f;
#else
   CURRENT_COLOR( RCOMP ) = v[0];
   CURRENT_COLOR( GCOMP ) = v[1];
   CURRENT_COLOR( BCOMP ) = v[2];
   CURRENT_COLOR( ACOMP ) = 255;
#endif
}

static void TAG(Color4f)( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   GET_CURRENT;
#ifdef COLOR_IS_FLOAT
   CURRENT_COLOR( RCOMP ) = CLAMP(r, 0.0f, 1.0f);
   CURRENT_COLOR( GCOMP ) = CLAMP(g, 0.0f, 1.0f);
   CURRENT_COLOR( BCOMP ) = CLAMP(b, 0.0f, 1.0f);
   CURRENT_COLOR( ACOMP ) = CLAMP(a, 0.0f, 1.0f);
#else
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( RCOMP ), r );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( GCOMP ), g );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( BCOMP ), b );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( ACOMP ), a );
#endif
}

static void TAG(Color4fv)( const GLfloat *v )
{
   GET_CURRENT;
#ifdef COLOR_IS_FLOAT
   CURRENT_COLOR( RCOMP ) = CLAMP(v[0], 0.0f, 1.0f);
   CURRENT_COLOR( GCOMP ) = CLAMP(v[1], 0.0f, 1.0f);
   CURRENT_COLOR( BCOMP ) = CLAMP(v[2], 0.0f, 1.0f);
   CURRENT_COLOR( ACOMP ) = CLAMP(v[3], 0.0f, 1.0f);
#else
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( RCOMP ), v[0] );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( GCOMP ), v[1] );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( BCOMP ), v[2] );
   UNCLAMPED_FLOAT_TO_UBYTE( CURRENT_COLOR( ACOMP ), v[3] );
#endif
}

static void TAG(Color4ub)( GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   GET_CURRENT;
#ifdef COLOR_IS_FLOAT
   CURRENT_COLOR( RCOMP ) = UBYTE_TO_FLOAT( r );
   CURRENT_COLOR( GCOMP ) = UBYTE_TO_FLOAT( g );
   CURRENT_COLOR( BCOMP ) = UBYTE_TO_FLOAT( b );
   CURRENT_COLOR( ACOMP ) = UBYTE_TO_FLOAT( a );
#else
   CURRENT_COLOR( RCOMP ) = r;
   CURRENT_COLOR( GCOMP ) = g;
   CURRENT_COLOR( BCOMP ) = b;
   CURRENT_COLOR( ACOMP ) = a;
#endif
}

static void TAG(Color4ubv)( const GLubyte *v )
{
   GET_CURRENT;
#ifdef COLOR_IS_FLOAT
   CURRENT_COLOR( RCOMP ) = UBYTE_TO_FLOAT( v[0] );
   CURRENT_COLOR( GCOMP ) = UBYTE_TO_FLOAT( v[1] );
   CURRENT_COLOR( BCOMP ) = UBYTE_TO_FLOAT( v[2] );
   CURRENT_COLOR( ACOMP ) = UBYTE_TO_FLOAT( v[3] );
#else
   CURRENT_COLOR( RCOMP ) = v[0];
   CURRENT_COLOR( GCOMP ) = v[1];
   CURRENT_COLOR( BCOMP ) = v[2];
   CURRENT_COLOR( ACOMP ) = v[3];
#endif
}


static void TAG(ColorMaterial3f)( GLfloat r, GLfloat g, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Color;

   color[0] = r;
   color[1] = g;
   color[2] = b;
   color[3] = 1.0;

   _mesa_update_color_material( ctx, color );
   RECALC_BASE_COLOR( ctx );
}

static void TAG(ColorMaterial3fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Color;

   color[0] = v[0];
   color[1] = v[1];
   color[2] = v[2];
   color[3] = 1.0;

   _mesa_update_color_material( ctx, color );
   RECALC_BASE_COLOR( ctx );
}

static void TAG(ColorMaterial3ub)( GLubyte r, GLubyte g, GLubyte b )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Color;

   color[0] = UBYTE_TO_FLOAT( r );
   color[1] = UBYTE_TO_FLOAT( g );
   color[2] = UBYTE_TO_FLOAT( b );
   color[3] = 1.0;

   _mesa_update_color_material( ctx, color );
   RECALC_BASE_COLOR( ctx );
}

static void TAG(ColorMaterial3ubv)( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Color;

   color[0] = UBYTE_TO_FLOAT( v[0] );
   color[1] = UBYTE_TO_FLOAT( v[1] );
   color[2] = UBYTE_TO_FLOAT( v[2] );
   color[3] = 1.0;

   _mesa_update_color_material( ctx, color );
   RECALC_BASE_COLOR( ctx );
}

static void TAG(ColorMaterial4f)( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Color;

   color[0] = r;
   color[1] = g;
   color[2] = b;
   color[3] = a;

   _mesa_update_color_material( ctx, color );
   RECALC_BASE_COLOR( ctx );
}

static void TAG(ColorMaterial4fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Color;

   color[0] = v[0];
   color[1] = v[1];
   color[2] = v[2];
   color[3] = v[3];

   _mesa_update_color_material( ctx, color );
   RECALC_BASE_COLOR( ctx );
}

static void TAG(ColorMaterial4ub)( GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Color;

   color[0] = UBYTE_TO_FLOAT( r );
   color[1] = UBYTE_TO_FLOAT( g );
   color[2] = UBYTE_TO_FLOAT( b );
   color[3] = UBYTE_TO_FLOAT( a );

   _mesa_update_color_material( ctx, color );
   RECALC_BASE_COLOR( ctx );
}

static void TAG(ColorMaterial4ubv)( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Color;

   color[0] = UBYTE_TO_FLOAT( v[0] );
   color[1] = UBYTE_TO_FLOAT( v[1] );
   color[2] = UBYTE_TO_FLOAT( v[2] );
   color[3] = UBYTE_TO_FLOAT( v[3] );

   _mesa_update_color_material( ctx, color );
   RECALC_BASE_COLOR( ctx );
}





/* =============================================================
 * Color chooser functions:
 */

static void TAG(choose_Color3f)( GLfloat r, GLfloat g, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);

   if ( ctx->Light.Enabled ) {
      if ( ctx->Light.ColorMaterialEnabled ) {
	 SET_Color3f(ctx->Exec, TAG(ColorMaterial3f));
      } else {
	 SET_Color3f(ctx->Exec, _mesa_noop_Color3f);
      }
   } else {
      SET_Color3f(ctx->Exec, TAG(Color3f));
   }
   glColor3f( r, g, b );
}

static void TAG(choose_Color3fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);

   if ( ctx->Light.Enabled ) {
      if ( ctx->Light.ColorMaterialEnabled ) {
	 SET_Color3fv(ctx->Exec, TAG(ColorMaterial3fv));
      } else {
	 SET_Color3fv(ctx->Exec, _mesa_noop_Color3fv);
      }
   } else {
      SET_Color3fv(ctx->Exec, TAG(Color3fv));
   }
   glColor3fv( v );
}

static void TAG(choose_Color3ub)( GLubyte r, GLubyte g, GLubyte b )
{
   GET_CURRENT_CONTEXT(ctx);

   if ( ctx->Light.Enabled ) {
      if ( ctx->Light.ColorMaterialEnabled ) {
	 SET_Color3ub(ctx->Exec, TAG(ColorMaterial3ub));
      } else {
	 SET_Color3ub(ctx->Exec, _mesa_noop_Color3ub);
      }
   } else {
      SET_Color3ub(ctx->Exec, TAG(Color3ub));
   }
   glColor3ub( r, g, b );
}

static void TAG(choose_Color3ubv)( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);

   if ( ctx->Light.Enabled ) {
      if ( ctx->Light.ColorMaterialEnabled ) {
	 SET_Color3ubv(ctx->Exec, TAG(ColorMaterial3ubv));
      } else {
	 SET_Color3ubv(ctx->Exec, _mesa_noop_Color3ubv);
      }
   } else {
      SET_Color3ubv(ctx->Exec, TAG(Color3ubv));
   }
   glColor3ubv( v );
}

static void TAG(choose_Color4f)( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);

   if ( ctx->Light.Enabled ) {
      if ( ctx->Light.ColorMaterialEnabled ) {
	 SET_Color4f(ctx->Exec, TAG(ColorMaterial4f));
      } else {
	 SET_Color4f(ctx->Exec, _mesa_noop_Color4f);
      }
   } else {
      SET_Color4f(ctx->Exec, TAG(Color4f));
   }
   glColor4f( r, g, b, a );
}

static void TAG(choose_Color4fv)( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);

   if ( ctx->Light.Enabled ) {
      if ( ctx->Light.ColorMaterialEnabled ) {
	 SET_Color4fv(ctx->Exec, TAG(ColorMaterial4fv));
      } else {
	 SET_Color4fv(ctx->Exec, _mesa_noop_Color4fv);
      }
   } else {
      SET_Color4fv(ctx->Exec, TAG(Color4fv));
   }
   glColor4fv( v );
}

static void TAG(choose_Color4ub)( GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   GET_CURRENT_CONTEXT(ctx);

   if ( ctx->Light.Enabled ) {
      if ( ctx->Light.ColorMaterialEnabled ) {
	 SET_Color4ub(ctx->Exec, TAG(ColorMaterial4ub));
      } else {
	 SET_Color4ub(ctx->Exec, _mesa_noop_Color4ub);
      }
   } else {
      SET_Color4ub(ctx->Exec, TAG(Color4ub));
   }
   glColor4ub( r, g, b, a );
}

static void TAG(choose_Color4ubv)( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);

   if ( ctx->Light.Enabled ) {
      if ( ctx->Light.ColorMaterialEnabled ) {
	 SET_Color4ubv(ctx->Exec, TAG(ColorMaterial4ubv));
      } else {
	 SET_Color4ubv(ctx->Exec, _mesa_noop_Color4ubv);
      }
   } else {
      SET_Color4ubv(ctx->Exec, TAG(Color4ubv));
   }
   glColor4ubv( v );
}



#undef GET_CURRENT
#undef CURRENT_COLOR
#undef CURRENT_SPECULAR
#undef COLOR_IS_FLOAT
#undef RECALC_BASE_COLOR
#undef TAG
