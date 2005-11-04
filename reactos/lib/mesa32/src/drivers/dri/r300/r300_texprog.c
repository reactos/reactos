#include "glheader.h"
#include "state.h"
#include "imports.h"
#include "enums.h"
#include "macros.h"
#include "context.h"
#include "dd.h"
#include "simple_list.h"

#include "api_arrayelt.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "texformat.h"

#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#if USE_ARB_F_P == 0
#include "r300_ioctl.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_program.h"
#include "r300_emit.h"
#include "r300_fixed_pipelines.h"
#include "r300_tex.h"
#include "pixel_shader.h"
#include "r300_texprog.h"

/* TODO: we probably should have a better way to emit alu instructions */
#define INST0 p->alu.inst[p->alu.length].inst0 = 
#define INST1 p->alu.inst[p->alu.length].inst1 = 
#define INST2 p->alu.inst[p->alu.length].inst2 = 
#define INST3 p->alu.inst[p->alu.length].inst3 = 
#define EMIT_INST p->alu.length++

void emit_tex(struct r300_pixel_shader_program *p, GLuint dest, GLuint unit, GLuint src)
{
	p->tex.inst[p->tex.length++] = 0
		| (src << R300_FPITX_SRC_SHIFT)
		| (dest << R300_FPITX_DST_SHIFT)
		| (unit << R300_FPITX_IMAGE_SHIFT)
		/* I don't know if this is needed, but the hardcoded 0x18000 set it, so I will too */
		| (3 << 15);
//	fprintf(stderr, "emit texinst: 0x%x\n", p->tex.inst[p->tex.length-1]);	
}

GLuint get_source(struct r300_pixel_shader_state *ps, GLenum src, GLuint unit, GLuint tc_reg) {
	switch (src) {
	case GL_TEXTURE:
		if (!ps->have_sample) {
			emit_tex(&ps->program, tc_reg, unit, tc_reg);
			ps->have_sample = 1;
		}
		return tc_reg;
	case GL_CONSTANT:
		WARN_ONCE("TODO: Implement envcolor\n");
		return ps->color_reg;
	case GL_PRIMARY_COLOR:
		return ps->color_reg;
	case GL_PREVIOUS:
		return ps->src_previous;
	default:
		WARN_ONCE("Unknown source enum\n");
		return ps->src_previous;
	}
}

GLuint get_temp(struct r300_pixel_shader_program *p)
{
	return p->temp_register_count++;
}

inline void emit_texenv_color(r300ContextPtr r300, struct r300_pixel_shader_state *ps,
				GLuint out, GLenum envmode, GLenum format, GLuint unit, GLuint tc_reg) {
	struct r300_pixel_shader_program *p = &ps->program;

	const GLuint Cp = get_source(ps, GL_PREVIOUS, unit, tc_reg);
	const GLuint Cs = get_source(ps, GL_TEXTURE, unit, tc_reg);

	switch(envmode) {
	case GL_DECAL: /* TODO */
	case GL_BLEND: /* TODO */
	case GL_REPLACE:
		INST0 EASY_PFS_INSTR0(MAD, SRC0C_XYZ, ONE, ZERO);
		switch (format) {
		case GL_ALPHA:
			// Cv = Cp
			INST1 EASY_PFS_INSTR1(out, Cp, PFS_FLAG_CONST, PFS_FLAG_CONST, ALL, NONE);
			break;
		default:
			// Cv = Cs
			INST1 EASY_PFS_INSTR1(out, Cs, PFS_FLAG_CONST, PFS_FLAG_CONST, ALL, NONE);
			break;
		}
		break;
	case GL_MODULATE:
		switch (format) {
		case GL_ALPHA:
			// Cv = Cp
			INST0 EASY_PFS_INSTR0(MAD, SRC0C_XYZ, ONE, ZERO);
			INST1 EASY_PFS_INSTR1(out, Cp, PFS_FLAG_CONST, PFS_FLAG_CONST, ALL, NONE);
			break;
		default:
			// Cv = CpCs
			INST0 EASY_PFS_INSTR0(MAD, SRC0C_XYZ, SRC1C_XYZ, ZERO);
			INST1 EASY_PFS_INSTR1(out, Cp, Cs, PFS_FLAG_CONST, ALL, NONE);
			break;
		}
		break;
	case GL_ADD:
		switch (format) {
		case GL_ALPHA:
			// Cv = Cp
			INST0 EASY_PFS_INSTR0(MAD, SRC0C_XYZ, ONE, ZERO);
			INST1 EASY_PFS_INSTR1(out, Cp, PFS_FLAG_CONST, PFS_FLAG_CONST, ALL, NONE);
			break;
		default:
			// Cv = Cp + Cs
			INST0 EASY_PFS_INSTR0(MAD, SRC0C_XYZ, ONE, SRC1C_XYZ);
			INST1 EASY_PFS_INSTR1(out, Cp, Cs, PFS_FLAG_CONST, ALL, NONE);
			break;
		}
		break;
	default:
		fprintf(stderr, "%s: should never get here!\n", __func__);
		break;
	}

	return;
}
				
inline void emit_texenv_alpha(r300ContextPtr r300, struct r300_pixel_shader_state *ps,
				GLuint out, GLenum envmode, GLenum format, GLuint unit, GLuint tc_reg) {
	struct r300_pixel_shader_program *p = &ps->program;

	const GLuint Ap = get_source(ps, GL_PREVIOUS, unit, tc_reg);
	const GLuint As = get_source(ps, GL_TEXTURE, unit, tc_reg);

	switch(envmode) {
	case GL_DECAL: /* TODO */
	case GL_BLEND: /* TODO */
	case GL_REPLACE:
		INST2 EASY_PFS_INSTR2(MAD, SRC0A, ONE, ZERO);
		switch (format) {
		case GL_LUMINANCE:
		case GL_RGB:
			// Av = Ap
			INST3 EASY_PFS_INSTR3(out, Ap, PFS_FLAG_CONST, PFS_FLAG_CONST, REG);
			break;
		default:
			INST3 EASY_PFS_INSTR3(out, As, PFS_FLAG_CONST, PFS_FLAG_CONST, REG);
			break;
		}
		break;
	case GL_MODULATE:
	case GL_ADD:
		switch (format) {
		case GL_LUMINANCE:
		case GL_RGB:
			// Av = Ap
			INST2 EASY_PFS_INSTR2(MAD, SRC0A, ONE, ZERO);
			INST3 EASY_PFS_INSTR3(out, Ap, PFS_FLAG_CONST, PFS_FLAG_CONST, REG);
			break;
		default:
			// Av = ApAs
			INST2 EASY_PFS_INSTR2(MAD, SRC0A, SRC1A, ZERO);
			INST3 EASY_PFS_INSTR3(out, Ap, As, PFS_FLAG_CONST, REG);
			break;
		}
		break;
	default:
		fprintf(stderr, "%s: should never get here!\n", __func__);
		break;
	}

	return;
}

GLuint emit_texenv(r300ContextPtr r300, GLuint tc_reg, GLuint unit)
{
	struct r300_pixel_shader_state *ps = &r300->state.pixel_shader;
	struct r300_pixel_shader_program *p = &ps->program;
	GLcontext *ctx = r300->radeon.glCtx;
	struct gl_texture_object *texobj = ctx->Texture.Unit[unit]._Current;
	GLenum envmode = ctx->Texture.Unit[unit].EnvMode;
	GLenum format = texobj->Image[0][texobj->BaseLevel]->Format;
	
	const GLuint out = tc_reg;
	const GLuint Cf = get_source(ps, GL_PRIMARY_COLOR, unit, tc_reg);
					
	WARN_ONCE("Texture environments are currently incomplete / wrong! Help me!\n");
//	fprintf(stderr, "EnvMode = %s\n", _mesa_lookup_enum_by_nr(ctx->Texture.Unit[unit].EnvMode));
	
	switch (envmode) {
	case GL_REPLACE:
	case GL_MODULATE:
	case GL_DECAL:
	case GL_BLEND:
	case GL_ADD:
		/* Maybe these should be combined?  I thought it'd be messy */
		emit_texenv_color(r300, ps, out, envmode, format, unit, tc_reg);
		emit_texenv_alpha(r300, ps, out, envmode, format, unit, tc_reg);
		EMIT_INST;
		return out;
		break;
	case GL_COMBINE:
		WARN_ONCE("EnvMode == GL_COMBINE unsupported! Help Me!!\n");
		return get_source(ps, GL_TEXTURE, unit, tc_reg);
		break;
	default:
		WARN_ONCE("Unknown EnvMode == %d, name=%s\n", envmode,
						_mesa_lookup_enum_by_nr(envmode));
		return get_source(ps, GL_TEXTURE, unit, tc_reg);
		break;
	}
	
}

void r300GenerateTextureFragmentShader(r300ContextPtr r300)
{
	struct r300_pixel_shader_state *ps = &r300->state.pixel_shader;
	struct r300_pixel_shader_program *p = &ps->program;
	GLcontext *ctx = r300->radeon.glCtx;
	int i, tc_reg;
	GLuint OutputsWritten;
	
	if(hw_tcl_on)
		OutputsWritten = CURRENT_VERTEX_SHADER(ctx)->OutputsWritten;

	p->tex.length = 0;
	p->alu.length = 0;
	p->active_nodes = 1;
	p->first_node_has_tex = 1;
	p->temp_register_count = r300->state.texture.tc_count + 1; /* texcoords and colour reg */

	ps->color_reg	 = r300->state.texture.tc_count;
	ps->src_previous = ps->color_reg;
		
	tc_reg = 0;
	for (i=0;i<ctx->Const.MaxTextureUnits;i++) {
		if (TMU_ENABLED(ctx, i)) {
			ps->have_sample = 0;
			ps->src_previous = emit_texenv(r300, tc_reg, i);
			tc_reg++;
		}
	}
	
/* Do a MOV from last output, to destination reg.. This won't be needed when we
 * have a better way of emitting alu instructions
 */
	INST0 EASY_PFS_INSTR0(MAD, SRC0C_XYZ, ONE, ZERO);
	INST1 EASY_PFS_INSTR1(0, ps->src_previous, PFS_FLAG_CONST, PFS_FLAG_CONST, NONE, ALL);
	INST2 EASY_PFS_INSTR2(MAD, SRC0A, ONE, ZERO);
	INST3 EASY_PFS_INSTR3(0, ps->src_previous, PFS_FLAG_CONST, PFS_FLAG_CONST, OUTPUT);
	EMIT_INST;
	
	p->node[3].tex_end = ps->program.tex.length - 1;
	p->node[3].tex_offset = 0;
	p->node[3].alu_end = ps->program.alu.length - 1;
	p->node[3].alu_offset = 0;

	p->tex_end		= ps->program.tex.length - 1;
	p->tex_offset	= 0;
	p->alu_end		= ps->program.alu.length - 1;
	p->alu_offset	= 0;
}
#endif // USE_ARB_F_P == 0

