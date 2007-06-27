/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Brian Paul   All Rights Reserved.
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

/**
 * \file slang_assemble_conditional.c
 * slang condtional expressions assembler
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_assemble_conditional.h"
#include "slang_assemble.h"

/* _slang_assemble_logicaland() */

int _slang_assemble_logicaland (slang_assembly_file *file, slang_operation *op,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	/*
		and:
			<left-expression>
			jumpz zero
			<right-expression>
			jump end
		zero:
			push 0
		end:
	*/

	unsigned int zero_jump, end_jump;
	slang_assembly_stack_info stk;

	/* evaluate left expression */
	if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
		return 0;
	/* TODO: inspect stk */

	/* jump to pushing 0 if not true */
	zero_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump_if_zero))
		return 0;

	/* evaluate right expression */
	if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info, &stk))
		return 0;
	/* TODO: inspect stk */

	/* jump to the end of the expression */
	end_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* push 0 on stack */
	file->code[zero_jump].param[0] = file->count;
	if (!slang_assembly_file_push (file, slang_asm_bool_push))
		return 0;

	/* the end of the expression */
	file->code[end_jump].param[0] = file->count;

	return 1;
}

/* _slang_assemble_logicalor() */

int _slang_assemble_logicalor (slang_assembly_file *file, slang_operation *op,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	/*
		or:
			<left-expression>
			jumpz right
			push 1
			jump end
		right:
			<right-expression>
		end:
	*/

	unsigned int right_jump, end_jump;
	slang_assembly_stack_info stk;

	/* evaluate left expression */
	if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
		return 0;
	/* TODO: inspect stk */

	/* jump to evaluation of right expression if not true */
	right_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump_if_zero))
		return 0;

	/* push 1 on stack */
	if (!slang_assembly_file_push_literal (file, slang_asm_bool_push, 1.0f))
		return 0;

	/* jump to the end of the expression */
	end_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* evaluate right expression */
	file->code[right_jump].param[0] = file->count;
	if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info, &stk))
		return 0;
	/* TODO: inspect stk */

	/* the end of the expression */
	file->code[end_jump].param[0] = file->count;

	return 1;
}

/* _slang_assemble_select() */

int _slang_assemble_select (slang_assembly_file *file, slang_operation *op,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	/*
		select:
			<condition-expression>
			jumpz false
			<true-expression>
			jump end
		false:
			<false-expression>
		end:
	*/

	unsigned int cond_jump, end_jump;
	slang_assembly_stack_info stk;

	/* execute condition expression */
	if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
		return 0;
	/* TODO: inspect stk */

	/* jump to false expression if not true */
	cond_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump_if_zero))
		return 0;

	/* execute true expression */
	if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info, &stk))
		return 0;
	/* TODO: inspect stk */

	/* jump to the end of the expression */
	end_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* resolve false point */
	file->code[cond_jump].param[0] = file->count;

	/* execute false expression */
	if (!_slang_assemble_operation (file, op->children + 2, 0, flow, space, info, &stk))
		return 0;
	/* TODO: inspect stk */

	/* resolve the end of the expression */
	file->code[end_jump].param[0] = file->count;

	return 1;
}

/* _slang_assemble_for() */

int _slang_assemble_for (slang_assembly_file *file, slang_operation *op,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	/*
		for:
			<init-statement>
			jump start
		break:
			jump end
		continue:
			<loop-increment>
		start:
			<condition-statement>
			jumpz end
			<loop-body>
			jump continue
		end:
	*/

	unsigned int start_jump, end_jump, cond_jump;
	unsigned int break_label, cont_label;
	slang_assembly_flow_control loop_flow = *flow;
	slang_assembly_stack_info stk;

	/* execute initialization statement */
	if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */
	if (!_slang_cleanup_stack (file, op->children, 0, space))
		return 0;

	/* skip the "go to the end of the loop" and loop-increment statements */
	start_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* go to the end of the loop - break statements are directed here */
	break_label = file->count;
	end_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* resolve the beginning of the loop - continue statements are directed here */
	cont_label = file->count;

	/* execute loop-increment statement */
	if (!_slang_assemble_operation (file, op->children + 2, 0, flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */
	if (!_slang_cleanup_stack (file, op->children + 2, 0, space))
		return 0;

	/* resolve the condition point */
	file->code[start_jump].param[0] = file->count;

	/* execute condition statement */
	if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info, &stk))
		return 0;
	/* TODO: inspect stk */

	/* jump to the end of the loop if not true */
	cond_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump_if_zero))
		return 0;

	/* execute loop body */
	loop_flow.loop_start = cont_label;
	loop_flow.loop_end = break_label;
	if (!_slang_assemble_operation (file, op->children + 3, 0, &loop_flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */
	if (!_slang_cleanup_stack (file, op->children + 3, 0, space))
		return 0;

	/* go to the beginning of the loop */
	if (!slang_assembly_file_push_label (file, slang_asm_jump, cont_label))
		return 0;

	/* resolve the end of the loop */
	file->code[end_jump].param[0] = file->count;
	file->code[cond_jump].param[0] = file->count;

	return 1;
}

/* _slang_assemble_do() */

int _slang_assemble_do (slang_assembly_file *file, slang_operation *op,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	/*
		do:
			jump start
		break:
			jump end
		continue:
			jump condition
		start:
			<loop-body>
		condition:
			<condition-statement>
			jumpz end
			jump start
		end:
	*/

	unsigned int skip_jump, end_jump, cont_jump, cond_jump;
	unsigned int break_label, cont_label;
	slang_assembly_flow_control loop_flow = *flow;
	slang_assembly_stack_info stk;

	/* skip the "go to the end of the loop" and "go to condition" statements */
	skip_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* go to the end of the loop - break statements are directed here */
	break_label = file->count;
	end_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* go to condition - continue statements are directed here */
	cont_label = file->count;
	cont_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* resolve the beginning of the loop */
	file->code[skip_jump].param[0] = file->count;

	/* execute loop body */
	loop_flow.loop_start = cont_label;
	loop_flow.loop_end = break_label;
	if (!_slang_assemble_operation (file, op->children, 0, &loop_flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */
	if (!_slang_cleanup_stack (file, op->children, 0, space))
		return 0;

	/* resolve condition point */
	file->code[cont_jump].param[0] = file->count;

	/* execute condition statement */
	if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */

	/* jump to the end of the loop if not true */
	cond_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump_if_zero))
		return 0;

	/* jump to the beginning of the loop */
	if (!slang_assembly_file_push_label (file, slang_asm_jump, file->code[skip_jump].param[0]))
		return 0;

	/* resolve the end of the loop */
	file->code[end_jump].param[0] = file->count;
	file->code[cond_jump].param[0] = file->count;

	return 1;
}

/* _slang_assemble_while() */

int _slang_assemble_while (slang_assembly_file *file, slang_operation *op,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	/*
		while:
			jump continue
		break:
			jump end
		continue:
			<condition-statement>
			jumpz end
			<loop-body>
			jump continue
		end:
	*/

	unsigned int skip_jump, end_jump, cond_jump;
	unsigned int break_label;
	slang_assembly_flow_control loop_flow = *flow;
	slang_assembly_stack_info stk;

	/* skip the "go to the end of the loop" statement */
	skip_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* go to the end of the loop - break statements are directed here */
	break_label = file->count;
	end_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* resolve the beginning of the loop - continue statements are directed here */
	file->code[skip_jump].param[0] = file->count;

	/* execute condition statement */
	if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */

	/* jump to the end of the loop if not true */
	cond_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump_if_zero))
		return 0;

	/* execute loop body */
	loop_flow.loop_start = file->code[skip_jump].param[0];
	loop_flow.loop_end = break_label;
	if (!_slang_assemble_operation (file, op->children + 1, 0, &loop_flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */
	if (!_slang_cleanup_stack (file, op->children + 1, 0, space))
		return 0;

	/* jump to the beginning of the loop */
	if (!slang_assembly_file_push_label (file, slang_asm_jump, file->code[skip_jump].param[0]))
		return 0;

	/* resolve the end of the loop */
	file->code[end_jump].param[0] = file->count;
	file->code[cond_jump].param[0] = file->count;

	return 1;
}

/* _slang_assemble_if() */

int _slang_assemble_if (slang_assembly_file *file, slang_operation *op,
	slang_assembly_flow_control *flow, slang_assembly_name_space *space,
	slang_assembly_local_info *info)
{
	/*
		if:
			<condition-statement>
			jumpz else
			<true-statement>
			jump end
		else:
			<false-statement>
		end:
	*/

	unsigned int cond_jump, else_jump;
	slang_assembly_stack_info stk;

	/* execute condition statement */
	if (!_slang_assemble_operation (file, op->children, 0, flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */

	/* jump to false-statement if not true */
	cond_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump_if_zero))
		return 0;

	/* execute true-statement */
	if (!_slang_assemble_operation (file, op->children + 1, 0, flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */
	if (!_slang_cleanup_stack (file, op->children + 1, 0, space))
		return 0;

	/* skip if-false statement */
	else_jump = file->count;
	if (!slang_assembly_file_push (file, slang_asm_jump))
		return 0;

	/* resolve start of false-statement */
	file->code[cond_jump].param[0] = file->count;

	/* execute false-statement */
	if (!_slang_assemble_operation (file, op->children + 2, 0, flow, space, info, &stk))
		return 0;
	/* TODO: pass-in stk to cleanup */
	if (!_slang_cleanup_stack (file, op->children + 2, 0, space))
		return 0;

	/* resolve end of if-false statement */
	file->code[else_jump].param[0] = file->count;

	return 1;
}

