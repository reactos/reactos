/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2007  Brian Paul   All Rights Reserved.
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

#include "imports.h"
#include "context.h"
#include "hash.h"
#include "macros.h"
#include "program.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_print.h"
#include "prog_statevars.h"
#include "shader_api.h"
#include "slang_link.h"




static GLboolean
link_varying_vars(struct gl_shader_program *shProg, struct gl_program *prog)
{
   GLuint *map, i, firstVarying, newFile;
   GLbitfield varsWritten, varsRead;

   map = (GLuint *) malloc(prog->Varying->NumParameters * sizeof(GLuint));
   if (!map)
      return GL_FALSE;

   for (i = 0; i < prog->Varying->NumParameters; i++) {
      /* see if this varying is in the linked varying list */
      const struct gl_program_parameter *var
         = prog->Varying->Parameters + i;

      GLint j = _mesa_lookup_parameter_index(shProg->Varying, -1, var->Name);
      if (j >= 0) {
         /* already in list, check size */
         if (var->Size != shProg->Varying->Parameters[j].Size) {
            /* error */
            return GL_FALSE;
         }
      }
      else {
         /* not already in linked list */
         j = _mesa_add_varying(shProg->Varying, var->Name, var->Size);
      }
      ASSERT(j >= 0);

      map[i] = j;
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

   /* keep track of which varying vars we read and write */
   varsWritten = varsRead = 0x0;

   /* OK, now scan the program/shader instructions looking for varying vars,
    * replacing the old index with the new index.
    */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      GLuint j;

      if (inst->DstReg.File == PROGRAM_VARYING) {
         inst->DstReg.File = newFile;
         inst->DstReg.Index = map[ inst->DstReg.Index ] + firstVarying;
         varsWritten |= (1 << inst->DstReg.Index);
      }

      for (j = 0; j < 3; j++) {
         if (inst->SrcReg[j].File == PROGRAM_VARYING) {
            inst->SrcReg[j].File = newFile;
            inst->SrcReg[j].Index = map[ inst->SrcReg[j].Index ] + firstVarying;
            varsRead |= (1 << inst->SrcReg[j].Index);
         }
      }
   }

   if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
      prog->OutputsWritten |= varsWritten;
      /*printf("VERT OUTPUTS: 0x%x \n", varsWritten);*/
   }
   else {
      assert(prog->Target == GL_FRAGMENT_PROGRAM_ARB);
      prog->InputsRead |= varsRead;
      /*printf("FRAG INPUTS: 0x%x\n", varsRead);*/
   }

   free(map);

   return GL_TRUE;
}


static GLboolean
is_uniform(GLuint file)
{
   return (file == PROGRAM_ENV_PARAM ||
           file == PROGRAM_STATE_VAR ||
           file == PROGRAM_NAMED_PARAM ||
           file == PROGRAM_CONSTANT ||
           file == PROGRAM_SAMPLER ||
           file == PROGRAM_UNIFORM);
}


static GLboolean
link_uniform_vars(struct gl_shader_program *shProg, struct gl_program *prog)
{
   GLuint *map, i;

#if 0
   printf("================ pre link uniforms ===============\n");
   _mesa_print_parameter_list(shProg->Uniforms);
#endif

   map = (GLuint *) malloc(prog->Parameters->NumParameters * sizeof(GLuint));
   if (!map)
      return GL_FALSE;

   for (i = 0; i < prog->Parameters->NumParameters; /* incr below*/) {
      /* see if this uniform is in the linked uniform list */
      const struct gl_program_parameter *p = prog->Parameters->Parameters + i;
      const GLfloat *pVals = prog->Parameters->ParameterValues[i];
      GLint j;
      GLint size;

      /* sanity check */
      assert(is_uniform(p->Type));

      if (p->Name) {
         j = _mesa_lookup_parameter_index(shProg->Uniforms, -1, p->Name);
      }
      else {
         /*GLuint swizzle;*/
         ASSERT(p->Type == PROGRAM_CONSTANT);
         if (_mesa_lookup_parameter_constant(shProg->Uniforms, pVals,
                                             p->Size, &j, NULL)) {
            assert(j >= 0);
         }
         else {
            j = -1;
         }
      }

      if (j >= 0) {
         /* already in list, check size XXX check this */
#if 0
         assert(p->Size == shProg->Uniforms->Parameters[j].Size);
#endif
      }
      else {
         /* not already in linked list */
         switch (p->Type) {
         case PROGRAM_ENV_PARAM:
            j = _mesa_add_named_parameter(shProg->Uniforms, p->Name, pVals);
            break;
         case PROGRAM_CONSTANT:
            j = _mesa_add_named_constant(shProg->Uniforms, p->Name, pVals, p->Size);
            break;
         case PROGRAM_STATE_VAR:
            j = _mesa_add_state_reference(shProg->Uniforms, p->StateIndexes);
            break;
         case PROGRAM_UNIFORM:
            j = _mesa_add_uniform(shProg->Uniforms, p->Name, p->Size, p->DataType);
            break;
         case PROGRAM_SAMPLER:
            j = _mesa_add_sampler(shProg->Uniforms, p->Name, p->DataType);
            break;
         default:
            _mesa_problem(NULL, "bad parameter type in link_uniform_vars()");
            return GL_FALSE;
         }
      }

      ASSERT(j >= 0);

      size = p->Size;
      while (size > 0) {
         map[i] = j;
         i++;
         j++;
         size -= 4;
      }

   }

#if 0
   printf("================ post link uniforms ===============\n");
   _mesa_print_parameter_list(shProg->Uniforms);
#endif

#if 0
   {
      GLuint i;
      for (i = 0; i < prog->Parameters->NumParameters; i++) {
         printf("map[%d] = %d\n", i, map[i]);
      }
      _mesa_print_parameter_list(shProg->Uniforms);
   }
#endif

   /* OK, now scan the program/shader instructions looking for uniform vars,
    * replacing the old index with the new index.
    */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      GLuint j;

      if (is_uniform(inst->DstReg.File)) {
         inst->DstReg.Index = map[ inst->DstReg.Index ];
      }

      for (j = 0; j < 3; j++) {
         if (is_uniform(inst->SrcReg[j].File)) {
            inst->SrcReg[j].Index = map[ inst->SrcReg[j].Index ];
         }
      }

      if (inst->Opcode == OPCODE_TEX ||
          inst->Opcode == OPCODE_TXB ||
          inst->Opcode == OPCODE_TXP) {
         /*
         printf("====== remap sampler from %d to %d\n",
                inst->Sampler, map[ inst->Sampler ]);
         */
         inst->Sampler = map[ inst->Sampler ];
      }
   }

   free(map);

   return GL_TRUE;
}


/**
 * Resolve binding of generic vertex attributes.
 * For example, if the vertex shader declared "attribute vec4 foobar" we'll
 * allocate a generic vertex attribute for "foobar" and plug that value into
 * the vertex program instructions.
 */
static GLboolean
_slang_resolve_attributes(struct gl_shader_program *shProg,
                          struct gl_program *prog)
{
   GLuint i, j;
   GLbitfield usedAttributes;
   GLint size = 4; /* XXX fix */

   assert(prog->Target == GL_VERTEX_PROGRAM_ARB);

   if (!shProg->Attributes)
      shProg->Attributes = _mesa_new_parameter_list();

   /* Build a bitmask indicating which attribute indexes have been
    * explicitly bound by the user with glBindAttributeLocation().
    */
   usedAttributes = 0x0;
   for (i = 0; i < shProg->Attributes->NumParameters; i++) {
      GLint attr = shProg->Attributes->Parameters[i].StateIndexes[0];
      usedAttributes |= attr;
   }

   /*
    * Scan program for generic attribute references
    */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      for (j = 0; j < 3; j++) {
         if (inst->SrcReg[j].File == PROGRAM_INPUT &&
             inst->SrcReg[j].Index >= VERT_ATTRIB_GENERIC0) {
            /* this is a generic attrib */
            const GLint k = inst->SrcReg[j].Index - VERT_ATTRIB_GENERIC0;
            const char *name = prog->Attributes->Parameters[k].Name;
            /* See if this attrib name is in the program's attribute list
             * (i.e. was bound by the user).
             */
            GLint index = _mesa_lookup_parameter_index(shProg->Attributes,
                                                          -1, name);
            GLint attr;
            if (index >= 0) {
               /* found, user must have specified a binding */
               attr = shProg->Attributes->Parameters[index].StateIndexes[0];
            }
            else {
               /* Not found, choose our own attribute number.
                * Start at 1 since generic attribute 0 always aliases
                * glVertex/position.
                */
               for (attr = 1; attr < MAX_VERTEX_ATTRIBS; attr++) {
                  if (((1 << attr) & usedAttributes) == 0) {
                     usedAttributes |= (1 << attr);
                     break;
                  }
               }
               if (attr == MAX_VERTEX_ATTRIBS) {
                  /* too many!  XXX record error log */
                  return GL_FALSE;
               }
               _mesa_add_attribute(shProg->Attributes, name, size, attr);
            }

            inst->SrcReg[j].Index = VERT_ATTRIB_GENERIC0 + attr;
         }
      }
   }
   return GL_TRUE;
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


/**
 * Scan a vertex program looking for instances of
 * (PROGRAM_INPUT, VERT_ATTRIB_GENERIC0 + oldAttrib) and replace with
 * (PROGRAM_INPUT, VERT_ATTRIB_GENERIC0 + newAttrib).
 * This is used when the user calls glBindAttribLocation on an already linked
 * shader program.
 */
void
_slang_remap_attribute(struct gl_program *prog, GLuint oldAttrib, GLuint newAttrib)
{
   GLuint i, j;

   assert(prog->Target == GL_VERTEX_PROGRAM_ARB);

   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      for (j = 0; j < 3; j++) {
         if (inst->SrcReg[j].File == PROGRAM_INPUT) {
            if (inst->SrcReg[j].Index == VERT_ATTRIB_GENERIC0 + oldAttrib) {
               inst->SrcReg[j].Index = VERT_ATTRIB_GENERIC0 + newAttrib;
            }
         }
      }
   }

   _slang_update_inputs_outputs(prog);
}



/**
 * Scan program for texture instructions, lookup sampler/uniform's value
 * to determine which texture unit to use.
 * Also, update the program's TexturesUsed[] array.
 */
void
_slang_resolve_samplers(struct gl_shader_program *shProg,
                        struct gl_program *prog)
{
   GLuint i;

   for (i = 0; i < MAX_TEXTURE_IMAGE_UNITS; i++)
      prog->TexturesUsed[i] = 0;

   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      if (inst->Opcode == OPCODE_TEX ||
          inst->Opcode == OPCODE_TXB ||
          inst->Opcode == OPCODE_TXP) {
         GLint sampleUnit = (GLint) shProg->Uniforms->ParameterValues[inst->Sampler][0];
         assert(sampleUnit < MAX_TEXTURE_IMAGE_UNITS);
         inst->TexSrcUnit = sampleUnit;

         prog->TexturesUsed[inst->TexSrcUnit] |= (1 << inst->TexSrcTarget);
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
 * Shader linker.  Currently:
 *
 * 1. The last attached vertex shader and fragment shader are linked.
 * 2. Varying vars in the two shaders are combined so their locations
 *    agree between the vertex and fragment stages.  They're treated as
 *    vertex program output attribs and as fragment program input attribs.
 * 3. Uniform vars (including state references, constants, etc) from the
 *    vertex and fragment shaders are merged into one group.  Recall that
 *    GLSL uniforms are shared by all linked shaders.
 * 4. The vertex and fragment programs are cloned and modified to update
 *    src/dst register references so they use the new, linked uniform/
 *    varying storage locations.
 */
void
_slang_link(GLcontext *ctx,
            GLhandleARB programObj,
            struct gl_shader_program *shProg)
{
   const struct gl_vertex_program *vertProg;
   const struct gl_fragment_program *fragProg;
   GLuint i;

   _mesa_clear_shader_program_data(ctx, shProg);

   shProg->Uniforms = _mesa_new_parameter_list();
   shProg->Varying = _mesa_new_parameter_list();

   /**
    * Find attached vertex shader, fragment shader
    */
   vertProg = NULL;
   fragProg = NULL;
   for (i = 0; i < shProg->NumShaders; i++) {
      if (shProg->Shaders[i]->Type == GL_VERTEX_SHADER)
         vertProg = vertex_program(shProg->Shaders[i]->Programs[0]);
      else if (shProg->Shaders[i]->Type == GL_FRAGMENT_SHADER)
         fragProg = fragment_program(shProg->Shaders[i]->Programs[0]);
      else
         _mesa_problem(ctx, "unexpected shader target in slang_link()");
   }

   /*
    * Make copies of the vertex/fragment programs now since we'll be
    * changing src/dst registers after merging the uniforms and varying vars.
    */
   if (vertProg) {
      shProg->VertexProgram
         = vertex_program(_mesa_clone_program(ctx, &vertProg->Base));
   }
   else {
      shProg->VertexProgram = NULL;
   }

   if (fragProg) {
      shProg->FragmentProgram
         = fragment_program(_mesa_clone_program(ctx, &fragProg->Base));
   }
   else {
      shProg->FragmentProgram = NULL;
   }

   if (shProg->VertexProgram)
      link_varying_vars(shProg, &shProg->VertexProgram->Base);
   if (shProg->FragmentProgram)
      link_varying_vars(shProg, &shProg->FragmentProgram->Base);

   if (shProg->VertexProgram)
      link_uniform_vars(shProg, &shProg->VertexProgram->Base);
   if (shProg->FragmentProgram)
      link_uniform_vars(shProg, &shProg->FragmentProgram->Base);

   /* The vertex and fragment programs share a common set of uniforms now */
   if (shProg->VertexProgram) {
      _mesa_free_parameter_list(shProg->VertexProgram->Base.Parameters);
      shProg->VertexProgram->Base.Parameters = shProg->Uniforms;
   }
   if (shProg->FragmentProgram) {
      _mesa_free_parameter_list(shProg->FragmentProgram->Base.Parameters);
      shProg->FragmentProgram->Base.Parameters = shProg->Uniforms;
   }

   if (shProg->VertexProgram) {
      _slang_resolve_samplers(shProg, &shProg->VertexProgram->Base);
   }
   if (shProg->FragmentProgram) {
      _slang_resolve_samplers(shProg, &shProg->FragmentProgram->Base);
   }

   if (shProg->VertexProgram) {
      if (!_slang_resolve_attributes(shProg, &shProg->VertexProgram->Base)) {
         /*goto cleanup;*/
         _mesa_problem(ctx, "_slang_resolve_attributes() failed");
         return;
      }
   }

   if (shProg->VertexProgram) {
      _slang_update_inputs_outputs(&shProg->VertexProgram->Base);
      if (!(shProg->VertexProgram->Base.OutputsWritten & (1 << VERT_RESULT_HPOS))) {
         /* the vertex program did not compute a vertex position */
         link_error(shProg,
                    "gl_Position was not written by vertex shader\n");
         return;
      }
   }
   if (shProg->FragmentProgram)
      _slang_update_inputs_outputs(&shProg->FragmentProgram->Base);

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

