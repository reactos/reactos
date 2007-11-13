/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 * \file nvfragparse.c
 * NVIDIA fragment program parser.
 * \author Brian Paul
 */

/*
 * Regarding GL_NV_fragment_program:
 *
 * Portions of this software may use or implement intellectual
 * property owned and licensed by NVIDIA Corporation. NVIDIA disclaims
 * any and all warranties with respect to such intellectual property,
 * including any use thereof or modifications thereto.
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "prog_parameter.h"
#include "prog_instruction.h"
#include "nvfragparse.h"
#include "program.h"


#define INPUT_1V     1
#define INPUT_2V     2
#define INPUT_3V     3
#define INPUT_1S     4
#define INPUT_2S     5
#define INPUT_CC     6
#define INPUT_1V_T   7  /* one source vector, plus textureId */
#define INPUT_3V_T   8  /* one source vector, plus textureId */
#define INPUT_NONE   9
#define INPUT_1V_S  10  /* a string and a vector register */
#define OUTPUT_V    20
#define OUTPUT_S    21
#define OUTPUT_NONE 22

/* IRIX defines some of these */
#undef _R
#undef _H
#undef _X
#undef _C
#undef _S

/* Optional suffixes */
#define _R  FLOAT32  /* float */
#define _H  FLOAT16  /* half-float */
#define _X  FIXED12  /* fixed */
#define _C  0x08     /* set cond codes */
#define _S  0x10     /* saturate, clamp result to [0,1] */

struct instruction_pattern {
   const char *name;
   enum prog_opcode opcode;
   GLuint inputs;
   GLuint outputs;
   GLuint suffixes;
};

static const struct instruction_pattern Instructions[] = {
   { "ADD", OPCODE_ADD, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "COS", OPCODE_COS, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "DDX", OPCODE_DDX, INPUT_1V, OUTPUT_V, _R | _H |      _C | _S },
   { "DDY", OPCODE_DDY, INPUT_1V, OUTPUT_V, _R | _H |      _C | _S },
   { "DP3", OPCODE_DP3, INPUT_2V, OUTPUT_S, _R | _H | _X | _C | _S },
   { "DP4", OPCODE_DP4, INPUT_2V, OUTPUT_S, _R | _H | _X | _C | _S },
   { "DST", OPCODE_DP4, INPUT_2V, OUTPUT_V, _R | _H |      _C | _S },
   { "EX2", OPCODE_DP4, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "FLR", OPCODE_FLR, INPUT_1V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "FRC", OPCODE_FRC, INPUT_1V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "KIL", OPCODE_KIL_NV, INPUT_CC, OUTPUT_NONE, 0                },
   { "LG2", OPCODE_LG2, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "LIT", OPCODE_LIT, INPUT_1V, OUTPUT_V, _R | _H |      _C | _S },
   { "LRP", OPCODE_LRP, INPUT_3V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MAD", OPCODE_MAD, INPUT_3V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MAX", OPCODE_MAX, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MIN", OPCODE_MIN, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MOV", OPCODE_MOV, INPUT_1V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MUL", OPCODE_MUL, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "PK2H",  OPCODE_PK2H,  INPUT_1V, OUTPUT_S, 0                  },
   { "PK2US", OPCODE_PK2US, INPUT_1V, OUTPUT_S, 0                  },
   { "PK4B",  OPCODE_PK4B,  INPUT_1V, OUTPUT_S, 0                  },
   { "PK4UB", OPCODE_PK4UB, INPUT_1V, OUTPUT_S, 0                  },
   { "POW", OPCODE_POW, INPUT_2S, OUTPUT_S, _R | _H |      _C | _S },
   { "RCP", OPCODE_RCP, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "RFL", OPCODE_RFL, INPUT_2V, OUTPUT_V, _R | _H |      _C | _S },
   { "RSQ", OPCODE_RSQ, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "SEQ", OPCODE_SEQ, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SFL", OPCODE_SFL, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SGE", OPCODE_SGE, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SGT", OPCODE_SGT, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SIN", OPCODE_SIN, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "SLE", OPCODE_SLE, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SLT", OPCODE_SLT, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SNE", OPCODE_SNE, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "STR", OPCODE_STR, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SUB", OPCODE_SUB, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "TEX", OPCODE_TEX, INPUT_1V_T, OUTPUT_V,              _C | _S },
   { "TXD", OPCODE_TXD, INPUT_3V_T, OUTPUT_V,              _C | _S },
   { "TXP", OPCODE_TXP_NV, INPUT_1V_T, OUTPUT_V,           _C | _S },
   { "UP2H",  OPCODE_UP2H,  INPUT_1S, OUTPUT_V,            _C | _S },
   { "UP2US", OPCODE_UP2US, INPUT_1S, OUTPUT_V,            _C | _S },
   { "UP4B",  OPCODE_UP4B,  INPUT_1S, OUTPUT_V,            _C | _S },
   { "UP4UB", OPCODE_UP4UB, INPUT_1S, OUTPUT_V,            _C | _S },
   { "X2D", OPCODE_X2D, INPUT_3V, OUTPUT_V, _R | _H |      _C | _S },
   { "PRINT", OPCODE_PRINT, INPUT_1V_S, OUTPUT_NONE, 0               },
   { NULL, (enum prog_opcode) -1, 0, 0, 0 }
};


/*
 * Information needed or computed during parsing.
 * Remember, we can't modify the target program object until we've
 * _successfully_ parsed the program text.
 */
struct parse_state {
   GLcontext *ctx;
   const GLubyte *start;              /* start of program string */
   const GLubyte *pos;                /* current position */
   const GLubyte *curLine;
   struct gl_fragment_program *program;  /* current program */

   struct gl_program_parameter_list *parameters;

   GLuint numInst;                    /* number of instructions parsed */
   GLuint inputsRead;                 /* bitmask of input registers used */
   GLuint outputsWritten;             /* bitmask of 1 << FRAG_OUTPUT_* bits */
   GLuint texturesUsed[MAX_TEXTURE_IMAGE_UNITS];
};



/*
 * Called whenever we find an error during parsing.
 */
static void
record_error(struct parse_state *parseState, const char *msg, int lineNo)
{
#ifdef DEBUG
   GLint line, column;
   const GLubyte *lineStr;
   lineStr = _mesa_find_line_column(parseState->start,
                                    parseState->pos, &line, &column);
   _mesa_debug(parseState->ctx,
               "nvfragparse.c(%d): line %d, column %d:%s (%s)\n",
               lineNo, line, column, (char *) lineStr, msg);
   _mesa_free((void *) lineStr);
#else
   (void) lineNo;
#endif

   /* Check that no error was already recorded.  Only record the first one. */
   if (parseState->ctx->Program.ErrorString[0] == 0) {
      _mesa_set_program_error(parseState->ctx,
                              parseState->pos - parseState->start,
                              msg);
   }
}


#define RETURN_ERROR							\
do {									\
   record_error(parseState, "Unexpected end of input.", __LINE__);	\
   return GL_FALSE;							\
} while(0)

#define RETURN_ERROR1(msg)						\
do {									\
   record_error(parseState, msg, __LINE__);				\
   return GL_FALSE;							\
} while(0)

#define RETURN_ERROR2(msg1, msg2)					\
do {									\
   char err[1000];							\
   _mesa_sprintf(err, "%s %s", msg1, msg2);				\
   record_error(parseState, err, __LINE__);				\
   return GL_FALSE;							\
} while(0)




/*
 * Search a list of instruction structures for a match.
 */
static struct instruction_pattern
MatchInstruction(const GLubyte *token)
{
   const struct instruction_pattern *inst;
   struct instruction_pattern result;

   for (inst = Instructions; inst->name; inst++) {
      if (_mesa_strncmp((const char *) token, inst->name, 3) == 0) {
         /* matched! */
         int i = 3;
         result = *inst;
         result.suffixes = 0;
         /* look at suffix */
         if (token[i] == 'R') {
            result.suffixes |= _R;
            i++;
         }
         else if (token[i] == 'H') {
            result.suffixes |= _H;
            i++;
         }
         else if (token[i] == 'X') {
            result.suffixes |= _X;
            i++;
         }
         if (token[i] == 'C') {
            result.suffixes |= _C;
            i++;
         }
         if (token[i] == '_' && token[i+1] == 'S' &&
             token[i+2] == 'A' && token[i+3] == 'T') {
            result.suffixes |= _S;
         }
         return result;
      }
   }
   result.opcode = MAX_OPCODE; /* i.e. invalid instruction */
   return result;
}




/**********************************************************************/


static GLboolean IsLetter(GLubyte b)
{
   return (b >= 'a' && b <= 'z') ||
          (b >= 'A' && b <= 'Z') ||
          (b == '_') ||
          (b == '$');
}


static GLboolean IsDigit(GLubyte b)
{
   return b >= '0' && b <= '9';
}


static GLboolean IsWhitespace(GLubyte b)
{
   return b == ' ' || b == '\t' || b == '\n' || b == '\r';
}


/**
 * Starting at 'str' find the next token.  A token can be an integer,
 * an identifier or punctuation symbol.
 * \return <= 0 we found an error, else, return number of characters parsed.
 */
static GLint
GetToken(struct parse_state *parseState, GLubyte *token)
{
   const GLubyte *str = parseState->pos;
   GLint i = 0, j = 0;

   token[0] = 0;

   /* skip whitespace and comments */
   while (str[i] && (IsWhitespace(str[i]) || str[i] == '#')) {
      if (str[i] == '#') {
         /* skip comment */
         while (str[i] && (str[i] != '\n' && str[i] != '\r')) {
            i++;
         }
         if (str[i] == '\n' || str[i] == '\r')
            parseState->curLine = str + i + 1;
      }
      else {
         /* skip whitespace */
         if (str[i] == '\n' || str[i] == '\r')
            parseState->curLine = str + i + 1;
         i++;
      }
   }

   if (str[i] == 0)
      return -i;

   /* try matching an integer */
   while (str[i] && IsDigit(str[i])) {
      token[j++] = str[i++];
   }
   if (j > 0 || !str[i]) {
      token[j] = 0;
      return i;
   }

   /* try matching an identifier */
   if (IsLetter(str[i])) {
      while (str[i] && (IsLetter(str[i]) || IsDigit(str[i]))) {
         token[j++] = str[i++];
      }
      token[j] = 0;
      return i;
   }

   /* punctuation character */
   if (str[i]) {
      token[0] = str[i++];
      token[1] = 0;
      return i;
   }

   /* end of input */
   token[0] = 0;
   return i;
}


/**
 * Get next token from input stream and increment stream pointer past token.
 */
static GLboolean
Parse_Token(struct parse_state *parseState, GLubyte *token)
{
   GLint i;
   i = GetToken(parseState, token);
   if (i <= 0) {
      parseState->pos += (-i);
      return GL_FALSE;
   }
   parseState->pos += i;
   return GL_TRUE;
}


/**
 * Get next token from input stream but don't increment stream pointer.
 */
static GLboolean
Peek_Token(struct parse_state *parseState, GLubyte *token)
{
   GLint i, len;
   i = GetToken(parseState, token);
   if (i <= 0) {
      parseState->pos += (-i);
      return GL_FALSE;
   }
   len = (GLint)_mesa_strlen((const char *) token);
   parseState->pos += (i - len);
   return GL_TRUE;
}


/**********************************************************************/

static const char *InputRegisters[MAX_NV_FRAGMENT_PROGRAM_INPUTS + 1] = {
   "WPOS", "COL0", "COL1", "FOGC",
   "TEX0", "TEX1", "TEX2", "TEX3", "TEX4", "TEX5", "TEX6", "TEX7", NULL
};

static const char *OutputRegisters[MAX_NV_FRAGMENT_PROGRAM_OUTPUTS + 1] = {
   "COLR", "COLH",
   /* These are only allows for register combiners */
   /*
   "TEX0", "TEX1", "TEX2", "TEX3",
   */
   "DEPR", NULL
};




/**********************************************************************/

/**
 * Try to match 'pattern' as the next token after any whitespace/comments.
 */
static GLboolean
Parse_String(struct parse_state *parseState, const char *pattern)
{
   const GLubyte *m;
   GLint i;

   /* skip whitespace and comments */
   while (IsWhitespace(*parseState->pos) || *parseState->pos == '#') {
      if (*parseState->pos == '#') {
         while (*parseState->pos && (*parseState->pos != '\n' && *parseState->pos != '\r')) {
            parseState->pos += 1;
         }
         if (*parseState->pos == '\n' || *parseState->pos == '\r')
            parseState->curLine = parseState->pos + 1;
      }
      else {
         /* skip whitespace */
         if (*parseState->pos == '\n' || *parseState->pos == '\r')
            parseState->curLine = parseState->pos + 1;
         parseState->pos += 1;
      }
   }

   /* Try to match the pattern */
   m = parseState->pos;
   for (i = 0; pattern[i]; i++) {
      if (*m != (GLubyte) pattern[i])
         return GL_FALSE;
      m += 1;
   }
   parseState->pos = m;

   return GL_TRUE; /* success */
}


static GLboolean
Parse_Identifier(struct parse_state *parseState, GLubyte *ident)
{
   if (!Parse_Token(parseState, ident))
      RETURN_ERROR;
   if (IsLetter(ident[0]))
      return GL_TRUE;
   else
      RETURN_ERROR1("Expected an identfier");
}


/**
 * Parse a floating point constant, or a defined symbol name.
 * [+/-]N[.N[eN]]
 * Output:  number[0 .. 3] will get the value.
 */
static GLboolean
Parse_ScalarConstant(struct parse_state *parseState, GLfloat *number)
{
   char *end = NULL;

   *number = (GLfloat) _mesa_strtod((const char *) parseState->pos, &end);

   if (end && end > (char *) parseState->pos) {
      /* got a number */
      parseState->pos = (GLubyte *) end;
      number[1] = *number;
      number[2] = *number;
      number[3] = *number;
      return GL_TRUE;
   }
   else {
      /* should be an identifier */
      GLubyte ident[100];
      const GLfloat *constant;
      if (!Parse_Identifier(parseState, ident))
         RETURN_ERROR1("Expected an identifier");
      constant = _mesa_lookup_parameter_value(parseState->parameters,
                                              -1, (const char *) ident);
      /* XXX Check that it's a constant and not a parameter */
      if (!constant) {
         RETURN_ERROR1("Undefined symbol");
      }
      else {
         COPY_4V(number, constant);
         return GL_TRUE;
      }
   }
}



/**
 * Parse a vector constant, one of:
 *   { float }
 *   { float, float }
 *   { float, float, float }
 *   { float, float, float, float }
 */
static GLboolean
Parse_VectorConstant(struct parse_state *parseState, GLfloat *vec)
{
   /* "{" was already consumed */

   ASSIGN_4V(vec, 0.0, 0.0, 0.0, 1.0);

   if (!Parse_ScalarConstant(parseState, vec+0))  /* X */
      return GL_FALSE;

   if (Parse_String(parseState, "}")) {
      return GL_TRUE;
   }

   if (!Parse_String(parseState, ","))
      RETURN_ERROR1("Expected comma in vector constant");

   if (!Parse_ScalarConstant(parseState, vec+1))  /* Y */
      return GL_FALSE;

   if (Parse_String(parseState, "}")) {
      return GL_TRUE;
   }

   if (!Parse_String(parseState, ","))
      RETURN_ERROR1("Expected comma in vector constant");

   if (!Parse_ScalarConstant(parseState, vec+2))  /* Z */
      return GL_FALSE;

   if (Parse_String(parseState, "}")) {
      return GL_TRUE;
   }

   if (!Parse_String(parseState, ","))
      RETURN_ERROR1("Expected comma in vector constant");

   if (!Parse_ScalarConstant(parseState, vec+3))  /* W */
      return GL_FALSE;

   if (!Parse_String(parseState, "}"))
      RETURN_ERROR1("Expected closing brace in vector constant");

   return GL_TRUE;
}


/**
 * Parse <number>, <varname> or {a, b, c, d}.
 * Return number of values in the vector or scalar, or zero if parse error.
 */
static GLuint
Parse_VectorOrScalarConstant(struct parse_state *parseState, GLfloat *vec)
{
   if (Parse_String(parseState, "{")) {
      return Parse_VectorConstant(parseState, vec);
   }
   else {
      GLboolean b = Parse_ScalarConstant(parseState, vec);
      if (b) {
         vec[1] = vec[2] = vec[3] = vec[0];
      }
      return b;
   }
}


/**
 * Parse a texture image source:
 *    [TEX0 | TEX1 | .. | TEX15] , [1D | 2D | 3D | CUBE | RECT]
 */
static GLboolean
Parse_TextureImageId(struct parse_state *parseState,
                     GLubyte *texUnit, GLubyte *texTargetBit)
{
   GLubyte imageSrc[100];
   GLint unit;

   if (!Parse_Token(parseState, imageSrc))
      RETURN_ERROR;
   
   if (imageSrc[0] != 'T' ||
       imageSrc[1] != 'E' ||
       imageSrc[2] != 'X') {
      RETURN_ERROR1("Expected TEX# source");
   }
   unit = _mesa_atoi((const char *) imageSrc + 3);
   if ((unit < 0 || unit > MAX_TEXTURE_IMAGE_UNITS) ||
       (unit == 0 && (imageSrc[3] != '0' || imageSrc[4] != 0))) {
      RETURN_ERROR1("Invalied TEX# source index");
   }
   *texUnit = unit;

   if (!Parse_String(parseState, ","))
      RETURN_ERROR1("Expected ,");

   if (Parse_String(parseState, "1D")) {
      *texTargetBit = TEXTURE_1D_BIT;
   }
   else if (Parse_String(parseState, "2D")) {
      *texTargetBit = TEXTURE_2D_BIT;
   }
   else if (Parse_String(parseState, "3D")) {
      *texTargetBit = TEXTURE_3D_BIT;
   }
   else if (Parse_String(parseState, "CUBE")) {
      *texTargetBit = TEXTURE_CUBE_BIT;
   }
   else if (Parse_String(parseState, "RECT")) {
      *texTargetBit = TEXTURE_RECT_BIT;
   }
   else {
      RETURN_ERROR1("Invalid texture target token");
   }

   /* update record of referenced texture units */
   parseState->texturesUsed[*texUnit] |= *texTargetBit;
   if (_mesa_bitcount(parseState->texturesUsed[*texUnit]) > 1) {
      RETURN_ERROR1("Only one texture target can be used per texture unit.");
   }

   return GL_TRUE;
}


/**
 * Parse a scalar suffix like .x, .y, .z or .w or parse a swizzle suffix
 * like .wxyz, .xxyy, etc and return the swizzle indexes.
 */
static GLboolean
Parse_SwizzleSuffix(const GLubyte *token, GLuint swizzle[4])
{
   if (token[1] == 0) {
      /* single letter swizzle (scalar) */
      if (token[0] == 'x')
         ASSIGN_4V(swizzle, 0, 0, 0, 0);
      else if (token[0] == 'y')
         ASSIGN_4V(swizzle, 1, 1, 1, 1);
      else if (token[0] == 'z')
         ASSIGN_4V(swizzle, 2, 2, 2, 2);
      else if (token[0] == 'w')
         ASSIGN_4V(swizzle, 3, 3, 3, 3);
      else
         return GL_FALSE;
   }
   else {
      /* 4-component swizzle (vector) */
      GLint k;
      for (k = 0; token[k] && k < 4; k++) {
         if (token[k] == 'x')
            swizzle[k] = 0;
         else if (token[k] == 'y')
            swizzle[k] = 1;
         else if (token[k] == 'z')
            swizzle[k] = 2;
         else if (token[k] == 'w')
            swizzle[k] = 3;
         else
            return GL_FALSE;
      }
      if (k != 4)
         return GL_FALSE;
   }
   return GL_TRUE;
}


static GLboolean
Parse_CondCodeMask(struct parse_state *parseState,
                   struct prog_dst_register *dstReg)
{
   if (Parse_String(parseState, "EQ"))
      dstReg->CondMask = COND_EQ;
   else if (Parse_String(parseState, "GE"))
      dstReg->CondMask = COND_GE;
   else if (Parse_String(parseState, "GT"))
      dstReg->CondMask = COND_GT;
   else if (Parse_String(parseState, "LE"))
      dstReg->CondMask = COND_LE;
   else if (Parse_String(parseState, "LT"))
      dstReg->CondMask = COND_LT;
   else if (Parse_String(parseState, "NE"))
      dstReg->CondMask = COND_NE;
   else if (Parse_String(parseState, "TR"))
      dstReg->CondMask = COND_TR;
   else if (Parse_String(parseState, "FL"))
      dstReg->CondMask = COND_FL;
   else
      RETURN_ERROR1("Invalid condition code mask");

   /* look for optional .xyzw swizzle */
   if (Parse_String(parseState, ".")) {
      GLubyte token[100];
      GLuint swz[4];

      if (!Parse_Token(parseState, token))  /* get xyzw suffix */
         RETURN_ERROR;

      if (!Parse_SwizzleSuffix(token, swz))
         RETURN_ERROR1("Invalid swizzle suffix");

      dstReg->CondSwizzle = MAKE_SWIZZLE4(swz[0], swz[1], swz[2], swz[3]);
   }

   return GL_TRUE;
}


/**
 * Parse a temporary register: Rnn or Hnn
 */
static GLboolean
Parse_TempReg(struct parse_state *parseState, GLint *tempRegNum)
{
   GLubyte token[100];

   /* Should be 'R##' or 'H##' */
   if (!Parse_Token(parseState, token))
      RETURN_ERROR;
   if (token[0] != 'R' && token[0] != 'H')
      RETURN_ERROR1("Expected R## or H##");

   if (IsDigit(token[1])) {
      GLint reg = _mesa_atoi((const char *) (token + 1));
      if (token[0] == 'H')
         reg += 32;
      if (reg >= MAX_NV_FRAGMENT_PROGRAM_TEMPS)
         RETURN_ERROR1("Invalid temporary register name");
      *tempRegNum = reg;
   }
   else {
      RETURN_ERROR1("Invalid temporary register name");
   }

   return GL_TRUE;
}


/**
 * Parse a write-only dummy register: RC or HC.
 */
static GLboolean
Parse_DummyReg(struct parse_state *parseState, GLint *regNum)
{
   if (Parse_String(parseState, "RC")) {
       *regNum = 0;
   }
   else if (Parse_String(parseState, "HC")) {
       *regNum = 1;
   }
   else {
      RETURN_ERROR1("Invalid write-only register name");
   }

   return GL_TRUE;
}


/**
 * Parse a program local parameter register "p[##]"
 */
static GLboolean
Parse_ProgramParamReg(struct parse_state *parseState, GLint *regNum)
{
   GLubyte token[100];

   if (!Parse_String(parseState, "p["))
      RETURN_ERROR1("Expected p[");

   if (!Parse_Token(parseState, token))
      RETURN_ERROR;

   if (IsDigit(token[0])) {
      /* a numbered program parameter register */
      GLint reg = _mesa_atoi((const char *) token);
      if (reg >= MAX_NV_FRAGMENT_PROGRAM_PARAMS)
         RETURN_ERROR1("Invalid constant program number");
      *regNum = reg;
   }
   else {
      RETURN_ERROR;
   }

   if (!Parse_String(parseState, "]"))
      RETURN_ERROR1("Expected ]");

   return GL_TRUE;
}


/**
 * Parse f[name]  - fragment input register
 */
static GLboolean
Parse_FragReg(struct parse_state *parseState, GLint *tempRegNum)
{
   GLubyte token[100];
   GLint j;

   /* Match 'f[' */
   if (!Parse_String(parseState, "f["))
      RETURN_ERROR1("Expected f[");

   /* get <name> and look for match */
   if (!Parse_Token(parseState, token)) {
      RETURN_ERROR;
   }
   for (j = 0; InputRegisters[j]; j++) {
      if (_mesa_strcmp((const char *) token, InputRegisters[j]) == 0) {
         *tempRegNum = j;
         parseState->inputsRead |= (1 << j);
         break;
      }
   }
   if (!InputRegisters[j]) {
      /* unknown input register label */
      RETURN_ERROR2("Invalid register name", token);
   }

   /* Match '[' */
   if (!Parse_String(parseState, "]"))
      RETURN_ERROR1("Expected ]");

   return GL_TRUE;
}


static GLboolean
Parse_OutputReg(struct parse_state *parseState, GLint *outputRegNum)
{
   GLubyte token[100];
   GLint j;

   /* Match "o[" */
   if (!Parse_String(parseState, "o["))
      RETURN_ERROR1("Expected o[");

   /* Get output reg name */
   if (!Parse_Token(parseState, token))
      RETURN_ERROR;

   /* try to match an output register name */
   for (j = 0; OutputRegisters[j]; j++) {
      if (_mesa_strcmp((const char *) token, OutputRegisters[j]) == 0) {
         static GLuint bothColors = (1 << FRAG_RESULT_COLR) | (1 << FRAG_RESULT_COLH);
         *outputRegNum = j;
         parseState->outputsWritten |= (1 << j);
         if ((parseState->outputsWritten & bothColors) == bothColors) {
            RETURN_ERROR1("Illegal to write to both o[COLR] and o[COLH]");
         }
         break;
      }
   }
   if (!OutputRegisters[j])
      RETURN_ERROR1("Invalid output register name");

   /* Match ']' */
   if (!Parse_String(parseState, "]"))
      RETURN_ERROR1("Expected ]");

   return GL_TRUE;
}


static GLboolean
Parse_MaskedDstReg(struct parse_state *parseState,
                   struct prog_dst_register *dstReg)
{
   GLubyte token[100];
   GLint idx;

   /* Dst reg can be R<n>, H<n>, o[n], RC or HC */
   if (!Peek_Token(parseState, token))
      RETURN_ERROR;

   if (_mesa_strcmp((const char *) token, "RC") == 0 ||
       _mesa_strcmp((const char *) token, "HC") == 0) {
      /* a write-only register */
      dstReg->File = PROGRAM_WRITE_ONLY;
      if (!Parse_DummyReg(parseState, &idx))
         RETURN_ERROR;
      dstReg->Index = idx;
   }
   else if (token[0] == 'R' || token[0] == 'H') {
      /* a temporary register */
      dstReg->File = PROGRAM_TEMPORARY;
      if (!Parse_TempReg(parseState, &idx))
         RETURN_ERROR;
      dstReg->Index = idx;
   }
   else if (token[0] == 'o') {
      /* an output register */
      dstReg->File = PROGRAM_OUTPUT;
      if (!Parse_OutputReg(parseState, &idx))
         RETURN_ERROR;
      dstReg->Index = idx;
   }
   else {
      RETURN_ERROR1("Invalid destination register name");
   }

   /* Parse optional write mask */
   if (Parse_String(parseState, ".")) {
      /* got a mask */
      GLint k = 0;

      if (!Parse_Token(parseState, token))  /* get xyzw writemask */
         RETURN_ERROR;

      dstReg->WriteMask = 0;

      if (token[k] == 'x') {
         dstReg->WriteMask |= WRITEMASK_X;
         k++;
      }
      if (token[k] == 'y') {
         dstReg->WriteMask |= WRITEMASK_Y;
         k++;
      }
      if (token[k] == 'z') {
         dstReg->WriteMask |= WRITEMASK_Z;
         k++;
      }
      if (token[k] == 'w') {
         dstReg->WriteMask |= WRITEMASK_W;
         k++;
      }
      if (k == 0) {
         RETURN_ERROR1("Invalid writemask character");
      }

   }
   else {
      dstReg->WriteMask = WRITEMASK_XYZW;
   }

   /* optional condition code mask */
   if (Parse_String(parseState, "(")) {
      /* ("EQ" | "GE" | "GT" | "LE" | "LT" | "NE" | "TR" | "FL".x|y|z|w) */
      /* ("EQ" | "GE" | "GT" | "LE" | "LT" | "NE" | "TR" | "FL".[xyzw]) */
      if (!Parse_CondCodeMask(parseState, dstReg))
         RETURN_ERROR;

      if (!Parse_String(parseState, ")"))  /* consume ")" */
         RETURN_ERROR1("Expected )");

      return GL_TRUE;
   }
   else {
      /* no cond code mask */
      dstReg->CondMask = COND_TR;
      dstReg->CondSwizzle = SWIZZLE_NOOP;
      return GL_TRUE;
   }
}


/**
 * Parse a vector source (register, constant, etc):
 *   <vectorSrc>    ::= <absVectorSrc>
 *                    | <baseVectorSrc>
 *   <absVectorSrc> ::= <negate> "|" <baseVectorSrc> "|"
 */
static GLboolean
Parse_VectorSrc(struct parse_state *parseState,
                struct prog_src_register *srcReg)
{
   GLfloat sign = 1.0F;
   GLubyte token[100];
   GLint idx;

   /*
    * First, take care of +/- and absolute value stuff.
    */
   if (Parse_String(parseState, "-"))
      sign = -1.0F;
   else if (Parse_String(parseState, "+"))
      sign = +1.0F;

   if (Parse_String(parseState, "|")) {
      srcReg->Abs = GL_TRUE;
      srcReg->NegateAbs = (sign < 0.0F) ? GL_TRUE : GL_FALSE;

      if (Parse_String(parseState, "-"))
         srcReg->NegateBase = NEGATE_XYZW;
      else if (Parse_String(parseState, "+"))
         srcReg->NegateBase = NEGATE_NONE;
      else
         srcReg->NegateBase = NEGATE_NONE;
   }
   else {
      srcReg->Abs = GL_FALSE;
      srcReg->NegateAbs = GL_FALSE;
      srcReg->NegateBase = (sign < 0.0F) ? NEGATE_XYZW : NEGATE_NONE;
   }

   /* This should be the real src vector/register name */
   if (!Peek_Token(parseState, token))
      RETURN_ERROR;

   /* Src reg can be Rn, Hn, f[n], p[n], a named parameter, a scalar
    * literal or vector literal.
    */
   if (token[0] == 'R' || token[0] == 'H') {
      srcReg->File = PROGRAM_TEMPORARY;
      if (!Parse_TempReg(parseState, &idx))
         RETURN_ERROR;
      srcReg->Index = idx;
   }
   else if (token[0] == 'f') {
      /* XXX this might be an identifier! */
      srcReg->File = PROGRAM_INPUT;
      if (!Parse_FragReg(parseState, &idx))
         RETURN_ERROR;
      srcReg->Index = idx;
   }
   else if (token[0] == 'p') {
      /* XXX this might be an identifier! */
      srcReg->File = PROGRAM_LOCAL_PARAM;
      if (!Parse_ProgramParamReg(parseState, &idx))
         RETURN_ERROR;
      srcReg->Index = idx;
   }
   else if (IsLetter(token[0])){
      GLubyte ident[100];
      GLint paramIndex;
      if (!Parse_Identifier(parseState, ident))
         RETURN_ERROR;
      paramIndex = _mesa_lookup_parameter_index(parseState->parameters,
                                                -1, (const char *) ident);
      if (paramIndex < 0) {
         RETURN_ERROR2("Undefined constant or parameter: ", ident);
      }
      srcReg->File = PROGRAM_NAMED_PARAM;
      srcReg->Index = paramIndex;      
   }
   else if (IsDigit(token[0]) || token[0] == '-' || token[0] == '+' || token[0] == '.'){
      /* literal scalar constant */
      GLfloat values[4];
      GLuint paramIndex;
      if (!Parse_ScalarConstant(parseState, values))
         RETURN_ERROR;
      paramIndex = _mesa_add_unnamed_constant(parseState->parameters,
                                              values, 4, NULL);
      srcReg->File = PROGRAM_NAMED_PARAM;
      srcReg->Index = paramIndex;
   }
   else if (token[0] == '{'){
      /* literal vector constant */
      GLfloat values[4];
      GLuint paramIndex;
      (void) Parse_String(parseState, "{");
      if (!Parse_VectorConstant(parseState, values))
         RETURN_ERROR;
      paramIndex = _mesa_add_unnamed_constant(parseState->parameters,
                                              values, 4, NULL);
      srcReg->File = PROGRAM_NAMED_PARAM;
      srcReg->Index = paramIndex;      
   }
   else {
      RETURN_ERROR2("Invalid source register name", token);
   }

   /* init swizzle fields */
   srcReg->Swizzle = SWIZZLE_NOOP;

   /* Look for optional swizzle suffix */
   if (Parse_String(parseState, ".")) {
      GLuint swz[4];

      if (!Parse_Token(parseState, token))
         RETURN_ERROR;

      if (!Parse_SwizzleSuffix(token, swz))
         RETURN_ERROR1("Invalid swizzle suffix");

      srcReg->Swizzle = MAKE_SWIZZLE4(swz[0], swz[1], swz[2], swz[3]);
   }

   /* Finish absolute value */
   if (srcReg->Abs && !Parse_String(parseState, "|")) {
      RETURN_ERROR1("Expected |");
   }

   return GL_TRUE;
}


static GLboolean
Parse_ScalarSrcReg(struct parse_state *parseState,
                   struct prog_src_register *srcReg)
{
   GLubyte token[100];
   GLfloat sign = 1.0F;
   GLboolean needSuffix = GL_TRUE;
   GLint idx;

   /*
    * First, take care of +/- and absolute value stuff.
    */
   if (Parse_String(parseState, "-"))
      sign = -1.0F;
   else if (Parse_String(parseState, "+"))
      sign = +1.0F;

   if (Parse_String(parseState, "|")) {
      srcReg->Abs = GL_TRUE;
      srcReg->NegateAbs = (sign < 0.0F) ? GL_TRUE : GL_FALSE;

      if (Parse_String(parseState, "-"))
         srcReg->NegateBase = NEGATE_XYZW;
      else if (Parse_String(parseState, "+"))
         srcReg->NegateBase = NEGATE_NONE;
      else
         srcReg->NegateBase = NEGATE_NONE;
   }
   else {
      srcReg->Abs = GL_FALSE;
      srcReg->NegateAbs = GL_FALSE;
      srcReg->NegateBase = (sign < 0.0F) ? NEGATE_XYZW : NEGATE_NONE;
   }

   if (!Peek_Token(parseState, token))
      RETURN_ERROR;

   /* Src reg can be R<n>, H<n> or a named fragment attrib */
   if (token[0] == 'R' || token[0] == 'H') {
      srcReg->File = PROGRAM_TEMPORARY;
      if (!Parse_TempReg(parseState, &idx))
         RETURN_ERROR;
      srcReg->Index = idx;
   }
   else if (token[0] == 'f') {
      srcReg->File = PROGRAM_INPUT;
      if (!Parse_FragReg(parseState, &idx))
         RETURN_ERROR;
      srcReg->Index = idx;
   }
   else if (token[0] == '{') {
      /* vector literal */
      GLfloat values[4];
      GLuint paramIndex;
      (void) Parse_String(parseState, "{");
      if (!Parse_VectorConstant(parseState, values))
         RETURN_ERROR;
      paramIndex = _mesa_add_unnamed_constant(parseState->parameters,
                                              values, 4, NULL);
      srcReg->File = PROGRAM_NAMED_PARAM;
      srcReg->Index = paramIndex;      
   }
   else if (IsLetter(token[0])){
      /* named param/constant */
      GLubyte ident[100];
      GLint paramIndex;
      if (!Parse_Identifier(parseState, ident))
         RETURN_ERROR;
      paramIndex = _mesa_lookup_parameter_index(parseState->parameters,
                                                -1, (const char *) ident);
      if (paramIndex < 0) {
         RETURN_ERROR2("Undefined constant or parameter: ", ident);
      }
      srcReg->File = PROGRAM_NAMED_PARAM;
      srcReg->Index = paramIndex;      
   }
   else if (IsDigit(token[0])) {
      /* scalar literal */
      GLfloat values[4];
      GLuint paramIndex;
      if (!Parse_ScalarConstant(parseState, values))
         RETURN_ERROR;
      paramIndex = _mesa_add_unnamed_constant(parseState->parameters,
                                              values, 4, NULL);
      srcReg->Index = paramIndex;      
      srcReg->File = PROGRAM_NAMED_PARAM;
      needSuffix = GL_FALSE;
   }
   else {
      RETURN_ERROR2("Invalid scalar source argument", token);
   }

   srcReg->Swizzle = 0;
   if (needSuffix) {
      /* parse .[xyzw] suffix */
      if (!Parse_String(parseState, "."))
         RETURN_ERROR1("Expected .");

      if (!Parse_Token(parseState, token))
         RETURN_ERROR;

      if (token[0] == 'x' && token[1] == 0) {
         srcReg->Swizzle = 0;
      }
      else if (token[0] == 'y' && token[1] == 0) {
         srcReg->Swizzle = 1;
      }
      else if (token[0] == 'z' && token[1] == 0) {
         srcReg->Swizzle = 2;
      }
      else if (token[0] == 'w' && token[1] == 0) {
         srcReg->Swizzle = 3;
      }
      else {
         RETURN_ERROR1("Invalid scalar source suffix");
      }
   }

   /* Finish absolute value */
   if (srcReg->Abs && !Parse_String(parseState, "|")) {
      RETURN_ERROR1("Expected |");
   }

   return GL_TRUE;
}


static GLboolean
Parse_PrintInstruction(struct parse_state *parseState,
                       struct prog_instruction *inst)
{
   const GLubyte *str;
   GLubyte *msg;
   GLuint len;
   GLint idx;

   /* The first argument is a literal string 'just like this' */
   if (!Parse_String(parseState, "'"))
      RETURN_ERROR1("Expected '");

   str = parseState->pos;
   for (len = 0; str[len] != '\''; len++) /* find closing quote */
      ;
   parseState->pos += len + 1;
   msg = (GLubyte*) _mesa_malloc(len + 1);

   _mesa_memcpy(msg, str, len);
   msg[len] = 0;
   inst->Data = msg;

   if (Parse_String(parseState, ",")) {
      /* got an optional register to print */
      GLubyte token[100];
      GetToken(parseState, token);
      if (token[0] == 'o') {
         /* dst reg */
         if (!Parse_OutputReg(parseState, &idx))
            RETURN_ERROR;
	 inst->SrcReg[0].Index = idx;
         inst->SrcReg[0].File = PROGRAM_OUTPUT;
      }
      else {
         /* src reg */
         if (!Parse_VectorSrc(parseState, &inst->SrcReg[0]))
            RETURN_ERROR;
      }
   }
   else {
      inst->SrcReg[0].File = PROGRAM_UNDEFINED;
   }

   inst->SrcReg[0].Swizzle = SWIZZLE_NOOP;
   inst->SrcReg[0].NegateBase = NEGATE_NONE;
   inst->SrcReg[0].Abs = GL_FALSE;
   inst->SrcReg[0].NegateAbs = GL_FALSE;

   return GL_TRUE;
}


static GLboolean
Parse_InstructionSequence(struct parse_state *parseState,
                          struct prog_instruction program[])
{
   while (1) {
      struct prog_instruction *inst = program + parseState->numInst;
      struct instruction_pattern instMatch;
      GLubyte token[100];

      /* Initialize the instruction */
      _mesa_init_instructions(inst, 1);

      /* special instructions */
      if (Parse_String(parseState, "DEFINE")) {
         GLubyte id[100];
         GLfloat value[7];  /* yes, 7 to be safe */
         if (!Parse_Identifier(parseState, id))
            RETURN_ERROR;
         /* XXX make sure id is not a reserved identifer, like R9 */
         if (!Parse_String(parseState, "="))
            RETURN_ERROR1("Expected =");
         if (!Parse_VectorOrScalarConstant(parseState, value))
            RETURN_ERROR;
         if (!Parse_String(parseState, ";"))
            RETURN_ERROR1("Expected ;");
         if (_mesa_lookup_parameter_index(parseState->parameters,
                                          -1, (const char *) id) >= 0) {
            RETURN_ERROR2(id, "already defined");
         }
         _mesa_add_named_parameter(parseState->parameters,
                                   (const char *) id, value);
      }
      else if (Parse_String(parseState, "DECLARE")) {
         GLubyte id[100];
         GLfloat value[7] = {0, 0, 0, 0, 0, 0, 0};  /* yes, to be safe */
         if (!Parse_Identifier(parseState, id))
            RETURN_ERROR;
         /* XXX make sure id is not a reserved identifer, like R9 */
         if (Parse_String(parseState, "=")) {
            if (!Parse_VectorOrScalarConstant(parseState, value))
               RETURN_ERROR;
         }
         if (!Parse_String(parseState, ";"))
            RETURN_ERROR1("Expected ;");
         if (_mesa_lookup_parameter_index(parseState->parameters,
                                          -1, (const char *) id) >= 0) {
            RETURN_ERROR2(id, "already declared");
         }
         _mesa_add_named_parameter(parseState->parameters,
                                   (const char *) id, value);
      }
      else if (Parse_String(parseState, "END")) {
         inst->Opcode = OPCODE_END;
         inst->StringPos = parseState->curLine - parseState->start;
         assert(inst->StringPos >= 0);
         parseState->numInst++;
         if (Parse_Token(parseState, token)) {
            RETURN_ERROR1("Code after END opcode.");
         }
         break;
      }
      else {
         /* general/arithmetic instruction */

         /* get token */
         if (!Parse_Token(parseState, token)) {
            RETURN_ERROR1("Missing END instruction.");
         }

         /* try to find matching instuction */
         instMatch = MatchInstruction(token);
         if (instMatch.opcode >= MAX_OPCODE) {
            /* bad instruction name */
            RETURN_ERROR2("Unexpected token: ", token);
         }

         inst->Opcode = instMatch.opcode;
         inst->Precision = instMatch.suffixes & (_R | _H | _X);
         inst->SaturateMode = (instMatch.suffixes & (_S))
            ? SATURATE_ZERO_ONE : SATURATE_OFF;
         inst->CondUpdate = (instMatch.suffixes & (_C)) ? GL_TRUE : GL_FALSE;
         inst->StringPos = parseState->curLine - parseState->start;
         assert(inst->StringPos >= 0);

         /*
          * parse the input and output operands
          */
         if (instMatch.outputs == OUTPUT_S || instMatch.outputs == OUTPUT_V) {
            if (!Parse_MaskedDstReg(parseState, &inst->DstReg))
               RETURN_ERROR;
            if (!Parse_String(parseState, ","))
               RETURN_ERROR1("Expected ,");
         }
         else if (instMatch.outputs == OUTPUT_NONE) {
            if (instMatch.opcode == OPCODE_KIL_NV) {
               /* This is a little weird, the cond code info is in
                * the dest register.
                */
               if (!Parse_CondCodeMask(parseState, &inst->DstReg))
                  RETURN_ERROR;
            }
            else {
               ASSERT(instMatch.opcode == OPCODE_PRINT);
            }
         }

         if (instMatch.inputs == INPUT_1V) {
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[0]))
               RETURN_ERROR;
         }
         else if (instMatch.inputs == INPUT_2V) {
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[0]))
               RETURN_ERROR;
            if (!Parse_String(parseState, ","))
               RETURN_ERROR1("Expected ,");
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[1]))
               RETURN_ERROR;
         }
         else if (instMatch.inputs == INPUT_3V) {
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[0]))
               RETURN_ERROR;
            if (!Parse_String(parseState, ","))
               RETURN_ERROR1("Expected ,");
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[1]))
               RETURN_ERROR;
            if (!Parse_String(parseState, ","))
               RETURN_ERROR1("Expected ,");
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[2]))
               RETURN_ERROR;
         }
         else if (instMatch.inputs == INPUT_1S) {
            if (!Parse_ScalarSrcReg(parseState, &inst->SrcReg[0]))
               RETURN_ERROR;
         }
         else if (instMatch.inputs == INPUT_2S) {
            if (!Parse_ScalarSrcReg(parseState, &inst->SrcReg[0]))
               RETURN_ERROR;
            if (!Parse_String(parseState, ","))
               RETURN_ERROR1("Expected ,");
            if (!Parse_ScalarSrcReg(parseState, &inst->SrcReg[1]))
               RETURN_ERROR;
         }
         else if (instMatch.inputs == INPUT_CC) {
            /* XXX to-do */
         }
         else if (instMatch.inputs == INPUT_1V_T) {
	    GLubyte unit, idx;
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[0]))
               RETURN_ERROR;
            if (!Parse_String(parseState, ","))
               RETURN_ERROR1("Expected ,");
            if (!Parse_TextureImageId(parseState, &unit, &idx))
               RETURN_ERROR;
	    inst->TexSrcUnit = unit;
	    inst->TexSrcTarget = idx;
         }
         else if (instMatch.inputs == INPUT_3V_T) {
	    GLubyte unit, idx;
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[0]))
               RETURN_ERROR;
            if (!Parse_String(parseState, ","))
               RETURN_ERROR1("Expected ,");
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[1]))
               RETURN_ERROR;
            if (!Parse_String(parseState, ","))
               RETURN_ERROR1("Expected ,");
            if (!Parse_VectorSrc(parseState, &inst->SrcReg[2]))
               RETURN_ERROR;
            if (!Parse_String(parseState, ","))
               RETURN_ERROR1("Expected ,");
            if (!Parse_TextureImageId(parseState, &unit, &idx))
               RETURN_ERROR;
	    inst->TexSrcUnit = unit;
	    inst->TexSrcTarget = idx;
         }
         else if (instMatch.inputs == INPUT_1V_S) {
            if (!Parse_PrintInstruction(parseState, inst))
               RETURN_ERROR;
         }

         /* end of statement semicolon */
         if (!Parse_String(parseState, ";"))
            RETURN_ERROR1("Expected ;");

         parseState->numInst++;

         if (parseState->numInst >= MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS)
            RETURN_ERROR1("Program too long");
      }
   }
   return GL_TRUE;
}



/**
 * Parse/compile the 'str' returning the compiled 'program'.
 * ctx->Program.ErrorPos will be -1 if successful.  Otherwise, ErrorPos
 * indicates the position of the error in 'str'.
 */
void
_mesa_parse_nv_fragment_program(GLcontext *ctx, GLenum dstTarget,
                                const GLubyte *str, GLsizei len,
                                struct gl_fragment_program *program)
{
   struct parse_state parseState;
   struct prog_instruction instBuffer[MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS];
   struct prog_instruction *newInst;
   GLenum target;
   GLubyte *programString;

   /* Make a null-terminated copy of the program string */
   programString = (GLubyte *) MALLOC(len + 1);
   if (!programString) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
      return;
   }
   MEMCPY(programString, str, len);
   programString[len] = 0;

   /* Get ready to parse */
   _mesa_bzero(&parseState, sizeof(struct parse_state));
   parseState.ctx = ctx;
   parseState.start = programString;
   parseState.program = program;
   parseState.numInst = 0;
   parseState.curLine = programString;
   parseState.parameters = _mesa_new_parameter_list();

   /* Reset error state */
   _mesa_set_program_error(ctx, -1, NULL);

   /* check the program header */
   if (_mesa_strncmp((const char *) programString, "!!FP1.0", 7) == 0) {
      target = GL_FRAGMENT_PROGRAM_NV;
      parseState.pos = programString + 7;
   }
   else if (_mesa_strncmp((const char *) programString, "!!FCP1.0", 8) == 0) {
      /* fragment / register combiner program - not supported */
      _mesa_set_program_error(ctx, 0, "Invalid fragment program header");
      _mesa_error(ctx, GL_INVALID_OPERATION, "glLoadProgramNV(bad header)");
      return;
   }
   else {
      /* invalid header */
      _mesa_set_program_error(ctx, 0, "Invalid fragment program header");
      _mesa_error(ctx, GL_INVALID_OPERATION, "glLoadProgramNV(bad header)");
      return;
   }

   /* make sure target and header match */
   if (target != dstTarget) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glLoadProgramNV(target mismatch 0x%x != 0x%x)",
                  target, dstTarget);
      return;
   }

   if (Parse_InstructionSequence(&parseState, instBuffer)) {
      GLuint u;
      /* successful parse! */

      if (parseState.outputsWritten == 0) {
         /* must write at least one output! */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "Invalid fragment program - no outputs written.");
         return;
      }

      /* copy the compiled instructions */
      assert(parseState.numInst <= MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS);
      newInst = _mesa_alloc_instructions(parseState.numInst);
      if (!newInst) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
         return;  /* out of memory */
      }
      _mesa_copy_instructions(newInst, instBuffer, parseState.numInst);

      /* install the program */
      program->Base.Target = target;
      if (program->Base.String) {
         FREE(program->Base.String);
      }
      program->Base.String = programString;
      program->Base.Format = GL_PROGRAM_FORMAT_ASCII_ARB;
      if (program->Base.Instructions) {
         _mesa_free(program->Base.Instructions);
      }
      program->Base.Instructions = newInst;
      program->Base.NumInstructions = parseState.numInst;
      program->Base.InputsRead = parseState.inputsRead;
      program->Base.OutputsWritten = parseState.outputsWritten;
      for (u = 0; u < ctx->Const.MaxTextureImageUnits; u++)
         program->Base.TexturesUsed[u] = parseState.texturesUsed[u];

      /* save program parameters */
      program->Base.Parameters = parseState.parameters;

      /* allocate registers for declared program parameters */
#if 00
      _mesa_assign_program_registers(&(program->SymbolTable));
#endif

#ifdef DEBUG_foo
      _mesa_printf("--- glLoadProgramNV(%d) result ---\n", program->Base.Id);
      _mesa_print_nv_fragment_program(program);
      _mesa_printf("----------------------------------\n");
#endif
   }
   else {
      /* Error! */
      _mesa_error(ctx, GL_INVALID_OPERATION, "glLoadProgramNV");
      /* NOTE: _mesa_set_program_error would have been called already */
   }
}


static void
PrintSrcReg(const struct gl_fragment_program *program,
            const struct prog_src_register *src)
{
   static const char comps[5] = "xyzw";

   if (src->NegateAbs) {
      _mesa_printf("-");
   }
   if (src->Abs) {
      _mesa_printf("|");
   }
   if (src->NegateBase) {
      _mesa_printf("-");
   }
   if (src->File == PROGRAM_NAMED_PARAM) {
      if (program->Base.Parameters->Parameters[src->Index].Type
          == PROGRAM_CONSTANT) {
         const GLfloat *v;
         v = program->Base.Parameters->ParameterValues[src->Index];
         _mesa_printf("{%g, %g, %g, %g}", v[0], v[1], v[2], v[3]);
      }
      else {
         ASSERT(program->Base.Parameters->Parameters[src->Index].Type
                == PROGRAM_NAMED_PARAM);
         _mesa_printf("%s", program->Base.Parameters->Parameters[src->Index].Name);
      }
   }
   else if (src->File == PROGRAM_OUTPUT) {
      _mesa_printf("o[%s]", OutputRegisters[src->Index]);
   }
   else if (src->File == PROGRAM_INPUT) {
      _mesa_printf("f[%s]", InputRegisters[src->Index]);
   }
   else if (src->File == PROGRAM_LOCAL_PARAM) {
      _mesa_printf("p[%d]", src->Index);
   }
   else if (src->File == PROGRAM_TEMPORARY) {
      if (src->Index >= 32)
         _mesa_printf("H%d", src->Index);
      else
         _mesa_printf("R%d", src->Index);
   }
   else if (src->File == PROGRAM_WRITE_ONLY) {
      _mesa_printf("%cC", "HR"[src->Index]);
   }
   else {
      _mesa_problem(NULL, "Invalid fragment register %d", src->Index);
      return;
   }
   if (GET_SWZ(src->Swizzle, 0) == GET_SWZ(src->Swizzle, 1) &&
       GET_SWZ(src->Swizzle, 0) == GET_SWZ(src->Swizzle, 2) &&
       GET_SWZ(src->Swizzle, 0) == GET_SWZ(src->Swizzle, 3)) {
      _mesa_printf(".%c", comps[GET_SWZ(src->Swizzle, 0)]);
   }
   else if (src->Swizzle != SWIZZLE_NOOP) {
      _mesa_printf(".%c%c%c%c",
                   comps[GET_SWZ(src->Swizzle, 0)],
                   comps[GET_SWZ(src->Swizzle, 1)],
                   comps[GET_SWZ(src->Swizzle, 2)],
                   comps[GET_SWZ(src->Swizzle, 3)]);
   }
   if (src->Abs) {
      _mesa_printf("|");
   }
}

static void
PrintTextureSrc(const struct prog_instruction *inst)
{
   _mesa_printf("TEX%d, ", inst->TexSrcUnit);
   switch (inst->TexSrcTarget) {
   case TEXTURE_1D_INDEX:
      _mesa_printf("1D");
      break;
   case TEXTURE_2D_INDEX:
      _mesa_printf("2D");
      break;
   case TEXTURE_3D_INDEX:
      _mesa_printf("3D");
      break;
   case TEXTURE_RECT_INDEX:
      _mesa_printf("RECT");
      break;
   case TEXTURE_CUBE_INDEX:
      _mesa_printf("CUBE");
      break;
   default:
      _mesa_problem(NULL, "Invalid textue target in PrintTextureSrc");
   }
}

static void
PrintCondCode(const struct prog_dst_register *dst)
{
   static const char *comps = "xyzw";
   static const char *ccString[] = {
      "??", "GT", "EQ", "LT", "UN", "GE", "LE", "NE", "TR", "FL", "??"
   };

   _mesa_printf("%s", ccString[dst->CondMask]);
   if (GET_SWZ(dst->CondSwizzle, 0) == GET_SWZ(dst->CondSwizzle, 1) &&
       GET_SWZ(dst->CondSwizzle, 0) == GET_SWZ(dst->CondSwizzle, 2) &&
       GET_SWZ(dst->CondSwizzle, 0) == GET_SWZ(dst->CondSwizzle, 3)) {
      _mesa_printf(".%c", comps[GET_SWZ(dst->CondSwizzle, 0)]);
   }
   else if (dst->CondSwizzle != SWIZZLE_NOOP) {
      _mesa_printf(".%c%c%c%c",
                   comps[GET_SWZ(dst->CondSwizzle, 0)],
                   comps[GET_SWZ(dst->CondSwizzle, 1)],
                   comps[GET_SWZ(dst->CondSwizzle, 2)],
                   comps[GET_SWZ(dst->CondSwizzle, 3)]);
   }
}


static void
PrintDstReg(const struct prog_dst_register *dst)
{
   if (dst->File == PROGRAM_OUTPUT) {
      _mesa_printf("o[%s]", OutputRegisters[dst->Index]);
   }
   else if (dst->File == PROGRAM_TEMPORARY) {
      if (dst->Index >= 32)
         _mesa_printf("H%d", dst->Index);
      else
         _mesa_printf("R%d", dst->Index);
   }
   else if (dst->File == PROGRAM_LOCAL_PARAM) {
      _mesa_printf("p[%d]", dst->Index);
   }
   else if (dst->File == PROGRAM_WRITE_ONLY) {
      _mesa_printf("%cC", "HR"[dst->Index]);
   }
   else {
      _mesa_printf("???");
   }

   if (dst->WriteMask != 0 && dst->WriteMask != WRITEMASK_XYZW) {
      _mesa_printf(".");
      if (dst->WriteMask & WRITEMASK_X)
         _mesa_printf("x");
      if (dst->WriteMask & WRITEMASK_Y)
         _mesa_printf("y");
      if (dst->WriteMask & WRITEMASK_Z)
         _mesa_printf("z");
      if (dst->WriteMask & WRITEMASK_W)
         _mesa_printf("w");
   }

   if (dst->CondMask != COND_TR ||
       dst->CondSwizzle != SWIZZLE_NOOP) {
      _mesa_printf(" (");
      PrintCondCode(dst);
      _mesa_printf(")");
   }
}


/**
 * Print (unparse) the given vertex program.  Just for debugging.
 */
void
_mesa_print_nv_fragment_program(const struct gl_fragment_program *program)
{
   const struct prog_instruction *inst;

   for (inst = program->Base.Instructions; inst->Opcode != OPCODE_END; inst++) {
      int i;
      for (i = 0; Instructions[i].name; i++) {
         if (inst->Opcode == Instructions[i].opcode) {
            /* print instruction name */
            _mesa_printf("%s", Instructions[i].name);
            if (inst->Precision == FLOAT16)
               _mesa_printf("H");
            else if (inst->Precision == FIXED12)
               _mesa_printf("X");
            if (inst->CondUpdate)
               _mesa_printf("C");
            if (inst->SaturateMode == SATURATE_ZERO_ONE)
               _mesa_printf("_SAT");
            _mesa_printf(" ");

            if (Instructions[i].inputs == INPUT_CC) {
               PrintCondCode(&inst->DstReg);
            }
            else if (Instructions[i].outputs == OUTPUT_V ||
                     Instructions[i].outputs == OUTPUT_S) {
               /* print dest register */
               PrintDstReg(&inst->DstReg);
               _mesa_printf(", ");
            }

            /* print source register(s) */
            if (Instructions[i].inputs == INPUT_1V ||
                Instructions[i].inputs == INPUT_1S) {
               PrintSrcReg(program, &inst->SrcReg[0]);
            }
            else if (Instructions[i].inputs == INPUT_2V ||
                     Instructions[i].inputs == INPUT_2S) {
               PrintSrcReg(program, &inst->SrcReg[0]);
               _mesa_printf(", ");
               PrintSrcReg(program, &inst->SrcReg[1]);
            }
            else if (Instructions[i].inputs == INPUT_3V) {
               PrintSrcReg(program, &inst->SrcReg[0]);
               _mesa_printf(", ");
               PrintSrcReg(program, &inst->SrcReg[1]);
               _mesa_printf(", ");
               PrintSrcReg(program, &inst->SrcReg[2]);
            }
            else if (Instructions[i].inputs == INPUT_1V_T) {
               PrintSrcReg(program, &inst->SrcReg[0]);
               _mesa_printf(", ");
               PrintTextureSrc(inst);
            }
            else if (Instructions[i].inputs == INPUT_3V_T) {
               PrintSrcReg(program, &inst->SrcReg[0]);
               _mesa_printf(", ");
               PrintSrcReg(program, &inst->SrcReg[1]);
               _mesa_printf(", ");
               PrintSrcReg(program, &inst->SrcReg[2]);
               _mesa_printf(", ");
               PrintTextureSrc(inst);
            }
            _mesa_printf(";\n");
            break;
         }
      }
      if (!Instructions[i].name) {
         _mesa_printf("Invalid opcode %d\n", inst->Opcode);
      }
   }
   _mesa_printf("END\n");
}


const char *
_mesa_nv_fragment_input_register_name(GLuint i)
{
   ASSERT(i < MAX_NV_FRAGMENT_PROGRAM_INPUTS);
   return InputRegisters[i];
}


const char *
_mesa_nv_fragment_output_register_name(GLuint i)
{
   ASSERT(i < MAX_NV_FRAGMENT_PROGRAM_OUTPUTS);
   return OutputRegisters[i];
}
