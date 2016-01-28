/*
 * Copyright 2008 Stefan Dösinger
 * Copyright 2009 Matteo Bruni
 * Copyright 2008-2009 Henri Verbeet for CodeWeavers
 * Copyright 2010 Rico Schüller
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
 *
 */

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

#define WINE_D3DCOMPILER_TO_STR(x) case x: return #x

const char *debug_d3dcompiler_shader_variable_class(D3D_SHADER_VARIABLE_CLASS c)
{
    switch (c)
    {
        WINE_D3DCOMPILER_TO_STR(D3D_SVC_SCALAR);
        WINE_D3DCOMPILER_TO_STR(D3D_SVC_VECTOR);
        WINE_D3DCOMPILER_TO_STR(D3D_SVC_MATRIX_ROWS);
        WINE_D3DCOMPILER_TO_STR(D3D_SVC_MATRIX_COLUMNS);
        WINE_D3DCOMPILER_TO_STR(D3D_SVC_OBJECT);
        WINE_D3DCOMPILER_TO_STR(D3D_SVC_STRUCT);
        WINE_D3DCOMPILER_TO_STR(D3D_SVC_INTERFACE_CLASS);
        WINE_D3DCOMPILER_TO_STR(D3D_SVC_INTERFACE_POINTER);
        default:
            FIXME("Unrecognized D3D_SHADER_VARIABLE_CLASS %#x.\n", c);
            return "unrecognized";
    }
}

const char *debug_d3dcompiler_shader_variable_type(D3D_SHADER_VARIABLE_TYPE t)
{
    switch (t)
    {
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_VOID);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_BOOL);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_INT);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_FLOAT);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_STRING);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURE);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURE1D);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURE2D);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURE3D);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURECUBE);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_SAMPLER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_PIXELSHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_VERTEXSHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_UINT);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_UINT8);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_GEOMETRYSHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RASTERIZER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_DEPTHSTENCIL);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_BLEND);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_BUFFER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_CBUFFER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TBUFFER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURE1DARRAY);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURE2DARRAY);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RENDERTARGETVIEW);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_DEPTHSTENCILVIEW);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURE2DMS);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURE2DMSARRAY);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_TEXTURECUBEARRAY);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_HULLSHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_DOMAINSHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_INTERFACE_POINTER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_COMPUTESHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_DOUBLE);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RWTEXTURE1D);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RWTEXTURE1DARRAY);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RWTEXTURE2D);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RWTEXTURE2DARRAY);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RWTEXTURE3D);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RWBUFFER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_BYTEADDRESS_BUFFER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RWBYTEADDRESS_BUFFER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_STRUCTURED_BUFFER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_RWSTRUCTURED_BUFFER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_APPEND_STRUCTURED_BUFFER);
        WINE_D3DCOMPILER_TO_STR(D3D_SVT_CONSUME_STRUCTURED_BUFFER);
        default:
            FIXME("Unrecognized D3D_SHADER_VARIABLE_TYPE %#x.\n", t);
            return "unrecognized";
    }
}

const char *debug_d3dcompiler_d3d_blob_part(D3D_BLOB_PART part)
{
    switch(part)
    {
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_INPUT_SIGNATURE_BLOB);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_OUTPUT_SIGNATURE_BLOB);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_ALL_SIGNATURE_BLOB);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_DEBUG_INFO);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_LEGACY_SHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_XNA_PREPASS_SHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_XNA_SHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_TEST_ALTERNATE_SHADER);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_TEST_COMPILE_DETAILS);
        WINE_D3DCOMPILER_TO_STR(D3D_BLOB_TEST_COMPILE_PERF);
        default:
            FIXME("Unrecognized D3D_BLOB_PART %#x\n", part);
            return "unrecognized";
    }
}

const char *debug_print_srcmod(DWORD mod)
{
    switch (mod)
    {
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_NEG);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_BIAS);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_BIASNEG);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_SIGN);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_SIGNNEG);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_COMP);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_X2);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_X2NEG);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_DZ);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_DW);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_ABS);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_ABSNEG);
        WINE_D3DCOMPILER_TO_STR(BWRITERSPSM_NOT);
        default:
            FIXME("Unrecognized source modifier %#x.\n", mod);
            return "unrecognized_src_mod";
    }
}

#undef WINE_D3DCOMPILER_TO_STR

const char *debug_print_dstmod(DWORD mod)
{
    switch (mod)
    {
        case 0:
            return "";
        case BWRITERSPDM_SATURATE:
            return "_sat";
        case BWRITERSPDM_PARTIALPRECISION:
            return "_pp";
        case BWRITERSPDM_MSAMPCENTROID:
            return "_centroid";
        case BWRITERSPDM_SATURATE | BWRITERSPDM_PARTIALPRECISION:
            return "_sat_pp";
        case BWRITERSPDM_SATURATE | BWRITERSPDM_MSAMPCENTROID:
            return "_sat_centroid";
        case BWRITERSPDM_PARTIALPRECISION | BWRITERSPDM_MSAMPCENTROID:
            return "_pp_centroid";
        case BWRITERSPDM_SATURATE | BWRITERSPDM_PARTIALPRECISION | BWRITERSPDM_MSAMPCENTROID:
            return "_sat_pp_centroid";
        default:
            return "Unexpected modifier\n";
    }
}

const char *debug_print_shift(DWORD shift)
{
    static const char * const shiftstrings[] =
    {
        "",
        "_x2",
        "_x4",
        "_x8",
        "_x16",
        "_x32",
        "",
        "",
        "",
        "",
        "",
        "",
        "_d16",
        "_d8",
        "_d4",
        "_d2",
    };
    return shiftstrings[shift];
}

static const char *get_regname(const struct shader_reg *reg)
{
    switch (reg->type)
    {
        case BWRITERSPR_TEMP:
            return wine_dbg_sprintf("r%u", reg->regnum);
        case BWRITERSPR_INPUT:
            return wine_dbg_sprintf("v%u", reg->regnum);
        case BWRITERSPR_CONST:
            return wine_dbg_sprintf("c%u", reg->regnum);
        case BWRITERSPR_ADDR:
            return wine_dbg_sprintf("a%u", reg->regnum);
        case BWRITERSPR_TEXTURE:
            return wine_dbg_sprintf("t%u", reg->regnum);
        case BWRITERSPR_RASTOUT:
            switch (reg->regnum)
            {
                case BWRITERSRO_POSITION:   return "oPos";
                case BWRITERSRO_FOG:        return "oFog";
                case BWRITERSRO_POINT_SIZE: return "oPts";
                default: return "Unexpected RASTOUT";
            }
        case BWRITERSPR_ATTROUT:
            return wine_dbg_sprintf("oD%u", reg->regnum);
        case BWRITERSPR_TEXCRDOUT:
            return wine_dbg_sprintf("oT%u", reg->regnum);
        case BWRITERSPR_OUTPUT:
            return wine_dbg_sprintf("o%u", reg->regnum);
        case BWRITERSPR_CONSTINT:
            return wine_dbg_sprintf("i%u", reg->regnum);
        case BWRITERSPR_COLOROUT:
            return wine_dbg_sprintf("oC%u", reg->regnum);
        case BWRITERSPR_DEPTHOUT:
            return "oDepth";
        case BWRITERSPR_SAMPLER:
            return wine_dbg_sprintf("s%u", reg->regnum);
        case BWRITERSPR_CONSTBOOL:
            return wine_dbg_sprintf("b%u", reg->regnum);
        case BWRITERSPR_LOOP:
            return "aL";
        case BWRITERSPR_MISCTYPE:
            switch (reg->regnum)
            {
                case 0: return "vPos";
                case 1: return "vFace";
                default: return "unexpected misctype";
            }
        case BWRITERSPR_LABEL:
            return wine_dbg_sprintf("l%u", reg->regnum);
        case BWRITERSPR_PREDICATE:
            return wine_dbg_sprintf("p%u", reg->regnum);
        default:
            return wine_dbg_sprintf("unknown regname %#x", reg->type);
    }
}

static const char *debug_print_writemask(DWORD mask)
{
    char ret[6];
    unsigned char pos = 1;

    if(mask == BWRITERSP_WRITEMASK_ALL) return "";
    ret[0] = '.';
    if(mask & BWRITERSP_WRITEMASK_0) ret[pos++] = 'x';
    if(mask & BWRITERSP_WRITEMASK_1) ret[pos++] = 'y';
    if(mask & BWRITERSP_WRITEMASK_2) ret[pos++] = 'z';
    if(mask & BWRITERSP_WRITEMASK_3) ret[pos++] = 'w';
    ret[pos] = 0;

    return wine_dbg_sprintf("%s", ret);
}

static const char *debug_print_swizzle(DWORD arg)
{
    char ret[6];
    unsigned int i;
    DWORD swizzle[4];

    switch (arg)
    {
        case BWRITERVS_NOSWIZZLE:
            return "";
        case BWRITERVS_SWIZZLE_X:
            return ".x";
        case BWRITERVS_SWIZZLE_Y:
            return ".y";
        case BWRITERVS_SWIZZLE_Z:
            return ".z";
        case BWRITERVS_SWIZZLE_W:
            return ".w";
    }

    swizzle[0] = (arg >> (BWRITERVS_SWIZZLE_SHIFT + 0)) & 0x03;
    swizzle[1] = (arg >> (BWRITERVS_SWIZZLE_SHIFT + 2)) & 0x03;
    swizzle[2] = (arg >> (BWRITERVS_SWIZZLE_SHIFT + 4)) & 0x03;
    swizzle[3] = (arg >> (BWRITERVS_SWIZZLE_SHIFT + 6)) & 0x03;

    ret[0] = '.';
    for (i = 0; i < 4; ++i)
    {
        switch (swizzle[i])
        {
            case 0: ret[1 + i] = 'x'; break;
            case 1: ret[1 + i] = 'y'; break;
            case 2: ret[1 + i] = 'z'; break;
            case 3: ret[1 + i] = 'w'; break;
        }
    }
    ret[5] = '\0';

    return wine_dbg_sprintf("%s", ret);
}

static const char *debug_print_relarg(const struct shader_reg *reg)
{
    const char *short_swizzle;
    if (!reg->rel_reg) return "";

    short_swizzle = debug_print_swizzle(reg->rel_reg->u.swizzle);

    if (reg->rel_reg->type == BWRITERSPR_ADDR)
        return wine_dbg_sprintf("[a%u%s]", reg->rel_reg->regnum, short_swizzle);
    else if(reg->rel_reg->type == BWRITERSPR_LOOP && reg->rel_reg->regnum == 0)
        return wine_dbg_sprintf("[aL%s]", short_swizzle);
    else
        return "Unexpected relative addressing argument";
}

const char *debug_print_dstreg(const struct shader_reg *reg)
{
    return wine_dbg_sprintf("%s%s%s", get_regname(reg),
            debug_print_relarg(reg),
            debug_print_writemask(reg->u.writemask));
}

const char *debug_print_srcreg(const struct shader_reg *reg)
{
    switch (reg->srcmod)
    {
        case BWRITERSPSM_NONE:
            return wine_dbg_sprintf("%s%s%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_NEG:
            return wine_dbg_sprintf("-%s%s%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_BIAS:
            return wine_dbg_sprintf("%s%s_bias%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_BIASNEG:
            return wine_dbg_sprintf("-%s%s_bias%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_SIGN:
            return wine_dbg_sprintf("%s%s_bx2%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_SIGNNEG:
            return wine_dbg_sprintf("-%s%s_bx2%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_COMP:
            return wine_dbg_sprintf("1 - %s%s%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_X2:
            return wine_dbg_sprintf("%s%s_x2%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_X2NEG:
            return wine_dbg_sprintf("-%s%s_x2%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_DZ:
            return wine_dbg_sprintf("%s%s_dz%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_DW:
            return wine_dbg_sprintf("%s%s_dw%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_ABS:
            return wine_dbg_sprintf("%s%s_abs%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_ABSNEG:
            return wine_dbg_sprintf("-%s%s_abs%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_NOT:
            return wine_dbg_sprintf("!%s%s%s", get_regname(reg),
                    debug_print_relarg(reg),
                    debug_print_swizzle(reg->u.swizzle));
    }
    return "Unknown modifier";
}

const char *debug_print_comp(DWORD comp)
{
    switch (comp)
    {
        case BWRITER_COMPARISON_NONE: return "";
        case BWRITER_COMPARISON_GT:   return "_gt";
        case BWRITER_COMPARISON_EQ:   return "_eq";
        case BWRITER_COMPARISON_GE:   return "_ge";
        case BWRITER_COMPARISON_LT:   return "_lt";
        case BWRITER_COMPARISON_NE:   return "_ne";
        case BWRITER_COMPARISON_LE:   return "_le";
        default: return "_unknown";
    }
}

const char *debug_print_opcode(DWORD opcode)
{
    switch (opcode)
    {
        case BWRITERSIO_NOP:          return "nop";
        case BWRITERSIO_MOV:          return "mov";
        case BWRITERSIO_ADD:          return "add";
        case BWRITERSIO_SUB:          return "sub";
        case BWRITERSIO_MAD:          return "mad";
        case BWRITERSIO_MUL:          return "mul";
        case BWRITERSIO_RCP:          return "rcp";
        case BWRITERSIO_RSQ:          return "rsq";
        case BWRITERSIO_DP3:          return "dp3";
        case BWRITERSIO_DP4:          return "dp4";
        case BWRITERSIO_MIN:          return "min";
        case BWRITERSIO_MAX:          return "max";
        case BWRITERSIO_SLT:          return "slt";
        case BWRITERSIO_SGE:          return "sge";
        case BWRITERSIO_EXP:          return "exp";
        case BWRITERSIO_LOG:          return "log";
        case BWRITERSIO_LIT:          return "lit";
        case BWRITERSIO_DST:          return "dst";
        case BWRITERSIO_LRP:          return "lrp";
        case BWRITERSIO_FRC:          return "frc";
        case BWRITERSIO_M4x4:         return "m4x4";
        case BWRITERSIO_M4x3:         return "m4x3";
        case BWRITERSIO_M3x4:         return "m3x4";
        case BWRITERSIO_M3x3:         return "m3x3";
        case BWRITERSIO_M3x2:         return "m3x2";
        case BWRITERSIO_CALL:         return "call";
        case BWRITERSIO_CALLNZ:       return "callnz";
        case BWRITERSIO_LOOP:         return "loop";
        case BWRITERSIO_RET:          return "ret";
        case BWRITERSIO_ENDLOOP:      return "endloop";
        case BWRITERSIO_LABEL:        return "label";
        case BWRITERSIO_DCL:          return "dcl";
        case BWRITERSIO_POW:          return "pow";
        case BWRITERSIO_CRS:          return "crs";
        case BWRITERSIO_SGN:          return "sgn";
        case BWRITERSIO_ABS:          return "abs";
        case BWRITERSIO_NRM:          return "nrm";
        case BWRITERSIO_SINCOS:       return "sincos";
        case BWRITERSIO_REP:          return "rep";
        case BWRITERSIO_ENDREP:       return "endrep";
        case BWRITERSIO_IF:           return "if";
        case BWRITERSIO_IFC:          return "ifc";
        case BWRITERSIO_ELSE:         return "else";
        case BWRITERSIO_ENDIF:        return "endif";
        case BWRITERSIO_BREAK:        return "break";
        case BWRITERSIO_BREAKC:       return "breakc";
        case BWRITERSIO_MOVA:         return "mova";
        case BWRITERSIO_DEFB:         return "defb";
        case BWRITERSIO_DEFI:         return "defi";
        case BWRITERSIO_TEXCOORD:     return "texcoord";
        case BWRITERSIO_TEXKILL:      return "texkill";
        case BWRITERSIO_TEX:          return "tex";
        case BWRITERSIO_TEXBEM:       return "texbem";
        case BWRITERSIO_TEXBEML:      return "texbeml";
        case BWRITERSIO_TEXREG2AR:    return "texreg2ar";
        case BWRITERSIO_TEXREG2GB:    return "texreg2gb";
        case BWRITERSIO_TEXM3x2PAD:   return "texm3x2pad";
        case BWRITERSIO_TEXM3x2TEX:   return "texm3x2tex";
        case BWRITERSIO_TEXM3x3PAD:   return "texm3x3pad";
        case BWRITERSIO_TEXM3x3TEX:   return "texm3x3tex";
        case BWRITERSIO_TEXM3x3SPEC:  return "texm3x3vspec";
        case BWRITERSIO_TEXM3x3VSPEC: return "texm3x3vspec";
        case BWRITERSIO_EXPP:         return "expp";
        case BWRITERSIO_LOGP:         return "logp";
        case BWRITERSIO_CND:          return "cnd";
        case BWRITERSIO_DEF:          return "def";
        case BWRITERSIO_TEXREG2RGB:   return "texreg2rgb";
        case BWRITERSIO_TEXDP3TEX:    return "texdp3tex";
        case BWRITERSIO_TEXM3x2DEPTH: return "texm3x2depth";
        case BWRITERSIO_TEXDP3:       return "texdp3";
        case BWRITERSIO_TEXM3x3:      return "texm3x3";
        case BWRITERSIO_TEXDEPTH:     return "texdepth";
        case BWRITERSIO_CMP:          return "cmp";
        case BWRITERSIO_BEM:          return "bem";
        case BWRITERSIO_DP2ADD:       return "dp2add";
        case BWRITERSIO_DSX:          return "dsx";
        case BWRITERSIO_DSY:          return "dsy";
        case BWRITERSIO_TEXLDD:       return "texldd";
        case BWRITERSIO_SETP:         return "setp";
        case BWRITERSIO_TEXLDL:       return "texldl";
        case BWRITERSIO_BREAKP:       return "breakp";
        case BWRITERSIO_PHASE:        return "phase";

        case BWRITERSIO_TEXLDP:       return "texldp";
        case BWRITERSIO_TEXLDB:       return "texldb";

        default:                      return "unknown";
    }
}

void skip_dword_unknown(const char **ptr, unsigned int count)
{
    unsigned int i;
    DWORD d;

    FIXME("Skipping %u unknown DWORDs:\n", count);
    for (i = 0; i < count; ++i)
    {
        read_dword(ptr, &d);
        FIXME("\t0x%08x\n", d);
    }
}

static void write_dword_unknown(char **ptr, DWORD d)
{
    FIXME("Writing unknown DWORD 0x%08x\n", d);
    write_dword(ptr, d);
}

HRESULT dxbc_add_section(struct dxbc *dxbc, DWORD tag, const char *data, DWORD data_size)
{
    TRACE("dxbc %p, tag %s, size %#x.\n", dxbc, debugstr_an((const char *)&tag, 4), data_size);

    if (dxbc->count >= dxbc->size)
    {
        struct dxbc_section *new_sections;
        DWORD new_size = dxbc->size << 1;

        new_sections = HeapReAlloc(GetProcessHeap(), 0, dxbc->sections, new_size * sizeof(*dxbc->sections));
        if (!new_sections)
        {
            ERR("Failed to allocate dxbc section memory\n");
            return E_OUTOFMEMORY;
        }

        dxbc->sections = new_sections;
        dxbc->size = new_size;
    }

    dxbc->sections[dxbc->count].tag = tag;
    dxbc->sections[dxbc->count].data_size = data_size;
    dxbc->sections[dxbc->count].data = data;
    ++dxbc->count;

    return S_OK;
}

HRESULT dxbc_init(struct dxbc *dxbc, UINT size)
{
    TRACE("dxbc %p, size %u.\n", dxbc, size);

    /* use a good starting value for the size if none specified */
    if (!size) size = 2;

    dxbc->sections = HeapAlloc(GetProcessHeap(), 0, size * sizeof(*dxbc->sections));
    if (!dxbc->sections)
    {
        ERR("Failed to allocate dxbc section memory\n");
        return E_OUTOFMEMORY;
    }

    dxbc->size = size;
    dxbc->count = 0;

    return S_OK;
}

HRESULT dxbc_parse(const char *data, SIZE_T data_size, struct dxbc *dxbc)
{
    const char *ptr = data;
    HRESULT hr;
    unsigned int i;
    DWORD tag, total_size, chunk_count;

    if (!data)
    {
        WARN("No data supplied.\n");
        return E_FAIL;
    }

    read_dword(&ptr, &tag);
    TRACE("tag: %s.\n", debugstr_an((const char *)&tag, 4));

    if (tag != TAG_DXBC)
    {
        WARN("Wrong tag.\n");
        return E_FAIL;
    }

    /* checksum? */
    skip_dword_unknown(&ptr, 4);

    skip_dword_unknown(&ptr, 1);

    read_dword(&ptr, &total_size);
    TRACE("total size: %#x\n", total_size);

    if (data_size != total_size)
    {
        WARN("Wrong size supplied.\n");
        return D3DERR_INVALIDCALL;
    }

    read_dword(&ptr, &chunk_count);
    TRACE("chunk count: %#x\n", chunk_count);

    hr = dxbc_init(dxbc, chunk_count);
    if (FAILED(hr))
    {
        WARN("Failed to init dxbc\n");
        return hr;
    }

    for (i = 0; i < chunk_count; ++i)
    {
        DWORD chunk_tag, chunk_size;
        const char *chunk_ptr;
        DWORD chunk_offset;

        read_dword(&ptr, &chunk_offset);
        TRACE("chunk %u at offset %#x\n", i, chunk_offset);

        chunk_ptr = data + chunk_offset;

        read_dword(&chunk_ptr, &chunk_tag);
        read_dword(&chunk_ptr, &chunk_size);

        hr = dxbc_add_section(dxbc, chunk_tag, chunk_ptr, chunk_size);
        if (FAILED(hr))
        {
            WARN("Failed to add section to dxbc\n");
            return hr;
        }
    }

    return hr;
}

void dxbc_destroy(struct dxbc *dxbc)
{
    TRACE("dxbc %p.\n", dxbc);

    HeapFree(GetProcessHeap(), 0, dxbc->sections);
}

HRESULT dxbc_write_blob(struct dxbc *dxbc, ID3DBlob **blob)
{
    DWORD size = 32, offset = size + 4 * dxbc->count;
    ID3DBlob *object;
    HRESULT hr;
    char *ptr;
    unsigned int i;

    TRACE("dxbc %p, blob %p.\n", dxbc, blob);

    for (i = 0; i < dxbc->count; ++i)
    {
        size += 12 + dxbc->sections[i].data_size;
    }

    hr = D3DCreateBlob(size, &object);
    if (FAILED(hr))
    {
        WARN("Failed to create blob\n");
        return hr;
    }

    ptr = ID3D10Blob_GetBufferPointer(object);

    write_dword(&ptr, TAG_DXBC);

    /* signature(?) */
    write_dword_unknown(&ptr, 0);
    write_dword_unknown(&ptr, 0);
    write_dword_unknown(&ptr, 0);
    write_dword_unknown(&ptr, 0);

    /* seems to be always 1 */
    write_dword_unknown(&ptr, 1);

    /* DXBC size */
    write_dword(&ptr, size);

    /* chunk count */
    write_dword(&ptr, dxbc->count);

    /* write the chunk offsets */
    for (i = 0; i < dxbc->count; ++i)
    {
        write_dword(&ptr, offset);
        offset += 8 + dxbc->sections[i].data_size;
    }

    /* write the chunks */
    for (i = 0; i < dxbc->count; ++i)
    {
        write_dword(&ptr, dxbc->sections[i].tag);
        write_dword(&ptr, dxbc->sections[i].data_size);
        memcpy(ptr, dxbc->sections[i].data, dxbc->sections[i].data_size);
        ptr += dxbc->sections[i].data_size;
    }

    TRACE("Created ID3DBlob %p\n", object);

    *blob = object;

    return S_OK;
}

void compilation_message(struct compilation_messages *msg, const char *fmt, va_list args)
{
    char* buffer;
    int rc, size;

    if (msg->capacity == 0)
    {
        msg->string = d3dcompiler_alloc(MESSAGEBUFFER_INITIAL_SIZE);
        if (msg->string == NULL)
        {
            ERR("Error allocating memory for parser messages\n");
            return;
        }
        msg->capacity = MESSAGEBUFFER_INITIAL_SIZE;
    }

    while (1)
    {
        rc = vsnprintf(msg->string + msg->size,
                msg->capacity - msg->size, fmt, args);

        if (rc < 0 || rc >= msg->capacity - msg->size)
        {
            size = msg->capacity * 2;
            buffer = d3dcompiler_realloc(msg->string, size);
            if (buffer == NULL)
            {
                ERR("Error reallocating memory for parser messages\n");
                return;
            }
            msg->string = buffer;
            msg->capacity = size;
        }
        else
        {
            TRACE("%s", msg->string + msg->size);
            msg->size += rc;
            return;
        }
    }
}

BOOL add_declaration(struct hlsl_scope *scope, struct hlsl_ir_var *decl, BOOL local_var)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
    {
        if (!strcmp(decl->name, var->name))
            return FALSE;
    }
    if (local_var && scope->upper->upper == hlsl_ctx.globals)
    {
        /* Check whether the variable redefines a function parameter. */
        LIST_FOR_EACH_ENTRY(var, &scope->upper->vars, struct hlsl_ir_var, scope_entry)
        {
            if (!strcmp(decl->name, var->name))
                return FALSE;
        }
    }

    list_add_tail(&scope->vars, &decl->scope_entry);
    return TRUE;
}

struct hlsl_ir_var *get_variable(struct hlsl_scope *scope, const char *name)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
    {
        if (!strcmp(name, var->name))
            return var;
    }
    if (!scope->upper)
        return NULL;
    return get_variable(scope->upper, name);
}

void free_declaration(struct hlsl_ir_var *decl)
{
    d3dcompiler_free((void *)decl->name);
    d3dcompiler_free((void *)decl->semantic);
    d3dcompiler_free((void *)decl->reg_reservation);
    d3dcompiler_free(decl);
}

struct hlsl_type *new_hlsl_type(const char *name, enum hlsl_type_class type_class,
        enum hlsl_base_type base_type, unsigned dimx, unsigned dimy)
{
    struct hlsl_type *type;

    type = d3dcompiler_alloc(sizeof(*type));
    if (!type)
    {
        ERR("Out of memory\n");
        return NULL;
    }
    type->name = name;
    type->type = type_class;
    type->base_type = base_type;
    type->dimx = dimx;
    type->dimy = dimy;

    list_add_tail(&hlsl_ctx.types, &type->entry);

    return type;
}

struct hlsl_type *new_array_type(struct hlsl_type *basic_type, unsigned int array_size)
{
    struct hlsl_type *type = new_hlsl_type(NULL, HLSL_CLASS_ARRAY, HLSL_TYPE_FLOAT, 1, 1);

    if (!type)
        return NULL;

    type->modifiers = basic_type->modifiers;
    type->e.array.elements_count = array_size;
    type->e.array.type = basic_type;
    return type;
}

struct hlsl_type *get_type(struct hlsl_scope *scope, const char *name, BOOL recursive)
{
    struct wine_rb_entry *entry = wine_rb_get(&scope->types, name);
    if (entry)
        return WINE_RB_ENTRY_VALUE(entry, struct hlsl_type, scope_entry);

    if (recursive && scope->upper)
        return get_type(scope->upper, name, recursive);
    return NULL;
}

BOOL find_function(const char *name)
{
    return wine_rb_get(&hlsl_ctx.functions, name) != NULL;
}

unsigned int components_count_type(struct hlsl_type *type)
{
    unsigned int count = 0;
    struct hlsl_struct_field *field;

    if (type->type <= HLSL_CLASS_LAST_NUMERIC)
    {
        return type->dimx * type->dimy;
    }
    if (type->type == HLSL_CLASS_ARRAY)
    {
        return components_count_type(type->e.array.type) * type->e.array.elements_count;
    }
    if (type->type != HLSL_CLASS_STRUCT)
    {
        ERR("Unexpected data type %s.\n", debug_hlsl_type(type));
        return 0;
    }

    LIST_FOR_EACH_ENTRY(field, type->e.elements, struct hlsl_struct_field, entry)
    {
        count += components_count_type(field->type);
    }
    return count;
}

BOOL compare_hlsl_types(const struct hlsl_type *t1, const struct hlsl_type *t2)
{
    if (t1 == t2)
        return TRUE;

    if (t1->type != t2->type)
        return FALSE;
    if (t1->base_type != t2->base_type)
        return FALSE;
    if (t1->base_type == HLSL_TYPE_SAMPLER && t1->sampler_dim != t2->sampler_dim)
        return FALSE;
    if ((t1->modifiers & HLSL_MODIFIERS_COMPARISON_MASK)
            != (t2->modifiers & HLSL_MODIFIERS_COMPARISON_MASK))
        return FALSE;
    if (t1->dimx != t2->dimx)
        return FALSE;
    if (t1->dimy != t2->dimy)
        return FALSE;
    if (t1->type == HLSL_CLASS_STRUCT)
    {
        struct list *t1cur, *t2cur;
        struct hlsl_struct_field *t1field, *t2field;

        t1cur = list_head(t1->e.elements);
        t2cur = list_head(t2->e.elements);
        while (t1cur && t2cur)
        {
            t1field = LIST_ENTRY(t1cur, struct hlsl_struct_field, entry);
            t2field = LIST_ENTRY(t2cur, struct hlsl_struct_field, entry);
            if (!compare_hlsl_types(t1field->type, t2field->type))
                return FALSE;
            if (strcmp(t1field->name, t2field->name))
                return FALSE;
            t1cur = list_next(t1->e.elements, t1cur);
            t2cur = list_next(t2->e.elements, t2cur);
        }
        if (t1cur != t2cur)
            return FALSE;
    }
    if (t1->type == HLSL_CLASS_ARRAY)
        return t1->e.array.elements_count == t2->e.array.elements_count
                && compare_hlsl_types(t1->e.array.type, t2->e.array.type);

    return TRUE;
}

struct hlsl_type *clone_hlsl_type(struct hlsl_type *old)
{
    struct hlsl_type *type;
    struct hlsl_struct_field *old_field, *field;

    type = d3dcompiler_alloc(sizeof(*type));
    if (!type)
    {
        ERR("Out of memory\n");
        return NULL;
    }
    if (old->name)
    {
        type->name = d3dcompiler_strdup(old->name);
        if (!type->name)
        {
            d3dcompiler_free(type);
            return NULL;
        }
    }
    type->type = old->type;
    type->base_type = old->base_type;
    type->dimx = old->dimx;
    type->dimy = old->dimy;
    type->modifiers = old->modifiers;
    type->sampler_dim = old->sampler_dim;
    switch (old->type)
    {
        case HLSL_CLASS_ARRAY:
            type->e.array.type = old->e.array.type;
            type->e.array.elements_count = old->e.array.elements_count;
            break;
        case HLSL_CLASS_STRUCT:
            type->e.elements = d3dcompiler_alloc(sizeof(*type->e.elements));
            if (!type->e.elements)
            {
                d3dcompiler_free((void *)type->name);
                d3dcompiler_free(type);
                return NULL;
            }
            list_init(type->e.elements);
            LIST_FOR_EACH_ENTRY(old_field, old->e.elements, struct hlsl_struct_field, entry)
            {
                field = d3dcompiler_alloc(sizeof(*field));
                if (!field)
                {
                    LIST_FOR_EACH_ENTRY_SAFE(field, old_field, type->e.elements, struct hlsl_struct_field, entry)
                    {
                        d3dcompiler_free((void *)field->semantic);
                        d3dcompiler_free((void *)field->name);
                        d3dcompiler_free(field);
                    }
                    d3dcompiler_free(type->e.elements);
                    d3dcompiler_free((void *)type->name);
                    d3dcompiler_free(type);
                    return NULL;
                }
                field->type = clone_hlsl_type(old_field->type);
                field->name = d3dcompiler_strdup(old_field->name);
                if (old_field->semantic)
                    field->semantic = d3dcompiler_strdup(old_field->semantic);
                field->modifiers = old_field->modifiers;
                list_add_tail(type->e.elements, &field->entry);
            }
            break;
        default:
            break;
    }

    list_add_tail(&hlsl_ctx.types, &type->entry);
    return type;
}

static BOOL convertible_data_type(struct hlsl_type *type)
{
    return type->type != HLSL_CLASS_OBJECT;
}

BOOL compatible_data_types(struct hlsl_type *t1, struct hlsl_type *t2)
{
   if (!convertible_data_type(t1) || !convertible_data_type(t2))
        return FALSE;

    if (t1->type <= HLSL_CLASS_LAST_NUMERIC)
    {
        /* Scalar vars can be cast to pretty much everything */
        if (t1->dimx == 1 && t1->dimy == 1)
            return TRUE;

        if (t1->type == HLSL_CLASS_VECTOR && t2->type == HLSL_CLASS_VECTOR)
            return t1->dimx >= t2->dimx;
    }

    /* The other way around is true too i.e. whatever to scalar */
    if (t2->type <= HLSL_CLASS_LAST_NUMERIC && t2->dimx == 1 && t2->dimy == 1)
        return TRUE;

    if (t1->type == HLSL_CLASS_ARRAY)
    {
        if (compare_hlsl_types(t1->e.array.type, t2))
            /* e.g. float4[3] to float4 is allowed */
            return TRUE;

        if (t2->type == HLSL_CLASS_ARRAY || t2->type == HLSL_CLASS_STRUCT)
            return components_count_type(t1) >= components_count_type(t2);
        else
            return components_count_type(t1) == components_count_type(t2);
    }

    if (t1->type == HLSL_CLASS_STRUCT)
        return components_count_type(t1) >= components_count_type(t2);

    if (t2->type == HLSL_CLASS_ARRAY || t2->type == HLSL_CLASS_STRUCT)
        return components_count_type(t1) == components_count_type(t2);

    if (t1->type == HLSL_CLASS_MATRIX || t2->type == HLSL_CLASS_MATRIX)
    {
        if (t1->type == HLSL_CLASS_MATRIX && t2->type == HLSL_CLASS_MATRIX && t1->dimx >= t2->dimx && t1->dimy >= t2->dimy)
            return TRUE;

        /* Matrix-vector conversion is apparently allowed if they have the same components count */
        if ((t1->type == HLSL_CLASS_VECTOR || t2->type == HLSL_CLASS_VECTOR)
                && components_count_type(t1) == components_count_type(t2))
            return TRUE;
        return FALSE;
    }

    if (components_count_type(t1) >= components_count_type(t2))
        return TRUE;
    return FALSE;
}

static BOOL implicit_compatible_data_types(struct hlsl_type *t1, struct hlsl_type *t2)
{
    if (!convertible_data_type(t1) || !convertible_data_type(t2))
        return FALSE;

    if (t1->type <= HLSL_CLASS_LAST_NUMERIC)
    {
        /* Scalar vars can be converted to any other numeric data type */
        if (t1->dimx == 1 && t1->dimy == 1 && t2->type <= HLSL_CLASS_LAST_NUMERIC)
            return TRUE;
        /* The other way around is true too */
        if (t2->dimx == 1 && t2->dimy == 1 && t2->type <= HLSL_CLASS_LAST_NUMERIC)
            return TRUE;
    }

    if (t1->type == HLSL_CLASS_ARRAY && t2->type == HLSL_CLASS_ARRAY)
    {
        return components_count_type(t1) == components_count_type(t2);
    }

    if ((t1->type == HLSL_CLASS_ARRAY && t2->type <= HLSL_CLASS_LAST_NUMERIC)
            || (t1->type <= HLSL_CLASS_LAST_NUMERIC && t2->type == HLSL_CLASS_ARRAY))
    {
        /* e.g. float4[3] to float4 is allowed */
        if (t1->type == HLSL_CLASS_ARRAY && compare_hlsl_types(t1->e.array.type, t2))
            return TRUE;
        if (components_count_type(t1) == components_count_type(t2))
            return TRUE;
        return FALSE;
    }

    if (t1->type <= HLSL_CLASS_VECTOR && t2->type <= HLSL_CLASS_VECTOR)
    {
        if (t1->dimx >= t2->dimx)
            return TRUE;
        return FALSE;
    }

    if (t1->type == HLSL_CLASS_MATRIX || t2->type == HLSL_CLASS_MATRIX)
    {
        if (t1->type == HLSL_CLASS_MATRIX && t2->type == HLSL_CLASS_MATRIX
                && t1->dimx >= t2->dimx && t1->dimy >= t2->dimy)
            return TRUE;

        /* Matrix-vector conversion is apparently allowed if they have the same components count */
        if ((t1->type == HLSL_CLASS_VECTOR || t2->type == HLSL_CLASS_VECTOR)
                && components_count_type(t1) == components_count_type(t2))
            return TRUE;
        return FALSE;
    }

    if (t1->type == HLSL_CLASS_STRUCT && t2->type == HLSL_CLASS_STRUCT)
        return compare_hlsl_types(t1, t2);

    return FALSE;
}

static BOOL expr_compatible_data_types(struct hlsl_type *t1, struct hlsl_type *t2)
{
    if (t1->base_type > HLSL_TYPE_LAST_SCALAR || t2->base_type > HLSL_TYPE_LAST_SCALAR)
        return FALSE;

    /* Scalar vars can be converted to pretty much everything */
    if ((t1->dimx == 1 && t1->dimy == 1) || (t2->dimx == 1 && t2->dimy == 1))
        return TRUE;

    if (t1->type == HLSL_CLASS_VECTOR && t2->type == HLSL_CLASS_VECTOR)
        return TRUE;

    if (t1->type == HLSL_CLASS_MATRIX || t2->type == HLSL_CLASS_MATRIX)
    {
        /* Matrix-vector conversion is apparently allowed if either they have the same components
           count or the matrix is nx1 or 1xn */
        if (t1->type == HLSL_CLASS_VECTOR || t2->type == HLSL_CLASS_VECTOR)
        {
            if (components_count_type(t1) == components_count_type(t2))
                return TRUE;

            return (t1->type == HLSL_CLASS_MATRIX && (t1->dimx == 1 || t1->dimy == 1))
                    || (t2->type == HLSL_CLASS_MATRIX && (t2->dimx == 1 || t2->dimy == 1));
        }

        /* Both matrices */
        if ((t1->dimx >= t2->dimx && t1->dimy >= t2->dimy)
                || (t1->dimx <= t2->dimx && t1->dimy <= t2->dimy))
            return TRUE;
    }

    return FALSE;
}

static enum hlsl_base_type expr_common_base_type(enum hlsl_base_type t1, enum hlsl_base_type t2)
{
    enum hlsl_base_type types[] =
    {
        HLSL_TYPE_BOOL,
        HLSL_TYPE_INT,
        HLSL_TYPE_UINT,
        HLSL_TYPE_HALF,
        HLSL_TYPE_FLOAT,
        HLSL_TYPE_DOUBLE,
    };
    int t1_idx = -1, t2_idx = -1, i;

    for (i = 0; i < sizeof(types) / sizeof(types[0]); ++i)
    {
        /* Always convert away from HLSL_TYPE_HALF */
        if (t1 == types[i])
            t1_idx = t1 == HLSL_TYPE_HALF ? i + 1 : i;
        if (t2 == types[i])
            t2_idx = t2 == HLSL_TYPE_HALF ? i + 1 : i;

        if (t1_idx != -1 && t2_idx != -1)
            break;
    }
    if (t1_idx == -1 || t2_idx == -1)
    {
        FIXME("Unexpected base type.\n");
        return HLSL_TYPE_FLOAT;
    }
    return t1_idx >= t2_idx ? t1 : t2;
}

static struct hlsl_type *expr_common_type(struct hlsl_type *t1, struct hlsl_type *t2,
        struct source_location *loc)
{
    enum hlsl_type_class type;
    enum hlsl_base_type base;
    unsigned int dimx, dimy;

    if (t1->type > HLSL_CLASS_LAST_NUMERIC || t2->type > HLSL_CLASS_LAST_NUMERIC)
    {
        hlsl_report_message(loc->file, loc->line, loc->col, HLSL_LEVEL_ERROR,
                "non scalar/vector/matrix data type in expression");
        return NULL;
    }

    if (compare_hlsl_types(t1, t2))
        return t1;

    if (!expr_compatible_data_types(t1, t2))
    {
        hlsl_report_message(loc->file, loc->line, loc->col, HLSL_LEVEL_ERROR,
                "expression data types are incompatible");
        return NULL;
    }

    if (t1->base_type == t2->base_type)
        base = t1->base_type;
    else
        base = expr_common_base_type(t1->base_type, t2->base_type);

    if (t1->dimx == 1 && t1->dimy == 1)
    {
        type = t2->type;
        dimx = t2->dimx;
        dimy = t2->dimy;
    }
    else if (t2->dimx == 1 && t2->dimy == 1)
    {
        type = t1->type;
        dimx = t1->dimx;
        dimy = t1->dimy;
    }
    else if (t1->type == HLSL_CLASS_MATRIX && t2->type == HLSL_CLASS_MATRIX)
    {
        type = HLSL_CLASS_MATRIX;
        dimx = min(t1->dimx, t2->dimx);
        dimy = min(t1->dimy, t2->dimy);
    }
    else
    {
        /* Two vectors or a vector and a matrix (matrix must be 1xn or nx1) */
        unsigned int max_dim_1, max_dim_2;

        max_dim_1 = max(t1->dimx, t1->dimy);
        max_dim_2 = max(t2->dimx, t2->dimy);
        if (t1->dimx * t1->dimy == t2->dimx * t2->dimy)
        {
            type = HLSL_CLASS_VECTOR;
            dimx = max(t1->dimx, t2->dimx);
            dimy = 1;
        }
        else if (max_dim_1 <= max_dim_2)
        {
            type = t1->type;
            if (type == HLSL_CLASS_VECTOR)
            {
                dimx = max_dim_1;
                dimy = 1;
            }
            else
            {
                dimx = t1->dimx;
                dimy = t1->dimy;
            }
        }
        else
        {
            type = t2->type;
            if (type == HLSL_CLASS_VECTOR)
            {
                dimx = max_dim_2;
                dimy = 1;
            }
            else
            {
                dimx = t2->dimx;
                dimy = t2->dimy;
            }
        }
    }

    return new_hlsl_type(NULL, type, base, dimx, dimy);
}

static struct hlsl_ir_node *implicit_conversion(struct hlsl_ir_node *node, struct hlsl_type *type,
        struct source_location *loc)
{
    struct hlsl_ir_expr *cast;
    struct hlsl_ir_node *operands[3];

    if (compare_hlsl_types(node->data_type, type))
        return node;
    TRACE("Implicit conversion of expression to %s\n", debug_hlsl_type(type));
    operands[0] = node;
    operands[1] = operands[2] = NULL;
    cast = new_expr(HLSL_IR_UNOP_CAST, operands, loc);
    if (!cast)
        return NULL;
    cast->node.data_type = type;
    return &cast->node;
}

struct hlsl_ir_expr *new_expr(enum hlsl_ir_expr_op op, struct hlsl_ir_node **operands,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr = d3dcompiler_alloc(sizeof(*expr));
    struct hlsl_type *type;
    unsigned int i;

    if (!expr)
    {
        ERR("Out of memory\n");
        return NULL;
    }
    expr->node.type = HLSL_IR_EXPR;
    expr->node.loc = *loc;
    type = operands[0]->data_type;
    for (i = 1; i <= 2; ++i)
    {
        if (!operands[i])
            break;
        type = expr_common_type(type, operands[i]->data_type, loc);
        if (!type)
        {
            d3dcompiler_free(expr);
            return NULL;
        }
    }
    for (i = 0; i <= 2; ++i)
    {
        if (!operands[i])
            break;
        if (compare_hlsl_types(operands[i]->data_type, type))
            continue;
        TRACE("Implicitly converting %s into %s in an expression\n", debug_hlsl_type(operands[i]->data_type), debug_hlsl_type(type));
        if (operands[i]->data_type->dimx * operands[i]->data_type->dimy != 1
                && operands[i]->data_type->dimx * operands[i]->data_type->dimy != type->dimx * type->dimy)
        {
            hlsl_report_message(operands[i]->loc.file,
                    operands[i]->loc.line, operands[i]->loc.col, HLSL_LEVEL_WARNING,
                    "implicit truncation of vector/matrix type");
        }
        operands[i] = implicit_conversion(operands[i], type, &operands[i]->loc);
        if (!operands[i])
        {
            ERR("Impossible to convert expression operand %u to %s\n", i + 1, debug_hlsl_type(type));
            d3dcompiler_free(expr);
            return NULL;
        }
    }
    expr->node.data_type = type;
    expr->op = op;
    expr->operands[0] = operands[0];
    expr->operands[1] = operands[1];
    expr->operands[2] = operands[2];

    return expr;
}

struct hlsl_ir_expr *new_cast(struct hlsl_ir_node *node, struct hlsl_type *type,
        struct source_location *loc)
{
    struct hlsl_ir_expr *cast;
    struct hlsl_ir_node *operands[3];

    operands[0] = node;
    operands[1] = operands[2] = NULL;
    cast = new_expr(HLSL_IR_UNOP_CAST, operands, loc);
    if (cast)
        cast->node.data_type = type;
    return cast;
}

struct hlsl_ir_expr *hlsl_mul(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_MUL, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_div(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_DIV, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_mod(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_MOD, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_add(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_ADD, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_sub(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_SUB, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_lt(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_LESS, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_gt(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_GREATER, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_le(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_LEQUAL, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_ge(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_GEQUAL, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_eq(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_EQUAL, ops, loc);
    return expr;
}

struct hlsl_ir_expr *hlsl_ne(struct hlsl_ir_node *op1, struct hlsl_ir_node *op2,
        struct source_location *loc)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *ops[3];

    ops[0] = op1;
    ops[1] = op2;
    ops[2] = NULL;
    expr = new_expr(HLSL_IR_BINOP_NEQUAL, ops, loc);
    return expr;
}

struct hlsl_ir_deref *new_var_deref(struct hlsl_ir_var *var)
{
    struct hlsl_ir_deref *deref = d3dcompiler_alloc(sizeof(*deref));

    if (!deref)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    deref->node.type = HLSL_IR_DEREF;
    deref->node.data_type = var->node.data_type;
    deref->type = HLSL_IR_DEREF_VAR;
    deref->v.var = var;
    return deref;
}

struct hlsl_ir_deref *new_record_deref(struct hlsl_ir_node *record, struct hlsl_struct_field *field)
{
    struct hlsl_ir_deref *deref = d3dcompiler_alloc(sizeof(*deref));

    if (!deref)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    deref->node.type = HLSL_IR_DEREF;
    deref->node.data_type = field->type;
    deref->type = HLSL_IR_DEREF_RECORD;
    if (record->type == HLSL_IR_VAR)
        deref->v.record.record = &new_var_deref(var_from_node(record))->node;
    else
        deref->v.record.record = record;
    deref->v.record.field = field;
    return deref;
}

static enum hlsl_ir_expr_op op_from_assignment(enum parse_assign_op op)
{
    static const enum hlsl_ir_expr_op ops[] =
    {
        0,
        HLSL_IR_BINOP_ADD,
        HLSL_IR_BINOP_SUB,
        HLSL_IR_BINOP_MUL,
        HLSL_IR_BINOP_DIV,
        HLSL_IR_BINOP_MOD,
        HLSL_IR_BINOP_LSHIFT,
        HLSL_IR_BINOP_RSHIFT,
        HLSL_IR_BINOP_BIT_AND,
        HLSL_IR_BINOP_BIT_OR,
        HLSL_IR_BINOP_BIT_XOR,
    };

    return ops[op];
}

struct hlsl_ir_node *make_assignment(struct hlsl_ir_node *left, enum parse_assign_op assign_op,
        DWORD writemask, struct hlsl_ir_node *right)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_assignment *assign = d3dcompiler_alloc(sizeof(*assign));
    struct hlsl_type *type;
    struct hlsl_ir_node *lhs, *rhs;

    if (!assign)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    TRACE("Creating proper assignment expression.\n");
    rhs = right;
    if (writemask == BWRITERSP_WRITEMASK_ALL)
        type = left->data_type;
    else
    {
        unsigned int dimx = 0;
        DWORD bitmask;
        enum hlsl_type_class type_class;

        if (left->data_type->type > HLSL_CLASS_LAST_NUMERIC)
        {
            hlsl_report_message(left->loc.file, left->loc.line, left->loc.col, HLSL_LEVEL_ERROR,
                    "writemask on a non scalar/vector/matrix type");
            d3dcompiler_free(assign);
            return NULL;
        }
        bitmask = writemask & ((1 << left->data_type->dimx) - 1);
        while (bitmask)
        {
            if (bitmask & 1)
                dimx++;
            bitmask >>= 1;
        }
        if (left->data_type->type == HLSL_CLASS_MATRIX)
            FIXME("Assignments with writemasks and matrices on lhs are not supported yet.\n");
        if (dimx == 1)
            type_class = HLSL_CLASS_SCALAR;
        else
            type_class = left->data_type->type;
        type = new_hlsl_type(NULL, type_class, left->data_type->base_type, dimx, 1);
    }
    assign->node.type = HLSL_IR_ASSIGNMENT;
    assign->node.loc = left->loc;
    assign->node.data_type = type;
    assign->writemask = writemask;
    FIXME("Check for casts in the lhs.\n");

    lhs = left;
    if (lhs->type == HLSL_IR_VAR)
    {
        struct hlsl_ir_deref *lhs_deref = new_var_deref(var_from_node(lhs));
        lhs = &lhs_deref->node;
    }
    /* FIXME: check for invalid writemasks on the lhs. */

    if (!compare_hlsl_types(type, rhs->data_type))
    {
        struct hlsl_ir_node *converted_rhs;

        if (!implicit_compatible_data_types(rhs->data_type, type))
        {
            hlsl_report_message(rhs->loc.file, rhs->loc.line, rhs->loc.col, HLSL_LEVEL_ERROR,
                    "can't implicitly convert %s to %s",
                    debug_hlsl_type(rhs->data_type), debug_hlsl_type(type));
            free_instr(lhs);
            free_instr(rhs);
            d3dcompiler_free(assign);
            return NULL;
        }
        if (lhs->data_type->dimx * lhs->data_type->dimy < rhs->data_type->dimx * rhs->data_type->dimy)
            hlsl_report_message(rhs->loc.file, rhs->loc.line, rhs->loc.col, HLSL_LEVEL_WARNING,
                    "implicit truncation of vector type");

        converted_rhs = implicit_conversion(rhs, type, &rhs->loc);
        if (!converted_rhs)
        {
            ERR("Couldn't implicitly convert expression to %s.\n", debug_hlsl_type(type));
            free_instr(lhs);
            free_instr(rhs);
            d3dcompiler_free(assign);
            return NULL;
        }
        rhs = converted_rhs;
    }

    assign->lhs = lhs;
    if (assign_op != ASSIGN_OP_ASSIGN)
    {
        struct hlsl_ir_node *operands[3];
        enum hlsl_ir_expr_op op = op_from_assignment(assign_op);

        if (lhs->type != HLSL_IR_DEREF || deref_from_node(lhs)->type != HLSL_IR_DEREF_VAR)
        {
            FIXME("LHS expression not supported in compound assignments yet.\n");
            assign->rhs = rhs;
        }
        else
        {
            struct hlsl_ir_deref *lhs_deref = deref_from_node(lhs), *new_deref;

            TRACE("Adding an expression for the compound assignment.\n");
            new_deref = new_var_deref(lhs_deref->v.var);
            operands[0] = &new_deref->node;
            operands[1] = rhs;
            operands[2] = NULL;
            expr = new_expr(op, operands, &left->loc);
            assign->rhs = &expr->node;
        }
    }
    else
        assign->rhs = rhs;

    return &assign->node;
}

static int compare_hlsl_types_rb(const void *key, const struct wine_rb_entry *entry)
{
    const char *name = key;
    const struct hlsl_type *type = WINE_RB_ENTRY_VALUE(entry, const struct hlsl_type, scope_entry);

    if (name == type->name)
        return 0;

    if (!name || !type->name)
    {
        ERR("hlsl_type without a name in a scope?\n");
        return -1;
    }
    return strcmp(name, type->name);
}

static inline void *d3dcompiler_alloc_rb(size_t size)
{
    return d3dcompiler_alloc(size);
}

static inline void *d3dcompiler_realloc_rb(void *ptr, size_t size)
{
    return d3dcompiler_realloc(ptr, size);
}

static inline void d3dcompiler_free_rb(void *ptr)
{
    d3dcompiler_free(ptr);
}
static const struct wine_rb_functions hlsl_type_rb_funcs =
{
    d3dcompiler_alloc_rb,
    d3dcompiler_realloc_rb,
    d3dcompiler_free_rb,
    compare_hlsl_types_rb,
};

void push_scope(struct hlsl_parse_ctx *ctx)
{
    struct hlsl_scope *new_scope = d3dcompiler_alloc(sizeof(*new_scope));

    if (!new_scope)
    {
        ERR("Out of memory!\n");
        return;
    }
    TRACE("Pushing a new scope\n");
    list_init(&new_scope->vars);
    if (wine_rb_init(&new_scope->types, &hlsl_type_rb_funcs) == -1)
    {
        ERR("Failed to initialize types rbtree.\n");
        d3dcompiler_free(new_scope);
        return;
    }
    new_scope->upper = ctx->cur_scope;
    ctx->cur_scope = new_scope;
    list_add_tail(&ctx->scopes, &new_scope->entry);
}

BOOL pop_scope(struct hlsl_parse_ctx *ctx)
{
    struct hlsl_scope *prev_scope = ctx->cur_scope->upper;
    if (!prev_scope)
        return FALSE;

    TRACE("Popping current scope\n");
    ctx->cur_scope = prev_scope;
    return TRUE;
}

struct hlsl_ir_function_decl *new_func_decl(struct hlsl_type *return_type, struct list *parameters)
{
    struct hlsl_ir_function_decl *decl;

    decl = d3dcompiler_alloc(sizeof(*decl));
    if (!decl)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    decl->node.type = HLSL_IR_FUNCTION_DECL;
    decl->node.data_type = return_type;
    decl->parameters = parameters;

    return decl;
}

static int compare_param_hlsl_types(const struct hlsl_type *t1, const struct hlsl_type *t2)
{
    if (t1->type != t2->type)
    {
        if (!((t1->type == HLSL_CLASS_SCALAR && t2->type == HLSL_CLASS_VECTOR)
                || (t1->type == HLSL_CLASS_VECTOR && t2->type == HLSL_CLASS_SCALAR)))
            return t1->type - t2->type;
    }
    if (t1->base_type != t2->base_type)
        return t1->base_type - t2->base_type;
    if (t1->base_type == HLSL_TYPE_SAMPLER && t1->sampler_dim != t2->sampler_dim)
        return t1->sampler_dim - t2->sampler_dim;
    if (t1->dimx != t2->dimx)
        return t1->dimx - t2->dimx;
    if (t1->dimy != t2->dimy)
        return t1->dimx - t2->dimx;
    if (t1->type == HLSL_CLASS_STRUCT)
    {
        struct list *t1cur, *t2cur;
        struct hlsl_struct_field *t1field, *t2field;
        int r;

        t1cur = list_head(t1->e.elements);
        t2cur = list_head(t2->e.elements);
        while (t1cur && t2cur)
        {
            t1field = LIST_ENTRY(t1cur, struct hlsl_struct_field, entry);
            t2field = LIST_ENTRY(t2cur, struct hlsl_struct_field, entry);
            if ((r = compare_param_hlsl_types(t1field->type, t2field->type)))
                return r;
            if ((r = strcmp(t1field->name, t2field->name)))
                return r;
            t1cur = list_next(t1->e.elements, t1cur);
            t2cur = list_next(t2->e.elements, t2cur);
        }
        if (t1cur != t2cur)
            return t1cur ? 1 : -1;
        return 0;
    }
    if (t1->type == HLSL_CLASS_ARRAY)
    {
        if (t1->e.array.elements_count != t2->e.array.elements_count)
            return t1->e.array.elements_count - t2->e.array.elements_count;
        return compare_param_hlsl_types(t1->e.array.type, t2->e.array.type);
    }

    return 0;
}

static int compare_function_decl_rb(const void *key, const struct wine_rb_entry *entry)
{
    const struct list *params = key;
    const struct hlsl_ir_function_decl *decl = WINE_RB_ENTRY_VALUE(entry, const struct hlsl_ir_function_decl, entry);
    int params_count = params ? list_count(params) : 0;
    int decl_params_count = decl->parameters ? list_count(decl->parameters) : 0;
    int r;
    struct list *p1cur, *p2cur;

    if (params_count != decl_params_count)
        return params_count - decl_params_count;

    p1cur = params ? list_head(params) : NULL;
    p2cur = decl->parameters ? list_head(decl->parameters) : NULL;
    while (p1cur && p2cur)
    {
        struct hlsl_ir_var *p1, *p2;
        p1 = LIST_ENTRY(p1cur, struct hlsl_ir_var, node.entry);
        p2 = LIST_ENTRY(p2cur, struct hlsl_ir_var, node.entry);
        if ((r = compare_param_hlsl_types(p1->node.data_type, p2->node.data_type)))
            return r;
        p1cur = list_next(params, p1cur);
        p2cur = list_next(decl->parameters, p2cur);
    }
    return 0;
}

static const struct wine_rb_functions hlsl_ir_function_decl_rb_funcs =
{
    d3dcompiler_alloc_rb,
    d3dcompiler_realloc_rb,
    d3dcompiler_free_rb,
    compare_function_decl_rb,
};

static int compare_function_rb(const void *key, const struct wine_rb_entry *entry)
{
    const char *name = key;
    const struct hlsl_ir_function *func = WINE_RB_ENTRY_VALUE(entry, const struct hlsl_ir_function,entry);

    return strcmp(name, func->name);
}

static const struct wine_rb_functions function_rb_funcs =
{
    d3dcompiler_alloc_rb,
    d3dcompiler_realloc_rb,
    d3dcompiler_free_rb,
    compare_function_rb,
};

void init_functions_tree(struct wine_rb_tree *funcs)
{
    if (wine_rb_init(&hlsl_ctx.functions, &function_rb_funcs) == -1)
        ERR("Failed to initialize functions rbtree.\n");
}

static const char *debug_base_type(const struct hlsl_type *type)
{
    const char *name = "(unknown)";

    switch (type->base_type)
    {
        case HLSL_TYPE_FLOAT:        name = "float";         break;
        case HLSL_TYPE_HALF:         name = "half";          break;
        case HLSL_TYPE_DOUBLE:       name = "double";        break;
        case HLSL_TYPE_INT:          name = "int";           break;
        case HLSL_TYPE_UINT:         name = "uint";          break;
        case HLSL_TYPE_BOOL:         name = "bool";          break;
        case HLSL_TYPE_SAMPLER:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_GENERIC: name = "sampler";       break;
                case HLSL_SAMPLER_DIM_1D:      name = "sampler1D";     break;
                case HLSL_SAMPLER_DIM_2D:      name = "sampler2D";     break;
                case HLSL_SAMPLER_DIM_3D:      name = "sampler3D";     break;
                case HLSL_SAMPLER_DIM_CUBE:    name = "samplerCUBE";   break;
            }
            break;
        default:
            FIXME("Unhandled case %u\n", type->base_type);
    }
    return name;
}

const char *debug_hlsl_type(const struct hlsl_type *type)
{
    const char *name;

    if (type->name)
        return debugstr_a(type->name);

    if (type->type == HLSL_CLASS_STRUCT)
        return "<anonymous struct>";

    if (type->type == HLSL_CLASS_ARRAY)
    {
        name = debug_base_type(type->e.array.type);
        return wine_dbg_sprintf("%s[%u]", name, type->e.array.elements_count);
    }

    name = debug_base_type(type);

    if (type->type == HLSL_CLASS_SCALAR)
        return wine_dbg_sprintf("%s", name);
    if (type->type == HLSL_CLASS_VECTOR)
        return wine_dbg_sprintf("%s%u", name, type->dimx);
    if (type->type == HLSL_CLASS_MATRIX)
        return wine_dbg_sprintf("%s%ux%u", name, type->dimx, type->dimy);
    return "unexpected_type";
}

const char *debug_modifiers(DWORD modifiers)
{
    char string[110];

    string[0] = 0;
    if (modifiers & HLSL_STORAGE_EXTERN)
        strcat(string, " extern");                       /* 7 */
    if (modifiers & HLSL_STORAGE_NOINTERPOLATION)
        strcat(string, " nointerpolation");              /* 16 */
    if (modifiers & HLSL_MODIFIER_PRECISE)
        strcat(string, " precise");                      /* 8 */
    if (modifiers & HLSL_STORAGE_SHARED)
        strcat(string, " shared");                       /* 7 */
    if (modifiers & HLSL_STORAGE_GROUPSHARED)
        strcat(string, " groupshared");                  /* 12 */
    if (modifiers & HLSL_STORAGE_STATIC)
        strcat(string, " static");                       /* 7 */
    if (modifiers & HLSL_STORAGE_UNIFORM)
        strcat(string, " uniform");                      /* 8 */
    if (modifiers & HLSL_STORAGE_VOLATILE)
        strcat(string, " volatile");                     /* 9 */
    if (modifiers & HLSL_MODIFIER_CONST)
        strcat(string, " const");                        /* 6 */
    if (modifiers & HLSL_MODIFIER_ROW_MAJOR)
        strcat(string, " row_major");                    /* 10 */
    if (modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
        strcat(string, " column_major");                 /* 13 */
    if ((modifiers & (HLSL_MODIFIER_IN | HLSL_MODIFIER_OUT)) == (HLSL_MODIFIER_IN | HLSL_MODIFIER_OUT))
        strcat(string, " inout");                        /* 6 */
    else if (modifiers & HLSL_MODIFIER_IN)
        strcat(string, " in");                           /* 3 */
    else if (modifiers & HLSL_MODIFIER_OUT)
        strcat(string, " out");                          /* 4 */

    return wine_dbg_sprintf("%s", string[0] ? string + 1 : "");
}

static const char *debug_node_type(enum hlsl_ir_node_type type)
{
    static const char * const names[] =
    {
        "HLSL_IR_VAR",
        "HLSL_IR_ASSIGNMENT",
        "HLSL_IR_CONSTANT",
        "HLSL_IR_CONSTRUCTOR",
        "HLSL_IR_DEREF",
        "HLSL_IR_EXPR",
        "HLSL_IR_FUNCTION_DECL",
        "HLSL_IR_IF",
        "HLSL_IR_JUMP",
        "HLSL_IR_SWIZZLE",
    };

    if (type >= sizeof(names) / sizeof(names[0]))
        return "Unexpected node type";
    return names[type];
}

static void debug_dump_instr(const struct hlsl_ir_node *instr);

static void debug_dump_instr_list(const struct list *list)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, list, struct hlsl_ir_node, entry)
    {
        debug_dump_instr(instr);
        TRACE("\n");
    }
}

static void debug_dump_ir_var(const struct hlsl_ir_var *var)
{
    if (var->modifiers)
        TRACE("%s ", debug_modifiers(var->modifiers));
    TRACE("%s %s", debug_hlsl_type(var->node.data_type), var->name);
    if (var->semantic)
        TRACE(" : %s", debugstr_a(var->semantic));
}

static void debug_dump_ir_deref(const struct hlsl_ir_deref *deref)
{
    switch (deref->type)
    {
        case HLSL_IR_DEREF_VAR:
            TRACE("deref(");
            debug_dump_ir_var(deref->v.var);
            TRACE(")");
            break;
        case HLSL_IR_DEREF_ARRAY:
            debug_dump_instr(deref->v.array.array);
            TRACE("[");
            debug_dump_instr(deref->v.array.index);
            TRACE("]");
            break;
        case HLSL_IR_DEREF_RECORD:
            debug_dump_instr(deref->v.record.record);
            TRACE(".%s", debugstr_a(deref->v.record.field->name));
            break;
    }
}

static void debug_dump_ir_constant(const struct hlsl_ir_constant *constant)
{
    struct hlsl_type *type = constant->node.data_type;
    unsigned int x, y;

    if (type->dimy != 1)
        TRACE("{");
    for (y = 0; y < type->dimy; ++y)
    {
        if (type->dimx != 1)
            TRACE("{");
        for (x = 0; x < type->dimx; ++x)
        {
            switch (type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    TRACE("%g ", (double)constant->v.value.f[y * type->dimx + x]);
                    break;
                case HLSL_TYPE_DOUBLE:
                    TRACE("%g ", constant->v.value.d[y * type->dimx + x]);
                    break;
                case HLSL_TYPE_INT:
                    TRACE("%d ", constant->v.value.i[y * type->dimx + x]);
                    break;
                case HLSL_TYPE_UINT:
                    TRACE("%u ", constant->v.value.u[y * type->dimx + x]);
                    break;
                case HLSL_TYPE_BOOL:
                    TRACE("%s ", constant->v.value.b[y * type->dimx + x] == FALSE ? "false" : "true");
                    break;
                default:
                    TRACE("Constants of type %s not supported\n", debug_base_type(type));
            }
        }
        if (type->dimx != 1)
            TRACE("}");
    }
    if (type->dimy != 1)
        TRACE("}");
}

static const char *debug_expr_op(const struct hlsl_ir_expr *expr)
{
    static const char * const op_names[] =
    {
        "~",
        "!",
        "-",
        "abs",
        "sign",
        "rcp",
        "rsq",
        "sqrt",
        "nrm",
        "exp2",
        "log2",

        "cast",

        "fract",

        "sin",
        "cos",
        "sin_reduced",
        "cos_reduced",

        "dsx",
        "dsy",

        "sat",

        "pre++",
        "pre--",
        "post++",
        "post--",

        "+",
        "-",
        "*",
        "/",

        "%",

        "<",
        ">",
        "<=",
        ">=",
        "==",
        "!=",

        "&&",
        "||",

        "<<",
        ">>",
        "&",
        "|",
        "^",

        "dot",
        "crs",
        "min",
        "max",

        "pow",

        "lerp",

        ",",
    };

    if (expr->op == HLSL_IR_UNOP_CAST)
        return debug_hlsl_type(expr->node.data_type);

    return op_names[expr->op];
}

/* Dumps the expression in a prefix "operator (operands)" form */
static void debug_dump_ir_expr(const struct hlsl_ir_expr *expr)
{
    unsigned int i;

    TRACE("%s (", debug_expr_op(expr));
    for (i = 0; i < 3 && expr->operands[i]; ++i)
    {
        debug_dump_instr(expr->operands[i]);
        TRACE(" ");
    }
    TRACE(")");
}

static void debug_dump_ir_constructor(const struct hlsl_ir_constructor *constructor)
{
    struct hlsl_ir_node *arg;

    TRACE("%s (", debug_hlsl_type(constructor->node.data_type));
    LIST_FOR_EACH_ENTRY(arg, constructor->arguments, struct hlsl_ir_node, entry)
    {
        debug_dump_instr(arg);
        TRACE(" ");
    }
    TRACE(")");
}

static const char *debug_writemask(DWORD writemask)
{
    char string[5], components[] = {'x', 'y', 'z', 'w'};
    unsigned int i = 0, pos = 0;

    while (writemask)
    {
        if (writemask & 1)
            string[pos++] = components[i];
        writemask >>= 1;
        i++;
    }
    string[pos] = '\0';
    return wine_dbg_sprintf(".%s", string);
}

static void debug_dump_ir_assignment(const struct hlsl_ir_assignment *assign)
{
    TRACE("= (");
    debug_dump_instr(assign->lhs);
    if (assign->writemask != BWRITERSP_WRITEMASK_ALL)
        TRACE("%s", debug_writemask(assign->writemask));
    TRACE(" ");
    debug_dump_instr(assign->rhs);
    TRACE(")");
}

static void debug_dump_ir_swizzle(const struct hlsl_ir_swizzle *swizzle)
{
    unsigned int i;

    debug_dump_instr(swizzle->val);
    TRACE(".");
    if (swizzle->val->data_type->dimy > 1)
    {
        for (i = 0; i < swizzle->node.data_type->dimx; ++i)
            TRACE("_m%u%u", (swizzle->swizzle >> i * 8) & 0xf, (swizzle->swizzle >> (i * 8 + 4)) & 0xf);
    }
    else
    {
        char c[] = {'x', 'y', 'z', 'w'};

        for (i = 0; i < swizzle->node.data_type->dimx; ++i)
            TRACE("%c", c[(swizzle->swizzle >> i * 2) & 0x3]);
    }
}

static void debug_dump_ir_jump(const struct hlsl_ir_jump *jump)
{
    switch (jump->type)
    {
        case HLSL_IR_JUMP_BREAK:
            TRACE("break");
            break;
        case HLSL_IR_JUMP_CONTINUE:
            TRACE("continue");
            break;
        case HLSL_IR_JUMP_DISCARD:
            TRACE("discard");
            break;
        case HLSL_IR_JUMP_RETURN:
            TRACE("return ");
            if (jump->return_value)
                debug_dump_instr(jump->return_value);
            TRACE(";");
            break;
    }
}

static void debug_dump_ir_if(const struct hlsl_ir_if *if_node)
{
    TRACE("if (");
    debug_dump_instr(if_node->condition);
    TRACE(")\n{\n");
    debug_dump_instr_list(if_node->then_instrs);
    TRACE("}\n");
    if (if_node->else_instrs)
    {
        TRACE("else\n{\n");
        debug_dump_instr_list(if_node->else_instrs);
        TRACE("}\n");
    }
}

static void debug_dump_instr(const struct hlsl_ir_node *instr)
{
    switch (instr->type)
    {
        case HLSL_IR_EXPR:
            debug_dump_ir_expr(expr_from_node(instr));
            break;
        case HLSL_IR_DEREF:
            debug_dump_ir_deref(deref_from_node(instr));
            break;
        case HLSL_IR_CONSTANT:
            debug_dump_ir_constant(constant_from_node(instr));
            break;
        case HLSL_IR_ASSIGNMENT:
            debug_dump_ir_assignment(assignment_from_node(instr));
            break;
        case HLSL_IR_SWIZZLE:
            debug_dump_ir_swizzle(swizzle_from_node(instr));
            break;
        case HLSL_IR_CONSTRUCTOR:
            debug_dump_ir_constructor(constructor_from_node(instr));
            break;
        case HLSL_IR_JUMP:
            debug_dump_ir_jump(jump_from_node(instr));
            break;
        case HLSL_IR_IF:
            debug_dump_ir_if(if_from_node(instr));
            break;
        default:
            TRACE("<No dump function for %s>", debug_node_type(instr->type));
    }
}

void debug_dump_ir_function_decl(const struct hlsl_ir_function_decl *func)
{
    struct hlsl_ir_var *param;

    TRACE("Dumping function %s.\n", debugstr_a(func->func->name));
    TRACE("Function parameters:\n");
    LIST_FOR_EACH_ENTRY(param, func->parameters, struct hlsl_ir_var, node.entry)
    {
        debug_dump_ir_var(param);
        TRACE("\n");
    }
    if (func->semantic)
        TRACE("Function semantic: %s\n", debugstr_a(func->semantic));
    if (func->body)
    {
        debug_dump_instr_list(func->body);
    }
}

void free_hlsl_type(struct hlsl_type *type)
{
    struct hlsl_struct_field *field, *next_field;

    d3dcompiler_free((void *)type->name);
    if (type->type == HLSL_CLASS_STRUCT)
    {
        LIST_FOR_EACH_ENTRY_SAFE(field, next_field, type->e.elements, struct hlsl_struct_field, entry)
        {
            d3dcompiler_free((void *)field->name);
            d3dcompiler_free((void *)field->semantic);
            d3dcompiler_free(field);
        }
    }
    d3dcompiler_free(type);
}

void free_instr_list(struct list *list)
{
    struct hlsl_ir_node *node, *next_node;

    if (!list)
        return;
    LIST_FOR_EACH_ENTRY_SAFE(node, next_node, list, struct hlsl_ir_node, entry)
        free_instr(node);
    d3dcompiler_free(list);
}

static void free_ir_constant(struct hlsl_ir_constant *constant)
{
    struct hlsl_type *type = constant->node.data_type;
    unsigned int i;
    struct hlsl_ir_constant *field, *next_field;

    switch (type->type)
    {
        case HLSL_CLASS_ARRAY:
            for (i = 0; i < type->e.array.elements_count; ++i)
                free_ir_constant(&constant->v.array_elements[i]);
            d3dcompiler_free(constant->v.array_elements);
            break;
        case HLSL_CLASS_STRUCT:
            LIST_FOR_EACH_ENTRY_SAFE(field, next_field, constant->v.struct_elements, struct hlsl_ir_constant, node.entry)
                free_ir_constant(field);
            break;
        default:
            break;
    }
    d3dcompiler_free(constant);
}

static void free_ir_deref(struct hlsl_ir_deref *deref)
{
    switch (deref->type)
    {
        case HLSL_IR_DEREF_VAR:
            /* Variables are shared among nodes in the tree. */
            break;
        case HLSL_IR_DEREF_ARRAY:
            free_instr(deref->v.array.array);
            free_instr(deref->v.array.index);
            break;
        case HLSL_IR_DEREF_RECORD:
            free_instr(deref->v.record.record);
            break;
    }
    d3dcompiler_free(deref);
}

static void free_ir_swizzle(struct hlsl_ir_swizzle *swizzle)
{
    free_instr(swizzle->val);
    d3dcompiler_free(swizzle);
}

static void free_ir_constructor(struct hlsl_ir_constructor *constructor)
{
    free_instr_list(constructor->arguments);
    d3dcompiler_free(constructor);
}

static void free_ir_expr(struct hlsl_ir_expr *expr)
{
    unsigned int i;

    for (i = 0; i < 3; ++i)
    {
        if (!expr->operands[i])
            break;
        free_instr(expr->operands[i]);
    }
    free_instr_list(expr->subexpressions);
    d3dcompiler_free(expr);
}

static void free_ir_assignment(struct hlsl_ir_assignment *assignment)
{
    free_instr(assignment->lhs);
    free_instr(assignment->rhs);
    d3dcompiler_free(assignment);
}

static void free_ir_if(struct hlsl_ir_if *if_node)
{
    free_instr(if_node->condition);
    free_instr_list(if_node->then_instrs);
    free_instr_list(if_node->else_instrs);
    d3dcompiler_free(if_node);
}

static void free_ir_jump(struct hlsl_ir_jump *jump)
{
    if (jump->type == HLSL_IR_JUMP_RETURN)
        free_instr(jump->return_value);
    d3dcompiler_free(jump);
}

void free_instr(struct hlsl_ir_node *node)
{
    switch (node->type)
    {
        case HLSL_IR_VAR:
            /* These are freed later on from the scopes. */
            break;
        case HLSL_IR_CONSTANT:
            free_ir_constant(constant_from_node(node));
            break;
        case HLSL_IR_DEREF:
            free_ir_deref(deref_from_node(node));
            break;
        case HLSL_IR_SWIZZLE:
            free_ir_swizzle(swizzle_from_node(node));
            break;
        case HLSL_IR_CONSTRUCTOR:
            free_ir_constructor(constructor_from_node(node));
            break;
        case HLSL_IR_EXPR:
            free_ir_expr(expr_from_node(node));
            break;
        case HLSL_IR_ASSIGNMENT:
            free_ir_assignment(assignment_from_node(node));
            break;
        case HLSL_IR_IF:
            free_ir_if(if_from_node(node));
            break;
        case HLSL_IR_JUMP:
            free_ir_jump(jump_from_node(node));
            break;
        default:
            FIXME("Unsupported node type %s\n", debug_node_type(node->type));
    }
}

static void free_function_decl(struct hlsl_ir_function_decl *decl)
{
    d3dcompiler_free((void *)decl->semantic);
    d3dcompiler_free(decl->parameters);
    free_instr_list(decl->body);
    d3dcompiler_free(decl);
}

static void free_function_decl_rb(struct wine_rb_entry *entry, void *context)
{
    free_function_decl(WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function_decl, entry));
}

static void free_function(struct hlsl_ir_function *func)
{
    wine_rb_destroy(&func->overloads, free_function_decl_rb, NULL);
    d3dcompiler_free((void *)func->name);
    d3dcompiler_free(func);
}

void free_function_rb(struct wine_rb_entry *entry, void *context)
{
    free_function(WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry));
}

void add_function_decl(struct wine_rb_tree *funcs, char *name, struct hlsl_ir_function_decl *decl, BOOL intrinsic)
{
    struct hlsl_ir_function *func;
    struct wine_rb_entry *func_entry, *old_entry;

    func_entry = wine_rb_get(funcs, name);
    if (func_entry)
    {
        func = WINE_RB_ENTRY_VALUE(func_entry, struct hlsl_ir_function, entry);
        if (intrinsic != func->intrinsic)
        {
            if (intrinsic)
            {
                ERR("Redeclaring a user defined function as an intrinsic.\n");
                return;
            }
            TRACE("Function %s redeclared as a user defined function.\n", debugstr_a(name));
            func->intrinsic = intrinsic;
            wine_rb_destroy(&func->overloads, free_function_decl_rb, NULL);
            if (wine_rb_init(&func->overloads, &hlsl_ir_function_decl_rb_funcs) == -1)
            {
                ERR("Failed to initialize function rbtree.\n");
                return;
            }
        }
        decl->func = func;
        if ((old_entry = wine_rb_get(&func->overloads, decl->parameters)))
        {
            struct hlsl_ir_function_decl *old_decl =
                    WINE_RB_ENTRY_VALUE(old_entry, struct hlsl_ir_function_decl, entry);

            if (!decl->body)
            {
                free_function_decl(decl);
                d3dcompiler_free(name);
                return;
            }
            wine_rb_remove(&func->overloads, decl->parameters);
            free_function_decl(old_decl);
        }
        wine_rb_put(&func->overloads, decl->parameters, &decl->entry);
        d3dcompiler_free(name);
        return;
    }
    func = d3dcompiler_alloc(sizeof(*func));
    func->name = name;
    if (wine_rb_init(&func->overloads, &hlsl_ir_function_decl_rb_funcs) == -1)
    {
        ERR("Failed to initialize function rbtree.\n");
        d3dcompiler_free(name);
        d3dcompiler_free(func);
        return;
    }
    decl->func = func;
    wine_rb_put(&func->overloads, decl->parameters, &decl->entry);
    func->intrinsic = intrinsic;
    wine_rb_put(funcs, func->name, &func->entry);
}
