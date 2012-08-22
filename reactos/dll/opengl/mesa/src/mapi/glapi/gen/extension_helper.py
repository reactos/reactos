#!/usr/bin/env python

# (C) Copyright IBM Corporation 2005
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
import sys, getopt, string

vtxfmt = [
    "ArrayElement", \
    "Color3f", \
    "Color3fv", \
    "Color4f", \
    "Color4fv", \
    "EdgeFlag", \
    "EdgeFlagv", \
    "EvalCoord1f", \
    "EvalCoord1fv", \
    "EvalCoord2f", \
    "EvalCoord2fv", \
    "EvalPoint1", \
    "EvalPoint2", \
    "FogCoordfEXT", \
    "FogCoordfvEXT", \
    "Indexf", \
    "Indexfv", \
    "Materialfv", \
    "MultiTexCoord1fARB", \
    "MultiTexCoord1fvARB", \
    "MultiTexCoord2fARB", \
    "MultiTexCoord2fvARB", \
    "MultiTexCoord3fARB", \
    "MultiTexCoord3fvARB", \
    "MultiTexCoord4fARB", \
    "MultiTexCoord4fvARB", \
    "Normal3f", \
    "Normal3fv", \
    "SecondaryColor3fEXT", \
    "SecondaryColor3fvEXT", \
    "TexCoord1f", \
    "TexCoord1fv", \
    "TexCoord2f", \
    "TexCoord2fv", \
    "TexCoord3f", \
    "TexCoord3fv", \
    "TexCoord4f", \
    "TexCoord4fv", \
    "Vertex2f", \
    "Vertex2fv", \
    "Vertex3f", \
    "Vertex3fv", \
    "Vertex4f", \
    "Vertex4fv", \
    "CallList", \
    "CallLists", \
    "Begin", \
    "End", \
    "VertexAttrib1fNV", \
    "VertexAttrib1fvNV", \
    "VertexAttrib2fNV", \
    "VertexAttrib2fvNV", \
    "VertexAttrib3fNV", \
    "VertexAttrib3fvNV", \
    "VertexAttrib4fNV", \
    "VertexAttrib4fvNV", \
    "VertexAttrib1fARB", \
    "VertexAttrib1fvARB", \
    "VertexAttrib2fARB", \
    "VertexAttrib2fvARB", \
    "VertexAttrib3fARB", \
    "VertexAttrib3fvARB", \
    "VertexAttrib4fARB", \
    "VertexAttrib4fvARB", \
    "Rectf", \
    "DrawArrays", \
    "DrawElements", \
    "DrawRangeElements", \
    "EvalMesh1", \
    "EvalMesh2", \
]

def all_entrypoints_in_abi(f, abi, api):
	for n in f.entry_points:
		[category, num] = api.get_category_for_name( n )
		if category not in abi:
			return 0

	return 1


def any_entrypoints_in_abi(f, abi, api):
	for n in f.entry_points:
		[category, num] = api.get_category_for_name( n )
		if category in abi:
			return 1

	return 0


def condition_for_function(f, abi, all_not_in_ABI):
	"""Create a C-preprocessor condition for the function.
	
	There are two modes of operation.  If all_not_in_ABI is set, a
	condition is only created is all of the entry-point names for f are
	not in the selected ABI.  If all_not_in_ABI is not set, a condition
	is created if any entryp-point name is not in the selected ABI.
	"""

	condition = []
	for n in f.entry_points:
		[category, num] = api.get_category_for_name( n )
		if category not in abi:
			condition.append( 'defined(need_%s)' % (gl_XML.real_category_name( category )) )
		elif all_not_in_ABI:
			return []

	return condition


class PrintGlExtensionGlue(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "extension_helper.py (from Mesa)"
		self.license = license.bsd_license_template % ("(C) Copyright IBM Corporation 2005", "IBM")
		return


	def printRealHeader(self):
		print '#include "utils.h"'
		print '#include "main/dispatch.h"'
		print ''
		return


	def printBody(self, api):
		abi = [ "1.0", "1.1", "1.2", "GL_ARB_multitexture" ]

		category_list = {}

		print '#ifndef NULL'
		print '# define NULL 0'
		print '#endif'
		print ''

		for f in api.functionIterateAll():
			condition = condition_for_function(f, abi, 0)
			if len(condition):
				print '#if %s' % (string.join(condition, " || "))
				print 'static const char %s_names[] =' % (f.name)

				parameter_signature = ''
				for p in f.parameterIterator():
					if p.is_padding:
						continue

					# FIXME: This is a *really* ugly hack. :(

					tn = p.type_expr.get_base_type_node()
					if p.is_pointer():
						parameter_signature += 'p'
					elif tn.integer:
						parameter_signature += 'i'
					elif tn.size == 4:
						parameter_signature += 'f'
					else:
						parameter_signature += 'd'

				print '    "%s\\0" /* Parameter signature */' % (parameter_signature)

				for n in f.entry_points:
					print '    "gl%s\\0"' % (n)

					[category, num] = api.get_category_for_name( n )
					if category not in abi:
						c = gl_XML.real_category_name(category)
						if not category_list.has_key(c):
							category_list[ c ] = []

						category_list[ c ].append( f )

				print '    "";'
				print '#endif'
				print ''

		keys = category_list.keys()
		keys.sort()

		for category in keys:
			print '#if defined(need_%s)' % (category)
			print 'static const struct dri_extension_function %s_functions[] = {' % (category)
			
			for f in category_list[ category ]:
				# A function either has an offset that is
				# assigned by the ABI, or it has a remap
				# index.
				if any_entrypoints_in_abi(f, abi, api):
					index_name = "-1"
					offset = f.offset
				else:
					index_name = "%s_remap_index" % (f.name)
					offset = -1

				print '    { %s_names, %s, %d },' % (f.name, index_name, offset)


			print '    { NULL, 0, 0 }'
			print '};'
			print '#endif'
			print ''
		
		return


class PrintInitDispatch(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "extension_helper.py (from Mesa)"
		self.license = license.bsd_license_template % ("(C) Copyright IBM Corporation 2005", "IBM")
		return


	def do_function_body(self, api, abi, vtxfmt_only):
		last_condition_string = None
		for f in api.functionIterateByOffset():
			if (f.name in vtxfmt) and not vtxfmt_only:
				continue

			if (f.name not in vtxfmt) and vtxfmt_only:
				continue

			condition = condition_for_function(f, abi, 1)
			condition_string = string.join(condition, " || ")

			if condition_string != last_condition_string:
				if last_condition_string:
					print '#endif /* %s */' % (last_condition_string)

				if condition_string:
					print '#if %s' % (condition_string)
				
			if vtxfmt_only:
				print '   disp->%s = vfmt->%s;' % (f.name, f.name)
			else:
				print '   disp->%s = _mesa_%s;' % (f.name, f.name)

			last_condition_string = condition_string

		if last_condition_string:
			print '#endif /* %s */' % (last_condition_string)
		


	def printBody(self, api):
		abi = [ "1.0", "1.1", "1.2", "GL_ARB_multitexture" ]
		
		print 'void driver_init_exec_table(struct _glapi_table *disp)'
		print '{'
		self.do_function_body(api, abi, 0)
		print '}'
		print ''
		print 'void driver_install_vtxfmt(struct _glapi_table *disp, const GLvertexformat *vfmt)'
		print '{'
		self.do_function_body(api, abi, 1)
		print '}'

		return


def show_usage():
	print "Usage: %s [-f input_file_name] [-m output_mode]" % sys.argv[0]
	print "    -m output_mode   Output mode can be one of 'extensions' or 'exec_init'."
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m:")
	except Exception,e:
		show_usage()

	mode = "extensions"
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		if arg == '-m':
			mode = val


	api = gl_XML.parse_GL_API( file_name )

	if mode == "extensions":
		printer = PrintGlExtensionGlue()
	elif mode == "exec_init":
		printer = PrintInitDispatch()
	else:
		show_usage()

	printer.Print( api )
