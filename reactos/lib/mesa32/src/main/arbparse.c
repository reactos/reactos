/*
 * Mesa 3-D graphics library
 * Version:  6.0
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

#define DEBUG_PARSING 0

/**
 * \file arbparse.c
 * ARB_*_program parser core
 * \author Michal Krol, Karl Rasche
 */

#include "mtypes.h"
#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "program.h"
#include "nvvertprog.h"
#include "nvfragprog.h"
#include "arbparse.h"

/* TODO:
 *    Fragment Program Stuff:
 *    -----------------------------------------------------
 *
 *    - things from Michal's email
 *       + overflow on atoi
 *       + not-overflowing floats (don't use parse_integer..)
 *       + can remove range checking in arbparse.c        
 *       
 *    - check all limits of number of various variables
 *      + parameters
 *
 *    - test! test! test!    
 *
 *    Vertex Program Stuff:
 *    -----------------------------------------------------
 *    - Optimize param array usage and count limits correctly, see spec,
 *         section 2.14.3.7        
 *       + Record if an array is reference absolutly or relatively (or both)
 *       + For absolute arrays, store a bitmap of accesses
 *       + For single parameters, store an access flag
 *       + After parsing, make a parameter cleanup and merging pass, where
 *           relative arrays are layed out first, followed by abs arrays, and
 *           finally single state.
 *       + Remap offsets for param src and dst registers
 *       + Now we can properly count parameter usage
 *                                                                         
 *    - Multiple state binding errors in param arrays (see spec, just before
 *         section 2.14.3.3)
 *    - grep for XXX
 *    
 *    Mesa Stuff
 *    -----------------------------------------------------
 *    - User clipping planes vs. PositionInvariant 
 *    - Is it sufficient to just multiply by the mvp to transform in the
 *        PositionInvariant case? Or do we need something more involved?
 *                    
 *    - vp_src swizzle is GLubyte, fp_src swizzle is GLuint
 *    - fetch state listed in program_parameters list
 *       + WTF should this go???
 *       + currently in nvvertexec.c and s_nvfragprog.c 
 *
 *    - allow for multiple address registers (and fetch address regs properly)
 *
 *    Cosmetic Stuff
 *    -----------------------------------------------------
 * 	- remove any leftover unused grammer.c stuff (dict_ ?)
 * 	- fix grammer.c error handling so its not static
 * 	- #ifdef around stuff pertaining to extentions
 *
 *    Outstanding Questions:
 *    -----------------------------------------------------
 *    - ARB_matrix_palette / ARB_vertex_blend -- not supported 
 *      what gets hacked off because of this:
 *       + VERTEX_ATTRIB_MATRIXINDEX          
 *       + VERTEX_ATTRIB_WEIGHT
 *       + MATRIX_MODELVIEW
 *       + MATRIX_PALETTE       
 *
 *    - When can we fetch env/local params from their own register files, and
 *      when to we have to fetch them into the main state register file?
 *      (think arrays)
 *
 *    Grammar Changes:
 *    -----------------------------------------------------
 */

typedef GLubyte *production;

/*-----------------------------------------------------------------------
 * From here on down is the syntax checking portion 
 */

/* VERSION: 0.4 */

/*
INTRODUCTION
------------

The task is to check the syntax of an input string. Input string is a
stream of ASCII characters terminated with null-character
('\0'). Checking it using C language is difficult and hard to
implement without bugs. It is hard to maintain and change prior to
further syntax changes.

This is because of high redundancy of the C code. Large blocks of code
are duplicated with only small changes. Even using macros does not
solve the problem, because macros cannot erase the complexity of the
code.

The resolution is to create a new language that will be highly
oriented to our task. Once we describe particular syntax, we are
done. We can then focus on the code that implements the language. The
size and complexity of it is relatively small than the code that
directly checks the syntax.

First, we must implement our new language. Here, the language is
implemented in C, but it could also be implemented in any other
language. The code is listed below. We must take a good care that it
is bug free. This is simple because the code is simple and clean.

Next, we must describe the syntax of our new language in itself. Once
created and checked manually that it is correct, we can use it to
check another scripts.

Note that our new language loading code does not have to check the
syntax. It is because we assume that the script describing itself is
correct, and other scripts can be syntactically checked by the former
script. The loading code must only do semantic checking which leads us
to simple resolving references.

THE LANGUAGE
------------

Here I will describe the syntax of the new language (further called
"Synek"). It is mainly a sequence of declarations terminated by a
semicolon. The declaration consists of a symbol, which is an
identifier, and its definition. A definition is in turn a sequence of
specifiers connected with ".and" or ".or" operator. These operators
cannot be mixed together in a one definition. Specifier can be a
symbol, string, character, character range or a special keyword
".true" or ".false".

On the very beginning of the script there is a declaration of a root
symbol and is in the form:
		.syntax <root_symbol>;

The <root_symbol> must be on of the symbols in declaration
sequence. The syntax is correct if the root symbol evaluates to
true. A symbol evaluates to true if the definition associated with the
symbol evaluates to true. Definition evaluation depends on the
operator used to connect specifiers in the definition. If ".and"
operator is used, definition evaluates to true if and only if all the
specifiers evaluate to true. If ".or" operator is used, definition
evalutes to true if any of the specifiers evaluates to true. If
definition contains only one specifier, it is evaluated as if it was
connected with ".true" keyword by ".and" operator.

If specifier is a ".true" keyword, it always evaluates to true.

If specifier is a ".false" keyword, it always evaluates to
false. Specifier evaluates to false when it does not evaluate to true.

Character range specifier is in the form:
	'<first_character>' - '<second_character>'

If specifier is a character range, it evaluates to true if character
in the stream is greater or equal to <first_character> and less or
equal to <second_character>. In that situation the stream pointer is
advanced to point to next character in the stream. All C-style escape
sequences are supported although trigraph sequences are not. The
comparisions are performed on 8-bit unsigned integers.

Character specifier is in the form:
	'<single_character>'

It evaluates to true if the following character range specifier evaluates to
true:
	'<single_character>' - '<single_character>'

String specifier is in the form:
		"<string>"

Let N be the number of characters in <string>. Let <string>[i]
designate i-th character in <string>. Then the string specifier
evaluates to true if and only if for i in the range [0, N) the
following character specifier evaluates to true:
	'<string>[i]'

If <string>[i] is a quotation mark, '<string>[i]' is replaced with
'\<string>[i]'.

Symbol specifier can be optionally preceded by a ".loop" keyword in the form:
	.loop <symbol>					(1)
	where <symbol> is defined as follows:
		<symbol> <definition>;			(2)
	Construction (1) is replaced by the following code:
		<symbol$1>
	and declaration (2) is replaced by the following:
		<symbol$1> <symbol$2> .or .true;
		<symbol$2> <symbol> .and <symbol$1>;
		<symbol> <definition>;


ESCAPE SEQUENCES
----------------

Synek supports all escape sequences in character specifiers. The
mapping table is listed below.  All occurences of the characters in
the first column are replaced with the corresponding character in the
second column.

	Escape sequence			Represents
	-----------------------------------------------------------------------
	\a				Bell (alert)
	\b				Backspace
	\f				Formfeed
	\n				New line
	\r				Carriage return
	\t				Horizontal tab
	\v				Vertical tab
	\'				Single quotation mark
	\"				Double quotation mark
	\\				Backslash
	\?				Literal question mark
	\ooo				ASCII character in octal notation
	\xhhh				ASCII character in hexadecimal notation
	-----------------------------------------------------------------------


RAISING ERRORS
--------------

Any specifier can be followed by a special construction that is
executed when the specifier evaluates to false. The construction is in
the form:
	.error <ERROR_TEXT>

<ERROR_TEXT> is an identifier declared earlier by error text
declaration. The declaration is in the form:

	.errtext <ERROR_TEXT> "<error_desc>"

When specifier evaluates to false and this construction is present,
parsing is stopped immediately and <error_desc> is returned as a
result of parsing. The error position is also returned and it is meant
as an offset from the beggining of the stream to the character that
was valid so far. Example:

	(**** syntax script ****)

	.syntax program;
	.errtext MISSING_SEMICOLON		"missing ';'"
	program			declaration .and .loop space .and ';'
                                .error MISSING_SEMICOLON .and
					.loop space .and '\0';
	declaration		"declare" .and .loop space .and identifier;
	space			' ';
		(**** sample code ****)
	declare foo ,

In the example above checking the sample code will result in error
message "missing ';'" and error position 12. The sample code is not
correct. Note the presence of '\0' specifier to assure that there is
no code after semicolon - only spaces.  <error_desc> can optionally
contain identifier surrounded by dollar signs $. In such a case, the
identifier and dollar signs are replaced by a string retrieved by
invoking symbol with the identifier name. The starting position is the
error position. The lenght of the resulting string is the position
after invoking the symbol.


PRODUCTION
----------

Synek not only checks the syntax but it can also produce (emit) bytes
associated with specifiers that evaluate to true. That is, every
specifier and optional error construction can be followed by a number
of emit constructions that are in the form:
	.emit <parameter>

<paramater> can be a HEX number, identifier, a star * or a dollar
$. HEX number is preceded by 0x or 0X. If <parameter> is an
identifier, it must be earlier declared by emit code declaration in
the form:
		.emtcode <identifier> <hex_number>

When given specifier evaluates to true, all emits associated with the
specifier are output in order they were declared. A star means that
last-read character should be output instead of constant
value. Example:

	(**** syntax script ****)

	.syntax foobar;
	.emtcode WORD_FOO		0x01
	.emtcode WORD_BAR		0x02
	foobar		FOO .emit WORD_FOO .or BAR .emit WORD_BAR .or .true .emit 0x00;
	FOO			"foo" .and SPACE;
	BAR			"bar" .and SPACE;
	SPACE		' ' .or '\0';

	(**** sample text 1 ****)

	foo

	(**** sample text 2 ****)

	foobar

For both samples the result will be one-element array. For first
sample text it will be value 1, for second - 0. Note that every text
will be accepted because of presence of .true as an alternative.

Another example:

	(**** syntax script ****)

	.syntax declaration;
	.emtcode VARIABLE		0x01
	declaration		"declare" .and .loop space .and
				identifier .emit VARIABLE .and		(1)
					.true .emit 0x00 .and		(2)
					.loop space .and ';';
	space			' ' .or '\t';
	identifier		.loop id_char .emit *;			(3)
	id_char			'a'-'z' .or 'A'-'Z' .or '_';
		(**** sample code ****)
	declare    fubar;

In specifier (1) symbol <identifier> is followed by .emit VARIABLE. If
it evaluates to true, VARIABLE constant and then production of the
symbol is output. Specifier (2) is used to terminate the string with
null to signal when the string ends. Specifier (3) outputs all
characters that make declared identifier. The result of sample code
will be the following array:
		{ 1, 'f', 'u', 'b', 'a', 'r', 0 }

If .emit is followed by dollar $, it means that current position
should be output. Current position is a 32-bit unsigned integer
distance from the very beginning of the parsed string to first
character consumed by the specifier associated with the .emit
instruction. Current position is stored in the output buffer in
Little-Endian convention (the lowest byte comes first).  */

/**
 * This is the text describing the rules to parse the grammar 
 */
#include "arbparse_syn.h"

/** 
 * These should match up with the values defined in arbparse.syn.h
 */

/*
    Changes:
    - changed and merged V_* and F_* opcode values to OP_*.
*/
#define REVISION                                   0x05

/* program type */
#define FRAGMENT_PROGRAM                           0x01
#define VERTEX_PROGRAM                             0x02

/* program section */
#define OPTION                                     0x01
#define INSTRUCTION                                0x02
#define DECLARATION                                0x03
#define END                                        0x04

/* fragment program option flags */
#define ARB_PRECISION_HINT_FASTEST                 0x01
#define ARB_PRECISION_HINT_NICEST                  0x02
#define ARB_FOG_EXP                                0x04
#define ARB_FOG_EXP2                               0x08
#define ARB_FOG_LINEAR                             0x10

/* vertex program option flags */
/*
$4: changed from 0x01 to 0x20.
*/
#define ARB_POSITION_INVARIANT                     0x20

/* fragment program 1.0 instruction class */
#define OP_ALU_INST                                0x00
#define OP_TEX_INST                                0x01

/* vertex program 1.0 instruction class */
/*       OP_ALU_INST */

/* fragment program 1.0 instruction type */
#define OP_ALU_VECTOR                               0x06
#define OP_ALU_SCALAR                               0x03
#define OP_ALU_BINSC                                0x02
#define OP_ALU_BIN                                  0x01
#define OP_ALU_TRI                                  0x05
#define OP_ALU_SWZ                                  0x04
#define OP_TEX_SAMPLE                               0x07
#define OP_TEX_KIL                                  0x08

/* vertex program 1.0 instruction type */
#define OP_ALU_ARL                                  0x00
/*       OP_ALU_VECTOR */
/*       OP_ALU_SCALAR */
/*       OP_ALU_BINSC */
/*       OP_ALU_BIN */
/*       OP_ALU_TRI */
/*       OP_ALU_SWZ */

/* fragment program 1.0 instruction code */
#define OP_ABS                                     0x00
#define OP_ABS_SAT                                 0x1B
#define OP_FLR                                     0x09
#define OP_FLR_SAT                                 0x26
#define OP_FRC                                     0x0A
#define OP_FRC_SAT                                 0x27
#define OP_LIT                                     0x0C
#define OP_LIT_SAT                                 0x2A
#define OP_MOV                                     0x11
#define OP_MOV_SAT                                 0x30
#define OP_COS                                     0x1F
#define OP_COS_SAT                                 0x20
#define OP_EX2                                     0x07
#define OP_EX2_SAT                                 0x25
#define OP_LG2                                     0x0B
#define OP_LG2_SAT                                 0x29
#define OP_RCP                                     0x14
#define OP_RCP_SAT                                 0x33
#define OP_RSQ                                     0x15
#define OP_RSQ_SAT                                 0x34
#define OP_SIN                                     0x38
#define OP_SIN_SAT                                 0x39
#define OP_SCS                                     0x35
#define OP_SCS_SAT                                 0x36
#define OP_POW                                     0x13
#define OP_POW_SAT                                 0x32
#define OP_ADD                                     0x01
#define OP_ADD_SAT                                 0x1C
#define OP_DP3                                     0x03
#define OP_DP3_SAT                                 0x21
#define OP_DP4                                     0x04
#define OP_DP4_SAT                                 0x22
#define OP_DPH                                     0x05
#define OP_DPH_SAT                                 0x23
#define OP_DST                                     0x06
#define OP_DST_SAT                                 0x24
#define OP_MAX                                     0x0F
#define OP_MAX_SAT                                 0x2E
#define OP_MIN                                     0x10
#define OP_MIN_SAT                                 0x2F
#define OP_MUL                                     0x12
#define OP_MUL_SAT                                 0x31
#define OP_SGE                                     0x16
#define OP_SGE_SAT                                 0x37
#define OP_SLT                                     0x17
#define OP_SLT_SAT                                 0x3A
#define OP_SUB                                     0x18
#define OP_SUB_SAT                                 0x3B
#define OP_XPD                                     0x1A
#define OP_XPD_SAT                                 0x43
#define OP_CMP                                     0x1D
#define OP_CMP_SAT                                 0x1E
#define OP_LRP                                     0x2B
#define OP_LRP_SAT                                 0x2C
#define OP_MAD                                     0x0E
#define OP_MAD_SAT                                 0x2D
#define OP_SWZ                                     0x19
#define OP_SWZ_SAT                                 0x3C
#define OP_TEX                                     0x3D
#define OP_TEX_SAT                                 0x3E
#define OP_TXB                                     0x3F
#define OP_TXB_SAT                                 0x40
#define OP_TXP                                     0x41
#define OP_TXP_SAT                                 0x42
#define OP_KIL                                     0x28

/* vertex program 1.0 instruction code */
#define OP_ARL                                     0x02
/*       OP_ABS */
/*       OP_FLR */
/*       OP_FRC */
/*       OP_LIT */
/*       OP_MOV */
/*       OP_EX2 */
#define OP_EXP                                     0x08
/*       OP_LG2 */
#define OP_LOG                                     0x0D
/*       OP_RCP */
/*       OP_RSQ */
/*       OP_POW */
/*       OP_ADD */
/*       OP_DP3 */
/*       OP_DP4 */
/*       OP_DPH */
/*       OP_DST */
/*       OP_MAX */
/*       OP_MIN */
/*       OP_MUL */
/*       OP_SGE */
/*       OP_SLT */
/*       OP_SUB */
/*       OP_XPD */
/*       OP_MAD */
/*       OP_SWZ */

/* fragment attribute binding */
#define FRAGMENT_ATTRIB_COLOR                      0x01
#define FRAGMENT_ATTRIB_TEXCOORD                   0x02
#define FRAGMENT_ATTRIB_FOGCOORD                   0x03
#define FRAGMENT_ATTRIB_POSITION                   0x04

/* vertex attribute binding */
#define VERTEX_ATTRIB_POSITION                     0x01
#define VERTEX_ATTRIB_WEIGHT                       0x02
#define VERTEX_ATTRIB_NORMAL                       0x03
#define VERTEX_ATTRIB_COLOR                        0x04
#define VERTEX_ATTRIB_FOGCOORD                     0x05
#define VERTEX_ATTRIB_TEXCOORD                     0x06
#define VERTEX_ATTRIB_MATRIXINDEX                  0x07
#define VERTEX_ATTRIB_GENERIC                      0x08

/* fragment result binding */
#define FRAGMENT_RESULT_COLOR                      0x01
#define FRAGMENT_RESULT_DEPTH                      0x02

/* vertex result binding */
#define VERTEX_RESULT_POSITION                     0x01
#define VERTEX_RESULT_COLOR                        0x02
#define VERTEX_RESULT_FOGCOORD                     0x03
#define VERTEX_RESULT_POINTSIZE                    0x04
#define VERTEX_RESULT_TEXCOORD                     0x05

/* texture target */
#define TEXTARGET_1D                               0x01
#define TEXTARGET_2D                               0x02
#define TEXTARGET_3D                               0x03
#define TEXTARGET_RECT                             0x04
#define TEXTARGET_CUBE                             0x05

/* sign */
/*
$3: removed. '+' and '-' are used instead.
*/
/*
#define SIGN_PLUS                                  0x00
#define SIGN_MINUS                                 0x01
*/

/* face type */
#define FACE_FRONT                                 0x00
#define FACE_BACK                                  0x01

/* color type */
#define COLOR_PRIMARY                              0x00
#define COLOR_SECONDARY                            0x01

/* component */
/*
$3: Added enumerants.
*/
#define COMPONENT_X                                0x00
#define COMPONENT_Y                                0x01
#define COMPONENT_Z                                0x02
#define COMPONENT_W                                0x03
#define COMPONENT_0                                0x04
#define COMPONENT_1                                0x05

#define ARRAY_INDEX_ABSOLUTE                       0x00
#define ARRAY_INDEX_RELATIVE                       0x01

/* matrix name */
#define MATRIX_MODELVIEW                           0x01
#define MATRIX_PROJECTION                          0x02
#define MATRIX_MVP                                 0x03
#define MATRIX_TEXTURE                             0x04
#define MATRIX_PALETTE                             0x05
#define MATRIX_PROGRAM                             0x06

/* matrix modifier */
#define MATRIX_MODIFIER_IDENTITY                   0x00
#define MATRIX_MODIFIER_INVERSE                    0x01
#define MATRIX_MODIFIER_TRANSPOSE                  0x02
#define MATRIX_MODIFIER_INVTRANS                   0x03

/* constant type */
#define CONSTANT_SCALAR                            0x01
#define CONSTANT_VECTOR                            0x02

/* program param type */
#define PROGRAM_PARAM_ENV                          0x01
#define PROGRAM_PARAM_LOCAL                        0x02

/* register type */
#define REGISTER_ATTRIB                            0x01
#define REGISTER_PARAM                             0x02
#define REGISTER_RESULT                            0x03
#define REGISTER_ESTABLISHED_NAME                  0x04

/* param binding */
#define PARAM_NULL                                 0x00
#define PARAM_ARRAY_ELEMENT                        0x01
#define PARAM_STATE_ELEMENT                        0x02
#define PARAM_PROGRAM_ELEMENT                      0x03
#define PARAM_PROGRAM_ELEMENTS                     0x04
#define PARAM_CONSTANT                             0x05

/* param state property */
#define STATE_MATERIAL_PARSER                      0x01
#define STATE_LIGHT_PARSER                         0x02
#define STATE_LIGHT_MODEL                          0x03
#define STATE_LIGHT_PROD                           0x04
#define STATE_FOG                                  0x05
#define STATE_MATRIX_ROWS                          0x06
/* fragment program only */
#define STATE_TEX_ENV                              0x07
#define STATE_DEPTH                                0x08
/* vertex program only */
/*
$4: incremented all the three emit codes by two to not collide with other STATE_* emit codes.
*/
#define STATE_TEX_GEN                              0x09
#define STATE_CLIP_PLANE                           0x0A
#define STATE_POINT                                0x0B

/* state material property */
#define MATERIAL_AMBIENT                           0x01
#define MATERIAL_DIFFUSE                           0x02
#define MATERIAL_SPECULAR                          0x03
#define MATERIAL_EMISSION                          0x04
#define MATERIAL_SHININESS                         0x05

/* state light property */
#define LIGHT_AMBIENT                              0x01
#define LIGHT_DIFFUSE                              0x02
#define LIGHT_SPECULAR                             0x03
#define LIGHT_POSITION                             0x04
#define LIGHT_ATTENUATION                          0x05
#define LIGHT_HALF                                 0x06
#define LIGHT_SPOT_DIRECTION                       0x07

/* state light model property */
#define LIGHT_MODEL_AMBIENT                        0x01
#define LIGHT_MODEL_SCENECOLOR                     0x02

/* state light product property */
#define LIGHT_PROD_AMBIENT                         0x01
#define LIGHT_PROD_DIFFUSE                         0x02
#define LIGHT_PROD_SPECULAR                        0x03

/* state texture environment property */
#define TEX_ENV_COLOR                              0x01

/* state texture generation coord property */
#define TEX_GEN_EYE                                0x01
#define TEX_GEN_OBJECT                             0x02

/* state fog property */
#define FOG_COLOR                                  0x01
#define FOG_PARAMS                                 0x02

/* state depth property */
#define DEPTH_RANGE                                0x01

/* state point parameters property */
#define POINT_SIZE                                 0x01
#define POINT_ATTENUATION                          0x02

/* declaration */
#define ATTRIB                                     0x01
#define PARAM                                      0x02
#define TEMP                                       0x03
#define OUTPUT                                     0x04
#define ALIAS                                      0x05
/* vertex program 1.0 only */
#define ADDRESS                                    0x06

/*
	memory management routines
*/
static GLvoid *mem_alloc (GLsizei);
static GLvoid mem_free (GLvoid **);
static GLvoid *mem_realloc (GLvoid *, GLsizei, GLsizei);
static GLubyte *str_duplicate (const GLubyte *);

/*
	internal error messages
*/
static const GLubyte *OUT_OF_MEMORY =
   (GLubyte *) "internal error 1001: out of physical memory";
static const GLubyte *UNRESOLVED_REFERENCE =
   (GLubyte *) "internal error 1002: unresolved reference '$'";
/*
static const GLubyte *INVALID_PARAMETER =
   (GLubyte *) "internal error 1003: invalid parameter";
*/

static const GLubyte *error_message = NULL;
static GLubyte *error_param = NULL;        /* this is inserted into error_message in place of $ */
static GLint error_position = -1;

static GLubyte *unknown = (GLubyte *) "???";

static GLvoid
clear_last_error (GLvoid)
{
   /* reset error message */
   error_message = NULL;

   /* free error parameter - if error_param is a "???" don't free it - it's static */
   if (error_param != unknown)
      mem_free ((GLvoid **) & error_param);
   else
      error_param = NULL;

   /* reset error position */
   error_position = -1;
}

static GLvoid
set_last_error (const GLubyte * msg, GLubyte * param, GLint pos)
{
   if (error_message != NULL)
      return;

   error_message = msg;
   if (param != NULL)
      error_param = param;
   else
      error_param = unknown;

   error_position = pos;
}

/*
 * memory management routines
 */
static GLvoid *
mem_alloc (GLsizei size)
{
   GLvoid *ptr = _mesa_malloc (size);
   if (ptr == NULL)
      set_last_error (OUT_OF_MEMORY, NULL, -1);
   return ptr;
}

static GLvoid
mem_free (GLvoid ** ptr)
{
   _mesa_free (*ptr);
   *ptr = NULL;
}

static GLvoid *
mem_realloc (GLvoid * ptr, GLsizei old_size, GLsizei new_size)
{
   GLvoid *ptr2 = _mesa_realloc (ptr, old_size, new_size);
   if (ptr2 == NULL)
      set_last_error (OUT_OF_MEMORY, NULL, -1);
   return ptr2;
}

static GLubyte *
str_duplicate (const GLubyte * str)
{
   return (GLubyte *) _mesa_strdup ((const char *) str);
}

/*
 * emit type typedef
 */
typedef enum emit_type_
{
   et_byte,                     /* explicit number */
   et_stream,                   /* eaten character */
   et_position                  /* current position */
}
emit_type;

/*
 * emit typedef
 */
typedef struct emit_
{
   emit_type m_emit_type;
   GLubyte m_byte;                 /* et_byte */
   struct emit_ *m_next;
}
emit;

static GLvoid
emit_create (emit ** em)
{
   *em = (emit *) mem_alloc (sizeof (emit));
   if (*em) {
      (**em).m_emit_type = et_byte;
      (**em).m_byte = 0;
      (**em).m_next = NULL;
   }
}

static GLvoid
emit_destroy (emit ** em)
{
   if (*em) {
      emit_destroy (&(**em).m_next);
      mem_free ((GLvoid **) em);
   }
}

static GLvoid
emit_append (emit ** em, emit ** ne)
{
   if (*em)
      emit_append (&(**em).m_next, ne);
   else
      *em = *ne;
}

/*
 * error typedef
 */
typedef struct error_
{
   GLubyte *m_text;
   GLubyte *m_token_name;
   struct defntn_ *m_token;
}
error;

static GLvoid
error_create (error ** er)
{
   *er = (error *) mem_alloc (sizeof (error));
   if (*er) {
      (**er).m_text = NULL;
      (**er).m_token_name = NULL;
      (**er).m_token = NULL;
   }
}

static GLvoid
error_destroy (error ** er)
{
   if (*er) {
      mem_free ((GLvoid **) & (**er).m_text);
      mem_free ((GLvoid **) & (**er).m_token_name);
      mem_free ((GLvoid **) er);
   }
}

struct dict_;
static GLubyte *error_get_token (error *, struct dict_ *, const GLubyte *, GLuint);

/*
 * specifier type typedef
*/
typedef enum spec_type_
{
   st_false,
   st_true,
   st_byte,
   st_byte_range,
   st_string,
   st_identifier,
   st_identifier_loop,
   st_debug
} spec_type;


/*
 * specifier typedef
 */
typedef struct spec_
{
   spec_type m_spec_type;
   GLubyte m_byte[2];              /* st_byte, st_byte_range */
   GLubyte *m_string;              /* st_string */
   struct defntn_ *m_defntn;    /* st_identifier, st_identifier_loop */
   emit *m_emits;
   error *m_errtext;
   struct spec_ *m_next;
} spec;


static GLvoid
spec_create (spec ** sp)
{
   *sp = (spec *) mem_alloc (sizeof (spec));
   if (*sp) {
      (**sp).m_spec_type = st_false;
      (**sp).m_byte[0] = '\0';
      (**sp).m_byte[1] = '\0';
      (**sp).m_string = NULL;
      (**sp).m_defntn = NULL;
      (**sp).m_emits = NULL;
      (**sp).m_errtext = NULL;
      (**sp).m_next = NULL;
   }
}

static GLvoid
spec_destroy (spec ** sp)
{
   if (*sp) {
      spec_destroy (&(**sp).m_next);
      emit_destroy (&(**sp).m_emits);
      error_destroy (&(**sp).m_errtext);
      mem_free ((GLvoid **) & (**sp).m_string);
      mem_free ((GLvoid **) sp);
   }
}

static GLvoid
spec_append (spec ** sp, spec ** ns)
{
   if (*sp)
      spec_append (&(**sp).m_next, ns);
   else
      *sp = *ns;
}

/*
 * operator typedef
 */
typedef enum oper_
{
   op_none,
   op_and,
   op_or
} oper;


/*
 * definition typedef
 */
typedef struct defntn_
{
   oper m_oper;
   spec *m_specs;
   struct defntn_ *m_next;
#ifndef NDEBUG
   GLint m_referenced;
#endif
} defntn;


static GLvoid
defntn_create (defntn ** de)
{
   *de = (defntn *) mem_alloc (sizeof (defntn));
   if (*de) {
      (**de).m_oper = op_none;
      (**de).m_specs = NULL;
      (**de).m_next = NULL;
#ifndef NDEBUG
      (**de).m_referenced = 0;
#endif
   }
}

static GLvoid
defntn_destroy (defntn ** de)
{
   if (*de) {
      defntn_destroy (&(**de).m_next);
      spec_destroy (&(**de).m_specs);
      mem_free ((GLvoid **) de);
   }
}

static GLvoid
defntn_append (defntn ** de, defntn ** nd)
{
   if (*de)
      defntn_append (&(**de).m_next, nd);
   else
      *de = *nd;
}

/*
 * dictionary typedef
 */
typedef struct dict_
{
   defntn *m_defntns;
   defntn *m_syntax;
   defntn *m_string;
   struct dict_ *m_next;
} dict;


static GLvoid
dict_create (dict ** di)
{
   *di = (dict *) mem_alloc (sizeof (dict));
   if (*di) {
      (**di).m_defntns = NULL;
      (**di).m_syntax = NULL;
      (**di).m_string = NULL;
      (**di).m_next = NULL;
   }
}

static GLvoid
dict_destroy (dict ** di)
{
   if (*di) {
      dict_destroy (&(**di).m_next);
      defntn_destroy (&(**di).m_defntns);
      mem_free ((GLvoid **) di);
   }
}

/*
 * GLubyte array typedef
 */
typedef struct barray_
{
   GLubyte *data;
   GLuint len;
} barray;


static GLvoid
barray_create (barray ** ba)
{
   *ba = (barray *) mem_alloc (sizeof (barray));
   if (*ba) {
      (**ba).data = NULL;
      (**ba).len = 0;
   }
}

static GLvoid
barray_destroy (barray ** ba)
{
   if (*ba) {
      mem_free ((GLvoid **) & (**ba).data);
      mem_free ((GLvoid **) ba);
   }
}

/*
 * reallocates GLubyte array to requested size,
 * returns 0 on success,
 * returns 1 otherwise
 */
static GLint
barray_resize (barray ** ba, GLuint nlen)
{
   GLubyte *new_pointer;

   if (nlen == 0) {
      mem_free ((void **) &(**ba).data);
      (**ba).data = NULL;
      (**ba).len = 0;

      return 0;
   }
   else {
      new_pointer = (GLubyte *)
         mem_realloc ((**ba).data, (**ba).len * sizeof (GLubyte),
                      nlen * sizeof (GLubyte));
      if (new_pointer) {
         (**ba).data = new_pointer;
         (**ba).len = nlen;

         return 0;
      }
   }

   return 1;
}

/*
 * adds GLubyte array pointed by *nb to the end of array pointed by *ba,
 * returns 0 on success,
 * returns 1 otherwise
 */
static GLint
barray_append (barray ** ba, barray ** nb)
{
   GLuint i;
   const GLuint len = (**ba).len;

   if (barray_resize (ba, (**ba).len + (**nb).len))
      return 1;

   for (i = 0; i < (**nb).len; i++)
      (**ba).data[len + i] = (**nb).data[i];

   return 0;
}


/**
 * Adds emit chain pointed by em to the end of array pointed by *ba.
 * \return 0 on success, 1 otherwise.
 */
static GLint
barray_push (barray ** ba, emit * em, GLubyte c, GLuint pos)
{
   emit *temp = em;
   GLuint count = 0;

   while (temp) {
      if (temp->m_emit_type == et_position)
         count += 4;            /* position is a 32-bit unsigned integer */
      else
         count++;

      temp = temp->m_next;
   }

   if (barray_resize (ba, (**ba).len + count))
      return 1;

   while (em) {
      if (em->m_emit_type == et_byte)
         (**ba).data[(**ba).len - count--] = em->m_byte;
      else if (em->m_emit_type == et_stream)
         (**ba).data[(**ba).len - count--] = c;

      /* This is where the position is emitted into the stream */
      else {                    /* em->type == et_position */
#if 0
         (**ba).data[(**ba).len - count--] = (GLubyte) pos,
            (**ba).data[(**ba).len - count--] = (GLubyte) (pos >> 8),
            (**ba).data[(**ba).len - count--] = (GLubyte) (pos >> 16),
            (**ba).data[(**ba).len - count--] = (GLubyte) (pos >> 24);
#else
         (**ba).data[(**ba).len - count--] = (GLubyte) pos;
         (**ba).data[(**ba).len - count--] = (GLubyte) (pos / 0x100);
         (**ba).data[(**ba).len - count--] = (GLubyte) (pos / 0x10000);
         (**ba).data[(**ba).len - count--] = (GLubyte) (pos / 0x1000000);
#endif
      }

      em = em->m_next;
   }

   return 0;
}

/**
 * string to string map typedef
 */
typedef struct map_str_
{
   GLubyte *key;
   GLubyte *data;
   struct map_str_ *next;
} map_str;


static GLvoid
map_str_create (map_str ** ma)
{
   *ma = (map_str *) mem_alloc (sizeof (map_str));
   if (*ma) {
      (**ma).key = NULL;
      (**ma).data = NULL;
      (**ma).next = NULL;
   }
}

static GLvoid
map_str_destroy (map_str ** ma)
{
   if (*ma) {
      map_str_destroy (&(**ma).next);
      mem_free ((GLvoid **) & (**ma).key);
      mem_free ((GLvoid **) & (**ma).data);
      mem_free ((GLvoid **) ma);
   }
}

static GLvoid
map_str_append (map_str ** ma, map_str ** nm)
{
   if (*ma)
      map_str_append (&(**ma).next, nm);
   else
      *ma = *nm;
}

/**
 * searches the map for specified key,
 * if the key is matched, *data is filled with data associated with the key,
 * \return 0 if the key is matched, 1 otherwise
 */
static GLint
map_str_find (map_str ** ma, const GLubyte * key, GLubyte ** data)
{
   while (*ma) {
      if (strcmp ((const char *) (**ma).key, (const char *) key) == 0) {
         *data = str_duplicate ((**ma).data);
         if (*data == NULL)
            return 1;

         return 0;
      }

      ma = &(**ma).next;
   }

   set_last_error (UNRESOLVED_REFERENCE, str_duplicate (key), -1);
   return 1;
}

/**
 * string to GLubyte map typedef
 */
typedef struct map_byte_
{
   GLubyte *key;
   GLubyte data;
   struct map_byte_ *next;
} map_byte;

static GLvoid
map_byte_create (map_byte ** ma)
{
   *ma = (map_byte *) mem_alloc (sizeof (map_byte));
   if (*ma) {
      (**ma).key = NULL;
      (**ma).data = 0;
      (**ma).next = NULL;
   }
}

static GLvoid
map_byte_destroy (map_byte ** ma)
{
   if (*ma) {
      map_byte_destroy (&(**ma).next);
      mem_free ((GLvoid **) & (**ma).key);
      mem_free ((GLvoid **) ma);
   }
}

static GLvoid
map_byte_append (map_byte ** ma, map_byte ** nm)
{
   if (*ma)
      map_byte_append (&(**ma).next, nm);
   else
      *ma = *nm;
}

/**
 * Searches the map for specified key,
 * If the key is matched, *data is filled with data associated with the key,
 * \return 0 if the is matched, 1 otherwise
 */
static GLint
map_byte_find (map_byte ** ma, const GLubyte * key, GLubyte * data)
{
   while (*ma) {
      if (strcmp ((const char *) (**ma).key, (const char *) key) == 0) {
         *data = (**ma).data;
         return 0;
      }

      ma = &(**ma).next;
   }

   set_last_error (UNRESOLVED_REFERENCE, str_duplicate (key), -1);
   return 1;
}

/*
 * string to defntn map typedef
 */
typedef struct map_def_
{
   GLubyte *key;
   defntn *data;
   struct map_def_ *next;
} map_def;

static GLvoid
map_def_create (map_def ** ma)
{
   *ma = (map_def *) mem_alloc (sizeof (map_def));
   if (*ma) {
      (**ma).key = NULL;
      (**ma).data = NULL;
      (**ma).next = NULL;
   }
}

static GLvoid
map_def_destroy (map_def ** ma)
{
   if (*ma) {
      map_def_destroy (&(**ma).next);
      mem_free ((GLvoid **) & (**ma).key);
      mem_free ((GLvoid **) ma);
   }
}

static GLvoid
map_def_append (map_def ** ma, map_def ** nm)
{
   if (*ma)
      map_def_append (&(**ma).next, nm);
   else
      *ma = *nm;
}

/**
 * searches the map for specified key,
 * if the key is matched, *data is filled with data associated with the key,
 * \return 0 if the is matched, 1 otherwise
 */
static GLint
map_def_find (map_def ** ma, const GLubyte * key, defntn ** data)
{
   while (*ma) {
      if (_mesa_strcmp ((const char *) (**ma).key, (const char *) key) == 0) {
         *data = (**ma).data;

         return 0;
      }

      ma = &(**ma).next;
   }

   set_last_error (UNRESOLVED_REFERENCE, str_duplicate (key), -1);
   return 1;
}

/*
 * returns 1 if given character is a space,
 * returns 0 otherwise
 */
static GLint
is_space (GLubyte c)
{
   return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*
 * advances text pointer by 1 if character pointed by *text is a space,
 * returns 1 if a space has been eaten,
 * returns 0 otherwise
 */
static GLint
eat_space (const GLubyte ** text)
{
   if (is_space (**text)) {
      (*text)++;

      return 1;
   }

   return 0;
}

/*
 * returns 1 if text points to C-style comment start string "/ *",
 * returns 0 otherwise
 */
static GLint
is_comment_start (const GLubyte * text)
{
   return text[0] == '/' && text[1] == '*';
}

/*
 * advances text pointer to first character after C-style comment block - if any,
 * returns 1 if C-style comment block has been encountered and eaten,
 * returns 0 otherwise
 */
static GLint
eat_comment (const GLubyte ** text)
{
   if (is_comment_start (*text)) {
      /* *text points to comment block - skip two characters to enter comment body */
      *text += 2;
      /* skip any character except consecutive '*' and '/' */
      while (!((*text)[0] == '*' && (*text)[1] == '/'))
         (*text)++;
      /* skip those two terminating characters */
      *text += 2;

      return 1;
   }

   return 0;
}

/*
 * advances text pointer to first character that is neither space nor C-style comment block
 */
static GLvoid
eat_spaces (const GLubyte ** text)
{
   while (eat_space (text) || eat_comment (text));
}

/*
 * resizes string pointed by *ptr to successfully add character c to the end of the string,
 * returns 0 on success,
 * returns 1 otherwise
 */
static GLint
string_grow (GLubyte ** ptr, GLuint * len, GLubyte c)
{
   /* reallocate the string in 16-length increments */
   if ((*len & 0x0F) == 0x0F || *ptr == NULL) {
      GLubyte *tmp = (GLubyte *) mem_realloc (*ptr, (*len) * sizeof (GLubyte),
                               ((*len + 1 + 1 +
                                 0x0F) & ~0x0F) * sizeof (GLubyte));
      if (tmp == NULL)
         return 1;

      *ptr = tmp;
   }

   if (c) {
      /* append given character */
      (*ptr)[*len] = c;
      (*len)++;
   }
   (*ptr)[*len] = '\0';

   return 0;
}

/*
 * returns 1 if given character is valid identifier character a-z, A-Z, 0-9 or _
 * returns 0 otherwise
 */
static GLint
is_identifier (GLubyte c)
{
   return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') || c == '_';
}

/*
 * copies characters from *text to *id until non-identifier character is encountered,
 * assumes that *id points to NULL object - caller is responsible for later freeing the string,
 * text pointer is advanced to point past the copied identifier,
 * returns 0 if identifier was successfully copied,
 * returns 1 otherwise
 */
static GLint
get_identifier (const GLubyte ** text, GLubyte ** id)
{
   const GLubyte *t = *text;
   GLubyte *p = NULL;
   GLuint len = 0;

   if (string_grow (&p, &len, '\0'))
      return 1;

   /* loop while next character in buffer is valid for identifiers */
   while (is_identifier (*t)) {
      if (string_grow (&p, &len, *t++)) {
         mem_free ((GLvoid **) & p);
         return 1;
      }
   }

   *text = t;
   *id = p;

   return 0;
}

/*
 * returns 1 if given character is HEX digit 0-9, A-F or a-f,
 * returns 0 otherwise
 */
static GLint
is_hex (GLubyte c)
{
   return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a'
                                                               && c <= 'f');
}

/*
 * returns value of passed character as if it was HEX digit
 */
static GLuint
hex2dec (GLubyte c)
{
   if (c >= '0' && c <= '9')
      return c - '0';
   if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
   return c - 'a' + 10;
}

/*
 * converts sequence of HEX digits pointed by *text until non-HEX digit is encountered,
 * advances text pointer past the converted sequence,
 * returns the converted value
 */
static GLuint
hex_convert (const GLubyte ** text)
{
   GLuint value = 0;

   while (is_hex (**text)) {
      value = value * 0x10 + hex2dec (**text);
      (*text)++;
   }

   return value;
}

/*
 * returns 1 if given character is OCT digit 0-7,
 * returns 0 otherwise
 */
static GLint
is_oct (GLubyte c)
{
   return c >= '0' && c <= '7';
}

/*
 * returns value of passed character as if it was OCT digit
 */
static GLint
oct2dec (GLubyte c)
{
   return c - '0';
}

static GLubyte
get_escape_sequence (const GLubyte ** text)
{
   GLint value = 0;

   /* skip '\' character */
   (*text)++;

   switch (*(*text)++) {
      case '\'':
         return '\'';
      case '"':
         return '\"';
      case '?':
         return '\?';
      case '\\':
         return '\\';
      case 'a':
         return '\a';
      case 'b':
         return '\b';
      case 'f':
         return '\f';
      case 'n':
         return '\n';
      case 'r':
         return '\r';
      case 't':
         return '\t';
      case 'v':
         return '\v';
      case 'x':
         return (GLubyte) hex_convert (text);
   }

   (*text)--;
   if (is_oct (**text)) {
      value = oct2dec (*(*text)++);
      if (is_oct (**text)) {
         value = value * 010 + oct2dec (*(*text)++);
         if (is_oct (**text))
            value = value * 010 + oct2dec (*(*text)++);
      }
   }

   return (GLubyte) value;
}

/*
 * copies characters from *text to *str until " or ' character is encountered,
 * assumes that *str points to NULL object - caller is responsible for later freeing the string,
 * assumes that *text points to " or ' character that starts the string,
 * text pointer is advanced to point past the " or ' character,
 * returns 0 if string was successfully copied,
 * returns 1 otherwise
 */
static GLint
get_string (const GLubyte ** text, GLubyte ** str)
{
   const GLubyte *t = *text;
   GLubyte *p = NULL;
   GLuint len = 0;
   GLubyte term_char;

   if (string_grow (&p, &len, '\0'))
      return 1;

   /* read " or ' character that starts the string */
   term_char = *t++;
   /* while next character is not the terminating character */
   while (*t && *t != term_char) {
      GLubyte c;

      if (*t == '\\')
         c = get_escape_sequence (&t);
      else
         c = *t++;

      if (string_grow (&p, &len, c)) {
         mem_free ((GLvoid **) & p);
         return 1;
      }
   }

   /* skip " or ' character that ends the string */
   t++;

   *text = t;
   *str = p;
   return 0;
}

/*
 * gets emit code, the syntax is: ".emtcode" " " <symbol> " " ("0x" | "0X") <hex_value>
 * assumes that *text already points to <symbol>,
 * returns 0 if emit code is successfully read,
 * returns 1 otherwise
 */
static GLint
get_emtcode (const GLubyte ** text, map_byte ** ma)
{
   const GLubyte *t = *text;
   map_byte *m = NULL;

   map_byte_create (&m);
   if (m == NULL)
      return 1;

   if (get_identifier (&t, &m->key)) {
      map_byte_destroy (&m);
      return 1;
   }
   eat_spaces (&t);

   if (*t == '\'') {
      GLubyte *c;

      if (get_string (&t, &c)) {
         map_byte_destroy (&m);
         return 1;
      }

      m->data = (GLubyte) c[0];
      mem_free ((GLvoid **) & c);
   }
   else {
      /* skip HEX "0x" or "0X" prefix */
      t += 2;
      m->data = (GLubyte) hex_convert (&t);
   }

   eat_spaces (&t);

   *text = t;
   *ma = m;
   return 0;
}

/*
 * returns 0 on success,
 * returns 1 otherwise
 */
static GLint
get_errtext (const GLubyte ** text, map_str ** ma)
{
   const GLubyte *t = *text;
   map_str *m = NULL;

   map_str_create (&m);
   if (m == NULL)
      return 1;

   if (get_identifier (&t, &m->key)) {
      map_str_destroy (&m);
      return 1;
   }
   eat_spaces (&t);

   if (get_string (&t, &m->data)) {
      map_str_destroy (&m);
      return 1;
   }
   eat_spaces (&t);

   *text = t;
   *ma = m;
   return 0;
}

/*
 * returns 0 on success,
 * returns 1 otherwise,
 */
static GLint
get_error (const GLubyte ** text, error ** er, map_str * maps)
{
   const GLubyte *t = *text;
   GLubyte *temp = NULL;

   if (*t != '.')
      return 0;

   t++;
   if (get_identifier (&t, &temp))
      return 1;
   eat_spaces (&t);

   if (_mesa_strcmp ("error", (char *) temp) != 0) {
      mem_free ((GLvoid **) & temp);
      return 0;
   }

   mem_free ((GLvoid **) & temp);

   error_create (er);
   if (*er == NULL)
      return 1;

   if (*t == '\"') {
      if (get_string (&t, &(**er).m_text)) {
         error_destroy (er);
         return 1;
      }
      eat_spaces (&t);
   }
   else {
      if (get_identifier (&t, &temp)) {
         error_destroy (er);
         return 1;
      }
      eat_spaces (&t);

      if (map_str_find (&maps, temp, &(**er).m_text)) {
         mem_free ((GLvoid **) & temp);
         error_destroy (er);
         return 1;
      }

      mem_free ((GLvoid **) & temp);
   }

   /* try to extract "token" from "...$token$..." */
   {
      char *processed = NULL;
      GLuint len = 0, i = 0;

      if (string_grow ((GLubyte **) (&processed), &len, '\0')) {
         error_destroy (er);
         return 1;
      }

      while (i < _mesa_strlen ((char *) ((**er).m_text))) {
         /* check if the dollar sign is repeated - if so skip it */
         if ((**er).m_text[i] == '$' && (**er).m_text[i + 1] == '$') {
            if (string_grow ((GLubyte **) (&processed), &len, '$')) {
               mem_free ((GLvoid **) & processed);
               error_destroy (er);
               return 1;
            }

            i += 2;
         }
         else if ((**er).m_text[i] != '$') {
            if (string_grow ((GLubyte **) (&processed), &len, (**er).m_text[i])) {
               mem_free ((GLvoid **) & processed);
               error_destroy (er);
               return 1;
            }

            i++;
         }
         else {
            if (string_grow ((GLubyte **) (&processed), &len, '$')) {
               mem_free ((GLvoid **) & processed);
               error_destroy (er);
               return 1;
            }

            {
               /* length of token being extracted */
               GLuint tlen = 0;

               if (string_grow (&(**er).m_token_name, &tlen, '\0')) {
                  mem_free ((GLvoid **) & processed);
                  error_destroy (er);
                  return 1;
               }

               /* skip the dollar sign */
               i++;

               while ((**er).m_text[i] != '$') {
                  if (string_grow
                      (&(**er).m_token_name, &tlen, (**er).m_text[i])) {
                     mem_free ((GLvoid **) & processed);
                     error_destroy (er);
                     return 1;
                  }

                  i++;
               }

               /* skip the dollar sign */
               i++;
            }
         }
      }

      mem_free ((GLvoid **) & (**er).m_text);
      (**er).m_text = (GLubyte *) processed;
   }

   *text = t;
   return 0;
}

/*
 * returns 0 on success,
 * returns 1 otherwise,
 */
static GLint
get_emits (const GLubyte ** text, emit ** em, map_byte * mapb)
{
   const GLubyte *t = *text;
   GLubyte *temp = NULL;
   emit *e = NULL;

   if (*t != '.')
      return 0;

   t++;
   if (get_identifier (&t, &temp))
      return 1;
   eat_spaces (&t);

   /* .emit */
   if (_mesa_strcmp ("emit", (char *) temp) != 0) {
      mem_free ((GLvoid **) & temp);
      return 0;
   }

   mem_free ((GLvoid **) & temp);

   emit_create (&e);
   if (e == NULL)
      return 1;

   /* 0xNN */
   if (*t == '0') {
      t += 2;
      e->m_byte = (GLubyte) hex_convert (&t);

      e->m_emit_type = et_byte;
   }
   /* * */
   else if (*t == '*') {
      t++;

      e->m_emit_type = et_stream;
   }
   /* $ */
   else if (*t == '$') {
      t++;

      e->m_emit_type = et_position;
   }
   /* 'c' */
   else if (*t == '\'') {
      if (get_string (&t, &temp)) {
         emit_destroy (&e);
         return 1;
      }
      e->m_byte = (GLubyte) temp[0];

      mem_free ((GLvoid **) & temp);

      e->m_emit_type = et_byte;
   }
   else {
      if (get_identifier (&t, &temp)) {
         emit_destroy (&e);
         return 1;
      }

      if (map_byte_find (&mapb, temp, &e->m_byte)) {
         mem_free ((GLvoid **) & temp);
         emit_destroy (&e);
         return 1;
      }

      mem_free ((GLvoid **) & temp);

      e->m_emit_type = et_byte;
   }

   eat_spaces (&t);

   if (get_emits (&t, &e->m_next, mapb)) {
      emit_destroy (&e);
      return 1;
   }

   *text = t;
   *em = e;
   return 0;
}

/*
 * returns 0 on success,
 * returns 1 otherwise,
 */
static GLint
get_spec (const GLubyte ** text, spec ** sp, map_str * maps, map_byte * mapb)
{
   const GLubyte *t = *text;
   spec *s = NULL;

   spec_create (&s);
   if (s == NULL)
      return 1;

   if (*t == '\'') {
      GLubyte *temp = NULL;

      if (get_string (&t, &temp)) {
         spec_destroy (&s);
         return 1;
      }
      eat_spaces (&t);

      if (*t == '-') {
         GLubyte *temp2 = NULL;

         /* skip the '-' character */
         t++;
         eat_spaces (&t);

         if (get_string (&t, &temp2)) {
            mem_free ((GLvoid **) & temp);
            spec_destroy (&s);
            return 1;
         }
         eat_spaces (&t);

         s->m_spec_type = st_byte_range;
         s->m_byte[0] = *temp;
         s->m_byte[1] = *temp2;

         mem_free ((GLvoid **) & temp2);
      }
      else {
         s->m_spec_type = st_byte;
         *s->m_byte = *temp;
      }

      mem_free ((GLvoid **) & temp);
   }
   else if (*t == '"') {
      if (get_string (&t, &s->m_string)) {
         spec_destroy (&s);
         return 1;
      }
      eat_spaces (&t);

      s->m_spec_type = st_string;
   }
   else if (*t == '.') {
      GLubyte *keyword = NULL;

      /* skip the dot */
      t++;

      if (get_identifier (&t, &keyword)) {
         spec_destroy (&s);
         return 1;
      }
      eat_spaces (&t);

      /* .true */
      if (_mesa_strcmp ("true", (char *) keyword) == 0) {
         s->m_spec_type = st_true;
      }
      /* .false */
      else if (_mesa_strcmp ("false", (char *) keyword) == 0) {
         s->m_spec_type = st_false;
      }
      /* .debug */
      else if (_mesa_strcmp ("debug", (char *) keyword) == 0) {
         s->m_spec_type = st_debug;
      }
      /* .loop */
      else if (_mesa_strcmp ("loop", (char *) keyword) == 0) {
         if (get_identifier (&t, &s->m_string)) {
            mem_free ((GLvoid **) & keyword);
            spec_destroy (&s);
            return 1;
         }
         eat_spaces (&t);

         s->m_spec_type = st_identifier_loop;
      }

      mem_free ((GLvoid **) & keyword);
   }
   else {
      if (get_identifier (&t, &s->m_string)) {
         spec_destroy (&s);
         return 1;
      }
      eat_spaces (&t);

      s->m_spec_type = st_identifier;
   }

   if (get_error (&t, &s->m_errtext, maps)) {
      spec_destroy (&s);
      return 1;
   }

   if (get_emits (&t, &s->m_emits, mapb)) {
      spec_destroy (&s);
      return 1;
   }

   *text = t;
   *sp = s;
   return 0;
}

/*
 * returns 0 on success,
 * returns 1 otherwise,
 */
static GLint
get_definition (const GLubyte ** text, defntn ** de, map_str * maps,
                map_byte * mapb)
{
   const GLubyte *t = *text;
   defntn *d = NULL;

   defntn_create (&d);
   if (d == NULL)
      return 1;

   if (get_spec (&t, &d->m_specs, maps, mapb)) {
      defntn_destroy (&d);
      return 1;
   }

   while (*t != ';') {
      GLubyte *op = NULL;
      spec *sp = NULL;

      /* skip the dot that precedes "and" or "or" */
      t++;

      /* read "and" or "or" keyword */
      if (get_identifier (&t, &op)) {
         defntn_destroy (&d);
         return 1;
      }
      eat_spaces (&t);

      if (d->m_oper == op_none) {
         /* .and */
         if (_mesa_strcmp ("and", (char *) op) == 0)
            d->m_oper = op_and;
         /* .or */
         else
            d->m_oper = op_or;
      }

      mem_free ((GLvoid **) & op);

      if (get_spec (&t, &sp, maps, mapb)) {
         defntn_destroy (&d);
         return 1;
      }

      spec_append (&d->m_specs, &sp);
   }

   /* skip the semicolon */
   t++;
   eat_spaces (&t);

   *text = t;
   *de = d;
   return 0;
}

/*
 * returns 0 on success,
 * returns 1 otherwise,
 */
static GLint
update_dependency (map_def * mapd, GLubyte * symbol, defntn ** def)
{
   if (map_def_find (&mapd, symbol, def))
      return 1;

#ifndef NDEBUG
   (**def).m_referenced = 1;
#endif

   return 0;
}

/*
 * returns 0 on success,
 * returns 1 otherwise,
 */
static GLint
update_dependencies (dict * di, map_def * mapd, GLubyte ** syntax_symbol,
                     GLubyte ** string_symbol)
{
   defntn *de = di->m_defntns;

   if (update_dependency (mapd, *syntax_symbol, &di->m_syntax) ||
       (*string_symbol != NULL
        && update_dependency (mapd, *string_symbol, &di->m_string)))
      return 1;

   mem_free ((GLvoid **) syntax_symbol);
   mem_free ((GLvoid **) string_symbol);

   while (de) {
      spec *sp = de->m_specs;

      while (sp) {
         if (sp->m_spec_type == st_identifier
             || sp->m_spec_type == st_identifier_loop) {
            if (update_dependency (mapd, sp->m_string, &sp->m_defntn))
               return 1;

            mem_free ((GLvoid **) & sp->m_string);
         }

         if (sp->m_errtext && sp->m_errtext->m_token_name) {
            if (update_dependency
                (mapd, sp->m_errtext->m_token_name, &sp->m_errtext->m_token))
               return 1;

            mem_free ((GLvoid **) & sp->m_errtext->m_token_name);
         }

         sp = sp->m_next;
      }

      de = de->m_next;
   }

   return 0;
}

typedef enum match_result_
{
   mr_not_matched,              /* the examined string does not match */
   mr_matched,                  /* the examined string matches */
   mr_error_raised,             /* mr_not_matched + error has been raised */
   mr_dont_emit,                /* used by identifier loops only */
   mr_internal_error            /* an internal error has occured such as out of memory */
} match_result;

static match_result
match (dict * di, const GLubyte * text, GLuint * index, defntn * de,
       barray ** ba, GLint filtering_string)
{
   GLuint ind = *index;
   match_result status = mr_not_matched;
   spec *sp = de->m_specs;

   /* for every specifier in the definition */
   while (sp) {
      GLuint i, len, save_ind = ind;
      barray *array = NULL;

      switch (sp->m_spec_type) {
         case st_identifier:
            barray_create (&array);
            if (array == NULL)
               return mr_internal_error;

            status =
               match (di, text, &ind, sp->m_defntn, &array, filtering_string);
            if (status == mr_internal_error) {
               barray_destroy (&array);
               return mr_internal_error;
            }
            break;
         case st_string:
            len = _mesa_strlen ((char *) (sp->m_string));

            /* prefilter the stream */
            if (!filtering_string && di->m_string) {
               barray *ba;
               GLuint filter_index = 0;
               match_result result;

               barray_create (&ba);
               if (ba == NULL)
                  return mr_internal_error;

               result =
                  match (di, text + ind, &filter_index, di->m_string, &ba, 1);

               if (result == mr_internal_error) {
                  barray_destroy (&ba);
                  return mr_internal_error;
               }

               if (result != mr_matched) {
                  barray_destroy (&ba);
                  status = mr_not_matched;
                  break;
               }

               barray_destroy (&ba);

               if (filter_index != len
                   || _mesa_strncmp ((char *)sp->m_string, (char *)(text + ind), len)) {
                  status = mr_not_matched;
                  break;
               }

               status = mr_matched;
               ind += len;
            }
            else {
               status = mr_matched;
               for (i = 0; status == mr_matched && i < len; i++)
                  if (text[ind + i] != sp->m_string[i])
                     status = mr_not_matched;
               if (status == mr_matched)
                  ind += len;
            }
            break;
         case st_byte:
            status = text[ind] == *sp->m_byte ? mr_matched : mr_not_matched;
            if (status == mr_matched)
               ind++;
            break;
         case st_byte_range:
            status = (text[ind] >= sp->m_byte[0]
                      && text[ind] <=
                      sp->m_byte[1]) ? mr_matched : mr_not_matched;
            if (status == mr_matched)
               ind++;
            break;
         case st_true:
            status = mr_matched;
            break;
         case st_false:
            status = mr_not_matched;
            break;
         case st_debug:
            status = mr_matched;
            break;
         case st_identifier_loop:
            barray_create (&array);
            if (array == NULL)
               return mr_internal_error;

            status = mr_dont_emit;
            for (;;) {
               match_result result;

               save_ind = ind;
               result =
                  match (di, text, &ind, sp->m_defntn, &array,
                         filtering_string);

               if (result == mr_error_raised) {
                  status = result;
                  break;
               }
               else if (result == mr_matched) {
                  if (barray_push (ba, sp->m_emits, text[ind - 1], save_ind)
                      || barray_append (ba, &array)) {
                     barray_destroy (&array);
                     return mr_internal_error;
                  }
                  barray_destroy (&array);
                  barray_create (&array);
                  if (array == NULL)
                     return mr_internal_error;
               }
               else if (result == mr_internal_error) {
                  barray_destroy (&array);
                  return mr_internal_error;
               }
               else
                  break;
            }
            break;
      };

      if (status == mr_error_raised) {
         barray_destroy (&array);

         return mr_error_raised;
      }

      if (de->m_oper == op_and && status != mr_matched
          && status != mr_dont_emit) {
         barray_destroy (&array);

         if (sp->m_errtext) {
            set_last_error (sp->m_errtext->m_text,
                            error_get_token (sp->m_errtext, di, text, ind),
                            ind);

            return mr_error_raised;
         }

         return mr_not_matched;
      }

      if (status == mr_matched) {
         if (sp->m_emits)
            if (barray_push (ba, sp->m_emits, text[ind - 1], save_ind)) {
               barray_destroy (&array);
               return mr_internal_error;
            }

         if (array)
            if (barray_append (ba, &array)) {
               barray_destroy (&array);
               return mr_internal_error;
            }
      }

      barray_destroy (&array);

      if (de->m_oper == op_or
          && (status == mr_matched || status == mr_dont_emit)) {
         *index = ind;
         return mr_matched;
      }

      sp = sp->m_next;
   }

   if (de->m_oper == op_and
       && (status == mr_matched || status == mr_dont_emit)) {
      *index = ind;
      return mr_matched;
   }

   return mr_not_matched;
}

static GLubyte *
error_get_token (error * er, dict * di, const GLubyte * text, unsigned int ind)
{
   GLubyte *str = NULL;

   if (er->m_token) {
      barray *ba;
      GLuint filter_index = 0;

      barray_create (&ba);
      if (ba != NULL) {
         if (match (di, text + ind, &filter_index, er->m_token, &ba, 0) ==
             mr_matched && filter_index) {
            str = (GLubyte *) mem_alloc (filter_index + 1);
            if (str != NULL) {
               _mesa_strncpy ((char *) str, (char *) (text + ind),
                              filter_index);
               str[filter_index] = '\0';
            }
         }
         barray_destroy (&ba);
      }
   }

   return str;
}

typedef struct grammar_load_state_
{
   dict *di;
   GLubyte *syntax_symbol;
   GLubyte *string_symbol;
   map_str *maps;
   map_byte *mapb;
   map_def *mapd;
} grammar_load_state;


static GLvoid
grammar_load_state_create (grammar_load_state ** gr)
{
   *gr = (grammar_load_state *) mem_alloc (sizeof (grammar_load_state));
   if (*gr) {
      (**gr).di = NULL;
      (**gr).syntax_symbol = NULL;
      (**gr).string_symbol = NULL;
      (**gr).maps = NULL;
      (**gr).mapb = NULL;
      (**gr).mapd = NULL;
   }
}

static GLvoid
grammar_load_state_destroy (grammar_load_state ** gr)
{
   if (*gr) {
      dict_destroy (&(**gr).di);
      mem_free ((GLvoid **) &(**gr).syntax_symbol);
      mem_free ((GLvoid **) &(**gr).string_symbol);
      map_str_destroy (&(**gr).maps);
      map_byte_destroy (&(**gr).mapb);
      map_def_destroy (&(**gr).mapd);
      mem_free ((GLvoid **) gr);
   }
}

/*
 * the API
 */

/*
 * loads grammar script from null-terminated ASCII text
 * returns the grammar object
 * returns NULL if an error occurs (call grammar_get_last_error to retrieve the error text)
 */

static dict *
grammar_load_from_text (const GLubyte * text)
{
   dict *d = NULL;
   grammar_load_state *g = NULL;

   clear_last_error ();

   grammar_load_state_create (&g);
   if (g == NULL)
      return NULL;

   dict_create (&g->di);
   if (g->di == NULL) {
      grammar_load_state_destroy (&g);
      return NULL;
   }

   eat_spaces (&text);

   /* skip ".syntax" keyword */
   text += 7;
   eat_spaces (&text);

   /* retrieve root symbol */
   if (get_identifier (&text, &g->syntax_symbol)) {
      grammar_load_state_destroy (&g);
      return NULL;
   }
   eat_spaces (&text);

   /* skip semicolon */
   text++;
   eat_spaces (&text);

   while (*text) {
      GLubyte *symbol = NULL;
      GLint is_dot = *text == '.';

      if (is_dot)
         text++;

      if (get_identifier (&text, &symbol)) {
         grammar_load_state_destroy (&g);
         return NULL;
      }
      eat_spaces (&text);

      /* .emtcode */
      if (is_dot && _mesa_strcmp ((char *) symbol, "emtcode") == 0) {
         map_byte *ma = NULL;

         mem_free ((void **) &symbol);

         if (get_emtcode (&text, &ma)) {
            grammar_load_state_destroy (&g);
            return NULL;
         }

         map_byte_append (&g->mapb, &ma);
      }
      /* .errtext */
      else if (is_dot && _mesa_strcmp ((char *) symbol, "errtext") == 0) {
         map_str *ma = NULL;

         mem_free ((GLvoid **) &symbol);

         if (get_errtext (&text, &ma)) {
            grammar_load_state_destroy (&g);
            return NULL;
         }

         map_str_append (&g->maps, &ma);
      }
      /* .string */
      else if (is_dot && _mesa_strcmp ((char *) symbol, "string") == 0) {
         mem_free ((GLvoid **) (&symbol));

         if (g->di->m_string != NULL) {
            grammar_load_state_destroy (&g);
            return NULL;
         }

         if (get_identifier (&text, &g->string_symbol)) {
            grammar_load_state_destroy (&g);
            return NULL;
         }

         /* skip semicolon */
         eat_spaces (&text);
         text++;
         eat_spaces (&text);
      }
      else {
         defntn *de = NULL;
         map_def *ma = NULL;

         if (get_definition (&text, &de, g->maps, g->mapb)) {
            grammar_load_state_destroy (&g);
            return NULL;
         }

         defntn_append (&g->di->m_defntns, &de);

         /* if definition consist of only one specifier, give it an ".and" operator */
         if (de->m_oper == op_none)
            de->m_oper = op_and;

         map_def_create (&ma);
         if (ma == NULL) {
            grammar_load_state_destroy (&g);
            return NULL;
         }

         ma->key = symbol;
         ma->data = de;
         map_def_append (&g->mapd, &ma);
      }
   }

   if (update_dependencies
       (g->di, g->mapd, &g->syntax_symbol, &g->string_symbol)) {
      grammar_load_state_destroy (&g);
      return NULL;
   }

   d = g->di;
   g->di = NULL;

   grammar_load_state_destroy (&g);

   return d;
}

/**
 * checks if a null-terminated text matches given grammar
 * returns 0 on error (call grammar_get_last_error to retrieve the error text)
 * returns 1 on success, the prod points to newly allocated buffer with
 * production and size is filled with the production size
 *
 * \param id         - The grammar returned from grammar_load_from_text()
 * \param text       - The program string
 * \param production - The return parameter for the binary array holding the
 *                     parsed results
 * \param size       - The return parameter for the size of production 
 *
 * \return 1 on sucess, 0 on parser error
 */
static GLint
grammar_check (dict * di, const GLubyte * text, GLubyte ** production,
               GLuint *size)
{
   barray *ba = NULL;
   GLuint index = 0;

   clear_last_error ();

   barray_create (&ba);
   if (ba == NULL)
      return 0;

   *production = NULL;
   *size = 0;

   if (match (di, text, &index, di->m_syntax, &ba, 0) != mr_matched) {
      barray_destroy (&ba);
      return 0;
   }

   *production = (GLubyte *) mem_alloc (ba->len * sizeof (GLubyte));
   if (*production == NULL) {
      barray_destroy (&ba);
      return 0;
   }

   _mesa_memcpy(*production, ba->data, ba->len * sizeof (GLubyte));
   *size = ba->len;
   barray_destroy (&ba);

   return 1;
}

static GLvoid
grammar_get_last_error (GLubyte * text, GLint size, GLint *pos)
{
   GLint len = 0, dots_made = 0;
   const GLubyte *p = error_message;

   *text = '\0';
#define APPEND_CHARACTER(x) if (dots_made == 0) {\
   if (len < size - 1) {\
      text[len++] = (x); text[len] = '\0';\
   } else {\
      GLint i;\
      for (i = 0; i < 3; i++)\
         if (--len >= 0)\
      text[len] = '.';\
      dots_made = 1;\
   }\
}

   if (p) {
      while (*p) {
         if (*p == '$') {
            const GLubyte *r = error_param;

            while (*r) {
               APPEND_CHARACTER (*r)
                  r++;
            }

            p++;
         }
         else {
            APPEND_CHARACTER (*p)
               p++;
         }
      }
   }
   *pos = error_position;
}

/*-----------------------------------------------------------------------
 * From here on down is the semantic checking portion 
 *
 */

/**
 * Variable Table Handling functions 
 */
typedef enum
{
   vt_none,
   vt_address,
   vt_attrib,
   vt_param,
   vt_temp,
   vt_output,
   vt_alias
} var_type;


/* 
 * Setting an explicit field for each of the binding properties is a bit wasteful
 * of space, but it should be much more clear when reading later on..
 */
struct var_cache
{
   GLubyte *name;
   var_type type;
   GLuint address_binding;      /* The index of the address register we should 
                                 * be using                                        */
   GLuint attrib_binding;       /* For type vt_attrib, see nvfragprog.h for values */
   GLuint attrib_binding_idx;   /* The index into the attrib register file corresponding
                                 * to the state in attrib_binding                  */
   GLuint attrib_is_generic;    /* If the attrib was specified through a generic
                                 * vertex attrib                                   */
   GLuint temp_binding;         /* The index of the temp register we are to use    */
   GLuint output_binding;       /* For type vt_output, see nvfragprog.h for values */
   GLuint output_binding_idx;   /* This is the index into the result register file
                                 * corresponding to the bound result state         */
   struct var_cache *alias_binding;     /* For type vt_alias, points to the var_cache entry
                                         * that this is aliased to                         */
   GLuint param_binding_type;   /* {PROGRAM_STATE_VAR, PROGRAM_LOCAL_PARAM, 
                                 *    PROGRAM_ENV_PARAM}                           */
   GLuint param_binding_begin;  /* This is the offset into the program_parameter_list where
                                 * the tokens representing our bound state (or constants)
                                 * start */
   GLuint param_binding_length; /* This is how many entries in the the program_parameter_list
                                 * we take up with our state tokens or constants. Note that
                                 * this is _not_ the same as the number of param registers
                                 * we eventually use */
   struct var_cache *next;
};

static GLvoid
var_cache_create (struct var_cache **va)
{
   *va = (struct var_cache *) _mesa_malloc (sizeof (struct var_cache));
   if (*va) {
      (**va).name = NULL;
      (**va).type = vt_none;
      (**va).attrib_binding = ~0;
      (**va).attrib_is_generic = 0;
      (**va).temp_binding = ~0;
      (**va).output_binding = ~0;
      (**va).output_binding_idx = ~0;
      (**va).param_binding_type = ~0;
      (**va).param_binding_begin = ~0;
      (**va).param_binding_length = ~0;
      (**va).alias_binding = NULL;
      (**va).next = NULL;
   }
}

static GLvoid
var_cache_destroy (struct var_cache **va)
{
   if (*va) {
      var_cache_destroy (&(**va).next);
      _mesa_free (*va);
      *va = NULL;
   }
}

static GLvoid
var_cache_append (struct var_cache **va, struct var_cache *nv)
{
   if (*va)
      var_cache_append (&(**va).next, nv);
   else
      *va = nv;
}

static struct var_cache *
var_cache_find (struct var_cache *va, GLubyte * name)
{
   struct var_cache *first = va;

   while (va) {
      if (!strcmp ( (const char*) name, (const char*) va->name)) {
         if (va->type == vt_alias)
            return var_cache_find (first, va->name);
         return va;
      }

      va = va->next;
   }

   return NULL;
}

/**
 * constructs an integer from 4 GLubytes in LE format
 */
static GLuint
parse_position (GLubyte ** inst)
{
   GLuint value;

   value =  (GLuint) (*(*inst)++);
   value += (GLuint) (*(*inst)++) * 0x100;
   value += (GLuint) (*(*inst)++) * 0x10000;
   value += (GLuint) (*(*inst)++) * 0x1000000;

   return value;
}

/**
 * This will, given a string, lookup the string as a variable name in the
 * var cache. If the name is found, the var cache node corresponding to the
 * var name is returned. If it is not found, a new entry is allocated
 *
 * \param  I     Points into the binary array where the string identifier begins
 * \param  found 1 if the string was found in the var_cache, 0 if it was allocated
 * \return       The location on the var_cache corresponding the the string starting at I
 */
static struct var_cache *
parse_string (GLubyte ** inst, struct var_cache **vc_head,
              struct arb_program *Program, GLuint * found)
{
   GLubyte *i = *inst;
   struct var_cache *va = NULL;

   *inst += _mesa_strlen ((char *) i) + 1;

   va = var_cache_find (*vc_head, i);

   if (va) {
      *found = 1;
      return va;
   }

   *found = 0;
   var_cache_create (&va);
   va->name = i;

   var_cache_append (vc_head, va);

   return va;
}

static char *
parse_string_without_adding (GLubyte ** inst, struct arb_program *Program)
{
   GLubyte *i = *inst;

   *inst += _mesa_strlen ((char *) i) + 1;

   return (char *) i;
}

/**
 * \return 0 if sign is plus, 1 if sign is minus
 */
static GLuint
parse_sign (GLubyte ** inst)
{
   /*return *(*inst)++ != '+'; */

   if (**inst == '-') {
      (*inst)++;
      return 1;
   }
   else if (**inst == '+') {
      (*inst)++;
      return 0;
   }

   return 0;
}

/**
 * parses and returns signed integer
 */
static GLint
parse_integer (GLubyte ** inst, struct arb_program *Program)
{
   GLint sign;
   GLint value;

   /* check if *inst points to '+' or '-'
    * if yes, grab the sign and increment *inst
    */
   sign = parse_sign (inst);

   /* now check if *inst points to 0
    * if yes, increment the *inst and return the default value
    */
   if (**inst == 0) {
      (*inst)++;
      return 0;
   }

   /* parse the integer as you normally would do it */
   value = _mesa_atoi (parse_string_without_adding (inst, Program));

   /* now, after terminating 0 there is a position
    * to parse it - parse_position()
    */
   Program->Position = parse_position (inst);

   if (sign)
      value *= -1;

   return value;
}

/**
 */
static GLfloat
parse_float (GLubyte ** inst, struct arb_program *Program)
{
   GLint tmp[5], denom;
   GLuint leading_zeros =0;
   GLfloat value = 0;

#if 0
   tmp[0] = parse_sign (inst);  /* This is the sign of the number + - >0, - -> 1 */
#endif
   tmp[1] = parse_integer (inst, Program);   /* This is the integer portion of the number */

   /* Now we grab the fractional portion of the number (the digits after 
	* the .). We can have leading 0's here, which parse_integer will ignore, 
	* so we'll check for those first
	*/
   while ((**inst == '0') && ( *(*inst+1) != 0))
   {
	  leading_zeros++;
	  (*inst)++;
   }
   tmp[2] = parse_integer (inst, Program);   /* This is the fractional portion of the number */
   tmp[3] = parse_sign (inst);               /* This is the sign of the exponent */
   tmp[4] = parse_integer (inst, Program);   /* This is the exponent */

   value = (GLfloat) tmp[1];
   denom = 1; 
   while (denom < tmp[2])
      denom *= 10;
   denom *= (GLint) _mesa_pow( 10, leading_zeros );
   value += (GLfloat) tmp[2] / (GLfloat) denom;
#if 0
   if (tmp[0])
      value *= -1;
#endif
   value *= (GLfloat) _mesa_pow (10, (GLfloat) tmp[3] * (GLfloat) tmp[4]);

   return value;
}


/**
 */
static GLfloat
parse_signed_float (GLubyte ** inst, struct arb_program *Program)
{
   GLint negate;
   GLfloat value;

   negate = parse_sign (inst);

   value = parse_float (inst, Program);

   if (negate)
      value *= -1;

   return value;
}

/**
 * This picks out a constant value from the parsed array. The constant vector is r
 * returned in the *values array, which should be of length 4.
 *
 * \param values - The 4 component vector with the constant value in it 
 */
static GLvoid
parse_constant (GLubyte ** inst, GLfloat *values, struct arb_program *Program,
                GLboolean use)
{
   GLuint components, i;


   switch (*(*inst)++) {
      case CONSTANT_SCALAR:
         if (use == GL_TRUE) {
            values[0] =
               values[1] =
               values[2] = values[3] = parse_float (inst, Program);
         }
         else {
            values[0] =
               values[1] =
               values[2] = values[3] = parse_signed_float (inst, Program);
         }

         break;
      case CONSTANT_VECTOR:
         values[0] = values[1] = values[2] = 0;
         values[3] = 1;
         components = *(*inst)++;
         for (i = 0; i < components; i++) {
            values[i] = parse_signed_float (inst, Program);
         }
         break;
   }
}

/**
 * \param offset The offset from the address register that we should
 *                address
 *
 * \return 0 on sucess, 1 on error                
 */
static GLuint
parse_relative_offset (GLcontext *ctx, GLubyte **inst, struct arb_program *Program,
                        GLint *offset)
{
   *offset = parse_integer(inst, Program);
   if ((*offset > 63) || (*offset < -64)) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Relative offset out of range");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Relative offset %d out of range",
                                                *offset);
      return 1;	  
   }

   return 0;
}

/**
 * \param  color 0 if color type is primary, 1 if color type is secondary
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_color_type (GLcontext * ctx, GLubyte ** inst, struct arb_program *Program,
                  GLint * color)
{
   *color = *(*inst)++ != COLOR_PRIMARY;
   return 0;
}

/**
 * Get an integer corresponding to a generic vertex attribute.
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_generic_attrib_num(GLcontext *ctx, GLubyte ** inst, 
                       struct arb_program *Program, GLuint *attrib)
{
   *attrib = parse_integer(inst, Program);

   if (*attrib > MAX_VERTEX_PROGRAM_ATTRIBS)
   {
      _mesa_set_program_error (ctx, Program->Position,
                               "Invalid generic vertex attribute index");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Invalid generic vertex attribute index");

      return 1;
   }

   return 0;
}


/**
 * \param coord The texture unit index
 * \return 0 on sucess, 1 on error 
 */
static GLuint
parse_texcoord_num (GLcontext * ctx, GLubyte ** inst,
                    struct arb_program *Program, GLuint * coord)
{
   *coord = parse_integer (inst, Program);

   if (*coord >= ctx->Const.MaxTextureUnits) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Invalid texture unit index");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Invalid texture unit index");
      return 1;
   }

   return 0;
}

/**
 * \param coord The weight index
 * \return 0 on sucess, 1 on error 
 */
static GLuint
parse_weight_num (GLcontext * ctx, GLubyte ** inst, struct arb_program *Program,
                  GLint * coord)
{
   *coord = parse_integer (inst, Program);

   if ((*coord < 0) || (*coord >= 1)) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Invalid weight index");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Invalid weight index");
      return 1;
   }

   return 0;
}

/**
 * \param coord The clip plane index
 * \return 0 on sucess, 1 on error 
 */
static GLuint
parse_clipplane_num (GLcontext * ctx, GLubyte ** inst,
                     struct arb_program *Program, GLint * coord)
{
   *coord = parse_integer (inst, Program);

   if ((*coord < 0) || (*coord >= (GLint) ctx->Const.MaxClipPlanes)) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Invalid clip plane index");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Invalid clip plane index");
      return 1;
   }

   return 0;
}


/**
 * \return 0 on front face, 1 on back face
 */
static GLuint
parse_face_type (GLubyte ** inst)
{
   switch (*(*inst)++) {
      case FACE_FRONT:
         return 0;

      case FACE_BACK:
         return 1;
   }
   return 0;
}


/**
 * Given a matrix and a modifier token on the binary array, return tokens
 * that _mesa_fetch_state() [program.c] can understand.
 *
 * \param matrix - the matrix we are talking about
 * \param matrix_idx - the index of the matrix we have (for texture & program matricies)
 * \param matrix_modifier - the matrix modifier (trans, inv, etc)
 * \return 0 on sucess, 1 on failure
 */
static GLuint
parse_matrix (GLcontext * ctx, GLubyte ** inst, struct arb_program *Program,
              GLint * matrix, GLint * matrix_idx, GLint * matrix_modifier)
{
   GLubyte mat = *(*inst)++;

   *matrix_idx = 0;
	
   switch (mat) {
      case MATRIX_MODELVIEW:
         *matrix = STATE_MODELVIEW;
         *matrix_idx = parse_integer (inst, Program);
         if (*matrix_idx > 0) {
            _mesa_set_program_error (ctx, Program->Position,
               "ARB_vertex_blend not supported\n");
            _mesa_error (ctx, GL_INVALID_OPERATION,
               "ARB_vertex_blend not supported\n");
            return 1;
         }
         break;

      case MATRIX_PROJECTION:
         *matrix = STATE_PROJECTION;
         break;

      case MATRIX_MVP:
         *matrix = STATE_MVP;
         break;

      case MATRIX_TEXTURE:
         *matrix = STATE_TEXTURE;
         *matrix_idx = parse_integer (inst, Program);
         if (*matrix_idx >= (GLint) ctx->Const.MaxTextureUnits) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Texture Unit");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Texture Unit: %d", *matrix_idx);
            return 1;
         }
         break;

         /* This is not currently supported (ARB_matrix_palette) */
      case MATRIX_PALETTE:
         *matrix_idx = parse_integer (inst, Program);
         _mesa_set_program_error (ctx, Program->Position,
              "ARB_matrix_palette not supported\n");
         _mesa_error (ctx, GL_INVALID_OPERATION,
              "ARB_matrix_palette not supported\n");
         return 1;
         break;

      case MATRIX_PROGRAM:
         *matrix = STATE_PROGRAM;
         *matrix_idx = parse_integer (inst, Program);
         if (*matrix_idx >= (GLint) ctx->Const.MaxProgramMatrices) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Program Matrix");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Program Matrix: %d", *matrix_idx);
            return 1;
         }
         break;
   }

   switch (*(*inst)++) {
      case MATRIX_MODIFIER_IDENTITY:
         *matrix_modifier = 0;
         break;
      case MATRIX_MODIFIER_INVERSE:
         *matrix_modifier = STATE_MATRIX_INVERSE;
         break;
      case MATRIX_MODIFIER_TRANSPOSE:
         *matrix_modifier = STATE_MATRIX_TRANSPOSE;
         break;
      case MATRIX_MODIFIER_INVTRANS:
         *matrix_modifier = STATE_MATRIX_INVTRANS;
         break;
   }

   return 0;
}


/**
 * This parses a state string (rather, the binary version of it) into
 * a 6-token sequence as described in _mesa_fetch_state() [program.c]
 *
 * \param inst         - the start in the binary arry to start working from
 * \param state_tokens - the storage for the 6-token state description
 * \return             - 0 on sucess, 1 on error
 */
static GLuint
parse_state_single_item (GLcontext * ctx, GLubyte ** inst,
                         struct arb_program *Program, GLint * state_tokens)
{
   switch (*(*inst)++) {
      case STATE_MATERIAL_PARSER:
         state_tokens[0] = STATE_MATERIAL;
         state_tokens[1] = parse_face_type (inst);
         switch (*(*inst)++) {
            case MATERIAL_AMBIENT:
               state_tokens[2] = STATE_AMBIENT;
               break;
            case MATERIAL_DIFFUSE:
               state_tokens[2] = STATE_DIFFUSE;
               break;
            case MATERIAL_SPECULAR:
               state_tokens[2] = STATE_SPECULAR;
               break;
            case MATERIAL_EMISSION:
               state_tokens[2] = STATE_EMISSION;
	       break;
            case MATERIAL_SHININESS:
               state_tokens[2] = STATE_SHININESS;
               break;
         }
         break;

      case STATE_LIGHT_PARSER:
         state_tokens[0] = STATE_LIGHT;
         state_tokens[1] = parse_integer (inst, Program);

         /* Check the value of state_tokens[1] against the # of lights */
         if (state_tokens[1] >= (GLint) ctx->Const.MaxLights) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Light Number");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Light Number: %d", state_tokens[1]);
            return 1;
         }

         switch (*(*inst)++) {
            case LIGHT_AMBIENT:
               state_tokens[2] = STATE_AMBIENT;
               break;
            case LIGHT_DIFFUSE:
               state_tokens[2] = STATE_DIFFUSE;
               break;
            case LIGHT_SPECULAR:
               state_tokens[2] = STATE_SPECULAR;
               break;
            case LIGHT_POSITION:
               state_tokens[2] = STATE_POSITION;
               break;
            case LIGHT_ATTENUATION:
               state_tokens[2] = STATE_ATTENUATION;
               break;
            case LIGHT_HALF:
               state_tokens[2] = STATE_HALF;
               break;
            case LIGHT_SPOT_DIRECTION:
               state_tokens[2] = STATE_SPOT_DIRECTION;
               break;
         }
         break;

      case STATE_LIGHT_MODEL:
         switch (*(*inst)++) {
            case LIGHT_MODEL_AMBIENT:
               state_tokens[0] = STATE_LIGHTMODEL_AMBIENT;
               break;
            case LIGHT_MODEL_SCENECOLOR:
               state_tokens[0] = STATE_LIGHTMODEL_SCENECOLOR;
               state_tokens[1] = parse_face_type (inst);
               break;
         }
         break;

      case STATE_LIGHT_PROD:
         state_tokens[0] = STATE_LIGHTPROD;
         state_tokens[1] = parse_integer (inst, Program);

         /* Check the value of state_tokens[1] against the # of lights */
         if (state_tokens[1] >= (GLint) ctx->Const.MaxLights) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Light Number");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Light Number: %d", state_tokens[1]);
            return 1;
         }

         state_tokens[2] = parse_face_type (inst);
         switch (*(*inst)++) {
            case LIGHT_PROD_AMBIENT:
               state_tokens[3] = STATE_AMBIENT;
               break;
            case LIGHT_PROD_DIFFUSE:
               state_tokens[3] = STATE_DIFFUSE;
               break;
            case LIGHT_PROD_SPECULAR:
               state_tokens[3] = STATE_SPECULAR;
               break;
         }
         break;


      case STATE_FOG:
         switch (*(*inst)++) {
            case FOG_COLOR:
               state_tokens[0] = STATE_FOG_COLOR;
               break;
            case FOG_PARAMS:
               state_tokens[0] = STATE_FOG_PARAMS;
               break;
         }
         break;

      case STATE_TEX_ENV:
         state_tokens[1] = parse_integer (inst, Program);
         switch (*(*inst)++) {
            case TEX_ENV_COLOR:
               state_tokens[0] = STATE_TEXENV_COLOR;
               break;
         }
         break;

      case STATE_TEX_GEN:
         {
            GLuint type, coord;

            state_tokens[0] = STATE_TEXGEN;
            /*state_tokens[1] = parse_integer (inst, Program);*/    /* Texture Unit */

            if (parse_texcoord_num (ctx, inst, Program, &coord))
               return 1;
	    state_tokens[1] = coord;
	    
            /* EYE or OBJECT */
            type = *(*inst++);

            /* 0 - s, 1 - t, 2 - r, 3 - q */
            coord = *(*inst++);

            if (type == TEX_GEN_EYE) {
               switch (coord) {
                  case COMPONENT_X:
                     state_tokens[2] = STATE_TEXGEN_EYE_S;
                     break;
                  case COMPONENT_Y:
                     state_tokens[2] = STATE_TEXGEN_EYE_T;
                     break;
                  case COMPONENT_Z:
                     state_tokens[2] = STATE_TEXGEN_EYE_R;
                     break;
                  case COMPONENT_W:
                     state_tokens[2] = STATE_TEXGEN_EYE_Q;
                     break;
               }
            }
            else {
               switch (coord) {
                  case COMPONENT_X:
                     state_tokens[2] = STATE_TEXGEN_OBJECT_S;
                     break;
                  case COMPONENT_Y:
                     state_tokens[2] = STATE_TEXGEN_OBJECT_T;
                     break;
                  case COMPONENT_Z:
                     state_tokens[2] = STATE_TEXGEN_OBJECT_R;
                     break;
                  case COMPONENT_W:
                     state_tokens[2] = STATE_TEXGEN_OBJECT_Q;
                     break;
               }
            }
         }
         break;

      case STATE_DEPTH:
         switch (*(*inst)++) {
            case DEPTH_RANGE:
               state_tokens[0] = STATE_DEPTH_RANGE;
               break;
         }
         break;

      case STATE_CLIP_PLANE:
         state_tokens[0] = STATE_CLIPPLANE;
         state_tokens[1] = parse_integer (inst, Program);
         if (parse_clipplane_num (ctx, inst, Program, &state_tokens[1]))
            return 1;
         break;

      case STATE_POINT:
         switch (*(*inst++)) {
            case POINT_SIZE:
               state_tokens[0] = STATE_POINT_SIZE;
               break;

            case POINT_ATTENUATION:
               state_tokens[0] = STATE_POINT_ATTENUATION;
               break;
         }
         break;

         /* XXX: I think this is the correct format for a matrix row */
      case STATE_MATRIX_ROWS:
         state_tokens[0] = STATE_MATRIX;
         if (parse_matrix
             (ctx, inst, Program, &state_tokens[1], &state_tokens[2],
              &state_tokens[5]))
            return 1;

         state_tokens[3] = parse_integer (inst, Program);       /* The first row to grab */

         if ((**inst) != 0) {                                   /* Either the last row, 0 */
            state_tokens[4] = parse_integer (inst, Program);       
            if (state_tokens[4] < state_tokens[3]) {
               _mesa_set_program_error (ctx, Program->Position,
                     "Second matrix index less than the first");
               _mesa_error (ctx, GL_INVALID_OPERATION,
                     "Second matrix index (%d) less than the first (%d)", 
                     state_tokens[4], state_tokens[3]);									 
               return 1;					
            }						  
         }
         else {			
            state_tokens[4] = state_tokens[3];
            (*inst)++;
         }
         break;
   }

   return 0;
}

/**
 * This parses a state string (rather, the binary version of it) into
 * a 6-token similar for the state fetching code in program.c
 *
 * One might ask, why fetch these parameters into just like  you fetch
 * state when they are already stored in other places? 
 *
 * Because of array offsets -> We can stick env/local parameters in the
 * middle of a parameter array and then index someplace into the array
 * when we execute. 
 *
 * One optimization might be to only do this for the cases where the 
 * env/local parameters end up inside of an array, and leave the 
 * single parameters (or arrays of pure env/local pareameters) in their
 * respective register files.
 * 
 * For ENV parameters, the format is:
 *    state_tokens[0] = STATE_FRAGMENT_PROGRAM / STATE_VERTEX_PROGRAM
 *    state_tokens[1] = STATE_ENV
 *    state_tokens[2] = the parameter index
 *
 * for LOCAL parameters, the format is:
 *    state_tokens[0] = STATE_FRAGMENT_PROGRAM / STATE_VERTEX_PROGRAM
 *    state_tokens[1] = STATE_LOCAL
 *    state_tokens[2] = the parameter index
 *
 * \param inst         - the start in the binary arry to start working from
 * \param state_tokens - the storage for the 6-token state description
 * \return             - 0 on sucess, 1 on failure
 */
static GLuint
parse_program_single_item (GLcontext * ctx, GLubyte ** inst,
                           struct arb_program *Program, GLint * state_tokens)
{
   if (Program->type == GL_FRAGMENT_PROGRAM_ARB)
      state_tokens[0] = STATE_FRAGMENT_PROGRAM;
   else
      state_tokens[0] = STATE_VERTEX_PROGRAM;


   switch (*(*inst)++) {
      case PROGRAM_PARAM_ENV:
         state_tokens[1] = STATE_ENV;
         state_tokens[2] = parse_integer (inst, Program);

         /* Check state_tokens[2] against the number of ENV parameters available */
         if (((Program->type == GL_FRAGMENT_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.MaxFragmentProgramEnvParams))
             ||
             ((Program->type == GL_VERTEX_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.MaxVertexProgramEnvParams))) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Program Env Parameter");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Program Env Parameter: %d",
                         state_tokens[2]);
            return 1;
         }

         break;

      case PROGRAM_PARAM_LOCAL:
         state_tokens[1] = STATE_LOCAL;
         state_tokens[2] = parse_integer (inst, Program);

         /* Check state_tokens[2] against the number of LOCAL parameters available */
         if (((Program->type == GL_FRAGMENT_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.MaxFragmentProgramLocalParams))
             ||
             ((Program->type == GL_VERTEX_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.MaxVertexProgramLocalParams))) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Program Local Parameter");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Program Local Parameter: %d",
                         state_tokens[2]);
            return 1;
         }
         break;
   }

   return 0;
}

/**
 * For ARB_vertex_program, programs are not allowed to use both an explicit
 * vertex attribute and a generic vertex attribute corresponding to the same
 * state. See section 2.14.3.1 of the GL_ARB_vertex_program spec. 
 *
 * This will walk our var_cache and make sure that nobody does anything fishy.
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
generic_attrib_check(struct var_cache *vc_head)
{
   int a;
   struct var_cache *curr;
   GLboolean explicitAttrib[MAX_VERTEX_PROGRAM_ATTRIBS],
      genericAttrib[MAX_VERTEX_PROGRAM_ATTRIBS];

   for (a=0; a<MAX_VERTEX_PROGRAM_ATTRIBS; a++) {
      explicitAttrib[a] = GL_FALSE;
      genericAttrib[a] = GL_FALSE;
   }	
   
   curr = vc_head;
   while (curr) {
      if (curr->type == vt_attrib) {
         if (curr->attrib_is_generic)
            genericAttrib[ curr->attrib_binding_idx ] = GL_TRUE;
         else
            explicitAttrib[ curr->attrib_binding_idx ] = GL_TRUE;
      }

      curr = curr->next;
   }

   for (a=0; a<MAX_VERTEX_PROGRAM_ATTRIBS; a++) {
      if ((explicitAttrib[a]) && (genericAttrib[a]))
         return 1;
   }	

   return 0;	
}

/**
 * This will handle the binding side of an ATTRIB var declaration
 *
 * \param binding     - the fragment input register state, defined in nvfragprog.h
 * \param binding_idx - the index in the attrib register file that binding is associated with 	
 * \return returns 0 on sucess, 1 on error
 *
 * See nvfragparse.c for attrib register file layout
 */
static GLuint
parse_attrib_binding (GLcontext * ctx, GLubyte ** inst,
                      struct arb_program *Program, GLuint * binding,
                      GLuint * binding_idx, GLuint *is_generic)
{
   GLuint texcoord;	
   GLint coord;
   GLint err = 0;

   *is_generic = 0;
   if (Program->type == GL_FRAGMENT_PROGRAM_ARB) {
      switch (*(*inst)++) {
         case FRAGMENT_ATTRIB_COLOR:
            err = parse_color_type (ctx, inst, Program, &coord);
            *binding = FRAG_ATTRIB_COL0 + coord;
            *binding_idx = 1 + coord;
            break;

         case FRAGMENT_ATTRIB_TEXCOORD:
            err = parse_texcoord_num (ctx, inst, Program, &texcoord);
            *binding = FRAG_ATTRIB_TEX0 + texcoord;
            *binding_idx = 4 + texcoord;
            break;

         case FRAGMENT_ATTRIB_FOGCOORD:
            *binding = FRAG_ATTRIB_FOGC;
            *binding_idx = 3;
            break;

         case FRAGMENT_ATTRIB_POSITION:
            *binding = FRAG_ATTRIB_WPOS;
            *binding_idx = 0;
            break;

         default:
            err = 1;
            break;
      }
   }
   else {
      switch (*(*inst)++) {
         case VERTEX_ATTRIB_POSITION:
            *binding = VERT_ATTRIB_POS;
            *binding_idx = 0;
            break;

         case VERTEX_ATTRIB_WEIGHT:
            {
               GLint weight;

               err = parse_weight_num (ctx, inst, Program, &weight);
               *binding = VERT_ATTRIB_WEIGHT;
               *binding_idx = 1;
            }
            _mesa_set_program_error (ctx, Program->Position,
                 "ARB_vertex_blend not supported\n");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                 "ARB_vertex_blend not supported\n");
            return 1;
            break;

         case VERTEX_ATTRIB_NORMAL:
            *binding = VERT_ATTRIB_NORMAL;
            *binding_idx = 2;
            break;

         case VERTEX_ATTRIB_COLOR:
            {
               GLint color;

               err = parse_color_type (ctx, inst, Program, &color);
               if (color) {
                  *binding = VERT_ATTRIB_COLOR1;
                  *binding_idx = 4;
               }
               else {
                  *binding = VERT_ATTRIB_COLOR0;
                  *binding_idx = 3;
               }
            }
            break;

         case VERTEX_ATTRIB_FOGCOORD:
            *binding = VERT_ATTRIB_FOG;
            *binding_idx = 5;
            break;

         case VERTEX_ATTRIB_TEXCOORD:
            {
               GLuint unit;

               err = parse_texcoord_num (ctx, inst, Program, &unit);
               *binding = VERT_ATTRIB_TEX0 + unit;
               *binding_idx = 8 + unit;
            }
            break;

            /* It looks like we don't support this at all, atm */
         case VERTEX_ATTRIB_MATRIXINDEX:
            parse_integer (inst, Program);
            _mesa_set_program_error (ctx, Program->Position,
                  "ARB_palette_matrix not supported");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                  "ARB_palette_matrix not supported");
            return 1;
            break;

         case VERTEX_ATTRIB_GENERIC:
            {
               GLuint attrib;

               if (!parse_generic_attrib_num(ctx, inst, Program, &attrib)) {
                  *is_generic = 1;                   
                  switch (attrib) {
                     case 0:
                        *binding = VERT_ATTRIB_POS;
                        break;
                     case 1:
                        *binding = VERT_ATTRIB_WEIGHT;
                        break;
                     case 2:
                        *binding = VERT_ATTRIB_NORMAL;
                        break;
                     case 3:
                        *binding = VERT_ATTRIB_COLOR0;
                        break;
                     case 4:
                        *binding = VERT_ATTRIB_COLOR1;
                        break;
                     case 5:
                        *binding = VERT_ATTRIB_FOG;
                        break;
                     case 6:
                        break;
                     case 7:
                        break;
                     default:
                        *binding = VERT_ATTRIB_TEX0 + (attrib-8);
                        break;
                  }
                  *binding_idx = attrib;
               }
            }
            break;

         default:
            err = 1;
            break;
      }
   }

   /* Can this even happen? */
   if (err) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Bad attribute binding");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Bad attribute binding");
   }

   Program->InputsRead |= (1 << *binding_idx);

   return err;
}

/**
 * This translates between a binary token for an output variable type
 * and the mesa token for the same thing.
 *
 *
 * XXX: What is the 'name' for vertex program state? -> do we need it?
 *         I don't think we do;
 *
 * See nvfragprog.h for definitions 
 *
 * \param inst        - The parsed tokens
 * \param binding     - The name of the state we are binding too
 * \param binding_idx - The index into the result register file that this is bound too
 *
 * See nvfragparse.c for the register file layout for fragment programs
 * See nvvertparse.c for the register file layout for vertex programs
 */
static GLuint
parse_result_binding (GLcontext * ctx, GLubyte ** inst, GLuint * binding,
                      GLuint * binding_idx, struct arb_program *Program)
{
   GLuint b;

   switch (*(*inst)++) {
      case FRAGMENT_RESULT_COLOR:
         /* for frag programs, this is FRAGMENT_RESULT_COLOR */
         if (Program->type == GL_FRAGMENT_PROGRAM_ARB) {
            *binding = FRAG_OUTPUT_COLR;
            *binding_idx = 0;
         }
         /* for vtx programs, this is VERTEX_RESULT_POSITION */
         else {
            *binding_idx = 0;
         }
         break;

      case FRAGMENT_RESULT_DEPTH:
         /* for frag programs, this is FRAGMENT_RESULT_DEPTH */
         if (Program->type == GL_FRAGMENT_PROGRAM_ARB) {
            *binding = FRAG_OUTPUT_DEPR;
            *binding_idx = 2;
         }
         /* for vtx programs, this is VERTEX_RESULT_COLOR */
         else {
            GLint color_type;
            GLuint face_type = parse_face_type(inst);
	    GLint color_type_ret = parse_color_type(ctx, inst, Program, &color_type);

            /* back face */
            if (face_type) {
               if (color_type_ret) return 1; 

               /* secondary color */
               if (color_type) {
                  *binding_idx = 4;
               }
               /*  primary color */
               else {
                  *binding_idx = 3;
               }
            }
            /* front face */
            else {
               /* secondary color */
               if (color_type) {
                  *binding_idx = 2;
               }
               /* primary color */
               else {
                  *binding_idx = 1;
               }
            }
         }
         break;

      case VERTEX_RESULT_FOGCOORD:
         *binding_idx = 5;
         break;

      case VERTEX_RESULT_POINTSIZE:
         *binding_idx = 6;
         break;

      case VERTEX_RESULT_TEXCOORD:
         if (parse_texcoord_num (ctx, inst, Program, &b))
            return 1;
         *binding_idx = 7 + b;
         break;
   }

   Program->OutputsWritten |= (1 << *binding_idx);

   return 0;
}

/**
 * This handles the declaration of ATTRIB variables
 *
 * XXX: Still needs 
 *      parse_vert_attrib_binding(), or something like that
 *
 * \return 0 on sucess, 1 on error
 */
static GLint
parse_attrib (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
              struct arb_program *Program)
{
   GLuint found;
   char *error_msg;
   struct var_cache *attrib_var;

   attrib_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);
   if (found) {
      error_msg = (char *)
         _mesa_malloc (_mesa_strlen ((char *) attrib_var->name) + 40);
      _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                     attrib_var->name);

      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

      _mesa_free (error_msg);
      return 1;
   }

   attrib_var->type = vt_attrib;

   /* I think this is ok now - karl */
   /* XXX: */
   /*if (Program->type == GL_FRAGMENT_PROGRAM_ARB) */
   {
      if (parse_attrib_binding
          (ctx, inst, Program, &attrib_var->attrib_binding,
           &attrib_var->attrib_binding_idx, &attrib_var->attrib_is_generic))
         return 1;
      if (generic_attrib_check(*vc_head)) {
         _mesa_set_program_error (ctx, Program->Position, 
   "Cannot use both a generic vertex attribute and a specific attribute of the same type");
         _mesa_error (ctx, GL_INVALID_OPERATION, 
   "Cannot use both a generic vertex attribute and a specific attribute of the same type");
         return 1;				
      }

   }

   Program->Base.NumAttributes++;
   return 0;
}

/**
 * \param use -- TRUE if we're called when declaring implicit parameters,
 *               FALSE if we're declaraing variables. This has to do with
 *               if we get a signed or unsigned float for scalar constants
 */
static GLuint
parse_param_elements (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache *param_var,
                      struct arb_program *Program, GLboolean use)
{
   GLint idx;
   GLuint err;
   GLint state_tokens[6];
   GLfloat const_values[4];

   err = 0;

   switch (*(*inst)++) {
      case PARAM_STATE_ELEMENT:

         if (parse_state_single_item (ctx, inst, Program, state_tokens))
            return 1;

         /* If we adding STATE_MATRIX that has multiple rows, we need to
          * unroll it and call _mesa_add_state_reference() for each row
          */
         if ((state_tokens[0] == STATE_MATRIX)
             && (state_tokens[3] != state_tokens[4])) {
            GLint row;
            GLint first_row = state_tokens[3];
            GLint last_row = state_tokens[4];

            for (row = first_row; row <= last_row; row++) {
               state_tokens[3] = state_tokens[4] = row;

               idx =
                  _mesa_add_state_reference (Program->Parameters,
                                             state_tokens);
               if (param_var->param_binding_begin == ~0U)
                  param_var->param_binding_begin = idx;
               param_var->param_binding_length++;
               Program->Base.NumParameters++;
            }
         }
         else {
            idx =
               _mesa_add_state_reference (Program->Parameters, state_tokens);
            if (param_var->param_binding_begin == ~0U)
               param_var->param_binding_begin = idx;
            param_var->param_binding_length++;
            Program->Base.NumParameters++;
         }
         break;

      case PARAM_PROGRAM_ELEMENT:

         if (parse_program_single_item (ctx, inst, Program, state_tokens))
            return 1;
         idx = _mesa_add_state_reference (Program->Parameters, state_tokens);
         if (param_var->param_binding_begin == ~0U)
            param_var->param_binding_begin = idx;
         param_var->param_binding_length++;
         Program->Base.NumParameters++;

         /* Check if there is more: 0 -> we're done, else its an integer */
         if (**inst) {
            GLuint out_of_range, new_idx;
            GLuint start_idx = state_tokens[2] + 1;
            GLuint end_idx = parse_integer (inst, Program);

            out_of_range = 0;
            if (Program->type == GL_FRAGMENT_PROGRAM_ARB) {
               if (((state_tokens[1] == STATE_ENV)
                    && (end_idx >= ctx->Const.MaxFragmentProgramEnvParams))
                   || ((state_tokens[1] == STATE_LOCAL)
                       && (end_idx >=
                           ctx->Const.MaxFragmentProgramLocalParams)))
                  out_of_range = 1;
            }
            else {
               if (((state_tokens[1] == STATE_ENV)
                    && (end_idx >= ctx->Const.MaxVertexProgramEnvParams))
                   || ((state_tokens[1] == STATE_LOCAL)
                       && (end_idx >=
                           ctx->Const.MaxVertexProgramLocalParams)))
                  out_of_range = 1;
            }
            if (out_of_range) {
               _mesa_set_program_error (ctx, Program->Position,
                                        "Invalid Program Parameter");
               _mesa_error (ctx, GL_INVALID_OPERATION,
                            "Invalid Program Parameter: %d", end_idx);
               return 1;
            }

            for (new_idx = start_idx; new_idx <= end_idx; new_idx++) {
               state_tokens[2] = new_idx;
               idx =
                  _mesa_add_state_reference (Program->Parameters,
                                             state_tokens);
               param_var->param_binding_length++;
               Program->Base.NumParameters++;
            }
         }
			else
			{
				(*inst)++;
			}
         break;

      case PARAM_CONSTANT:
         parse_constant (inst, const_values, Program, use);
         idx =
            _mesa_add_named_constant (Program->Parameters,
                                      (char *) param_var->name, const_values);
         if (param_var->param_binding_begin == ~0U)
            param_var->param_binding_begin = idx;
         param_var->param_binding_length++;
         Program->Base.NumParameters++;
         break;

      default:
         _mesa_set_program_error (ctx, Program->Position,
                                  "Unexpected token in parse_param_elements()");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Unexpected token in parse_param_elements()");
         return 1;
   }

   /* Make sure we haven't blown past our parameter limits */
   if (((Program->type == GL_VERTEX_PROGRAM_ARB) &&
        (Program->Base.NumParameters >=
         ctx->Const.MaxVertexProgramLocalParams))
       || ((Program->type == GL_FRAGMENT_PROGRAM_ARB)
           && (Program->Base.NumParameters >=
               ctx->Const.MaxFragmentProgramLocalParams))) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Too many parameter variables");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Too many parameter variables");
      return 1;
   }

   return err;
}

/**
 * This picks out PARAM program parameter bindings.
 *
 * XXX: This needs to be stressed & tested
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_param (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
             struct arb_program *Program)
{
   GLuint found, err;
   GLint specified_length;
   char *error_msg;
   struct var_cache *param_var;

   err = 0;
   param_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);

   if (found) {
      error_msg = (char *) _mesa_malloc (_mesa_strlen ((char *) param_var->name) + 40);
      _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                     param_var->name);

      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

      _mesa_free (error_msg);
      return 1;
   }

   specified_length = parse_integer (inst, Program);

   if (specified_length < 0) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Negative parameter array length");
      _mesa_error (ctx, GL_INVALID_OPERATION,
                   "Negative parameter array length: %d", specified_length);
      return 1;
   }

   param_var->type = vt_param;
   param_var->param_binding_length = 0;

   /* Right now, everything is shoved into the main state register file.
    * 
    * In the future, it would be nice to leave things ENV/LOCAL params
    * in their respective register files, if possible 
    */
   param_var->param_binding_type = PROGRAM_STATE_VAR;

   /* Remember to:
    * *   - add each guy to the parameter list
    * *   - increment the param_var->param_binding_len
    * *   - store the param_var->param_binding_begin for the first one
    * *   - compare the actual len to the specified len at the end
    */
   while (**inst != PARAM_NULL) {
      if (parse_param_elements (ctx, inst, param_var, Program, GL_FALSE))
         return 1;
   }

   /* Test array length here! */
   if (specified_length) {
      if (specified_length != param_var->param_binding_length) {
         _mesa_set_program_error (ctx, Program->Position,
                                  "Declared parameter array lenght does not match parameter list");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Declared parameter array lenght does not match parameter list");
      }
   }

   (*inst)++;

   return 0;
}

/**
 *
 */
static GLuint
parse_param_use (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
                 struct arb_program *Program, struct var_cache **new_var)
{
   struct var_cache *param_var;

   /* First, insert a dummy entry into the var_cache */
   var_cache_create (&param_var);
   param_var->name = (GLubyte *) _mesa_strdup (" ");
   param_var->type = vt_param;

   param_var->param_binding_length = 0;
   /* Don't fill in binding_begin; We use the default value of -1
    * to tell if its already initialized, elsewhere.
    *
    * param_var->param_binding_begin  = 0;
    */
   param_var->param_binding_type = PROGRAM_STATE_VAR;

   var_cache_append (vc_head, param_var);

   /* Then fill it with juicy parameter goodness */
   if (parse_param_elements (ctx, inst, param_var, Program, GL_TRUE))
      return 1;

   *new_var = param_var;

   return 0;
}


/**
 * This handles the declaration of TEMP variables
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_temp (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
            struct arb_program *Program)
{
   GLuint found;
   struct var_cache *temp_var;
   char *error_msg;

   while (**inst != 0) {
      temp_var = parse_string (inst, vc_head, Program, &found);
      Program->Position = parse_position (inst);
      if (found) {
         error_msg = (char *)
            _mesa_malloc (_mesa_strlen ((char *) temp_var->name) + 40);
         _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                        temp_var->name);

         _mesa_set_program_error (ctx, Program->Position, error_msg);
         _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

         _mesa_free (error_msg);
         return 1;
      }

      temp_var->type = vt_temp;

      if (((Program->type == GL_FRAGMENT_PROGRAM_ARB) &&
           (Program->Base.NumTemporaries >=
            ctx->Const.MaxFragmentProgramTemps))
          || ((Program->type == GL_VERTEX_PROGRAM_ARB)
              && (Program->Base.NumTemporaries >=
                  ctx->Const.MaxVertexProgramTemps))) {
         _mesa_set_program_error (ctx, Program->Position,
                                  "Too many TEMP variables declared");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Too many TEMP variables declared");
         return 1;
      }

      temp_var->temp_binding = Program->Base.NumTemporaries;
      Program->Base.NumTemporaries++;
   }
   (*inst)++;

   return 0;
}

/**
 * This handles variables of the OUTPUT variety
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_output (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
              struct arb_program *Program)
{
   GLuint found;
   struct var_cache *output_var;

   output_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);
   if (found) {
      char *error_msg;
      error_msg = (char *)
         _mesa_malloc (_mesa_strlen ((char *) output_var->name) + 40);
      _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                     output_var->name);

      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

      _mesa_free (error_msg);
      return 1;
   }

   output_var->type = vt_output;
   return parse_result_binding (ctx, inst, &output_var->output_binding,
                                &output_var->output_binding_idx, Program);
}

/**
 * This handles variables of the ALIAS kind
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_alias (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
             struct arb_program *Program)
{
   GLuint found;
   struct var_cache *temp_var;
   char *error_msg;

	
   temp_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);

   if (found) {
      error_msg = (char *)
         _mesa_malloc (_mesa_strlen ((char *) temp_var->name) + 40);
      _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                     temp_var->name);

      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

      _mesa_free (error_msg);
      return 1;
   }

   temp_var->type = vt_alias;
   temp_var->alias_binding =  parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);

   if (!found)
   {
      error_msg = (char *)
         _mesa_malloc (_mesa_strlen ((char *) temp_var->name) + 40);
      _mesa_sprintf (error_msg, "Alias value %s is not defined",
                     temp_var->alias_binding->name);
      
      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);
      
      _mesa_free (error_msg);
      return 1;
   }

   return 0;
}

/**
 * This handles variables of the ADDRESS kind
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_address (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
               struct arb_program *Program)
{
   GLuint found;
   struct var_cache *temp_var;
   char *error_msg;

   while (**inst != 0) {
      temp_var = parse_string (inst, vc_head, Program, &found);
      Program->Position = parse_position (inst);
      if (found) {
         error_msg = (char *)
            _mesa_malloc (_mesa_strlen ((char *) temp_var->name) + 40);
         _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                        temp_var->name);

         _mesa_set_program_error (ctx, Program->Position, error_msg);
         _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

         _mesa_free (error_msg);
         return 1;
      }

      temp_var->type = vt_address;

      if (Program->Base.NumAddressRegs >=
          ctx->Const.MaxVertexProgramAddressRegs) {
         _mesa_set_program_error (ctx, Program->Position,
                                  "Too many ADDRESS variables declared");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Too many ADDRESS variables declared");
         return 1;
      }

      temp_var->address_binding = Program->Base.NumAddressRegs;
      Program->Base.NumAddressRegs++;
   }
   (*inst)++;

   return 0;
}

/**
 * Parse a program declaration 
 *
 * \return 0 on sucess, 1 on error
 */
static GLint
parse_declaration (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
                   struct arb_program *Program)
{
   GLint err = 0;

   switch (*(*inst)++) {
      case ADDRESS:
         err = parse_address (ctx, inst, vc_head, Program);
         break;

      case ALIAS:
         err = parse_alias (ctx, inst, vc_head, Program);
         break;

      case ATTRIB:
         err = parse_attrib (ctx, inst, vc_head, Program);
         break;

      case OUTPUT:
         err = parse_output (ctx, inst, vc_head, Program);
         break;

      case PARAM:
         err = parse_param (ctx, inst, vc_head, Program);
         break;

      case TEMP:
         err = parse_temp (ctx, inst, vc_head, Program);
         break;
   }

   return err;
}

/**
 * Handle the parsing out of a masked destination register
 *
 * If we are a vertex program, make sure we don't write to
 * result.position of we have specified that the program is
 * position invariant
 * 
 * \param File      - The register file we write to
 * \param Index     - The register index we write to
 * \param WriteMask - The mask controlling which components we write (1->write)
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_masked_dst_reg (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      GLint * File, GLint * Index, GLboolean * WriteMask)
{
   GLuint result;
   GLubyte mask;
   struct var_cache *dst;

   /* We either have a result register specified, or a
    * variable that may or may not be writable 
    */
   switch (*(*inst)++) {
      case REGISTER_RESULT:
         if (parse_result_binding
             (ctx, inst, &result, (GLuint *) Index, Program))
            return 1;
         *File = PROGRAM_OUTPUT;
         break;

      case REGISTER_ESTABLISHED_NAME:
         dst = parse_string (inst, vc_head, Program, &result);
         Program->Position = parse_position (inst);

         /* If the name has never been added to our symbol table, we're hosed */
         if (!result) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "0: Undefined variable");
            _mesa_error (ctx, GL_INVALID_OPERATION, "0: Undefined variable: %s",
                         dst->name);
            return 1;
         }

         switch (dst->type) {
            case vt_output:
               *File = PROGRAM_OUTPUT;
               *Index = dst->output_binding_idx;
               break;

            case vt_temp:
               *File = PROGRAM_TEMPORARY;
               *Index = dst->temp_binding;
               break;

               /* If the var type is not vt_output or vt_temp, no go */
            default:
               _mesa_set_program_error (ctx, Program->Position,
                                        "Destination register is read only");
               _mesa_error (ctx, GL_INVALID_OPERATION,
                            "Destination register is read only: %s",
                            dst->name);
               return 1;
         }
         break;

      default:
         _mesa_set_program_error (ctx, Program->Position,
                                  "Unexpected opcode in parse_masked_dst_reg()");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Unexpected opcode in parse_masked_dst_reg()");
         return 1;
   }


   /* Position invariance test */	
   if ((Program->HintPositionInvariant) && (*File == PROGRAM_OUTPUT) &&
      (*Index == 0))   {
      _mesa_set_program_error (ctx, Program->Position,
                  "Vertex program specified position invariance and wrote vertex position");
      _mesa_error (ctx, GL_INVALID_OPERATION,
                  "Vertex program specified position invariance and wrote vertex position");
   }
	
   /* And then the mask.
    *  w,a -> bit 0
    *  z,b -> bit 1
    *  y,g -> bit 2
    *  x,r -> bit 3
    */
   mask = *(*inst)++;

   WriteMask[0] = (GLboolean) (mask & (1 << 3)) >> 3;
   WriteMask[1] = (GLboolean) (mask & (1 << 2)) >> 2;
   WriteMask[2] = (GLboolean) (mask & (1 << 1)) >> 1;
   WriteMask[3] = (GLboolean) (mask & (1));

   return 0;
}


/** 
 * Handle the parsing of a address register 
 *
 * \param Index     - The register index we write to
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_address_reg (GLcontext * ctx, GLubyte ** inst,
                          struct var_cache **vc_head,
                          struct arb_program *Program, GLint * Index)
{
   struct var_cache *dst;
   GLuint result;

   dst = parse_string (inst, vc_head, Program, &result);
   Program->Position = parse_position (inst);

   /* If the name has never been added to our symbol table, we're hosed */
   if (!result) {
      _mesa_set_program_error (ctx, Program->Position, "Undefined variable");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Undefined variable: %s",
                   dst->name);
      return 1;
   }

   if (dst->type != vt_address) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Variable is not of type ADDRESS");
      _mesa_error (ctx, GL_INVALID_OPERATION,
                   "Variable: %s is not of type ADDRESS", dst->name);
      return 1;
   }

   return 0;
}

/**
 * Handle the parsing out of a masked address register
 *
 * \param Index     - The register index we write to
 * \param WriteMask - The mask controlling which components we write (1->write)
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_masked_address_reg (GLcontext * ctx, GLubyte ** inst,
                          struct var_cache **vc_head,
                          struct arb_program *Program, GLint * Index,
                          GLboolean * WriteMask)
{
   if (parse_address_reg (ctx, inst, vc_head, Program, Index))
      return 1;	     

   /* This should be 0x8 */
   (*inst)++;

   /* Writemask of .x is implied */
   WriteMask[0] = 1;
   WriteMask[1] = WriteMask[2] = WriteMask[3] = 0;

   return 0;
}


/** 
 * Parse out a swizzle mask.
 *
 * The values in the input stream are: 
 *   COMPONENT_X -> x/r
 *   COMPONENT_Y -> y/g
 *   COMPONENT_Z-> z/b
 *   COMPONENT_W-> w/a
 *
 * The values in the output mask are:
 *   0 -> x/r
 *   1 -> y/g
 *   2 -> z/b
 *   3 -> w/a
 *
 * The len parameter allows us to grab 4 components for a vector
 * swizzle, or just 1 component for a scalar src register selection
 */
static GLuint
parse_swizzle_mask (GLubyte ** inst, GLubyte * mask, GLint len)
{
   GLint a;

   for (a = 0; a < 4; a++)
      mask[a] = a;

   for (a = 0; a < len; a++) {
      switch (*(*inst)++) {
         case COMPONENT_X:
            mask[a] = 0;
            break;

         case COMPONENT_Y:
            mask[a] = 1;
            break;

         case COMPONENT_Z:
            mask[a] = 2;
            break;

         case COMPONENT_W:
            mask[a] = 3;
            break;
      }
   }

   return 0;
}

/** 
 */
static GLuint
parse_extended_swizzle_mask (GLubyte ** inst, GLubyte * mask, GLboolean * Negate)
{
   GLint a;
   GLubyte swz;

   *Negate = GL_FALSE;
   for (a = 0; a < 4; a++) {
      if (parse_sign (inst))
         *Negate = GL_TRUE;

      swz = *(*inst)++;

      switch (swz) {
         case COMPONENT_0:
            mask[a] = SWIZZLE_ZERO;
            break;
         case COMPONENT_1:
            mask[a] = SWIZZLE_ONE;
            break;
         case COMPONENT_X:
            mask[a] = 0;
            break;
         case COMPONENT_Y:
            mask[a] = 1;
            break;
         case COMPONENT_Z:
            mask[a] = 2;
            break;
         case COMPONENT_W:
            mask[a] = 3;
            break;

      }
#if 0
      if (swz == 0)
         mask[a] = SWIZZLE_ZERO;
      else if (swz == 1)
         mask[a] = SWIZZLE_ONE;
      else
         mask[a] = swz - 2;
#endif

   }

   return 0;
}


static GLuint
parse_src_reg (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
               struct arb_program *Program, GLint * File, GLint * Index,
               GLboolean *IsRelOffset )
{
   struct var_cache *src;
   GLuint binding_state, binding_idx, is_generic, found;
   GLint offset;

   /* And the binding for the src */
   switch (*(*inst)++) {
      case REGISTER_ATTRIB:
         if (parse_attrib_binding
             (ctx, inst, Program, &binding_state, &binding_idx, &is_generic))
            return 1;
         *File = PROGRAM_INPUT;
         *Index = binding_idx;

         /* We need to insert a dummy variable into the var_cache so we can 
          * catch generic vertex attrib aliasing errors 
          */
         var_cache_create(&src);
         src->type = vt_attrib;
         src->name = (GLubyte *)_mesa_strdup("Dummy Attrib Variable");
         src->attrib_binding     = binding_state;
         src->attrib_binding_idx = binding_idx;
         src->attrib_is_generic  = is_generic;
         var_cache_append(vc_head, src);
         if (generic_attrib_check(*vc_head)) {
            _mesa_set_program_error (ctx, Program->Position, 
   "Cannot use both a generic vertex attribute and a specific attribute of the same type");
            _mesa_error (ctx, GL_INVALID_OPERATION, 
   "Cannot use both a generic vertex attribute and a specific attribute of the same type");
            return 1;				
         }
         break;

      case REGISTER_PARAM:
         switch (**inst) {
            case PARAM_ARRAY_ELEMENT:
               (*inst)++;
               src = parse_string (inst, vc_head, Program, &found);
               Program->Position = parse_position (inst);

               if (!found) {
                  _mesa_set_program_error (ctx, Program->Position,
                                           "2: Undefined variable");
                  _mesa_error (ctx, GL_INVALID_OPERATION,
                               "2: Undefined variable: %s", src->name);
                  return 1;
               }

               *File = src->param_binding_type;

               switch (*(*inst)++) {
                  case ARRAY_INDEX_ABSOLUTE:
                     offset = parse_integer (inst, Program);

                     if ((offset < 0)
                         || (offset >= src->param_binding_length)) {
                        _mesa_set_program_error (ctx, Program->Position,
                                                 "Index out of range");
                        _mesa_error (ctx, GL_INVALID_OPERATION,
                                     "Index %d out of range for %s", offset,
                                     src->name);
                        return 1;
                     }

                     *Index = src->param_binding_begin + offset;
                     break;

                  case ARRAY_INDEX_RELATIVE:
                     {
                        GLint addr_reg_idx, rel_off;

                        /* First, grab the address regiseter */
                        if (parse_address_reg (ctx, inst, vc_head, Program, &addr_reg_idx))
                           return 1;					   

                        /* And the .x */			   
                        ((*inst)++);
                        ((*inst)++);
                        ((*inst)++);
                        ((*inst)++);

                        /* Then the relative offset */
                        if (parse_relative_offset(ctx, inst, Program, &rel_off)) return 1;	

                        /* And store it properly */
                        *Index = src->param_binding_begin + rel_off;
                        *IsRelOffset = 1;				
                     }
                     break;
               }
               break;

            default:

               if (parse_param_use (ctx, inst, vc_head, Program, &src))
                  return 1;

               *File = src->param_binding_type;
               *Index = src->param_binding_begin;
               break;
         }
         break;

      case REGISTER_ESTABLISHED_NAME:

         src = parse_string (inst, vc_head, Program, &found);
         Program->Position = parse_position (inst);

         /* If the name has never been added to our symbol table, we're hosed */
         if (!found) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "3: Undefined variable");
            _mesa_error (ctx, GL_INVALID_OPERATION, "3: Undefined variable: %s",
                         src->name);
            return 1;
         }

         switch (src->type) {
            case vt_attrib:
               *File = PROGRAM_INPUT;
               *Index = src->attrib_binding_idx;
               break;

               /* XXX: We have to handle offsets someplace in here!  -- or are those above? */
            case vt_param:
               *File = src->param_binding_type;
               *Index = src->param_binding_begin;
               break;

            case vt_temp:
               *File = PROGRAM_TEMPORARY;
               *Index = src->temp_binding;
               break;

               /* If the var type is vt_output no go */
            default:
               _mesa_set_program_error (ctx, Program->Position,
                                        "destination register is read only");
               _mesa_error (ctx, GL_INVALID_OPERATION,
                            "destination register is read only: %s",
                            src->name);
               return 1;
         }
         break;

      default:
         _mesa_set_program_error (ctx, Program->Position,
                                  "Unknown token in parse_src_reg");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Unknown token in parse_src_reg");
         return 1;
   }

   return 0;
}

/**
 */
static GLuint
parse_vector_src_reg (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      GLint * File, GLint * Index, GLboolean * Negate,
                      GLubyte * Swizzle, GLboolean *IsRelOffset)
{
   /* Grab the sign */
   *Negate = parse_sign (inst);

   /* And the src reg */
   if (parse_src_reg (ctx, inst, vc_head, Program, File, Index, IsRelOffset))
      return 1;

   /* finally, the swizzle */
   parse_swizzle_mask (inst, Swizzle, 4);
   
   return 0;
}

/**
 */
static GLuint
parse_scalar_src_reg (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      GLint * File, GLint * Index, GLboolean * Negate,
                      GLubyte * Swizzle, GLboolean *IsRelOffset)
{
   /* Grab the sign */
   *Negate = parse_sign (inst);

   /* And the src reg */
   if (parse_src_reg (ctx, inst, vc_head, Program, File, Index, IsRelOffset))
      return 1;

   /* Now, get the component and shove it into all the swizzle slots  */
   parse_swizzle_mask (inst, Swizzle, 1);

   return 0;
}

/**
 * This is a big mother that handles getting opcodes into the instruction
 * and handling the src & dst registers for fragment program instructions
 */
static GLuint
parse_fp_instruction (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      struct fp_instruction *fp)
{
   GLint a, b;
   GLubyte swz[4]; /* FP's swizzle mask is a GLubyte, while VP's is GLuint */
   GLuint texcoord;
   GLubyte instClass, type, code;
   GLboolean rel;

   /* No condition codes in ARB_fp */
   fp->UpdateCondRegister = 0;

   /* Record the position in the program string for debugging */
   fp->StringPos = Program->Position;

   /* OP_ALU_INST or OP_TEX_INST */
   instClass = *(*inst)++;

   /* OP_ALU_{VECTOR, SCALAR, BINSC, BIN, TRI, SWZ}, 
    * OP_TEX_{SAMPLE, KIL}
    */
   type = *(*inst)++;

   /* The actual opcode name */
   code = *(*inst)++;

   /* Increment the correct count */
   switch (instClass) {
      case OP_ALU_INST:
         Program->NumAluInstructions++;
         break;
      case OP_TEX_INST:
         Program->NumTexInstructions++;
         break;
   }

   fp->Saturate = 0;
   fp->Precision = FLOAT32;

   fp->DstReg.CondMask = COND_TR;

   switch (type) {
      case OP_ALU_VECTOR:
         switch (code) {
            case OP_ABS_SAT:
               fp->Saturate = 1;
            case OP_ABS:
               fp->Opcode = FP_OPCODE_ABS;
               break;

            case OP_FLR_SAT:
               fp->Saturate = 1;
            case OP_FLR:
               fp->Opcode = FP_OPCODE_FLR;
               break;

            case OP_FRC_SAT:
               fp->Saturate = 1;
            case OP_FRC:
               fp->Opcode = FP_OPCODE_FRC;
               break;

            case OP_LIT_SAT:
               fp->Saturate = 1;
            case OP_LIT:
               fp->Opcode = FP_OPCODE_LIT;
               break;

            case OP_MOV_SAT:
               fp->Saturate = 1;
            case OP_MOV:
               fp->Opcode = FP_OPCODE_MOV;
               break;
         }

         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->DstReg.File,
              &fp->DstReg.Index, fp->DstReg.WriteMask))
            return 1;

         fp->SrcReg[0].Abs = GL_FALSE;
         fp->SrcReg[0].NegateAbs = GL_FALSE;
         if (parse_vector_src_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->SrcReg[0].File,
              &fp->SrcReg[0].Index, &fp->SrcReg[0].NegateBase,
              swz, &rel))
            return 1;
         for (b=0; b<4; b++)
            fp->SrcReg[0].Swizzle[b] = swz[b];
         break;

      case OP_ALU_SCALAR:
         switch (code) {
            case OP_COS_SAT:
               fp->Saturate = 1;
            case OP_COS:
               fp->Opcode = FP_OPCODE_COS;
               break;

            case OP_EX2_SAT:
               fp->Saturate = 1;
            case OP_EX2:
               fp->Opcode = FP_OPCODE_EX2;
               break;

            case OP_LG2_SAT:
               fp->Saturate = 1;
            case OP_LG2:
               fp->Opcode = FP_OPCODE_LG2;
               break;

            case OP_RCP_SAT:
               fp->Saturate = 1;
            case OP_RCP:
               fp->Opcode = FP_OPCODE_RCP;
               break;

            case OP_RSQ_SAT:
               fp->Saturate = 1;
            case OP_RSQ:
               fp->Opcode = FP_OPCODE_RSQ;
               break;

            case OP_SIN_SAT:
               fp->Saturate = 1;
            case OP_SIN:
               fp->Opcode = FP_OPCODE_SIN;
               break;

            case OP_SCS_SAT:
               fp->Saturate = 1;
            case OP_SCS:
	      
               fp->Opcode = FP_OPCODE_SCS;
               break;
         }

         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->DstReg.File,
              &fp->DstReg.Index, fp->DstReg.WriteMask))
            return 1;
         fp->SrcReg[0].Abs = GL_FALSE;
         fp->SrcReg[0].NegateAbs = GL_FALSE;
         if (parse_scalar_src_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->SrcReg[0].File,
              &fp->SrcReg[0].Index, &fp->SrcReg[0].NegateBase,
              swz, &rel))
            return 1;
         for (b=0; b<4; b++)
            fp->SrcReg[0].Swizzle[b] = swz[b];
         break;

      case OP_ALU_BINSC:
         switch (code) {
            case OP_POW_SAT:
               fp->Saturate = 1;
            case OP_POW:
               fp->Opcode = FP_OPCODE_POW;
               break;
         }

         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->DstReg.File,
              &fp->DstReg.Index, fp->DstReg.WriteMask))
            return 1;
         for (a = 0; a < 2; a++) {
            fp->SrcReg[a].Abs = GL_FALSE;
            fp->SrcReg[a].NegateAbs = GL_FALSE;
            if (parse_scalar_src_reg
                (ctx, inst, vc_head, Program, (GLint *) & fp->SrcReg[a].File,
                 &fp->SrcReg[a].Index, &fp->SrcReg[a].NegateBase,
                 swz, &rel))
               return 1;
            for (b=0; b<4; b++)
               fp->SrcReg[a].Swizzle[b] = swz[b];
         }
         break;


      case OP_ALU_BIN:
         switch (code) {
            case OP_ADD_SAT:
               fp->Saturate = 1;
            case OP_ADD:
               fp->Opcode = FP_OPCODE_ADD;
               break;

            case OP_DP3_SAT:
               fp->Saturate = 1;
            case OP_DP3:
               fp->Opcode = FP_OPCODE_DP3;
               break;

            case OP_DP4_SAT:
               fp->Saturate = 1;
            case OP_DP4:
               fp->Opcode = FP_OPCODE_DP4;
               break;

            case OP_DPH_SAT:
               fp->Saturate = 1;
            case OP_DPH:
               fp->Opcode = FP_OPCODE_DPH;
               break;

            case OP_DST_SAT:
               fp->Saturate = 1;
            case OP_DST:
               fp->Opcode = FP_OPCODE_DST;
               break;

            case OP_MAX_SAT:
               fp->Saturate = 1;
            case OP_MAX:
               fp->Opcode = FP_OPCODE_MAX;
               break;

            case OP_MIN_SAT:
               fp->Saturate = 1;
            case OP_MIN:
               fp->Opcode = FP_OPCODE_MIN;
               break;

            case OP_MUL_SAT:
               fp->Saturate = 1;
            case OP_MUL:
               fp->Opcode = FP_OPCODE_MUL;
               break;

            case OP_SGE_SAT:
               fp->Saturate = 1;
            case OP_SGE:
               fp->Opcode = FP_OPCODE_SGE;
               break;

            case OP_SLT_SAT:
               fp->Saturate = 1;
            case OP_SLT:
               fp->Opcode = FP_OPCODE_SLT;
               break;

            case OP_SUB_SAT:
               fp->Saturate = 1;
            case OP_SUB:
               fp->Opcode = FP_OPCODE_SUB;
               break;

            case OP_XPD_SAT:
               fp->Saturate = 1;
            case OP_XPD:
               fp->Opcode = FP_OPCODE_XPD;
               break;
         }

         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->DstReg.File,
              &fp->DstReg.Index, fp->DstReg.WriteMask))
            return 1;
         for (a = 0; a < 2; a++) {
            fp->SrcReg[a].Abs = GL_FALSE;
            fp->SrcReg[a].NegateAbs = GL_FALSE;
            if (parse_vector_src_reg
                (ctx, inst, vc_head, Program, (GLint *) & fp->SrcReg[a].File,
                 &fp->SrcReg[a].Index, &fp->SrcReg[a].NegateBase,
                 swz, &rel))
               return 1;
            for (b=0; b<4; b++)
               fp->SrcReg[a].Swizzle[b] = swz[b];
         }
         break;

      case OP_ALU_TRI:
         switch (code) {
            case OP_CMP_SAT:
               fp->Saturate = 1;
            case OP_CMP:
               fp->Opcode = FP_OPCODE_CMP;
               break;

            case OP_LRP_SAT:
               fp->Saturate = 1;
            case OP_LRP:
               fp->Opcode = FP_OPCODE_LRP;
               break;

            case OP_MAD_SAT:
               fp->Saturate = 1;
            case OP_MAD:
               fp->Opcode = FP_OPCODE_MAD;
               break;
         }

         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->DstReg.File,
              &fp->DstReg.Index, fp->DstReg.WriteMask))
            return 1;
         for (a = 0; a < 3; a++) {
            fp->SrcReg[a].Abs = GL_FALSE;
            fp->SrcReg[a].NegateAbs = GL_FALSE;
            if (parse_vector_src_reg
                (ctx, inst, vc_head, Program, (GLint *) & fp->SrcReg[a].File,
                 &fp->SrcReg[a].Index, &fp->SrcReg[a].NegateBase,
                 swz, &rel))
               return 1;
            for (b=0; b<4; b++)
               fp->SrcReg[a].Swizzle[b] = swz[b];
         }
         break;

      case OP_ALU_SWZ:
         switch (code) {
            case OP_SWZ_SAT:
               fp->Saturate = 1;
            case OP_SWZ:
               fp->Opcode = FP_OPCODE_SWZ;
               break;
         }
         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->DstReg.File,
              &fp->DstReg.Index, fp->DstReg.WriteMask))
            return 1;

         if (parse_src_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->SrcReg[0].File,
              &fp->SrcReg[0].Index, &rel))
            return 1;
         parse_extended_swizzle_mask (inst, swz,
                                      &fp->SrcReg[0].NegateBase);
         for (b=0; b<4; b++)
            fp->SrcReg[0].Swizzle[b] = swz[b];
         break;

      case OP_TEX_SAMPLE:
         switch (code) {
            case OP_TEX_SAT:
               fp->Saturate = 1;
            case OP_TEX:
               fp->Opcode = FP_OPCODE_TEX;
               break;

            case OP_TXP_SAT:
               fp->Saturate = 1;
            case OP_TXP:
               fp->Opcode = FP_OPCODE_TXP;
               break;

            case OP_TXB_SAT:
	      
               fp->Saturate = 1;
            case OP_TXB:
               fp->Opcode = FP_OPCODE_TXB;
               break;
         }

         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->DstReg.File,
              &fp->DstReg.Index, fp->DstReg.WriteMask))
            return 1;
         fp->SrcReg[0].Abs = GL_FALSE;
         fp->SrcReg[0].NegateAbs = GL_FALSE;
         if (parse_vector_src_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->SrcReg[0].File,
              &fp->SrcReg[0].Index, &fp->SrcReg[0].NegateBase,
              swz, &rel))
            return 1;
         for (b=0; b<4; b++)
            fp->SrcReg[0].Swizzle[b] = swz[b];

         /* texImageUnit */
         if (parse_texcoord_num (ctx, inst, Program, &texcoord))
            return 1;
         fp->TexSrcUnit = texcoord;

         /* texTarget */
         switch (*(*inst)++) {
            case TEXTARGET_1D:
               fp->TexSrcBit = TEXTURE_1D_BIT;
               break;
            case TEXTARGET_2D:
               fp->TexSrcBit = TEXTURE_2D_BIT;
               break;
            case TEXTARGET_3D:
               fp->TexSrcBit = TEXTURE_3D_BIT;
               break;
            case TEXTARGET_RECT:
               fp->TexSrcBit = TEXTURE_RECT_BIT;
               break;
            case TEXTARGET_CUBE:
               fp->TexSrcBit = TEXTURE_CUBE_BIT;
               break;
         }
         Program->TexturesUsed[texcoord] |= fp->TexSrcBit;			
         break;

      case OP_TEX_KIL:
         fp->Opcode = FP_OPCODE_KIL;
         fp->SrcReg[0].Abs = GL_FALSE;
         fp->SrcReg[0].NegateAbs = GL_FALSE;
         if (parse_vector_src_reg
             (ctx, inst, vc_head, Program, (GLint *) & fp->SrcReg[0].File,
              &fp->SrcReg[0].Index, &fp->SrcReg[0].NegateBase,
              swz, &rel))
            return 1;
         for (b=0; b<4; b++)
            fp->SrcReg[0].Swizzle[b] = swz[b];
         break;
   }

   return 0;
}

/**
 * This is a big mother that handles getting opcodes into the instruction
 * and handling the src & dst registers for vertex program instructions
 */
static GLuint
parse_vp_instruction (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      struct vp_instruction *vp)
{
   GLint a;
   GLubyte type, code;

   /* OP_ALU_{ARL, VECTOR, SCALAR, BINSC, BIN, TRI, SWZ} */
   type = *(*inst)++;

   /* The actual opcode name */
   code = *(*inst)++;

   /* Record the position in the program string for debugging */
   vp->StringPos = Program->Position;

   vp->SrcReg[0].RelAddr = vp->SrcReg[1].RelAddr = vp->SrcReg[2].RelAddr = 0;

   for (a = 0; a < 4; a++) {
      vp->SrcReg[0].Swizzle[a] = a;
      vp->SrcReg[1].Swizzle[a] = a;
      vp->SrcReg[2].Swizzle[a] = a;
      vp->DstReg.WriteMask[a] = 1;
   }

   switch (type) {
         /* XXX: */
      case OP_ALU_ARL:
         vp->Opcode = VP_OPCODE_ARL;

         /* Remember to set SrcReg.RelAddr; */

         /* Get the masked address register [dst] */
         if (parse_masked_address_reg
             (ctx, inst, vc_head, Program, &vp->DstReg.Index,
              vp->DstReg.WriteMask))
            return 1;
         vp->DstReg.File = PROGRAM_ADDRESS;

         /* Get a scalar src register */
         if (parse_scalar_src_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->SrcReg[0].File,
              &vp->SrcReg[0].Index, &vp->SrcReg[0].Negate,
              vp->SrcReg[0].Swizzle, &vp->SrcReg[0].RelAddr))
            return 1;

         break;

      case OP_ALU_VECTOR:
         switch (code) {
            case OP_ABS:
               vp->Opcode = VP_OPCODE_ABS;
               break;
            case OP_FLR:
               vp->Opcode = VP_OPCODE_FLR;
               break;
            case OP_FRC:
               vp->Opcode = VP_OPCODE_FRC;
               break;
            case OP_LIT:
               vp->Opcode = VP_OPCODE_LIT;
               break;
            case OP_MOV:
               vp->Opcode = VP_OPCODE_MOV;
               break;
         }
         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->DstReg.File,
              &vp->DstReg.Index, vp->DstReg.WriteMask))
            return 1;
         if (parse_vector_src_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->SrcReg[0].File,
              &vp->SrcReg[0].Index, &vp->SrcReg[0].Negate,
              vp->SrcReg[0].Swizzle, &vp->SrcReg[0].RelAddr))
            return 1;
         break;

      case OP_ALU_SCALAR:
         switch (code) {
            case OP_EX2:
               vp->Opcode = VP_OPCODE_EX2;
               break;
            case OP_EXP:
               vp->Opcode = VP_OPCODE_EXP;
               break;
            case OP_LG2:
               vp->Opcode = VP_OPCODE_LG2;
               break;
            case OP_LOG:
               vp->Opcode = VP_OPCODE_LOG;
               break;
            case OP_RCP:
               vp->Opcode = VP_OPCODE_RCP;
               break;
            case OP_RSQ:
               vp->Opcode = VP_OPCODE_RSQ;
               break;
         }
         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->DstReg.File,
              &vp->DstReg.Index, vp->DstReg.WriteMask))
            return 1;
         if (parse_scalar_src_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->SrcReg[0].File,
              &vp->SrcReg[0].Index, &vp->SrcReg[0].Negate,
              vp->SrcReg[0].Swizzle, &vp->SrcReg[0].RelAddr))
            return 1;
         break;

      case OP_ALU_BINSC:
         switch (code) {
            case OP_POW:
               vp->Opcode = VP_OPCODE_POW;
               break;
         }
         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->DstReg.File,
              &vp->DstReg.Index, vp->DstReg.WriteMask))
            return 1;
         for (a = 0; a < 2; a++) {
            if (parse_scalar_src_reg
                (ctx, inst, vc_head, Program, (GLint *) & vp->SrcReg[a].File,
                 &vp->SrcReg[a].Index, &vp->SrcReg[a].Negate,
                 vp->SrcReg[a].Swizzle, &vp->SrcReg[a].RelAddr))
               return 1;
         }
         break;

      case OP_ALU_BIN:
         switch (code) {
            case OP_ADD:
               vp->Opcode = VP_OPCODE_ADD;
               break;
            case OP_DP3:
               vp->Opcode = VP_OPCODE_DP3;
               break;
            case OP_DP4:
               vp->Opcode = VP_OPCODE_DP4;
               break;
            case OP_DPH:
               vp->Opcode = VP_OPCODE_DPH;
               break;
            case OP_DST:
               vp->Opcode = VP_OPCODE_DST;
               break;
            case OP_MAX:
               vp->Opcode = VP_OPCODE_MAX;
               break;
            case OP_MIN:
               vp->Opcode = VP_OPCODE_MIN;
               break;
            case OP_MUL:
               vp->Opcode = VP_OPCODE_MUL;
               break;
            case OP_SGE:
               vp->Opcode = VP_OPCODE_SGE;
               break;
            case OP_SLT:
               vp->Opcode = VP_OPCODE_SLT;
               break;
            case OP_SUB:
               vp->Opcode = VP_OPCODE_SUB;
               break;
            case OP_XPD:
               vp->Opcode = VP_OPCODE_XPD;
               break;
         }
         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->DstReg.File,
              &vp->DstReg.Index, vp->DstReg.WriteMask))
            return 1;
         for (a = 0; a < 2; a++) {
            if (parse_vector_src_reg
                (ctx, inst, vc_head, Program, (GLint *) & vp->SrcReg[a].File,
                 &vp->SrcReg[a].Index, &vp->SrcReg[a].Negate,
                 vp->SrcReg[a].Swizzle, &vp->SrcReg[a].RelAddr))
               return 1;
         }
         break;

      case OP_ALU_TRI:
         switch (code) {
            case OP_MAD:
               vp->Opcode = VP_OPCODE_MAD;
               break;
         }

         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->DstReg.File,
              &vp->DstReg.Index, vp->DstReg.WriteMask))
            return 1;
         for (a = 0; a < 3; a++) {
            if (parse_vector_src_reg
                (ctx, inst, vc_head, Program, (GLint *) & vp->SrcReg[a].File,
                 &vp->SrcReg[a].Index, &vp->SrcReg[a].Negate,
                 vp->SrcReg[a].Swizzle, &vp->SrcReg[a].RelAddr))
               return 1;
         }
         break;

      case OP_ALU_SWZ:
         switch (code) {
            case OP_SWZ:
               vp->Opcode = VP_OPCODE_SWZ;
               break;
         }
         if (parse_masked_dst_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->DstReg.File,
              &vp->DstReg.Index, vp->DstReg.WriteMask))
            return 1;

         if (parse_src_reg
             (ctx, inst, vc_head, Program, (GLint *) & vp->SrcReg[0].File,
              &vp->SrcReg[0].Index, &vp->SrcReg[0].RelAddr))
            return 1;
         parse_extended_swizzle_mask (inst, vp->SrcReg[0].Swizzle,
                                      &vp->SrcReg[0].Negate);
         break;
   }
   return 0;
}

#if DEBUG_PARSING

static GLvoid
print_state_token (GLint token)
{
   switch (token) {
      case STATE_MATERIAL:
         fprintf (stderr, "STATE_MATERIAL ");
         break;
      case STATE_LIGHT:
         fprintf (stderr, "STATE_LIGHT ");
         break;

      case STATE_LIGHTMODEL_AMBIENT:
         fprintf (stderr, "STATE_AMBIENT ");
         break;

      case STATE_LIGHTMODEL_SCENECOLOR:
         fprintf (stderr, "STATE_SCENECOLOR ");
         break;

      case STATE_LIGHTPROD:
         fprintf (stderr, "STATE_LIGHTPROD ");
         break;

      case STATE_TEXGEN:
         fprintf (stderr, "STATE_TEXGEN ");
         break;

      case STATE_FOG_COLOR:
         fprintf (stderr, "STATE_FOG_COLOR ");
         break;

      case STATE_FOG_PARAMS:
         fprintf (stderr, "STATE_FOG_PARAMS ");
         break;

      case STATE_CLIPPLANE:
         fprintf (stderr, "STATE_CLIPPLANE ");
         break;

      case STATE_POINT_SIZE:
         fprintf (stderr, "STATE_POINT_SIZE ");
         break;

      case STATE_POINT_ATTENUATION:
         fprintf (stderr, "STATE_ATTENUATION ");
         break;

      case STATE_MATRIX:
         fprintf (stderr, "STATE_MATRIX ");
         break;

      case STATE_MODELVIEW:
         fprintf (stderr, "STATE_MODELVIEW ");
         break;

      case STATE_PROJECTION:
         fprintf (stderr, "STATE_PROJECTION ");
         break;

      case STATE_MVP:
         fprintf (stderr, "STATE_MVP ");
         break;

      case STATE_TEXTURE:
         fprintf (stderr, "STATE_TEXTURE ");
         break;

      case STATE_PROGRAM:
         fprintf (stderr, "STATE_PROGRAM ");
         break;

      case STATE_MATRIX_INVERSE:
         fprintf (stderr, "STATE_INVERSE ");
         break;

      case STATE_MATRIX_TRANSPOSE:
         fprintf (stderr, "STATE_TRANSPOSE ");
         break;

      case STATE_MATRIX_INVTRANS:
         fprintf (stderr, "STATE_INVTRANS ");
         break;

      case STATE_AMBIENT:
         fprintf (stderr, "STATE_AMBIENT ");
         break;

      case STATE_DIFFUSE:
         fprintf (stderr, "STATE_DIFFUSE ");
         break;

      case STATE_SPECULAR:
         fprintf (stderr, "STATE_SPECULAR ");
         break;

      case STATE_EMISSION:
         fprintf (stderr, "STATE_EMISSION ");
         break;

      case STATE_SHININESS:
         fprintf (stderr, "STATE_SHININESS ");
         break;

      case STATE_HALF:
         fprintf (stderr, "STATE_HALF ");
         break;

      case STATE_POSITION:
         fprintf (stderr, "STATE_POSITION ");
         break;

      case STATE_ATTENUATION:
         fprintf (stderr, "STATE_ATTENUATION ");
         break;

      case STATE_SPOT_DIRECTION:
         fprintf (stderr, "STATE_DIRECTION ");
         break;

      case STATE_TEXGEN_EYE_S:
         fprintf (stderr, "STATE_TEXGEN_EYE_S ");
         break;

      case STATE_TEXGEN_EYE_T:
         fprintf (stderr, "STATE_TEXGEN_EYE_T ");
         break;

      case STATE_TEXGEN_EYE_R:
         fprintf (stderr, "STATE_TEXGEN_EYE_R ");
         break;

      case STATE_TEXGEN_EYE_Q:
         fprintf (stderr, "STATE_TEXGEN_EYE_Q ");
         break;

      case STATE_TEXGEN_OBJECT_S:
         fprintf (stderr, "STATE_TEXGEN_EYE_S ");
         break;

      case STATE_TEXGEN_OBJECT_T:
         fprintf (stderr, "STATE_TEXGEN_OBJECT_T ");
         break;

      case STATE_TEXGEN_OBJECT_R:
         fprintf (stderr, "STATE_TEXGEN_OBJECT_R ");
         break;

      case STATE_TEXGEN_OBJECT_Q:
         fprintf (stderr, "STATE_TEXGEN_OBJECT_Q ");
         break;

      case STATE_TEXENV_COLOR:
         fprintf (stderr, "STATE_TEXENV_COLOR ");
         break;

      case STATE_DEPTH_RANGE:
         fprintf (stderr, "STATE_DEPTH_RANGE ");
         break;

      case STATE_VERTEX_PROGRAM:
         fprintf (stderr, "STATE_VERTEX_PROGRAM ");
         break;

      case STATE_FRAGMENT_PROGRAM:
         fprintf (stderr, "STATE_FRAGMENT_PROGRAM ");
         break;

      case STATE_ENV:
         fprintf (stderr, "STATE_ENV ");
         break;

      case STATE_LOCAL:
         fprintf (stderr, "STATE_LOCAL ");
         break;

   }
   fprintf (stderr, "[%d] ", token);
}


static GLvoid
debug_variables (GLcontext * ctx, struct var_cache *vc_head,
                 struct arb_program *Program)
{
   struct var_cache *vc;
   GLint a, b;

   fprintf (stderr, "debug_variables, vc_head: %x\n", vc_head);

   /* First of all, print out the contents of the var_cache */
   vc = vc_head;
   while (vc) {
      fprintf (stderr, "[%x]\n", vc);
      switch (vc->type) {
         case vt_none:
            fprintf (stderr, "UNDEFINED %s\n", vc->name);
            break;
         case vt_attrib:
            fprintf (stderr, "ATTRIB    %s\n", vc->name);
            fprintf (stderr, "          binding: 0x%x\n", vc->attrib_binding);
            break;
         case vt_param:
            fprintf (stderr, "PARAM     %s  begin: %d len: %d\n", vc->name,
                     vc->param_binding_begin, vc->param_binding_length);
            b = vc->param_binding_begin;
            for (a = 0; a < vc->param_binding_length; a++) {
               fprintf (stderr, "%s\n",
                        Program->Parameters->Parameters[a + b].Name);
               if (Program->Parameters->Parameters[a + b].Type == STATE) {
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[0]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[1]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[2]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[3]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[4]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[5]);
               }
               else
                  fprintf (stderr, "%f %f %f %f\n",
                           Program->Parameters->Parameters[a + b].Values[0],
                           Program->Parameters->Parameters[a + b].Values[1],
                           Program->Parameters->Parameters[a + b].Values[2],
                           Program->Parameters->Parameters[a + b].Values[3]);
            }
            break;
         case vt_temp:
            fprintf (stderr, "TEMP      %s\n", vc->name);
            fprintf (stderr, "          binding: 0x%x\n", vc->temp_binding);
            break;
         case vt_output:
            fprintf (stderr, "OUTPUT    %s\n", vc->name);
            fprintf (stderr, "          binding: 0x%x\n", vc->output_binding);
            break;
         case vt_alias:
            fprintf (stderr, "ALIAS     %s\n", vc->name);
            fprintf (stderr, "          binding: 0x%x (%s)\n",
                     vc->alias_binding, vc->alias_binding->name);
            break;
      }
      vc = vc->next;
   }
}

#endif


/** 
 * The main loop for parsing a fragment or vertex program
 * 
 * \return 0 on sucess, 1 on error
 */
static GLint
parse_arb_program (GLcontext * ctx, GLubyte * inst, struct var_cache **vc_head,
                   struct arb_program *Program)
{
   GLint err = 0;

   Program->MajorVersion = (GLuint) * inst++;
   Program->MinorVersion = (GLuint) * inst++;

   while (*inst != END) {
      switch (*inst++) {
            
         case OPTION:
            switch (*inst++) {
               case ARB_PRECISION_HINT_FASTEST:
                  Program->HintPrecisionFastest = 1;
                  break;

               case ARB_PRECISION_HINT_NICEST:
                  Program->HintPrecisionNicest = 1;
                  break;

               case ARB_FOG_EXP:
                  Program->HintFogExp = 1;
                  break;

               case ARB_FOG_EXP2:
                  Program->HintFogExp2 = 1;
                  break;

               case ARB_FOG_LINEAR:
                  Program->HintFogLinear = 1;
                  break;

               case ARB_POSITION_INVARIANT:
                  if (Program->type == GL_VERTEX_PROGRAM_ARB)						
                     Program->HintPositionInvariant = 1;
                  break;
            }
            break;

         case INSTRUCTION:
            Program->Position = parse_position (&inst);

            if (Program->type == GL_FRAGMENT_PROGRAM_ARB) {

               /* Check the instruction count 
                * XXX: Does END count as an instruction?
                */						  
               if (Program->Base.NumInstructions+1 == MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS) {
                  _mesa_set_program_error (ctx, Program->Position, 
                      "Max instruction count exceeded!");
                  _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Max instruction count exceeded!");
               }
					
               /* Realloc Program->FPInstructions */
               Program->FPInstructions =
                  (struct fp_instruction *) _mesa_realloc (Program->FPInstructions,
                                                           Program->Base.NumInstructions*sizeof(struct fp_instruction),
                                                           (Program->Base.NumInstructions+1)*sizeof (struct fp_instruction));

               /* parse the current instruction   */
               err = parse_fp_instruction (ctx, &inst, vc_head, Program,
                                           &Program->FPInstructions[Program->Base.NumInstructions]);
                                                                    
            }
            else {
               /* Check the instruction count 
                * XXX: Does END count as an instruction?
                */						  
               if (Program->Base.NumInstructions+1 == MAX_NV_VERTEX_PROGRAM_INSTRUCTIONS) {
                  _mesa_set_program_error (ctx, Program->Position, 
                      "Max instruction count exceeded!");
                  _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Max instruction count exceeded!");
               }

               /* Realloc Program->VPInstructions */
               Program->VPInstructions =
                  (struct vp_instruction *) _mesa_realloc (Program->VPInstructions,
                                                           Program->Base.NumInstructions*sizeof(struct vp_instruction),
                                                           (Program->Base.NumInstructions +1)*sizeof(struct vp_instruction));

               /* parse the current instruction   */
               err = parse_vp_instruction (ctx, &inst, vc_head, Program,
                                           &Program->VPInstructions[Program->Base.NumInstructions]);
            }

            /* increment Program->Base.NumInstructions */
            Program->Base.NumInstructions++;
            break;

         case DECLARATION:
            err = parse_declaration (ctx, &inst, vc_head, Program);
            break;

         default:
            break;
      }

      if (err)
         break;
   }

   /* Finally, tag on an OPCODE_END instruction */
   if (Program->type == GL_FRAGMENT_PROGRAM_ARB) {
      Program->FPInstructions =
         (struct fp_instruction *) _mesa_realloc (Program->FPInstructions,
						  Program->Base.NumInstructions*sizeof(struct fp_instruction),
                                                  (Program->Base.NumInstructions+1)*sizeof(struct fp_instruction));

      Program->FPInstructions[Program->Base.NumInstructions].Opcode = FP_OPCODE_END;
      /* YYY Wrong Position in program, whatever, at least not random -> crash
	 Program->Position = parse_position (&inst);
      */
      Program->FPInstructions[Program->Base.NumInstructions].StringPos = Program->Position;
   }
   else {
      Program->VPInstructions =
         (struct vp_instruction *) _mesa_realloc (Program->VPInstructions,
                                                  Program->Base.NumInstructions*sizeof(struct vp_instruction),
                                                  (Program->Base.NumInstructions+1)*sizeof(struct vp_instruction));

      Program->VPInstructions[Program->Base.NumInstructions].Opcode = VP_OPCODE_END;
      /* YYY Wrong Position in program, whatever, at least not random -> crash
	 Program->Position = parse_position (&inst);
      */
      Program->VPInstructions[Program->Base.NumInstructions].StringPos = Program->Position;
   }

   /* increment Program->Base.NumInstructions */
   Program->Base.NumInstructions++;

   return err;
}


/** 
 * This kicks everything off.
 *
 * \param ctx - The GL Context
 * \param str - The program string
 * \param len - The program string length
 * \param Program - The arb_program struct to return all the parsed info in
 * \return 0 on sucess, 1 on error
 */
GLuint
_mesa_parse_arb_program (GLcontext * ctx, const GLubyte * str, GLsizei len,
                         struct arb_program * program)
{
   GLint a, err, error_pos;
   char error_msg[300];
   GLuint parsed_len;
   struct var_cache *vc_head;
   dict *dt;
   GLubyte *parsed, *inst;
   GLubyte *strCopy;

   /* init to zero in case of parse error */
   _mesa_bzero(program, sizeof(*program));

   /* Need a null-terminated string for parsing */
   strCopy = (GLubyte *) _mesa_malloc(len + 1);
   if (!strCopy) {
      _mesa_error (ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      return 1;
   }
   _mesa_memcpy(strCopy, str, len);
   strCopy[len] = 0;
   str = strCopy;


#if DEBUG_PARSING
   fprintf (stderr, "Loading grammar text!\n");
#endif
   dt = grammar_load_from_text ((GLubyte *) arb_grammar_text);
   if (!dt) {
      grammar_get_last_error ((GLubyte *) error_msg, 300, &error_pos);
      _mesa_set_program_error (ctx, error_pos, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION,
                   "Error loading grammer rule set");
      _mesa_free(strCopy);
      return 1;
   }

#if DEBUG_PARSING
   printf ("Checking Grammar!\n");
#endif
   err = grammar_check (dt, str, &parsed, &parsed_len);


   /* Syntax parse error */
   if (err == 0) {
      grammar_get_last_error ((GLubyte *) error_msg, 300, &error_pos);
      _mesa_set_program_error (ctx, error_pos, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, "Parse Error");

      dict_destroy (&dt);
      _mesa_free(strCopy);
      return 1;
   }

#if DEBUG_PARSING
   printf ("Destroying grammer dict [parse retval: %d]\n", err);
#endif
   dict_destroy (&dt);

   /* Initialize the arb_program struct */
   program->Base.String = strCopy;
   program->Base.NumInstructions =
   program->Base.NumTemporaries =
   program->Base.NumParameters =
   program->Base.NumAttributes = program->Base.NumAddressRegs = 0;
   program->Parameters = _mesa_new_parameter_list ();
   program->InputsRead = 0;
   program->OutputsWritten = 0;
   program->Position = 0;
   program->MajorVersion = program->MinorVersion = 0;
   program->HintPrecisionFastest =
   program->HintPrecisionNicest =
   program->HintFogExp2 =
   program->HintFogExp =
   program->HintFogLinear = program->HintPositionInvariant = 0;
   for (a = 0; a < MAX_TEXTURE_IMAGE_UNITS; a++)
      program->TexturesUsed[a] = 0;
   program->NumAluInstructions =
   program->NumTexInstructions = 
   program->NumTexIndirections = 0;

   program->FPInstructions = NULL;
   program->VPInstructions = NULL;

   vc_head = NULL;
   err = 0;

   /* Start examining the tokens in the array */
   inst = parsed;

   /* Check the grammer rev */
   if (*inst++ != REVISION) {
      _mesa_set_program_error (ctx, 0, "Grammar version mismatch");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Grammar verison mismatch");
      err = 1;
   }
   else {
      switch (*inst++) {
         case FRAGMENT_PROGRAM:
            program->type = GL_FRAGMENT_PROGRAM_ARB;
            break;

         case VERTEX_PROGRAM:
            program->type = GL_VERTEX_PROGRAM_ARB;
            break;
      }

      err = parse_arb_program (ctx, inst, &vc_head, program);
#if DEBUG_PARSING
      fprintf (stderr, "Symantic analysis returns %d [1 is bad!]\n", err);
#endif
   }

   /*debug_variables(ctx, vc_head, program); */

   /* We're done with the parsed binary array */
   var_cache_destroy (&vc_head);

   _mesa_free (parsed);
#if DEBUG_PARSING
   printf ("_mesa_parse_arb_program() done\n");
#endif

   return err;
}
