/*
 * HLSL parser
 *
 * Copyright 2008 Stefan Dösinger
 * Copyright 2012 Matteo Bruni for CodeWeavers
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

#define YY_NO_UNISTD_H
#include "d3dcompiler_private.h"
#include "hlsl.tab.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlsl_parser);

#define YY_USER_ACTION                               \
   do {                                              \
      hlsl_lloc.first_column = hlsl_ctx.column;      \
      hlsl_lloc.first_line = hlsl_ctx.line_no;       \
      hlsl_ctx.column += yyleng;                     \
   } while(0);

%}

%option noyywrap nounput noinput never-interactive
%option prefix="hlsl_"

%x pp pp_line pp_pragma pp_ignore

RESERVED1               auto|case|catch|char|class|const_cast|default|delete|dynamic_cast|enum
RESERVED2               explicit|friend|goto|long|mutable|new|operator|private|protected|public
RESERVED3               reinterpret_cast|short|signed|sizeof|static_cast|template|this|throw|try
RESERVED4               typename|union|unsigned|using|virtual

WS                      [ \t]
NEWLINE                 (\n)|(\r\n)
DOUBLESLASHCOMMENT      "//"[^\n]*
STRING                  \"[^\"]*\"
IDENTIFIER              [A-Za-z_][A-Za-z0-9_]*

ANY                     (.)

%%
{RESERVED1}             {
                            hlsl_message("Line %u: Reserved keyword \"%s\" used.\n", hlsl_ctx.line_no, yytext);
                            set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                        }
{RESERVED2}             {
                            hlsl_message("Line %u: Reserved keyword \"%s\" used.\n", hlsl_ctx.line_no, yytext);
                            set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                        }
{RESERVED3}             {
                            hlsl_message("Line %u: Reserved keyword \"%s\" used.\n", hlsl_ctx.line_no, yytext);
                            set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                        }
{RESERVED4}             {
                            hlsl_message("Line %u: Reserved keyword \"%s\" used.\n", hlsl_ctx.line_no, yytext);
                            set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                        }

BlendState              {return KW_BLENDSTATE;          }
break                   {return KW_BREAK;               }
Buffer                  {return KW_BUFFER;              }
cbuffer                 {return KW_CBUFFER;             }
compile                 {return KW_COMPILE;             }
const                   {return KW_CONST;               }
continue                {return KW_CONTINUE;            }
DepthStencilState       {return KW_DEPTHSTENCILSTATE;   }
DepthStencilView        {return KW_DEPTHSTENCILVIEW;    }
discard                 {return KW_DISCARD;             }
do                      {return KW_DO;                  }
double                  {return KW_DOUBLE;              }
else                    {return KW_ELSE;                }
extern                  {return KW_EXTERN;              }
false                   {return KW_FALSE;               }
for                     {return KW_FOR;                 }
GeometryShader          {return KW_GEOMETRYSHADER;      }
groupshared             {return KW_GROUPSHARED;         }
if                      {return KW_IF;                  }
in                      {return KW_IN;                  }
inline                  {return KW_INLINE;              }
inout                   {return KW_INOUT;               }
matrix                  {return KW_MATRIX;              }
namespace               {return KW_NAMESPACE;           }
nointerpolation         {return KW_NOINTERPOLATION;     }
out                     {return KW_OUT;                 }
pass                    {return KW_PASS;                }
PixelShader             {return KW_PIXELSHADER;         }
precise                 {return KW_PRECISE;             }
RasterizerState         {return KW_RASTERIZERSTATE;     }
RenderTargetView        {return KW_RENDERTARGETVIEW;    }
return                  {return KW_RETURN;              }
register                {return KW_REGISTER;            }
sampler                 {return KW_SAMPLER;             }
sampler1D               {return KW_SAMPLER1D;           }
sampler2D               {return KW_SAMPLER2D;           }
sampler3D               {return KW_SAMPLER3D;           }
samplerCUBE             {return KW_SAMPLERCUBE;         }
sampler_state           {return KW_SAMPLER_STATE;       }
SamplerComparisonState  {return KW_SAMPLERCOMPARISONSTATE;}
shared                  {return KW_SHARED;              }
stateblock              {return KW_STATEBLOCK;          }
stateblock_state        {return KW_STATEBLOCK_STATE;    }
static                  {return KW_STATIC;              }
string                  {return KW_STRING;              }
struct                  {return KW_STRUCT;              }
switch                  {return KW_SWITCH;              }
tbuffer                 {return KW_TBUFFER;             }
technique               {return KW_TECHNIQUE;           }
technique10             {return KW_TECHNIQUE10;         }
texture                 {return KW_TEXTURE;             }
texture1D               {return KW_TEXTURE1D;           }
Texture1DArray          {return KW_TEXTURE1DARRAY;      }
texture2D               {return KW_TEXTURE2D;           }
Texture2DArray          {return KW_TEXTURE2DARRAY;      }
Texture2DMS             {return KW_TEXTURE2DMS;         }
Texture2DMSArray        {return KW_TEXTURE2DMSARRAY;    }
texture3D               {return KW_TEXTURE3D;           }
Texture3DArray          {return KW_TEXTURE3DARRAY;      }
textureCUBE             {return KW_TEXTURECUBE;         }
true                    {return KW_TRUE;                }
typedef                 {return KW_TYPEDEF;             }
uniform                 {return KW_UNIFORM;             }
vector                  {return KW_VECTOR;              }
VertexShader            {return KW_VERTEXSHADER;        }
void                    {return KW_VOID;                }
volatile                {return KW_VOLATILE;            }
while                   {return KW_WHILE;               }

\+\+                    {return OP_INC;                 }
\-\-                    {return OP_DEC;                 }
&&                      {return OP_AND;                 }
\|\|                    {return OP_OR;                  }
==                      {return OP_EQ;                  }
\<\<                    {return OP_LEFTSHIFT;           }
\<\<=                   {return OP_LEFTSHIFTASSIGN;     }
\>\>                    {return OP_RIGHTSHIFT;          }
\>\>=                   {return OP_RIGHTSHIFTASSIGN;    }
\.\.\.                  {return OP_ELLIPSIS;            }
\<=                     {return OP_LE;                  }
\>=                     {return OP_GE;                  }
!=                      {return OP_NE;                  }
\+=                     {return OP_ADDASSIGN;           }
\-=                     {return OP_SUBASSIGN;           }
\*=                     {return OP_MULASSIGN;           }
\/=                     {return OP_DIVASSIGN;           }
%=                      {return OP_MODASSIGN;           }
&=                      {return OP_ANDASSIGN;           }
\|=                     {return OP_ORASSIGN;            }
^=                      {return OP_XORASSIGN;           }
##                      {return OP_UNKNOWN1;            }
#@                      {return OP_UNKNOWN2;            }
::                      {return OP_UNKNOWN3;            }
\-\>                    {return OP_UNKNOWN4;            }

column_major            {return KW_COLUMN_MAJOR;        }
row_major               {return KW_ROW_MAJOR;           }

{IDENTIFIER}            {
                            hlsl_lval.name = d3dcompiler_strdup(yytext);
                            if (get_variable(hlsl_ctx.cur_scope, yytext)
                                    || find_function(yytext))
                                return VAR_IDENTIFIER;
                            else if (get_type(hlsl_ctx.cur_scope, yytext, TRUE))
                                return TYPE_IDENTIFIER;
                            else
                                return NEW_IDENTIFIER;
                        }

[0-9]*\.[0-9]+([eE][+-]?[0-9]+)?[h|H|f|F]? {
                            hlsl_lval.floatval = atof(yytext);
                            return C_FLOAT;
                        }
[0-9]+\.([eE][+-]?[0-9]+)?[h|H|f|F]? {
                            hlsl_lval.floatval = atof(yytext);
                            return C_FLOAT;
                        }
[0-9]+([eE][+-]?[0-9]+)?[h|H|f|F] {
                            hlsl_lval.floatval = atof(yytext);
                            return C_FLOAT;
                        }
0x[0-9a-fA-F]+          {
                            sscanf(yytext, "0x%x", &hlsl_lval.intval);
                            return C_INTEGER;
                        }
0[0-7]+                 {
                            sscanf(yytext, "0%o", &hlsl_lval.intval);
                            return C_INTEGER;
                        }
[0-9]+                  {
                            hlsl_lval.intval = (atoi(yytext));
                            return C_INTEGER;
                        }

{DOUBLESLASHCOMMENT}    {}

{WS}+                   {}
{NEWLINE}               {
                            hlsl_ctx.line_no++;
                            hlsl_ctx.column = 1;
                        }

^#                      {
                            BEGIN pp;
                        }

<pp>pragma{WS}+         {
                            TRACE("Got a #pragma.\n");
                            BEGIN pp_pragma;
                        }
<pp_pragma>pack_matrix{WS}*\({WS}*row_major{WS}*\) {
                            TRACE("#pragma setting row_major mode.\n");
                            hlsl_ctx.matrix_majority = HLSL_ROW_MAJOR;
                            BEGIN pp_ignore;
                        }
<pp_pragma>pack_matrix{WS}*\({WS}*column_major{WS}*\) {
                            TRACE("#pragma setting column_major mode.\n");
                            hlsl_ctx.matrix_majority = HLSL_COLUMN_MAJOR;
                            BEGIN pp_ignore;
                        }
<pp_pragma>{NEWLINE}    {
                            FIXME("Unsupported preprocessor #pragma directive at line %u.\n", hlsl_ctx.line_no);
                            BEGIN INITIAL;
                        }
<pp_pragma>{ANY}        {}
<pp>[0-9]+              {
                            TRACE("Preprocessor line info.\n");
                            BEGIN pp_line;
                            hlsl_lval.intval = (atoi(yytext));
                            return PRE_LINE;
                        }
<pp_line>{STRING}       {
                            char *string = d3dcompiler_strdup(yytext + 1);

                            BEGIN pp_ignore;
                            string[strlen(string) - 1] = 0;
                            hlsl_lval.name = string;
                            return STRING;
                        }
<pp_line>{WS}+          {}
<pp_line>{NEWLINE}      {
                            FIXME("Malformed preprocessor line directive?\n");
                            BEGIN INITIAL;
                        }
<pp_ignore>{NEWLINE}    {
                            BEGIN INITIAL;
                        }
<pp_ignore>{ANY}        {}
<pp>{NEWLINE}           {
                            FIXME("Unexpected preprocessor directive.\n");
                            BEGIN INITIAL;
                        }
<pp>{ANY}               {}

{ANY}                   {
                            return yytext[0];
                        }

%%

struct bwriter_shader *parse_hlsl(enum shader_type type, DWORD major, DWORD minor,
        const char *entrypoint, char **messages);

struct bwriter_shader *parse_hlsl_shader(const char *text, enum shader_type type, DWORD major, DWORD minor,
        const char *entrypoint, char **messages)
{
    struct bwriter_shader *ret = NULL;
    YY_BUFFER_STATE buffer;

    buffer = hlsl__scan_string(text);
    hlsl__switch_to_buffer(buffer);

    ret = parse_hlsl(type, major, minor, entrypoint, messages);

    hlsl__delete_buffer(buffer);
    return ret;
}
