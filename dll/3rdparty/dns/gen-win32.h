/*
 * Copyright (C) 2004-2007, 2009  Internet Systems Consortium, Inc. ("ISC")
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

/*
 * Copyright (c) 1987, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* $Id: gen-win32.h,v 1.23.332.2 2009/01/18 23:47:35 tbox Exp $ */

/*! \file
 * \author Principal Authors: Computer Systems Research Group at UC Berkeley
 * \author Principal ISC caretaker: DCL
 */

/*
 * \note This file was adapted from the NetBSD project's source tree, RCS ID:
 *    NetBSD: getopt.c,v 1.15 1999/09/20 04:39:37 lukem Exp
 *
 * The primary change has been to rename items to the ISC namespace
 * and format in the ISC coding style.
 *
 * This file is responsible for defining two operations that are not
 * directly portable between Unix-like systems and Windows NT, option
 * parsing and directory scanning.  It is here because it was decided
 * that the "gen" build utility was not to depend on libisc.a, so
 * the functions declared in isc/commandline.h and isc/dir.h could not
 * be used.
 *
 * The commandline stuff is pretty much a straight copy from the initial
 * isc/commandline.c.  The dir stuff was shrunk to fit the needs of gen.c.
 */

#ifndef DNS_GEN_WIN32_H
#define DNS_GEN_WIN32_H 1

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <isc/boolean.h>
#include <isc/lang.h>

int isc_commandline_index = 1;		/* Index into parent argv vector. */
int isc_commandline_option;		/* Character checked for validity. */

char *isc_commandline_argument;		/* Argument associated with option. */
char *isc_commandline_progname;		/* For printing error messages. */

isc_boolean_t isc_commandline_errprint = ISC_TRUE; /* Print error messages. */
isc_boolean_t isc_commandline_reset = ISC_TRUE; /* Reset processing. */

#define BADOPT	'?'
#define BADARG	':'
#define ENDOPT	""

ISC_LANG_BEGINDECLS

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int
isc_commandline_parse(int argc, char * const *argv, const char *options) {
	static char *place = ENDOPT;
	char *option;			/* Index into *options of option. */

	/*
	 * Update scanning pointer, either because a reset was requested or
	 * the previous argv was finished.
	 */
	if (isc_commandline_reset || *place == '\0') {
		isc_commandline_reset = ISC_FALSE;

		if (isc_commandline_progname == NULL)
			isc_commandline_progname = argv[0];

		if (isc_commandline_index >= argc ||
		    *(place = argv[isc_commandline_index]) != '-') {
			/*
			 * Index out of range or points to non-option.
			 */
			place = ENDOPT;
			return (-1);
		}

		if (place[1] != '\0' && *++place == '-' && place[1] == '\0') {
			/*
			 * Found '--' to signal end of options.	 Advance
			 * index to next argv, the first non-option.
			 */
			isc_commandline_index++;
			place = ENDOPT;
			return (-1);
		}
	}

	isc_commandline_option = *place++;
	option = strchr(options, isc_commandline_option);

	/*
	 * Ensure valid option has been passed as specified by options string.
	 * '-:' is never a valid command line option because it could not
	 * distinguish ':' from the argument specifier in the options string.
	 */
	if (isc_commandline_option == ':' || option == NULL) {
		if (*place == '\0')
			isc_commandline_index++;

		if (isc_commandline_errprint && *options != ':')
			fprintf(stderr, "%s: illegal option -- %c\n",
				isc_commandline_progname,
				isc_commandline_option);

		return (BADOPT);
	}

	if (*++option != ':') {
		/*
		 * Option does not take an argument.
		 */
		isc_commandline_argument = NULL;

		/*
		 * Skip to next argv if at the end of the current argv.
		 */
		if (*place == '\0')
			++isc_commandline_index;

	} else {
		/*
		 * Option needs an argument.
		 */
		if (*place != '\0')
			/*
			 * Option is in this argv, -D1 style.
			 */
			isc_commandline_argument = place;

		else if (argc > ++isc_commandline_index)
			/*
			 * Option is next argv, -D 1 style.
			 */
			isc_commandline_argument = argv[isc_commandline_index];

		else {
			/*
			 * Argument needed, but no more argv.
			 */
			place = ENDOPT;

			/*
			 * Silent failure with "missing argument" return
			 * when ':' starts options string, per historical spec.
			 */
			if (*options == ':')
				return (BADARG);

			if (isc_commandline_errprint)
				fprintf(stderr,
				    "%s: option requires an argument -- %c\n",
				    isc_commandline_progname,
				    isc_commandline_option);

			return (BADOPT);
		}

		place = ENDOPT;

		/*
		 * Point to argv that follows argument.
		 */
		isc_commandline_index++;
	}

	return (isc_commandline_option);
}

typedef struct {
	HANDLE handle;
	WIN32_FIND_DATA	find_data;
	isc_boolean_t first_file;
	char *filename;
} isc_dir_t;

isc_boolean_t
start_directory(const char *path, isc_dir_t *dir) {
	char pattern[_MAX_PATH], *p;

	/*
	 * Need space for slash-splat and final NUL.
	 */
	if (strlen(path) + 3 > sizeof(pattern))
		return (ISC_FALSE);

	strcpy(pattern, path);

	/*
	 * Append slash (if needed) and splat.
	 */
	p = pattern + strlen(pattern);
	if (p != pattern  && p[-1] != '\\' && p[-1] != ':')
		*p++ = '\\';
	*p++ = '*';
	*p++ = '\0';

	dir->first_file = ISC_TRUE;

	dir->handle = FindFirstFile(pattern, &dir->find_data);

	if (dir->handle == INVALID_HANDLE_VALUE) {
		dir->filename = NULL;
		return (ISC_FALSE);
	} else {
		dir->filename = dir->find_data.cFileName;
		return (ISC_TRUE);
	}
}

isc_boolean_t
next_file(isc_dir_t *dir) {
	if (dir->first_file)
		dir->first_file = ISC_FALSE;

	else if (dir->handle != INVALID_HANDLE_VALUE) {
		if (FindNextFile(dir->handle, &dir->find_data) == TRUE)
			dir->filename = dir->find_data.cFileName;
		else
			dir->filename = NULL;

	} else
		dir->filename = NULL;

	if (dir->filename != NULL)
		return (ISC_TRUE);
	else
		return (ISC_FALSE);
}

void
end_directory(isc_dir_t *dir) {
	if (dir->handle != INVALID_HANDLE_VALUE)
		FindClose(dir->handle);
}

ISC_LANG_ENDDECLS

#endif /* DNS_GEN_WIN32_H */
