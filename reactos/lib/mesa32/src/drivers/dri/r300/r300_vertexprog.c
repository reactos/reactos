/**************************************************************************

Copyright (C) 2005 Aapo Tahkola.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Aapo Tahkola <aet@rasterburn.org>
 */
#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "r300_context.h"
#include "r300_program.h"
#include "nvvertprog.h"

#define SCALAR_FLAG (1<<31)
#define FLAG_MASK (1<<31)
#define OP_MASK	(0xf)  /* we are unlikely to have more than 15 */
#define OPN(operator, ip, op) {#operator, VP_OPCODE_##operator, ip, op}

static struct{
	char *name;
	int opcode;
	unsigned long ip; /* number of input operands and flags */
	unsigned long op;
}op_names[]={
	OPN(ABS, 1, 1),
	OPN(ADD, 2, 1),
	OPN(ARL, 1, 1|SCALAR_FLAG),
	OPN(DP3, 2, 3|SCALAR_FLAG),
	OPN(DP4, 2, 3|SCALAR_FLAG),
	OPN(DPH, 2, 3|SCALAR_FLAG),
	OPN(DST, 2, 1),
	OPN(EX2, 1|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(EXP, 1|SCALAR_FLAG, 1),
	OPN(FLR, 1, 1),
	OPN(FRC, 1, 1),
	OPN(LG2, 1|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(LIT, 1, 1),
	OPN(LOG, 1|SCALAR_FLAG, 1),
	OPN(MAD, 3, 1),
	OPN(MAX, 2, 1),
	OPN(MIN, 2, 1),
	OPN(MOV, 1, 1),
	OPN(MUL, 2, 1),
	OPN(POW, 2|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(RCP, 1|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(RSQ, 1|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(SGE, 2, 1),
	OPN(SLT, 2, 1),
	OPN(SUB, 2, 1),
	OPN(SWZ, 1, 1),
	OPN(XPD, 2, 1),
	OPN(RCC, 0, 0), //extra
	OPN(PRINT, 0, 0),
	OPN(END, 0, 0),
};
#undef OPN
#define OPN(rf) {#rf, PROGRAM_##rf}

static struct{
	char *name;
	int id;
}register_file_names[]={
	OPN(TEMPORARY),
	OPN(INPUT),
	OPN(OUTPUT),
	OPN(LOCAL_PARAM),
	OPN(ENV_PARAM),
	OPN(NAMED_PARAM),
	OPN(STATE_VAR),
	OPN(WRITE_ONLY),
	OPN(ADDRESS),
};
	
static char *dst_mask_names[4]={ "X", "Y", "Z", "W" };

/* from vertex program spec:
      Instruction    Inputs  Output   Description
      -----------    ------  ------   --------------------------------
      ABS            v       v        absolute value
      ADD            v,v     v        add
      ARL            v       a        address register load
      DP3            v,v     ssss     3-component dot product
      DP4            v,v     ssss     4-component dot product
      DPH            v,v     ssss     homogeneous dot product
      DST            v,v     v        distance vector
      EX2            s       ssss     exponential base 2
      EXP            s       v        exponential base 2 (approximate)
      FLR            v       v        floor
      FRC            v       v        fraction
      LG2            s       ssss     logarithm base 2
      LIT            v       v        compute light coefficients
      LOG            s       v        logarithm base 2 (approximate)
      MAD            v,v,v   v        multiply and add
      MAX            v,v     v        maximum
      MIN            v,v     v        minimum
      MOV            v       v        move
      MUL            v,v     v        multiply
      POW            s,s     ssss     exponentiate
      RCP            s       ssss     reciprocal
      RSQ            s       ssss     reciprocal square root
      SGE            v,v     v        set on greater than or equal
      SLT            v,v     v        set on less than
      SUB            v,v     v        subtract
      SWZ            v       v        extended swizzle
      XPD            v,v     v        cross product
*/

void dump_program_params(GLcontext *ctx, struct vertex_program *vp)
{
	int i;
	int pi;

	fprintf(stderr, "NumInstructions=%d\n", vp->Base.NumInstructions);
	fprintf(stderr, "NumTemporaries=%d\n", vp->Base.NumTemporaries);
	fprintf(stderr, "NumParameters=%d\n", vp->Base.NumParameters);
	fprintf(stderr, "NumAttributes=%d\n", vp->Base.NumAttributes);
	fprintf(stderr, "NumAddressRegs=%d\n", vp->Base.NumAddressRegs);
	
	_mesa_load_state_parameters(ctx, vp->Parameters);
			
#if 0	
	for(pi=0; pi < vp->Base.NumParameters; pi++){
		fprintf(stderr, "{ ");
		for(i=0; i < 4; i++)
			fprintf(stderr, "%f ", vp->Base.LocalParams[pi][i]);
		fprintf(stderr, "}\n");
	}
#endif	
	for(pi=0; pi < vp->Parameters->NumParameters; pi++){
		fprintf(stderr, "param %02d:", pi);
		
		switch(vp->Parameters->Parameters[pi].Type){
			
		case NAMED_PARAMETER:
			fprintf(stderr, "%s", vp->Parameters->Parameters[pi].Name);
			fprintf(stderr, "(NAMED_PARAMETER)");
		break;
		
		case CONSTANT:
			fprintf(stderr, "(CONSTANT)");
		break;
		
		case STATE:
			fprintf(stderr, "(STATE)\n");
		break;

		}
		
		fprintf(stderr, "{ ");
		for(i=0; i < 4; i++)
			fprintf(stderr, "%f ", vp->Parameters->ParameterValues[pi][i]);
		fprintf(stderr, "}\n");
		
	}
}

void debug_vp(GLcontext *ctx, struct vertex_program *vp)
{
	struct vp_instruction *vpi;
	int i, operand_index;
	int operator_index;
	
	dump_program_params(ctx, vp);

	vpi=vp->Instructions;
	
	for(;; vpi++){
		if(vpi->Opcode == VP_OPCODE_END)
			break;
		
		for(i=0; i < sizeof(op_names) / sizeof(*op_names); i++){
			if(vpi->Opcode == op_names[i].opcode){
				fprintf(stderr, "%s ", op_names[i].name);
				break;
			}
		}
		operator_index=i;
		
		for(i=0; i < sizeof(register_file_names) / sizeof(*register_file_names); i++){
			if(vpi->DstReg.File == register_file_names[i].id){
				fprintf(stderr, "%s ", register_file_names[i].name);
				break;
			}
		}
		
		fprintf(stderr, "%d.", vpi->DstReg.Index);
		
		for(i=0; i < 4; i++)
			if(vpi->DstReg.WriteMask & (1<<i))
				fprintf(stderr, "%s", dst_mask_names[i]);
		fprintf(stderr, " ");
		
		for(operand_index=0; operand_index < (op_names[operator_index].ip & (~FLAG_MASK));
			operand_index++){
				
			if(vpi->SrcReg[operand_index].Negate)
				fprintf(stderr, "-");
			
			for(i=0; i < sizeof(register_file_names) / sizeof(*register_file_names); i++){
				if(vpi->SrcReg[operand_index].File == register_file_names[i].id){
					fprintf(stderr, "%s ", register_file_names[i].name);
					break;
				}
			}
			fprintf(stderr, "%d.", vpi->SrcReg[operand_index].Index);
			
			for(i=0; i < 4; i++)
				fprintf(stderr, "%s", dst_mask_names[GET_SWZ(vpi->SrcReg[operand_index].Swizzle, i)]);
			
			if(operand_index+1 < (op_names[operator_index].ip & (~FLAG_MASK)) )
				fprintf(stderr, ",");
		}
		fprintf(stderr, "\n");
	}
	
}

void r300VertexProgUpdateParams(GLcontext *ctx, struct r300_vertex_program *vp)
{
	int pi;
	struct vertex_program *mesa_vp=(void *)vp;
	int dst_index;
	
	_mesa_load_state_parameters(ctx, mesa_vp->Parameters);
	
	//debug_vp(ctx, mesa_vp);
	if(mesa_vp->Parameters->NumParameters * 4 > VSF_MAX_FRAGMENT_LENGTH){
		fprintf(stderr, "%s:Params exhausted\n", __FUNCTION__);
		exit(-1);
	}
	dst_index=0;
	for(pi=0; pi < mesa_vp->Parameters->NumParameters; pi++){
		switch(mesa_vp->Parameters->Parameters[pi].Type){
			
		case STATE:
		case NAMED_PARAMETER:
			//fprintf(stderr, "%s", vp->Parameters->Parameters[pi].Name);
		case CONSTANT:
			vp->params.body.f[dst_index++]=mesa_vp->Parameters->ParameterValues[pi][0];
			vp->params.body.f[dst_index++]=mesa_vp->Parameters->ParameterValues[pi][1];
			vp->params.body.f[dst_index++]=mesa_vp->Parameters->ParameterValues[pi][2];
			vp->params.body.f[dst_index++]=mesa_vp->Parameters->ParameterValues[pi][3];
		break;
		
		default: _mesa_problem(NULL, "Bad param type in %s", __FUNCTION__);
		}
	
	}
	
	vp->params.length=dst_index;
}
		
static unsigned long t_dst_mask(GLuint mask)
{
	unsigned long flags=0;
	
	if(mask & WRITEMASK_X) flags |= VSF_FLAG_X;
	if(mask & WRITEMASK_Y) flags |= VSF_FLAG_Y;
	if(mask & WRITEMASK_Z) flags |= VSF_FLAG_Z;
	if(mask & WRITEMASK_W) flags |= VSF_FLAG_W;
	
	return flags;
}

static unsigned long t_dst_class(enum register_file file)
{
	
	switch(file){
		case PROGRAM_TEMPORARY:
			return VSF_OUT_CLASS_TMP;
		case PROGRAM_OUTPUT:
			return VSF_OUT_CLASS_RESULT;
		/*	
		case PROGRAM_INPUT:
		case PROGRAM_LOCAL_PARAM:
		case PROGRAM_ENV_PARAM:
		case PROGRAM_NAMED_PARAM:
		case PROGRAM_STATE_VAR:
		case PROGRAM_WRITE_ONLY:
		case PROGRAM_ADDRESS:
		*/
		default:
			fprintf(stderr, "problem in %s", __FUNCTION__);
			exit(0);
	}
}

static unsigned long t_dst_index(struct r300_vertex_program *vp, struct vp_dst_register *dst)
{
	if(dst->File == PROGRAM_OUTPUT) {
		if (vp->outputs[dst->Index] != -1)
			return vp->outputs[dst->Index];
		else {
			WARN_ONCE("Unknown output %d\n", dst->Index);
			return 10;
		}
	}
	return dst->Index;
}

static unsigned long t_src_class(enum register_file file)
{
	
	switch(file){
		case PROGRAM_TEMPORARY:
			return VSF_IN_CLASS_TMP;
			
		case PROGRAM_INPUT:
			return VSF_IN_CLASS_ATTR;
			
		case PROGRAM_LOCAL_PARAM:
		case PROGRAM_ENV_PARAM:
		case PROGRAM_NAMED_PARAM:
		case PROGRAM_STATE_VAR:
			return VSF_IN_CLASS_PARAM;
		/*	
		case PROGRAM_OUTPUT:
		case PROGRAM_WRITE_ONLY:
		case PROGRAM_ADDRESS:
		*/
		default:
			fprintf(stderr, "problem in %s", __FUNCTION__);
			exit(0);
	}
}

static unsigned long t_swizzle(GLubyte swizzle)
{
	switch(swizzle){
		case SWIZZLE_X: return VSF_IN_COMPONENT_X;
		case SWIZZLE_Y: return VSF_IN_COMPONENT_Y;
		case SWIZZLE_Z: return VSF_IN_COMPONENT_Z;
		case SWIZZLE_W: return VSF_IN_COMPONENT_W;
		case SWIZZLE_ZERO: return VSF_IN_COMPONENT_ZERO;
		case SWIZZLE_ONE: return VSF_IN_COMPONENT_ONE;
		default:
			fprintf(stderr, "problem in %s", __FUNCTION__);
			exit(0);
	}
}

static void vp_dump_inputs(struct r300_vertex_program *vp, char *caller)
{
	int i;
	
	if(vp == NULL){
		fprintf(stderr, "vp null in call to %s from %s\n", __FUNCTION__, caller);
		return ;
	}
	
	fprintf(stderr, "%s:<", caller);
	for(i=0; i < VERT_ATTRIB_MAX; i++)
		fprintf(stderr, "%d ", vp->inputs[i]);
	fprintf(stderr, ">\n");
	
}

static unsigned long t_src_index(struct r300_vertex_program *vp, struct vp_src_register *src)
{
	int i;
	int max_reg=-1;
	
	if(src->File == PROGRAM_INPUT){
		if(vp->inputs[src->Index] != -1)
			return vp->inputs[src->Index];
		
		for(i=0; i < VERT_ATTRIB_MAX; i++)
			if(vp->inputs[i] > max_reg)
				max_reg=vp->inputs[i];
		
		vp->inputs[src->Index]=max_reg+1;
		
		//vp_dump_inputs(vp, __FUNCTION__);	
		
		return vp->inputs[src->Index];
	}else{
		return src->Index;
	}
}

static unsigned long t_src(struct r300_vertex_program *vp, struct vp_src_register *src)
{
	
	return MAKE_VSF_SOURCE(t_src_index(vp, src),
				t_swizzle(GET_SWZ(src->Swizzle, 0)),
				t_swizzle(GET_SWZ(src->Swizzle, 1)),
				t_swizzle(GET_SWZ(src->Swizzle, 2)),
				t_swizzle(GET_SWZ(src->Swizzle, 3)),
				t_src_class(src->File),
				src->Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
}

static unsigned long t_src_scalar(struct r300_vertex_program *vp, struct vp_src_register *src)
{
			
	return MAKE_VSF_SOURCE(t_src_index(vp, src),
				t_swizzle(GET_SWZ(src->Swizzle, 0)),
				t_swizzle(GET_SWZ(src->Swizzle, 0)),
				t_swizzle(GET_SWZ(src->Swizzle, 0)),
				t_swizzle(GET_SWZ(src->Swizzle, 0)),
				t_src_class(src->File),
				src->Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
}

static unsigned long t_opcode(enum vp_opcode opcode)
{

	switch(opcode){
		case VP_OPCODE_DST: return R300_VPI_OUT_OP_DST;
		case VP_OPCODE_EX2: return R300_VPI_OUT_OP_EX2;
		case VP_OPCODE_EXP: return R300_VPI_OUT_OP_EXP;
		case VP_OPCODE_FRC: return R300_VPI_OUT_OP_FRC;
		case VP_OPCODE_LG2: return R300_VPI_OUT_OP_LG2;
		case VP_OPCODE_LOG: return R300_VPI_OUT_OP_LOG;
		case VP_OPCODE_MAX: return R300_VPI_OUT_OP_MAX;
		case VP_OPCODE_MIN: return R300_VPI_OUT_OP_MIN;
		case VP_OPCODE_MUL: return R300_VPI_OUT_OP_MUL;
		case VP_OPCODE_POW: return R300_VPI_OUT_OP_POW;
		case VP_OPCODE_RCP: return R300_VPI_OUT_OP_RCP;
		case VP_OPCODE_RSQ: return R300_VPI_OUT_OP_RSQ;
		case VP_OPCODE_SGE: return R300_VPI_OUT_OP_SGE;
		case VP_OPCODE_SLT: return R300_VPI_OUT_OP_SLT;
		case VP_OPCODE_DP4: return R300_VPI_OUT_OP_DOT;
		
		default: 
			fprintf(stderr, "%s: Should not be called with opcode %d!", __FUNCTION__, opcode);
	}
	exit(-1);
	return 0;
}

static unsigned long op_operands(enum vp_opcode opcode)
{
	int i;
	
	/* Can we trust mesas opcodes to be in order ? */
	for(i=0; i < sizeof(op_names) / sizeof(*op_names); i++)
		if(op_names[i].opcode == opcode)
			return op_names[i].ip;
	
	fprintf(stderr, "op %d not found in op_names\n", opcode);
	exit(-1);
	return 0;
}

/* TODO: Get rid of t_src_class call */
#define CMP_SRCS(a, b) (a.Index != b.Index && \
		       ((t_src_class(a.File) == VSF_IN_CLASS_PARAM && \
			 t_src_class(b.File) == VSF_IN_CLASS_PARAM) || \
			(t_src_class(a.File) == VSF_IN_CLASS_ATTR && \
			 t_src_class(b.File) == VSF_IN_CLASS_ATTR))) \
			 
#define SRCS_WRITABLE 1
void translate_vertex_shader(struct r300_vertex_program *vp)
{
	struct vertex_program *mesa_vp=(void *)vp;
	struct vp_instruction *vpi;
	int i, cur_reg=0;
	VERTEX_SHADER_INSTRUCTION *o_inst;
	unsigned long operands;
	int are_srcs_scalar;
	unsigned long hw_op;
	/* Initial value should be last tmp reg that hw supports.
	   Strangely enough r300 doesnt mind even though these would be out of range.
	   Smart enough to realize that it doesnt need it? */
	int u_temp_i=VSF_MAX_FRAGMENT_TEMPS-1;
#ifdef SRCS_WRITABLE
	struct vp_src_register src[3];
#else	
#define src	vpi->SrcReg	
#endif			
	vp->pos_end=0; /* Not supported yet */
	vp->program.length=0;
	vp->num_temporaries=mesa_vp->Base.NumTemporaries;
	
	for(i=0; i < VERT_ATTRIB_MAX; i++)
		vp->inputs[i] = -1;

	for(i=0; i < VERT_RESULT_MAX; i++)
		vp->outputs[i] = -1;
	
	assert(mesa_vp->OutputsWritten & (1 << VERT_RESULT_HPOS));
	assert(mesa_vp->OutputsWritten & (1 << VERT_RESULT_COL0));
	
	/* Assign outputs */
	if(mesa_vp->OutputsWritten & (1 << VERT_RESULT_HPOS))
		vp->outputs[VERT_RESULT_HPOS] = cur_reg++;
	
	if(mesa_vp->OutputsWritten & (1 << VERT_RESULT_PSIZ))
		vp->outputs[VERT_RESULT_PSIZ] = cur_reg++;
	
	if(mesa_vp->OutputsWritten & (1 << VERT_RESULT_COL0))
		vp->outputs[VERT_RESULT_COL0] = cur_reg++;
	
#if 0 /* Not supported yet */
	if(mesa_vp->OutputsWritten & (1 << VERT_RESULT_BFC0))
		vp->outputs[VERT_RESULT_BFC0] = cur_reg++;
	
	if(mesa_vp->OutputsWritten & (1 << VERT_RESULT_COL1))
		vp->outputs[VERT_RESULT_COL1] = cur_reg++;
	
	if(mesa_vp->OutputsWritten & (1 << VERT_RESULT_BFC1))
		vp->outputs[VERT_RESULT_BFC1] = cur_reg++;
	
	if(mesa_vp->OutputsWritten & (1 << VERT_RESULT_FOGC))
		vp->outputs[VERT_RESULT_FOGC] = cur_reg++;
#endif
	
	for(i=VERT_RESULT_TEX0; i <= VERT_RESULT_TEX7; i++)
		if(mesa_vp->OutputsWritten & (1 << i))
			vp->outputs[i] = cur_reg++;
	
	o_inst=vp->program.body.i;
	for(vpi=mesa_vp->Instructions; vpi->Opcode != VP_OPCODE_END; vpi++, o_inst++){
		
		operands=op_operands(vpi->Opcode);
		are_srcs_scalar=operands & SCALAR_FLAG;
		operands &= OP_MASK;
		
		for(i=0; i < operands; i++)
			src[i]=vpi->SrcReg[i];
#if 1
		if(operands == 3){ /* TODO: scalars */
			if( CMP_SRCS(src[1], src[2]) || CMP_SRCS(src[0], src[2]) ){
				o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_ADD, u_temp_i,
						VSF_FLAG_ALL, VSF_OUT_CLASS_TMP);
				
				o_inst->src1=MAKE_VSF_SOURCE(t_src_index(vp, &src[2]),
						SWIZZLE_X, SWIZZLE_Y,
						SWIZZLE_Z, SWIZZLE_W,
						t_src_class(src[2].File), VSF_FLAG_NONE);

				o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[2]),
						SWIZZLE_ZERO, SWIZZLE_ZERO,
						SWIZZLE_ZERO, SWIZZLE_ZERO,
						t_src_class(src[2].File), VSF_FLAG_NONE);
				o_inst->src3=0;
				o_inst++;
						
				src[2].File=PROGRAM_TEMPORARY;
				src[2].Index=u_temp_i;
				u_temp_i--;
			}
			
		}
		if(operands >= 2){
			if( CMP_SRCS(src[1], src[0]) ){
				o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_ADD, u_temp_i,
						VSF_FLAG_ALL, VSF_OUT_CLASS_TMP);
				
				o_inst->src1=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
						SWIZZLE_X, SWIZZLE_Y,
						SWIZZLE_Z, SWIZZLE_W,
						t_src_class(src[0].File), VSF_FLAG_NONE);

				o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
						SWIZZLE_ZERO, SWIZZLE_ZERO,
						SWIZZLE_ZERO, SWIZZLE_ZERO,
						t_src_class(src[0].File), VSF_FLAG_NONE);
				o_inst->src3=0;
				o_inst++;
						
				src[0].File=PROGRAM_TEMPORARY;
				src[0].Index=u_temp_i;
				u_temp_i--;
			}
		}
#endif		
		/* these ops need special handling.
		   Ops that need temp vars should probably be given reg indexes starting at the end of tmp area. */
		switch(vpi->Opcode){
		case VP_OPCODE_MOV://ADD RESULT 1.X Y Z W PARAM 0{} {X Y Z W} PARAM 0{} {ZERO ZERO ZERO ZERO} 
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_ADD, t_dst_index(vp, &vpi->DstReg),
					t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			o_inst->src1=t_src(vp, &src[0]);
			o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					SWIZZLE_ZERO, SWIZZLE_ZERO,
					SWIZZLE_ZERO, SWIZZLE_ZERO,
					t_src_class(src[0].File), VSF_FLAG_NONE);

			o_inst->src3=0;

			goto next;
			
		case VP_OPCODE_ADD:
			hw_op=(src[0].File == PROGRAM_TEMPORARY &&
				src[1].File == PROGRAM_TEMPORARY) ? R300_VPI_OUT_OP_MAD_2 : R300_VPI_OUT_OP_MAD;
			
			o_inst->op=MAKE_VSF_OP(hw_op, t_dst_index(vp, &vpi->DstReg),
				t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			o_inst->src1=t_src(vp, &src[0]);
			o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
						SWIZZLE_ONE, SWIZZLE_ONE,
						SWIZZLE_ONE, SWIZZLE_ONE,
						t_src_class(src[0].File), VSF_FLAG_NONE);
			o_inst->src3=t_src(vp, &src[1]);
			goto next;
			
		case VP_OPCODE_MAD:
			hw_op=(src[0].File == PROGRAM_TEMPORARY &&
				src[1].File == PROGRAM_TEMPORARY &&
				src[2].File == PROGRAM_TEMPORARY) ? R300_VPI_OUT_OP_MAD_2 : R300_VPI_OUT_OP_MAD;
			
			o_inst->op=MAKE_VSF_OP(hw_op, t_dst_index(vp, &vpi->DstReg),
				t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			o_inst->src1=t_src(vp, &src[0]);
			o_inst->src2=t_src(vp, &src[1]);
			o_inst->src3=t_src(vp, &src[2]);
			goto next;
			
		case VP_OPCODE_MUL: /* HW mul can take third arg but appears to have some other limitations. */
			hw_op=(src[0].File == PROGRAM_TEMPORARY &&
				src[1].File == PROGRAM_TEMPORARY) ? R300_VPI_OUT_OP_MAD_2 : R300_VPI_OUT_OP_MAD;
			
			o_inst->op=MAKE_VSF_OP(hw_op, t_dst_index(vp, &vpi->DstReg),
				t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			o_inst->src1=t_src(vp, &src[0]);
			o_inst->src2=t_src(vp, &src[1]);

			o_inst->src3=MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
					SWIZZLE_ZERO, SWIZZLE_ZERO,
					SWIZZLE_ZERO, SWIZZLE_ZERO,
					t_src_class(src[1].File), VSF_FLAG_NONE);
			goto next;
			
		case VP_OPCODE_DP3://DOT RESULT 1.X Y Z W PARAM 0{} {X Y Z ZERO} PARAM 0{} {X Y Z ZERO} 
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_DOT, t_dst_index(vp, &vpi->DstReg),
					t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			
			o_inst->src1=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 1)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 2)),
					SWIZZLE_ZERO,
					t_src_class(src[0].File),
					src[0].Negate ? VSF_FLAG_XYZ : VSF_FLAG_NONE);
			
			o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
					t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
					t_swizzle(GET_SWZ(src[1].Swizzle, 1)),
					t_swizzle(GET_SWZ(src[1].Swizzle, 2)),
					SWIZZLE_ZERO,
					t_src_class(src[1].File),
					src[1].Negate ? VSF_FLAG_XYZ : VSF_FLAG_NONE);

			o_inst->src3=0;
			goto next;

		case VP_OPCODE_SUB://ADD RESULT 1.X Y Z W TMP 0{} {X Y Z W} PARAM 1{X Y Z W } {X Y Z W} neg Xneg Yneg Zneg W
#if 1
			hw_op=(src[0].File == PROGRAM_TEMPORARY &&
				src[1].File == PROGRAM_TEMPORARY) ? R300_VPI_OUT_OP_MAD_2 : R300_VPI_OUT_OP_MAD;
			
			o_inst->op=MAKE_VSF_OP(hw_op, t_dst_index(vp, &vpi->DstReg),
				t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			o_inst->src1=t_src(vp, &src[0]);
			o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
						SWIZZLE_ONE, SWIZZLE_ONE,
						SWIZZLE_ONE, SWIZZLE_ONE,
						t_src_class(src[0].File), VSF_FLAG_NONE);
			o_inst->src3=MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
					t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
					t_swizzle(GET_SWZ(src[1].Swizzle, 1)),
					t_swizzle(GET_SWZ(src[1].Swizzle, 2)),
					t_swizzle(GET_SWZ(src[1].Swizzle, 3)),
					t_src_class(src[1].File),
					(!src[1].Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE);
#else
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_ADD, t_dst_index(vp, &vpi->DstReg),
					t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			
			o_inst->src1=t_src(vp, &src[0]);
			o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
					t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
					t_swizzle(GET_SWZ(src[1].Swizzle, 1)),
					t_swizzle(GET_SWZ(src[1].Swizzle, 2)),
					t_swizzle(GET_SWZ(src[1].Swizzle, 3)),
					t_src_class(src[1].File),
					(!src[1].Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			o_inst->src3=0;
#endif
			goto next;
			
		case VP_OPCODE_ABS://MAX RESULT 1.X Y Z W PARAM 0{} {X Y Z W} PARAM 0{X Y Z W } {X Y Z W} neg Xneg Yneg Zneg W
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_MAX, t_dst_index(vp, &vpi->DstReg),
					t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			
			o_inst->src1=t_src(vp, &src[0]);
			o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 1)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 2)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 3)),
					t_src_class(src[0].File),
					(!src[0].Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			o_inst->src3=0;
			goto next;
			
		case VP_OPCODE_FLR:
		/* FRC TMP 0.X Y Z W PARAM 0{} {X Y Z W} 
		   ADD RESULT 1.X Y Z W PARAM 0{} {X Y Z W} TMP 0{X Y Z W } {X Y Z W} neg Xneg Yneg Zneg W */

			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_FRC, u_temp_i,
					t_dst_mask(vpi->DstReg.WriteMask), VSF_OUT_CLASS_TMP);
			
			o_inst->src1=t_src(vp, &src[0]);
			o_inst->src2=0;
			o_inst->src3=0;
			o_inst++;
			
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_ADD, t_dst_index(vp, &vpi->DstReg),
					t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			
			o_inst->src1=t_src(vp, &src[0]);
			o_inst->src2=MAKE_VSF_SOURCE(u_temp_i,
					VSF_IN_COMPONENT_X,
					VSF_IN_COMPONENT_Y,
					VSF_IN_COMPONENT_Z,
					VSF_IN_COMPONENT_W,
					VSF_IN_CLASS_TMP,
					/* Not 100% sure about this */
					(!src[1].Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE/*VSF_FLAG_ALL*/);

			o_inst->src3=0;
			u_temp_i--;
			goto next;
			
		case VP_OPCODE_LG2:// LG2 RESULT 1.X Y Z W PARAM 0{} {X X X X}
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_LG2, t_dst_index(vp, &vpi->DstReg),
					t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			
			o_inst->src1=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
					t_src_class(src[0].File),
					src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			o_inst->src2=0;
			o_inst->src3=0;
			goto next;
			
		case VP_OPCODE_LIT://LIT TMP 1.Y Z TMP 1{} {X W Z Y} TMP 1{} {Y W Z X} TMP 1{} {Y X Z W} 
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_LIT, t_dst_index(vp, &vpi->DstReg),
					t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			/* NOTE: Users swizzling might not work. */
			o_inst->src1=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)), // x
					t_swizzle(GET_SWZ(src[0].Swizzle, 3)), // w
					t_swizzle(GET_SWZ(src[0].Swizzle, 2)), // z
					t_swizzle(GET_SWZ(src[0].Swizzle, 1)), // y
					t_src_class(src[0].File),
					src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					t_swizzle(GET_SWZ(src[0].Swizzle, 1)), // y
					t_swizzle(GET_SWZ(src[0].Swizzle, 3)), // w
					t_swizzle(GET_SWZ(src[0].Swizzle, 2)), // z
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)), // x
					t_src_class(src[0].File),
					src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			o_inst->src3=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					t_swizzle(GET_SWZ(src[0].Swizzle, 1)), // y
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)), // x
					t_swizzle(GET_SWZ(src[0].Swizzle, 2)), // z
					t_swizzle(GET_SWZ(src[0].Swizzle, 3)), // w
					t_src_class(src[0].File),
					src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			goto next;
			
		case VP_OPCODE_DPH://DOT RESULT 1.X Y Z W PARAM 0{} {X Y Z ONE} PARAM 0{} {X Y Z W} 
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_DOT, t_dst_index(vp, &vpi->DstReg),
					t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			
			o_inst->src1=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 1)),
					t_swizzle(GET_SWZ(src[0].Swizzle, 2)),
					VSF_IN_COMPONENT_ONE,
					t_src_class(src[0].File),
					src[0].Negate ? VSF_FLAG_XYZ : VSF_FLAG_NONE);
			o_inst->src2=t_src(vp, &src[1]);
			o_inst->src3=0;
			goto next;
			
		case VP_OPCODE_XPD:
			/* mul r0, r1.yzxw, r2.zxyw
			   mad r0, -r2.yzxw, r1.zxyw, r0
			   NOTE: might need MAD_2
			 */
			
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_MAD, u_temp_i,
					t_dst_mask(vpi->DstReg.WriteMask), VSF_OUT_CLASS_TMP);
			
			o_inst->src1=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					t_swizzle(GET_SWZ(src[0].Swizzle, 1)), // y
					t_swizzle(GET_SWZ(src[0].Swizzle, 2)), // z
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)), // x
					t_swizzle(GET_SWZ(src[0].Swizzle, 3)), // w
					t_src_class(src[0].File),
					src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			
			o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
					t_swizzle(GET_SWZ(src[1].Swizzle, 2)), // z
					t_swizzle(GET_SWZ(src[1].Swizzle, 0)), // x
					t_swizzle(GET_SWZ(src[1].Swizzle, 1)), // y
					t_swizzle(GET_SWZ(src[1].Swizzle, 3)), // w
					t_src_class(src[1].File),
					src[1].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			
			o_inst->src3=MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
					SWIZZLE_ZERO, SWIZZLE_ZERO,
					SWIZZLE_ZERO, SWIZZLE_ZERO,
					t_src_class(src[1].File),
					VSF_FLAG_NONE);
			o_inst++;
			u_temp_i--;
			
			o_inst->op=MAKE_VSF_OP(R300_VPI_OUT_OP_MAD, t_dst_index(vp, &vpi->DstReg),
					t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
			
			o_inst->src1=MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
					t_swizzle(GET_SWZ(src[1].Swizzle, 1)), // y
					t_swizzle(GET_SWZ(src[1].Swizzle, 2)), // z
					t_swizzle(GET_SWZ(src[1].Swizzle, 0)), // x
					t_swizzle(GET_SWZ(src[1].Swizzle, 3)), // w
					t_src_class(src[1].File),
					(!src[1].Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			
			o_inst->src2=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
					t_swizzle(GET_SWZ(src[0].Swizzle, 2)), // z
					t_swizzle(GET_SWZ(src[0].Swizzle, 0)), // x
					t_swizzle(GET_SWZ(src[0].Swizzle, 1)), // y
					t_swizzle(GET_SWZ(src[0].Swizzle, 3)), // w
					t_src_class(src[0].File),
					src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
			
			o_inst->src3=MAKE_VSF_SOURCE(u_temp_i+1,
					VSF_IN_COMPONENT_X,
					VSF_IN_COMPONENT_Y,
					VSF_IN_COMPONENT_Z,
					VSF_IN_COMPONENT_W,
					VSF_IN_CLASS_TMP,
					VSF_FLAG_NONE);
		
			goto next;

		case VP_OPCODE_ARL:
		case VP_OPCODE_SWZ:
		case VP_OPCODE_RCC:
		case VP_OPCODE_PRINT:
			//vp->num_temporaries++;
			fprintf(stderr, "Dont know how to handle op %d yet\n", vpi->Opcode);
			exit(-1);
		break;
		case VP_OPCODE_END:
			break;
		default:
			break;
		}
	
		o_inst->op=MAKE_VSF_OP(t_opcode(vpi->Opcode), t_dst_index(vp, &vpi->DstReg),
				t_dst_mask(vpi->DstReg.WriteMask), t_dst_class(vpi->DstReg.File));
		
		if(are_srcs_scalar){
			switch(operands){
				case 1:
					o_inst->src1=t_src_scalar(vp, &src[0]);
					o_inst->src2=0;
					o_inst->src3=0;
				break;
				
				case 2:
					o_inst->src1=t_src_scalar(vp, &src[0]);
					o_inst->src2=t_src_scalar(vp, &src[1]);
					o_inst->src3=0;
				break;
				
				case 3:
					o_inst->src1=t_src_scalar(vp, &src[0]);
					o_inst->src2=t_src_scalar(vp, &src[1]);
					o_inst->src3=t_src_scalar(vp, &src[2]);
				break;
				
				default:
					fprintf(stderr, "scalars and op RCC not handled yet");
					exit(-1);
				break;
			}
		}else{
			switch(operands){
				case 1:
					o_inst->src1=t_src(vp, &src[0]);
					o_inst->src2=0;
					o_inst->src3=0;
				break;
			
				case 2:
					o_inst->src1=t_src(vp, &src[0]);
					o_inst->src2=t_src(vp, &src[1]);
					o_inst->src3=0;
				break;
			
				case 3:
					o_inst->src1=t_src(vp, &src[0]);
					o_inst->src2=t_src(vp, &src[1]);
					o_inst->src3=t_src(vp, &src[2]);
				break;
			
				default:
					fprintf(stderr, "scalars and op RCC not handled yet");
					exit(-1);
				break;
			}
		}
		next: ;
	}
	
	vp->program.length=(o_inst - vp->program.body.i) * 4;
	
	if(u_temp_i < vp->num_temporaries)
		vp->translated=GL_FALSE; /* temps exhausted - program cannot be run */
	else
		vp->translated=GL_TRUE;
}

