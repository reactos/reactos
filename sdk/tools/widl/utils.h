/*
 * Utility routines' prototypes etc.
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
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

#ifndef __WIDL_UTILS_H
#define __WIDL_UTILS_H

#include "widltypes.h"

#include <stddef.h>	/* size_t */

void *xmalloc(size_t);
void *xrealloc(void *, size_t);
char *xstrdup(const char *str);

#ifndef __GNUC__
#define __attribute__(X)
#endif

void parser_error(const char *s) __attribute__((noreturn));
int parser_warning(const char *s, ...) __attribute__((format (printf, 1, 2)));
void error_loc(const char *s, ...) __attribute__((format (printf, 1, 2))) __attribute__((noreturn));
void error(const char *s, ...) __attribute__((format (printf, 1, 2))) __attribute__((noreturn));
void error_loc_info(const loc_info_t *, const char *s, ...) __attribute__((format (printf, 2, 3))) __attribute__((noreturn));
void warning(const char *s, ...) __attribute__((format (printf, 1, 2)));
void warning_loc_info(const loc_info_t *, const char *s, ...) __attribute__((format (printf, 2, 3)));
void chat(const char *s, ...) __attribute__((format (printf, 1, 2)));

char *dup_basename(const char *name, const char *ext);
size_t widl_getline(char **linep, size_t *lenp, FILE *fp);

UUID *parse_uuid(const char *u);
int is_valid_uuid(const char *s);

/* typelibs expect the minor version to be stored in the higher bits and
 * major to be stored in the lower bits */
#define MAKEVERSION(major, minor) ((((minor) & 0xffff) << 16) | ((major) & 0xffff))
#define MAJORVERSION(version) ((version) & 0xffff)
#define MINORVERSION(version) (((version) >> 16) & 0xffff)

#endif
