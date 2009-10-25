/*
 * Copyright (C) 2004, 2005, 2007, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: gen-unix.h,v 1.19.332.2 2009/01/18 23:47:35 tbox Exp $ */

/*! \file
 * \brief
 * This file is responsible for defining two operations that are not
 * directly portable between Unix-like systems and Windows NT, option
 * parsing and directory scanning.  It is here because it was decided
 * that the "gen" build utility was not to depend on libisc.a, so
 * the functions declared in isc/commandline.h and isc/dir.h could not
 * be used.
 *
 * The commandline stuff is really just a wrapper around getopt().
 * The dir stuff was shrunk to fit the needs of gen.c.
 */

#ifndef DNS_GEN_UNIX_H
#define DNS_GEN_UNIX_H 1

#include <sys/types.h>          /* Required on some systems for dirent.h. */

#include <dirent.h>
#include <unistd.h>		/* XXXDCL Required for ?. */

#include <isc/boolean.h>
#include <isc/lang.h>

#ifdef NEED_OPTARG
extern char *optarg;
#endif

#define isc_commandline_parse		getopt
#define isc_commandline_argument 	optarg

typedef struct {
	DIR *handle;
	char *filename;
} isc_dir_t;

ISC_LANG_BEGINDECLS

static isc_boolean_t
start_directory(const char *path, isc_dir_t *dir) {
	dir->handle = opendir(path);

	if (dir->handle != NULL)
		return (ISC_TRUE);
	else
		return (ISC_FALSE);

}

static isc_boolean_t
next_file(isc_dir_t *dir) {
	struct dirent *dirent;

	dir->filename = NULL;

	if (dir->handle != NULL) {
		dirent = readdir(dir->handle);
		if (dirent != NULL)
			dir->filename = dirent->d_name;
	}

	if (dir->filename != NULL)
		return (ISC_TRUE);
	else
		return (ISC_FALSE);
}

static void
end_directory(isc_dir_t *dir) {
	if (dir->handle != NULL)
		(void)closedir(dir->handle);

	dir->handle = NULL;
}

ISC_LANG_ENDDECLS

#endif /* DNS_GEN_UNIX_H */
