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

import gl_XML, glX_XML, glX_proto_common, license
import sys, getopt, string


class PrintGlxDispatch_h(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "glX_proto_recv.py (from Mesa)"
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2005", "IBM")

		self.header_tag = "_INDIRECT_DISPATCH_H_"
		return


	def printRealHeader(self):
		print '#  include <X11/Xfuncproto.h>'
		print ''
		print 'struct __GLXclientStateRec;'
		print ''
		return


	def printBody(self, api):
		for func in api.functionIterateAll():
			if not func.ignore and not func.vectorequiv:
				if func.glx_rop:
					print 'extern _X_HIDDEN void __glXDisp_%s(GLbyte * pc);' % (func.name)
					print 'extern _X_HIDDEN void __glXDispSwap_%s(GLbyte * pc);' % (func.name)
				elif func.glx_sop or func.glx_vendorpriv:
					print 'extern _X_HIDDEN int __glXDisp_%s(struct __GLXclientStateRec *, GLbyte *);' % (func.name)
					print 'extern _X_HIDDEN int __glXDispSwap_%s(struct __GLXclientStateRec *, GLbyte *);' % (func.name)

					if func.glx_sop and func.glx_vendorpriv:
						n = func.glx_vendorpriv_names[0]
						print 'extern _X_HIDDEN int __glXDisp_%s(struct __GLXclientStateRec *, GLbyte *);' % (n)
						print 'extern _X_HIDDEN int __glXDispSwap_%s(struct __GLXclientStateRec *, GLbyte *);' % (n)

		return


class PrintGlxDispatchFunctions(glX_proto_common.glx_print_proto):
	def __init__(self, do_swap):
		gl_XML.gl_print_base.__init__(self)
		self.name = "glX_proto_recv.py (from Mesa)"
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2005", "IBM")

		self.real_types = [ '', '', 'uint16_t', '', 'uint32_t', '', '', '', 'uint64_t' ]
		self.do_swap = do_swap
		return


	def printRealHeader(self):
		print '#include <X11/Xmd.h>'
		print '#include <GL/gl.h>'
		print '#include <GL/glxproto.h>'

		print '#include <inttypes.h>'
		print '#include "indirect_size.h"'
		print '#include "indirect_size_get.h"'
		print '#include "indirect_dispatch.h"'
		print '#include "glxserver.h"'
		print '#include "glxbyteorder.h"'
		print '#include "indirect_util.h"'
		print '#include "singlesize.h"'
		print '#include "glapi.h"'
		print '#include "glapitable.h"'
		print '#include "glthread.h"'
		print '#include "dispatch.h"'
		print ''
		print '#define __GLX_PAD(x)  (((x) + 3) & ~3)'
		print ''
		print 'typedef struct {'
		print '    __GLX_PIXEL_3D_HDR;'
		print '} __GLXpixel3DHeader;'
		print ''
		print 'extern GLboolean __glXErrorOccured( void );'
		print 'extern void __glXClearErrorOccured( void );'
		print ''
		print 'static const unsigned dummy_answer[2] = {0, 0};'
		print ''
		return


	def printBody(self, api):
		if self.do_swap:
			self.emit_swap_wrappers(api)


		for func in api.functionIterateByOffset():
			if not func.ignore and not func.server_handcode and not func.vectorequiv and (func.glx_rop or func.glx_sop or func.glx_vendorpriv):
				self.printFunction(func, func.name)
				if func.glx_sop and func.glx_vendorpriv:
					self.printFunction(func, func.glx_vendorpriv_names[0])
					

		return


	def printFunction(self, f, name):
		if (f.glx_sop or f.glx_vendorpriv) and (len(f.get_images()) != 0):
			return

		if not self.do_swap:
			base = '__glXDisp'
		else:
			base = '__glXDispSwap'

		if f.glx_rop:
			print 'void %s_%s(GLbyte * pc)' % (base, name)
		else:
			print 'int %s_%s(__GLXclientState *cl, GLbyte *pc)' % (base, name)

		print '{'

		if f.glx_rop or f.vectorequiv:
			self.printRenderFunction(f)
		elif f.glx_sop or f.glx_vendorpriv:
			if len(f.get_images()) == 0: 
				self.printSingleFunction(f, name)
		else:
			print "/* Missing GLX protocol for %s. */" % (name)

		print '}'
		print ''
		return


	def swap_name(self, bytes):
		return 'bswap_%u_array' % (8 * bytes)


	def emit_swap_wrappers(self, api):
		self.type_map = {}
		already_done = [ ]

		for t in api.typeIterate():
			te = t.get_type_expression()
			t_size = te.get_element_size()

			if t_size > 1 and t.glx_name:
				
				t_name = "GL" + t.name
				self.type_map[ t_name ] = t.glx_name

				if t.glx_name not in already_done:
					real_name = self.real_types[t_size]

					print 'static %s' % (t_name)
					print 'bswap_%s( const void * src )' % (t.glx_name)
					print '{'
					print '    union { %s dst; %s ret; } x;' % (real_name, t_name)
					print '    x.dst = bswap_%u( *(%s *) src );' % (t_size * 8, real_name)
					print '    return x.ret;'
					print '}'
					print ''
					already_done.append( t.glx_name )

		for bits in [16, 32, 64]:
			print 'static void *'
			print 'bswap_%u_array( uint%u_t * src, unsigned count )' % (bits, bits)
			print '{'
			print '    unsigned  i;'
			print ''
			print '    for ( i = 0 ; i < count ; i++ ) {'
			print '        uint%u_t temp = bswap_%u( src[i] );' % (bits, bits)
			print '        src[i] = temp;'
			print '    }'
			print ''
			print '    return src;'
			print '}'
			print ''
			

	def fetch_param(self, param):
		t = param.type_string()
		o = param.offset
		element_size = param.size() / param.get_element_count()

		if self.do_swap and (element_size != 1):
			if param.is_array():
				real_name = self.real_types[ element_size ]

				swap_func = self.swap_name( element_size )
				return ' (%-8s)%s( (%s *) (pc + %2s), %s )' % (t, swap_func, real_name, o, param.count)
			else:
				t_name = param.get_base_type_string()
				return ' (%-8s)bswap_%-7s( pc + %2s )' % (t, self.type_map[ t_name ], o)
		else:
			if param.is_array():
				return ' (%-8s)(pc + %2u)' % (t, o)
			else:
				return '*(%-8s *)(pc + %2u)' % (t, o)
				
		return None


	def emit_function_call(self, f, retval_assign, indent):
		list = []

		for param in f.parameterIterator():
			if param.is_padding:
				continue

			if param.is_counter or param.is_image() or param.is_output or param.name in f.count_parameter_list or len(param.count_parameter_list):
				location = param.name
			else:
				location = self.fetch_param(param)

			list.append( '%s        %s' % (indent, location) )
			

		if len( list ):
			print '%s    %sCALL_%s( GET_DISPATCH(), (' % (indent, retval_assign, f.name)
			print string.join( list, ",\n" )
			print '%s    ) );' % (indent)
		else:
			print '%s    %sCALL_%s( GET_DISPATCH(), () );' % (indent, retval_assign, f.name)
		return


	def common_func_print_just_start(self, f, indent):
		align64 = 0
		need_blank = 0


		f.calculate_offsets()
		for param in f.parameterIterateGlxSend():
			# If any parameter has a 64-bit base type, then we
			# have to do alignment magic for the while thing.

			if param.is_64_bit():
				align64 = 1


			# FIXME img_null_flag is over-loaded.  In addition to
			# FIXME being used for images, it is used to signify
			# FIXME NULL data pointers for vertex buffer object
			# FIXME related functions.  Re-name it to null_data
			# FIXME or something similar.

			if param.img_null_flag:
				print '%s    const CARD32 ptr_is_null = *(CARD32 *)(pc + %s);' % (indent, param.offset - 4)
				cond = '(ptr_is_null != 0) ? NULL : '
			else:
				cond = ""


			type_string = param.type_string()

			if param.is_image():
				offset = f.offset_of( param.name )

				print '%s    %s const %s = (%s) (%s(pc + %s));' % (indent, type_string, param.name, type_string, cond, offset)
				
				if param.depth:
					print '%s    __GLXpixel3DHeader * const hdr = (__GLXpixel3DHeader *)(pc);' % (indent)
				else:
					print '%s    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);' % (indent)

				need_blank = 1
			elif param.is_counter or param.name in f.count_parameter_list:
				location = self.fetch_param(param)
				print '%s    const %s %s = %s;' % (indent, type_string, param.name, location)
				need_blank = 1
			elif len(param.count_parameter_list):
				if param.size() == 1 and not self.do_swap:
					location = self.fetch_param(param)
					print '%s    %s %s = %s%s;' % (indent, type_string, param.name, cond, location)
				else:
					print '%s    %s %s;' % (indent, type_string, param.name)
				need_blank = 1



		if need_blank:
			print ''

		if align64:
			print '#ifdef __GLX_ALIGN64'

			if f.has_variable_size_request():
				self.emit_packet_size_calculation(f, 4)
				s = "cmdlen"
			else:
				s = str((f.command_fixed_length() + 3) & ~3)

			print '    if ((unsigned long)(pc) & 7) {'
			print '        (void) memmove(pc-4, pc, %s);' % (s)
			print '        pc -= 4;'
			print '    }'
			print '#endif'
			print ''


		need_blank = 0
		if self.do_swap:
			for param in f.parameterIterateGlxSend():
				if param.count_parameter_list:
					o = param.offset
					count = param.get_element_count()
					type_size = param.size() / count
					
					if param.counter:
						count_name = param.counter
					else:
						count_name = str(count)

					# This is basically an ugly special-
					# case for glCallLists.

					if type_size == 1:
						x = [] 
						x.append( [1, ['BYTE', 'UNSIGNED_BYTE', '2_BYTES', '3_BYTES', '4_BYTES']] )
						x.append( [2, ['SHORT', 'UNSIGNED_SHORT']] )
						x.append( [4, ['INT', 'UNSIGNED_INT', 'FLOAT']] )

						print '    switch(%s) {' % (param.count_parameter_list[0])
						for sub in x:
							for t_name in sub[1]:
								print '    case GL_%s:' % (t_name)

							if sub[0] == 1:
								print '        %s = (%s) (pc + %s); break;' % (param.name, param.type_string(), o)
							else:
								swap_func = self.swap_name(sub[0])
								print '        %s = (%s) %s( (%s *) (pc + %s), %s ); break;' % (param.name, param.type_string(), swap_func, self.real_types[sub[0]], o, count_name)
						print '    default:'
						print '        return;'
						print '    }'
					else:
						swap_func = self.swap_name(type_size)
						compsize = self.size_call(f, 1)
						print '    %s = (%s) %s( (%s *) (pc + %s), %s );' % (param.name, param.type_string(), swap_func, self.real_types[type_size], o, compsize)

					need_blank = 1

		else:
			for param in f.parameterIterateGlxSend():
				if param.count_parameter_list:
					print '%s    %s = (%s) (pc + %s);' % (indent, param.name, param.type_string(), param.offset)
					need_blank = 1


		if need_blank:
			print ''


		return


	def printSingleFunction(self, f, name):
		if name not in f.glx_vendorpriv_names:
			print '    xGLXSingleReq * const req = (xGLXSingleReq *) pc;'
		else:
			print '    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;'

		print '    int error;'

		if self.do_swap:
		    print '    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);'
		else:
		    print '    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);'

		print ''
		if name not in f.glx_vendorpriv_names:
			print '    pc += __GLX_SINGLE_HDR_SIZE;'
		else:
			print '    pc += __GLX_VENDPRIV_HDR_SIZE;'

		print '    if ( cx != NULL ) {'
		self.common_func_print_just_start(f, "    ")
		

		if f.return_type != 'void':
			print '        %s retval;' % (f.return_type)
			retval_string = "retval"
			retval_assign = "retval = "
		else:
			retval_string = "0"
			retval_assign = ""


		type_size = 0
		answer_string = "dummy_answer"
		answer_count = "0"
		is_array_string = "GL_FALSE"

		for param in f.parameterIterateOutputs():
			answer_type = param.get_base_type_string()
			if answer_type == "GLvoid":
				answer_type = "GLubyte"


			c = param.get_element_count()
			type_size = (param.size() / c)
			if type_size == 1:
				size_scale = ""
			else:
				size_scale = " * %u" % (type_size)


			if param.count_parameter_list:
				print '        const GLuint compsize = %s;' % (self.size_call(f, 1))
				print '        %s answerBuffer[200];' %  (answer_type)
				print '        %s %s = __glXGetAnswerBuffer(cl, compsize%s, answerBuffer, sizeof(answerBuffer), %u);' % (param.type_string(), param.name, size_scale, type_size )
				answer_string = param.name
				answer_count = "compsize"

				print ''
				print '        if (%s == NULL) return BadAlloc;' % (param.name)
				print '        __glXClearErrorOccured();'
				print ''
			elif param.counter:
				print '        %s answerBuffer[200];' %  (answer_type)
				print '        %s %s = __glXGetAnswerBuffer(cl, %s%s, answerBuffer, sizeof(answerBuffer), %u);' % (param.type_string(), param.name, param.counter, size_scale, type_size)
				answer_string = param.name
				answer_count = param.counter
			elif c >= 1:
				print '        %s %s[%u];' % (answer_type, param.name, c)
				answer_string = param.name
				answer_count = "%u" % (c)

			if f.reply_always_array:
				is_array_string = "GL_TRUE"


		self.emit_function_call(f, retval_assign, "    ")


		if f.needs_reply():
			if self.do_swap:
				for param in f.parameterIterateOutputs():
					c = param.get_element_count()
					type_size = (param.size() / c)

					if type_size > 1:
						swap_name = self.swap_name( type_size )
						print '        (void) %s( (uint%u_t *) %s, %s );' % (swap_name, 8 * type_size, param.name, answer_count)


				reply_func = '__glXSendReplySwap'
			else:
				reply_func = '__glXSendReply'

			print '        %s(cl->client, %s, %s, %u, %s, %s);' % (reply_func, answer_string, answer_count, type_size, is_array_string, retval_string)
		#elif f.note_unflushed:
		#	print '        cx->hasUnflushedCommands = GL_TRUE;'

		print '        error = Success;'
		print '    }'
		print ''
		print '    return error;'
		return


	def printRenderFunction(self, f):
		# There are 4 distinct phases in a rendering dispatch function.
		# In the first phase we compute the sizes and offsets of each
		# element in the command.  In the second phase we (optionally)
		# re-align 64-bit data elements.  In the third phase we
		# (optionally) byte-swap array data.  Finally, in the fourth
		# phase we actually dispatch the function.

		self.common_func_print_just_start(f, "")

		images = f.get_images()
		if len(images):
			if self.do_swap:
				pre = "bswap_CARD32( & "
				post = " )"
			else:
				pre = ""
				post = ""

			img = images[0]

			# swapBytes and lsbFirst are single byte fields, so
			# the must NEVER be byte-swapped.

			if not (img.img_type == "GL_BITMAP" and img.img_format == "GL_COLOR_INDEX"):
				print '    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );'

			print '    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );'

			print '    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) %shdr->rowLength%s) );' % (pre, post)
			if img.depth:
				print '    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_IMAGE_HEIGHT, (GLint) %shdr->imageHeight%s) );' % (pre, post)
			print '    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) %shdr->skipRows%s) );' % (pre, post)
			if img.depth:
				print '    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_IMAGES,  (GLint) %shdr->skipImages%s) );' % (pre, post)
			print '    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) %shdr->skipPixels%s) );' % (pre, post)
			print '    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) %shdr->alignment%s) );' % (pre, post)
			print ''


		self.emit_function_call(f, "", "")
		return


if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m:s")
	except Exception,e:
		show_usage()

	mode = "dispatch_c"
	do_swap = 0
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-m":
			mode = val
		elif arg == "-s":
			do_swap = 1

	if mode == "dispatch_c":
		printer = PrintGlxDispatchFunctions(do_swap)
	elif mode == "dispatch_h":
		printer = PrintGlxDispatch_h()
	else:
		show_usage()

	api = gl_XML.parse_GL_API( file_name, glX_proto_common.glx_proto_item_factory() )

	printer.Print( api )
