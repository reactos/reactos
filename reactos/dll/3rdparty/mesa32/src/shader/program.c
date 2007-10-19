/*
 * Mesa 3-D graphics library
 * Version:  6.2
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/**
 * \file program.c
 * Vertex and fragment program support functions.
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "program.h"
#include "nvfragparse.h"
#include "nvfragprog.h"
#include "nvvertparse.h"
#include "nvvertprog.h"


/**********************************************************************/
/* Utility functions                                                  */
/**********************************************************************/


/* A pointer to this dummy program is put into the hash table when
 * glGenPrograms is called.
 */
struct program _mesa_DummyProgram;


/**
 * Init context's vertex/fragment program state
 */
void
_mesa_init_program(GLcontext *ctx)
{
   GLuint i;

   ctx->Program.ErrorPos = -1;
   ctx->Program.ErrorString = _mesa_strdup("");

#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program
   ctx->VertexProgram.Enabled = GL_FALSE;
   ctx->VertexProgram.PointSizeEnabled = GL_FALSE;
   ctx->VertexProgram.TwoSideEnabled = GL_FALSE;
   ctx->VertexProgram.Current = (struct vertex_program *) ctx->Shared->DefaultVertexProgram;
   assert(ctx->VertexProgram.Current);
   ctx->VertexProgram.Current->Base.RefCount++;
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS / 4; i++) {
      ctx->VertexProgram.TrackMatrix[i] = GL_NONE;
      ctx->VertexProgram.TrackMatrixTransform[i] = GL_IDENTITY_NV;
   }
#endif

#if FEATURE_NV_fragment_program || FEATURE_ARB_fragment_program
   ctx->FragmentProgram.Enabled = GL_FALSE;
   ctx->FragmentProgram.Current = (struct fragment_program *) ctx->Shared->DefaultFragmentProgram;
   assert(ctx->FragmentProgram.Current);
   ctx->FragmentProgram.Current->Base.RefCount++;
#endif

#if FEATURE_ATI_fragment_shader
   ctx->ATIFragmentShader.Enabled = GL_FALSE;
   ctx->ATIFragmentShader.Current = (struct ati_fragment_shader *) ctx->Shared->DefaultFragmentShader;
   assert(ctx->ATIFragmentShader.Current);
   ctx->ATIFragmentShader.Current->Base.RefCount++;
#endif
}


/**
 * Free a context's vertex/fragment program state
 */
void
_mesa_free_program_data(GLcontext *ctx)
{
#if FEATURE_NV_vertex_program
   if (ctx->VertexProgram.Current) {
      ctx->VertexProgram.Current->Base.RefCount--;
      if (ctx->VertexProgram.Current->Base.RefCount <= 0)
         ctx->Driver.DeleteProgram(ctx, &(ctx->VertexProgram.Current->Base));
   }
#endif
#if FEATURE_NV_fragment_program
   if (ctx->FragmentProgram.Current) {
      ctx->FragmentProgram.Current->Base.RefCount--;
      if (ctx->FragmentProgram.Current->Base.RefCount <= 0)
         ctx->Driver.DeleteProgram(ctx, &(ctx->FragmentProgram.Current->Base));
   }
#endif
#if FEATURE_ATI_fragment_shader
   if (ctx->ATIFragmentShader.Current) {
      ctx->ATIFragmentShader.Current->Base.RefCount--;
      if (ctx->ATIFragmentShader.Current->Base.RefCount <= 0)
	ctx->Driver.DeleteProgram(ctx, &(ctx->ATIFragmentShader.Current->Base));
   }
#endif
   _mesa_free((void *) ctx->Program.ErrorString);
}




/**
 * Set the vertex/fragment program error state (position and error string).
 * This is generally called from within the parsers.
 */
void
_mesa_set_program_error(GLcontext *ctx, GLint pos, const char *string)
{
   ctx->Program.ErrorPos = pos;
   _mesa_free((void *) ctx->Program.ErrorString);
   if (!string)
      string = "";
   ctx->Program.ErrorString = _mesa_strdup(string);
}


/**
 * Find the line number and column for 'pos' within 'string'.
 * Return a copy of the line which contains 'pos'.  Free the line with
 * _mesa_free().
 * \param string  the program string
 * \param pos     the position within the string
 * \param line    returns the line number corresponding to 'pos'.
 * \param col     returns the column number corresponding to 'pos'.
 * \return copy of the line containing 'pos'.
 */
const GLubyte *
_mesa_find_line_column(const GLubyte *string, const GLubyte *pos,
                       GLint *line, GLint *col)
{
   const GLubyte *lineStart = string;
   const GLubyte *p = string;
   GLubyte *s;
   int len;

   *line = 1;

   while (p != pos) {
      if (*p == (GLubyte) '\n') {
         (*line)++;
         lineStart = p + 1;
      }
      p++;
   }

   *col = (pos - lineStart) + 1;

   /* return copy of this line */
   while (*p != 0 && *p != '\n')
      p++;
   len = p - lineStart;
   s = (GLubyte *) _mesa_malloc(len + 1);
   _mesa_memcpy(s, lineStart, len);
   s[len] = 0;

   return s;
}


/**
 * Initialize a new vertex/fragment program object.
 */
static struct program *
_mesa_init_program_struct( GLcontext *ctx, struct program *prog,
                           GLenum target, GLuint id)
{
   (void) ctx;
   if (prog) {
      prog->Id = id;
      prog->Target = target;
      prog->Resident = GL_TRUE;
      prog->RefCount = 1;
   }

   return prog;
}


/**
 * Initialize a new fragment program object.
 */
struct program *
_mesa_init_fragment_program( GLcontext *ctx, struct fragment_program *prog,
                             GLenum target, GLuint id)
{
   if (prog)
      return _mesa_init_program_struct( ctx, &prog->Base, target, id );
   else
      return NULL;
}


/**
 * Initialize a new vertex program object.
 */
struct program *
_mesa_init_vertex_program( GLcontext *ctx, struct vertex_program *prog,
                           GLenum target, GLuint id)
{
   if (prog)
      return _mesa_init_program_struct( ctx, &prog->Base, target, id );
   else
      return NULL;
}

/**
 * Initialize a new ATI fragment shader object.
 */
struct program *
_mesa_init_ati_fragment_shader( GLcontext *ctx,
                                struct ati_fragment_shader *prog,
                                GLenum target, GLuint id )
{
   if (prog)
      return _mesa_init_program_struct( ctx, &prog->Base, target, id );
   else
      return NULL;
}



/**
 * Allocate and initialize a new fragment/vertex program object but
 * don't put it into the program hash table.  Called via
 * ctx->Driver.NewProgram.  May be overridden (ie. replaced) by a
 * device driver function to implement OO deriviation with additional
 * types not understood by this function.
 *
 * \param ctx  context
 * \param id   program id/number
 * \param target  program target/type
 * \return  pointer to new program object
 */
struct program *
_mesa_new_program(GLcontext *ctx, GLenum target, GLuint id)
{
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: /* == GL_VERTEX_PROGRAM_NV */
      return _mesa_init_vertex_program( ctx, CALLOC_STRUCT(vertex_program),
					target, id );
   case GL_FRAGMENT_PROGRAM_NV:
   case GL_FRAGMENT_PROGRAM_ARB:
      return _mesa_init_fragment_program( ctx, CALLOC_STRUCT(fragment_program),
					  target, id );
   case GL_FRAGMENT_SHADER_ATI:
      return _mesa_init_ati_fragment_shader( ctx, CALLOC_STRUCT(ati_fragment_shader),
					  target, id );

   default:
      _mesa_problem(ctx, "bad target in _mesa_new_program");
      return NULL;
   }
}


/**
 * Delete a program and remove it from the hash table, ignoring the
 * reference count.
 * Called via ctx->Driver.DeleteProgram.  May be wrapped (OO deriviation)
 * by a device driver function.
 */
void
_mesa_delete_program(GLcontext *ctx, struct program *prog)
{
   (void) ctx;
   ASSERT(prog);

   if (prog->String)
      _mesa_free(prog->String);
   if (prog->Target == GL_VERTEX_PROGRAM_NV ||
       prog->Target == GL_VERTEX_STATE_PROGRAM_NV) {
      struct vertex_program *vprog = (struct vertex_program *) prog;
      if (vprog->Instructions) {
         GLuint i;
         for (i = 0; i < vprog->Base.NumInstructions; i++) {
            if (vprog->Instructions[i].Data)
               _mesa_free(vprog->Instructions[i].Data);
         }
         _mesa_free(vprog->Instructions);
      }
      if (vprog->Parameters)
         _mesa_free_parameter_list(vprog->Parameters);
   }
   else if (prog->Target == GL_FRAGMENT_PROGRAM_NV ||
            prog->Target == GL_FRAGMENT_PROGRAM_ARB) {
      struct fragment_program *fprog = (struct fragment_program *) prog;
      if (fprog->Instructions) {
         GLuint i;
         for (i = 0; i < fprog->Base.NumInstructions; i++) {
            if (fprog->Instructions[i].Data)
               _mesa_free(fprog->Instructions[i].Data);
         }
         _mesa_free(fprog->Instructions);
      }
      if (fprog->Parameters)
         _mesa_free_parameter_list(fprog->Parameters);
   }
   else if (prog->Target == GL_FRAGMENT_SHADER_ATI) {
      struct ati_fragment_shader *atifs = (struct ati_fragment_shader *)prog;
      if (atifs->Instructions)
	 _mesa_free(atifs->Instructions);
   }

   _mesa_free(prog);
}



/**********************************************************************/
/* Program parameter functions                                        */
/**********************************************************************/

struct program_parameter_list *
_mesa_new_parameter_list(void)
{
   return (struct program_parameter_list *)
      _mesa_calloc(sizeof(struct program_parameter_list));
}


/**
 * Free a parameter list and all its parameters
 */
void
_mesa_free_parameter_list(struct program_parameter_list *paramList)
{
   _mesa_free_parameters(paramList);
   _mesa_free(paramList->Parameters);
   if (paramList->ParameterValues)
      ALIGN_FREE(paramList->ParameterValues);
   _mesa_free(paramList);
}


/**
 * Free all the parameters in the given list, but don't free the
 * paramList structure itself.
 */
void
_mesa_free_parameters(struct program_parameter_list *paramList)
{
   GLuint i;
   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Name)
	 _mesa_free((void *) paramList->Parameters[i].Name);
   }
   paramList->NumParameters = 0;
}


/**
 * Helper function used by the functions below.
 */
static GLint
add_parameter(struct program_parameter_list *paramList,
              const char *name, const GLfloat values[4],
              enum parameter_type type)
{
   const GLuint n = paramList->NumParameters;

   if (n == paramList->Size) {
      GLfloat (*tmp)[4];

      paramList->Size *= 2;
      if (!paramList->Size)
	 paramList->Size = 8;

      paramList->Parameters = (struct program_parameter *)
	 _mesa_realloc(paramList->Parameters,
		       n * sizeof(struct program_parameter),
		       paramList->Size * sizeof(struct program_parameter));

      tmp = paramList->ParameterValues;
      paramList->ParameterValues = ALIGN_MALLOC(paramList->Size * 4 * sizeof(GLfloat), 16);
      if (tmp) {
	 _mesa_memcpy(paramList->ParameterValues, tmp,
		      n * 4 * sizeof(GLfloat));
	 ALIGN_FREE(tmp);
      }
   }

   if (!paramList->Parameters ||
       !paramList->ParameterValues) {
      /* out of memory */
      paramList->NumParameters = 0;
      paramList->Size = 0;
      return -1;
   }
   else {
      paramList->NumParameters = n + 1;

      _mesa_memset(&paramList->Parameters[n], 0,
		   sizeof(struct program_parameter));

      paramList->Parameters[n].Name = name ? _mesa_strdup(name) : NULL;
      paramList->Parameters[n].Type = type;
      if (values)
         COPY_4V(paramList->ParameterValues[n], values);
      return (GLint) n;
   }
}


/**
 * Add a new named program parameter (Ex: NV_fragment_program DEFINE statement)
 * \return index of the new entry in the parameter list
 */
GLint
_mesa_add_named_parameter(struct program_parameter_list *paramList,
                          const char *name, const GLfloat values[4])
{
   return add_parameter(paramList, name, values, NAMED_PARAMETER);
}


/**
 * Add a new unnamed constant to the parameter list.
 * \param paramList - the parameter list
 * \param values - four float values
 * \return index of the new parameter.
 */
GLint
_mesa_add_named_constant(struct program_parameter_list *paramList,
                         const char *name, const GLfloat values[4])
{
   return add_parameter(paramList, name, values, CONSTANT);
}


/**
 * Add a new unnamed constant to the parameter list.
 * \param paramList - the parameter list
 * \param values - four float values
 * \return index of the new parameter.
 */
GLint
_mesa_add_unnamed_constant(struct program_parameter_list *paramList,
                           const GLfloat values[4])
{
   return add_parameter(paramList, NULL, values, CONSTANT);
}


/**
 * Add a new state reference to the parameter list.
 * \param paramList - the parameter list
 * \param state     - an array of 6 state tokens
 *
 * \return index of the new parameter.
 */
GLint
_mesa_add_state_reference(struct program_parameter_list *paramList,
                          GLint *stateTokens)
{
   /* XXX Should we parse <stateString> here and produce the parameter's
    * list of STATE_* tokens here, or in the parser?
    */
   GLint a, idx;

   idx = add_parameter(paramList, NULL, NULL, STATE);

   for (a=0; a<6; a++)
      paramList->Parameters[idx].StateIndexes[a] = (enum state_index) stateTokens[a];

   return idx;
}


/**
 * Lookup a parameter value by name in the given parameter list.
 * \return pointer to the float[4] values.
 */
GLfloat *
_mesa_lookup_parameter_value(struct program_parameter_list *paramList,
                             GLsizei nameLen, const char *name)
{
   GLuint i;

   if (!paramList)
      return NULL;

   if (nameLen == -1) {
      /* name is null-terminated */
      for (i = 0; i < paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strcmp(paramList->Parameters[i].Name, name) == 0)
            return paramList->ParameterValues[i];
      }
   }
   else {
      /* name is not null-terminated, use nameLen */
      for (i = 0; i < paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strncmp(paramList->Parameters[i].Name, name, nameLen) == 0
             && _mesa_strlen(paramList->Parameters[i].Name) == (size_t)nameLen)
            return paramList->ParameterValues[i];
      }
   }
   return NULL;
}


/**
 * Lookup a parameter index by name in the given parameter list.
 * \return index of parameter in the list.
 */
GLint
_mesa_lookup_parameter_index(struct program_parameter_list *paramList,
                             GLsizei nameLen, const char *name)
{
   GLint i;

   if (!paramList)
      return -1;

   if (nameLen == -1) {
      /* name is null-terminated */
      for (i = 0; i < (GLint) paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strcmp(paramList->Parameters[i].Name, name) == 0)
            return i;
      }
   }
   else {
      /* name is not null-terminated, use nameLen */
      for (i = 0; i < (GLint) paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strncmp(paramList->Parameters[i].Name, name, nameLen) == 0
             && _mesa_strlen(paramList->Parameters[i].Name) == (size_t)nameLen)
            return i;
      }
   }
   return -1;
}


/**
 * Use the list of tokens in the state[] array to find global GL state
 * and return it in <value>.  Usually, four values are returned in <value>
 * but matrix queries may return as many as 16 values.
 * This function is used for ARB vertex/fragment programs.
 * The program parser will produce the state[] values.
 */
static void
_mesa_fetch_state(GLcontext *ctx, const enum state_index state[],
                  GLfloat *value)
{
   switch (state[0]) {
   case STATE_MATERIAL:
      {
         /* state[1] is either 0=front or 1=back side */
         const GLuint face = (GLuint) state[1];
         /* state[2] is the material attribute */
         switch (state[2]) {
         case STATE_AMBIENT:
            if (face == 0)
               COPY_4V(value, ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT]);
            else
               COPY_4V(value, ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_AMBIENT]);
            return;
         case STATE_DIFFUSE:
            if (face == 0)
               COPY_4V(value, ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE]);
            else
               COPY_4V(value, ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_DIFFUSE]);
            return;
         case STATE_SPECULAR:
            if (face == 0)
               COPY_4V(value, ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SPECULAR]);
            else
               COPY_4V(value, ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_SPECULAR]);
            return;
         case STATE_EMISSION:
            if (face == 0)
               COPY_4V(value, ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_EMISSION]);
            else
               COPY_4V(value, ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_EMISSION]);
            return;
         case STATE_SHININESS:
            if (face == 0)
               value[0] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SHININESS][0];
            else
               value[0] = ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_SHININESS][0];
            value[1] = 0.0F;
            value[2] = 0.0F;
            value[3] = 1.0F;
            return;
         default:
            _mesa_problem(ctx, "Invalid material state in fetch_state");
            return;
         }
      }
   case STATE_LIGHT:
      {
         /* state[1] is the light number */
         const GLuint ln = (GLuint) state[1];
         /* state[2] is the light attribute */
         switch (state[2]) {
         case STATE_AMBIENT:
            COPY_4V(value, ctx->Light.Light[ln].Ambient);
            return;
         case STATE_DIFFUSE:
            COPY_4V(value, ctx->Light.Light[ln].Diffuse);
            return;
         case STATE_SPECULAR:
            COPY_4V(value, ctx->Light.Light[ln].Specular);
            return;
         case STATE_POSITION:
            COPY_4V(value, ctx->Light.Light[ln].EyePosition);
            return;
         case STATE_ATTENUATION:
            value[0] = ctx->Light.Light[ln].ConstantAttenuation;
            value[1] = ctx->Light.Light[ln].LinearAttenuation;
            value[2] = ctx->Light.Light[ln].QuadraticAttenuation;
            value[3] = ctx->Light.Light[ln].SpotExponent;
            return;
         case STATE_SPOT_DIRECTION:
            COPY_3V(value, ctx->Light.Light[ln].EyeDirection);
            value[3] = ctx->Light.Light[ln]._CosCutoff;
            return;
         case STATE_HALF:
            {
               GLfloat eye_z[] = {0, 0, 1};

               /* Compute infinite half angle vector:
                *   half-vector = light_position + (0, 0, 1)
                * and then normalize.  w = 0
		*
		* light.EyePosition.w should be 0 for infinite lights.
                */
	       ADD_3V(value, eye_z, ctx->Light.Light[ln].EyePosition);
	       NORMALIZE_3FV(value);
	       value[3] = 0;
            }
            return;
	 case STATE_POSITION_NORMALIZED:
            COPY_4V(value, ctx->Light.Light[ln].EyePosition);
	    NORMALIZE_3FV( value );
            return;
         default:
            _mesa_problem(ctx, "Invalid light state in fetch_state");
            return;
         }
      }
   case STATE_LIGHTMODEL_AMBIENT:
      COPY_4V(value, ctx->Light.Model.Ambient);
      return;
   case STATE_LIGHTMODEL_SCENECOLOR:
      if (state[1] == 0) {
         /* front */
         GLint i;
         for (i = 0; i < 3; i++) {
            value[i] = ctx->Light.Model.Ambient[i]
               * ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT][i]
               + ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_EMISSION][i];
         }
	 value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE][3];
      }
      else {
         /* back */
         GLint i;
         for (i = 0; i < 3; i++) {
            value[i] = ctx->Light.Model.Ambient[i]
               * ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_AMBIENT][i]
               + ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_EMISSION][i];
         }
	 value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_DIFFUSE][3];
      }
      return;
   case STATE_LIGHTPROD:
      {
         const GLuint ln = (GLuint) state[1];
         const GLuint face = (GLuint) state[2];
         GLint i;
         ASSERT(face == 0 || face == 1);
         switch (state[3]) {
            case STATE_AMBIENT:
               for (i = 0; i < 3; i++) {
                  value[i] = ctx->Light.Light[ln].Ambient[i] *
                     ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT+face][i];
               }
               /* [3] = material alpha */
               value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][3];
               return;
            case STATE_DIFFUSE:
               for (i = 0; i < 3; i++) {
                  value[i] = ctx->Light.Light[ln].Diffuse[i] *
                     ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][i];
               }
               /* [3] = material alpha */
               value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][3];
               return;
            case STATE_SPECULAR:
               for (i = 0; i < 3; i++) {
                  value[i] = ctx->Light.Light[ln].Specular[i] *
                     ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SPECULAR+face][i];
               }
               /* [3] = material alpha */
               value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][3];
               return;
            default:
               _mesa_problem(ctx, "Invalid lightprod state in fetch_state");
               return;
         }
      }
   case STATE_TEXGEN:
      {
         /* state[1] is the texture unit */
         const GLuint unit = (GLuint) state[1];
         /* state[2] is the texgen attribute */
         switch (state[2]) {
         case STATE_TEXGEN_EYE_S:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneS);
            return;
         case STATE_TEXGEN_EYE_T:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneT);
            return;
         case STATE_TEXGEN_EYE_R:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneR);
            return;
         case STATE_TEXGEN_EYE_Q:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneQ);
            return;
         case STATE_TEXGEN_OBJECT_S:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneS);
            return;
         case STATE_TEXGEN_OBJECT_T:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneT);
            return;
         case STATE_TEXGEN_OBJECT_R:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneR);
            return;
         case STATE_TEXGEN_OBJECT_Q:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneQ);
            return;
         default:
            _mesa_problem(ctx, "Invalid texgen state in fetch_state");
            return;
         }
      }
   case STATE_TEXENV_COLOR:
      {
         /* state[1] is the texture unit */
         const GLuint unit = (GLuint) state[1];
         COPY_4V(value, ctx->Texture.Unit[unit].EnvColor);
      }
      return;
   case STATE_FOG_COLOR:
      COPY_4V(value, ctx->Fog.Color);
      return;
   case STATE_FOG_PARAMS:
      value[0] = ctx->Fog.Density;
      value[1] = ctx->Fog.Start;
      value[2] = ctx->Fog.End;
      value[3] = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
      return;
   case STATE_CLIPPLANE:
      {
         const GLuint plane = (GLuint) state[1];
         COPY_4V(value, ctx->Transform.EyeUserPlane[plane]);
      }
      return;
   case STATE_POINT_SIZE:
      value[0] = ctx->Point.Size;
      value[1] = ctx->Point.MinSize;
      value[2] = ctx->Point.MaxSize;
      value[3] = ctx->Point.Threshold;
      return;
   case STATE_POINT_ATTENUATION:
      value[0] = ctx->Point.Params[0];
      value[1] = ctx->Point.Params[1];
      value[2] = ctx->Point.Params[2];
      value[3] = 1.0F;
      return;
   case STATE_MATRIX:
      {
         /* state[1] = modelview, projection, texture, etc. */
         /* state[2] = which texture matrix or program matrix */
         /* state[3] = first column to fetch */
         /* state[4] = last column to fetch */
         /* state[5] = transpose, inverse or invtrans */

         const GLmatrix *matrix;
         const enum state_index mat = state[1];
         const GLuint index = (GLuint) state[2];
         const GLuint first = (GLuint) state[3];
         const GLuint last = (GLuint) state[4];
         const enum state_index modifier = state[5];
         const GLfloat *m;
         GLuint row, i;
         if (mat == STATE_MODELVIEW) {
            matrix = ctx->ModelviewMatrixStack.Top;
         }
         else if (mat == STATE_PROJECTION) {
            matrix = ctx->ProjectionMatrixStack.Top;
         }
         else if (mat == STATE_MVP) {
            matrix = &ctx->_ModelProjectMatrix;
         }
         else if (mat == STATE_TEXTURE) {
            matrix = ctx->TextureMatrixStack[index].Top;
         }
         else if (mat == STATE_PROGRAM) {
            matrix = ctx->ProgramMatrixStack[index].Top;
         }
         else {
            _mesa_problem(ctx, "Bad matrix name in _mesa_fetch_state()");
            return;
         }
         if (modifier == STATE_MATRIX_INVERSE ||
             modifier == STATE_MATRIX_INVTRANS) {
            /* Be sure inverse is up to date:
	     */
	    _math_matrix_analyse( (GLmatrix*) matrix );
            m = matrix->inv;
         }
         else {
            m = matrix->m;
         }
         if (modifier == STATE_MATRIX_TRANSPOSE ||
             modifier == STATE_MATRIX_INVTRANS) {
            for (i = 0, row = first; row <= last; row++) {
               value[i++] = m[row * 4 + 0];
               value[i++] = m[row * 4 + 1];
               value[i++] = m[row * 4 + 2];
               value[i++] = m[row * 4 + 3];
            }
         }
         else {
            for (i = 0, row = first; row <= last; row++) {
               value[i++] = m[row + 0];
               value[i++] = m[row + 4];
               value[i++] = m[row + 8];
               value[i++] = m[row + 12];
            }
         }
      }
      return;
   case STATE_DEPTH_RANGE:
      value[0] = ctx->Viewport.Near;                     /* near       */
      value[1] = ctx->Viewport.Far;                      /* far        */
      value[2] = ctx->Viewport.Far - ctx->Viewport.Near; /* far - near */
      value[3] = 0;
      return;
   case STATE_FRAGMENT_PROGRAM:
      {
         /* state[1] = {STATE_ENV, STATE_LOCAL} */
         /* state[2] = parameter index          */
         const int idx = (int) state[2];
         switch (state[1]) {
            case STATE_ENV:
               COPY_4V(value, ctx->FragmentProgram.Parameters[idx]);
               break;
            case STATE_LOCAL:
               COPY_4V(value, ctx->FragmentProgram.Current->Base.LocalParams[idx]);
               break;
            default:
               _mesa_problem(ctx, "Bad state switch in _mesa_fetch_state()");
               return;
         }
      }
      return;

   case STATE_VERTEX_PROGRAM:
      {
         /* state[1] = {STATE_ENV, STATE_LOCAL} */
         /* state[2] = parameter index          */
         const int idx = (int) state[2];
         switch (state[1]) {
            case STATE_ENV:
               COPY_4V(value, ctx->VertexProgram.Parameters[idx]);
               break;
            case STATE_LOCAL:
               COPY_4V(value, ctx->VertexProgram.Current->Base.LocalParams[idx]);
               break;
            default:
               _mesa_problem(ctx, "Bad state switch in _mesa_fetch_state()");
               return;
         }
      }
      return;

   case STATE_INTERNAL:
      {
         switch (state[1]) {
	    case STATE_NORMAL_SCALE:
               ASSIGN_4V(value, ctx->_ModelViewInvScale, 0, 0, 1);
               break;
	    default:
               _mesa_problem(ctx, "Bad state switch in _mesa_fetch_state()");
               return;
         }
      }
      return;

   default:
      _mesa_problem(ctx, "Invalid state in _mesa_fetch_state");
      return;
   }
}


/**
 * Loop over all the parameters in a parameter list.  If the parameter
 * is a GL state reference, look up the current value of that state
 * variable and put it into the parameter's Value[4] array.
 * This would be called at glBegin time when using a fragment program.
 */
void
_mesa_load_state_parameters(GLcontext *ctx,
                            struct program_parameter_list *paramList)
{
   GLuint i;

   if (!paramList)
      return;

   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Type == STATE) {
         _mesa_fetch_state(ctx,
			   paramList->Parameters[i].StateIndexes,
                           paramList->ParameterValues[i]);
      }
   }
}



/**********************************************************************/
/* API functions                                                      */
/**********************************************************************/


/**
 * Bind a program (make it current)
 * \note Called from the GL API dispatcher by both glBindProgramNV
 * and glBindProgramARB.
 */
void GLAPIENTRY
_mesa_BindProgram(GLenum target, GLuint id)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if ((target == GL_VERTEX_PROGRAM_NV
        && ctx->Extensions.NV_vertex_program) ||
       (target == GL_VERTEX_PROGRAM_ARB
        && ctx->Extensions.ARB_vertex_program)) {
      /*** Vertex program binding ***/
      struct vertex_program *curProg = ctx->VertexProgram.Current;
      if (curProg->Base.Id == id) {
         /* binding same program - no change */
         return;
      }
      if (curProg->Base.Id != 0) {
         /* decrement refcount on previously bound vertex program */
         curProg->Base.RefCount--;
         /* and delete if refcount goes below one */
         if (curProg->Base.RefCount <= 0) {
            /* the program ID was already removed from the hash table */
            ctx->Driver.DeleteProgram(ctx, &(curProg->Base));
         }
      }
   }
   else if ((target == GL_FRAGMENT_PROGRAM_NV
             && ctx->Extensions.NV_fragment_program) ||
            (target == GL_FRAGMENT_PROGRAM_ARB
             && ctx->Extensions.ARB_fragment_program)) {
      /*** Fragment program binding ***/
      struct fragment_program *curProg = ctx->FragmentProgram.Current;
      if (curProg->Base.Id == id) {
         /* binding same program - no change */
         return;
      }
      if (curProg->Base.Id != 0) {
         /* decrement refcount on previously bound fragment program */
         curProg->Base.RefCount--;
         /* and delete if refcount goes below one */
         if (curProg->Base.RefCount <= 0) {
            /* the program ID was already removed from the hash table */
            ctx->Driver.DeleteProgram(ctx, &(curProg->Base));
         }
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindProgramNV/ARB(target)");
      return;
   }

   /* NOTE: binding to a non-existant program is not an error.
    * That's supposed to be caught in glBegin.
    */
   if (id == 0) {
      /* Bind default program */
      prog = NULL;
      if (target == GL_VERTEX_PROGRAM_NV || target == GL_VERTEX_PROGRAM_ARB)
         prog = ctx->Shared->DefaultVertexProgram;
      else
         prog = ctx->Shared->DefaultFragmentProgram;
   }
   else {
      /* Bind user program */
      prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, id);
      if (!prog || prog == &_mesa_DummyProgram) {
         /* allocate a new program now */
         prog = ctx->Driver.NewProgram(ctx, target, id);
         if (!prog) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindProgramNV/ARB");
            return;
         }
         _mesa_HashInsert(ctx->Shared->Programs, id, prog);
      }
      else if (prog->Target != target) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBindProgramNV/ARB(target mismatch)");
         return;
      }
   }

   /* bind now */
   if (target == GL_VERTEX_PROGRAM_NV || target == GL_VERTEX_PROGRAM_ARB) {
      ctx->VertexProgram.Current = (struct vertex_program *) prog;
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV || target == GL_FRAGMENT_PROGRAM_ARB) {
      ctx->FragmentProgram.Current = (struct fragment_program *) prog;
   }

   /* Never null pointers */
   ASSERT(ctx->VertexProgram.Current);
   ASSERT(ctx->FragmentProgram.Current);

   if (prog)
      prog->RefCount++;

   if (ctx->Driver.BindProgram)
      ctx->Driver.BindProgram(ctx, target, prog);
}


/**
 * Delete a list of programs.
 * \note Not compiled into display lists.
 * \note Called by both glDeleteProgramsNV and glDeleteProgramsARB.
 */
void GLAPIENTRY
_mesa_DeletePrograms(GLsizei n, const GLuint *ids)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glDeleteProgramsNV" );
      return;
   }

   for (i = 0; i < n; i++) {
      if (ids[i] != 0) {
         struct program *prog = (struct program *)
            _mesa_HashLookup(ctx->Shared->Programs, ids[i]);
         if (prog == &_mesa_DummyProgram) {
            _mesa_HashRemove(ctx->Shared->Programs, ids[i]);
         }
         else if (prog) {
            /* Unbind program if necessary */
            if (prog->Target == GL_VERTEX_PROGRAM_NV ||
                prog->Target == GL_VERTEX_STATE_PROGRAM_NV) {
               if (ctx->VertexProgram.Current &&
                   ctx->VertexProgram.Current->Base.Id == ids[i]) {
                  /* unbind this currently bound program */
                  _mesa_BindProgram(prog->Target, 0);
               }
            }
            else if (prog->Target == GL_FRAGMENT_PROGRAM_NV ||
                     prog->Target == GL_FRAGMENT_PROGRAM_ARB) {
               if (ctx->FragmentProgram.Current &&
                   ctx->FragmentProgram.Current->Base.Id == ids[i]) {
                  /* unbind this currently bound program */
                  _mesa_BindProgram(prog->Target, 0);
               }
            }
            else {
               _mesa_problem(ctx, "bad target in glDeleteProgramsNV");
               return;
            }
            /* The ID is immediately available for re-use now */
            _mesa_HashRemove(ctx->Shared->Programs, ids[i]);
            prog->RefCount--;
            if (prog->RefCount <= 0) {
               ctx->Driver.DeleteProgram(ctx, prog);
            }
         }
      }
   }
}


/**
 * Generate a list of new program identifiers.
 * \note Not compiled into display lists.
 * \note Called by both glGenProgramsNV and glGenProgramsARB.
 */
void GLAPIENTRY
_mesa_GenPrograms(GLsizei n, GLuint *ids)
{
   GLuint first;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenPrograms");
      return;
   }

   if (!ids)
      return;

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->Programs, n);

   /* Insert pointer to dummy program as placeholder */
   for (i = 0; i < (GLuint) n; i++) {
      _mesa_HashInsert(ctx->Shared->Programs, first + i, &_mesa_DummyProgram);
   }

   /* Return the program names */
   for (i = 0; i < (GLuint) n; i++) {
      ids[i] = first + i;
   }
}


/**
 * Determine if id names a vertex or fragment program.
 * \note Not compiled into display lists.
 * \note Called from both glIsProgramNV and glIsProgramARB.
 * \param id is the program identifier
 * \return GL_TRUE if id is a program, else GL_FALSE.
 */
GLboolean GLAPIENTRY
_mesa_IsProgram(GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (id == 0)
      return GL_FALSE;

   if (_mesa_HashLookup(ctx->Shared->Programs, id))
      return GL_TRUE;
   else
      return GL_FALSE;
}



/**********************************************************************/
/* GL_MESA_program_debug extension                                    */
/**********************************************************************/


/* XXX temporary */
GLAPI void GLAPIENTRY
glProgramCallbackMESA(GLenum target, GLprogramcallbackMESA callback,
                      GLvoid *data)
{
   _mesa_ProgramCallbackMESA(target, callback, data);
}


void
_mesa_ProgramCallbackMESA(GLenum target, GLprogramcallbackMESA callback,
                          GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);

   switch (target) {
      case GL_FRAGMENT_PROGRAM_ARB:
         if (!ctx->Extensions.ARB_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
            return;
         }
         ctx->FragmentProgram.Callback = callback;
         ctx->FragmentProgram.CallbackData = data;
         break;
      case GL_FRAGMENT_PROGRAM_NV:
         if (!ctx->Extensions.NV_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
            return;
         }
         ctx->FragmentProgram.Callback = callback;
         ctx->FragmentProgram.CallbackData = data;
         break;
      case GL_VERTEX_PROGRAM_ARB: /* == GL_VERTEX_PROGRAM_NV */
         if (!ctx->Extensions.ARB_vertex_program &&
             !ctx->Extensions.NV_vertex_program) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
            return;
         }
         ctx->VertexProgram.Callback = callback;
         ctx->VertexProgram.CallbackData = data;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glProgramCallbackMESA(target)");
         return;
   }
}


/* XXX temporary */
GLAPI void GLAPIENTRY
glGetProgramRegisterfvMESA(GLenum target,
                           GLsizei len, const GLubyte *registerName,
                           GLfloat *v)
{
   _mesa_GetProgramRegisterfvMESA(target, len, registerName, v);
}


void
_mesa_GetProgramRegisterfvMESA(GLenum target,
                               GLsizei len, const GLubyte *registerName,
                               GLfloat *v)
{
   char reg[1000];
   GET_CURRENT_CONTEXT(ctx);

   /* We _should_ be inside glBegin/glEnd */
#if 0
   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramRegisterfvMESA");
      return;
   }
#endif

   /* make null-terminated copy of registerName */
   len = MIN2((unsigned int) len, sizeof(reg) - 1);
   _mesa_memcpy(reg, registerName, len);
   reg[len] = 0;

   switch (target) {
      case GL_VERTEX_PROGRAM_NV:
         if (!ctx->Extensions.ARB_vertex_program &&
             !ctx->Extensions.NV_vertex_program) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetProgramRegisterfvMESA(target)");
            return;
         }
         if (!ctx->VertexProgram._Enabled) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetProgramRegisterfvMESA");
            return;
         }
         /* GL_NV_vertex_program */
         if (reg[0] == 'R') {
            /* Temp register */
            GLint i = _mesa_atoi(reg + 1);
            if (i >= (GLint)ctx->Const.MaxVertexProgramTemps) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glGetProgramRegisterfvMESA(registerName)");
               return;
            }
            COPY_4V(v, ctx->VertexProgram.Temporaries[i]);
         }
         else if (reg[0] == 'v' && reg[1] == '[') {
            /* Vertex Input attribute */
            GLuint i;
            for (i = 0; i < ctx->Const.MaxVertexProgramAttribs; i++) {
               const char *name = _mesa_nv_vertex_input_register_name(i);
               char number[10];
               sprintf(number, "%d", i);
               if (_mesa_strncmp(reg + 2, name, 4) == 0 ||
                   _mesa_strncmp(reg + 2, number, _mesa_strlen(number)) == 0) {
                  COPY_4V(v, ctx->VertexProgram.Inputs[i]);
                  return;
               }
            }
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramRegisterfvMESA(registerName)");
            return;
         }
         else if (reg[0] == 'o' && reg[1] == '[') {
            /* Vertex output attribute */
         }
         /* GL_ARB_vertex_program */
         else if (_mesa_strncmp(reg, "vertex.", 7) == 0) {

         }
         else {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramRegisterfvMESA(registerName)");
            return;
         }
         break;
      case GL_FRAGMENT_PROGRAM_ARB:
         if (!ctx->Extensions.ARB_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetProgramRegisterfvMESA(target)");
            return;
         }
         if (!ctx->FragmentProgram._Enabled) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetProgramRegisterfvMESA");
            return;
         }
         /* XXX to do */
         break;
      case GL_FRAGMENT_PROGRAM_NV:
         if (!ctx->Extensions.NV_fragment_program) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetProgramRegisterfvMESA(target)");
            return;
         }
         if (!ctx->FragmentProgram._Enabled) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glGetProgramRegisterfvMESA");
            return;
         }
         if (reg[0] == 'R') {
            /* Temp register */
            GLint i = _mesa_atoi(reg + 1);
            if (i >= (GLint)ctx->Const.MaxFragmentProgramTemps) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glGetProgramRegisterfvMESA(registerName)");
               return;
            }
            COPY_4V(v, ctx->FragmentProgram.Machine.Temporaries[i]);
         }
         else if (reg[0] == 'f' && reg[1] == '[') {
            /* Fragment input attribute */
            GLuint i;
            for (i = 0; i < ctx->Const.MaxFragmentProgramAttribs; i++) {
               const char *name = _mesa_nv_fragment_input_register_name(i);
               if (_mesa_strncmp(reg + 2, name, 4) == 0) {
                  COPY_4V(v, ctx->FragmentProgram.Machine.Inputs[i]);
                  return;
               }
            }
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramRegisterfvMESA(registerName)");
            return;
         }
         else if (_mesa_strcmp(reg, "o[COLR]") == 0) {
            /* Fragment output color */
            COPY_4V(v, ctx->FragmentProgram.Machine.Outputs[FRAG_OUTPUT_COLR]);
         }
         else if (_mesa_strcmp(reg, "o[COLH]") == 0) {
            /* Fragment output color */
            COPY_4V(v, ctx->FragmentProgram.Machine.Outputs[FRAG_OUTPUT_COLH]);
         }
         else if (_mesa_strcmp(reg, "o[DEPR]") == 0) {
            /* Fragment output depth */
            COPY_4V(v, ctx->FragmentProgram.Machine.Outputs[FRAG_OUTPUT_DEPR]);
         }
         else {
            /* try user-defined identifiers */
            const GLfloat *value = _mesa_lookup_parameter_value(
                       ctx->FragmentProgram.Current->Parameters, -1, reg);
            if (value) {
               COPY_4V(v, value);
            }
            else {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glGetProgramRegisterfvMESA(registerName)");
               return;
            }
         }
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glGetProgramRegisterfvMESA(target)");
         return;
   }

}
