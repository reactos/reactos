/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
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

#ifndef SLANG_COMPILE_OPERATION_H
#define SLANG_COMPILE_OPERATION_H


/**
 * Types of slang operations.
 * These are the types of the AST (abstract syntax tree) nodes.
 * [foo] indicates a sub-tree or reference to another type of node
 */
typedef enum slang_operation_type_
{
   SLANG_OPER_NONE,
   SLANG_OPER_BLOCK_NO_NEW_SCOPE,       /* "{" sequence "}" */
   SLANG_OPER_BLOCK_NEW_SCOPE,  /* "{" sequence "}" */
   SLANG_OPER_VARIABLE_DECL,    /* [type] [var] or [var] = [expr] */
   SLANG_OPER_ASM,
   SLANG_OPER_BREAK,            /* "break" statement */
   SLANG_OPER_CONTINUE,         /* "continue" statement */
   SLANG_OPER_DISCARD,          /* "discard" (kill fragment) statement */
   SLANG_OPER_RETURN,           /* "return" [expr]  */
   SLANG_OPER_LABEL,            /* a jump target */
   SLANG_OPER_EXPRESSION,       /* [expr] */
   SLANG_OPER_IF,               /* "if" [0] then [1] else [2] */
   SLANG_OPER_WHILE,            /* "while" [cond] [body] */
   SLANG_OPER_DO,               /* "do" [body] "while" [cond] */
   SLANG_OPER_FOR,              /* "for" [init] [while] [incr] [body] */
   SLANG_OPER_VOID,             /* nop */
   SLANG_OPER_LITERAL_BOOL,     /* "true" or "false" */
   SLANG_OPER_LITERAL_INT,      /* integer literal */
   SLANG_OPER_LITERAL_FLOAT,    /* float literal */
   SLANG_OPER_IDENTIFIER,       /* var name, func name, etc */
   SLANG_OPER_SEQUENCE,         /* [expr] "," [expr] "," etc */
   SLANG_OPER_ASSIGN,           /* [var] "=" [expr] */
   SLANG_OPER_ADDASSIGN,        /* [var] "+=" [expr] */
   SLANG_OPER_SUBASSIGN,        /* [var] "-=" [expr] */
   SLANG_OPER_MULASSIGN,        /* [var] "*=" [expr] */
   SLANG_OPER_DIVASSIGN,        /* [var] "/=" [expr] */
   /*SLANG_OPER_MODASSIGN, */
   /*SLANG_OPER_LSHASSIGN, */
   /*SLANG_OPER_RSHASSIGN, */
   /*SLANG_OPER_ORASSIGN, */
   /*SLANG_OPER_XORASSIGN, */
   /*SLANG_OPER_ANDASSIGN, */
   SLANG_OPER_SELECT,           /* [expr] "?" [expr] ":" [expr] */
   SLANG_OPER_LOGICALOR,        /* [expr] "||" [expr] */
   SLANG_OPER_LOGICALXOR,       /* [expr] "^^" [expr] */
   SLANG_OPER_LOGICALAND,       /* [expr] "&&" [expr] */
   /*SLANG_OPER_BITOR, */
   /*SLANG_OPER_BITXOR, */
   /*SLANG_OPER_BITAND, */
   SLANG_OPER_EQUAL,            /* [expr] "==" [expr] */
   SLANG_OPER_NOTEQUAL,         /* [expr] "!=" [expr] */
   SLANG_OPER_LESS,             /* [expr] "<" [expr] */
   SLANG_OPER_GREATER,          /* [expr] ">" [expr] */
   SLANG_OPER_LESSEQUAL,        /* [expr] "<=" [expr] */
   SLANG_OPER_GREATEREQUAL,     /* [expr] ">=" [expr] */
   /*SLANG_OPER_LSHIFT, */
   /*SLANG_OPER_RSHIFT, */
   SLANG_OPER_ADD,              /* [expr] "+" [expr] */
   SLANG_OPER_SUBTRACT,         /* [expr] "-" [expr] */
   SLANG_OPER_MULTIPLY,         /* [expr] "*" [expr] */
   SLANG_OPER_DIVIDE,           /* [expr] "/" [expr] */
   /*SLANG_OPER_MODULUS, */
   SLANG_OPER_PREINCREMENT,     /* "++" [var] */
   SLANG_OPER_PREDECREMENT,     /* "--" [var] */
   SLANG_OPER_PLUS,             /* "-" [expr] */
   SLANG_OPER_MINUS,            /* "+" [expr] */
   /*SLANG_OPER_COMPLEMENT, */
   SLANG_OPER_NOT,              /* "!" [expr] */
   SLANG_OPER_SUBSCRIPT,        /* [expr] "[" [expr] "]" */
   SLANG_OPER_CALL,             /* [func name] [param] [param] [...] */
   SLANG_OPER_NON_INLINED_CALL, /* a real function call */
   SLANG_OPER_FIELD,            /* i.e.: ".next" or ".xzy" or ".xxx" etc */
   SLANG_OPER_POSTINCREMENT,    /* [var] "++" */
   SLANG_OPER_POSTDECREMENT     /* [var] "--" */
} slang_operation_type;


/**
 * A slang_operation is basically a compiled instruction (such as assignment,
 * a while-loop, a conditional, a multiply, a function call, etc).
 * The AST (abstract syntax tree) is built from these nodes.
 * NOTE: This structure could have been implemented as a union of simpler
 * structs which would correspond to the operation types above.
 */
typedef struct slang_operation_
{
   slang_operation_type type;
   struct slang_operation_ *children;
   GLuint num_children;
   GLfloat literal[4];           /**< Used for float, int and bool values */
   GLuint literal_size;          /**< 1, 2, 3, or 4 */
   slang_atom a_id;              /**< type: asm, identifier, call, field */
   slang_variable_scope *locals; /**< local vars for scope */
   struct slang_function_ *fun;  /**< If type == SLANG_OPER_CALL */
   struct slang_variable_ *var;  /**< If type == slang_oper_identier */
   struct slang_label_ *label;   /**< If type == SLANG_OPER_LABEL */
} slang_operation;


extern GLboolean
slang_operation_construct(slang_operation *);

extern void
slang_operation_destruct(slang_operation *);

extern void
slang_replace_scope(slang_operation *oper,
                    slang_variable_scope *oldScope,
                    slang_variable_scope *newScope);

extern GLboolean
slang_operation_copy(slang_operation *, const slang_operation *);

extern slang_operation *
slang_operation_new(GLuint count);

extern void
slang_operation_delete(slang_operation *oper);

extern slang_operation *
slang_operation_grow(GLuint *numChildren, slang_operation **children);

extern slang_operation *
slang_operation_insert(GLuint *numChildren, slang_operation **children,
                       GLuint pos);

extern void
_slang_operation_swap(slang_operation *oper0, slang_operation *oper1);


#endif /* SLANG_COMPILE_OPERATION_H */
