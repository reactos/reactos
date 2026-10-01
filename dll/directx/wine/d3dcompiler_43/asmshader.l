/*
 * Direct3D shader assembler
 *
 * Copyright 2008 Stefan Dösinger
 * Copyright 2009 Matteo Bruni
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

%{
#include "wine/debug.h"

#include "d3dcompiler_private.h"
#include "asmshader.tab.h"

WINE_DEFAULT_DEBUG_CHANNEL(asmshader);
%}

%option noyywrap
%option prefix="asmshader_"
%option noinput nounput never-interactive

/* Swizzles and writemasks consist of a dot and up to 4 x, y, z or w characters,
 * or up to 4 a, r, g, b characters. There are different rules for swizzles and
 * writemasks wrt repetition, those are handled in the grammar.
 */
DOT                     \.
COMPONENT               [xyzw]|[rgba]

/* Registers */
REG_TEMP                r[0-9]+
/* for relative addressing in the form o[x], v[x] and c[x] */
REG_OUTPUT              o[0-9]*
REG_INPUT               v[0-9]*
REG_CONSTFLOAT          c[0-9]*
REG_CONSTINT            i[0-9]+
REG_CONSTBOOL           b[0-9]+
REG_TEXTURE             t[0-9]+
REG_TEXCRDOUT           oT[0-9]+
REG_SAMPLER             s[0-9]+
REG_OPOS                oPos
REG_OFOG                oFog
REG_OPTS                oPts
REG_VERTEXCOLOR         oD[01]
REG_FRAGCOLOR           oC[0-9]+
REG_FRAGDEPTH           oDepth
REG_VPOS                vPos
REG_VFACE               vFace
REG_ADDRESS             a0
REG_LOOP                aL
REG_PREDICATE           p0
/* Not really a register, but it is considered as such */
REG_LABEL               l[0-9]+

DCL_POSITION            _position[0-9]*
DCL_BLENDWEIGHT         _blendweight[0-9]*
DCL_BLENDINDICES        _blendindices[0-9]*
DCL_NORMAL              _normal[0-9]*
DCL_PSIZE               _psize[0-9]*
DCL_TEXCOORD            _texcoord[0-9]*
DCL_TANGENT             _tangent[0-9]*
DCL_BINORMAL            _binormal[0-9]*
DCL_TESSFACTOR          _tessfactor[0-9]*
DCL_POSITIONT           _positiont[0-9]*
DCL_COLOR               _color[0-9]*
DCL_FOG                 _fog[0-9]*
DCL_DEPTH               _depth[0-9]*
DCL_SAMPLE              _sample[0-9]*

DCL_SAMPLER1D           _1d
DCL_SAMPLER2D           _2d
DCL_SAMPLERCUBE         _cube
DCL_SAMPLERVOLUME       _volume

PREPROCESSORDIRECTIVE   #[^\n]*\n

/* Comments */
DOUBLESLASHCOMMENT      "//"[^\n]*
SEMICOLONCOMMENT        ";"[^\n]*

/* Whitespaces are spaces, tabs and newlines */
WHITESPACE              [ \t]+
NEWLINE                 (\n)|(\r\n)

COMMA                   ","

IMMVAL                  (([0-9]+\.?)|([0-9]*\.[0-9]+))(f)?

ANY                     (.)

%%

    /* Common instructions(vertex and pixel shaders) */
add                     {return INSTR_ADD;          }
nop                     {return INSTR_NOP;          }
mov                     {return INSTR_MOV;          }
sub                     {return INSTR_SUB;          }
mad                     {return INSTR_MAD;          }
mul                     {return INSTR_MUL;          }
rcp                     {return INSTR_RCP;          }
rsq                     {return INSTR_RSQ;          }
dp3                     {return INSTR_DP3;          }
dp4                     {return INSTR_DP4;          }
min                     {return INSTR_MIN;          }
max                     {return INSTR_MAX;          }
slt                     {return INSTR_SLT;          }
sge                     {return INSTR_SGE;          }
abs                     {return INSTR_ABS;          }
exp                     {return INSTR_EXP;          }
log                     {return INSTR_LOG;          }
expp                    {return INSTR_EXPP;         }
logp                    {return INSTR_LOGP;         }
dst                     {return INSTR_DST;          }
lrp                     {return INSTR_LRP;          }
frc                     {return INSTR_FRC;          }
pow                     {return INSTR_POW;          }
crs                     {return INSTR_CRS;          }
sgn                     {return INSTR_SGN;          }
nrm                     {return INSTR_NRM;          }
sincos                  {return INSTR_SINCOS;       }
m4x4                    {return INSTR_M4x4;         }
m4x3                    {return INSTR_M4x3;         }
m3x4                    {return INSTR_M3x4;         }
m3x3                    {return INSTR_M3x3;         }
m3x2                    {return INSTR_M3x2;         }
dcl                     {return INSTR_DCL;          }
def                     {return INSTR_DEF;          }
defb                    {return INSTR_DEFB;         }
defi                    {return INSTR_DEFI;         }
rep                     {return INSTR_REP;          }
endrep                  {return INSTR_ENDREP;       }
if                      {return INSTR_IF;           }
else                    {return INSTR_ELSE;         }
endif                   {return INSTR_ENDIF;        }
break                   {return INSTR_BREAK;        }
breakp                  {return INSTR_BREAKP;       }
call                    {return INSTR_CALL;         }
callnz                  {return INSTR_CALLNZ;       }
loop                    {return INSTR_LOOP;         }
ret                     {return INSTR_RET;          }
endloop                 {return INSTR_ENDLOOP;      }
label                   {return INSTR_LABEL;        }
setp                    {return INSTR_SETP;         }
texldl                  {return INSTR_TEXLDL;       }

    /* Vertex shader only instructions  */
lit                     {return INSTR_LIT;          }
mova                    {return INSTR_MOVA;         }

    /* Pixel shader only instructions   */
cnd                     {return INSTR_CND;          }
cmp                     {return INSTR_CMP;          }
dp2add                  {return INSTR_DP2ADD;       }
texcoord                {return INSTR_TEXCOORD;     }
texcrd                  {return INSTR_TEXCRD;       }
texkill                 {return INSTR_TEXKILL;      }
tex                     {return INSTR_TEX;          }
texld                   {return INSTR_TEXLD;        }
texbem                  {return INSTR_TEXBEM;       }
texbeml                 {return INSTR_TEXBEML;      }
texreg2ar               {return INSTR_TEXREG2AR;    }
texreg2gb               {return INSTR_TEXREG2GB;    }
texreg2rgb              {return INSTR_TEXREG2RGB;   }
texm3x2pad              {return INSTR_TEXM3x2PAD;   }
texm3x2tex              {return INSTR_TEXM3x2TEX;   }
texm3x3pad              {return INSTR_TEXM3x3PAD;   }
texm3x3spec             {return INSTR_TEXM3x3SPEC;  }
texm3x3vspec            {return INSTR_TEXM3x3VSPEC; }
texm3x3tex              {return INSTR_TEXM3x3TEX;   }
texdp3tex               {return INSTR_TEXDP3TEX;    }
texm3x2depth            {return INSTR_TEXM3x2DEPTH; }
texdp3                  {return INSTR_TEXDP3;       }
texm3x3                 {return INSTR_TEXM3x3;      }
texdepth                {return INSTR_TEXDEPTH;     }
bem                     {return INSTR_BEM;          }
dsx                     {return INSTR_DSX;          }
dsy                     {return INSTR_DSY;          }
texldp                  {return INSTR_TEXLDP;       }
texldb                  {return INSTR_TEXLDB;       }
texldd                  {return INSTR_TEXLDD;       }
phase                   {return INSTR_PHASE;        }

{REG_TEMP}              {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_TEMP;
                        }
{REG_OUTPUT}            {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_OUTPUT;
                        }
{REG_INPUT}             {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_INPUT;
                        }
{REG_CONSTFLOAT}        {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_CONSTFLOAT;
                        }
{REG_CONSTINT}          {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_CONSTINT;
                        }
{REG_CONSTBOOL}         {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_CONSTBOOL;
                        }
{REG_TEXTURE}           {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_TEXTURE;
                        }
{REG_TEXCRDOUT}         {
                            asmshader_lval.regnum = atoi(yytext + 2);
                            return REG_TEXCRDOUT;
                        }
{REG_SAMPLER}           {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_SAMPLER;
                        }
{REG_OPOS}              {return REG_OPOS;           }
{REG_OFOG}              {return REG_OFOG;           }
{REG_OPTS}              {return REG_OPTS;           }
{REG_VERTEXCOLOR}       {
                            asmshader_lval.regnum = atoi(yytext + 2);
                            return REG_VERTEXCOLOR;
                        }
{REG_FRAGCOLOR}         {
                            asmshader_lval.regnum = atoi(yytext + 2);
                            return REG_FRAGCOLOR;
                        }
{REG_FRAGDEPTH}         {return REG_FRAGDEPTH;      }
{REG_VPOS}              {return REG_VPOS;           }
{REG_VFACE}             {return REG_VFACE;          }
{REG_ADDRESS}           {return REG_ADDRESS;        }
{REG_LOOP}              {return REG_LOOP;           }
{REG_PREDICATE}         {return REG_PREDICATE;      }

{REG_LABEL}             {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_LABEL;
                        }

    /* Shader versions. These are important to select the correct
     * parser profile.
     */
vs\.1\.0|vs_1_0         {return VER_VS10;       }
vs\.1\.1|vs_1_1         {return VER_VS11;       }

vs\.2\.0|vs_2_0         {return VER_VS20;       }
vs\.2\.x|vs_2_x         {return VER_VS2X;       }
vs\.3\.0|vs_3_0         {return VER_VS30;       }

ps\.1\.0|ps_1_0         {return VER_PS10;       }
ps\.1\.1|ps_1_1         {return VER_PS11;       }
ps\.1\.2|ps_1_2         {return VER_PS12;       }
ps\.1\.3|ps_1_3         {return VER_PS13;       }
ps\.1\.4|ps_1_4         {return VER_PS14;       }

ps\.2\.0|ps_2_0         {return VER_PS20;       }
ps\.2\.x|ps_2_x         {return VER_PS2X;       }
ps\.3\.0|ps_3_0         {return VER_PS30;       }

{DOT}                   {return yytext[0];      }
{COMPONENT}             {
                            switch(yytext[0]) {
                                case 'x':
                                case 'r':
                                    asmshader_lval.component = 0;
                                    break;
                                case 'y':
                                case 'g':
                                    asmshader_lval.component = 1;
                                    break;
                                case 'z':
                                case 'b':
                                    asmshader_lval.component = 2;
                                    break;
                                case 'w':
                                case 'a':
                                    asmshader_lval.component = 3;
                                    break;
                            }
                            return COMPONENT;
                        }

    /* Output modifiers */
\_x2                    {return SHIFT_X2;           }
\_x4                    {return SHIFT_X4;           }
\_x8                    {return SHIFT_X8;           }
\_d2                    {return SHIFT_D2;           }
\_d4                    {return SHIFT_D4;           }
\_d8                    {return SHIFT_D8;           }
\_sat                   {return MOD_SAT;            }
\_pp                    {return MOD_PP;             }
\_centroid              {return MOD_CENTROID;       }

    /* compare params */
\_gt                    {return COMP_GT;            }
\_lt                    {return COMP_LT;            }
\_ge                    {return COMP_GE;            }
\_le                    {return COMP_LE;            }
\_eq                    {return COMP_EQ;            }
\_ne                    {return COMP_NE;            }

{IMMVAL}                {
                            asmshader_lval.immval.val = atof(yytext);
                            asmshader_lval.immval.integer = ((strstr(yytext, ".") == NULL) && (strstr(yytext, "f") == NULL));
                            return IMMVAL;
                        }
true                    {
                            asmshader_lval.immbool = TRUE;
                            return IMMBOOL;
                        }
false                   {
                            asmshader_lval.immbool = FALSE;
                            return IMMBOOL;
                        }

{COMMA}                 {return yytext[0];          }
-                       {return yytext[0];          }
\(                      {return yytext[0];          }
\)                      {return yytext[0];          }

    /* for relative addressing */
\[|\]|\+                {return yytext[0];          }

\_bias                  {return SMOD_BIAS;          }
    /* No _x2 here; it is identical to MOD_X2 */
\_bx2                   {return SMOD_SCALEBIAS;     }
\_dz                    {return SMOD_DZ;            }
\_dw                    {return SMOD_DW;            }
\_abs                   {return SMOD_ABS;           }

!                       {return SMOD_NOT;           }

{DCL_POSITION}          {
                            if(yytext[strlen("_position")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_position"));
                            }
                            return USAGE_POSITION;
                        }
{DCL_BLENDWEIGHT}       {
                            if(yytext[strlen("_blendweight")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_blendweight"));
                            }
                            return USAGE_BLENDWEIGHT;
                        }
{DCL_BLENDINDICES}      {
                            if(yytext[strlen("_blendindices")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_blendindices"));
                            }
                            return USAGE_BLENDINDICES;
                        }
{DCL_NORMAL}            {
                            if(yytext[strlen("_normal")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_normal"));
                            }
                            return USAGE_NORMAL;
                        }
{DCL_PSIZE}             {
                            if(yytext[strlen("_psize")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_psize"));
                            }
                            return USAGE_PSIZE;
                        }
{DCL_TEXCOORD}          {
                            if(yytext[strlen("_texcoord")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_texcoord"));
                            }
                            return USAGE_TEXCOORD;
                        }
{DCL_TANGENT}           {
                            if(yytext[strlen("_tangent")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_tangent"));
                            }
                            return USAGE_TANGENT;
                        }
{DCL_BINORMAL}          {
                            if(yytext[strlen("_binormal")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_binormal"));
                            }
                            return USAGE_BINORMAL;
                        }
{DCL_TESSFACTOR}        {
                            if(yytext[strlen("_tessfactor")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_tessfactor"));
                            }
                            return USAGE_TESSFACTOR;
                        }
{DCL_POSITIONT}         {
                            if(yytext[strlen("_positiont")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_positiont"));
                            }
                            return USAGE_POSITIONT;
                        }
{DCL_COLOR}             {
                            if(yytext[strlen("_color")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_color"));
                            }
                            return USAGE_COLOR;
                        }
{DCL_FOG}               {
                            if(yytext[strlen("_fog")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_fog"));
                            }
                            return USAGE_FOG;
                        }
{DCL_DEPTH}             {
                            if(yytext[strlen("_depth")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_depth"));
                            }
                            return USAGE_DEPTH;
                        }
{DCL_SAMPLE}            {
                            if(yytext[strlen("_sample")] == '\0') {
                                asmshader_lval.regnum = 0;
                            } else {
                                asmshader_lval.regnum = atoi(yytext + strlen("_sample"));
                            }
                            return USAGE_SAMPLE;
                        }

{DCL_SAMPLER1D}         { return SAMPTYPE_1D;       }
{DCL_SAMPLER2D}         { return SAMPTYPE_2D;       }
{DCL_SAMPLERCUBE}       { return SAMPTYPE_CUBE;     }
{DCL_SAMPLERVOLUME}     { return SAMPTYPE_VOLUME;   }

{PREPROCESSORDIRECTIVE} {
                            /* TODO: update current line information */
                            TRACE("line info update: %s", yytext);
                        }

    /* Skip comments */
{DOUBLESLASHCOMMENT}    {                           }
{SEMICOLONCOMMENT}      {                           }

{WHITESPACE}            { /* Do nothing */          }
{NEWLINE}               {
                            asm_ctx.line_no++;
                        }

{ANY}                   {
                            asmparser_message(&asm_ctx, "Line %u: Unexpected input %s\n", asm_ctx.line_no, yytext);
                            set_parse_status(&asm_ctx.status, PARSE_ERR);
                        }

%%

struct bwriter_shader *SlAssembleShader(const char *text, char **messages) {
    struct bwriter_shader *ret = NULL;
    YY_BUFFER_STATE buffer;
    TRACE("%p, %p\n", text, messages);

    buffer = asmshader__scan_string(text);
    asmshader__switch_to_buffer(buffer);

    ret = parse_asm_shader(messages);

    asmshader__delete_buffer(buffer);

    return ret;
}
