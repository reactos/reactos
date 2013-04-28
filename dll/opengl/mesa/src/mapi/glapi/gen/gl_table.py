#!/usr/bin/python2

# (C) Copyright IBM Corporation 2004
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
#    Ian Romanick <idr@us.ibm.com>

import gl_XML
import license
import sys, getopt

class PrintGlTable(gl_XML.gl_print_base):
	def __init__(self, es=False):
		gl_XML.gl_print_base.__init__(self)

		self.es = es
		self.header_tag = '_GLAPI_TABLE_H_'
		self.name = "gl_table.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")
		return


	def printBody(self, api):
		for f in api.functionIterateByOffset():
			arg_string = f.get_parameter_string()
			print '   %s (GLAPIENTRYP %s)(%s); /* %d */' % (f.return_type, f.name, arg_string, f.offset)


	def printRealHeader(self):
		print '#ifndef GLAPIENTRYP'
		print '# ifndef GLAPIENTRY'
		print '#  define GLAPIENTRY'
		print '# endif'
		print ''
		print '# define GLAPIENTRYP GLAPIENTRY *'
		print '#endif'
		print ''
		print ''
		print 'struct _glapi_table'
		print '{'
		return


	def printRealFooter(self):
		print '};'
		return


class PrintRemapTable(gl_XML.gl_print_base):
	def __init__(self, es=False):
		gl_XML.gl_print_base.__init__(self)

		self.es = es
		self.header_tag = '_DISPATCH_H_'
		self.name = "gl_table.py (from Mesa)"
		self.license = license.bsd_license_template % ("(C) Copyright IBM Corporation 2005", "IBM")
		return


	def printRealHeader(self):
		print """
/**
 * \\file main/dispatch.h
 * Macros for handling GL dispatch tables.
 *
 * For each known GL function, there are 3 macros in this file.  The first
 * macro is named CALL_FuncName and is used to call that GL function using
 * the specified dispatch table.  The other 2 macros, called GET_FuncName
 * can SET_FuncName, are used to get and set the dispatch pointer for the
 * named function in the specified dispatch table.
 */

/* GLXEXT is defined when building the GLX extension in the xserver.
 */
#if !defined(GLXEXT)
#include "main/mfeatures.h"
#endif
"""
		return

	def printBody(self, api):
		print '#define CALL_by_offset(disp, cast, offset, parameters) \\'
		print '    (*(cast (GET_by_offset(disp, offset)))) parameters'
		print '#define GET_by_offset(disp, offset) \\'
		print '    (offset >= 0) ? (((_glapi_proc *)(disp))[offset]) : NULL'
		print '#define SET_by_offset(disp, offset, fn) \\'
		print '    do { \\'
		print '        if ( (offset) < 0 ) { \\'
		print '            /* fprintf( stderr, "[%s:%u] SET_by_offset(%p, %d, %s)!\\n", */ \\'
		print '            /*         __func__, __LINE__, disp, offset, # fn); */ \\'
		print '            /* abort(); */ \\'
		print '        } \\'
		print '        else { \\'
		print '            ( (_glapi_proc *) (disp) )[offset] = (_glapi_proc) fn; \\'
		print '        } \\'
		print '    } while(0)'
		print ''

		functions = []
		abi_functions = []
		alias_functions = []
		count = 0
		for f in api.functionIterateByOffset():
			if not f.is_abi():
				functions.append( [f, count] )
				count += 1
			else:
				abi_functions.append( [f, -1] )

			if self.es:
				# remember functions with aliases
				if len(f.entry_points) > 1:
					alias_functions.append(f)

		print '/* total number of offsets below */'
		print '#define _gloffset_COUNT %d' % (len(abi_functions + functions))
		print ''

		for f, index in abi_functions:
			print '#define _gloffset_%s %d' % (f.name, f.offset)

		print ''
		print '#if !FEATURE_remap_table'
		print ''

		for f, index in functions:
			print '#define _gloffset_%s %d' % (f.name, f.offset)

		print ''
		print '#else /* !FEATURE_remap_table */'
		print ''

		if self.es:
			remap_table = "esLocalRemapTable"

			print '#define %s_size %u' % (remap_table, count)
			print 'static int %s[ %s_size ];' % (remap_table, remap_table)
			print ''
		else:
			remap_table = "driDispatchRemapTable"

			print '#define %s_size %u' % (remap_table, count)
			print 'extern int %s[ %s_size ];' % (remap_table, remap_table)
			print ''

		for f, index in functions:
			print '#define %s_remap_index %u' % (f.name, index)

		print ''

		for f, index in functions:
			print '#define _gloffset_%s %s[%s_remap_index]' % (f.name, remap_table, f.name)

		print ''
		print '#endif /* !FEATURE_remap_table */'
		print ''

		for f, index in abi_functions + functions:
			arg_string = gl_XML.create_parameter_string( f.parameters, 0 )

			print 'typedef %s (GLAPIENTRYP _glptr_%s)(%s);' % (f.return_type, f.name, arg_string)
			print '#define CALL_%s(disp, parameters) \\' % (f.name)
			print '    (* GET_%s(disp)) parameters' % (f.name)
			print 'static inline _glptr_%s GET_%s(struct _glapi_table *disp) {' % (f.name, f.name)
			print '   return (_glptr_%s) (GET_by_offset(disp, _gloffset_%s));' % (f.name, f.name)
			print '}'
			print
			print 'static inline void SET_%s(struct _glapi_table *disp, %s (GLAPIENTRYP fn)(%s)) {' % (f.name, f.return_type, arg_string)
			print '   SET_by_offset(disp, _gloffset_%s, fn);' % (f.name)
			print '}'
			print

		if alias_functions:
			print ''
			print '/* define aliases for compatibility */'
			for f in alias_functions:
				for name in f.entry_points:
					if name != f.name:
						print '#define CALL_%s(disp, parameters) CALL_%s(disp, parameters)' % (name, f.name)
						print '#define GET_%s(disp) GET_%s(disp)' % (name, f.name)
						print '#define SET_%s(disp, fn) SET_%s(disp, fn)' % (name, f.name)
			print ''

			print '#if FEATURE_remap_table'
			for f in alias_functions:
				for name in f.entry_points:
					if name != f.name:
						print '#define %s_remap_index %s_remap_index' % (name, f.name)
			print '#endif /* FEATURE_remap_table */'
			print ''

		return


def show_usage():
	print "Usage: %s [-f input_file_name] [-m mode] [-c ver]" % sys.argv[0]
	print "    -m mode   Mode can be 'table' or 'remap_table'."
	print "    -c ver    Version can be 'es1' or 'es2'."
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m:c:")
	except Exception,e:
		show_usage()

	mode = "table"
	es = None
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-m":
			mode = val
		elif arg == "-c":
			es = val

	if mode == "table":
		printer = PrintGlTable(es)
	elif mode == "remap_table":
		printer = PrintRemapTable(es)
	else:
		show_usage()

	api = gl_XML.parse_GL_API( file_name )

	if es is not None:
		import gles_api

		api_map = {
			'es1': gles_api.es1_api,
			'es2': gles_api.es2_api,
		}

		api.filter_functions(api_map[es])

	printer.Print( api )
