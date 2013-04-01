#!/usr/bin/env python

# (C) Copyright IBM Corporation 2004, 2005
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

import gl_XML, glX_XML
import license
import sys, getopt

class PrintGlOffsets(gl_XML.gl_print_base):
	def __init__(self, es=False):
		gl_XML.gl_print_base.__init__(self)

		self.name = "gl_apitemp.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")

		self.es = es

		self.undef_list.append( "KEYWORD1" )
		self.undef_list.append( "KEYWORD1_ALT" )
		self.undef_list.append( "KEYWORD2" )
		self.undef_list.append( "NAME" )
		self.undef_list.append( "DISPATCH" )
		self.undef_list.append( "RETURN_DISPATCH" )
		self.undef_list.append( "DISPATCH_TABLE_NAME" )
		self.undef_list.append( "UNUSED_TABLE_NAME" )
		self.undef_list.append( "TABLE_ENTRY" )


	def printFunction(self, f, name):
		p_string = ""
		o_string = ""
		t_string = ""
		comma = ""

		if f.is_static_entry_point(name):
			keyword = "KEYWORD1"
		else:
			keyword = "KEYWORD1_ALT"

		n = f.static_name(name)

		silence = ''
		space = ''
		for p in f.parameterIterator():
			if p.is_padding:
				continue

			if p.is_pointer():
				cast = "(const void *) "
			else:
				cast = ""

			t_string = t_string + comma + p.format_string()
			p_string = p_string + comma + p.name
			o_string = o_string + comma + cast + p.name
			comma = ", "

			silence += "%s(void) %s;" % (space, p.name);
			space = ' '


		if f.return_type != 'void':
			dispatch = "RETURN_DISPATCH"
		else:
			dispatch = "DISPATCH"

		need_proto = False
		if not f.is_static_entry_point(name):
			need_proto = True
		elif self.es:
			cat, num = api.get_category_for_name(name)
			if (cat.startswith("es") or cat.startswith("GL_OES")):
				need_proto = True
		if need_proto:
			print '%s %s KEYWORD2 NAME(%s)(%s);' % (keyword, f.return_type, n, f.get_parameter_string(name))
			print ''

		print '%s %s KEYWORD2 NAME(%s)(%s)' % (keyword, f.return_type, n, f.get_parameter_string(name))
		print '{'
		if silence:
			print '    %s' % (silence)
		if p_string == "":
			print '   %s(%s, (), (F, "gl%s();\\n"));' \
				% (dispatch, f.name, name)
		else:
			print '   %s(%s, (%s), (F, "gl%s(%s);\\n", %s));' \
				% (dispatch, f.name, p_string, name, t_string, o_string)
		print '}'
		print ''
		return

	def printRealHeader(self):
		print ''
		self.printVisibility( "HIDDEN", "hidden" )
		print """
/*
 * This file is a template which generates the OpenGL API entry point
 * functions.  It should be included by a .c file which first defines
 * the following macros:
 *   KEYWORD1 - usually nothing, but might be __declspec(dllexport) on Win32
 *   KEYWORD2 - usually nothing, but might be __stdcall on Win32
 *   NAME(n)  - builds the final function name (usually add "gl" prefix)
 *   DISPATCH(func, args, msg) - code to do dispatch of named function.
 *                               msg is a printf-style debug message.
 *   RETURN_DISPATCH(func, args, msg) - code to do dispatch with a return value
 *
 * Here is an example which generates the usual OpenGL functions:
 *   #define KEYWORD1
 *   #define KEYWORD2
 *   #define NAME(func)  gl##func
 *   #define DISPATCH(func, args, msg)                           \\
 *          struct _glapi_table *dispatch = CurrentDispatch;     \\
 *          (*dispatch->func) args
 *   #define RETURN DISPATCH(func, args, msg)                    \\
 *          struct _glapi_table *dispatch = CurrentDispatch;     \\
 *          return (*dispatch->func) args
 *
 */


#if defined( NAME )
#ifndef KEYWORD1
#define KEYWORD1
#endif

#ifndef KEYWORD1_ALT
#define KEYWORD1_ALT HIDDEN
#endif

#ifndef KEYWORD2
#define KEYWORD2
#endif

#ifndef DISPATCH
#error DISPATCH must be defined
#endif

#ifndef RETURN_DISPATCH
#error RETURN_DISPATCH must be defined
#endif

"""
		return

    

	def printInitDispatch(self, api):
		print """
#endif /* defined( NAME ) */

/*
 * This is how a dispatch table can be initialized with all the functions
 * we generated above.
 */
#ifdef DISPATCH_TABLE_NAME

#ifndef TABLE_ENTRY
#error TABLE_ENTRY must be defined
#endif

#ifdef _GLAPI_SKIP_NORMAL_ENTRY_POINTS
#error _GLAPI_SKIP_NORMAL_ENTRY_POINTS must not be defined
#endif

_glapi_proc DISPATCH_TABLE_NAME[] = {"""
		for f in api.functionIterateByOffset():
			print '   TABLE_ENTRY(%s),' % (f.dispatch_name())

		print '   /* A whole bunch of no-op functions.  These might be called'
		print '    * when someone tries to call a dynamically-registered'
		print '    * extension function without a current rendering context.'
		print '    */'
		for i in range(1, 100):
			print '   TABLE_ENTRY(Unused),'

		print '};'
		print '#endif /* DISPATCH_TABLE_NAME */'
		print ''
		return


	def printAliasedTable(self, api):
		print """
/*
 * This is just used to silence compiler warnings.
 * We list the functions which are not otherwise used.
 */
#ifdef UNUSED_TABLE_NAME
_glapi_proc UNUSED_TABLE_NAME[] = {"""

		normal_entries = []
		proto_entries = []
		for f in api.functionIterateByOffset():
			normal_ents, proto_ents = self.classifyEntryPoints(f)

			# exclude f.name
			if f.name in normal_ents:
				normal_ents.remove(f.name)
			elif f.name in proto_ents:
				proto_ents.remove(f.name)

			normal_ents = [f.static_name(ent) for ent in normal_ents]
			proto_ents = [f.static_name(ent) for ent in proto_ents]

			normal_entries.extend(normal_ents)
			proto_entries.extend(proto_ents)

		print '#ifndef _GLAPI_SKIP_NORMAL_ENTRY_POINTS'
		for ent in normal_entries:
			print '   TABLE_ENTRY(%s),' % (ent)
		print '#endif /* _GLAPI_SKIP_NORMAL_ENTRY_POINTS */'
		print '#ifndef _GLAPI_SKIP_PROTO_ENTRY_POINTS'
		for ent in proto_entries:
			print '   TABLE_ENTRY(%s),' % (ent)
		print '#endif /* _GLAPI_SKIP_PROTO_ENTRY_POINTS */'

		print '};'
		print '#endif /*UNUSED_TABLE_NAME*/'
		print ''
		return


	def classifyEntryPoints(self, func):
		normal_names = []
		normal_stubs = []
		proto_names = []
		proto_stubs = []
		# classify the entry points
		for name in func.entry_points:
			if func.has_different_protocol(name):
				if func.is_static_entry_point(name):
					proto_names.append(name)
				else:
					proto_stubs.append(name)
			else:
				if func.is_static_entry_point(name):
					normal_names.append(name)
				else:
					normal_stubs.append(name)
		# there can be at most one stub for a function
		if normal_stubs:
			normal_names.append(normal_stubs[0])
		elif proto_stubs:
			proto_names.append(proto_stubs[0])

		return (normal_names, proto_names)

	def printBody(self, api):
		normal_entry_points = []
		proto_entry_points = []
		for func in api.functionIterateByOffset():
			normal_ents, proto_ents = self.classifyEntryPoints(func)
			normal_entry_points.append((func, normal_ents))
			proto_entry_points.append((func, proto_ents))

		print '#ifndef _GLAPI_SKIP_NORMAL_ENTRY_POINTS'
		print ''
		for func, ents in normal_entry_points:
			for ent in ents:
				self.printFunction(func, ent)
		print ''
		print '#endif /* _GLAPI_SKIP_NORMAL_ENTRY_POINTS */'
		print ''
		print '/* these entry points might require different protocols */'
		print '#ifndef _GLAPI_SKIP_PROTO_ENTRY_POINTS'
		print ''
		for func, ents in proto_entry_points:
			for ent in ents:
				self.printFunction(func, ent)
		print ''
		print '#endif /* _GLAPI_SKIP_PROTO_ENTRY_POINTS */'
		print ''

		self.printInitDispatch(api)
		self.printAliasedTable(api)
		return


def show_usage():
	print "Usage: %s [-f input_file_name] [-c]" % sys.argv[0]
	print "-c          Enable compatibility with OpenGL ES."
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:c")
	except Exception,e:
		show_usage()

	es = False
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-c":
			es = True

	api = gl_XML.parse_GL_API(file_name, glX_XML.glx_item_factory())

	printer = PrintGlOffsets(es)
	printer.Print(api)
