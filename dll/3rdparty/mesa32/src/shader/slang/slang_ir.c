/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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


#include "imports.h"
#include "context.h"
#include "slang_ir.h"
#include "slang_mem.h"
#include "shader/prog_print.h"


static const slang_ir_info IrInfo[] = {
   /* binary ops */
   { IR_ADD, "IR_ADD", OPCODE_ADD, 4, 2 },
   { IR_SUB, "IR_SUB", OPCODE_SUB, 4, 2 },
   { IR_MUL, "IR_MUL", OPCODE_MUL, 4, 2 },
   { IR_DIV, "IR_DIV", OPCODE_NOP, 0, 2 }, /* XXX broke */
   { IR_DOT4, "IR_DOT_4", OPCODE_DP4, 1, 2 },
   { IR_DOT3, "IR_DOT_3", OPCODE_DP3, 1, 2 },
   { IR_CROSS, "IR_CROSS", OPCODE_XPD, 3, 2 },
   { IR_LRP, "IR_LRP", OPCODE_LRP, 4, 3 },
   { IR_MIN, "IR_MIN", OPCODE_MIN, 4, 2 },
   { IR_MAX, "IR_MAX", OPCODE_MAX, 4, 2 },
   { IR_CLAMP, "IR_CLAMP", OPCODE_NOP, 4, 3 }, /* special case: emit_clamp() */
   { IR_SEQUAL, "IR_SEQUAL", OPCODE_SEQ, 4, 2 },
   { IR_SNEQUAL, "IR_SNEQUAL", OPCODE_SNE, 4, 2 },
   { IR_SGE, "IR_SGE", OPCODE_SGE, 4, 2 },
   { IR_SGT, "IR_SGT", OPCODE_SGT, 4, 2 },
   { IR_SLE, "IR_SLE", OPCODE_SLE, 4, 2 },
   { IR_SLT, "IR_SLT", OPCODE_SLT, 4, 2 },
   { IR_POW, "IR_POW", OPCODE_POW, 1, 2 },
   /* unary ops */
   { IR_I_TO_F, "IR_I_TO_F", OPCODE_NOP, 1, 1 },
   { IR_F_TO_I, "IR_F_TO_I", OPCODE_INT, 4, 1 }, /* 4 floats to 4 ints */
   { IR_EXP, "IR_EXP", OPCODE_EXP, 1, 1 },
   { IR_EXP2, "IR_EXP2", OPCODE_EX2, 1, 1 },
   { IR_LOG2, "IR_LOG2", OPCODE_LG2, 1, 1 },
   { IR_RSQ, "IR_RSQ", OPCODE_RSQ, 1, 1 },
   { IR_RCP, "IR_RCP", OPCODE_RCP, 1, 1 },
   { IR_FLOOR, "IR_FLOOR", OPCODE_FLR, 4, 1 },
   { IR_FRAC, "IR_FRAC", OPCODE_FRC, 4, 1 },
   { IR_ABS, "IR_ABS", OPCODE_ABS, 4, 1 },
   { IR_NEG, "IR_NEG", OPCODE_NOP, 4, 1 }, /* special case: emit_negation() */
   { IR_DDX, "IR_DDX", OPCODE_DDX, 4, 1 },
   { IR_DDY, "IR_DDY", OPCODE_DDY, 4, 1 },
   { IR_SIN, "IR_SIN", OPCODE_SIN, 1, 1 },
   { IR_COS, "IR_COS", OPCODE_COS, 1, 1 },
   { IR_NOISE1, "IR_NOISE1", OPCODE_NOISE1, 1, 1 },
   { IR_NOISE2, "IR_NOISE2", OPCODE_NOISE2, 1, 1 },
   { IR_NOISE3, "IR_NOISE3", OPCODE_NOISE3, 1, 1 },
   { IR_NOISE4, "IR_NOISE4", OPCODE_NOISE4, 1, 1 },

   /* other */
   { IR_SEQ, "IR_SEQ", OPCODE_NOP, 0, 0 },
   { IR_SCOPE, "IR_SCOPE", OPCODE_NOP, 0, 0 },
   { IR_LABEL, "IR_LABEL", OPCODE_NOP, 0, 0 },
   { IR_IF, "IR_IF", OPCODE_NOP, 0, 0 },
   { IR_KILL, "IR_KILL", OPCODE_NOP, 0, 0 },
   { IR_COND, "IR_COND", OPCODE_NOP, 0, 0 },
   { IR_CALL, "IR_CALL", OPCODE_NOP, 0, 0 },
   { IR_MOVE, "IR_MOVE", OPCODE_NOP, 0, 1 },
   { IR_NOT, "IR_NOT", OPCODE_NOP, 1, 1 },
   { IR_VAR, "IR_VAR", OPCODE_NOP, 0, 0 },
   { IR_VAR_DECL, "IR_VAR_DECL", OPCODE_NOP, 0, 0 },
   { IR_TEX, "IR_TEX", OPCODE_TEX, 4, 1 },
   { IR_TEXB, "IR_TEXB", OPCODE_TXB, 4, 1 },
   { IR_TEXP, "IR_TEXP", OPCODE_TXP, 4, 1 },
   { IR_FLOAT, "IR_FLOAT", OPCODE_NOP, 0, 0 }, /* float literal */
   { IR_FIELD, "IR_FIELD", OPCODE_NOP, 0, 0 },
   { IR_ELEMENT, "IR_ELEMENT", OPCODE_NOP, 0, 0 },
   { IR_SWIZZLE, "IR_SWIZZLE", OPCODE_NOP, 0, 0 },
   { IR_NOP, NULL, OPCODE_NOP, 0, 0 }
};


const slang_ir_info *
_slang_ir_info(slang_ir_opcode opcode)
{
   GLuint i;
   for (i = 0; IrInfo[i].IrName; i++) {
      if (IrInfo[i].IrOpcode == opcode) {
	 return IrInfo + i;
      }
   }
   return NULL;
}


static const char *
_slang_ir_name(slang_ir_opcode opcode)
{
   return _slang_ir_info(opcode)->IrName;
}


#if 0 /* no longer needed with mempool */
/**
 * Since many IR nodes might point to the same IR storage info, we need
 * to be careful when deleting things.
 * Before deleting an IR tree, traverse it and do refcounting on the
 * IR storage nodes.  Use the refcount info during delete to free things
 * properly.
 */
static void
_slang_refcount_storage(slang_ir_node *n)
{
   GLuint i;
   if (!n)
      return;
   if (n->Store)
      n->Store->RefCount++;
   for (i = 0; i < 3; i++)
      _slang_refcount_storage(n->Children[i]);
}
#endif


static void
_slang_free_ir(slang_ir_node *n)
{
   GLuint i;
   if (!n)
      return;

#if 0
   if (n->Store) {
      n->Store->RefCount--;
      if (n->Store->RefCount == 0) {
         _slang_free(n->Store);
         n->Store = NULL;
      }
   }
#endif

   for (i = 0; i < 3; i++)
      _slang_free_ir(n->Children[i]);
   /* Do not free n->List since it's a child elsewhere */
   _slang_free(n);
}


/**
 * Recursively free an IR tree.
 */
void
_slang_free_ir_tree(slang_ir_node *n)
{
#if 0
   _slang_refcount_storage(n);
#endif
   _slang_free_ir(n);
}



static const char *
swizzle_string(GLuint swizzle)
{
   static char s[6];
   GLuint i;
   s[0] = '.';
   for (i = 1; i < 5; i++) {
      s[i] = "xyzw"[GET_SWZ(swizzle, i-1)];
   }
   s[i] = 0;
   return s;
}


static const char *
writemask_string(GLuint writemask)
{
   static char s[6];
   GLuint i, j = 0;
   s[j++] = '.';
   for (i = 0; i < 4; i++) {
      if (writemask & (1 << i))
         s[j++] = "xyzw"[i];
   }
   s[j] = 0;
   return s;
}


static const char *
storage_string(const slang_ir_storage *st)
{
   static const char *files[] = {
      "TEMP",
      "LOCAL_PARAM",
      "ENV_PARAM",
      "STATE",
      "INPUT",
      "OUTPUT",
      "NAMED_PARAM",
      "CONSTANT",
      "UNIFORM",
      "WRITE_ONLY",
      "ADDRESS",
      "SAMPLER",
      "UNDEFINED"
   };
   static char s[100];
#if 0
   if (st->Size == 1)
      sprintf(s, "%s[%d]", files[st->File], st->Index);
   else
      sprintf(s, "%s[%d..%d]", files[st->File], st->Index,
              st->Index + st->Size - 1);
#endif
   assert(st->File < (GLint) (sizeof(files) / sizeof(files[0])));
   sprintf(s, "%s[%d]", files[st->File], st->Index);
   return s;
}


static void
spaces(int n)
{
   while (n-- > 0) {
      printf(" ");
   }
}


void
_slang_print_ir_tree(const slang_ir_node *n, int indent)
{
#define IND 0

   if (!n)
      return;
#if !IND
   if (n->Opcode != IR_SEQ)
#else
      printf("%3d:", indent);
#endif
      spaces(indent);

   switch (n->Opcode) {
   case IR_SEQ:
#if IND
      printf("SEQ  at %p\n", (void*) n);
#endif
      assert(n->Children[0]);
      assert(n->Children[1]);
      _slang_print_ir_tree(n->Children[0], indent + IND);
      _slang_print_ir_tree(n->Children[1], indent + IND);
      break;
   case IR_SCOPE:
      printf("NEW SCOPE\n");
      assert(!n->Children[1]);
      _slang_print_ir_tree(n->Children[0], indent + 3);
      break;
   case IR_MOVE:
      printf("MOVE (writemask = %s)\n", writemask_string(n->Writemask));
      _slang_print_ir_tree(n->Children[0], indent+3);
      _slang_print_ir_tree(n->Children[1], indent+3);
      break;
   case IR_LABEL:
      printf("LABEL: %s\n", n->Label->Name);
      break;
   case IR_COND:
      printf("COND\n");
      _slang_print_ir_tree(n->Children[0], indent + 3);
      break;

   case IR_IF:
      printf("IF \n");
      _slang_print_ir_tree(n->Children[0], indent+3);
      spaces(indent);
      printf("THEN\n");
      _slang_print_ir_tree(n->Children[1], indent+3);
      if (n->Children[2]) {
         spaces(indent);
         printf("ELSE\n");
         _slang_print_ir_tree(n->Children[2], indent+3);
      }
      spaces(indent);
      printf("ENDIF\n");
      break;

   case IR_BEGIN_SUB:
      printf("BEGIN_SUB\n");
      break;
   case IR_END_SUB:
      printf("END_SUB\n");
      break;
   case IR_RETURN:
      printf("RETURN\n");
      break;
   case IR_CALL:
      printf("CALL %s\n", n->Label->Name);
      break;

   case IR_LOOP:
      printf("LOOP\n");
      _slang_print_ir_tree(n->Children[0], indent+3);
      if (n->Children[1]) {
         spaces(indent);
         printf("TAIL:\n");
         _slang_print_ir_tree(n->Children[1], indent+3);
      }
      spaces(indent);
      printf("ENDLOOP\n");
      break;
   case IR_CONT:
      printf("CONT\n");
      break;
   case IR_BREAK:
      printf("BREAK\n");
      break;
   case IR_BREAK_IF_TRUE:
      printf("BREAK_IF_TRUE\n");
      _slang_print_ir_tree(n->Children[0], indent+3);
      break;
   case IR_CONT_IF_TRUE:
      printf("CONT_IF_TRUE\n");
      _slang_print_ir_tree(n->Children[0], indent+3);
      break;

   case IR_VAR:
      printf("VAR %s%s at %s  store %p\n",
             (n->Var ? (char *) n->Var->a_name : "TEMP"),
             swizzle_string(n->Store->Swizzle),
             storage_string(n->Store), (void*) n->Store);
      break;
   case IR_VAR_DECL:
      printf("VAR_DECL %s (%p) at %s  store %p\n",
             (n->Var ? (char *) n->Var->a_name : "TEMP"),
             (void*) n->Var, storage_string(n->Store),
             (void*) n->Store);
      break;
   case IR_FIELD:
      printf("FIELD %s of\n", n->Field);
      _slang_print_ir_tree(n->Children[0], indent+3);
      break;
   case IR_FLOAT:
      printf("FLOAT %g %g %g %g\n",
             n->Value[0], n->Value[1], n->Value[2], n->Value[3]);
      break;
   case IR_I_TO_F:
      printf("INT_TO_FLOAT\n");
      _slang_print_ir_tree(n->Children[0], indent+3);
      break;
   case IR_F_TO_I:
      printf("FLOAT_TO_INT\n");
      _slang_print_ir_tree(n->Children[0], indent+3);
      break;
   case IR_SWIZZLE:
      printf("SWIZZLE %s of  (store %p) \n",
             swizzle_string(n->Store->Swizzle), (void*) n->Store);
      _slang_print_ir_tree(n->Children[0], indent + 3);
      break;
   default:
      printf("%s (%p, %p)  (store %p)\n", _slang_ir_name(n->Opcode),
             (void*) n->Children[0], (void*) n->Children[1], (void*) n->Store);
      _slang_print_ir_tree(n->Children[0], indent+3);
      _slang_print_ir_tree(n->Children[1], indent+3);
   }
}
