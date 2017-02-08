/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "glcpp.h"
#include "main/mtypes.h"
#include "main/shaderobj.h"

extern int yydebug;

void
_mesa_reference_shader(struct gl_context *ctx, struct gl_shader **ptr,
                       struct gl_shader *sh)
{
   (void) ctx;
   *ptr = sh;
}

/* Read from fp until EOF and return a string of everything read.
 */
static char *
load_text_fp (void *ctx, FILE *fp)
{
#define CHUNK 4096
	char *text = NULL;
	size_t text_size = 0;
	size_t total_read = 0;
	size_t bytes;

	while (1) {
		if (total_read + CHUNK + 1 > text_size) {
			text_size = text_size ? text_size * 2 : CHUNK + 1;
			text = reralloc_size (ctx, text, text_size);
			if (text == NULL) {
				fprintf (stderr, "Out of memory\n");
				return NULL;
			}
		}
		bytes = fread (text + total_read, 1, CHUNK, fp);
		total_read += bytes;

		if (bytes < CHUNK) {
			break;
		}
	}

	text[total_read] = '\0';

	return text;
}

static char *
load_text_file(void *ctx, const char *filename)
{
	char *text;
	FILE *fp;

	if (filename == NULL || strcmp (filename, "-") == 0)
		return load_text_fp (ctx, stdin);

	fp = fopen (filename, "r");
	if (fp == NULL) {
		fprintf (stderr, "Failed to open file %s: %s\n",
			 filename, strerror (errno));
		return NULL;
	}

	text = load_text_fp (ctx, fp);

	fclose(fp);

	return text;
}

int
main (int argc, char *argv[])
{
	char *filename = NULL;
	void *ctx = ralloc(NULL, void*);
	char *info_log = ralloc_strdup(ctx, "");
	const char *shader;
	int ret;

	if (argc) {
		filename = argv[1];
	}

	shader = load_text_file (ctx, filename);
	if (shader == NULL)
	   return 1;

	ret = preprocess(ctx, &shader, &info_log, NULL, API_OPENGL);

	printf("%s", shader);
	fprintf(stderr, "%s", info_log);

	ralloc_free(ctx);

	return ret;
}
