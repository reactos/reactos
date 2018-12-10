/* $Id: texstate.c,v 1.10 1998/02/03 23:45:02 brianp Exp $ */

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
 * $Log: texstate.c,v $
 * Revision 1.10  1998/02/03 23:45:02  brianp
 * added casts to prevent warnings with Amiga StormC compiler
 *
 * Revision 1.9  1997/12/31 06:10:03  brianp
 * added Henk Kok's texture validation optimization (AnyDirty flag)
 *
 * Revision 1.8  1997/10/13 23:56:01  brianp
 * replaced UpdateTexture() call with TexParameter()
 *
 * Revision 1.7  1997/09/29 23:28:14  brianp
 * updated for new device driver texture functions
 *
 * Revision 1.6  1997/09/27 00:14:39  brianp
 * added GL_EXT_paletted_texture extension
 *
 * Revision 1.5  1997/07/24 01:25:34  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.4  1997/05/28 03:26:49  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.3  1997/05/03 00:53:28  brianp
 * misc changes related to new texture object sampling function pointer
 *
 * Revision 1.2  1997/04/28 23:34:39  brianp
 * simplified gl_update_texture_state()
 *
 * Revision 1.1  1997/04/14 01:59:54  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "context.h"
#include "macros.h"
#include "matrix.h"
#include "texobj.h"
#include "texstate.h"
#include "texture.h"
#include "types.h"
#include "xform.h"
#endif


#ifdef SPECIALCAST
/* Needed for an Amiga compiler */
#define ENUM_TO_FLOAT(X) ((GLfloat)(GLint)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(GLint)(X))
#else
/* all other compilers */
#define ENUM_TO_FLOAT(X) ((GLfloat)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(X))
#endif



/**********************************************************************/
/*                       Texture Environment                          */
/**********************************************************************/


void gl_TexEnvfv( GLcontext *ctx,
                  GLenum target, GLenum pname, const GLfloat *param )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexEnv" );
      return;
   }

   if (target!=GL_TEXTURE_ENV) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexEnv(target)" );
      return;
   }

   if (pname==GL_TEXTURE_ENV_MODE) {
      GLenum mode = (GLenum) (GLint) *param;
      switch (mode) {
	 case GL_MODULATE:
	 case GL_BLEND:
	 case GL_DECAL:
	 case GL_REPLACE:
	    ctx->Texture.EnvMode = mode;
	    break;
	 default:
	    gl_error( ctx, GL_INVALID_ENUM, "glTexEnv(param)" );
	    return;
      }
   }
   else if (pname==GL_TEXTURE_ENV_COLOR) {
      ctx->Texture.EnvColor[0] = CLAMP( param[0], 0.0, 1.0 );
      ctx->Texture.EnvColor[1] = CLAMP( param[1], 0.0, 1.0 );
      ctx->Texture.EnvColor[2] = CLAMP( param[2], 0.0, 1.0 );
      ctx->Texture.EnvColor[3] = CLAMP( param[3], 0.0, 1.0 );
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname)" );
      return;
   }

   /* Tell device driver about the new texture environment */
   if (ctx->Driver.TexEnv) {
      (*ctx->Driver.TexEnv)( ctx, pname, param );
   }
}





void gl_GetTexEnvfv( GLcontext *ctx,
                     GLenum target, GLenum pname, GLfloat *params )
{
   if (target!=GL_TEXTURE_ENV) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
      return;
   }
   switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         *params = ENUM_TO_FLOAT(ctx->Texture.EnvMode);
	 break;
      case GL_TEXTURE_ENV_COLOR:
	 COPY_4V( params, ctx->Texture.EnvColor );
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
   }
}


void gl_GetTexEnviv( GLcontext *ctx,
                     GLenum target, GLenum pname, GLint *params )
{
   if (target!=GL_TEXTURE_ENV) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
      return;
   }
   switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         *params = (GLint) ctx->Texture.EnvMode;
	 break;
      case GL_TEXTURE_ENV_COLOR:
	 params[0] = FLOAT_TO_INT( ctx->Texture.EnvColor[0] );
	 params[1] = FLOAT_TO_INT( ctx->Texture.EnvColor[1] );
	 params[2] = FLOAT_TO_INT( ctx->Texture.EnvColor[2] );
	 params[3] = FLOAT_TO_INT( ctx->Texture.EnvColor[3] );
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
   }
}




/**********************************************************************/
/*                       Texture Parameters                           */
/**********************************************************************/


void gl_TexParameterfv( GLcontext *ctx,
                        GLenum target, GLenum pname, const GLfloat *params )
{
   GLenum eparam = (GLenum) (GLint) params[0];
   struct gl_texture_object *texObj;

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = ctx->Texture.Current1D;
         break;
      case GL_TEXTURE_2D:
         texObj = ctx->Texture.Current2D;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
         return;
   }

   switch (pname) {
      case GL_TEXTURE_MIN_FILTER:
         if (eparam==GL_NEAREST || eparam==GL_LINEAR
             || eparam==GL_NEAREST_MIPMAP_NEAREST
             || eparam==GL_LINEAR_MIPMAP_NEAREST
             || eparam==GL_NEAREST_MIPMAP_LINEAR
             || eparam==GL_LINEAR_MIPMAP_LINEAR) {
            texObj->MinFilter = eparam;
            ctx->NewState |= NEW_TEXTURING;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_MAG_FILTER:
         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            texObj->MagFilter = eparam;
            ctx->NewState |= NEW_TEXTURING;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_S:
         if (eparam==GL_CLAMP || eparam==GL_REPEAT) {
            texObj->WrapS = eparam;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_T:
         if (eparam==GL_CLAMP || eparam==GL_REPEAT) {
            texObj->WrapT = eparam;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_R_EXT:
         if (eparam==GL_CLAMP || eparam==GL_REPEAT) {
            texObj->WrapR = eparam;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
         }
         break;
      case GL_TEXTURE_BORDER_COLOR:
         texObj->BorderColor[0] = CLAMP((GLint)(params[0]*255.0), 0, 255);
         texObj->BorderColor[1] = CLAMP((GLint)(params[1]*255.0), 0, 255);
         texObj->BorderColor[2] = CLAMP((GLint)(params[2]*255.0), 0, 255);
         texObj->BorderColor[3] = CLAMP((GLint)(params[3]*255.0), 0, 255);
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexParameter(pname)" );
         return;
   }

   texObj->Dirty = GL_TRUE;
   ctx->Texture.AnyDirty = GL_TRUE;
   
   if (ctx->Driver.TexParameter) {
      (*ctx->Driver.TexParameter)( ctx, target, texObj, pname, params );
   }
}



void gl_GetTexLevelParameterfv( GLcontext *ctx, GLenum target, GLint level,
                                GLenum pname, GLfloat *params )
{
   GLint iparam;

   gl_GetTexLevelParameteriv( ctx, target, level, pname, &iparam );
   *params = (GLfloat) iparam;
}



void gl_GetTexLevelParameteriv( GLcontext *ctx, GLenum target, GLint level,
                                GLenum pname, GLint *params )
{
   struct gl_texture_image *tex;

   if (level<0 || level>=MAX_TEXTURE_LEVELS) {
      gl_error( ctx, GL_INVALID_VALUE, "glGetTexLevelParameter[if]v" );
      return;
   }

   switch (target) {
      case GL_TEXTURE_1D:
         tex = ctx->Texture.Current1D->Image[level];
         switch (pname) {
	    case GL_TEXTURE_WIDTH:
	       *params = tex->Width;
	       break;
	    case GL_TEXTURE_COMPONENTS:
	       *params = tex->Format;
	       break;
	    case GL_TEXTURE_BORDER:
	       *params = tex->Border;
	       break;
            case GL_TEXTURE_RED_SIZE:
            case GL_TEXTURE_GREEN_SIZE:
            case GL_TEXTURE_BLUE_SIZE:
            case GL_TEXTURE_ALPHA_SIZE:
            case GL_TEXTURE_INTENSITY_SIZE:
            case GL_TEXTURE_LUMINANCE_SIZE:
               *params = 8;  /* 8-bits */
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
	 }
	 break;
      case GL_TEXTURE_2D:
         tex = ctx->Texture.Current2D->Image[level];
	 switch (pname) {
	    case GL_TEXTURE_WIDTH:
	       *params = tex->Width;
	       break;
	    case GL_TEXTURE_HEIGHT:
	       *params = tex->Height;
	       break;
	    case GL_TEXTURE_COMPONENTS:
	       *params = tex->Format;
	       break;
	    case GL_TEXTURE_BORDER:
	       *params = tex->Border;
	       break;
            case GL_TEXTURE_RED_SIZE:
            case GL_TEXTURE_GREEN_SIZE:
            case GL_TEXTURE_BLUE_SIZE:
            case GL_TEXTURE_ALPHA_SIZE:
            case GL_TEXTURE_INTENSITY_SIZE:
            case GL_TEXTURE_LUMINANCE_SIZE:
               *params = 8;  /* 8-bits */
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
	 }
	 break;
      case GL_PROXY_TEXTURE_1D:
         tex = ctx->Texture.Proxy1D->Image[level];
         switch (pname) {
	    case GL_TEXTURE_WIDTH:
	       *params = tex->Width;
	       break;
	    case GL_TEXTURE_COMPONENTS:
	       *params = tex->Format;
	       break;
	    case GL_TEXTURE_BORDER:
	       *params = tex->Border;
	       break;
            case GL_TEXTURE_RED_SIZE:
            case GL_TEXTURE_GREEN_SIZE:
            case GL_TEXTURE_BLUE_SIZE:
            case GL_TEXTURE_ALPHA_SIZE:
            case GL_TEXTURE_INTENSITY_SIZE:
            case GL_TEXTURE_LUMINANCE_SIZE:
               *params = 8;  /* 8-bits */
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
	 }
	 break;
      case GL_PROXY_TEXTURE_2D:
         tex = ctx->Texture.Proxy2D->Image[level];
	 switch (pname) {
	    case GL_TEXTURE_WIDTH:
	       *params = tex->Width;
	       break;
	    case GL_TEXTURE_HEIGHT:
	       *params = tex->Height;
	       break;
	    case GL_TEXTURE_COMPONENTS:
	       *params = tex->Format;
	       break;
	    case GL_TEXTURE_BORDER:
	       *params = tex->Border;
	       break;
            case GL_TEXTURE_RED_SIZE:
            case GL_TEXTURE_GREEN_SIZE:
            case GL_TEXTURE_BLUE_SIZE:
            case GL_TEXTURE_ALPHA_SIZE:
            case GL_TEXTURE_INTENSITY_SIZE:
            case GL_TEXTURE_LUMINANCE_SIZE:
               *params = 8;  /* 8-bits */
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
	 }
	 break;
     default:
	 gl_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(target)");
   }	 
}




void gl_GetTexParameterfv( GLcontext *ctx,
                           GLenum target, GLenum pname, GLfloat *params )
{
   switch (target) {
      case GL_TEXTURE_1D:
         switch (pname) {
	    case GL_TEXTURE_MAG_FILTER:
	       *params = ENUM_TO_FLOAT(ctx->Texture.Current1D->MagFilter);
	       break;
	    case GL_TEXTURE_MIN_FILTER:
	       *params = ENUM_TO_FLOAT(ctx->Texture.Current1D->MinFilter);
	       break;
	    case GL_TEXTURE_WRAP_S:
	       *params = ENUM_TO_FLOAT(ctx->Texture.Current1D->WrapS);
	       break;
	    case GL_TEXTURE_WRAP_T:
	       *params = ENUM_TO_FLOAT(ctx->Texture.Current1D->WrapT);
	       break;
	    case GL_TEXTURE_BORDER_COLOR:
               params[0] = ctx->Texture.Current1D->BorderColor[0] / 255.0f;
               params[1] = ctx->Texture.Current1D->BorderColor[1] / 255.0f;
               params[2] = ctx->Texture.Current1D->BorderColor[2] / 255.0f;
               params[3] = ctx->Texture.Current1D->BorderColor[3] / 255.0f;
	       break;
	    case GL_TEXTURE_RESIDENT:
               *params = ENUM_TO_FLOAT(GL_TRUE);
	       break;
            case GL_TEXTURE_PRIORITY:
               *params = ctx->Texture.Current1D->Priority;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname)" );
	 }
         break;
      case GL_TEXTURE_2D:
         switch (pname) {
	    case GL_TEXTURE_MAG_FILTER:
	       *params = ENUM_TO_FLOAT(ctx->Texture.Current2D->MagFilter);
	       break;
	    case GL_TEXTURE_MIN_FILTER:
	       *params = ENUM_TO_FLOAT(ctx->Texture.Current2D->MinFilter);
	       break;
	    case GL_TEXTURE_WRAP_S:
	       *params = ENUM_TO_FLOAT(ctx->Texture.Current2D->WrapS);
	       break;
	    case GL_TEXTURE_WRAP_T:
	       *params = ENUM_TO_FLOAT(ctx->Texture.Current2D->WrapT);
	       break;
	    case GL_TEXTURE_BORDER_COLOR:
               params[0] = ctx->Texture.Current2D->BorderColor[0] / 255.0f;
               params[1] = ctx->Texture.Current2D->BorderColor[1] / 255.0f;
               params[2] = ctx->Texture.Current2D->BorderColor[2] / 255.0f;
               params[3] = ctx->Texture.Current2D->BorderColor[3] / 255.0f;
               break;
	    case GL_TEXTURE_RESIDENT:
               *params = ENUM_TO_FLOAT(GL_TRUE);
	       break;
	    case GL_TEXTURE_PRIORITY:
               *params = ctx->Texture.Current2D->Priority;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname)" );
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameterfv(target)" );
   }
}


void gl_GetTexParameteriv( GLcontext *ctx,
                           GLenum target, GLenum pname, GLint *params )
{
   switch (target) {
      case GL_TEXTURE_1D:
         switch (pname) {
	    case GL_TEXTURE_MAG_FILTER:
	       *params = (GLint) ctx->Texture.Current1D->MagFilter;
	       break;
	    case GL_TEXTURE_MIN_FILTER:
	       *params = (GLint) ctx->Texture.Current1D->MinFilter;
	       break;
	    case GL_TEXTURE_WRAP_S:
	       *params = (GLint) ctx->Texture.Current1D->WrapS;
	       break;
	    case GL_TEXTURE_WRAP_T:
	       *params = (GLint) ctx->Texture.Current1D->WrapT;
	       break;
	    case GL_TEXTURE_BORDER_COLOR:
               {
                  GLfloat color[4];
                  color[0] = ctx->Texture.Current1D->BorderColor[0]/255.0;
                  color[1] = ctx->Texture.Current1D->BorderColor[1]/255.0;
                  color[2] = ctx->Texture.Current1D->BorderColor[2]/255.0;
                  color[3] = ctx->Texture.Current1D->BorderColor[3]/255.0;
                  params[0] = FLOAT_TO_INT( color[0] );
                  params[1] = FLOAT_TO_INT( color[1] );
                  params[2] = FLOAT_TO_INT( color[2] );
                  params[3] = FLOAT_TO_INT( color[3] );
               }
	       break;
	    case GL_TEXTURE_RESIDENT:
               *params = (GLint) GL_TRUE;
	       break;
	    case GL_TEXTURE_PRIORITY:
               *params = (GLint) ctx->Texture.Current1D->Priority;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname)" );
	 }
         break;
      case GL_TEXTURE_2D:
         switch (pname) {
	    case GL_TEXTURE_MAG_FILTER:
	       *params = (GLint) ctx->Texture.Current2D->MagFilter;
	       break;
	    case GL_TEXTURE_MIN_FILTER:
	       *params = (GLint) ctx->Texture.Current2D->MinFilter;
	       break;
	    case GL_TEXTURE_WRAP_S:
	       *params = (GLint) ctx->Texture.Current2D->WrapS;
	       break;
	    case GL_TEXTURE_WRAP_T:
	       *params = (GLint) ctx->Texture.Current2D->WrapT;
	       break;
	    case GL_TEXTURE_BORDER_COLOR:
               {
                  GLfloat color[4];
                  color[0] = ctx->Texture.Current2D->BorderColor[0]/255.0;
                  color[1] = ctx->Texture.Current2D->BorderColor[1]/255.0;
                  color[2] = ctx->Texture.Current2D->BorderColor[2]/255.0;
                  color[3] = ctx->Texture.Current2D->BorderColor[3]/255.0;
                  params[0] = FLOAT_TO_INT( color[0] );
                  params[1] = FLOAT_TO_INT( color[1] );
                  params[2] = FLOAT_TO_INT( color[2] );
                  params[3] = FLOAT_TO_INT( color[3] );
               }
	       break;
	    case GL_TEXTURE_RESIDENT:
               *params = (GLint) GL_TRUE;
	       break;
	    case GL_TEXTURE_PRIORITY:
               *params = (GLint) ctx->Texture.Current2D->Priority;
	       break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname)" );
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameteriv(target)" );
   }
}




/**********************************************************************/
/*                    Texture Coord Generation                        */
/**********************************************************************/


void gl_TexGenfv( GLcontext *ctx,
                  GLenum coord, GLenum pname, const GLfloat *params )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexGenfv" );
      return;
   }

   switch( coord ) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    if (mode==GL_OBJECT_LINEAR ||
		mode==GL_EYE_LINEAR ||
		mode==GL_SPHERE_MAP) {
	       ctx->Texture.GenModeS = mode;
	    }
	    else {
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    ctx->Texture.ObjectPlaneS[0] = params[0];
	    ctx->Texture.ObjectPlaneS[1] = params[1];
	    ctx->Texture.ObjectPlaneS[2] = params[2];
	    ctx->Texture.ObjectPlaneS[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->NewModelViewMatrix) {
               gl_analyze_modelview_matrix(ctx);
            }
            gl_transform_vector( ctx->Texture.EyePlaneS, params,
                                 ctx->ModelViewInv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    if (mode==GL_OBJECT_LINEAR ||
		mode==GL_EYE_LINEAR ||
		mode==GL_SPHERE_MAP) {
	       ctx->Texture.GenModeT = mode;
	    }
	    else {
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    ctx->Texture.ObjectPlaneT[0] = params[0];
	    ctx->Texture.ObjectPlaneT[1] = params[1];
	    ctx->Texture.ObjectPlaneT[2] = params[2];
	    ctx->Texture.ObjectPlaneT[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->NewModelViewMatrix) {
               gl_analyze_modelview_matrix(ctx);
            }
            gl_transform_vector( ctx->Texture.EyePlaneT, params,
                                 ctx->ModelViewInv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    if (mode==GL_OBJECT_LINEAR ||
		mode==GL_EYE_LINEAR) {
	       ctx->Texture.GenModeR = mode;
	    }
	    else {
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    ctx->Texture.ObjectPlaneR[0] = params[0];
	    ctx->Texture.ObjectPlaneR[1] = params[1];
	    ctx->Texture.ObjectPlaneR[2] = params[2];
	    ctx->Texture.ObjectPlaneR[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->NewModelViewMatrix) {
               gl_analyze_modelview_matrix(ctx);
            }
            gl_transform_vector( ctx->Texture.EyePlaneR, params,
                                 ctx->ModelViewInv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    if (mode==GL_OBJECT_LINEAR ||
		mode==GL_EYE_LINEAR) {
	       ctx->Texture.GenModeQ = mode;
	    }
	    else {
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    ctx->Texture.ObjectPlaneQ[0] = params[0];
	    ctx->Texture.ObjectPlaneQ[1] = params[1];
	    ctx->Texture.ObjectPlaneQ[2] = params[2];
	    ctx->Texture.ObjectPlaneQ[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->NewModelViewMatrix) {
               gl_analyze_modelview_matrix(ctx);
            }
            gl_transform_vector( ctx->Texture.EyePlaneQ, params,
                                 ctx->ModelViewInv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(coord)" );
	 return;
   }

   ctx->NewState |= NEW_TEXTURING;
}



void gl_GetTexGendv( GLcontext *ctx,
                     GLenum coord, GLenum pname, GLdouble *params )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetTexGendv" );
      return;
   }

   switch( coord ) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(ctx->Texture.GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneS );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(ctx->Texture.GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneT );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(ctx->Texture.GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneR );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(ctx->Texture.GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneQ );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(coord)" );
	 return;
   }
}



void gl_GetTexGenfv( GLcontext *ctx,
                     GLenum coord, GLenum pname, GLfloat *params )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetTexGenfv" );
      return;
   }

   switch( coord ) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(ctx->Texture.GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneS );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(ctx->Texture.GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneT );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(ctx->Texture.GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneR );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(ctx->Texture.GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneQ );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(coord)" );
	 return;
   }
}



void gl_GetTexGeniv( GLcontext *ctx,
                     GLenum coord, GLenum pname, GLint *params )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetTexGeniv" );
      return;
   }

   switch( coord ) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ctx->Texture.GenModeS;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneS );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ctx->Texture.GenModeT;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneT );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ctx->Texture.GenModeR;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneR );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ctx->Texture.GenModeQ;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, ctx->Texture.ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, ctx->Texture.EyePlaneQ );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(coord)" );
	 return;
   }
}



/*
 * This is called by gl_update_state() if the NEW_TEXTURING bit in
 * ctx->NewState is set.
 */
void gl_update_texture_state( GLcontext *ctx )
{
   struct gl_texture_object *t;

   if (ctx->Texture.Enabled & TEXTURE_2D)
      ctx->Texture.Current = ctx->Texture.Current2D;
   else if (ctx->Texture.Enabled & TEXTURE_1D)
      ctx->Texture.Current = ctx->Texture.Current1D;
   else
      ctx->Texture.Current = NULL;

   if (ctx->Texture.AnyDirty) {
      for (t = ctx->Shared->TexObjectList; t; t = t->Next) {
         if (t->Dirty) {
            gl_test_texture_object_completeness(t);
            gl_set_texture_sampler(t);
            t->Dirty = GL_FALSE;
         }
      }
      ctx->Texture.AnyDirty = GL_FALSE;
   }
}
