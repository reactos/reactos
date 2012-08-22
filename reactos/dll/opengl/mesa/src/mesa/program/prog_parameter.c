/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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


struct gl_program_parameter_list *
_mesa_new_parameter_list_sized(unsigned size)
{
   struct gl_program_parameter_list *p = _mesa_new_parameter_list();

   if ((p != NULL) && (size != 0)) {
      p->Size = size;

      /* alloc arrays */
      p->Parameters = (struct gl_program_parameter *)
	 calloc(1, size * sizeof(struct gl_program_parameter));

      p->ParameterValues = (gl_constant_value (*)[4])
         _mesa_align_malloc(size * 4 *sizeof(gl_constant_value), 16);


      if ((p->Parameters == NULL) || (p->ParameterValues == NULL)) {
	 free(p->Parameters);
	 _mesa_align_free(p->ParameterValues);
	 free(p);
	 p = NULL;
      }
   }

   return p;
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
	 free((void *) paramList->Parameters[i].Name);
   }
   free(paramList->Parameters);
   if (paramList->ParameterValues)
      _mesa_align_free(paramList->ParameterValues);
   free(paramList);
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
 * \param datatype  GL_FLOAT, GL_FLOAT_VECx, GL_INT, GL_INT_VECx or GL_NONE.
 * \param values  initial parameter value, up to 4 gl_constant_values, or NULL
 * \param state  state indexes, or NULL
 * \return  index of new parameter in the list, or -1 if error (out of mem)
 */
GLint
_mesa_add_parameter(struct gl_program_parameter_list *paramList,
                    gl_register_file type, const char *name,
                    GLuint size, GLenum datatype,
                    const gl_constant_value *values,
                    const gl_state_index state[STATE_LENGTH],
                    GLbitfield flags)
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

      paramList->ParameterValues = (gl_constant_value (*)[4])
         _mesa_align_realloc(paramList->ParameterValues,         /* old buf */
                             oldNum * 4 * sizeof(gl_constant_value),/* old sz */
                             paramList->Size*4*sizeof(gl_constant_value),/*new*/
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
      GLuint i, j;

      paramList->NumParameters = oldNum + sz4;

      memset(&paramList->Parameters[oldNum], 0,
             sz4 * sizeof(struct gl_program_parameter));

      for (i = 0; i < sz4; i++) {
         struct gl_program_parameter *p = paramList->Parameters + oldNum + i;
         p->Name = name ? _mesa_strdup(name) : NULL;
         p->Type = type;
         p->Size = size;
         p->DataType = datatype;
         p->Flags = flags;
         if (values) {
            COPY_4V(paramList->ParameterValues[oldNum + i], values);
            values += 4;
            p->Initialized = GL_TRUE;
         }
         else {
            /* silence valgrind */
            for (j = 0; j < 4; j++)
            	paramList->ParameterValues[oldNum + i][j].f = 0;
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
                          const char *name, const gl_constant_value values[4])
{
   return _mesa_add_parameter(paramList, PROGRAM_NAMED_PARAM, name,
                              4, GL_NONE, values, NULL, 0x0);
                              
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
                         const char *name, const gl_constant_value values[4],
                         GLuint size)
{
   /* first check if this is a duplicate constant */
   GLint pos;
   for (pos = 0; pos < (GLint)paramList->NumParameters; pos++) {
      const gl_constant_value *pvals = paramList->ParameterValues[pos];
      if (pvals[0].u == values[0].u &&
          pvals[1].u == values[1].u &&
          pvals[2].u == values[2].u &&
          pvals[3].u == values[3].u &&
          strcmp(paramList->Parameters[pos].Name, name) == 0) {
         /* Same name and value is already in the param list - reuse it */
         return pos;
      }
   }
   /* not found, add new parameter */
   return _mesa_add_parameter(paramList, PROGRAM_CONSTANT, name,
                              size, GL_NONE, values, NULL, 0x0);
}


/**
 * Add a new unnamed constant to the parameter list.  This will be used
 * when a fragment/vertex program contains something like this:
 *    MOV r, { 0, 1, 2, 3 };
 * If swizzleOut is non-null we'll search the parameter list for an
 * existing instance of the constant which matches with a swizzle.
 *
 * \param paramList  the parameter list
 * \param values  four float values
 * \param swizzleOut  returns swizzle mask for accessing the constant
 * \return index/position of the new parameter in the parameter list.
 */
GLint
_mesa_add_typed_unnamed_constant(struct gl_program_parameter_list *paramList,
                           const gl_constant_value values[4], GLuint size,
                           GLenum datatype, GLuint *swizzleOut)
{
   GLint pos;
   ASSERT(size >= 1);
   ASSERT(size <= 4);

   if (swizzleOut &&
       _mesa_lookup_parameter_constant(paramList, values,
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
            gl_constant_value *pVal = paramList->ParameterValues[pos];
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
                             size, datatype, values, NULL, 0x0);
   if (pos >= 0 && swizzleOut) {
      if (size == 1)
         *swizzleOut = SWIZZLE_XXXX;
      else
         *swizzleOut = SWIZZLE_NOOP;
   }
   return pos;
}

/**
 * Add a new unnamed constant to the parameter list.  This will be used
 * when a fragment/vertex program contains something like this:
 *    MOV r, { 0, 1, 2, 3 };
 * If swizzleOut is non-null we'll search the parameter list for an
 * existing instance of the constant which matches with a swizzle.
 *
 * \param paramList  the parameter list
 * \param values  four float values
 * \param swizzleOut  returns swizzle mask for accessing the constant
 * \return index/position of the new parameter in the parameter list.
 * \sa _mesa_add_typed_unnamed_constant
 */
GLint
_mesa_add_unnamed_constant(struct gl_program_parameter_list *paramList,
                           const gl_constant_value values[4], GLuint size,
                           GLuint *swizzleOut)
{
   return _mesa_add_typed_unnamed_constant(paramList, values, size, GL_NONE,
                                           swizzleOut);
}

/**
 * Add parameter representing a varying variable.
 */
GLint
_mesa_add_varying(struct gl_program_parameter_list *paramList,
                  const char *name, GLuint size, GLenum datatype,
                  GLbitfield flags)
{
   GLint i = _mesa_lookup_parameter_index(paramList, -1, name);
   if (i >= 0 && paramList->Parameters[i].Type == PROGRAM_VARYING) {
      /* already in list */
      return i;
   }
   else {
      /*assert(size == 4);*/
      i = _mesa_add_parameter(paramList, PROGRAM_VARYING, name,
                              size, datatype, NULL, NULL, flags);
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
                              size, datatype, NULL, state, 0x0);
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
   char *name;
   GLint index;

   /* Check if the state reference is already in the list */
   for (index = 0; index < (GLint) paramList->NumParameters; index++) {
      if (!memcmp(paramList->Parameters[index].StateIndexes,
		  stateTokens, STATE_LENGTH * sizeof(gl_state_index))) {
	 return index;
      }
   }

   name = _mesa_program_state_string(stateTokens);
   index = _mesa_add_parameter(paramList, PROGRAM_STATE_VAR, name,
                               size, GL_NONE,
                               NULL, (gl_state_index *) stateTokens, 0x0);
   paramList->StateFlags |= _mesa_program_state_flags(stateTokens);

   /* free name string here since we duplicated it in add_parameter() */
   free(name);

   return index;
}


/**
 * Lookup a parameter value by name in the given parameter list.
 * \return pointer to the float[4] values.
 */
gl_constant_value *
_mesa_lookup_parameter_value(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name)
{
   GLint i = _mesa_lookup_parameter_index(paramList, nameLen, name);
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
	     strcmp(paramList->Parameters[i].Name, name) == 0)
            return i;
      }
   }
   else {
      /* name is not null-terminated, use nameLen */
      for (i = 0; i < (GLint) paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     strncmp(paramList->Parameters[i].Name, name, nameLen) == 0
             && strlen(paramList->Parameters[i].Name) == (size_t)nameLen)
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
 * \param vSize  number of element in v
 * \param posOut  returns the position of the constant, if found
 * \param swizzleOut  returns a swizzle mask describing location of the
 *                    vector elements if found.
 * \return GL_TRUE if found, GL_FALSE if not found
 */
GLboolean
_mesa_lookup_parameter_constant(const struct gl_program_parameter_list *list,
                                const gl_constant_value v[], GLuint vSize,
                                GLint *posOut, GLuint *swizzleOut)
{
   GLuint i;

   assert(vSize >= 1);
   assert(vSize <= 4);

   if (!list) {
      *posOut = -1;
      return GL_FALSE;
   }

   for (i = 0; i < list->NumParameters; i++) {
      if (list->Parameters[i].Type == PROGRAM_CONSTANT) {
         if (!swizzleOut) {
            /* swizzle not allowed */
            GLuint j, match = 0;
            for (j = 0; j < vSize; j++) {
               if (v[j].u == list->ParameterValues[i][j].u)
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
                for (j = 0; j < list->Parameters[i].Size; j++) {
                   if (list->ParameterValues[i][j].u == v[0].u) {
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
                   if (v[j].u == list->ParameterValues[i][j].u) {
                      swz[j] = j;
                      match++;
                   }
                   else {
                      for (k = 0; k < list->Parameters[i].Size; k++) {
                         if (v[j].u == list->ParameterValues[i][k].u) {
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
                                    list->ParameterValues[i], NULL, 0x0);
      ASSERT(j >= 0);
      pCopy = clone->Parameters + j;
      pCopy->Flags = p->Flags;
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
                                param->StateIndexes,
                                param->Flags);
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
 * Count the number of parameters in the last that match the given type.
 */
GLuint
_mesa_num_parameters_of_type(const struct gl_program_parameter_list *list,
                             gl_register_file type)
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
