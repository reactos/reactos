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
 * \file slang_execute.c
 * intermediate code interpreter
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"
#include "slang_assemble.h"
#include "slang_storage.h"
#include "slang_execute.h"

static void dump_instruction (FILE *f, slang_assembly *a, unsigned int i)
{
	fprintf (f, "%.5u:\t", i);

	switch (a->type)
	{
	case slang_asm_none:
		fprintf (f, "none");
		break;
	case slang_asm_float_copy:
		fprintf (f, "float_copy\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_float_move:
		fprintf (f, "float_move\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_float_push:
		fprintf (f, "float_push\t%f", a->literal);
		break;
	case slang_asm_float_deref:
		fprintf (f, "float_deref");
		break;
	case slang_asm_float_add:
		fprintf (f, "float_add");
		break;
	case slang_asm_float_multiply:
		fprintf (f, "float_multiply");
		break;
	case slang_asm_float_divide:
		fprintf (f, "float_divide");
		break;
	case slang_asm_float_negate:
		fprintf (f, "float_negate");
		break;
	case slang_asm_float_less:
		fprintf (f, "float_less");
		break;
	case slang_asm_float_equal:
		fprintf (f, "float_equal\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_float_to_int:
		fprintf (f, "float_to_int");
		break;
	case slang_asm_int_copy:
		fprintf (f, "int_copy\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_int_move:
		fprintf (f, "int_move\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_int_push:
		fprintf (f, "int_push\t%d", (GLint) a->literal);
		break;
	case slang_asm_int_deref:
		fprintf (f, "int_deref");
		break;
	case slang_asm_int_to_float:
		fprintf (f, "int_to_float");
		break;
	case slang_asm_int_to_addr:
		fprintf (f, "int_to_addr");
		break;
	case slang_asm_bool_copy:
		fprintf (f, "bool_copy\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_bool_move:
		fprintf (f, "bool_move\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_bool_push:
		fprintf (f, "bool_push\t%d", a->literal != 0.0f);
		break;
	case slang_asm_bool_deref:
		fprintf (f, "bool_deref");
		break;
	case slang_asm_addr_copy:
		fprintf (f, "addr_copy");
		break;
	case slang_asm_addr_push:
		fprintf (f, "addr_push\t%u", a->param[0]);
		break;
	case slang_asm_addr_deref:
		fprintf (f, "addr_deref");
		break;
	case slang_asm_addr_add:
		fprintf (f, "addr_add");
		break;
	case slang_asm_addr_multiply:
		fprintf (f, "addr_multiply");
		break;
	case slang_asm_jump:
		fprintf (f, "jump\t%u", a->param[0]);
		break;
	case slang_asm_jump_if_zero:
		fprintf (f, "jump_if_zero\t%u", a->param[0]);
		break;
	case slang_asm_enter:
		fprintf (f, "enter\t%u", a->param[0]);
		break;
	case slang_asm_leave:
		fprintf (f, "leave");
		break;
	case slang_asm_local_alloc:
		fprintf (f, "local_alloc\t%u", a->param[0]);
		break;
	case slang_asm_local_free:
		fprintf (f, "local_free\t%u", a->param[0]);
		break;
	case slang_asm_local_addr:
		fprintf (f, "local_addr\t%u, %u", a->param[0], a->param[1]);
		break;
	case slang_asm_call:
		fprintf (f, "call\t%u", a->param[0]);
		break;
	case slang_asm_return:
		fprintf (f, "return");
		break;
	case slang_asm_discard:
		fprintf (f, "discard");
		break;
	case slang_asm_exit:
		fprintf (f, "exit");
		break;
	default:
		break;
	}

	fprintf (f, "\n");
}

static void dump (const slang_assembly_file *file)
{
	unsigned int i;
	static unsigned int counter = 0;
	FILE *f;
	char filename[256];

	counter++;
	sprintf (filename, "~mesa-slang-assembly-dump-(%u).txt", counter);
	f = fopen (filename, "w");
	if (f == NULL)
		return;

	for (i = 0; i < file->count; i++)
		dump_instruction (f, file->code + i, i);

	fclose (f);
}

int _slang_execute (const slang_assembly_file *file)
{
	slang_machine mach;
	FILE *f;

	mach.ip = 0;
	mach.sp = SLANG_MACHINE_STACK_SIZE;
	mach.bp = 0;
	mach.kill = 0;
	mach.exit = 0;

	/* assume 32-bit machine */
	/* XXX why???, disabling the pointer size assertions here.
	 * See bug 4021.
	 */
	static_assert(sizeof (GLfloat) == 4);
	/*static_assert(sizeof (GLfloat *) == 4);*/
	static_assert(sizeof (GLuint) == 4);
	/*static_assert(sizeof (GLuint *) == 4);*/

	dump (file);

	f = fopen ("~mesa-slang-assembly-execution.txt", "w");

	while (!mach.exit)
	{
		slang_assembly *a = file->code + mach.ip;
		if (f != NULL)
		{
			unsigned int i;
			dump_instruction (f, a, mach.ip);
			fprintf (f, "\t\tsp=%u bp=%u\n", mach.sp, mach.bp);
			for (i = mach.sp; i < SLANG_MACHINE_STACK_SIZE; i++)
				fprintf (f, "\t%.5u\t%6f\t%u\n", i, mach.stack._float[i], mach.stack._addr[i]);
			fflush (f);
		}
		mach.ip++;

		switch (a->type)
		{
		case slang_asm_none:
			break;
		case slang_asm_float_copy:
		case slang_asm_int_copy:
		case slang_asm_bool_copy:
			*(mach.stack._floatp[mach.sp + a->param[0] / 4] + a->param[1] / 4) =
				mach.stack._float[mach.sp];
			mach.sp++;
			break;
		case slang_asm_float_move:
		case slang_asm_int_move:
		case slang_asm_bool_move:
			mach.stack._float[mach.sp + a->param[0] / 4] =
				mach.stack._float[mach.sp + (mach.stack._addr[mach.sp] + a->param[1]) / 4];
			break;
		case slang_asm_float_push:
		case slang_asm_int_push:
		case slang_asm_bool_push:
			mach.sp--;
			mach.stack._float[mach.sp] = a->literal;
			break;
		case slang_asm_float_deref:
		case slang_asm_int_deref:
		case slang_asm_bool_deref:
			mach.stack._float[mach.sp] = *mach.stack._floatp[mach.sp];
			break;
		case slang_asm_float_add:
			mach.stack._float[mach.sp + 1] += mach.stack._float[mach.sp];
			mach.sp++;
			break;
		case slang_asm_float_multiply:
			mach.stack._float[mach.sp + 1] *= mach.stack._float[mach.sp];
			mach.sp++;
			break;
		case slang_asm_float_divide:
			mach.stack._float[mach.sp + 1] /= mach.stack._float[mach.sp];
			mach.sp++;
			break;
		case slang_asm_float_negate:
			mach.stack._float[mach.sp] = -mach.stack._float[mach.sp];
			break;
		case slang_asm_float_less:
			mach.stack._float[mach.sp + 1] =
				mach.stack._float[mach.sp + 1] < mach.stack._float[mach.sp] ? 1.0f : 0.0f;
			mach.sp++;
			break;
		case slang_asm_float_equal:
			mach.sp--;
			mach.stack._float[mach.sp] = mach.stack._float[mach.sp + 1 + a->param[0] / 4] ==
				mach.stack._float[mach.sp + 1 + a->param[1] / 4] ? 1.0f : 0.0f;
			break;
		case slang_asm_float_to_int:
			mach.stack._float[mach.sp] = (GLfloat) (GLint) mach.stack._float[mach.sp];
			break;
		case slang_asm_int_to_float:
			break;
		case slang_asm_int_to_addr:
			mach.stack._addr[mach.sp] = (GLuint) (GLint) mach.stack._float[mach.sp];
			break;
		case slang_asm_addr_copy:
			*mach.stack._addrp[mach.sp + 1] = mach.stack._addr[mach.sp];
			mach.sp++;
			break;
		case slang_asm_addr_push:
			mach.sp--;
			mach.stack._addr[mach.sp] = a->param[0];
			break;
		case slang_asm_addr_deref:
			mach.stack._addr[mach.sp] = *mach.stack._addrp[mach.sp];
			break;
		case slang_asm_addr_add:
			mach.stack._addr[mach.sp + 1] += mach.stack._addr[mach.sp];
			mach.sp++;
			break;
		case slang_asm_addr_multiply:
			mach.stack._addr[mach.sp + 1] *= mach.stack._addr[mach.sp];
			mach.sp++;
			break;
		case slang_asm_jump:
			mach.ip = a->param[0];
			break;
		case slang_asm_jump_if_zero:
			if (mach.stack._float[mach.sp] == 0.0f)
				mach.ip = a->param[0];
			mach.sp++;
			break;
		case slang_asm_enter:
			mach.sp--;
			mach.stack._addr[mach.sp] = mach.bp;
			mach.bp = mach.sp + a->param[0] / 4;
			break;
		case slang_asm_leave:
			mach.bp = mach.stack._addr[mach.sp];
			mach.sp++;
			break;
		case slang_asm_local_alloc:
			mach.sp -= a->param[0] / 4;
			break;
		case slang_asm_local_free:
			mach.sp += a->param[0] / 4;
			break;
		case slang_asm_local_addr:
			mach.sp--;
			mach.stack._addr[mach.sp] = (GLuint) mach.stack._addr + mach.bp * 4 -
				(a->param[0] + a->param[1]) + 4;
			break;
		case slang_asm_call:
			mach.sp--;
			mach.stack._addr[mach.sp] = mach.ip;
			mach.ip = a->param[0];
			break;
		case slang_asm_return:
			mach.ip = mach.stack._addr[mach.sp];
			mach.sp++;
			break;
		case slang_asm_discard:
			mach.kill = 1;
			break;
		case slang_asm_exit:
			mach.exit = 1;
			break;
		}
	}

	if (f != NULL)
		fclose (f);

	return 0;
}

