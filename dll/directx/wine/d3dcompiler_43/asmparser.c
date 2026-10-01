/*
 * Direct3D asm shader parser
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
 *
 */

#include "wine/debug.h"

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(asmshader);
WINE_DECLARE_DEBUG_CHANNEL(parsed_shader);


/* How to map vs 1.0 and 2.0 varyings to 3.0 ones
 * oTx is mapped to ox, which happens to be an
 * identical mapping since BWRITERSPR_TEXCRDOUT == BWRITERSPR_OUTPUT
 * oPos, oFog and point size are mapped to general output regs as well.
 * the vs 1.x and 2.x parser functions add varying declarations
 * to the shader, and the 1.x and 2.x output functions check those varyings
 */
#define OT0_REG         0
#define OT1_REG         1
#define OT2_REG         2
#define OT3_REG         3
#define OT4_REG         4
#define OT5_REG         5
#define OT6_REG         6
#define OT7_REG         7
#define OPOS_REG        8
#define OFOG_REG        9
#define OFOG_WRITEMASK  BWRITERSP_WRITEMASK_0
#define OPTS_REG        9
#define OPTS_WRITEMASK  BWRITERSP_WRITEMASK_1
#define OD0_REG         10
#define OD1_REG         11

/* Input color registers 0-1 are identically mapped */
#define C0_VARYING      0
#define C1_VARYING      1
#define T0_VARYING      2
#define T1_VARYING      3
#define T2_VARYING      4
#define T3_VARYING      5
#define T4_VARYING      6
#define T5_VARYING      7
#define T6_VARYING      8
#define T7_VARYING      9

/****************************************************************
 * Common(non-version specific) shader parser control code      *
 ****************************************************************/

static void asmparser_end(struct asm_parser *This) {
    TRACE("Finalizing shader\n");
}

static void asmparser_constF(struct asm_parser *parser, uint32_t reg, float x, float y, float z, float w)
{
    if (!parser->shader)
        return;
    TRACE("Adding float constant %u at pos %u.\n", reg, parser->shader->num_cf);
    TRACE_(parsed_shader)("def c%u, %f, %f, %f, %f\n", reg, x, y, z, w);
    if (!add_constF(parser->shader, reg, x, y, z, w))
    {
        ERR("Out of memory.\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_constB(struct asm_parser *parser, uint32_t reg, BOOL x)
{
    if (!parser->shader)
        return;
    TRACE("Adding boolean constant %u at pos %u.\n", reg, parser->shader->num_cb);
    TRACE_(parsed_shader)("def b%u, %s\n", reg, x ? "true" : "false");
    if (!add_constB(parser->shader, reg, x))
    {
        ERR("Out of memory.\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_constI(struct asm_parser *parser, uint32_t reg, int x, int y, int z, int w)
{
    if (!parser->shader)
        return;
    TRACE("Adding integer constant %u at pos %u.\n", reg, parser->shader->num_ci);
    TRACE_(parsed_shader)("def i%u, %d, %d, %d, %d\n", reg, x, y, z, w);
    if (!add_constI(parser->shader, reg, x, y, z, w))
    {
        ERR("Out of memory.\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_dcl_output(struct asm_parser *parser, uint32_t usage, uint32_t num,
        const struct shader_reg *reg)
{
    if (!parser->shader)
        return;
    if (parser->shader->type == ST_PIXEL)
    {
        asmparser_message(parser, "Line %u: Output register declared in a pixel shader\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
    }
    if (!record_declaration(parser->shader, usage, num, 0, TRUE, reg->regnum, reg->writemask, FALSE))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_dcl_output_unsupported(struct asm_parser *parser, uint32_t usage, uint32_t num,
        const struct shader_reg *reg)
{
    asmparser_message(parser, "Line %u: Output declaration unsupported in this shader version\n", parser->line_no);
    set_parse_status(&parser->status, PARSE_ERR);
}

static void asmparser_dcl_input(struct asm_parser *parser, uint32_t usage, uint32_t num, uint32_t mod,
        const struct shader_reg *reg)
{
    struct instruction instr;

    if (!parser->shader)
        return;
    if (mod && (parser->shader->type != ST_PIXEL || parser->shader->major_version != 3
            || (mod != BWRITERSPDM_MSAMPCENTROID && mod != BWRITERSPDM_PARTIALPRECISION)))
    {
        asmparser_message(parser, "Line %u: Unsupported modifier in dcl instruction\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    /* Check register type and modifiers */
    instr.dstmod = mod;
    instr.shift = 0;
    parser->funcs->dstreg(parser, &instr, reg);

    if (!record_declaration(parser->shader, usage, num, mod, FALSE, reg->regnum, reg->writemask, FALSE))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_dcl_input_ps_2(struct asm_parser *parser, uint32_t usage, uint32_t num, uint32_t mod,
        const struct shader_reg *reg)
{
    struct instruction instr;

    if (!parser->shader)
        return;
    instr.dstmod = mod;
    instr.shift = 0;
    parser->funcs->dstreg(parser, &instr, reg);
    if (!record_declaration(parser->shader, usage, num, mod, FALSE, instr.dst.regnum, instr.dst.writemask, FALSE))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_dcl_input_unsupported(struct asm_parser *parser, uint32_t usage, uint32_t num, uint32_t mod,
        const struct shader_reg *reg)
{
    asmparser_message(parser, "Line %u: Input declaration unsupported in this shader version\n", parser->line_no);
    set_parse_status(&parser->status, PARSE_ERR);
}

static void asmparser_dcl_sampler(struct asm_parser *parser, uint32_t samptype, uint32_t mod, uint32_t regnum,
        unsigned int line_no)
{
    if (!parser->shader)
        return;
    if (mod && (parser->shader->type != ST_PIXEL || parser->shader->major_version != 3
            || (mod != BWRITERSPDM_MSAMPCENTROID && mod != BWRITERSPDM_PARTIALPRECISION)))
    {
        asmparser_message(parser, "Line %u: Unsupported modifier in dcl instruction\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }
    if (!record_sampler(parser->shader, samptype, mod, regnum))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_dcl_sampler_unsupported(struct asm_parser *parser, uint32_t samptype, uint32_t mod,
        uint32_t regnum, unsigned int line_no)
{
    asmparser_message(parser, "Line %u: Sampler declaration unsupported in this shader version\n", parser->line_no);
    set_parse_status(&parser->status, PARSE_ERR);
}

static void asmparser_sincos(struct asm_parser *parser, uint32_t mod, uint32_t shift, const struct shader_reg *dst,
        const struct src_regs *srcs)
{
    struct instruction *instr;

    if (!srcs || srcs->count != 3)
    {
        asmparser_message(parser, "Line %u: sincos (vs 2) has an incorrect number of source registers\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    instr = alloc_instr(3);
    if (!instr)
    {
        ERR("Error allocating memory for the instruction\n");
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    instr->opcode = BWRITERSIO_SINCOS;
    instr->dstmod = mod;
    instr->shift = shift;
    instr->comptype = 0;

    parser->funcs->dstreg(parser, instr, dst);
    parser->funcs->srcreg(parser, instr, 0, &srcs->reg[0]);
    parser->funcs->srcreg(parser, instr, 1, &srcs->reg[1]);
    parser->funcs->srcreg(parser, instr, 2, &srcs->reg[2]);

    if (!add_instruction(parser->shader, instr))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static struct shader_reg map_oldps_register(const struct shader_reg *reg, BOOL tex_varying)
{
    struct shader_reg ret;

    switch (reg->type)
    {
        case BWRITERSPR_TEXTURE:
            if (tex_varying)
            {
                ret = *reg;
                ret.type = BWRITERSPR_INPUT;
                switch (reg->regnum)
                {
                    case 0:     ret.regnum = T0_VARYING; break;
                    case 1:     ret.regnum = T1_VARYING; break;
                    case 2:     ret.regnum = T2_VARYING; break;
                    case 3:     ret.regnum = T3_VARYING; break;
                    case 4:     ret.regnum = T4_VARYING; break;
                    case 5:     ret.regnum = T5_VARYING; break;
                    case 6:     ret.regnum = T6_VARYING; break;
                    case 7:     ret.regnum = T7_VARYING; break;
                    default:
                        FIXME("Unexpected TEXTURE register t%u.\n", reg->regnum);
                        return *reg;
                }
                return ret;
            }
            else
            {
                ret = *reg;
                ret.type = BWRITERSPR_TEMP;
                switch(reg->regnum)
                {
                    case 0:     ret.regnum = T0_REG; break;
                    case 1:     ret.regnum = T1_REG; break;
                    case 2:     ret.regnum = T2_REG; break;
                    case 3:     ret.regnum = T3_REG; break;
                    default:
                        FIXME("Unexpected TEXTURE register t%u.\n", reg->regnum);
                        return *reg;
                }
                return ret;
            }

        /* case BWRITERSPR_INPUT - Identical mapping of 1.x/2.0 color varyings
           to 3.0 ones */

        default: return *reg;
    }
}

static void asmparser_texcoord(struct asm_parser *parser, uint32_t mod, uint32_t shift, const struct shader_reg *dst,
        const struct src_regs *srcs)
{
    struct instruction *instr;

    if (srcs)
    {
        asmparser_message(parser, "Line %u: Source registers in texcoord instruction\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    instr = alloc_instr(1);
    if (!instr)
    {
        ERR("Error allocating memory for the instruction\n");
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    /* texcoord copies the texture coord data into a temporary register-like
     * readable form. In newer shader models this equals a MOV from v0 to r0,
     * record it as this.
     */
    instr->opcode = BWRITERSIO_MOV;
    instr->dstmod = mod | BWRITERSPDM_SATURATE; /* texcoord clamps to [0;1] */
    instr->shift = shift;
    instr->comptype = 0;

    parser->funcs->dstreg(parser, instr, dst);
    /* The src reg needs special care */
    instr->src[0] = map_oldps_register(dst, TRUE);

    if (!add_instruction(parser->shader, instr))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_texcrd(struct asm_parser *parser, uint32_t mod, uint32_t shift, const struct shader_reg *dst,
        const struct src_regs *srcs)
{
    struct instruction *instr;

    if (!srcs || srcs->count != 1)
    {
        asmparser_message(parser, "Line %u: Wrong number of source registers in texcrd instruction\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    instr = alloc_instr(1);
    if (!instr)
    {
        ERR("Error allocating memory for the instruction\n");
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    /* The job of texcrd is done by mov in later shader versions */
    instr->opcode = BWRITERSIO_MOV;
    instr->dstmod = mod;
    instr->shift = shift;
    instr->comptype = 0;

    parser->funcs->dstreg(parser, instr, dst);
    parser->funcs->srcreg(parser, instr, 0, &srcs->reg[0]);

    if (!add_instruction(parser->shader, instr))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_texkill(struct asm_parser *parser, const struct shader_reg *dst)
{
    struct instruction *instr = alloc_instr(0);

    if (!instr)
    {
        ERR("Error allocating memory for the instruction\n");
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    instr->opcode = BWRITERSIO_TEXKILL;
    instr->dstmod = 0;
    instr->shift = 0;
    instr->comptype = 0;

    /* Do not run the dst register through the normal
     * register conversion. If used with ps_1_0 to ps_1_3
     * the texture coordinate from that register is used,
     * not the temporary register value. In ps_1_4 and
     * ps_2_0 t0 is always a varying and temporaries can
     * be used with texkill.
     */
    instr->dst = map_oldps_register(dst, TRUE);
    instr->has_dst = TRUE;

    if (!add_instruction(parser->shader, instr))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_texhelper(struct asm_parser *parser, uint32_t mod, uint32_t shift, const struct shader_reg *dst,
        const struct shader_reg *src0)
{
    struct instruction *instr = alloc_instr(2);

    if (!instr)
    {
        ERR("Error allocating memory for the instruction\n");
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    instr->opcode = BWRITERSIO_TEX;
    instr->dstmod = mod;
    instr->shift = shift;
    instr->comptype = 0;
    /* The dest register can be mapped normally to a temporary register */
    parser->funcs->dstreg(parser, instr, dst);
    /* Use the src passed as parameter by the specific instruction handler */
    instr->src[0] = *src0;

    /* The 2nd source register is the sampler register with the
     * destination's regnum
     */
    ZeroMemory(&instr->src[1], sizeof(instr->src[1]));
    instr->src[1].type = BWRITERSPR_SAMPLER;
    instr->src[1].regnum = dst->regnum;
    instr->src[1].swizzle = BWRITERVS_NOSWIZZLE;
    instr->src[1].srcmod = BWRITERSPSM_NONE;
    instr->src[1].rel_reg = NULL;

    if (!add_instruction(parser->shader, instr))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_tex(struct asm_parser *parser, uint32_t mod, uint32_t shift, const struct shader_reg *dst)
{
    struct shader_reg src;

    /* The first source register is the varying containing the coordinate */
    src = map_oldps_register(dst, TRUE);
    asmparser_texhelper(parser, mod, shift, dst, &src);
}

static void asmparser_texld14(struct asm_parser *parser, uint32_t mod, uint32_t shift, const struct shader_reg *dst,
        const struct src_regs *srcs)
{
    struct instruction *instr;

    if (!srcs || srcs->count != 1)
    {
        asmparser_message(parser, "Line %u: texld (PS 1.4) has a wrong number of source registers\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    instr = alloc_instr(2);
    if (!instr)
    {
        ERR("Error allocating memory for the instruction\n");
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    /* This code is recording a texld instruction, not tex. However,
     * texld borrows the opcode of tex
     */
    instr->opcode = BWRITERSIO_TEX;
    instr->dstmod = mod;
    instr->shift = shift;
    instr->comptype = 0;

    parser->funcs->dstreg(parser, instr, dst);
    parser->funcs->srcreg(parser, instr, 0, &srcs->reg[0]);

    /* The 2nd source register is the sampler register with the
     * destination's regnum
     */
    ZeroMemory(&instr->src[1], sizeof(instr->src[1]));
    instr->src[1].type = BWRITERSPR_SAMPLER;
    instr->src[1].regnum = dst->regnum;
    instr->src[1].swizzle = BWRITERVS_NOSWIZZLE;
    instr->src[1].srcmod = BWRITERSPSM_NONE;
    instr->src[1].rel_reg = NULL;

    if (!add_instruction(parser->shader, instr))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void asmparser_texreg2ar(struct asm_parser *parser, uint32_t mod, uint32_t shift, const struct shader_reg *dst,
        const struct shader_reg *src0)
{
    struct shader_reg src;

    src = map_oldps_register(src0, FALSE);
    /* Supply the correct swizzle */
    src.swizzle = BWRITERVS_X_W | BWRITERVS_Y_X | BWRITERVS_Z_X | BWRITERVS_W_X;
    asmparser_texhelper(parser, mod, shift, dst, &src);
}

static void asmparser_texreg2gb(struct asm_parser *parser, uint32_t mod, uint32_t shift, const struct shader_reg *dst,
        const struct shader_reg *src0)
{
    struct shader_reg src;

    src = map_oldps_register(src0, FALSE);
    /* Supply the correct swizzle */
    src.swizzle = BWRITERVS_X_Y | BWRITERVS_Y_Z | BWRITERVS_Z_Z | BWRITERVS_W_Z;
    asmparser_texhelper(parser, mod, shift, dst, &src);
}

static void asmparser_texreg2rgb(struct asm_parser *parser, uint32_t mod, uint32_t shift, const struct shader_reg *dst,
        const struct shader_reg *src0)
{
    struct shader_reg src;

    src = map_oldps_register(src0, FALSE);
    /* Supply the correct swizzle */
    src.swizzle = BWRITERVS_X_X | BWRITERVS_Y_Y | BWRITERVS_Z_Z | BWRITERVS_W_Z;
    asmparser_texhelper(parser, mod, shift, dst, &src);
}

/* Complex pixel shader 1.3 instructions like texm3x3tex are tricky - the
 * bytecode writer works instruction by instruction, so we can't properly
 * convert these from/to equivalent ps_3_0 instructions. Then simply keep using
 * the ps_1_3 opcodes and just adapt the registers in the common fashion (i.e.
 * go through asmparser_instr).
 */

static void asmparser_instr(struct asm_parser *parser, uint32_t opcode, uint32_t mod, uint32_t shift,
        enum bwriter_comparison_type comp, const struct shader_reg *dst,
        const struct src_regs *srcs, int expectednsrcs)
{
    unsigned int src_count = srcs ? srcs->count : 0;
    struct bwriter_shader *shader = parser->shader;
    struct instruction *instr;
    BOOL firstreg = TRUE;
    unsigned int i;

    if (!parser->shader)
        return;

    TRACE_(parsed_shader)("%s%s%s%s ", debug_print_opcode(opcode),
                          debug_print_dstmod(mod),
                          debug_print_shift(shift),
                          debug_print_comp(comp));
    if (dst)
    {
        TRACE_(parsed_shader)("%s", debug_print_dstreg(dst));
        firstreg = FALSE;
    }
    for (i = 0; i < src_count; i++)
    {
        if (!firstreg)
            TRACE_(parsed_shader)(", ");
        else firstreg = FALSE;
        TRACE_(parsed_shader)("%s", debug_print_srcreg(&srcs->reg[i]));
    }
    TRACE_(parsed_shader)("\n");

 /* Check for instructions with different syntaxes in different shader versions */
    switch(opcode)
    {
        case BWRITERSIO_SINCOS:
            /* The syntax changes between vs 2 and the other shader versions */
            if (parser->shader->type == ST_VERTEX && parser->shader->major_version == 2)
            {
                asmparser_sincos(parser, mod, shift, dst, srcs);
                return;
            }
            /* Use the default handling */
            break;
        case BWRITERSIO_TEXCOORD:
            /* texcoord/texcrd are two instructions present only in PS <= 1.3 and PS 1.4 respectively */
            if (shader->type == ST_PIXEL && shader->major_version == 1 && shader->minor_version == 4)
                asmparser_texcrd(parser, mod, shift, dst, srcs);
            else
                asmparser_texcoord(parser, mod, shift, dst, srcs);
            return;
        case BWRITERSIO_TEX:
            /* this encodes both the tex PS 1.x instruction and the
               texld 1.4/2.0+ instruction */
            if (shader->type == ST_PIXEL && shader->major_version == 1)
            {
                if (shader->minor_version < 4)
                    asmparser_tex(parser, mod, shift, dst);
                else
                    asmparser_texld14(parser, mod, shift, dst, srcs);
                return;
            }
            /* else fallback to the standard behavior */
            break;
    }

    if (src_count != expectednsrcs)
    {
        asmparser_message(parser, "Line %u: Wrong number of source registers\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    /* Handle PS 1.x instructions, "regularizing" them */
    switch(opcode)
    {
        case BWRITERSIO_TEXKILL:
            asmparser_texkill(parser, dst);
            return;
        case BWRITERSIO_TEXREG2AR:
            asmparser_texreg2ar(parser, mod, shift, dst, &srcs->reg[0]);
            return;
        case BWRITERSIO_TEXREG2GB:
            asmparser_texreg2gb(parser, mod, shift, dst, &srcs->reg[0]);
            return;
        case BWRITERSIO_TEXREG2RGB:
            asmparser_texreg2rgb(parser, mod, shift, dst, &srcs->reg[0]);
            return;
    }

    instr = alloc_instr(src_count);
    if (!instr)
    {
        ERR("Error allocating memory for the instruction\n");
        set_parse_status(&parser->status, PARSE_ERR);
        return;
    }

    instr->opcode = opcode;
    instr->dstmod = mod;
    instr->shift = shift;
    instr->comptype = comp;
    if (dst)
        parser->funcs->dstreg(parser, instr, dst);
    for (i = 0; i < src_count; i++)
    {
        parser->funcs->srcreg(parser, instr, i, &srcs->reg[i]);
    }

    if (!add_instruction(parser->shader, instr))
    {
        ERR("Out of memory\n");
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static struct shader_reg map_oldvs_register(const struct shader_reg *reg)
{
    struct shader_reg ret;

    switch(reg->type)
    {
        case BWRITERSPR_RASTOUT:
            ret = *reg;
            ret.type = BWRITERSPR_OUTPUT;
            switch(reg->regnum)
            {
                case BWRITERSRO_POSITION:
                    ret.regnum = OPOS_REG;
                    break;
                case BWRITERSRO_FOG:
                    ret.regnum = OFOG_REG;
                    ret.writemask = OFOG_WRITEMASK;
                    break;
                case BWRITERSRO_POINT_SIZE:
                    ret.regnum = OPTS_REG;
                    ret.writemask = OPTS_WRITEMASK;
                    break;
                default:
                    FIXME("Unhandled RASTOUT register %u.\n", reg->regnum);
                    return *reg;
            }
            return ret;

        case BWRITERSPR_TEXCRDOUT:
            ret = *reg;
            ret.type = BWRITERSPR_OUTPUT;
            switch(reg->regnum)
            {
                case 0: ret.regnum = OT0_REG; break;
                case 1: ret.regnum = OT1_REG; break;
                case 2: ret.regnum = OT2_REG; break;
                case 3: ret.regnum = OT3_REG; break;
                case 4: ret.regnum = OT4_REG; break;
                case 5: ret.regnum = OT5_REG; break;
                case 6: ret.regnum = OT6_REG; break;
                case 7: ret.regnum = OT7_REG; break;
                default:
                    FIXME("Unhandled TEXCRDOUT regnum %u.\n", reg->regnum);
                    return *reg;
            }
            return ret;

        case BWRITERSPR_ATTROUT:
            ret = *reg;
            ret.type = BWRITERSPR_OUTPUT;
            switch(reg->regnum)
            {
                case 0: ret.regnum = OD0_REG; break;
                case 1: ret.regnum = OD1_REG; break;
                default:
                    FIXME("Unhandled ATTROUT regnum %u.\n", reg->regnum);
                    return *reg;
            }
            return ret;

        default: return *reg;
    }
}

/* Checks for unsupported source modifiers in VS (all versions) or
   PS 2.0 and newer */
static void check_legacy_srcmod(struct asm_parser *parser, uint32_t srcmod)
{
    if (srcmod == BWRITERSPSM_BIAS || srcmod == BWRITERSPSM_BIASNEG ||
            srcmod == BWRITERSPSM_SIGN || srcmod == BWRITERSPSM_SIGNNEG ||
            srcmod == BWRITERSPSM_COMP || srcmod == BWRITERSPSM_X2 ||
            srcmod == BWRITERSPSM_X2NEG || srcmod == BWRITERSPSM_DZ ||
            srcmod == BWRITERSPSM_DW)
    {
        asmparser_message(parser, "Line %u: Source modifier %s not supported in this shader version\n",
                parser->line_no, debug_print_srcmod(srcmod));
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void check_abs_srcmod(struct asm_parser *parser, uint32_t srcmod)
{
    if (srcmod == BWRITERSPSM_ABS || srcmod == BWRITERSPSM_ABSNEG)
    {
        asmparser_message(parser, "Line %u: Source modifier %s not supported in this shader version\n",
                parser->line_no, debug_print_srcmod(srcmod));
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void check_loop_swizzle(struct asm_parser *parser, const struct shader_reg *src)
{
    if ((src->type == BWRITERSPR_LOOP && src->swizzle != BWRITERVS_NOSWIZZLE)
            || (src->rel_reg && src->rel_reg->type == BWRITERSPR_LOOP &&
            src->rel_reg->swizzle != BWRITERVS_NOSWIZZLE))
    {
        asmparser_message(parser, "Line %u: Swizzle not allowed on aL register\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void check_shift_dstmod(struct asm_parser *parser, uint32_t shift)
{
    if (shift != 0)
    {
        asmparser_message(parser, "Line %u: Shift modifiers not supported in this shader version\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

static void check_ps_dstmod(struct asm_parser *parser, uint32_t dstmod)
{
    if(dstmod == BWRITERSPDM_PARTIALPRECISION || dstmod == BWRITERSPDM_MSAMPCENTROID)
    {
        asmparser_message(parser, "Line %u: Instruction modifier %s not supported in this shader version\n",
                parser->line_no, debug_print_dstmod(dstmod));
        set_parse_status(&parser->status, PARSE_ERR);
    }
}

struct allowed_reg_type
{
    uint32_t type;
    unsigned int count;
    BOOL reladdr;
};

static BOOL check_reg_type(const struct shader_reg *reg, const struct allowed_reg_type *allowed)
{
    unsigned int i = 0;

    while (allowed[i].type != ~0u)
    {
        if (reg->type == allowed[i].type)
        {
            if (reg->rel_reg)
            {
                /* The relative addressing register can have a negative value,
                 * we can't check the register index. */
                if (allowed[i].reladdr)
                    return TRUE;
                return FALSE;
            }
            if (reg->regnum < allowed[i].count)
                return TRUE;
            return FALSE;
        }
        i++;
    }
    return FALSE;
}

/* Native assembler doesn't do separate checks for src and dst registers */
static const struct allowed_reg_type vs_1_reg_allowed[] =
{
    { BWRITERSPR_TEMP,         12,  FALSE },
    { BWRITERSPR_INPUT,        16,  FALSE },
    { BWRITERSPR_CONST,       ~0u,   TRUE },
    { BWRITERSPR_ADDR,          1,  FALSE },
    { BWRITERSPR_RASTOUT,       3,  FALSE }, /* oPos, oFog and oPts */
    { BWRITERSPR_ATTROUT,       2,  FALSE },
    { BWRITERSPR_TEXCRDOUT,     8,  FALSE },
    { ~0u, 0 } /* End tag */
};

/* struct instruction *asmparser_srcreg
 *
 * Records a source register in the instruction and does shader version
 * specific checks and modifications on it
 *
 * Parameters:
 *  This: Shader parser instance
 *  instr: instruction to store the register in
 *  num: Number of source register
 *  src: Pointer to source the register structure. The caller can free
 *  it afterwards
 */
static void asmparser_srcreg_vs_1(struct asm_parser *parser, struct instruction *instr, int num,
        const struct shader_reg *src)
{
    struct shader_reg reg;

    if (!check_reg_type(src, vs_1_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Source register %s not supported in VS 1\n",
                parser->line_no, debug_print_srcreg(src));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_legacy_srcmod(parser, src->srcmod);
    check_abs_srcmod(parser, src->srcmod);
    reg = map_oldvs_register(src);
    instr->src[num] = reg;
}

static const struct allowed_reg_type vs_2_reg_allowed[] =
{
    { BWRITERSPR_TEMP,      12,  FALSE },
    { BWRITERSPR_INPUT,     16,  FALSE },
    { BWRITERSPR_CONST,    ~0u,   TRUE },
    { BWRITERSPR_ADDR,       1,  FALSE },
    { BWRITERSPR_CONSTBOOL, 16,  FALSE },
    { BWRITERSPR_CONSTINT,  16,  FALSE },
    { BWRITERSPR_LOOP,       1,  FALSE },
    { BWRITERSPR_LABEL,   2048,  FALSE },
    { BWRITERSPR_PREDICATE,  1,  FALSE },
    { BWRITERSPR_RASTOUT,    3,  FALSE }, /* oPos, oFog and oPts */
    { BWRITERSPR_ATTROUT,    2,  FALSE },
    { BWRITERSPR_TEXCRDOUT,  8,  FALSE },
    { ~0u, 0 } /* End tag */
};

static void asmparser_srcreg_vs_2(struct asm_parser *parser, struct instruction *instr, int num,
        const struct shader_reg *src)
{
    struct shader_reg reg;

    if (!check_reg_type(src, vs_2_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Source register %s not supported in VS 2\n",
                parser->line_no, debug_print_srcreg(src));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_loop_swizzle(parser, src);
    check_legacy_srcmod(parser, src->srcmod);
    check_abs_srcmod(parser, src->srcmod);
    reg = map_oldvs_register(src);
    instr->src[num] = reg;
}

static const struct allowed_reg_type vs_3_reg_allowed[] =
{
    { BWRITERSPR_TEMP,         32,  FALSE },
    { BWRITERSPR_INPUT,        16,   TRUE },
    { BWRITERSPR_CONST,       ~0u,   TRUE },
    { BWRITERSPR_ADDR,          1,  FALSE },
    { BWRITERSPR_CONSTBOOL,    16,  FALSE },
    { BWRITERSPR_CONSTINT,     16,  FALSE },
    { BWRITERSPR_LOOP,          1,  FALSE },
    { BWRITERSPR_LABEL,      2048,  FALSE },
    { BWRITERSPR_PREDICATE,     1,  FALSE },
    { BWRITERSPR_SAMPLER,       4,  FALSE },
    { BWRITERSPR_OUTPUT,       12,   TRUE },
    { ~0u, 0 } /* End tag */
};

static void asmparser_srcreg_vs_3(struct asm_parser *parser, struct instruction *instr, int num,
        const struct shader_reg *src)
{
    if (!check_reg_type(src, vs_3_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Source register %s not supported in VS 3.0\n",
                parser->line_no, debug_print_srcreg(src));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_loop_swizzle(parser, src);
    check_legacy_srcmod(parser, src->srcmod);
    instr->src[num] = *src;
}

static const struct allowed_reg_type ps_1_0123_reg_allowed[] =
{
    { BWRITERSPR_CONST,     8,  FALSE },
    { BWRITERSPR_TEMP,      2,  FALSE },
    { BWRITERSPR_TEXTURE,   4,  FALSE },
    { BWRITERSPR_INPUT,     2,  FALSE },
    { ~0u, 0 } /* End tag */
};

static void asmparser_srcreg_ps_1_0123(struct asm_parser *parser, struct instruction *instr, int num,
        const struct shader_reg *src)
{
    struct shader_reg reg;

    if (!check_reg_type(src, ps_1_0123_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Source register %s not supported in <== PS 1.3\n",
                parser->line_no, debug_print_srcreg(src));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_abs_srcmod(parser, src->srcmod);
    reg = map_oldps_register(src, FALSE);
    instr->src[num] = reg;
}

static const struct allowed_reg_type ps_1_4_reg_allowed[] =
{
    { BWRITERSPR_CONST,     8,  FALSE },
    { BWRITERSPR_TEMP,      6,  FALSE },
    { BWRITERSPR_TEXTURE,   6,  FALSE },
    { BWRITERSPR_INPUT,     2,  FALSE },
    { ~0u, 0 } /* End tag */
};

static void asmparser_srcreg_ps_1_4(struct asm_parser *parser, struct instruction *instr, int num,
        const struct shader_reg *src)
{
    struct shader_reg reg;

    if (!check_reg_type(src, ps_1_4_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Source register %s not supported in PS 1.4\n",
                parser->line_no, debug_print_srcreg(src));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_abs_srcmod(parser, src->srcmod);
    reg = map_oldps_register(src, TRUE);
    instr->src[num] = reg;
}

static const struct allowed_reg_type ps_2_0_reg_allowed[] =
{
    { BWRITERSPR_INPUT,         2,  FALSE },
    { BWRITERSPR_TEMP,         32,  FALSE },
    { BWRITERSPR_CONST,        32,  FALSE },
    { BWRITERSPR_CONSTINT,     16,  FALSE },
    { BWRITERSPR_CONSTBOOL,    16,  FALSE },
    { BWRITERSPR_SAMPLER,      16,  FALSE },
    { BWRITERSPR_TEXTURE,       8,  FALSE },
    { BWRITERSPR_COLOROUT,      4,  FALSE },
    { BWRITERSPR_DEPTHOUT,      1,  FALSE },
    { ~0u, 0 } /* End tag */
};

static void asmparser_srcreg_ps_2(struct asm_parser *parser, struct instruction *instr, int num,
        const struct shader_reg *src)
{
    struct shader_reg reg;

    if (!check_reg_type(src, ps_2_0_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Source register %s not supported in PS 2.0\n",
                parser->line_no, debug_print_srcreg(src));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_legacy_srcmod(parser, src->srcmod);
    check_abs_srcmod(parser, src->srcmod);
    reg = map_oldps_register(src, TRUE);
    instr->src[num] = reg;
}

static const struct allowed_reg_type ps_2_x_reg_allowed[] =
{
    { BWRITERSPR_INPUT,         2,  FALSE },
    { BWRITERSPR_TEMP,         32,  FALSE },
    { BWRITERSPR_CONST,        32,  FALSE },
    { BWRITERSPR_CONSTINT,     16,  FALSE },
    { BWRITERSPR_CONSTBOOL,    16,  FALSE },
    { BWRITERSPR_PREDICATE,     1,  FALSE },
    { BWRITERSPR_SAMPLER,      16,  FALSE },
    { BWRITERSPR_TEXTURE,       8,  FALSE },
    { BWRITERSPR_LABEL,      2048,  FALSE },
    { BWRITERSPR_COLOROUT,      4,  FALSE },
    { BWRITERSPR_DEPTHOUT,      1,  FALSE },
    { ~0u, 0 } /* End tag */
};

static void asmparser_srcreg_ps_2_x(struct asm_parser *parser, struct instruction *instr, int num,
        const struct shader_reg *src)
{
    struct shader_reg reg;

    if (!check_reg_type(src, ps_2_x_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Source register %s not supported in PS 2.x\n",
                parser->line_no, debug_print_srcreg(src));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_legacy_srcmod(parser, src->srcmod);
    check_abs_srcmod(parser, src->srcmod);
    reg = map_oldps_register(src, TRUE);
    instr->src[num] = reg;
}

static const struct allowed_reg_type ps_3_reg_allowed[] =
{
    { BWRITERSPR_INPUT,        10,   TRUE },
    { BWRITERSPR_TEMP,         32,  FALSE },
    { BWRITERSPR_CONST,       224,  FALSE },
    { BWRITERSPR_CONSTINT,     16,  FALSE },
    { BWRITERSPR_CONSTBOOL,    16,  FALSE },
    { BWRITERSPR_PREDICATE,     1,  FALSE },
    { BWRITERSPR_SAMPLER,      16,  FALSE },
    { BWRITERSPR_MISCTYPE,      2,  FALSE }, /* vPos and vFace */
    { BWRITERSPR_LOOP,          1,  FALSE },
    { BWRITERSPR_LABEL,      2048,  FALSE },
    { BWRITERSPR_COLOROUT,      4,  FALSE },
    { BWRITERSPR_DEPTHOUT,      1,  FALSE },
    { ~0u, 0 } /* End tag */
};

static void asmparser_srcreg_ps_3(struct asm_parser *parser,struct instruction *instr, int num,
        const struct shader_reg *src)
{
    if (!check_reg_type(src, ps_3_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Source register %s not supported in PS 3.0\n",
                parser->line_no, debug_print_srcreg(src));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_loop_swizzle(parser, src);
    check_legacy_srcmod(parser, src->srcmod);
    instr->src[num] = *src;
}

static void asmparser_dstreg_vs_1(struct asm_parser *parser, struct instruction *instr,
        const struct shader_reg *dst)
{
    struct shader_reg reg;

    if (!check_reg_type(dst, vs_1_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Destination register %s not supported in VS 1\n",
                parser->line_no, debug_print_dstreg(dst));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_ps_dstmod(parser, instr->dstmod);
    check_shift_dstmod(parser, instr->shift);
    reg = map_oldvs_register(dst);
    instr->dst = reg;
    instr->has_dst = TRUE;
}

static void asmparser_dstreg_vs_2(struct asm_parser *parser, struct instruction *instr,
        const struct shader_reg *dst)
{
    struct shader_reg reg;

    if (!check_reg_type(dst, vs_2_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Destination register %s not supported in VS 2.0\n",
                parser->line_no, debug_print_dstreg(dst));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_ps_dstmod(parser, instr->dstmod);
    check_shift_dstmod(parser, instr->shift);
    reg = map_oldvs_register(dst);
    instr->dst = reg;
    instr->has_dst = TRUE;
}

static void asmparser_dstreg_vs_3(struct asm_parser *parser,struct instruction *instr,
        const struct shader_reg *dst)
{
    if (!check_reg_type(dst, vs_3_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Destination register %s not supported in VS 3.0\n",
                parser->line_no, debug_print_dstreg(dst));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_ps_dstmod(parser, instr->dstmod);
    check_shift_dstmod(parser, instr->shift);
    instr->dst = *dst;
    instr->has_dst = TRUE;
}

static void asmparser_dstreg_ps_1_0123(struct asm_parser *parser, struct instruction *instr,
        const struct shader_reg *dst)
{
    struct shader_reg reg;

    if (!check_reg_type(dst, ps_1_0123_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Destination register %s not supported in PS 1\n",
                parser->line_no, debug_print_dstreg(dst));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    reg = map_oldps_register(dst, FALSE);
    instr->dst = reg;
    instr->has_dst = TRUE;
}

static void asmparser_dstreg_ps_1_4(struct asm_parser *parser, struct instruction *instr,
        const struct shader_reg *dst)
{
    struct shader_reg reg;

    if (!check_reg_type(dst, ps_1_4_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Destination register %s not supported in PS 1\n",
                parser->line_no, debug_print_dstreg(dst));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    reg = map_oldps_register(dst, TRUE);
    instr->dst = reg;
    instr->has_dst = TRUE;
}

static void asmparser_dstreg_ps_2(struct asm_parser *parser, struct instruction *instr,
        const struct shader_reg *dst)
{
    struct shader_reg reg;

    if (!check_reg_type(dst, ps_2_0_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Destination register %s not supported in PS 2.0\n",
                parser->line_no, debug_print_dstreg(dst));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_shift_dstmod(parser, instr->shift);
    reg = map_oldps_register(dst, TRUE);
    instr->dst = reg;
    instr->has_dst = TRUE;
}

static void asmparser_dstreg_ps_2_x(struct asm_parser *parser, struct instruction *instr,
        const struct shader_reg *dst)
{
    struct shader_reg reg;

    if (!check_reg_type(dst, ps_2_x_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Destination register %s not supported in PS 2.x\n",
                parser->line_no, debug_print_dstreg(dst));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_shift_dstmod(parser, instr->shift);
    reg = map_oldps_register(dst, TRUE);
    instr->dst = reg;
    instr->has_dst = TRUE;
}

static void asmparser_dstreg_ps_3(struct asm_parser *parser, struct instruction *instr,
        const struct shader_reg *dst)
{
    if (!check_reg_type(dst, ps_3_reg_allowed))
    {
        asmparser_message(parser, "Line %u: Destination register %s not supported in PS 3.0\n",
                parser->line_no, debug_print_dstreg(dst));
        set_parse_status(&parser->status, PARSE_ERR);
    }
    check_shift_dstmod(parser, instr->shift);
    instr->dst = *dst;
    instr->has_dst = TRUE;
}

static void asmparser_predicate_supported(struct asm_parser *parser, const struct shader_reg *predicate)
{
    if (!parser->shader)
        return;
    if (parser->shader->num_instrs == 0)
        ERR("Predicate without an instruction.\n");
    /* Set the predicate of the last instruction added to the shader. */
    parser->shader->instr[parser->shader->num_instrs - 1]->has_predicate = TRUE;
    parser->shader->instr[parser->shader->num_instrs - 1]->predicate = *predicate;
}

static void asmparser_predicate_unsupported(struct asm_parser *parser, const struct shader_reg *predicate)
{
    asmparser_message(parser, "Line %u: Predicate not supported in < VS 2.0 or PS 2.x\n", parser->line_no);
    set_parse_status(&parser->status, PARSE_ERR);
}

static void asmparser_coissue_supported(struct asm_parser *parser)
{
    if (!parser->shader)
        return;
    if (parser->shader->num_instrs == 0)
    {
        asmparser_message(parser, "Line %u: Coissue flag on the first shader instruction\n", parser->line_no);
        set_parse_status(&parser->status, PARSE_ERR);
    }
    /* Set the coissue flag of the last instruction added to the shader. */
    parser->shader->instr[parser->shader->num_instrs - 1]->coissue = TRUE;
}

static void asmparser_coissue_unsupported(struct asm_parser *parser)
{
    asmparser_message(parser, "Line %u: Coissue is only supported in pixel shaders versions <= 1.4\n", parser->line_no);
    set_parse_status(&parser->status, PARSE_ERR);
}

static const struct asmparser_backend parser_vs_1 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_vs_1,
    asmparser_srcreg_vs_1,

    asmparser_predicate_unsupported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output_unsupported,
    asmparser_dcl_input,
    asmparser_dcl_sampler_unsupported,

    asmparser_end,

    asmparser_instr,
};

static const struct asmparser_backend parser_vs_2 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_vs_2,
    asmparser_srcreg_vs_2,

    asmparser_predicate_supported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output_unsupported,
    asmparser_dcl_input,
    asmparser_dcl_sampler_unsupported,

    asmparser_end,

    asmparser_instr,
};

static const struct asmparser_backend parser_vs_3 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_vs_3,
    asmparser_srcreg_vs_3,

    asmparser_predicate_supported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output,
    asmparser_dcl_input,
    asmparser_dcl_sampler,

    asmparser_end,

    asmparser_instr,
};

static const struct asmparser_backend parser_ps_1_0123 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_ps_1_0123,
    asmparser_srcreg_ps_1_0123,

    asmparser_predicate_unsupported,
    asmparser_coissue_supported,

    asmparser_dcl_output_unsupported,
    asmparser_dcl_input_unsupported,
    asmparser_dcl_sampler_unsupported,

    asmparser_end,

    asmparser_instr,
};

static const struct asmparser_backend parser_ps_1_4 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_ps_1_4,
    asmparser_srcreg_ps_1_4,

    asmparser_predicate_unsupported,
    asmparser_coissue_supported,

    asmparser_dcl_output_unsupported,
    asmparser_dcl_input_unsupported,
    asmparser_dcl_sampler_unsupported,

    asmparser_end,

    asmparser_instr,
};

static const struct asmparser_backend parser_ps_2 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_ps_2,
    asmparser_srcreg_ps_2,

    asmparser_predicate_unsupported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output_unsupported,
    asmparser_dcl_input_ps_2,
    asmparser_dcl_sampler,

    asmparser_end,

    asmparser_instr,
};

static const struct asmparser_backend parser_ps_2_x = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_ps_2_x,
    asmparser_srcreg_ps_2_x,

    asmparser_predicate_supported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output_unsupported,
    asmparser_dcl_input_ps_2,
    asmparser_dcl_sampler,

    asmparser_end,

    asmparser_instr,
};

static const struct asmparser_backend parser_ps_3 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_ps_3,
    asmparser_srcreg_ps_3,

    asmparser_predicate_supported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output_unsupported,
    asmparser_dcl_input,
    asmparser_dcl_sampler,

    asmparser_end,

    asmparser_instr,
};

static void gen_oldvs_output(struct bwriter_shader *shader) {
    record_declaration(shader, BWRITERDECLUSAGE_POSITION, 0, 0, TRUE, OPOS_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 0, 0, TRUE, OT0_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 1, 0, TRUE, OT1_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 2, 0, TRUE, OT2_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 3, 0, TRUE, OT3_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 4, 0, TRUE, OT4_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 5, 0, TRUE, OT5_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 6, 0, TRUE, OT6_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 7, 0, TRUE, OT7_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_FOG, 0, 0, TRUE, OFOG_REG, OFOG_WRITEMASK, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_PSIZE, 0, 0, TRUE, OPTS_REG, OPTS_WRITEMASK, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_COLOR, 0, 0, TRUE, OD0_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_COLOR, 1, 0, TRUE, OD1_REG, BWRITERSP_WRITEMASK_ALL, TRUE);
}

static void gen_oldps_input(struct bwriter_shader *shader, uint32_t texcoords)
{
    switch(texcoords)
    {
        case 8: record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 7, 0, FALSE, T7_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
            /* fall through */
        case 7: record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 6, 0, FALSE, T6_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
            /* fall through */
        case 6: record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 5, 0, FALSE, T5_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
            /* fall through */
        case 5: record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 4, 0, FALSE, T4_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
            /* fall through */
        case 4: record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 3, 0, FALSE, T3_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
            /* fall through */
        case 3: record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 2, 0, FALSE, T2_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
            /* fall through */
        case 2: record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 1, 0, FALSE, T1_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
            /* fall through */
        case 1: record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 0, 0, FALSE, T0_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
    };
    record_declaration(shader, BWRITERDECLUSAGE_COLOR, 0, 0, FALSE, C0_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
    record_declaration(shader, BWRITERDECLUSAGE_COLOR, 1, 0, FALSE, C1_VARYING, BWRITERSP_WRITEMASK_ALL, TRUE);
}

void create_vs10_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("vs_1_0\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_VERTEX;
    ret->shader->major_version = 1;
    ret->shader->minor_version = 0;
    ret->funcs = &parser_vs_1;
    gen_oldvs_output(ret->shader);
}

void create_vs11_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("vs_1_1\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_VERTEX;
    ret->shader->major_version = 1;
    ret->shader->minor_version = 1;
    ret->funcs = &parser_vs_1;
    gen_oldvs_output(ret->shader);
}

void create_vs20_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("vs_2_0\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_VERTEX;
    ret->shader->major_version = 2;
    ret->shader->minor_version = 0;
    ret->funcs = &parser_vs_2;
    gen_oldvs_output(ret->shader);
}

void create_vs2x_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("vs_2_x\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_VERTEX;
    ret->shader->major_version = 2;
    ret->shader->minor_version = 1;
    ret->funcs = &parser_vs_2;
    gen_oldvs_output(ret->shader);
}

void create_vs30_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("vs_3_0\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_VERTEX;
    ret->shader->major_version = 3;
    ret->shader->minor_version = 0;
    ret->funcs = &parser_vs_3;
}

void create_ps10_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("ps_1_0\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_PIXEL;
    ret->shader->major_version = 1;
    ret->shader->minor_version = 0;
    ret->funcs = &parser_ps_1_0123;
    gen_oldps_input(ret->shader, 4);
}

void create_ps11_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("ps_1_1\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_PIXEL;
    ret->shader->major_version = 1;
    ret->shader->minor_version = 1;
    ret->funcs = &parser_ps_1_0123;
    gen_oldps_input(ret->shader, 4);
}

void create_ps12_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("ps_1_2\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_PIXEL;
    ret->shader->major_version = 1;
    ret->shader->minor_version = 2;
    ret->funcs = &parser_ps_1_0123;
    gen_oldps_input(ret->shader, 4);
}

void create_ps13_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("ps_1_3\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_PIXEL;
    ret->shader->major_version = 1;
    ret->shader->minor_version = 3;
    ret->funcs = &parser_ps_1_0123;
    gen_oldps_input(ret->shader, 4);
}

void create_ps14_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("ps_1_4\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_PIXEL;
    ret->shader->major_version = 1;
    ret->shader->minor_version = 4;
    ret->funcs = &parser_ps_1_4;
    gen_oldps_input(ret->shader, 6);
}

void create_ps20_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("ps_2_0\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_PIXEL;
    ret->shader->major_version = 2;
    ret->shader->minor_version = 0;
    ret->funcs = &parser_ps_2;
    gen_oldps_input(ret->shader, 8);
}

void create_ps2x_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("ps_2_x\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_PIXEL;
    ret->shader->major_version = 2;
    ret->shader->minor_version = 1;
    ret->funcs = &parser_ps_2_x;
    gen_oldps_input(ret->shader, 8);
}

void create_ps30_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("ps_3_0\n");

    ret->shader = calloc(1, sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(&ret->status, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_PIXEL;
    ret->shader->major_version = 3;
    ret->shader->minor_version = 0;
    ret->funcs = &parser_ps_3;
}
