/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
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

/**
 * \file prog_parameter.c
 * Program parameter lists and functions.
 * \author Brian Paul
 */


#include "main/glheader.h"
#include "main/imports.h"
#include "main/macros.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_statevars.h"


struct gl_program_parameter_list *
_mesa_new_parameter_list(void)
{
   return CALLOC_STRUCT(gl_program_parameter_list);
}


/**
 * Free a parameter list and all its parameters
 */
void
_mesa_free_parameter_list(struct gl_program_parameter_list *paramList)
{
   GLuint i;
   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Name)
	 _mesa_free((void *) paramList->Parameters[i].Name);
   }
   _mesa_free(paramList->Parameters);
   if (paramList->ParameterValues)
      _mesa_align_free(paramList->ParameterValues);
   _mesa_free(paramList);
}


/**
 * Add a new parameter to a parameter list.
 * Note that parameter values are usually 4-element GLfloat vectors.
 * When size > 4 we'll allocate a sequential block of parameters to
 * store all the values (in blocks of 4).
 *
 * \param paramList  the list to add the parameter to
 * \param type  type of parameter, such as 
 * \param name  the parameter name, will be duplicated/copied!
 * \param size  number of elements in 'values' vector (1..4, or more)
 * \param values  initial parameter value, up to 4 GLfloats, or NULL
 * \param state  state indexes, or NULL
 * \return  index of new parameter in the list, or -1 if error (out of mem)
 */
GLint
_mesa_add_parameter(struct gl_program_parameter_list *paramList,
                    enum register_file type, const char *name,
                    GLuint size, GLenum datatype, const GLfloat *values,
                    const gl_state_index state[STATE_LENGTH])
{
   const GLuint oldNum = paramList->NumParameters;
   const GLuint sz4 = (size + 3) / 4; /* no. of new param slots needed */

   assert(size > 0);

   if (oldNum + sz4 > paramList->Size) {
      /* Need to grow the parameter list array (alloc some extra) */
      paramList->Size = paramList->Size + 4 * sz4;

      /* realloc arrays */
      paramList->Parameters = (struct gl_program_parameter *)
	 _mesa_realloc(paramList->Parameters,
		       oldNum * sizeof(struct gl_program_parameter),
		       paramList->Size * sizeof(struct gl_program_parameter));

      paramList->ParameterValues = (GLfloat (*)[4])
         _mesa_align_realloc(paramList->ParameterValues,         /* old buf */
                             oldNum * 4 * sizeof(GLfloat),      /* old size */
                             paramList->Size * 4 *sizeof(GLfloat), /* new sz */
                             16);
   }

   if (!paramList->Parameters ||
       !paramList->ParameterValues) {
      /* out of memory */
      paramList->NumParameters = 0;
      paramList->Size = 0;
      return -1;
   }
   else {
      GLuint i;

      paramList->NumParameters = oldNum + sz4;

      _mesa_memset(&paramList->Parameters[oldNum], 0, 
		   sz4 * sizeof(struct gl_program_parameter));

      for (i = 0; i < sz4; i++) {
         struct gl_program_parameter *p = paramList->Parameters + oldNum + i;
         p->Name = name ? _mesa_strdup(name) : NULL;
         p->Type = type;
         p->Size = size;
         p->DataType = datatype;
         if (values) {
            COPY_4V(paramList->ParameterValues[oldNum + i], values);
            values += 4;
         }
         else {
            /* silence valgrind */
            ASSIGN_4V(paramList->ParameterValues[oldNum + i], 0, 0, 0, 0);
         }
         size -= 4;
      }

      if (state) {
         for (i = 0; i < STATE_LENGTH; i++)
            paramList->Parameters[oldNum].StateIndexes[i] = state[i];
      }

      return (GLint) oldNum;
   }
}


/**
 * Add a new named program parameter (Ex: NV_fragment_program DEFINE statement)
 * \return index of the new entry in the parameter list
 */
GLint
_mesa_add_named_parameter(struct gl_program_parameter_list *paramList,
                          const char *name, const GLfloat values[4])
{
   return _mesa_add_parameter(paramList, PROGRAM_NAMED_PARAM, name,
                              4, GL_NONE, values, NULL);
                              
}


/**
 * Add a new named constant to the parameter list.
 * This will be used when the program contains something like this:
 *    PARAM myVals = { 0, 1, 2, 3 };
 *
 * \param paramList  the parameter list
 * \param name  the name for the constant
 * \param values  four float values
 * \return index/position of the new parameter in the parameter list
 */
GLint
_mesa_add_named_constant(struct gl_program_parameter_list *paramList,
                         const char *name, const GLfloat values[4],
                         GLuint size)
{
#if 0 /* disable this for now -- we need to save the name! */
   GLint pos;
   GLuint swizzle;
   ASSERT(size == 4); /* XXX future feature */
   /* check if we already have this constant */
   if (_mesa_lookup_parameter_constant(paramList, values, 4, &pos, &swizzle)) {
      return pos;
   }
#endif
   size = 4; /** XXX fix */
   return _mesa_add_parameter(paramList, PROGRAM_CONSTANT, name,
                              size, GL_NONE, values, NULL);
}


/**
 * Add a new unnamed constant to the parameter list.  This will be used
 * when a fragment/vertex program contains something like this:
 *    MOV r, { 0, 1, 2, 3 };
 * We'll search the parameter list for an existing instance of the
 * constant.  If swizzleOut is non-null, we'll try swizzling when
 * looking for a match.
 *
 * \param paramList  the parameter list
 * \param values  four float values
 * \param swizzleOut  returns swizzle mask for accessing the constant
 * \return index/position of the new parameter in the parameter list.
 */
GLint
_mesa_add_unnamed_constant(struct gl_program_parameter_list *paramList,
                           const GLfloat values[4], GLuint size,
                           GLuint *swizzleOut)
{
   GLint pos;
   ASSERT(size >= 1);
   ASSERT(size <= 4);

   if (_mesa_lookup_parameter_constant(paramList, values,
                                       size, &pos, swizzleOut)) {
      return pos;
   }

   /* Look for empty space in an already unnamed constant parameter
    * to add this constant.  This will only work for single-element
    * constants because we rely on smearing (i.e. .yyyy or .zzzz).
    */
   if (size == 1 && swizzleOut) {
      for (pos = 0; pos < (GLint) paramList->NumParameters; pos++) {
         struct gl_program_parameter *p = paramList->Parameters + pos;
         if (p->Type == PROGRAM_CONSTANT && p->Size + size <= 4) {
            /* ok, found room */
            GLfloat *pVal = paramList->ParameterValues[pos];
            GLuint swz = p->Size; /* 1, 2 or 3 for Y, Z, W */
            pVal[p->Size] = values[0];
            p->Size++;
            *swizzleOut = MAKE_SWIZZLE4(swz, swz, swz, swz);
            return pos;
         }
      }
   }

   /* add a new parameter to store this constant */
   pos = _mesa_add_parameter(paramList, PROGRAM_CONSTANT, NULL,
                             size, GL_NONE, values, NULL);
   if (pos >= 0 && swizzleOut) {
      if (size == 1)
         *swizzleOut = SWIZZLE_XXXX;
      else
         *swizzleOut = SWIZZLE_NOOP;
   }
   return pos;
}


/**
 * Add a uniform to the parameter list.
 * Note that if the uniform is an array, size may be greater than
 * what's implied by the datatype.
 * \param name  uniform's name
 * \param size  number of floats to allocate
 * \param datatype  GL_FLOAT_VEC3, GL_FLOAT_MAT4, etc.
 */
GLint
_mesa_add_uniform(struct gl_program_parameter_list *paramList,
                  const char *name, GLuint size, GLenum datatype)
{
   GLint i = _mesa_lookup_parameter_index(paramList, -1, name);
   ASSERT(datatype != GL_NONE);
   if (i >= 0 && paramList->Parameters[i].Type == PROGRAM_UNIFORM) {
      ASSERT(paramList->Parameters[i].Size == size);
      ASSERT(paramList->Parameters[i].DataType == datatype);
      /* already in list */
      return i;
   }
   else {
      i = _mesa_add_parameter(paramList, PROGRAM_UNIFORM, name,
                              size, datatype, NULL, NULL);
      return i;
   }
}


/**
 * Mark the named uniform as 'used'.
 */
void
_mesa_use_uniform(struct gl_program_parameter_list *paramList,
                  const char *name)
{
   GLuint i;
   for (i = 0; i < paramList->NumParameters; i++) {
      struct gl_program_parameter *p = paramList->Parameters + i;
      if (p->Type == PROGRAM_UNIFORM && _mesa_strcmp(p->Name, name) == 0) {
         p->Used = GL_TRUE;
         /* Note that large uniforms may occupy several slots so we're
          * not done searching yet.
          */
      }
   }
}


/**
 * Add a sampler to the parameter list.
 * \param name  uniform's name
 * \param datatype  GL_SAMPLER_2D, GL_SAMPLER_2D_RECT_ARB, etc.
 * \param index  the sampler number (as seen in TEX instructions)
 * \return  sampler index (starting at zero) or -1 if error
 */
GLint
_mesa_add_sampler(struct gl_program_parameter_list *paramList,
                  const char *name, GLenum datatype)
{
   GLint i = _mesa_lookup_parameter_index(paramList, -1, name);
   if (i >= 0 && paramList->Parameters[i].Type == PROGRAM_SAMPLER) {
      ASSERT(paramList->Parameters[i].Size == 1);
      ASSERT(paramList->Parameters[i].DataType == datatype);
      /* already in list */
      return (GLint) paramList->ParameterValues[i][0];
   }
   else {
      GLuint i;
      const GLint size = 1; /* a sampler is basically a texture unit number */
      GLfloat value;
      GLint numSamplers = 0;
      for (i = 0; i < paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Type == PROGRAM_SAMPLER)
            numSamplers++;
      }
      value = (GLfloat) numSamplers;
      (void) _mesa_add_parameter(paramList, PROGRAM_SAMPLER, name,
                                 size, datatype, &value, NULL);
      return numSamplers;
   }
}


/**
 * Add parameter representing a varying variable.
 */
GLint
_mesa_add_varying(struct gl_program_parameter_list *paramList,
                  const char *name, GLuint size)
{
   GLint i = _mesa_lookup_parameter_index(paramList, -1, name);
   if (i >= 0 && paramList->Parameters[i].Type == PROGRAM_VARYING) {
      /* already in list */
      return i;
   }
   else {
      /*assert(size == 4);*/
      i = _mesa_add_parameter(paramList, PROGRAM_VARYING, name,
                              size, GL_NONE, NULL, NULL);
      return i;
   }
}


/**
 * Add parameter representing a vertex program attribute.
 * \param size  size of attribute (in floats), may be -1 if unknown
 * \param attrib  the attribute index, or -1 if unknown
 */
GLint
_mesa_add_attribute(struct gl_program_parameter_list *paramList,
                    const char *name, GLint size, GLenum datatype, GLint attrib)
{
   GLint i = _mesa_lookup_parameter_index(paramList, -1, name);
   if (i >= 0) {
      /* replace */
      if (attrib < 0)
         attrib = i;
      paramList->Parameters[i].StateIndexes[0] = attrib;
   }
   else {
      /* add */
      gl_state_index state[STATE_LENGTH];
      state[0] = (gl_state_index) attrib;
      if (size < 0)
         size = 4;
      i = _mesa_add_parameter(paramList, PROGRAM_INPUT, name,
                              size, datatype, NULL, state);
   }
   return i;
}



#if 0 /* not used yet */
/**
 * Returns the number of 4-component registers needed to store a piece
 * of GL state.  For matrices this may be as many as 4 registers,
 * everything else needs
 * just 1 register.
 */
static GLuint
sizeof_state_reference(const GLint *stateTokens)
{
   if (stateTokens[0] == STATE_MATRIX) {
      GLuint rows = stateTokens[4] - stateTokens[3] + 1;
      assert(rows >= 1);
      assert(rows <= 4);
      return rows;
   }
   else {
      return 1;
   }
}
#endif


/**
 * Add a new state reference to the parameter list.
 * This will be used when the program contains something like this:
 *    PARAM ambient = state.material.front.ambient;
 *
 * \param paramList  the parameter list
 * \param stateTokens  an array of 5 (STATE_LENGTH) state tokens
 * \return index of the new parameter.
 */
GLint
_mesa_add_state_reference(struct gl_program_parameter_list *paramList,
                          const gl_state_index stateTokens[STATE_LENGTH])
{
   const GLuint size = 4; /* XXX fix */
   const char *name;
   GLint index;

   /* Check if the state reference is already in the list */
   for (index = 0; index < (GLint) paramList->NumParameters; index++) {
      GLuint i, match = 0;
      for (i = 0; i < STATE_LENGTH; i++) {
         if (paramList->Parameters[index].StateIndexes[i] == stateTokens[i]) {
            match++;
         }
         else {
            break;
         }
      }
      if (match == STATE_LENGTH) {
         /* this state reference is already in the parameter list */
         return index;
      }
   }

   name = _mesa_program_state_string(stateTokens);
   index = _mesa_add_parameter(paramList, PROGRAM_STATE_VAR, name,
                               size, GL_NONE,
                               NULL, (gl_state_index *) stateTokens);
   paramList->StateFlags |= _mesa_program_state_flags(stateTokens);

   /* free name string here since we duplicated it in add_parameter() */
   _mesa_free((void *) name);

   return index;
}


/**
 * Lookup a parameter value by name in the given parameter list.
 * \return pointer to the float[4] values.
 */
GLfloat *
_mesa_lookup_parameter_value(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name)
{
   GLuint i = _mesa_lookup_parameter_index(paramList, nameLen, name);
   if (i < 0)
      return NULL;
   else
      return paramList->ParameterValues[i];
}


/**
 * Given a program parameter name, find its position in the list of parameters.
 * \param paramList  the parameter list to search
 * \param nameLen  length of name (in chars).
 *                 If length is negative, assume that name is null-terminated.
 * \param name  the name to search for
 * \return index of parameter in the list.
 */
GLint
_mesa_lookup_parameter_index(const struct gl_program_parameter_list *paramList,
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
 * Look for a float vector in the given parameter list.  The float vector
 * may be of length 1, 2, 3 or 4.  If swizzleOut is non-null, we'll try
 * swizzling to find a match.
 * \param list  the parameter list to search
 * \param v  the float vector to search for
 * \param size  number of element in v
 * \param posOut  returns the position of the constant, if found
 * \param swizzleOut  returns a swizzle mask describing location of the
 *                    vector elements if found.
 * \return GL_TRUE if found, GL_FALSE if not found
 */
GLboolean
_mesa_lookup_parameter_constant(const struct gl_program_parameter_list *list,
                                const GLfloat v[], GLuint vSize,
                                GLint *posOut, GLuint *swizzleOut)
{
   GLuint i;

   assert(vSize >= 1);
   assert(vSize <= 4);

   if (!list)
      return -1;

   for (i = 0; i < list->NumParameters; i++) {
      if (list->Parameters[i].Type == PROGRAM_CONSTANT) {
         if (!swizzleOut) {
            /* swizzle not allowed */
            GLuint j, match = 0;
            for (j = 0; j < vSize; j++) {
               if (v[j] == list->ParameterValues[i][j])
                  match++;
            }
            if (match == vSize) {
               *posOut = i;
               return GL_TRUE;
            }
         }
         else {
            /* try matching w/ swizzle */
             if (vSize == 1) {
                /* look for v[0] anywhere within float[4] value */
                GLuint j;
                for (j = 0; j < 4; j++) {
                   if (list->ParameterValues[i][j] == v[0]) {
                      /* found it */
                      *posOut = i;
                      *swizzleOut = MAKE_SWIZZLE4(j, j, j, j);
                      return GL_TRUE;
                   }
                }
             }
             else if (vSize <= list->Parameters[i].Size) {
                /* see if we can match this constant (with a swizzle) */
                GLuint swz[4];
                GLuint match = 0, j, k;
                for (j = 0; j < vSize; j++) {
                   if (v[j] == list->ParameterValues[i][j]) {
                      swz[j] = j;
                      match++;
                   }
                   else {
                      for (k = 0; k < list->Parameters[i].Size; k++) {
                         if (v[j] == list->ParameterValues[i][k]) {
                            swz[j] = k;
                            match++;
                            break;
                         }
                      }
                   }
                }
                /* smear last value to remaining positions */
                for (; j < 4; j++)
                   swz[j] = swz[j-1];

                if (match == vSize) {
                   *posOut = i;
                   *swizzleOut = MAKE_SWIZZLE4(swz[0], swz[1], swz[2], swz[3]);
                   return GL_TRUE;
                }
             }
         }
      }
   }

   *posOut = -1;
   return GL_FALSE;
}


struct gl_program_parameter_list *
_mesa_clone_parameter_list(const struct gl_program_parameter_list *list)
{
   struct gl_program_parameter_list *clone;
   GLuint i;

   clone = _mesa_new_parameter_list();
   if (!clone)
      return NULL;

   /** Not too efficient, but correct */
   for (i = 0; i < list->NumParameters; i++) {
      struct gl_program_parameter *p = list->Parameters + i;
      struct gl_program_parameter *pCopy;
      GLuint size = MIN2(p->Size, 4);
      GLint j = _mesa_add_parameter(clone, p->Type, p->Name, size, p->DataType,
                                    list->ParameterValues[i], NULL);
      ASSERT(j >= 0);
      pCopy = clone->Parameters + j;
      pCopy->Used = p->Used;
      /* copy state indexes */
      if (p->Type == PROGRAM_STATE_VAR) {
         GLint k;
         for (k = 0; k < STATE_LENGTH; k++) {
            pCopy->StateIndexes[k] = p->StateIndexes[k];
         }
      }
      else {
         clone->Parameters[j].Size = p->Size;
      }
      
   }

   clone->StateFlags = list->StateFlags;

   return clone;
}


/**
 * Return a new parameter list which is listA + listB.
 */
struct gl_program_parameter_list *
_mesa_combine_parameter_lists(const struct gl_program_parameter_list *listA,
                              const struct gl_program_parameter_list *listB)
{
   struct gl_program_parameter_list *list;

   if (listA) {
      list = _mesa_clone_parameter_list(listA);
      if (list && listB) {
         GLuint i;
         for (i = 0; i < listB->NumParameters; i++) {
            struct gl_program_parameter *param = listB->Parameters + i;
            _mesa_add_parameter(list, param->Type, param->Name, param->Size,
                                param->DataType,
                                listB->ParameterValues[i],
                                param->StateIndexes);
         }
      }
   }
   else if (listB) {
      list = _mesa_clone_parameter_list(listB);
   }
   else {
      list = NULL;
   }
   return list;
}



/**
 * Find longest name of all uniform parameters in list.
 */
GLuint
_mesa_longest_parameter_name(const struct gl_program_parameter_list *list,
                             enum register_file type)
{
   GLuint i, maxLen = 0;
   if (!list)
      return 0;
   for (i = 0; i < list->NumParameters; i++) {
      if (list->Parameters[i].Type == type) {
         GLuint len = _mesa_strlen(list->Parameters[i].Name);
         if (len > maxLen)
            maxLen = len;
      }
   }
   return maxLen;
}


/**
 * Count the number of parameters in the last that match the given type.
 */
GLuint
_mesa_num_parameters_of_type(const struct gl_program_parameter_list *list,
                             enum register_file type)
{
   GLuint i, count = 0;
   if (list) {
      for (i = 0; i < list->NumParameters; i++) {
         if (list->Parameters[i].Type == type)
            count++;
      }
   }
   return count;
}
