%{
/*
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "main/glheader.h"
#include "main/imports.h"
#include "program/prog_instruction.h"
#include "program/prog_statevars.h"
#include "program/symbol_table.h"
#include "program/program_parser.h"
#include "program/program_parse.tab.h"

#define require_ARB_vp (yyextra->mode == ARB_vertex)
#define require_ARB_fp (yyextra->mode == ARB_fragment)
#define require_NV_fp  (yyextra->option.NV_fragment)
#define require_shadow (yyextra->option.Shadow)
#define require_rect   (yyextra->option.TexRect)
#define require_texarray        (yyextra->option.TexArray)

#ifndef HAVE_UNISTD_H
#define YY_NO_UNISTD_H
#endif

#define return_token_or_IDENTIFIER(condition, token)	\
   do {							\
      if (condition) {					\
	 return token;					\
      } else {						\
	 return handle_ident(yyextra, yytext, yylval);	\
      }							\
   } while (0)

#define return_token_or_DOT(condition, token)		\
   do {							\
      if (condition) {					\
	 return token;					\
      } else {						\
	 yyless(1);					\
	 return DOT;					\
      }							\
   } while (0)


#define return_opcode(condition, token, opcode, len)	\
   do {							\
      if (condition &&					\
	  _mesa_parse_instruction_suffix(yyextra,	\
					 yytext + len,	\
					 & yylval->temp_inst)) {	\
	 yylval->temp_inst.Opcode = OPCODE_ ## opcode;	\
	 return token;					\
      } else {						\
	 return handle_ident(yyextra, yytext, yylval);	\
      }							\
   } while (0)

#define SWIZZLE_INVAL  MAKE_SWIZZLE4(SWIZZLE_NIL, SWIZZLE_NIL, \
				     SWIZZLE_NIL, SWIZZLE_NIL)

static unsigned
mask_from_char(char c)
{
   switch (c) {
   case 'x':
   case 'r':
      return WRITEMASK_X;
   case 'y':
   case 'g':
      return WRITEMASK_Y;
   case 'z':
   case 'b':
      return WRITEMASK_Z;
   case 'w':
   case 'a':
      return WRITEMASK_W;
   }

   return 0;
}

static unsigned
swiz_from_char(char c)
{
   switch (c) {
   case 'x':
   case 'r':
      return SWIZZLE_X;
   case 'y':
   case 'g':
      return SWIZZLE_Y;
   case 'z':
   case 'b':
      return SWIZZLE_Z;
   case 'w':
   case 'a':
      return SWIZZLE_W;
   }

   return 0;
}

static int
handle_ident(struct asm_parser_state *state, const char *text, YYSTYPE *lval)
{
   lval->string = strdup(text);

   return (_mesa_symbol_table_find_symbol(state->st, 0, text) == NULL)
      ? IDENTIFIER : USED_IDENTIFIER;
}

#define YY_USER_ACTION							\
   do {									\
      yylloc->first_column = yylloc->last_column;			\
      yylloc->last_column += yyleng;					\
      if ((yylloc->first_line == 1)					\
	  && (yylloc->first_column == 1)) {				\
	 yylloc->position = 1;						\
      } else {								\
	 yylloc->position += yylloc->last_column - yylloc->first_column; \
      }									\
   } while(0);

#define YY_NO_INPUT

/* Yes, this is intentionally doing nothing. We have this line of code
here only to avoid the compiler complaining about an unput function
that is defined, but never called. */
#define YY_USER_INIT while (0) { unput(0); }

#define YY_EXTRA_TYPE struct asm_parser_state *

/* Flex defines a couple of functions with no declarations nor the
static keyword. Declare them here to avoid a compiler warning. */
int yyget_column  (yyscan_t yyscanner);
void yyset_column (int  column_no , yyscan_t yyscanner);

%}

num    [0-9]+
exp    [Ee][-+]?[0-9]+
frac   "."[0-9]+
dot    "."[ \t]*

sz     [HRX]?
szf    [HR]?
cc     C?
sat    (_SAT)?

%option bison-bridge bison-locations reentrant noyywrap
%%

"!!ARBvp1.0"              { return ARBvp_10; }
"!!ARBfp1.0"              { return ARBfp_10; }
ADDRESS                   {
   yylval->integer = at_address;
   return_token_or_IDENTIFIER(require_ARB_vp, ADDRESS);
}
ALIAS                     { return ALIAS; }
ATTRIB                    { return ATTRIB; }
END                       { return END; }
OPTION                    { return OPTION; }
OUTPUT                    { return OUTPUT; }
PARAM                     { return PARAM; }
TEMP                      { yylval->integer = at_temp; return TEMP; }

ABS{sz}{cc}{sat}   { return_opcode(             1, VECTOR_OP, ABS, 3); }
ADD{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, ADD, 3); }
ARL                { return_opcode(require_ARB_vp, ARL, ARL, 3); }

CMP{sat}           { return_opcode(require_ARB_fp, TRI_OP, CMP, 3); }
COS{szf}{cc}{sat}  { return_opcode(require_ARB_fp, SCALAR_OP, COS, 3); }

DDX{szf}{cc}{sat}  { return_opcode(require_NV_fp,  VECTOR_OP, DDX, 3); }
DDY{szf}{cc}{sat}  { return_opcode(require_NV_fp,  VECTOR_OP, DDY, 3); }
DP3{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, DP3, 3); }
DP4{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, DP4, 3); }
DPH{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, DPH, 3); }
DST{szf}{cc}{sat}  { return_opcode(             1, BIN_OP, DST, 3); }

EX2{szf}{cc}{sat}  { return_opcode(             1, SCALAR_OP, EX2, 3); }
EXP                { return_opcode(require_ARB_vp, SCALAR_OP, EXP, 3); }

FLR{sz}{cc}{sat}   { return_opcode(             1, VECTOR_OP, FLR, 3); }
FRC{sz}{cc}{sat}   { return_opcode(             1, VECTOR_OP, FRC, 3); }

KIL                { return_opcode(require_ARB_fp, KIL, KIL, 3); }

LIT{szf}{cc}{sat}  { return_opcode(             1, VECTOR_OP, LIT, 3); }
LG2{szf}{cc}{sat}  { return_opcode(             1, SCALAR_OP, LG2, 3); }
LOG                { return_opcode(require_ARB_vp, SCALAR_OP, LOG, 3); }
LRP{sz}{cc}{sat}   { return_opcode(require_ARB_fp, TRI_OP, LRP, 3); }

MAD{sz}{cc}{sat}   { return_opcode(             1, TRI_OP, MAD, 3); }
MAX{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, MAX, 3); }
MIN{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, MIN, 3); }
MOV{sz}{cc}{sat}   { return_opcode(             1, VECTOR_OP, MOV, 3); }
MUL{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, MUL, 3); }

PK2H               { return_opcode(require_NV_fp,  VECTOR_OP, PK2H, 4); }
PK2US              { return_opcode(require_NV_fp,  VECTOR_OP, PK2US, 5); }
PK4B               { return_opcode(require_NV_fp,  VECTOR_OP, PK4B, 4); }
PK4UB              { return_opcode(require_NV_fp,  VECTOR_OP, PK4UB, 5); }
POW{szf}{cc}{sat}  { return_opcode(             1, BINSC_OP, POW, 3); }

RCP{szf}{cc}{sat}  { return_opcode(             1, SCALAR_OP, RCP, 3); }
RFL{szf}{cc}{sat}  { return_opcode(require_NV_fp,  BIN_OP,    RFL, 3); }
RSQ{szf}{cc}{sat}  { return_opcode(             1, SCALAR_OP, RSQ, 3); }

SCS{sat}           { return_opcode(require_ARB_fp, SCALAR_OP, SCS, 3); }
SEQ{sz}{cc}{sat}   { return_opcode(require_NV_fp,  BIN_OP, SEQ, 3); }
SFL{sz}{cc}{sat}   { return_opcode(require_NV_fp,  BIN_OP, SFL, 3); }
SGE{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, SGE, 3); }
SGT{sz}{cc}{sat}   { return_opcode(require_NV_fp,  BIN_OP, SGT, 3); }
SIN{szf}{cc}{sat}  { return_opcode(require_ARB_fp, SCALAR_OP, SIN, 3); }
SLE{sz}{cc}{sat}   { return_opcode(require_NV_fp,  BIN_OP, SLE, 3); }
SLT{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, SLT, 3); }
SNE{sz}{cc}{sat}   { return_opcode(require_NV_fp,  BIN_OP, SNE, 3); }
STR{sz}{cc}{sat}   { return_opcode(require_NV_fp,  BIN_OP, STR, 3); }
SUB{sz}{cc}{sat}   { return_opcode(             1, BIN_OP, SUB, 3); }
SWZ{sat}           { return_opcode(             1, SWZ, SWZ, 3); }

TEX{cc}{sat}       { return_opcode(require_ARB_fp, SAMPLE_OP, TEX, 3); }
TXB{cc}{sat}       { return_opcode(require_ARB_fp, SAMPLE_OP, TXB, 3); }
TXD{cc}{sat}       { return_opcode(require_NV_fp,  TXD_OP, TXD, 3); }
TXP{cc}{sat}       { return_opcode(require_ARB_fp, SAMPLE_OP, TXP, 3); }

UP2H{cc}{sat}      { return_opcode(require_NV_fp,  SCALAR_OP, UP2H, 4); }
UP2US{cc}{sat}     { return_opcode(require_NV_fp,  SCALAR_OP, UP2US, 5); }
UP4B{cc}{sat}      { return_opcode(require_NV_fp,  SCALAR_OP, UP4B, 4); }
UP4UB{cc}{sat}     { return_opcode(require_NV_fp,  SCALAR_OP, UP4UB, 5); }

X2D{szf}{cc}{sat}  { return_opcode(require_NV_fp,  TRI_OP, X2D, 3); }
XPD{sat}           { return_opcode(             1, BIN_OP, XPD, 3); }

vertex                    { return_token_or_IDENTIFIER(require_ARB_vp, VERTEX); }
fragment                  { return_token_or_IDENTIFIER(require_ARB_fp, FRAGMENT); }
program                   { return PROGRAM; }
state                     { return STATE; }
result                    { return RESULT; }

{dot}ambient              { return AMBIENT; }
{dot}attenuation          { return ATTENUATION; }
{dot}back                 { return BACK; }
{dot}clip                 { return_token_or_DOT(require_ARB_vp, CLIP); }
{dot}color                { return COLOR; }
{dot}depth                { return_token_or_DOT(require_ARB_fp, DEPTH); }
{dot}diffuse              { return DIFFUSE; }
{dot}direction            { return DIRECTION; }
{dot}emission             { return EMISSION; }
{dot}env                  { return ENV; }
{dot}eye                  { return EYE; }
{dot}fogcoord             { return FOGCOORD; }
{dot}fog                  { return FOG; }
{dot}front                { return FRONT; }
{dot}half                 { return HALF; }
{dot}inverse              { return INVERSE; }
{dot}invtrans             { return INVTRANS; }
{dot}light                { return LIGHT; }
{dot}lightmodel           { return LIGHTMODEL; }
{dot}lightprod            { return LIGHTPROD; }
{dot}local                { return LOCAL; }
{dot}material             { return MATERIAL; }
{dot}program              { return MAT_PROGRAM; }
{dot}matrix               { return MATRIX; }
{dot}matrixindex          { return_token_or_DOT(require_ARB_vp, MATRIXINDEX); }
{dot}modelview            { return MODELVIEW; }
{dot}mvp                  { return MVP; }
{dot}normal               { return_token_or_DOT(require_ARB_vp, NORMAL); }
{dot}object               { return OBJECT; }
{dot}palette              { return PALETTE; }
{dot}params               { return PARAMS; }
{dot}plane                { return PLANE; }
{dot}point                { return_token_or_DOT(require_ARB_vp, POINT_TOK); }
{dot}pointsize            { return_token_or_DOT(require_ARB_vp, POINTSIZE); }
{dot}position             { return POSITION; }
{dot}primary              { return PRIMARY; }
{dot}projection           { return PROJECTION; }
{dot}range                { return_token_or_DOT(require_ARB_fp, RANGE); }
{dot}row                  { return ROW; }
{dot}scenecolor           { return SCENECOLOR; }
{dot}secondary            { return SECONDARY; }
{dot}shininess            { return SHININESS; }
{dot}size                 { return_token_or_DOT(require_ARB_vp, SIZE_TOK); }
{dot}specular             { return SPECULAR; }
{dot}spot                 { return SPOT; }
{dot}texcoord             { return TEXCOORD; }
{dot}texenv               { return_token_or_DOT(require_ARB_fp, TEXENV); }
{dot}texgen               { return_token_or_DOT(require_ARB_vp, TEXGEN); }
{dot}q                    { return_token_or_DOT(require_ARB_vp, TEXGEN_Q); }
{dot}s                    { return_token_or_DOT(require_ARB_vp, TEXGEN_S); }
{dot}t                    { return_token_or_DOT(require_ARB_vp, TEXGEN_T); }
{dot}texture              { return TEXTURE; }
{dot}transpose            { return TRANSPOSE; }
{dot}attrib               { return_token_or_DOT(require_ARB_vp, VTXATTRIB); }
{dot}weight               { return_token_or_DOT(require_ARB_vp, WEIGHT); }

texture                   { return_token_or_IDENTIFIER(require_ARB_fp, TEXTURE_UNIT); }
1D                        { return_token_or_IDENTIFIER(require_ARB_fp, TEX_1D); }
2D                        { return_token_or_IDENTIFIER(require_ARB_fp, TEX_2D); }
3D                        { return_token_or_IDENTIFIER(require_ARB_fp, TEX_3D); }
CUBE                      { return_token_or_IDENTIFIER(require_ARB_fp, TEX_CUBE); }
RECT                      { return_token_or_IDENTIFIER(require_ARB_fp && require_rect, TEX_RECT); }
SHADOW1D                  { return_token_or_IDENTIFIER(require_ARB_fp && require_shadow, TEX_SHADOW1D); }
SHADOW2D                  { return_token_or_IDENTIFIER(require_ARB_fp && require_shadow, TEX_SHADOW2D); }
SHADOWRECT                { return_token_or_IDENTIFIER(require_ARB_fp && require_shadow && require_rect, TEX_SHADOWRECT); }
ARRAY1D                   { return_token_or_IDENTIFIER(require_ARB_fp && require_texarray, TEX_ARRAY1D); }
ARRAY2D                   { return_token_or_IDENTIFIER(require_ARB_fp && require_texarray, TEX_ARRAY2D); }
ARRAYSHADOW1D             { return_token_or_IDENTIFIER(require_ARB_fp && require_shadow && require_texarray, TEX_ARRAYSHADOW1D); }
ARRAYSHADOW2D             { return_token_or_IDENTIFIER(require_ARB_fp && require_shadow && require_texarray, TEX_ARRAYSHADOW2D); }

[_a-zA-Z$][_a-zA-Z0-9$]*  { return handle_ident(yyextra, yytext, yylval); }

".."                      { return DOT_DOT; }

{num}                     {
   yylval->integer = strtol(yytext, NULL, 10);
   return INTEGER;
}
{num}?{frac}{exp}?        {
   yylval->real = _mesa_strtof(yytext, NULL);
   return REAL;
}
{num}"."/[^.]             {
   yylval->real = _mesa_strtof(yytext, NULL);
   return REAL;
}
{num}{exp}                {
   yylval->real = _mesa_strtof(yytext, NULL);
   return REAL;
}
{num}"."{exp}             {
   yylval->real = _mesa_strtof(yytext, NULL);
   return REAL;
}

".xyzw"                   {
   yylval->swiz_mask.swizzle = SWIZZLE_NOOP;
   yylval->swiz_mask.mask = WRITEMASK_XYZW;
   return MASK4;
}

".xy"[zw]                 {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_XY
      | mask_from_char(yytext[3]);
   return MASK3;
}
".xzw"                    {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_XZW;
   return MASK3;
}
".yzw"                    {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_YZW;
   return MASK3;
}

".x"[yzw]                 {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_X
      | mask_from_char(yytext[2]);
   return MASK2;
}
".y"[zw]                  {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_Y
      | mask_from_char(yytext[2]);
   return MASK2;
}
".zw"                     {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_ZW;
   return MASK2;
}

"."[xyzw]                 {
   const unsigned s = swiz_from_char(yytext[1]);
   yylval->swiz_mask.swizzle = MAKE_SWIZZLE4(s, s, s, s);
   yylval->swiz_mask.mask = mask_from_char(yytext[1]);
   return MASK1; 
}

"."[xyzw]{4}              {
   yylval->swiz_mask.swizzle = MAKE_SWIZZLE4(swiz_from_char(yytext[1]),
					    swiz_from_char(yytext[2]),
					    swiz_from_char(yytext[3]),
					    swiz_from_char(yytext[4]));
   yylval->swiz_mask.mask = 0;
   return SWIZZLE;
}

".rgba"                   {
   yylval->swiz_mask.swizzle = SWIZZLE_NOOP;
   yylval->swiz_mask.mask = WRITEMASK_XYZW;
   return_token_or_DOT(require_ARB_fp, MASK4);
}

".rg"[ba]                 {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_XY
      | mask_from_char(yytext[3]);
   return_token_or_DOT(require_ARB_fp, MASK3);
}
".rba"                    {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_XZW;
   return_token_or_DOT(require_ARB_fp, MASK3);
}
".gba"                    {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_YZW;
   return_token_or_DOT(require_ARB_fp, MASK3);
}

".r"[gba]                 {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_X
      | mask_from_char(yytext[2]);
   return_token_or_DOT(require_ARB_fp, MASK2);
}
".g"[ba]                  {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_Y
      | mask_from_char(yytext[2]);
   return_token_or_DOT(require_ARB_fp, MASK2);
}
".ba"                     {
   yylval->swiz_mask.swizzle = SWIZZLE_INVAL;
   yylval->swiz_mask.mask = WRITEMASK_ZW;
   return_token_or_DOT(require_ARB_fp, MASK2);
}

"."[gba]                  {
   const unsigned s = swiz_from_char(yytext[1]);
   yylval->swiz_mask.swizzle = MAKE_SWIZZLE4(s, s, s, s);
   yylval->swiz_mask.mask = mask_from_char(yytext[1]);
   return_token_or_DOT(require_ARB_fp, MASK1);
}


".r"                      {
   if (require_ARB_vp) {
      return TEXGEN_R;
   } else {
      yylval->swiz_mask.swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X,
						SWIZZLE_X, SWIZZLE_X);
      yylval->swiz_mask.mask = WRITEMASK_X;
      return MASK1;
   }
}

"."[rgba]{4}              {
   yylval->swiz_mask.swizzle = MAKE_SWIZZLE4(swiz_from_char(yytext[1]),
					    swiz_from_char(yytext[2]),
					    swiz_from_char(yytext[3]),
					    swiz_from_char(yytext[4]));
   yylval->swiz_mask.mask = 0;
   return_token_or_DOT(require_ARB_fp, SWIZZLE);
}

"."                       { return DOT; }

\n                        {
   yylloc->first_line++;
   yylloc->first_column = 1;
   yylloc->last_line++;
   yylloc->last_column = 1;
   yylloc->position++;
}
[ \t\r]+                  /* eat whitespace */ ;
#.*$                      /* eat comments */ ;
.                         { return yytext[0]; }
%%

void
_mesa_program_lexer_ctor(void **scanner, struct asm_parser_state *state,
			 const char *string, size_t len)
{
   yylex_init_extra(state, scanner);
   yy_scan_bytes(string, len, *scanner);
}

void
_mesa_program_lexer_dtor(void *scanner)
{
   yylex_destroy(scanner);
}
