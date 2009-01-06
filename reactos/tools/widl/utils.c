/*
 * Utility routines
 *
 * Copyright 1998 Bertho A. Stultiens
 * Copyright 2002 Ove Kaaven
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
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"

#define CURRENT_LOCATION { input_name ? input_name : "stdin", line_number, parser_text }

static const int want_near_indication = 0;

static void make_print(char *str)
{
	while(*str)
	{
		if(!isprint(*str))
			*str = ' ';
		str++;
	}
}

static void generic_msg(const loc_info_t *loc_info, const char *s, const char *t, va_list ap)
{
	fprintf(stderr, "%s:%d: %s: ", loc_info->input_name, loc_info->line_number, t);
	vfprintf(stderr, s, ap);

	if (want_near_indication)
	{
		char *cpy;
		if(loc_info->near_text)
		{
			cpy = xstrdup(loc_info->near_text);
			make_print(cpy);
			fprintf(stderr, " near '%s'", cpy);
			free(cpy);
		}
	}
}


void error_loc(const char *s, ...)
{
	loc_info_t cur_loc = CURRENT_LOCATION;
	va_list ap;
	va_start(ap, s);
	generic_msg(&cur_loc, s, "Error", ap);
	va_end(ap);
	exit(1);
}

/* yyerror:  yacc assumes this is not newline terminated.  */
void parser_error(const char *s)
{
	error_loc("%s\n", s);
}

void error_loc_info(const loc_info_t *loc_info, const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(loc_info, s, "Error", ap);
	va_end(ap);
	exit(1);
}

int parser_warning(const char *s, ...)
{
	loc_info_t cur_loc = CURRENT_LOCATION;
	va_list ap;
	va_start(ap, s);
	generic_msg(&cur_loc, s, "Warning", ap);
	va_end(ap);
	return 0;
}

void error(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "error: ");
	vfprintf(stderr, s, ap);
	va_end(ap);
	exit(2);
}

void warning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "warning: ");
	vfprintf(stderr, s, ap);
	va_end(ap);
}

void warning_loc_info(const loc_info_t *loc_info, const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(loc_info, s, "Warning", ap);
	va_end(ap);
}

void chat(const char *s, ...)
{
	if(debuglevel & DEBUGLEVEL_CHAT)
	{
		va_list ap;
		va_start(ap, s);
		fprintf(stderr, "chat: ");
		vfprintf(stderr, s, ap);
		va_end(ap);
	}
}

char *dup_basename(const char *name, const char *ext)
{
	int namelen;
	int extlen = strlen(ext);
	char *base;
	char *slash;

	if(!name)
		name = "widl.tab";

	slash = strrchr(name, '/');
	if (!slash)
		slash = strrchr(name, '\\');

	if (slash)
		name = slash + 1;

	namelen = strlen(name);

	/* +4 for later extension and +1 for '\0' */
	base = xmalloc(namelen +4 +1);
	strcpy(base, name);
	if(!strcasecmp(name + namelen-extlen, ext))
	{
		base[namelen - extlen] = '\0';
	}
	return base;
}

size_t widl_getline(char **linep, size_t *lenp, FILE *fp)
{
    char *line = *linep;
    size_t len = *lenp;
    size_t n = 0;

    if (!line)
    {
        len = 64;
        line = xmalloc(len);
    }

    while (fgets(&line[n], len - n, fp))
    {
        n += strlen(&line[n]);
        if (line[n - 1] == '\n')
            break;
        else if (n == len - 1)
        {
            len *= 2;
            line = xrealloc(line, len);
        }
    }

    *linep = line;
    *lenp = len;
    return n;
}

void *xmalloc(size_t size)
{
    void *res;

    assert(size > 0);
    res = malloc(size);
    if(res == NULL)
    {
	error("Virtual memory exhausted.\n");
    }
    memset(res, 0x55, size);
    return res;
}


void *xrealloc(void *p, size_t size)
{
    void *res;

    assert(size > 0);
    res = realloc(p, size);
    if(res == NULL)
    {
	error("Virtual memory exhausted.\n");
    }
    return res;
}

char *xstrdup(const char *str)
{
	char *s;

	assert(str != NULL);
	s = xmalloc(strlen(str)+1);
	return strcpy(s, str);
}
