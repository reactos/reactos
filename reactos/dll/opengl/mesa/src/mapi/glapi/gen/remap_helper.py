#!/usr/bin/env python

# Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
# All Rights Reserved.
#
# This is based on extension_helper.py by Ian Romanick.
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

import gl_XML
import license
import sys, getopt, string

def get_function_spec(func):
	sig = ""
	# derive parameter signature
	for p in func.parameterIterator():
		if p.is_padding:
			continue
		# FIXME: This is a *really* ugly hack. :(
		tn = p.type_expr.get_base_type_node()
		if p.is_pointer():
			sig += 'p'
		elif tn.integer:
			sig += 'i'
		elif tn.size == 4:
			sig += 'f'
		else:
			sig += 'd'

	spec = [sig]
	for ent in func.entry_points:
		spec.append("gl" + ent)

	# spec is terminated by an empty string
	spec.append('')

	return spec

class PrintGlRemap(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "remap_helper.py (from Mesa)"
		self.license = license.bsd_license_template % ("Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>", "Chia-I Wu")
		return


	def printRealHeader(self):
		print '#include "main/dispatch.h"'
		print '#include "main/remap.h"'
		print ''
		return


	def printBody(self, api):
		pool_indices = {}

		print '/* this is internal to remap.c */'
		print '#ifndef need_MESA_remap_table'
		print '#error Only remap.c should include this file!'
		print '#endif /* need_MESA_remap_table */'
		print ''

		print ''
		print 'static const char _mesa_function_pool[] ='

		# output string pool
		index = 0;
		for f in api.functionIterateAll():
			pool_indices[f] = index

			spec = get_function_spec(f)

			# a function has either assigned offset, fixed offset,
			# or no offset
			if f.assign_offset:
				comments = "will be remapped"
			elif f.offset > 0:
				comments = "offset %d" % f.offset
			else:
				comments = "dynamic"

			print '   /* _mesa_function_pool[%d]: %s (%s) */' \
					% (index, f.name, comments)
			for line in spec:
				print '   "%s\\0"' % line
				index += len(line) + 1
		print '   ;'
		print ''

		print '/* these functions need to be remapped */'
		print 'static const struct gl_function_pool_remap MESA_remap_table_functions[] = {'
		# output all functions that need to be remapped
		# iterate by offsets so that they are sorted by remap indices
		for f in api.functionIterateByOffset():
			if not f.assign_offset:
				continue
			print '   { %5d, %s_remap_index },' \
					% (pool_indices[f], f.name)
		print '   {    -1, -1 }'
		print '};'
		print ''

		# collect functions by versions/extensions
		extension_functions = {}
		abi_extensions = []
		for f in api.functionIterateAll():
			for n in f.entry_points:
				category, num = api.get_category_for_name(n)
				# consider only GL_VERSION_X_Y or extensions
				c = gl_XML.real_category_name(category)
				if c.startswith("GL_"):
					if not extension_functions.has_key(c):
						extension_functions[c] = []
					extension_functions[c].append(f)
					# remember the ext names of the ABI
					if (f.is_abi() and n == f.name and
						c not in abi_extensions):
						abi_extensions.append(c)
		# ignore the ABI itself
		for ext in abi_extensions:
			extension_functions.pop(ext)

		extensions = extension_functions.keys()
		extensions.sort()

		# output ABI functions that have alternative names (with ext suffix)
		print '/* these functions are in the ABI, but have alternative names */'
		print 'static const struct gl_function_remap MESA_alt_functions[] = {'
		for ext in extensions:
			funcs = []
			for f in extension_functions[ext]:
				# test if the function is in the ABI and has alt names
				if f.is_abi() and len(f.entry_points) > 1:
					funcs.append(f)
			if not funcs:
				continue
			print '   /* from %s */' % ext
			for f in funcs:
				print '   { %5d, _gloffset_%s },' \
						% (pool_indices[f], f.name)
		print '   {    -1, -1 }'
		print '};'
		print ''
		return


def show_usage():
	print "Usage: %s [-f input_file_name] [-c ver]" % sys.argv[0]
	print "    -c ver    Version can be 'es1' or 'es2'."
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:c:")
	except Exception,e:
		show_usage()

	es = None
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-c":
			es = val

	api = gl_XML.parse_GL_API( file_name )

	if es is not None:
		import gles_api

		api_map = {
			'es1': gles_api.es1_api,
			'es2': gles_api.es2_api,
		}

		api.filter_functions(api_map[es])

	printer = PrintGlRemap()
	printer.Print( api )
