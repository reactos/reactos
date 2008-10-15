
/**
 * Dump/print a slang_operation tree
 */


#include "main/imports.h"
#include "slang_compile.h"
#include "slang_print.h"


static void
spaces(int n)
{
   while (n--)
      printf(" ");
}


static void
print_type(const slang_fully_specified_type *t)
{
   switch (t->qualifier) {
   case SLANG_QUAL_NONE:
      /*printf("");*/
      break;
   case SLANG_QUAL_CONST:
      printf("const ");
      break;
   case SLANG_QUAL_ATTRIBUTE:
      printf("attrib ");
      break;
   case SLANG_QUAL_VARYING:
      printf("varying ");
      break;
   case SLANG_QUAL_UNIFORM:
      printf("uniform ");
      break;
   case SLANG_QUAL_OUT:
      printf("output ");
      break;
   case SLANG_QUAL_INOUT:
      printf("inout ");
      break;
   case SLANG_QUAL_FIXEDOUTPUT:
      printf("fixedoutput");
      break;
   case SLANG_QUAL_FIXEDINPUT:
      printf("fixedinput");
      break;
   default:
      printf("unknown qualifer!");
   }

   switch (t->specifier.type) {
   case SLANG_SPEC_VOID:
      printf("void");
      break;
   case SLANG_SPEC_BOOL:
      printf("bool");
      break;
   case SLANG_SPEC_BVEC2:
      printf("bvec2");
      break;
   case SLANG_SPEC_BVEC3:
      printf("bvec3");
      break;
   case SLANG_SPEC_BVEC4:
      printf("bvec4");
      break;
   case SLANG_SPEC_INT:
      printf("int");
      break;
   case SLANG_SPEC_IVEC2:
      printf("ivec2");
      break;
   case SLANG_SPEC_IVEC3:
      printf("ivec3");
      break;
   case SLANG_SPEC_IVEC4:
      printf("ivec4");
      break;
   case SLANG_SPEC_FLOAT:
      printf("float");
      break;
   case SLANG_SPEC_VEC2:
      printf("vec2");
      break;
   case SLANG_SPEC_VEC3:
      printf("vec3");
      break;
   case SLANG_SPEC_VEC4:
      printf("vec4");
      break;
   case SLANG_SPEC_MAT2:
      printf("mat2");
      break;
   case SLANG_SPEC_MAT3:
      printf("mat3");
      break;
   case SLANG_SPEC_MAT4:
      printf("mat4");
      break;
   case SLANG_SPEC_MAT23:
      printf("mat2x3");
      break;
   case SLANG_SPEC_MAT32:
      printf("mat3x2");
      break;
   case SLANG_SPEC_MAT24:
      printf("mat2x4");
      break;
   case SLANG_SPEC_MAT42:
      printf("mat4x2");
      break;
   case SLANG_SPEC_MAT34:
      printf("mat3x4");
      break;
   case SLANG_SPEC_MAT43:
      printf("mat4x3");
      break;
   case SLANG_SPEC_SAMPLER1D:
      printf("sampler1D");
      break;
   case SLANG_SPEC_SAMPLER2D:
      printf("sampler2D");
      break;
   case SLANG_SPEC_SAMPLER3D:
      printf("sampler3D");
      break;
   case SLANG_SPEC_SAMPLERCUBE:
      printf("samplerCube");
      break;
   case SLANG_SPEC_SAMPLER1DSHADOW:
      printf("sampler1DShadow");
      break;
   case SLANG_SPEC_SAMPLER2DSHADOW:
      printf("sampler2DShadow");
      break;
   case SLANG_SPEC_STRUCT:
      printf("struct");
      break;
   case SLANG_SPEC_ARRAY:
      printf("array");
      break;
   default:
      printf("unknown type");
   }
   /*printf("\n");*/
}


static void
print_variable(const slang_variable *v, int indent)
{
   spaces(indent);
   printf("VAR ");
   print_type(&v->type);
   printf(" %s (at %p)", (char *) v->a_name, (void *) v);
   if (v->initializer) {
      printf(" :=\n");
      slang_print_tree(v->initializer, indent + 3);
   }
   else {
      printf(";\n");
   }
}


static void
print_binary(const slang_operation *op, const char *oper, int indent)
{
   assert(op->num_children == 2);
#if 0
   printf("binary at %p locals=%p outer=%p\n",
          (void *) op,
          (void *) op->locals,
          (void *) op->locals->outer_scope);
#endif
   slang_print_tree(&op->children[0], indent + 3);
   spaces(indent);
   printf("%s at %p locals=%p outer=%p\n",
          oper, (void *) op, (void *) op->locals,
          (void *) op->locals->outer_scope);
   slang_print_tree(&op->children[1], indent + 3);
}


static void
print_generic2(const slang_operation *op, const char *oper,
               const char *s, int indent)
{
   GLuint i;
   if (oper) {
      spaces(indent);
      printf("%s %s at %p locals=%p outer=%p\n",
             oper, s, (void *) op, (void *) op->locals, 
             (void *) op->locals->outer_scope);
   }
   for (i = 0; i < op->num_children; i++) {
      spaces(indent);
      printf("//child %u of %u:\n", i, op->num_children);
      slang_print_tree(&op->children[i], indent);
   }
}

static void
print_generic(const slang_operation *op, const char *oper, int indent)
{
   print_generic2(op, oper, "", indent);
}


static const slang_variable_scope *
find_scope(const slang_variable_scope *s, slang_atom name)
{
   GLuint i;
   for (i = 0; i < s->num_variables; i++) {
      if (s->variables[i]->a_name == name)
         return s;
   }
   if (s->outer_scope)
      return find_scope(s->outer_scope, name);
   else
      return NULL;
}

static const slang_variable *
find_var(const slang_variable_scope *s, slang_atom name)
{
   GLuint i;
   for (i = 0; i < s->num_variables; i++) {
      if (s->variables[i]->a_name == name)
         return s->variables[i];
   }
   if (s->outer_scope)
      return find_var(s->outer_scope, name);
   else
      return NULL;
}


void
slang_print_tree(const slang_operation *op, int indent)
{
   GLuint i;

   switch (op->type) {

   case SLANG_OPER_NONE:
      spaces(indent);
      printf("SLANG_OPER_NONE\n");
      break;

   case SLANG_OPER_BLOCK_NO_NEW_SCOPE:
      spaces(indent);
      printf("{ locals=%p  outer=%p\n", (void*)op->locals, (void*)op->locals->outer_scope);
      print_generic(op, NULL, indent+3);
      spaces(indent);
      printf("}\n");
      break;

   case SLANG_OPER_BLOCK_NEW_SCOPE:
      spaces(indent);
      printf("{{ // new scope  locals=%p outer=%p: ",
             (void *) op->locals,
             (void *) op->locals->outer_scope);
      for (i = 0; i < op->locals->num_variables; i++) {
         printf("%s ", (char *) op->locals->variables[i]->a_name);
      }
      printf("\n");
      print_generic(op, NULL, indent+3);
      spaces(indent);
      printf("}}\n");
      break;

   case SLANG_OPER_VARIABLE_DECL:
      assert(op->num_children == 0 || op->num_children == 1);
      {
         slang_variable *v;
         v = _slang_locate_variable(op->locals, op->a_id, GL_TRUE);
         if (v) {
            const slang_variable_scope *scope;
            spaces(indent);
            printf("DECL (locals=%p outer=%p) ", (void*)op->locals, (void*) op->locals->outer_scope);
            print_type(&v->type);
            printf(" %s (%p)", (char *) op->a_id,
                   (void *) find_var(op->locals, op->a_id));

            scope = find_scope(op->locals, op->a_id);
            printf(" (in scope %p) ", (void *) scope);
            assert(scope);
            if (op->num_children == 1) {
               printf(" :=\n");
               slang_print_tree(&op->children[0], indent + 3);
            }
            else if (v->initializer) {
               printf(" := INITIALIZER\n");
               slang_print_tree(v->initializer, indent + 3);
            }
            else {
               printf(";\n");
            }
            /*
            spaces(indent);
            printf("TYPE: ");
            print_type(&v->type);
            spaces(indent);
            printf("ADDR: %d  size: %d\n", v->address, v->size);
            */
         }
         else {
            spaces(indent);
            printf("DECL %s (anonymous variable!!!!)\n", (char *) op->a_id);
         }
      }
      break;

   case SLANG_OPER_ASM:
      spaces(indent);
      printf("ASM: %s at %p locals=%p outer=%p\n",
             (char *) op->a_id,
             (void *) op,
             (void *) op->locals,
             (void *) op->locals->outer_scope);
      print_generic(op, "ASM", indent+3);
      break;

   case SLANG_OPER_BREAK:
      spaces(indent);
      printf("BREAK\n");
      break;

   case SLANG_OPER_CONTINUE:
      spaces(indent);
      printf("CONTINUE\n");
      break;

   case SLANG_OPER_DISCARD:
      spaces(indent);
      printf("DISCARD\n");
      break;

   case SLANG_OPER_RETURN:
      spaces(indent);
      printf("RETURN\n");
      if (op->num_children > 0)
         slang_print_tree(&op->children[0], indent + 3);
      break;

   case SLANG_OPER_LABEL:
      spaces(indent);
      printf("LABEL %s\n", (char *) op->a_id);
      break;

   case SLANG_OPER_EXPRESSION:
      spaces(indent);
      printf("EXPR:  locals=%p outer=%p\n",
             (void *) op->locals,
             (void *) op->locals->outer_scope);
      /*print_generic(op, "SLANG_OPER_EXPRESSION", indent);*/
      slang_print_tree(&op->children[0], indent + 3);
      break;

   case SLANG_OPER_IF:
      spaces(indent);
      printf("IF\n");
      slang_print_tree(&op->children[0], indent + 3);
      spaces(indent);
      printf("THEN\n");
      slang_print_tree(&op->children[1], indent + 3);
      spaces(indent);
      printf("ELSE\n");
      slang_print_tree(&op->children[2], indent + 3);
      spaces(indent);
      printf("ENDIF\n");
      break;

   case SLANG_OPER_WHILE:
      assert(op->num_children == 2);
      spaces(indent);
      printf("WHILE cond:\n");
      slang_print_tree(&op->children[0], indent + 3);
      spaces(indent);
      printf("WHILE body:\n");
      slang_print_tree(&op->children[1], indent + 3);
      break;

   case SLANG_OPER_DO:
      spaces(indent);
      printf("DO body:\n");
      slang_print_tree(&op->children[0], indent + 3);
      spaces(indent);
      printf("DO cond:\n");
      slang_print_tree(&op->children[1], indent + 3);
      break;

   case SLANG_OPER_FOR:
      spaces(indent);
      printf("FOR init:\n");
      slang_print_tree(&op->children[0], indent + 3);
      spaces(indent);
      printf("FOR while:\n");
      slang_print_tree(&op->children[1], indent + 3);
      spaces(indent);
      printf("FOR step:\n");
      slang_print_tree(&op->children[2], indent + 3);
      spaces(indent);
      printf("FOR body:\n");
      slang_print_tree(&op->children[3], indent + 3);
      spaces(indent);
      printf("ENDFOR\n");
      /*
      print_generic(op, "FOR", indent + 3);
      */
      break;

   case SLANG_OPER_VOID:
      spaces(indent);
      printf("(oper-void)\n");
      break;

   case SLANG_OPER_LITERAL_BOOL:
      spaces(indent);
      printf("LITERAL (");
      for (i = 0; i < op->literal_size; i++)
         printf("%s ", op->literal[0] ? "TRUE" : "FALSE");
      printf(")\n");

      break;

   case SLANG_OPER_LITERAL_INT:
      spaces(indent);
      printf("LITERAL (");
      for (i = 0; i < op->literal_size; i++)
         printf("%d ", (int) op->literal[i]);
      printf(")\n");
      break;

   case SLANG_OPER_LITERAL_FLOAT:
      spaces(indent);
      printf("LITERAL (");
      for (i = 0; i < op->literal_size; i++)
         printf("%f ", op->literal[i]);
      printf(")\n");
      break;

   case SLANG_OPER_IDENTIFIER:
      {
         const slang_variable_scope *scope;
         spaces(indent);
         if (op->var && op->var->a_name) {
            scope = find_scope(op->locals, op->var->a_name);
            printf("VAR %s  (in scope %p)\n", (char *) op->var->a_name,
                   (void *) scope);
            assert(scope);
         }
         else {
            scope = find_scope(op->locals, op->a_id);
            printf("VAR' %s  (in scope %p) locals=%p outer=%p\n",
                   (char *) op->a_id,
                   (void *) scope,
                   (void *) op->locals,
                   (void *) op->locals->outer_scope);
            assert(scope);
         }
      }
      break;

   case SLANG_OPER_SEQUENCE:
      print_generic(op, "COMMA-SEQ", indent+3);
      break;

   case SLANG_OPER_ASSIGN:
      spaces(indent);
      printf("ASSIGNMENT  locals=%p outer=%p\n",
             (void *) op->locals,
             (void *) op->locals->outer_scope);
      print_binary(op, ":=", indent);
      break;

   case SLANG_OPER_ADDASSIGN:
      spaces(indent);
      printf("ASSIGN\n");
      print_binary(op, "+=", indent);
      break;

   case SLANG_OPER_SUBASSIGN:
      spaces(indent);
      printf("ASSIGN\n");
      print_binary(op, "-=", indent);
      break;

   case SLANG_OPER_MULASSIGN:
      spaces(indent);
      printf("ASSIGN\n");
      print_binary(op, "*=", indent);
      break;

   case SLANG_OPER_DIVASSIGN:
      spaces(indent);
      printf("ASSIGN\n");
      print_binary(op, "/=", indent);
      break;

	/*SLANG_OPER_MODASSIGN,*/
	/*SLANG_OPER_LSHASSIGN,*/
	/*SLANG_OPER_RSHASSIGN,*/
	/*SLANG_OPER_ORASSIGN,*/
	/*SLANG_OPER_XORASSIGN,*/
	/*SLANG_OPER_ANDASSIGN,*/
   case SLANG_OPER_SELECT:
      spaces(indent);
      printf("SLANG_OPER_SELECT n=%d\n", op->num_children);
      assert(op->num_children == 3);
      slang_print_tree(&op->children[0], indent+3);
      spaces(indent);
      printf("?\n");
      slang_print_tree(&op->children[1], indent+3);
      spaces(indent);
      printf(":\n");
      slang_print_tree(&op->children[2], indent+3);
      break;

   case SLANG_OPER_LOGICALOR:
      print_binary(op, "||", indent);
      break;

   case SLANG_OPER_LOGICALXOR:
      print_binary(op, "^^", indent);
      break;

   case SLANG_OPER_LOGICALAND:
      print_binary(op, "&&", indent);
      break;

   /*SLANG_OPER_BITOR*/
   /*SLANG_OPER_BITXOR*/
   /*SLANG_OPER_BITAND*/
   case SLANG_OPER_EQUAL:
      print_binary(op, "==", indent);
      break;

   case SLANG_OPER_NOTEQUAL:
      print_binary(op, "!=", indent);
      break;

   case SLANG_OPER_LESS:
      print_binary(op, "<", indent);
      break;

   case SLANG_OPER_GREATER:
      print_binary(op, ">", indent);
      break;

   case SLANG_OPER_LESSEQUAL:
      print_binary(op, "<=", indent);
      break;

   case SLANG_OPER_GREATEREQUAL:
      print_binary(op, ">=", indent);
      break;

   /*SLANG_OPER_LSHIFT*/
   /*SLANG_OPER_RSHIFT*/
   case SLANG_OPER_ADD:
      print_binary(op, "+", indent);
      break;

   case SLANG_OPER_SUBTRACT:
      print_binary(op, "-", indent);
      break;

   case SLANG_OPER_MULTIPLY:
      print_binary(op, "*", indent);
      break;

   case SLANG_OPER_DIVIDE:
      print_binary(op, "/", indent);
      break;

   /*SLANG_OPER_MODULUS*/
   case SLANG_OPER_PREINCREMENT:
      spaces(indent);
      printf("PRE++\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   case SLANG_OPER_PREDECREMENT:
      spaces(indent);
      printf("PRE--\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   case SLANG_OPER_PLUS:
      spaces(indent);
      printf("SLANG_OPER_PLUS\n");
      break;

   case SLANG_OPER_MINUS:
      spaces(indent);
      printf("SLANG_OPER_MINUS\n");
      break;

   /*SLANG_OPER_COMPLEMENT*/
   case SLANG_OPER_NOT:
      spaces(indent);
      printf("NOT\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   case SLANG_OPER_SUBSCRIPT:
      spaces(indent);
      printf("SLANG_OPER_SUBSCRIPT locals=%p outer=%p\n",
             (void *) op->locals,
             (void *) op->locals->outer_scope);
      print_generic(op, NULL, indent+3);
      break;

   case SLANG_OPER_CALL:
#if 0
         slang_function *fun
            = _slang_locate_function(A->space.funcs, oper->a_id,
                                     oper->children,
                                     oper->num_children, &A->space, A->atoms);
#endif
      spaces(indent);
      printf("CALL %s(\n", (char *) op->a_id);
      for (i = 0; i < op->num_children; i++) {
         slang_print_tree(&op->children[i], indent+3);
         if (i + 1 < op->num_children) {
            spaces(indent + 3);
            printf(",\n");
         }
      }
      spaces(indent);
      printf(")\n");
      break;

   case SLANG_OPER_FIELD:
      spaces(indent);
      printf("FIELD %s of\n", (char*) op->a_id);
      slang_print_tree(&op->children[0], indent+3);
      break;

   case SLANG_OPER_POSTINCREMENT:
      spaces(indent);
      printf("POST++\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   case SLANG_OPER_POSTDECREMENT:
      spaces(indent);
      printf("POST--\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   default:
      printf("unknown op->type %d\n", (int) op->type);
   }

}



void
slang_print_function(const slang_function *f, GLboolean body)
{
   GLuint i;

#if 0
   if (_mesa_strcmp((char *) f->header.a_name, "main") != 0)
     return;
#endif

   printf("FUNCTION %s ( scope=%p\n",
          (char *) f->header.a_name, (void *) f->parameters);

   for (i = 0; i < f->param_count; i++) {
      print_variable(f->parameters->variables[i], 3);
   }

   printf(") param scope = %p\n", (void *) f->parameters);

   if (body && f->body)
      slang_print_tree(f->body, 0);
}





const char *
slang_type_qual_string(slang_type_qualifier q)
{
   switch (q) {
   case SLANG_QUAL_NONE:
      return "none";
   case SLANG_QUAL_CONST:
      return "const";
   case SLANG_QUAL_ATTRIBUTE:
      return "attribute";
   case SLANG_QUAL_VARYING:
      return "varying";
   case SLANG_QUAL_UNIFORM:
      return "uniform";
   case SLANG_QUAL_OUT:
      return "out";
   case SLANG_QUAL_INOUT:
      return "inout";
   case SLANG_QUAL_FIXEDOUTPUT:
      return "fixedoutput";
   case SLANG_QUAL_FIXEDINPUT:
      return "fixedinputk";
   default:
      return "qual?";
   }
}


static const char *
slang_type_string(slang_type_specifier_type t)
{
   switch (t) {
   case SLANG_SPEC_VOID:
      return "void";
   case SLANG_SPEC_BOOL:
      return "bool";
   case SLANG_SPEC_BVEC2:
      return "bvec2";
   case SLANG_SPEC_BVEC3:
      return "bvec3";
   case SLANG_SPEC_BVEC4:
      return "bvec4";
   case SLANG_SPEC_INT:
      return "int";
   case SLANG_SPEC_IVEC2:
      return "ivec2";
   case SLANG_SPEC_IVEC3:
      return "ivec3";
   case SLANG_SPEC_IVEC4:
      return "ivec4";
   case SLANG_SPEC_FLOAT:
      return "float";
   case SLANG_SPEC_VEC2:
      return "vec2";
   case SLANG_SPEC_VEC3:
      return "vec3";
   case SLANG_SPEC_VEC4:
      return "vec4";
   case SLANG_SPEC_MAT2:
      return "mat2";
   case SLANG_SPEC_MAT3:
      return "mat3";
   case SLANG_SPEC_MAT4:
      return "mat4";
   case SLANG_SPEC_SAMPLER1D:
      return "sampler1D";
   case SLANG_SPEC_SAMPLER2D:
      return "sampler2D";
   case SLANG_SPEC_SAMPLER3D:
      return "sampler3D";
   case SLANG_SPEC_SAMPLERCUBE:
      return "samplerCube";
   case SLANG_SPEC_SAMPLER1DSHADOW:
      return "sampler1DShadow";
   case SLANG_SPEC_SAMPLER2DSHADOW:
      return "sampler2DShadow";
   case SLANG_SPEC_SAMPLER2DRECT:
      return "sampler2DRect";
   case SLANG_SPEC_SAMPLER2DRECTSHADOW:
      return "sampler2DRectShadow";
   case SLANG_SPEC_STRUCT:
      return "struct";
   case SLANG_SPEC_ARRAY:
      return "array";
   default:
      return "type?";
   }
}


static const char *
slang_fq_type_string(const slang_fully_specified_type *t)
{
   static char str[1000];
   sprintf(str, "%s %s", slang_type_qual_string(t->qualifier),
      slang_type_string(t->specifier.type));
   return str;
}


void
slang_print_type(const slang_fully_specified_type *t)
{
   printf("%s %s", slang_type_qual_string(t->qualifier),
      slang_type_string(t->specifier.type));
}


#if 0
static char *
slang_var_string(const slang_variable *v)
{
   static char str[1000];
   sprintf(str, "%s : %s",
           (char *) v->a_name,
           slang_fq_type_string(&v->type));
   return str;
}
#endif


void
slang_print_variable(const slang_variable *v)
{
   printf("Name: %s\n", (char *) v->a_name);
   printf("Type: %s\n", slang_fq_type_string(&v->type));
}


void
_slang_print_var_scope(const slang_variable_scope *vars, int indent)
{
   GLuint i;

   spaces(indent);
   printf("Var scope %p  %d vars:\n", (void *) vars, vars->num_variables);
   for (i = 0; i < vars->num_variables; i++) {
      spaces(indent + 3);
      printf("%s (at %p)\n", (char *) vars->variables[i]->a_name, (void*) (vars->variables + i));
   }
   spaces(indent + 3);
   printf("outer_scope = %p\n", (void*) vars->outer_scope);

   if (vars->outer_scope) {
      /*spaces(indent + 3);*/
      _slang_print_var_scope(vars->outer_scope, indent + 3);
   }
}



int
slang_checksum_tree(const slang_operation *op)
{
   int s = op->num_children;
   GLuint i;

   for (i = 0; i < op->num_children; i++) {
      s += slang_checksum_tree(&op->children[i]);
   }
   return s;
}
