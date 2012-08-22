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

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "glcpp.h"
#include "main/core.h" /* for isblank() on MSVC */

void
glcpp_error (YYLTYPE *locp, glcpp_parser_t *parser, const char *fmt, ...)
{
	va_list ap;

	parser->error = 1;
	ralloc_asprintf_append(&parser->info_log, "%u:%u(%u): "
						  "preprocessor error: ",
						  locp->source,
						  locp->first_line,
						  locp->first_column);
	va_start(ap, fmt);
	ralloc_vasprintf_append(&parser->info_log, fmt, ap);
	va_end(ap);
	ralloc_strcat(&parser->info_log, "\n");
}

void
glcpp_warning (YYLTYPE *locp, glcpp_parser_t *parser, const char *fmt, ...)
{
	va_list ap;

	ralloc_asprintf_append(&parser->info_log, "%u:%u(%u): "
						  "preprocessor warning: ",
						  locp->source,
						  locp->first_line,
						  locp->first_column);
	va_start(ap, fmt);
	ralloc_vasprintf_append(&parser->info_log, fmt, ap);
	va_end(ap);
	ralloc_strcat(&parser->info_log, "\n");
}

/* Searches backwards for '^ *#' from a given starting point. */
static int
in_directive(const char *shader, const char *ptr)
{
	assert(ptr >= shader);

	/* Search backwards for '#'. If we find a \n first, it doesn't count */
	for (; ptr >= shader && *ptr != '#'; ptr--) {
		if (*ptr == '\n')
			return 0;
	}
	if (ptr >= shader) {
		/* Found '#'...look for spaces preceded by a newline */
		for (ptr--; ptr >= shader && isblank(*ptr); ptr--);
		// FIXME: I don't think the '\n' case can happen
		if (ptr < shader || *ptr == '\n')
			return 1;
	}
	return 0;
}

/* Remove any line continuation characters in preprocessing directives.
 * However, ignore any in GLSL code, as "There is no line continuation
 * character" (1.30 page 9) in GLSL.
 */
static char *
remove_line_continuations(glcpp_parser_t *ctx, const char *shader)
{
	int in_continued_line = 0;
	int extra_newlines = 0;
	char *clean = ralloc_strdup(ctx, "");
	const char *search_start = shader;
	const char *newline;
	while ((newline = strchr(search_start, '\n')) != NULL) {
		const char *backslash = NULL;

		/* # of characters preceding the newline. */
		int n = newline - shader;

		/* Find the preceding '\', if it exists */
		if (n >= 1 && newline[-1] == '\\')
			backslash = newline - 1;
		else if (n >= 2 && newline[-1] == '\r' && newline[-2] == '\\')
			backslash = newline - 2;

		/* Double backslashes don't count (the backslash is escaped) */
		if (backslash != NULL && backslash[-1] == '\\') {
			backslash = NULL;
		}

		if (backslash != NULL) {
			/* We found a line continuation, but do we care? */
			if (!in_continued_line) {
				if (in_directive(shader, backslash)) {
					in_continued_line = 1;
					extra_newlines = 0;
				}
			}
			if (in_continued_line) {
				/* Copy everything before the \ */
				ralloc_strncat(&clean, shader, backslash - shader);
				shader = newline + 1;
				extra_newlines++;
			}
		} else if (in_continued_line) {
			/* Copy everything up to and including the \n */
			ralloc_strncat(&clean, shader, newline - shader + 1);
			shader = newline + 1;
			/* Output extra newlines to make line numbers match */
			for (; extra_newlines > 0; extra_newlines--)
				ralloc_strcat(&clean, "\n");
			in_continued_line = 0;
		}
		search_start = newline + 1;
	}
	ralloc_strcat(&clean, shader);
	return clean;
}

int
preprocess(void *ralloc_ctx, const char **shader, char **info_log,
	   const struct gl_extensions *extensions, int api)
{
	int errors;
	glcpp_parser_t *parser = glcpp_parser_create (extensions, api);
	*shader = remove_line_continuations(parser, *shader);

	glcpp_lex_set_source_string (parser, *shader);

	glcpp_parser_parse (parser);

	if (parser->skip_stack)
		glcpp_error (&parser->skip_stack->loc, parser, "Unterminated #if\n");

	ralloc_strcat(info_log, parser->info_log);

	ralloc_steal(ralloc_ctx, parser->output);
	*shader = parser->output;

	errors = parser->error;
	glcpp_parser_destroy (parser);
	return errors;
}
