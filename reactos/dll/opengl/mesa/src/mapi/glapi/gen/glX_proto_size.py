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
import sys, getopt, copy, string


class glx_enum_function:
	def __init__(self, func_name, enum_dict):
		self.name = func_name
		self.mode = 1
		self.sig = None

		# "enums" is a set of lists.  The element in the set is the
		# value of the enum.  The list is the list of names for that
		# value.  For example, [0x8126] = {"POINT_SIZE_MIN",
		# "POINT_SIZE_MIN_ARB", "POINT_SIZE_MIN_EXT",
		# "POINT_SIZE_MIN_SGIS"}.

		self.enums = {}

		# "count" is indexed by count values.  Each element of count
		# is a list of index to "enums" that have that number of
		# associated data elements.  For example, [4] = 
		# {GL_AMBIENT, GL_DIFFUSE, GL_SPECULAR, GL_EMISSION,
		# GL_AMBIENT_AND_DIFFUSE} (the enum names are used here,
		# but the actual hexadecimal values would be in the array).

		self.count = {}


		# Fill self.count and self.enums using the dictionary of enums
		# that was passed in.  The generic Get functions (e.g.,
		# GetBooleanv and friends) are handled specially here.  In
		# the data the generic Get functions are refered to as "Get".

		if func_name in ["GetIntegerv", "GetBooleanv", "GetFloatv", "GetDoublev"]:
			match_name = "Get"
		else:
			match_name = func_name

		mode_set = 0
		for enum_name in enum_dict:
			e = enum_dict[ enum_name ]

			if e.functions.has_key( match_name ):
				[count, mode] = e.functions[ match_name ]

				if mode_set and mode != self.mode:
					raise RuntimeError("Not all enums for %s have the same mode." % (func_name))

				self.mode = mode

				if self.enums.has_key( e.value ):
					if e.name not in self.enums[ e.value ]:
						self.enums[ e.value ].append( e )
				else:
					if not self.count.has_key( count ):
						self.count[ count ] = []

					self.enums[ e.value ] = [ e ]
					self.count[ count ].append( e.value )


		return


	def signature( self ):
		if self.sig == None:
			self.sig = ""
			for i in self.count:
				if i == None:
					raise RuntimeError("i is None.  WTF?")

				self.count[i].sort()
				for e in self.count[i]:
					self.sig += "%04x,%d," % (e, i)

		return self.sig


	def is_set( self ):
		return self.mode


	def PrintUsingTable(self):
		"""Emit the body of the __gl*_size function using a pair
		of look-up tables and a mask.  The mask is calculated such
		that (e & mask) is unique for all the valid values of e for
		this function.  The result of (e & mask) is used as an index
		into the first look-up table.  If it matches e, then the
		same entry of the second table is returned.  Otherwise zero
		is returned.
		
		It seems like this should cause better code to be generated.
		However, on x86 at least, the resulting .o file is about 20%
		larger then the switch-statment version.  I am leaving this
		code in because the results may be different on other
		platforms (e.g., PowerPC or x86-64)."""

		return 0
		count = 0
		for a in self.enums:
			count += 1

		if self.count.has_key(-1):
			return 0

		# Determine if there is some mask M, such that M = (2^N) - 1,
		# that will generate unique values for all of the enums.

		mask = 0
		for i in [1, 2, 3, 4, 5, 6, 7, 8]:
			mask = (1 << i) - 1

			fail = 0;
			for a in self.enums:
				for b in self.enums:
					if a != b:
						if (a & mask) == (b & mask):
							fail = 1;

			if not fail:
				break;
			else:
				mask = 0

		if (mask != 0) and (mask < (2 * count)):
			masked_enums = {}
			masked_count = {}

			for i in range(0, mask + 1):
				masked_enums[i] = "0";
				masked_count[i] = 0;

			for c in self.count:
				for e in self.count[c]:
					i = e & mask
					enum_obj = self.enums[e][0]
					masked_enums[i] = '0x%04x /* %s */' % (e, enum_obj.name )
					masked_count[i] = c


			print '    static const GLushort a[%u] = {' % (mask + 1)
			for e in masked_enums:
				print '        %s, ' % (masked_enums[e])
			print '    };'

			print '    static const GLubyte b[%u] = {' % (mask + 1)
			for c in masked_count:
				print '        %u, ' % (masked_count[c])
			print '    };'

			print '    const unsigned idx = (e & 0x%02xU);' % (mask)
			print ''
			print '    return (e == a[idx]) ? (GLint) b[idx] : 0;'
			return 1;
		else:
			return 0;


	def PrintUsingSwitch(self, name):
		"""Emit the body of the __gl*_size function using a 
		switch-statement."""

		print '    switch( e ) {'

		for c in self.count:
			for e in self.count[c]:
				first = 1

				# There may be multiple enums with the same
				# value.  This happens has extensions are
				# promoted from vendor-specific or EXT to
				# ARB and to the core.  Emit the first one as
				# a case label, and emit the others as
				# commented-out case labels.

				list = {}
				for enum_obj in self.enums[e]:
					list[ enum_obj.priority() ] = enum_obj.name

				keys = list.keys()
				keys.sort()
				for k in keys:
					j = list[k]
					if first:
						print '        case GL_%s:' % (j)
						first = 0
					else:
						print '/*      case GL_%s:*/' % (j)
					
			if c == -1:
				print '            return __gl%s_variable_size( e );' % (name)
			else:
				print '            return %u;' % (c)
					
		print '        default: return 0;'
		print '    }'


	def Print(self, name):
		print '_X_INTERNAL PURE FASTCALL GLint'
		print '__gl%s_size( GLenum e )' % (name)
		print '{'

		if not self.PrintUsingTable():
			self.PrintUsingSwitch(name)

		print '}'
		print ''


class glx_server_enum_function(glx_enum_function):
	def __init__(self, func, enum_dict):
		glx_enum_function.__init__(self, func.name, enum_dict)
		
		self.function = func
		return


	def signature( self ):
		if self.sig == None:
			sig = glx_enum_function.signature(self)

			p = self.function.variable_length_parameter()
			if p:
				sig += "%u" % (p.size())

			self.sig = sig

		return self.sig;


	def Print(self, name, printer):
		f = self.function
		printer.common_func_print_just_header( f )

		fixup = []
		
		foo = {}
		for param_name in f.count_parameter_list:
			o = f.offset_of( param_name )
			foo[o] = param_name

		for param_name in f.counter_list:
			o = f.offset_of( param_name )
			foo[o] = param_name

		keys = foo.keys()
		keys.sort()
		for o in keys:
			p = f.parameters_by_name[ foo[o] ]

			printer.common_emit_one_arg(p, "pc", 0)
			fixup.append( p.name )


		print '    GLsizei compsize;'
		print ''

		printer.common_emit_fixups(fixup)

		print ''
		print '    compsize = __gl%s_size(%s);' % (f.name, string.join(f.count_parameter_list, ","))
		p = f.variable_length_parameter()
		print '    return __GLX_PAD(%s);' % (p.size_string())

		print '}'
		print ''


class PrintGlxSizeStubs_common(gl_XML.gl_print_base):
	do_get = (1 << 0)
	do_set = (1 << 1)

	def __init__(self, which_functions):
		gl_XML.gl_print_base.__init__(self)

		self.name = "glX_proto_size.py (from Mesa)"
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2004", "IBM")

		self.emit_set = ((which_functions & PrintGlxSizeStubs_common.do_set) != 0)
		self.emit_get = ((which_functions & PrintGlxSizeStubs_common.do_get) != 0)
		return


class PrintGlxSizeStubs_c(PrintGlxSizeStubs_common):
	def printRealHeader(self):
		print ''
		print '#include <X11/Xfuncproto.h>'
		print '#include <GL/gl.h>'
		if self.emit_get:
			print '#include "indirect_size_get.h"'
			print '#include "glxserver.h"'
			print '#include "indirect_util.h"'
		
		print '#include "indirect_size.h"'

		print ''
		self.printPure()
		print ''
		self.printFastcall()
		print ''
		print ''
		print '#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(GLX_USE_APPLEGL)'
		print '#  undef HAVE_ALIAS'
		print '#endif'
		print '#ifdef HAVE_ALIAS'
		print '#  define ALIAS2(from,to) \\'
		print '    _X_INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \\'
		print '        __attribute__ ((alias( # to )));'
		print '#  define ALIAS(from,to) ALIAS2( from, __gl ## to ## _size )'
		print '#else'
		print '#  define ALIAS(from,to) \\'
		print '    _X_INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \\'
		print '    { return __gl ## to ## _size( e ); }'
		print '#endif'
		print ''
		print ''


	def printBody(self, api):
		enum_sigs = {}
		aliases = []

		for func in api.functionIterateGlx():
			ef = glx_enum_function( func.name, api.enums_by_name )
			if len(ef.enums) == 0:
				continue

			if (ef.is_set() and self.emit_set) or (not ef.is_set() and self.emit_get):
				sig = ef.signature()
				if enum_sigs.has_key( sig ):
					aliases.append( [func.name, enum_sigs[ sig ]] )
				else:
					enum_sigs[ sig ] = func.name
					ef.Print( func.name )


		for [alias_name, real_name] in aliases:
			print 'ALIAS( %s, %s )' % (alias_name, real_name)


				
class PrintGlxSizeStubs_h(PrintGlxSizeStubs_common):
	def printRealHeader(self):
		print """/**
 * \\file
 * Prototypes for functions used to determine the number of data elements in
 * various GLX protocol messages.
 *
 * \\author Ian Romanick <idr@us.ibm.com>
 */
"""
		print '#include <X11/Xfuncproto.h>'
		print ''
		self.printPure();
		print ''
		self.printFastcall();
		print ''


	def printBody(self, api):
		for func in api.functionIterateGlx():
			ef = glx_enum_function( func.name, api.enums_by_name )
			if len(ef.enums) == 0:
				continue

			if (ef.is_set() and self.emit_set) or (not ef.is_set() and self.emit_get):
				print 'extern _X_INTERNAL PURE FASTCALL GLint __gl%s_size(GLenum);' % (func.name)


class PrintGlxReqSize_common(gl_XML.gl_print_base):
	"""Common base class for PrintGlxSizeReq_h and PrintGlxSizeReq_h.

	The main purpose of this common base class is to provide the infrastructure
	for the derrived classes to iterate over the same set of functions.
	"""

	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "glX_proto_size.py (from Mesa)"
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2005", "IBM")


class PrintGlxReqSize_h(PrintGlxReqSize_common):
	def __init__(self):
		PrintGlxReqSize_common.__init__(self)
		self.header_tag = "_INDIRECT_REQSIZE_H_"


	def printRealHeader(self):
		print '#include <X11/Xfuncproto.h>'
		print ''
		self.printPure()
		print ''


	def printBody(self, api):
		for func in api.functionIterateGlx():
			if not func.ignore and func.has_variable_size_request():
				print 'extern PURE _X_HIDDEN int __glX%sReqSize(const GLbyte *pc, Bool swap);' % (func.name)


class PrintGlxReqSize_c(PrintGlxReqSize_common):
	"""Create the server-side 'request size' functions.

	Create the server-side functions that are used to determine what the
	size of a varible length command should be.  The server then uses
	this value to determine if the incoming command packed it malformed.
	"""

	def __init__(self):
		PrintGlxReqSize_common.__init__(self)
		self.counter_sigs = {}


	def printRealHeader(self):
		print ''
		print '#include <GL/gl.h>'
		print '#include "glxserver.h"'
		print '#include "glxbyteorder.h"'
		print '#include "indirect_size.h"'
		print '#include "indirect_reqsize.h"'
		print ''
		print '#define __GLX_PAD(x)  (((x) + 3) & ~3)'
		print ''
		print '#if defined(__CYGWIN__) || defined(__MINGW32__)'
		print '#  undef HAVE_ALIAS'
		print '#endif'
		print '#ifdef HAVE_ALIAS'
		print '#  define ALIAS2(from,to) \\'
		print '    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap ) \\'
		print '        __attribute__ ((alias( # to )));'
		print '#  define ALIAS(from,to) ALIAS2( from, __glX ## to ## ReqSize )'
		print '#else'
		print '#  define ALIAS(from,to) \\'
		print '    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap ) \\'
		print '    { return __glX ## to ## ReqSize( pc, swap ); }'
		print '#endif'
		print ''
		print ''


	def printBody(self, api):
		aliases = []
		enum_functions = {}
		enum_sigs = {}

		for func in api.functionIterateGlx():
			if not func.has_variable_size_request(): continue

			ef = glx_server_enum_function( func, api.enums_by_name )
			if len(ef.enums) == 0: continue

			sig = ef.signature()

			if not enum_functions.has_key(func.name):
				enum_functions[ func.name ] = sig

			if not enum_sigs.has_key( sig ):
				enum_sigs[ sig ] = ef
			


		for func in api.functionIterateGlx():
			# Even though server-handcode fuctions are on "the
			# list", and prototypes are generated for them, there
			# isn't enough information to generate a size
			# function.  If there was enough information, they
			# probably wouldn't need to be handcoded in the first
			# place!

			if func.server_handcode: continue
			if not func.has_variable_size_request(): continue

			if enum_functions.has_key(func.name):
				sig = enum_functions[func.name]
				ef = enum_sigs[ sig ]

				if ef.name != func.name:
					aliases.append( [func.name, ef.name] )
				else:
					ef.Print( func.name, self )

			elif func.images:
				self.printPixelFunction(func)
			elif func.has_variable_size_request():
				a = self.printCountedFunction(func)
				if a: aliases.append(a)


		for [alias_name, real_name] in aliases:
			print 'ALIAS( %s, %s )' % (alias_name, real_name)

		return


	def common_emit_fixups(self, fixup):
		"""Utility function to emit conditional byte-swaps."""

		if fixup:
			print '    if (swap) {'
			for name in fixup:
				print '        %s = bswap_32(%s);' % (name, name)
			print '    }'

		return


	def common_emit_one_arg(self, p, pc, adjust):
		offset = p.offset
		dst = p.string()
		src = '(%s *)' % (p.type_string())
		print '%-18s = *%11s(%s + %u);' % (dst, src, pc, offset + adjust);
		return


	def common_func_print_just_header(self, f):
		print 'int'
		print '__glX%sReqSize( const GLbyte * pc, Bool swap )' % (f.name)
		print '{'


	def printPixelFunction(self, f):
		self.common_func_print_just_header(f)
		
		f.offset_of( f.parameters[0].name )
		[dim, w, h, d, junk] = f.get_images()[0].get_dimensions()

		print '    GLint row_length   = *  (GLint *)(pc +  4);'

		if dim < 3:
			fixup = ['row_length', 'skip_rows', 'alignment']
			print '    GLint image_height = 0;'
			print '    GLint skip_images  = 0;'
			print '    GLint skip_rows    = *  (GLint *)(pc +  8);'
			print '    GLint alignment    = *  (GLint *)(pc + 16);'
		else:
			fixup = ['row_length', 'image_height', 'skip_rows', 'skip_images', 'alignment']
			print '    GLint image_height = *  (GLint *)(pc +  8);'
			print '    GLint skip_rows    = *  (GLint *)(pc + 16);'
			print '    GLint skip_images  = *  (GLint *)(pc + 20);'
			print '    GLint alignment    = *  (GLint *)(pc + 32);'

		img = f.images[0]
		for p in f.parameterIterateGlxSend():
			if p.name in [w, h, d, img.img_format, img.img_type, img.img_target]:
				self.common_emit_one_arg(p, "pc", 0)
				fixup.append( p.name )

		print ''

		self.common_emit_fixups(fixup)

		if img.img_null_flag:
			print ''
			print '	   if (*(CARD32 *) (pc + %s))' % (img.offset - 4)
			print '	       return 0;'

		print ''
		print '    return __glXImageSize(%s, %s, %s, %s, %s, %s,' % (img.img_format, img.img_type, img.img_target, w, h, d )
		print '                          image_height, row_length, skip_images,'
		print '                          skip_rows, alignment);'
		print '}'
		print ''
		return


	def printCountedFunction(self, f):

		sig = ""
		offset = 0
		fixup = []
		params = []
		plus = ''
		size = ''
		param_offsets = {}

		# Calculate the offset of each counter parameter and the
		# size string for the variable length parameter(s).  While
		# that is being done, calculate a unique signature for this
		# function.

		for p in f.parameterIterateGlxSend():
			if p.is_counter:
				fixup.append( p.name )
				params.append( p )
			elif p.counter:
				s = p.size()
				if s == 0: s = 1

				sig += "(%u,%u)" % (f.offset_of(p.counter), s)
				size += '%s%s' % (plus, p.size_string())
				plus = ' + '


		# If the calculated signature matches a function that has
		# already be emitted, don't emit this function.  Instead, add
		# it to the list of function aliases.

		if self.counter_sigs.has_key(sig):
			n = self.counter_sigs[sig];
			alias = [f.name, n]
		else:
			alias = None
			self.counter_sigs[sig] = f.name

			self.common_func_print_just_header(f)

			for p in params:
				self.common_emit_one_arg(p, "pc", 0)


			print ''
			self.common_emit_fixups(fixup)
			print ''

			print '    return __GLX_PAD(%s);' % (size)
			print '}'
			print ''

		return alias


def show_usage():
	print "Usage: %s [-f input_file_name] -m output_mode [--only-get | --only-set] [--get-alias-set]" % sys.argv[0]
	print "    -m output_mode   Output mode can be one of 'size_c' or 'size_h'."
	print "    --only-get       Only emit 'get'-type functions."
	print "    --only-set       Only emit 'set'-type functions."
	print ""
	print "By default, both 'get' and 'set'-type functions are emitted."
	sys.exit(1)


if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m:h:", ["only-get", "only-set", "header-tag"])
	except Exception,e:
		show_usage()

	mode = None
	header_tag = None
	which_functions = PrintGlxSizeStubs_common.do_get | PrintGlxSizeStubs_common.do_set

	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-m":
			mode = val
		elif arg == "--only-get":
			which_functions = PrintGlxSizeStubs_common.do_get
		elif arg == "--only-set":
			which_functions = PrintGlxSizeStubs_common.do_set
		elif (arg == '-h') or (arg == "--header-tag"):
			header_tag = val

	if mode == "size_c":
		printer = PrintGlxSizeStubs_c( which_functions )
	elif mode == "size_h":
		printer = PrintGlxSizeStubs_h( which_functions )
		if header_tag:
			printer.header_tag = header_tag
	elif mode == "reqsize_c":
		printer = PrintGlxReqSize_c()
	elif mode == "reqsize_h":
		printer = PrintGlxReqSize_h()
	else:
		show_usage()

	api = gl_XML.parse_GL_API( file_name, glX_XML.glx_item_factory() )


	printer.Print( api )
