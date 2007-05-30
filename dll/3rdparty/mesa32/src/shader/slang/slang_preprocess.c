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
 * \file slang_preprocess.c
 * slang preprocessor
 * \author Michal Krol
 */

#include "imports.h"
#include "grammar_mesa.h"
#include "slang_compile.h"
#include "slang_preprocess.h"

static const char *slang_version_syn =
#include "library/slang_version_syn.h"
;

int _slang_preprocess_version (const char *text, unsigned int *version, unsigned int *eaten,
	slang_info_log *log)
{
	grammar id;
	byte *prod, *I;
	unsigned int size;

	id = grammar_load_from_text ((const byte *) slang_version_syn);
	if (id == 0)
	{
		char buf[1024];
		unsigned int pos;
		grammar_get_last_error ( (unsigned char*) buf, 1024, (int*) &pos);
		slang_info_log_error (log, buf);
		return 0;
	}

	if (!grammar_fast_check (id, (const byte *) text, &prod, &size, 8))
	{
		char buf[1024];
		unsigned int pos;
		grammar_get_last_error ( (unsigned char*) buf, 1024, (int*) &pos);
		slang_info_log_error (log, buf);
		grammar_destroy (id);
		return 0;
	}

	grammar_destroy (id);

	/* there can be multiple #version directives - grab the last one */
	I = prod;
	while (I < prod + size)
	{
		*version =
			(unsigned int) I[0] +
			(unsigned int) I[1] * 100;
		*eaten =
			((unsigned int) I[2]) +
			((unsigned int) I[3] << 8) +
			((unsigned int) I[4] << 16) +
			((unsigned int) I[5] << 24);
		I += 6;
	}

	grammar_alloc_free (prod);
	return 1;
}

