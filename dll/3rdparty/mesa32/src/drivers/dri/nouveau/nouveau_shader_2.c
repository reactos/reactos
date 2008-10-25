/*
 * Copyright (C) 2006 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Ben Skeggs <darktama@iinet.net.au>
 */

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "nouveau_context.h"
#include "nouveau_shader.h"
#include "nouveau_msg.h"

struct pass2_rec {
	/* Map nvsRegister temp ID onto hw temp ID */
	unsigned int temps[NVS_MAX_TEMPS];
	/* Track free hw registers */
	unsigned int hw_temps[NVS_MAX_TEMPS];
};

static int
pass2_alloc_hw_temp(nvsPtr nvs)
{
	struct pass2_rec *rec = nvs->pass_rec;
	int i;

	for (i=0; i<nvs->func->MaxTemp; i++) {
		/* This is a *horrible* hack.. R0 is both temp0 and result.color
		 * in NV30/40 fragprogs, we can use R0 as a temp before result
		 * is written however..
		 */
		if (nvs->mesa.vp.Base.Target == GL_FRAGMENT_PROGRAM_ARB && i==0)
			continue;
		if (rec->hw_temps[i] == 0) {
			rec->hw_temps[i] = 1;
			return i;
		}
	}

	return -1;
}

static nvsRegister
pass2_mangle_reg(nvsPtr nvs, nvsInstruction *inst, nvsRegister reg)
{
	struct pass2_rec *rec = nvs->pass_rec;

	if (reg.file == NVS_FILE_TEMP) {
		if (rec->temps[reg.index] == -1)
			rec->temps[reg.index] = pass2_alloc_hw_temp(nvs);
		reg.index = rec->temps[reg.index];
	}

	return reg;
}

static void
pass2_add_instruction(nvsPtr nvs, nvsInstruction *inst, 
		      struct _op_xlat *op, int slot)
{
	nvsSwzComp default_swz[4] = { NVS_SWZ_X, NVS_SWZ_Y,
				      NVS_SWZ_Z, NVS_SWZ_W };
	nvsFunc *shader = nvs->func;
	nvsRegister reg;
	int i;

	shader->SetOpcode(shader, op->NV, slot);
	if (inst->saturate	) shader->SetSaturate(shader);
	if (inst->cond_update	) shader->SetCCUpdate(shader);
	if (inst->cond_test	) shader->SetCondition(shader, 1, inst->cond,
						       inst->cond_reg,
						       inst->cond_swizzle);
	else			  shader->SetCondition(shader, 0, NVS_COND_TR,
						       0,
						       default_swz);
	switch (inst->op) {
	case NVS_OP_TEX:
	case NVS_OP_TXB:
	case NVS_OP_TXL:
	case NVS_OP_TXP:
	case NVS_OP_TXD:
		shader->SetTexImageUnit(shader, inst->tex_unit);
		break;
	default:
		break;
	}

	for (i = 0; i < 3; i++) {
		if (op->srcpos[i] != -1) {
			reg = pass2_mangle_reg(nvs, inst, inst->src[i]);

			shader->SetSource(shader, &reg, op->srcpos[i]);

			if (reg.file == NVS_FILE_CONST &&
					shader->GetSourceConstVal) {
				int idx_slot =
					nvs->params[reg.index].hw_index_cnt++;
				nvs->params[reg.index].hw_index = realloc(
						nvs->params[reg.index].hw_index,
						sizeof(int) * idx_slot+1);
				nvs->params[reg.index].hw_index[idx_slot] =
					nvs->program_current + 4;
			}
		}
	}

	reg = pass2_mangle_reg(nvs, inst, inst->dest);
	shader->SetResult(shader, &reg, inst->mask, slot);

	if (inst->dest_scale != NVS_SCALE_1X) {
		shader->SetResultScale(shader, inst->dest_scale);
	}
}

static int
pass2_assemble_instruction(nvsPtr nvs, nvsInstruction *inst, int last)
{
	nvsFunc *shader = nvs->func;
	struct _op_xlat *op;
	unsigned int hw_inst[8];
	int slot;
	int instsz;
	int i;

	shader->inst = hw_inst;

	/* Assemble this instruction */
	if (!(op = shader->GetOPTXFromSOP(inst->op, &slot)))
		return 0;
	shader->InitInstruction(shader);
	pass2_add_instruction(nvs, inst, op, slot);
	if (last)
		shader->SetLastInst(shader);

	instsz = shader->GetOffsetNext(nvs->func);
	if (nvs->program_size + instsz >= nvs->program_alloc_size) {
		nvs->program_alloc_size *= 2;
		nvs->program = realloc(nvs->program,
				       nvs->program_alloc_size *
				       sizeof(uint32_t));
	}

	for (i=0; i<instsz; i++)
		nvs->program[nvs->program_current++] = hw_inst[i];
	nvs->program_size = nvs->program_current;
	return 1;
}

static GLboolean
pass2_translate(nvsPtr nvs, nvsFragmentHeader *f)
{
	nvsFunc *shader = nvs->func;
	GLboolean last;

	while (f) {
		last = (f == ((nvsSubroutine*)nvs->program_tree)->insn_tail);

		switch (f->type) {
		case NVS_INSTRUCTION:
			if (!pass2_assemble_instruction(nvs,
							(nvsInstruction *)f,
							last))
				return GL_FALSE;
			break;
		default:
			WARN_ONCE("Unimplemented fragment type\n");
			return GL_FALSE;
		}

		f = f->next;
	}

	return GL_TRUE;
}

/* Translate program into hardware format */
GLboolean
nouveau_shader_pass2(nvsPtr nvs)
{
	struct pass2_rec *rec;
	int i;

	NVSDBG("start: nvs=%p\n", nvs);

	rec = calloc(1, sizeof(struct pass2_rec));
	for (i=0; i<NVS_MAX_TEMPS; i++)
		rec->temps[i] = -1;
	nvs->pass_rec = rec;

	/* Start off with allocating 4 uint32_t's for each inst, will be grown
	 * if necessary..
	 */
	nvs->program_alloc_size = nvs->mesa.vp.Base.NumInstructions * 4;
	nvs->program = calloc(nvs->program_alloc_size, sizeof(uint32_t));
	nvs->program_size    = 0;
	nvs->program_current = 0;

	if (!pass2_translate(nvs,
			     ((nvsSubroutine*)nvs->program_tree)->insn_head)) {
		free(nvs->program);
		nvs->program = NULL;
		return GL_FALSE;
	}

	/* Shrink allocated memory to only what we need */
	nvs->program = realloc(nvs->program,
			       nvs->program_size * sizeof(uint32_t));
	nvs->program_alloc_size = nvs->program_size;

	nvs->translated  = 1;
	nvs->on_hardware = 0;

	if (NOUVEAU_DEBUG & DEBUG_SHADERS) {
		fflush(stdout); fflush(stderr);
		fprintf(stderr, "-----------MESA PROGRAM target=%s, id=0x%x\n",
				_mesa_lookup_enum_by_nr(
					nvs->mesa.vp.Base.Target),
				nvs->mesa.vp.Base.Id);
		fflush(stdout); fflush(stderr);
		_mesa_print_program(&nvs->mesa.vp.Base);
		fflush(stdout); fflush(stderr);
		fprintf(stderr, "^^^^^^^^^^^^^^^^MESA PROGRAM\n");
		fflush(stdout); fflush(stderr);
		fprintf(stderr, "----------------NV PROGRAM\n");
		fflush(stdout); fflush(stderr);
		nvsDisasmHWShader(nvs);
		fflush(stdout); fflush(stderr);
		fprintf(stderr, "^^^^^^^^^^^^^^^^NV PROGRAM\n");
		fflush(stdout); fflush(stderr);
	}

	return GL_TRUE;
}

