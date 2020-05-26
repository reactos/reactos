/* A Bison parser, made by GNU Bison 3.4.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_ASMSHADER_E_REACTOSSYNC_GCC_DLL_DIRECTX_WINE_D3DCOMPILER_43_ASMSHADER_TAB_H_INCLUDED
# define YY_ASMSHADER_E_REACTOSSYNC_GCC_DLL_DIRECTX_WINE_D3DCOMPILER_43_ASMSHADER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int asmshader_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INSTR_ADD = 258,
    INSTR_NOP = 259,
    INSTR_MOV = 260,
    INSTR_SUB = 261,
    INSTR_MAD = 262,
    INSTR_MUL = 263,
    INSTR_RCP = 264,
    INSTR_RSQ = 265,
    INSTR_DP3 = 266,
    INSTR_DP4 = 267,
    INSTR_MIN = 268,
    INSTR_MAX = 269,
    INSTR_SLT = 270,
    INSTR_SGE = 271,
    INSTR_ABS = 272,
    INSTR_EXP = 273,
    INSTR_LOG = 274,
    INSTR_EXPP = 275,
    INSTR_LOGP = 276,
    INSTR_DST = 277,
    INSTR_LRP = 278,
    INSTR_FRC = 279,
    INSTR_POW = 280,
    INSTR_CRS = 281,
    INSTR_SGN = 282,
    INSTR_NRM = 283,
    INSTR_SINCOS = 284,
    INSTR_M4x4 = 285,
    INSTR_M4x3 = 286,
    INSTR_M3x4 = 287,
    INSTR_M3x3 = 288,
    INSTR_M3x2 = 289,
    INSTR_DCL = 290,
    INSTR_DEF = 291,
    INSTR_DEFB = 292,
    INSTR_DEFI = 293,
    INSTR_REP = 294,
    INSTR_ENDREP = 295,
    INSTR_IF = 296,
    INSTR_ELSE = 297,
    INSTR_ENDIF = 298,
    INSTR_BREAK = 299,
    INSTR_BREAKP = 300,
    INSTR_CALL = 301,
    INSTR_CALLNZ = 302,
    INSTR_LOOP = 303,
    INSTR_RET = 304,
    INSTR_ENDLOOP = 305,
    INSTR_LABEL = 306,
    INSTR_SETP = 307,
    INSTR_TEXLDL = 308,
    INSTR_LIT = 309,
    INSTR_MOVA = 310,
    INSTR_CND = 311,
    INSTR_CMP = 312,
    INSTR_DP2ADD = 313,
    INSTR_TEXCOORD = 314,
    INSTR_TEXCRD = 315,
    INSTR_TEXKILL = 316,
    INSTR_TEX = 317,
    INSTR_TEXLD = 318,
    INSTR_TEXBEM = 319,
    INSTR_TEXBEML = 320,
    INSTR_TEXREG2AR = 321,
    INSTR_TEXREG2GB = 322,
    INSTR_TEXREG2RGB = 323,
    INSTR_TEXM3x2PAD = 324,
    INSTR_TEXM3x2TEX = 325,
    INSTR_TEXM3x3PAD = 326,
    INSTR_TEXM3x3SPEC = 327,
    INSTR_TEXM3x3VSPEC = 328,
    INSTR_TEXM3x3TEX = 329,
    INSTR_TEXDP3TEX = 330,
    INSTR_TEXM3x2DEPTH = 331,
    INSTR_TEXDP3 = 332,
    INSTR_TEXM3x3 = 333,
    INSTR_TEXDEPTH = 334,
    INSTR_BEM = 335,
    INSTR_DSX = 336,
    INSTR_DSY = 337,
    INSTR_TEXLDP = 338,
    INSTR_TEXLDB = 339,
    INSTR_TEXLDD = 340,
    INSTR_PHASE = 341,
    REG_TEMP = 342,
    REG_OUTPUT = 343,
    REG_INPUT = 344,
    REG_CONSTFLOAT = 345,
    REG_CONSTINT = 346,
    REG_CONSTBOOL = 347,
    REG_TEXTURE = 348,
    REG_SAMPLER = 349,
    REG_TEXCRDOUT = 350,
    REG_OPOS = 351,
    REG_OFOG = 352,
    REG_OPTS = 353,
    REG_VERTEXCOLOR = 354,
    REG_FRAGCOLOR = 355,
    REG_FRAGDEPTH = 356,
    REG_VPOS = 357,
    REG_VFACE = 358,
    REG_ADDRESS = 359,
    REG_LOOP = 360,
    REG_PREDICATE = 361,
    REG_LABEL = 362,
    VER_VS10 = 363,
    VER_VS11 = 364,
    VER_VS20 = 365,
    VER_VS2X = 366,
    VER_VS30 = 367,
    VER_PS10 = 368,
    VER_PS11 = 369,
    VER_PS12 = 370,
    VER_PS13 = 371,
    VER_PS14 = 372,
    VER_PS20 = 373,
    VER_PS2X = 374,
    VER_PS30 = 375,
    SHIFT_X2 = 376,
    SHIFT_X4 = 377,
    SHIFT_X8 = 378,
    SHIFT_D2 = 379,
    SHIFT_D4 = 380,
    SHIFT_D8 = 381,
    MOD_SAT = 382,
    MOD_PP = 383,
    MOD_CENTROID = 384,
    COMP_GT = 385,
    COMP_LT = 386,
    COMP_GE = 387,
    COMP_LE = 388,
    COMP_EQ = 389,
    COMP_NE = 390,
    SMOD_BIAS = 391,
    SMOD_SCALEBIAS = 392,
    SMOD_DZ = 393,
    SMOD_DW = 394,
    SMOD_ABS = 395,
    SMOD_NOT = 396,
    SAMPTYPE_1D = 397,
    SAMPTYPE_2D = 398,
    SAMPTYPE_CUBE = 399,
    SAMPTYPE_VOLUME = 400,
    USAGE_POSITION = 401,
    USAGE_BLENDWEIGHT = 402,
    USAGE_BLENDINDICES = 403,
    USAGE_NORMAL = 404,
    USAGE_PSIZE = 405,
    USAGE_TEXCOORD = 406,
    USAGE_TANGENT = 407,
    USAGE_BINORMAL = 408,
    USAGE_TESSFACTOR = 409,
    USAGE_POSITIONT = 410,
    USAGE_COLOR = 411,
    USAGE_FOG = 412,
    USAGE_DEPTH = 413,
    USAGE_SAMPLE = 414,
    COMPONENT = 415,
    IMMVAL = 416,
    IMMBOOL = 417
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 68 "asmshader.y"

    struct {
        float           val;
        BOOL            integer;
    } immval;
    BOOL                immbool;
    unsigned int        regnum;
    struct shader_reg   reg;
    DWORD               srcmod;
    DWORD               writemask;
    struct {
        DWORD           writemask;
        DWORD           idx;
        DWORD           last;
    } wm_components;
    DWORD               swizzle;
    struct {
        DWORD           swizzle;
        DWORD           idx;
    } sw_components;
    DWORD               component;
    struct {
        DWORD           mod;
        DWORD           shift;
    } modshift;
    enum bwriter_comparison_type comptype;
    struct {
        DWORD           dclusage;
        unsigned int    regnum;
    } declaration;
    enum bwritersampler_texture_type samplertype;
    struct rel_reg      rel_reg;
    struct src_regs     sregs;

#line 255 "asmshader.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE asmshader_lval;

int asmshader_parse (void);

#endif /* !YY_ASMSHADER_E_REACTOSSYNC_GCC_DLL_DIRECTX_WINE_D3DCOMPILER_43_ASMSHADER_TAB_H_INCLUDED  */
