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
#define YY_NO_UNISTD_H
#include "hlsl.h"
#include "hlsl.tab.h"

#define YYSTYPE HLSL_YYSTYPE
#define YYLTYPE HLSL_YYLTYPE

static void update_location(struct hlsl_ctx *ctx, YYLTYPE *loc);

static void apply_escape_sequences(char *str);

#define YY_USER_ACTION update_location(yyget_extra(yyscanner), yyget_lloc(yyscanner));

%}

%option bison-bridge
%option bison-locations
%option extra-type="struct hlsl_ctx *"
%option never-interactive
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option prefix="hlsl_yy"
%option reentrant

%x pp pp_line pp_pragma pp_ignore

RESERVED1               auto|catch|char|class|const_cast|delete|dynamic_cast|enum
RESERVED2               explicit|friend|goto|long|mutable|new|operator|private|protected|public
RESERVED3               reinterpret_cast|short|signed|sizeof|static_cast|template|this|throw|try
RESERVED4               typename|union|using|virtual

WS                      [ \t]
NEWLINE                 (\n)|(\r\n)
STRING                  \"([^\"\\]|\\.)*\"
IDENTIFIER              [A-Za-z_][A-Za-z0-9_]*

ANY                     (.)

%%
{RESERVED1}             |
{RESERVED2}             |
{RESERVED3}             |
{RESERVED4}             {
                            struct hlsl_ctx *ctx = yyget_extra(yyscanner);

                            hlsl_error(ctx, yylloc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                                    "Reserved keyword \"%s\" used.", yytext);
                        }

BlendState              {return KW_BLENDSTATE;          }
break                   {return KW_BREAK;               }
Buffer                  {return KW_BUFFER;              }
ByteAddressBuffer       {return KW_BYTEADDRESSBUFFER;   }
case                    {return KW_CASE;                }
cbuffer                 {return KW_CBUFFER;             }
centroid                {return KW_CENTROID;            }
column_major            {return KW_COLUMN_MAJOR;        }
ComputeShader           {return KW_COMPUTESHADER;       }
compile                 {return KW_COMPILE;             }
CompileShader           {return KW_COMPILESHADER;       }
const                   {return KW_CONST;               }
ConstructGSWithSO       {return KW_CONSTRUCTGSWITHSO;   }
continue                {return KW_CONTINUE;            }
DepthStencilState       {return KW_DEPTHSTENCILSTATE;   }
DepthStencilView        {return KW_DEPTHSTENCILVIEW;    }
default                 {return KW_DEFAULT;             }
discard                 {return KW_DISCARD;             }
DomainShader            {return KW_DOMAINSHADER;        }
do                      {return KW_DO;                  }
else                    {return KW_ELSE;                }
export                  {return KW_EXPORT;              }
extern                  {return KW_EXTERN;              }
false                   {return KW_FALSE;               }
for                     {return KW_FOR;                 }
fxgroup                 {return KW_FXGROUP;             }
GeometryShader          {return KW_GEOMETRYSHADER;      }
groupshared             {return KW_GROUPSHARED;         }
HullShader              {return KW_HULLSHADER;          }
if                      {return KW_IF;                  }
in                      {return KW_IN;                  }
inline                  {return KW_INLINE;              }
inout                   {return KW_INOUT;               }
linear                  {return KW_LINEAR;              }
matrix                  {return KW_MATRIX;              }
namespace               {return KW_NAMESPACE;           }
nointerpolation         {return KW_NOINTERPOLATION;     }
noperspective           {return KW_NOPERSPECTIVE;       }
NULL                    {return KW_NULL;                }
out                     {return KW_OUT;                 }
packoffset              {return KW_PACKOFFSET;          }
pass                    {return KW_PASS;                }
PixelShader             {return KW_PIXELSHADER;         }
pixelshader             {return KW_PIXELSHADER;         }
RasterizerOrderedBuffer           {return KW_RASTERIZERORDEREDBUFFER;            }
RasterizerOrderedStructuredBuffer {return KW_RASTERIZERORDEREDSTRUCTUREDBUFFER;  }
RasterizerOrderedTexture1D        {return KW_RASTERIZERORDEREDTEXTURE1D;         }
RasterizerOrderedTexture1DArray   {return KW_RASTERIZERORDEREDTEXTURE1DARRAY;    }
RasterizerOrderedTexture2D        {return KW_RASTERIZERORDEREDTEXTURE2D;         }
RasterizerOrderedTexture2DArray   {return KW_RASTERIZERORDEREDTEXTURE2DARRAY;    }
RasterizerOrderedTexture3D        {return KW_RASTERIZERORDEREDTEXTURE3D;         }
RasterizerState         {return KW_RASTERIZERSTATE;     }
register                {return KW_REGISTER;            }
RenderTargetView        {return KW_RENDERTARGETVIEW;    }
return                  {return KW_RETURN;              }
row_major               {return KW_ROW_MAJOR;           }
RWBuffer                {return KW_RWBUFFER;            }
RWByteAddressBuffer     {return KW_RWBYTEADDRESSBUFFER; }
RWStructuredBuffer      {return KW_RWSTRUCTUREDBUFFER;  }
RWTexture1D             {return KW_RWTEXTURE1D;         }
RWTexture1DArray        {return KW_RWTEXTURE1DARRAY;    }
RWTexture2D             {return KW_RWTEXTURE2D;         }
RWTexture2DArray        {return KW_RWTEXTURE2DARRAY;    }
RWTexture3D             {return KW_RWTEXTURE3D;         }
sampler                 {return KW_SAMPLER;             }
sampler1D               {return KW_SAMPLER1D;           }
sampler2D               {return KW_SAMPLER2D;           }
sampler3D               {return KW_SAMPLER3D;           }
SamplerComparisonState  {return KW_SAMPLERCOMPARISONSTATE;}
samplerCUBE             {return KW_SAMPLERCUBE;         }
SamplerState            {return KW_SAMPLER;             }
sampler_state           {return KW_SAMPLER_STATE;       }
shared                  {return KW_SHARED;              }
snorm                   {return KW_SNORM;               }
stateblock              {return KW_STATEBLOCK;          }
stateblock_state        {return KW_STATEBLOCK_STATE;    }
static                  {return KW_STATIC;              }
string                  {return KW_STRING;              }
String                  {return KW_STRING;              }
struct                  {return KW_STRUCT;              }
switch                  {return KW_SWITCH;              }
tbuffer                 {return KW_TBUFFER;             }
(?i:technique)          {return KW_TECHNIQUE;           }
technique10             {return KW_TECHNIQUE10;         }
technique11             {return KW_TECHNIQUE11;         }
texture                 {return KW_TEXTURE;             }
Texture1D               {return KW_TEXTURE1D;           }
texture1D               {return KW_TEXTURE1D;           }
Texture1DArray          {return KW_TEXTURE1DARRAY;      }
Texture2D               {return KW_TEXTURE2D;           }
texture2D               {return KW_TEXTURE2D;           }
Texture2DArray          {return KW_TEXTURE2DARRAY;      }
Texture2DMS             {return KW_TEXTURE2DMS;         }
Texture2DMSArray        {return KW_TEXTURE2DMSARRAY;    }
Texture3D               {return KW_TEXTURE3D;           }
texture3D               {return KW_TEXTURE3D;           }
TextureCube             {return KW_TEXTURECUBE;         }
textureCUBE             {return KW_TEXTURECUBE;         }
TextureCubeArray        {return KW_TEXTURECUBEARRAY;    }
true                    {return KW_TRUE;                }
typedef                 {return KW_TYPEDEF;             }
unsigned                {return KW_UNSIGNED;            }
uniform                 {return KW_UNIFORM;             }
unorm                   {return KW_UNORM;               }
vector                  {return KW_VECTOR;              }
VertexShader            {return KW_VERTEXSHADER;        }
vertexshader            {return KW_VERTEXSHADER;        }
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
\^=                     {return OP_XORASSIGN;           }

{IDENTIFIER}            {
                            struct hlsl_ctx *ctx = yyget_extra(yyscanner);

                            yylval->name = hlsl_strdup(ctx, yytext);
                            if (hlsl_version_ge(ctx, 5, 1) && !strcmp(yytext, "ConstantBuffer"))
                                return KW_CONSTANTBUFFER;
                            else if (hlsl_get_var(ctx->cur_scope, yytext) || hlsl_get_function(ctx, yytext))
                                return VAR_IDENTIFIER;
                            else if (hlsl_get_type(ctx->cur_scope, yytext, true, true))
                                return TYPE_IDENTIFIER;
                            else
                                return NEW_IDENTIFIER;
                        }

{STRING}                {
                            struct hlsl_ctx *ctx = yyget_extra(yyscanner);
                            char *string = hlsl_strdup(ctx, yytext + 1);

                            string[strlen(string) - 1] = 0;
                            apply_escape_sequences(string);
                            yylval->name = string;
                            return STRING;
                        }

[0-9]*\.[0-9]+([eE][+-]?[0-9]+)?[h|H|f|F]? {
                            yylval->floatval = atof(yytext);
                            return C_FLOAT;
                        }
[0-9]+\.([eE][+-]?[0-9]+)?[h|H|f|F]? {
                            yylval->floatval = atof(yytext);
                            return C_FLOAT;
                        }
[0-9]+[eE][+-]?[0-9]+[h|H|f|F]? {
                            yylval->floatval = atof(yytext);
                            return C_FLOAT;
                        }
0x[0-9a-fA-F]+[lL]?     {
                            yylval->intval = vkd3d_parse_integer(yytext);
                            return C_INTEGER;
                        }
0[0-7]+[lL]?            {
                            yylval->intval = vkd3d_parse_integer(yytext);
                            return C_INTEGER;
                        }
[0-9]+[lL]?             {
                            yylval->intval = vkd3d_parse_integer(yytext);
                            return C_INTEGER;
                        }
0x[0-9a-fA-F]+([uU]|[uU][lL]|[lL][uU]) {
                            yylval->intval = vkd3d_parse_integer(yytext);
                            return C_UNSIGNED;
                        }
0[0-7]+([uU]|[uU][lL]|[lL][uU]) {
                            yylval->intval = vkd3d_parse_integer(yytext);
                            return C_UNSIGNED;
                        }
[0-9]+([uU]|[uU][lL]|[lL][uU]) {
                            yylval->intval = vkd3d_parse_integer(yytext);
                            return C_UNSIGNED;
                        }

{WS}+                   {}
{NEWLINE}               {
                            struct hlsl_ctx *ctx = yyget_extra(yyscanner);

                            ++ctx->location.line;
                            ctx->location.column = 1;
                        }

^#                      {
                            BEGIN(pp);
                        }

<pp>pragma{WS}+         {
                            BEGIN(pp_pragma);
                        }
<pp_pragma>pack_matrix{WS}*\({WS}*row_major{WS}*\) {
                            struct hlsl_ctx *ctx = yyget_extra(yyscanner);

                            TRACE("#pragma setting row_major mode.\n");
                            ctx->matrix_majority = HLSL_MODIFIER_ROW_MAJOR;
                            BEGIN(pp_ignore);
                        }
<pp_pragma>pack_matrix{WS}*\({WS}*column_major{WS}*\) {
                            struct hlsl_ctx *ctx = yyget_extra(yyscanner);

                            TRACE("#pragma setting column_major mode.\n");
                            ctx->matrix_majority = HLSL_MODIFIER_COLUMN_MAJOR;
                            BEGIN(pp_ignore);
                        }
<pp_pragma>{NEWLINE}    {
                            struct hlsl_ctx *ctx = yyget_extra(yyscanner);

                            FIXME("Unsupported preprocessor #pragma directive at line %u.\n", ctx->location.line);
                            BEGIN(INITIAL);
                        }
<pp_pragma>{ANY}        {}
<pp>[0-9]+              {
                            BEGIN(pp_line);
                            yylval->intval = (atoi(yytext));
                            return PRE_LINE;
                        }
<pp_line>{STRING}       {
                            struct hlsl_ctx *ctx = yyget_extra(yyscanner);
                            char *string = hlsl_strdup(ctx, yytext + 1);

                            BEGIN(pp_ignore);
                            string[strlen(string) - 1] = 0;
                            apply_escape_sequences(string);
                            yylval->name = string;
                            return STRING;
                        }
<pp_line>{WS}+          {}
<pp_line>{ANY}          {
                            FIXME("Malformed preprocessor line directive?\n");
                            BEGIN(INITIAL);
                        }
<pp_line>{NEWLINE}      {
                            FIXME("Malformed preprocessor line directive?\n");
                            BEGIN(INITIAL);
                        }
<pp_ignore>{NEWLINE}    {
                            BEGIN(INITIAL);
                        }
<pp_ignore>{ANY}        {}
<pp>{NEWLINE}           {
                            FIXME("Unexpected preprocessor directive.\n");
                            BEGIN(INITIAL);
                        }
<pp>{ANY}               {}

{ANY}                   {
                            return yytext[0];
                        }

%%

static void update_location(struct hlsl_ctx *ctx, YYLTYPE *lloc)
{
    *lloc = ctx->location;
    ctx->location.column += yyget_leng(ctx->scanner);
}

int hlsl_lexer_compile(struct hlsl_ctx *ctx, const struct vkd3d_shader_code *hlsl)
{
    YY_BUFFER_STATE buffer;
    int ret;

    yylex_init_extra(ctx, &ctx->scanner);
    buffer = yy_scan_bytes(hlsl->code, hlsl->size, ctx->scanner);
    yy_switch_to_buffer(buffer, ctx->scanner);

    ret = hlsl_yyparse(ctx->scanner, ctx);

    yy_delete_buffer(buffer, ctx->scanner);
    yylex_destroy(ctx->scanner);
    return ret;
}

static void apply_escape_sequences(char *str)
{
    unsigned int i = 0, k = 0, r;

    while (str[i])
    {
        unsigned char v = 0;

        if (str[i] != '\\')
        {
            str[k++] = str[i];
            ++i;
            continue;
        }

        ++i;
        VKD3D_ASSERT(str[i]);

        if ('0' <= str[i] && str[i] <= '7')
        {
            /* octal, up to 3 digits. */
            for (r = 0; r < 3; ++r)
            {
                char c = str[i];

                if ('0' <= c && c <= '7')
                {
                    v = v << 3;
                    v += c - '0';
                    ++i;
                }
                else
                    break;
            }
            str[k++] = v;
            continue;
        }

        if (str[i] == 'x')
        {
            bool number = false;

            /* hexadecimal */
            ++i;
            while (1)
            {
                char c = str[i];

                if ('0' <= c && c <= '9')
                {
                    v = v << 4;
                    v += c - '0';
                    number = true;
                    ++i;
                }
                else if ('a' <= c && c <= 'f')
                {
                    v = v << 4;
                    v += c - 'a' + 10;
                    number = true;
                    ++i;
                }
                else if ('A' <= c && c <= 'F')
                {
                    v = v << 4;
                    v += c - 'A' + 10;
                    number = true;
                    ++i;
                }
                else
                    break;
            }
            if (number)
                str[k++] = v;
            else
                str[k++] = 'x';
            continue;
        }

        switch (str[i])
        {
            case 'a':
                str[k++] = '\a';
                break;
            case 'b':
                str[k++] = '\b';
                break;
            case 'f':
                str[k++] = '\f';
                break;
            case 'n':
                str[k++] = '\n';
                break;
            case 'r':
                str[k++] = '\r';
                break;
            case 't':
                str[k++] = '\t';
                break;
            case 'v':
                str[k++] = '\v';
                break;

            default:
                str[k++] = str[i];
                break;
        }
        ++i;
    }
    str[k++] = '\0';
}
