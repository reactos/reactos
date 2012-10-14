#!/usr/bin/python2
# -*- Mode: Python; py-indent-offset: 8 -*-

# (C) Copyright Zack Rusin 2005
# All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Authors:
#    Zack Rusin <zack@kde.org>

import license
import gl_XML
import sys, getopt

class PrintGlEnums(gl_XML.gl_print_base):

	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "gl_enums.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2005 Brian Paul All Rights Reserved.""", "BRIAN PAUL")
		self.enum_table = {}


	def printRealHeader(self):
		print '#include "main/glheader.h"'
		print '#include "main/mfeatures.h"'
		print '#include "main/enums.h"'
		print '#include "main/imports.h"'
		print '#include "main/mtypes.h"'
		print ''
		print 'typedef struct {'
		print '   size_t offset;'
		print '   int n;'
		print '} enum_elt;'
		print ''
		return

	def print_code(self):
		print """
typedef int (*cfunc)(const void *, const void *);

/**
 * Compare a key name to an element in the \c all_enums array.
 *
 * \c bsearch always passes the key as the first parameter and the pointer
 * to the array element as the second parameter.  We can elimiate some
 * extra work by taking advantage of that fact.
 *
 * \param a  Pointer to the desired enum name.
 * \param b  Pointer to an element of the \c all_enums array.
 */
static int compar_name( const char *a, const enum_elt *b )
{
   return strcmp( a, & enum_string_table[ b->offset ] );
}

/**
 * Compare a key enum value to an element in the \c all_enums array.
 *
 * \c bsearch always passes the key as the first parameter and the pointer
 * to the array element as the second parameter.  We can elimiate some
 * extra work by taking advantage of that fact.
 *
 * \param a  Pointer to the desired enum name.
 * \param b  Pointer to an index into the \c all_enums array.
 */
static int compar_nr( const int *a, const unsigned *b )
{
   return a[0] - all_enums[*b].n;
}


static char token_tmp[20];

const char *_mesa_lookup_enum_by_nr( int nr )
{
   unsigned * i;

   i = (unsigned *) _mesa_bsearch(& nr, reduced_enums,
                                  Elements(reduced_enums),
                                  sizeof(reduced_enums[0]),
                                  (cfunc) compar_nr);

   if ( i != NULL ) {
      return & enum_string_table[ all_enums[ *i ].offset ];
   }
   else {
      /* this is not re-entrant safe, no big deal here */
      _mesa_snprintf(token_tmp, sizeof(token_tmp) - 1, "0x%x", nr);
      token_tmp[sizeof(token_tmp) - 1] = '\\0';
      return token_tmp;
   }
}

/**
 * Primitive names
 */
static const char *prim_names[PRIM_UNKNOWN + 1] = {
   "GL_POINTS",
   "GL_LINES",
   "GL_LINE_LOOP",
   "GL_LINE_STRIP",
   "GL_TRIANGLES",
   "GL_TRIANGLE_STRIP",
   "GL_TRIANGLE_FAN",
   "GL_QUADS",
   "GL_QUAD_STRIP",
   "GL_POLYGON",
   "outside begin/end",
   "inside unknown primitive",
   "unknown state"
};


/* Get the name of an enum given that it is a primitive type.  Avoids
 * GL_FALSE/GL_POINTS ambiguity and others.
 */
const char *
_mesa_lookup_prim_by_nr(GLuint nr)
{
   if (nr < Elements(prim_names))
      return prim_names[nr];
   else
      return "invalid mode";
}


int _mesa_lookup_enum_by_name( const char *symbol )
{
   enum_elt * f = NULL;

   if ( symbol != NULL ) {
      f = (enum_elt *) _mesa_bsearch(symbol, all_enums,
                                     Elements(all_enums),
                                     sizeof( enum_elt ),
                                     (cfunc) compar_name);
   }

   return (f != NULL) ? f->n : -1;
}

"""
		return


	def printBody(self, api_list):
		self.enum_table = {}
		for api in api_list:
			self.process_enums( api )

		keys = self.enum_table.keys()
		keys.sort()

		name_table = []
		enum_table = {}

		for enum in keys:
			low_pri = 9
			for [name, pri] in self.enum_table[ enum ]:
				name_table.append( [name, enum] )

				if pri < low_pri:
					low_pri = pri
					enum_table[enum] = name
						

		name_table.sort()

		string_offsets = {}
		i = 0;
		print 'LONGSTRING static const char enum_string_table[] = '
		for [name, enum] in name_table:
			print '   "%s\\0"' % (name)
			string_offsets[ name ] = i
			i += len(name) + 1

		print '   ;'
		print ''


		print 'static const enum_elt all_enums[%u] =' % (len(name_table))
		print '{'
		for [name, enum] in name_table:
			print '   { %5u, 0x%08X }, /* %s */' % (string_offsets[name], enum, name)
		print '};'
		print ''

		print 'static const unsigned reduced_enums[%u] =' % (len(keys))
		print '{'
		for enum in keys:
			name = enum_table[ enum ]
			if [name, enum] not in name_table:
				print '      /* Error! %s, 0x%04x */ 0,' % (name, enum)
			else:
				i = name_table.index( [name, enum] )

				print '      %4u, /* %s */' % (i, name)
		print '};'


		self.print_code()
		return


	def process_enums(self, api):
		for obj in api.enumIterateByName():
			if obj.value not in self.enum_table:
				self.enum_table[ obj.value ] = []


			enum = self.enum_table[ obj.value ]
			name = "GL_" + obj.name
			priority = obj.priority()
			already_in = False;
			for n, p in enum:
				if n == name:
					already_in = True
			if not already_in:
				enum.append( [name, priority] )


def show_usage():
	print "Usage: %s [-f input_file_name]" % sys.argv[0]
	sys.exit(1)

if __name__ == '__main__':
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:")
	except Exception,e:
		show_usage()

	api_list = []
	for (arg,val) in args:
		if arg == "-f":
			api = gl_XML.parse_GL_API( val )
			api_list.append(api);

	printer = PrintGlEnums()
	printer.Print( api_list )
