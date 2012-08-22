%{
/*
 * Copyright © 2008, 2009 Intel Corporation
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
#include <ctype.h>
#include <limits.h>
#include "strtod.h"
#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_parser.h"

static int classify_identifier(struct _mesa_glsl_parse_state *, const char *);

#ifdef _MSC_VER
#define YY_NO_UNISTD_H
#endif

#define YY_USER_ACTION						\
   do {								\
      yylloc->source = 0;					\
      yylloc->first_column = yycolumn + 1;			\
      yylloc->first_line = yylineno + 1;			\
      yycolumn += yyleng;					\
   } while(0);

#define YY_USER_INIT yylineno = 0; yycolumn = 0;

/* A macro for handling reserved words and keywords across language versions.
 *
 * Certain words start out as identifiers, become reserved words in
 * later language revisions, and finally become language keywords.
 *
 * For example, consider the following lexer rule:
 * samplerBuffer       KEYWORD(130, 140, SAMPLERBUFFER)
 *
 * This means that "samplerBuffer" will be treated as:
 * - a keyword (SAMPLERBUFFER token)         ...in GLSL >= 1.40
 * - a reserved word - error                 ...in GLSL >= 1.30
 * - an identifier                           ...in GLSL <  1.30
 */
#define KEYWORD(reserved_version, allowed_version, token)		\
   do {									\
      if (yyextra->language_version >= allowed_version) {		\
	 return token;							\
      } else if (yyextra->language_version >= reserved_version) {	\
	 _mesa_glsl_error(yylloc, yyextra,				\
			  "Illegal use of reserved word `%s'", yytext);	\
	 return ERROR_TOK;						\
      } else {								\
	 yylval->identifier = strdup(yytext);				\
	 return classify_identifier(yyextra, yytext);			\
      }									\
   } while (0)

/* The ES macro can be used in KEYWORD checks:
 *
 *    word      KEYWORD(110 || ES, 400, TOKEN)
 * ...means the word is reserved in GLSL ES 1.00, while
 *
 *    word      KEYWORD(110, 130 || ES, TOKEN)
 * ...means the word is a legal keyword in GLSL ES 1.00.
 */
#define ES yyextra->es_shader

static int
literal_integer(char *text, int len, struct _mesa_glsl_parse_state *state,
		YYSTYPE *lval, YYLTYPE *lloc, int base)
{
   bool is_uint = (text[len - 1] == 'u' ||
		   text[len - 1] == 'U');
   const char *digits = text;

   /* Skip "0x" */
   if (base == 16)
      digits += 2;

#ifdef _MSC_VER
   unsigned __int64 value = _strtoui64(digits, NULL, base);
#else
   unsigned long long value = strtoull(digits, NULL, base);
#endif

   lval->n = (int)value;

   if (value > UINT_MAX) {
      /* Note that signed 0xffffffff is valid, not out of range! */
      if (state->language_version >= 130) {
	 _mesa_glsl_error(lloc, state,
			  "Literal value `%s' out of range", text);
      } else {
	 _mesa_glsl_warning(lloc, state,
			    "Literal value `%s' out of range", text);
      }
   } else if (base == 10 && !is_uint && (unsigned)value > (unsigned)INT_MAX + 1) {
      /* Tries to catch unintentionally providing a negative value.
       * Note that -2147483648 is parsed as -(2147483648), so we don't
       * want to warn for INT_MAX.
       */
      _mesa_glsl_warning(lloc, state,
			 "Signed literal value `%s' is interpreted as %d",
			 text, lval->n);
   }
   return is_uint ? UINTCONSTANT : INTCONSTANT;
}

#define LITERAL_INTEGER(base) \
   literal_integer(yytext, yyleng, yyextra, yylval, yylloc, base)

%}

%option bison-bridge bison-locations reentrant noyywrap
%option nounput noyy_top_state
%option never-interactive
%option prefix="_mesa_glsl_"
%option extra-type="struct _mesa_glsl_parse_state *"

%x PP PRAGMA

DEC_INT		[1-9][0-9]*
HEX_INT		0[xX][0-9a-fA-F]+
OCT_INT		0[0-7]*
INT		({DEC_INT}|{HEX_INT}|{OCT_INT})
SPC		[ \t]*
SPCP		[ \t]+
HASH		^{SPC}#{SPC}
%%

[ \r\t]+		;

    /* Preprocessor tokens. */ 
^[ \t]*#[ \t]*$			;
^[ \t]*#[ \t]*version		{ BEGIN PP; return VERSION_TOK; }
^[ \t]*#[ \t]*extension		{ BEGIN PP; return EXTENSION; }
{HASH}line{SPCP}{INT}{SPCP}{INT}{SPC}$ {
				   /* Eat characters until the first digit is
				    * encountered
				    */
				   char *ptr = yytext;
				   while (!isdigit(*ptr))
				      ptr++;

				   /* Subtract one from the line number because
				    * yylineno is zero-based instead of
				    * one-based.
				    */
				   yylineno = strtol(ptr, &ptr, 0) - 1;
				   yylloc->source = strtol(ptr, NULL, 0);
				}
{HASH}line{SPCP}{INT}{SPC}$	{
				   /* Eat characters until the first digit is
				    * encountered
				    */
				   char *ptr = yytext;
				   while (!isdigit(*ptr))
				      ptr++;

				   /* Subtract one from the line number because
				    * yylineno is zero-based instead of
				    * one-based.
				    */
				   yylineno = strtol(ptr, &ptr, 0) - 1;
				}
^{SPC}#{SPC}pragma{SPCP}debug{SPC}\({SPC}on{SPC}\) {
				  BEGIN PP;
				  return PRAGMA_DEBUG_ON;
				}
^{SPC}#{SPC}pragma{SPCP}debug{SPC}\({SPC}off{SPC}\) {
				  BEGIN PP;
				  return PRAGMA_DEBUG_OFF;
				}
^{SPC}#{SPC}pragma{SPCP}optimize{SPC}\({SPC}on{SPC}\) {
				  BEGIN PP;
				  return PRAGMA_OPTIMIZE_ON;
				}
^{SPC}#{SPC}pragma{SPCP}optimize{SPC}\({SPC}off{SPC}\) {
				  BEGIN PP;
				  return PRAGMA_OPTIMIZE_OFF;
				}
^{SPC}#{SPC}pragma{SPCP}STDGL{SPCP}invariant{SPC}\({SPC}all{SPC}\) {
				  BEGIN PP;
				  return PRAGMA_INVARIANT_ALL;
				}
^{SPC}#{SPC}pragma{SPCP}	{ BEGIN PRAGMA; }

<PRAGMA>\n			{ BEGIN 0; yylineno++; yycolumn = 0; }
<PRAGMA>.			{ }

<PP>\/\/[^\n]*			{ }
<PP>[ \t\r]*			{ }
<PP>:				return COLON;
<PP>[_a-zA-Z][_a-zA-Z0-9]*	{
				   yylval->identifier = strdup(yytext);
				   return IDENTIFIER;
				}
<PP>[1-9][0-9]*			{
				    yylval->n = strtol(yytext, NULL, 10);
				    return INTCONSTANT;
				}
<PP>\n				{ BEGIN 0; yylineno++; yycolumn = 0; return EOL; }

\n		{ yylineno++; yycolumn = 0; }

attribute	return ATTRIBUTE;
const		return CONST_TOK;
bool		return BOOL_TOK;
float		return FLOAT_TOK;
int		return INT_TOK;
uint		KEYWORD(130, 130, UINT_TOK);

break		return BREAK;
continue	return CONTINUE;
do		return DO;
while		return WHILE;
else		return ELSE;
for		return FOR;
if		return IF;
discard		return DISCARD;
return		return RETURN;

bvec2		return BVEC2;
bvec3		return BVEC3;
bvec4		return BVEC4;
ivec2		return IVEC2;
ivec3		return IVEC3;
ivec4		return IVEC4;
uvec2		KEYWORD(130, 130, UVEC2);
uvec3		KEYWORD(130, 130, UVEC3);
uvec4		KEYWORD(130, 130, UVEC4);
vec2		return VEC2;
vec3		return VEC3;
vec4		return VEC4;
mat2		return MAT2X2;
mat3		return MAT3X3;
mat4		return MAT4X4;
mat2x2		KEYWORD(120, 120, MAT2X2);
mat2x3		KEYWORD(120, 120, MAT2X3);
mat2x4		KEYWORD(120, 120, MAT2X4);
mat3x2		KEYWORD(120, 120, MAT3X2);
mat3x3		KEYWORD(120, 120, MAT3X3);
mat3x4		KEYWORD(120, 120, MAT3X4);
mat4x2		KEYWORD(120, 120, MAT4X2);
mat4x3		KEYWORD(120, 120, MAT4X3);
mat4x4		KEYWORD(120, 120, MAT4X4);

in		return IN_TOK;
out		return OUT_TOK;
inout		return INOUT_TOK;
uniform		return UNIFORM;
varying		return VARYING;
centroid	KEYWORD(120, 120, CENTROID);
invariant	KEYWORD(120 || ES, 120 || ES, INVARIANT);
flat		KEYWORD(130 || ES, 130, FLAT);
smooth		KEYWORD(130, 130, SMOOTH);
noperspective	KEYWORD(130, 130, NOPERSPECTIVE);

sampler1D	return SAMPLER1D;
sampler2D	return SAMPLER2D;
sampler3D	return SAMPLER3D;
samplerCube	return SAMPLERCUBE;
sampler1DArray	KEYWORD(130, 130, SAMPLER1DARRAY);
sampler2DArray	KEYWORD(130, 130, SAMPLER2DARRAY);
sampler1DShadow	return SAMPLER1DSHADOW;
sampler2DShadow	return SAMPLER2DSHADOW;
samplerCubeShadow	KEYWORD(130, 130, SAMPLERCUBESHADOW);
sampler1DArrayShadow	KEYWORD(130, 130, SAMPLER1DARRAYSHADOW);
sampler2DArrayShadow	KEYWORD(130, 130, SAMPLER2DARRAYSHADOW);
isampler1D		KEYWORD(130, 130, ISAMPLER1D);
isampler2D		KEYWORD(130, 130, ISAMPLER2D);
isampler3D		KEYWORD(130, 130, ISAMPLER3D);
isamplerCube		KEYWORD(130, 130, ISAMPLERCUBE);
isampler1DArray		KEYWORD(130, 130, ISAMPLER1DARRAY);
isampler2DArray		KEYWORD(130, 130, ISAMPLER2DARRAY);
usampler1D		KEYWORD(130, 130, USAMPLER1D);
usampler2D		KEYWORD(130, 130, USAMPLER2D);
usampler3D		KEYWORD(130, 130, USAMPLER3D);
usamplerCube		KEYWORD(130, 130, USAMPLERCUBE);
usampler1DArray		KEYWORD(130, 130, USAMPLER1DARRAY);
usampler2DArray		KEYWORD(130, 130, USAMPLER2DARRAY);

samplerExternalOES	{
			  if (yyextra->OES_EGL_image_external_enable)
			     return SAMPLEREXTERNALOES;
			  else
			     return IDENTIFIER;
			}


struct		return STRUCT;
void		return VOID_TOK;

layout		{
		  if ((yyextra->language_version >= 140)
		      || yyextra->AMD_conservative_depth_enable
		      || yyextra->ARB_conservative_depth_enable
		      || yyextra->ARB_explicit_attrib_location_enable
		      || yyextra->ARB_fragment_coord_conventions_enable) {
		      return LAYOUT_TOK;
		   } else {
		      yylval->identifier = strdup(yytext);
		      return IDENTIFIER;
		   }
		}

\+\+		return INC_OP;
--		return DEC_OP;
\<=		return LE_OP;
>=		return GE_OP;
==		return EQ_OP;
!=		return NE_OP;
&&		return AND_OP;
\|\|		return OR_OP;
"^^"		return XOR_OP;
"<<"		return LEFT_OP;
">>"		return RIGHT_OP;

\*=		return MUL_ASSIGN;
\/=		return DIV_ASSIGN;
\+=		return ADD_ASSIGN;
\%=		return MOD_ASSIGN;
\<\<=		return LEFT_ASSIGN;
>>=		return RIGHT_ASSIGN;
&=		return AND_ASSIGN;
"^="		return XOR_ASSIGN;
\|=		return OR_ASSIGN;
-=		return SUB_ASSIGN;

[1-9][0-9]*[uU]?	{
			    return LITERAL_INTEGER(10);
			}
0[xX][0-9a-fA-F]+[uU]?	{
			    return LITERAL_INTEGER(16);
			}
0[0-7]*[uU]?		{
			    return LITERAL_INTEGER(8);
			}

[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?[fF]?	{
			    yylval->real = glsl_strtod(yytext, NULL);
			    return FLOATCONSTANT;
			}
\.[0-9]+([eE][+-]?[0-9]+)?[fF]?		{
			    yylval->real = glsl_strtod(yytext, NULL);
			    return FLOATCONSTANT;
			}
[0-9]+\.([eE][+-]?[0-9]+)?[fF]?		{
			    yylval->real = glsl_strtod(yytext, NULL);
			    return FLOATCONSTANT;
			}
[0-9]+[eE][+-]?[0-9]+[fF]?		{
			    yylval->real = glsl_strtod(yytext, NULL);
			    return FLOATCONSTANT;
			}
[0-9]+[fF]		{
			    yylval->real = glsl_strtod(yytext, NULL);
			    return FLOATCONSTANT;
			}

true			{
			    yylval->n = 1;
			    return BOOLCONSTANT;
			}
false			{
			    yylval->n = 0;
			    return BOOLCONSTANT;
			}


    /* Reserved words in GLSL 1.10. */
asm		KEYWORD(110 || ES, 999, ASM);
class		KEYWORD(110 || ES, 999, CLASS);
union		KEYWORD(110 || ES, 999, UNION);
enum		KEYWORD(110 || ES, 999, ENUM);
typedef		KEYWORD(110 || ES, 999, TYPEDEF);
template	KEYWORD(110 || ES, 999, TEMPLATE);
this		KEYWORD(110 || ES, 999, THIS);
packed		KEYWORD(110 || ES, 999, PACKED_TOK);
goto		KEYWORD(110 || ES, 999, GOTO);
switch		KEYWORD(110 || ES, 130, SWITCH);
default		KEYWORD(110 || ES, 130, DEFAULT);
inline		KEYWORD(110 || ES, 999, INLINE_TOK);
noinline	KEYWORD(110 || ES, 999, NOINLINE);
volatile	KEYWORD(110 || ES, 999, VOLATILE);
public		KEYWORD(110 || ES, 999, PUBLIC_TOK);
static		KEYWORD(110 || ES, 999, STATIC);
extern		KEYWORD(110 || ES, 999, EXTERN);
external	KEYWORD(110 || ES, 999, EXTERNAL);
interface	KEYWORD(110 || ES, 999, INTERFACE);
long		KEYWORD(110 || ES, 999, LONG_TOK);
short		KEYWORD(110 || ES, 999, SHORT_TOK);
double		KEYWORD(110 || ES, 400, DOUBLE_TOK);
half		KEYWORD(110 || ES, 999, HALF);
fixed		KEYWORD(110 || ES, 999, FIXED_TOK);
unsigned	KEYWORD(110 || ES, 999, UNSIGNED);
input		KEYWORD(110 || ES, 999, INPUT_TOK);
output		KEYWORD(110 || ES, 999, OUTPUT);
hvec2		KEYWORD(110 || ES, 999, HVEC2);
hvec3		KEYWORD(110 || ES, 999, HVEC3);
hvec4		KEYWORD(110 || ES, 999, HVEC4);
dvec2		KEYWORD(110 || ES, 400, DVEC2);
dvec3		KEYWORD(110 || ES, 400, DVEC3);
dvec4		KEYWORD(110 || ES, 400, DVEC4);
fvec2		KEYWORD(110 || ES, 999, FVEC2);
fvec3		KEYWORD(110 || ES, 999, FVEC3);
fvec4		KEYWORD(110 || ES, 999, FVEC4);
sampler2DRect		return SAMPLER2DRECT;
sampler3DRect		KEYWORD(110 || ES, 999, SAMPLER3DRECT);
sampler2DRectShadow	return SAMPLER2DRECTSHADOW;
sizeof		KEYWORD(110 || ES, 999, SIZEOF);
cast		KEYWORD(110 || ES, 999, CAST);
namespace	KEYWORD(110 || ES, 999, NAMESPACE);
using		KEYWORD(110 || ES, 999, USING);

    /* Additional reserved words in GLSL 1.20. */
lowp		KEYWORD(120, 130 || ES, LOWP);
mediump		KEYWORD(120, 130 || ES, MEDIUMP);
highp		KEYWORD(120, 130 || ES, HIGHP);
precision	KEYWORD(120, 130 || ES, PRECISION);

    /* Additional reserved words in GLSL 1.30. */
case		KEYWORD(130, 130, CASE);
common		KEYWORD(130, 999, COMMON);
partition	KEYWORD(130, 999, PARTITION);
active		KEYWORD(130, 999, ACTIVE);
superp		KEYWORD(130 || ES, 999, SUPERP);
samplerBuffer	KEYWORD(130, 140, SAMPLERBUFFER);
filter		KEYWORD(130, 999, FILTER);
image1D		KEYWORD(130, 999, IMAGE1D);
image2D		KEYWORD(130, 999, IMAGE2D);
image3D		KEYWORD(130, 999, IMAGE3D);
imageCube	KEYWORD(130, 999, IMAGECUBE);
iimage1D	KEYWORD(130, 999, IIMAGE1D);
iimage2D	KEYWORD(130, 999, IIMAGE2D);
iimage3D	KEYWORD(130, 999, IIMAGE3D);
iimageCube	KEYWORD(130, 999, IIMAGECUBE);
uimage1D	KEYWORD(130, 999, UIMAGE1D);
uimage2D	KEYWORD(130, 999, UIMAGE2D);
uimage3D	KEYWORD(130, 999, UIMAGE3D);
uimageCube	KEYWORD(130, 999, UIMAGECUBE);
image1DArray	KEYWORD(130, 999, IMAGE1DARRAY);
image2DArray	KEYWORD(130, 999, IMAGE2DARRAY);
iimage1DArray	KEYWORD(130, 999, IIMAGE1DARRAY);
iimage2DArray	KEYWORD(130, 999, IIMAGE2DARRAY);
uimage1DArray	KEYWORD(130, 999, UIMAGE1DARRAY);
uimage2DArray	KEYWORD(130, 999, UIMAGE2DARRAY);
image1DShadow	KEYWORD(130, 999, IMAGE1DSHADOW);
image2DShadow	KEYWORD(130, 999, IMAGE2DSHADOW);
image1DArrayShadow KEYWORD(130, 999, IMAGE1DARRAYSHADOW);
image2DArrayShadow KEYWORD(130, 999, IMAGE2DARRAYSHADOW);
imageBuffer	KEYWORD(130, 999, IMAGEBUFFER);
iimageBuffer	KEYWORD(130, 999, IIMAGEBUFFER);
uimageBuffer	KEYWORD(130, 999, UIMAGEBUFFER);
row_major	KEYWORD(130, 999, ROW_MAJOR);

[_a-zA-Z][_a-zA-Z0-9]*	{
			    struct _mesa_glsl_parse_state *state = yyextra;
			    void *ctx = state;	
			    yylval->identifier = ralloc_strdup(ctx, yytext);
			    return classify_identifier(state, yytext);
			}

.			{ return yytext[0]; }

%%

int
classify_identifier(struct _mesa_glsl_parse_state *state, const char *name)
{
   if (state->symbols->get_variable(name) || state->symbols->get_function(name))
      return IDENTIFIER;
   else if (state->symbols->get_type(name))
      return TYPE_IDENTIFIER;
   else
      return NEW_IDENTIFIER;
}

void
_mesa_glsl_lexer_ctor(struct _mesa_glsl_parse_state *state, const char *string)
{
   yylex_init_extra(state, & state->scanner);
   yy_scan_string(string, state->scanner);
}

void
_mesa_glsl_lexer_dtor(struct _mesa_glsl_parse_state *state)
{
   yylex_destroy(state->scanner);
}
