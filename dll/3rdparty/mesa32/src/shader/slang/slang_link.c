/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 2008  Brian Paul   All Rights Reserved.
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
 * \file slang_link.c
 * GLSL linker
 * \author Brian Paul
 */

#include "main/imports.h"
#include "main/context.h"
#include "main/hash.h"
#include "main/macros.h"
#include "shader/program.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"
#include "shader/prog_uniform.h"
#include "shader/shader_api.h"
#include "slang_link.h"


/**
 * Record a linking error.
 */
static void
link_error(struct gl_shader_program *shProg, const char *msg)
{
   if (shProg->InfoLog) {
      _mesa_free(shProg->InfoLog);
   }
   shProg->InfoLog = _mesa_strdup(msg);
   shProg->LinkStatus = GL_FALSE;
}



/**
 * Linking varying vars involves rearranging varying vars so that the
 * vertex program's output varyings matches the order of the fragment
 * program's input varyings.
 */
static GLboolean
link_varying_vars(struct gl_shader_program *shProg, struct gl_program *prog)
{
   GLuint *map, i, firstVarying, newFile;

   map = (GLuint *) malloc(prog->Varying->NumParameters * sizeof(GLuint));
   if (!map)
      return GL_FALSE;

   for (i = 0; i < prog->Varying->NumParameters; i++) {
      /* see if this varying is in the linked varying list */
      const struct gl_program_parameter *var = prog->Varying->Parameters + i;
      GLint j = _mesa_lookup_parameter_index(shProg->Varying, -1, var->Name);
      if (j >= 0) {
         /* already in list, check size */
         if (var->Size != shProg->Varying->Parameters[j].Size) {
            /* error */
            link_error(shProg, "mismatched varying variable types");
            return GL_FALSE;
         }
      }
      else {
         /* not already in linked list */
         j = _mesa_add_varying(shProg->Varying, var->Name, var->Size);
      }

      /* map varying[i] to varying[j].
       * Note: the loop here takes care of arrays or large (sz>4) vars.
       */
      {
         GLint sz = var->Size;
         while (sz > 0) {
            /*printf("Link varying from %d to %d\n", i, j);*/
            map[i++] = j++;
            sz -= 4;
         }
         i--; /* go back one */
      }
   }


   /* Varying variables are treated like other vertex program outputs
    * (and like other fragment program inputs).  The position of the
    * first varying differs for vertex/fragment programs...
    * Also, replace File=PROGRAM_VARYING with File=PROGRAM_INPUT/OUTPUT.
    */
   if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
      firstVarying = VERT_RESULT_VAR0;
      newFile = PROGRAM_OUTPUT;
   }
   else {
      assert(prog->Target == GL_FRAGMENT_PROGRAM_ARB);
      firstVarying = FRAG_ATTRIB_VAR0;
      newFile = PROGRAM_INPUT;
   }

   /* OK, now scan the program/shader instructions looking for varying vars,
    * replacing the old index with the new index.
    */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      GLuint j;

      if (inst->DstReg.File == PROGRAM_VARYING) {
         inst->DstReg.File = newFile;
         inst->DstReg.Index = map[ inst->DstReg.Index ] + firstVarying;
      }

      for (j = 0; j < 3; j++) {
         if (inst->SrcReg[j].File == PROGRAM_VARYING) {
            inst->SrcReg[j].File = newFile;
            inst->SrcReg[j].Index = map[ inst->SrcReg[j].Index ] + firstVarying;
         }
      }
   }

   free(map);

   /* these will get recomputed before linking is completed */
   prog->InputsRead = 0x0;
   prog->OutputsWritten = 0x0;

   return GL_TRUE;
}


/**
 * Build the shProg->Uniforms list.
 * This is basically a list/index of all uniforms found in either/both of
 * the vertex and fragment shaders.
 */
static void
link_uniform_vars(struct gl_shader_program *shProg,
                  struct gl_program *prog,
                  GLuint *numSamplers)
{
   GLuint samplerMap[MAX_SAMPLERS];
   GLuint i;

   for (i = 0; i < prog->Parameters->NumParameters; i++) {
      const struct gl_program_parameter *p = prog->Parameters->Parameters + i;

      /*
       * XXX FIX NEEDED HERE
       * We should also be adding a uniform if p->Type == PROGRAM_STATE_VAR.
       * For example, modelview matrix, light pos, etc.
       * Also, we need to update the state-var name-generator code to
       * generate GLSL-style names, like "gl_LightSource[0].position".
       * Furthermore, we'll need to fix the state-var's size/datatype info.
       */

      if ((p->Type == PROGRAM_UNIFORM && p->Used) ||
          p->Type == PROGRAM_SAMPLER) {
         _mesa_append_uniform(shProg->Uniforms, p->Name, prog->Target, i);
      }

      if (p->Type == PROGRAM_SAMPLER) {
         /* Allocate a new sampler index */
         GLuint sampNum = *numSamplers;
         GLuint oldSampNum = (GLuint) prog->Parameters->ParameterValues[i][0];
         assert(oldSampNum < MAX_SAMPLERS);
         samplerMap[oldSampNum] = sampNum;
         (*numSamplers)++;
      }
   }


   /* OK, now scan the program/shader instructions looking for sampler vars,
    * replacing the old index with the new index.
    */
   prog->SamplersUsed = 0x0;
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      if (_mesa_is_tex_instruction(inst->Opcode)) {
         /*
         printf("====== remap sampler from %d to %d\n",
                inst->Sampler, map[ inst->Sampler ]);
         */
         /* here, texUnit is really samplerUnit */
         inst->TexSrcUnit = samplerMap[inst->TexSrcUnit];
         prog->SamplerTargets[inst->TexSrcUnit] = inst->TexSrcTarget;
         prog->SamplersUsed |= (1 << inst->TexSrcUnit);
      }
   }

}


/**
 * Resolve binding of generic vertex attributes.
 * For example, if the vertex shader declared "attribute vec4 foobar" we'll
 * allocate a generic vertex attribute for "foobar" and plug that value into
 * the vertex program instructions.
 * But if the user called glBindAttributeLocation(), those bindings will
 * have priority.
 */
static GLboolean
_slang_resolve_attributes(struct gl_shader_program *shProg,
                          const struct gl_program *origProg,
                          struct gl_program *linkedProg)
{
   GLint attribMap[MAX_VERTEX_ATTRIBS];
   GLuint i, j;
   GLbitfield usedAttributes;

   assert(origProg != linkedProg);
   assert(origProg->Target == GL_VERTEX_PROGRAM_ARB);
   assert(linkedProg->Target == GL_VERTEX_PROGRAM_ARB);

   if (!shProg->Attributes)
      shProg->Attributes = _mesa_new_parameter_list();

   if (linkedProg->Attributes) {
      _mesa_free_parameter_list(linkedProg->Attributes);
   }
   linkedProg->Attributes = _mesa_new_parameter_list();


   /* Build a bitmask indicating which attribute indexes have been
    * explicitly bound by the user with glBindAttributeLocation().
    */
   usedAttributes = 0x0;
   for (i = 0; i < shProg->Attributes->NumParameters; i++) {
      GLint attr = shProg->Attributes->Parameters[i].StateIndexes[0];
      usedAttributes |= (1 << attr);
   }

   /* initialize the generic attribute map entries to -1 */
   for (i = 0; i < MAX_VERTEX_ATTRIBS; i++) {
      attribMap[i] = -1;
   }

   /*
    * Scan program for generic attribute references
    */
   for (i = 0; i < linkedProg->NumInstructions; i++) {
      struct prog_instruction *inst = linkedProg->Instructions + i;
      for (j = 0; j < 3; j++) {
         if (inst->SrcReg[j].File == PROGRAM_INPUT &&
             inst->SrcReg[j].Index >= VERT_ATTRIB_GENERIC0) {
            /*
             * OK, we've found a generic vertex attribute reference.
             */
            const GLint k = inst->SrcReg[j].Index - VERT_ATTRIB_GENERIC0;

            GLint attr = attribMap[k];

            if (attr < 0) {
               /* Need to figure out attribute mapping now.
                */
               const char *name = origProg->Attributes->Parameters[k].Name;
               const GLint size = origProg->Attributes->Parameters[k].Size;
               const GLenum type =origProg->Attributes->Parameters[k].DataType;
               GLint index;

               /* See if there's a user-defined attribute binding for
                * this name.
                */
               index = _mesa_lookup_parameter_index(shProg->Attributes,
                                                    -1, name);
               if (index >= 0) {
                  /* Found a user-defined binding */
                  attr = shProg->Attributes->Parameters[index].StateIndexes[0];
               }
               else {
                  /* No user-defined binding, choose our own attribute number.
                   * Start at 1 since generic attribute 0 always aliases
                   * glVertex/position.
                   */
                  for (attr = 1; attr < MAX_VERTEX_ATTRIBS; attr++) {
                     if (((1 << attr) & usedAttributes) == 0)
                        break;
                  }
                  if (attr == MAX_VERTEX_ATTRIBS) {
                     link_error(shProg, "Too many vertex attributes");
                     return GL_FALSE;
                  }

                  /* mark this attribute as used */
                  usedAttributes |= (1 << attr);
               }

               attribMap[k] = attr;

               /* Save the final name->attrib binding so it can be queried
                * with glGetAttributeLocation().
                */
               _mesa_add_attribute(linkedProg->Attributes, name,
                                   size, type, attr);
            }

            assert(attr >= 0);

            /* update the instruction's src reg */
            inst->SrcReg[j].Index = VERT_ATTRIB_GENERIC0 + attr;
         }
      }
   }

   return GL_TRUE;
}


/**
 * Scan program instructions to update the program's NumTemporaries field.
 * Note: this implemenation relies on the code generator allocating
 * temps in increasing order (0, 1, 2, ... ).
 */
static void
_slang_count_temporaries(struct gl_program *prog)
{
   GLuint i, j;
   GLint maxIndex = -1;

   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == PROGRAM_TEMPORARY) {
            if (maxIndex < inst->SrcReg[j].Index)
               maxIndex = inst->SrcReg[j].Index;
         }
         if (inst->DstReg.File == PROGRAM_TEMPORARY) {
            if (maxIndex < (GLint) inst->DstReg.Index)
               maxIndex = inst->DstReg.Index;
         }
      }
   }

   prog->NumTemporaries = (GLuint) (maxIndex + 1);
}


/**
 * Scan program instructions to update the program's InputsRead and
 * OutputsWritten fields.
 */
static void
_slang_update_inputs_outputs(struct gl_program *prog)
{
   GLuint i, j;

   prog->InputsRead = 0x0;
   prog->OutputsWritten = 0x0;

   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == PROGRAM_INPUT) {
            prog->InputsRead |= 1 << inst->SrcReg[j].Index;
         }
      }
      if (inst->DstReg.File == PROGRAM_OUTPUT) {
         prog->OutputsWritten |= 1 << inst->DstReg.Index;
      }
   }
}


/** cast wrapper */
static struct gl_vertex_program *
vertex_program(struct gl_program *prog)
{
   assert(prog->Target == GL_VERTEX_PROGRAM_ARB);
   return (struct gl_vertex_program *) prog;
}


/** cast wrapper */
static struct gl_fragment_program *
fragment_program(struct gl_program *prog)
{
   assert(prog->Target == GL_FRAGMENT_PROGRAM_ARB);
   return (struct gl_fragment_program *) prog;
}


/**
 * Shader linker.  Currently:
 *
 * 1. The last attached vertex shader and fragment shader are linked.
 * 2. Varying vars in the two shaders are combined so their locations
 *    agree between the vertex and fragment stages.  They're treated as
 *    vertex program output attribs and as fragment program input attribs.
 * 3. The vertex and fragment programs are cloned and modified to update
 *    src/dst register references so they use the new, linked varying
 *    storage locations.
 */
void
_slang_link(GLcontext *ctx,
            GLhandleARB programObj,
            struct gl_shader_program *shProg)
{
   const struct gl_vertex_program *vertProg;
   const struct gl_fragment_program *fragProg;
   GLuint numSamplers = 0;
   GLuint i;

   _mesa_clear_shader_program_data(ctx, shProg);

   /* check that all programs compiled successfully */
   for (i = 0; i < shProg->NumShaders; i++) {
      if (!shProg->Shaders[i]->CompileStatus) {
         link_error(shProg, "linking with uncompiled shader\n");
         return;
      }
   }

   shProg->Uniforms = _mesa_new_uniform_list();
   shProg->Varying = _mesa_new_parameter_list();

   /**
    * Find attached vertex, fragment shaders defining main()
    */
   vertProg = NULL;
   fragProg = NULL;
   for (i = 0; i < shProg->NumShaders; i++) {
      struct gl_shader *shader = shProg->Shaders[i];
      if (shader->Type == GL_VERTEX_SHADER && shader->Main)
         vertProg = vertex_program(shader->Program);
      else if (shader->Type == GL_FRAGMENT_SHADER && shader->Main)
         fragProg = fragment_program(shader->Program);
      else
         _mesa_problem(ctx, "unexpected shader target in slang_link()");
   }

#if FEATURE_es2_glsl
   /* must have both a vertex and fragment program for ES2 */
   if (!vertProg) {
      link_error(shProg, "missing vertex shader\n");
      return;
   }
   if (!fragProg) {
      link_error(shProg, "missing fragment shader\n");
      return;
   }
#endif

   /*
    * Make copies of the vertex/fragment programs now since we'll be
    * changing src/dst registers after merging the uniforms and varying vars.
    */
   _mesa_reference_vertprog(ctx, &shProg->VertexProgram, NULL);
   if (vertProg) {
      struct gl_vertex_program *linked_vprog =
         vertex_program(_mesa_clone_program(ctx, &vertProg->Base));
      shProg->VertexProgram = linked_vprog; /* refcount OK */
      ASSERT(shProg->VertexProgram->Base.RefCount == 1);
   }

   _mesa_reference_fragprog(ctx, &shProg->FragmentProgram, NULL);
   if (fragProg) {
      struct gl_fragment_program *linked_fprog = 
         fragment_program(_mesa_clone_program(ctx, &fragProg->Base));
      shProg->FragmentProgram = linked_fprog; /* refcount OK */
      ASSERT(shProg->FragmentProgram->Base.RefCount == 1);
   }

   /* link varying vars */
   if (shProg->VertexProgram) {
      if (!link_varying_vars(shProg, &shProg->VertexProgram->Base))
         return;
   }
   if (shProg->FragmentProgram) {
      if (!link_varying_vars(shProg, &shProg->FragmentProgram->Base))
         return;
   }

   /* link uniform vars */
   if (shProg->VertexProgram)
      link_uniform_vars(shProg, &shProg->VertexProgram->Base, &numSamplers);
   if (shProg->FragmentProgram)
      link_uniform_vars(shProg, &shProg->FragmentProgram->Base, &numSamplers);

   /*_mesa_print_uniforms(shProg->Uniforms);*/

   if (shProg->VertexProgram) {
      if (!_slang_resolve_attributes(shProg, &vertProg->Base,
                                     &shProg->VertexProgram->Base)) {
         return;
      }
   }

   if (shProg->VertexProgram) {
      _slang_update_inputs_outputs(&shProg->VertexProgram->Base);
      _slang_count_temporaries(&shProg->VertexProgram->Base);
      if (!(shProg->VertexProgram->Base.OutputsWritten & (1 << VERT_RESULT_HPOS))) {
         /* the vertex program did not compute a vertex position */
         link_error(shProg,
                    "gl_Position was not written by vertex shader\n");
         return;
      }
   }
   if (shProg->FragmentProgram) {
      _slang_count_temporaries(&shProg->FragmentProgram->Base);
      _slang_update_inputs_outputs(&shProg->FragmentProgram->Base);
   }

   /* Check that all the varying vars needed by the fragment shader are
    * actually produced by the vertex shader.
    */
   if (shProg->FragmentProgram) {
      const GLbitfield varyingRead
         = shProg->FragmentProgram->Base.InputsRead >> FRAG_ATTRIB_VAR0;
      const GLbitfield varyingWritten = shProg->VertexProgram ?
         shProg->VertexProgram->Base.OutputsWritten >> VERT_RESULT_VAR0 : 0x0;
      if ((varyingRead & varyingWritten) != varyingRead) {
         link_error(shProg,
          "Fragment program using varying vars not written by vertex shader\n");
         return;
      }         
   }


   if (fragProg && shProg->FragmentProgram) {
      /* notify driver that a new fragment program has been compiled/linked */
      ctx->Driver.ProgramStringNotify(ctx, GL_FRAGMENT_PROGRAM_ARB,
                                      &shProg->FragmentProgram->Base);
#if 0
      printf("************** original fragment program\n");
      _mesa_print_program(&fragProg->Base);
      _mesa_print_program_parameters(ctx, &fragProg->Base);
#endif
#if 0
      printf("************** linked fragment prog\n");
      _mesa_print_program(&shProg->FragmentProgram->Base);
      _mesa_print_program_parameters(ctx, &shProg->FragmentProgram->Base);
#endif
   }

   if (vertProg && shProg->VertexProgram) {
      /* notify driver that a new vertex program has been compiled/linked */
      ctx->Driver.ProgramStringNotify(ctx, GL_VERTEX_PROGRAM_ARB,
                                      &shProg->VertexProgram->Base);
#if 0
      printf("************** original vertex program\n");
      _mesa_print_program(&vertProg->Base);
      _mesa_print_program_parameters(ctx, &vertProg->Base);
#endif
#if 0
      printf("************** linked vertex prog\n");
      _mesa_print_program(&shProg->VertexProgram->Base);
      _mesa_print_program_parameters(ctx, &shProg->VertexProgram->Base);
#endif
   }

   shProg->LinkStatus = (shProg->VertexProgram || shProg->FragmentProgram);
}

