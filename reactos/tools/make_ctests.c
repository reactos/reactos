/* make_ctests.c

 * This program is a port of wine project's make_ctests script

# Script to generate a C file containing a list of tests
#
# Copyright 2002 Alexandre Julliard
# Copyright 2002 Dimitrie O. Paun
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

# ***** Keep in sync with tools/winapi/msvcmaker:_generate_testlist_c *****
*/

#include <stdio.h>
#include <stdlib.h>

const char* header =
	"/* Automatically generated file; DO NOT EDIT!! */\n"
	"\n"
	"/* stdarg.h is needed for Winelib */\n"
	"#include <stdarg.h>\n"
	"#include <stdio.h>\n"
	"#include <stdlib.h>\n"
	"#include \"windef.h\"\n"
	"#include \"winbase.h\"\n"
	"\n";

const char* middle =
	"\n"
	"struct test\n"
	"{\n"
	"    const char *name;\n"
	"    void (*func)(void);\n"
	"};\n"
	"\n"
	"static const struct test winetest_testlist[] =\n"
	"{\n";

const char* end =
	"    { 0, 0 }\n"
	"};\n"
	"\n"
	"#define WINETEST_WANT_MAIN\n"
	"#include \"wine/test.h\"\n"
	"\n";

char*
basename ( const char* filename )
{
	const char *p, *p2;
	char *out;
	size_t out_len;

	if ( filename == NULL )
	{
		fprintf ( stderr, "basename() called with null filename\n" );
		return NULL;
	}
	p = strrchr ( filename, '/' );
	if ( !p )
		p = filename;
	else
		++p;

	/* look for backslashes, too... */
	p2 = strrchr ( filename, '\\' );
	if ( p2 > p )
		p = p2 + 1;

	/* find extension... */
	p2 = strrchr ( filename, '.' );
	if ( !p2 )
		p2 = p + strlen(p);

	/* malloc a copy */
	out_len = p2-p;
	out = malloc ( out_len+1 );
	if ( out == NULL )
	{
		fprintf ( stderr, "malloc() failed\n" );
		return NULL;
	}
	memmove ( out, p, out_len );
	out[out_len] = '\0';
	return out;
}

int
main ( int argc, const char** argv )
{
	size_t i;

	printf ( "%s", header );

	for ( i = 1; i < argc; i++ )
	{
		char* test = basename(argv[i]);
		if ( test == NULL )
			return 255;
		printf ( "extern void func_%s(void);\n", test );
		free ( test );
	}

	printf ( "%s", middle );

	for ( i = 1; i < argc; i++ )
	{
		char* test = basename(argv[i]);
		if ( test == NULL )
			return 255;
		printf ( "    { \"%s\", func_%s },\n", test, test );
		free ( test );
	}

	printf ( "%s", end );

	return 0;
}
