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
#    Jeremy Kolb <jkolb@brandeis.edu>

import gl_XML, glX_XML, glX_proto_common, license
import sys, getopt, copy, string

def convertStringForXCB(str):
    tmp = ""
    special = [ "ARB" ]
    i = 0
    while i < len(str):
        if str[i:i+3] in special:
            tmp = '%s_%s' % (tmp, string.lower(str[i:i+3]))
            i = i + 2;
        elif str[i].isupper():
            tmp = '%s_%s' % (tmp, string.lower(str[i]))
        else:
            tmp = '%s%s' % (tmp, str[i])
        i += 1
    return tmp

def hash_pixel_function(func):
	"""Generate a 'unique' key for a pixel function.  The key is based on
	the parameters written in the command packet.  This includes any
	padding that might be added for the original function and the 'NULL
	image' flag."""


	h = ""
	hash_pre = ""
	hash_suf = ""
	for param in func.parameterIterateGlxSend():
		if param.is_image():
			[dim, junk, junk, junk, junk] = param.get_dimensions()

			d = (dim + 1) & ~1
			hash_pre = "%uD%uD_" % (d - 1, d)

			if param.img_null_flag:
				hash_suf = "_NF"

		h += "%u" % (param.size())

		if func.pad_after(param):
			h += "4"


	n = func.name.replace("%uD" % (dim), "")
	n = "__glx_%s_%uD%uD" % (n, d - 1, d)

	h = hash_pre + h + hash_suf
	return [h, n]


class glx_pixel_function_stub(glX_XML.glx_function):
	"""Dummy class used to generate pixel "utility" functions that are
	shared by multiple dimension image functions.  For example, these
	objects are used to generate shared functions used to send GLX
	protocol for TexImage1D and TexImage2D, TexSubImage1D and
	TexSubImage2D, etc."""

	def __init__(self, func, name):
		# The parameters to the utility function are the same as the
		# parameters to the real function except for the added "pad"
		# parameters.

		self.name = name
		self.images = []
		self.parameters = []
		self.parameters_by_name = {}
		for _p in func.parameterIterator():
			p = copy.copy(_p)
			self.parameters.append(p)
			self.parameters_by_name[ p.name ] = p


			if p.is_image():
				self.images.append(p)
				p.height = "height"

				if p.img_yoff == None:
					p.img_yoff = "yoffset"

				if p.depth:
					if p.extent == None:
						p.extent = "extent"

					if p.img_woff == None:
						p.img_woff = "woffset"


			pad_name = func.pad_after(p)
			if pad_name:
				pad = copy.copy(p)
				pad.name = pad_name
				self.parameters.append(pad)
				self.parameters_by_name[ pad.name ] = pad
				

		self.return_type = func.return_type

		self.glx_rop = ~0
		self.glx_sop = 0
		self.glx_vendorpriv = 0

		self.glx_doubles_in_order = func.glx_doubles_in_order

		self.vectorequiv = None
		self.output = None
		self.can_be_large = func.can_be_large
		self.reply_always_array = func.reply_always_array
		self.dimensions_in_reply = func.dimensions_in_reply
		self.img_reset = None

		self.server_handcode = 0
		self.client_handcode = 0
		self.ignore = 0

		self.count_parameter_list = func.count_parameter_list
		self.counter_list = func.counter_list
		self.offsets_calculated = 0
		return


class PrintGlxProtoStubs(glX_proto_common.glx_print_proto):
	def __init__(self):
		glX_proto_common.glx_print_proto.__init__(self)
		self.name = "glX_proto_send.py (from Mesa)"
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2004, 2005", "IBM")


		self.last_category = ""
		self.generic_sizes = [3, 4, 6, 8, 12, 16, 24, 32]
		self.pixel_stubs = {}
		self.debug = 0
		return

	def printRealHeader(self):
		print ''
		print '#include <GL/gl.h>'
		print '#include "indirect.h"'
		print '#include "glxclient.h"'
		print '#include "indirect_size.h"'
		print '#include "glapi.h"'
		print '#include "glthread.h"'
		print '#include <GL/glxproto.h>'
		print '#ifdef USE_XCB'
		print '#include <X11/Xlib-xcb.h>'
		print '#include <xcb/xcb.h>'
		print '#include <xcb/glx.h>'
		print '#endif /* USE_XCB */'

		print ''
		print '#define __GLX_PAD(n) (((n) + 3) & ~3)'
		print ''
		self.printFastcall()
		self.printNoinline()
		print ''
		print '#ifndef __GNUC__'
		print '#  define __builtin_expect(x, y) x'
		print '#endif'
		print ''
		print '/* If the size and opcode values are known at compile-time, this will, on'
		print ' * x86 at least, emit them with a single instruction.'
		print ' */'
		print '#define emit_header(dest, op, size)            \\'
		print '    do { union { short s[2]; int i; } temp;    \\'
		print '         temp.s[0] = (size); temp.s[1] = (op); \\'
		print '         *((int *)(dest)) = temp.i; } while(0)'
		print ''
		print """NOINLINE CARD32
__glXReadReply( Display *dpy, size_t size, void * dest, GLboolean reply_is_always_array )
{
    xGLXSingleReply reply;
    
    (void) _XReply(dpy, (xReply *) & reply, 0, False);
    if (size != 0) {
        if ((reply.length > 0) || reply_is_always_array) {
            const GLint bytes = (reply_is_always_array) 
              ? (4 * reply.length) : (reply.size * size);
            const GLint extra = 4 - (bytes & 3);

            _XRead(dpy, dest, bytes);
            if ( extra < 4 ) {
                _XEatData(dpy, extra);
            }
        }
        else {
            (void) memcpy( dest, &(reply.pad3), size);
        }
    }

    return reply.retval;
}

NOINLINE void
__glXReadPixelReply( Display *dpy, struct glx_context * gc, unsigned max_dim,
    GLint width, GLint height, GLint depth, GLenum format, GLenum type,
    void * dest, GLboolean dimensions_in_reply )
{
    xGLXSingleReply reply;
    GLint size;
    
    (void) _XReply(dpy, (xReply *) & reply, 0, False);

    if ( dimensions_in_reply ) {
        width  = reply.pad3;
        height = reply.pad4;
        depth  = reply.pad5;
	
	if ((height == 0) || (max_dim < 2)) { height = 1; }
	if ((depth  == 0) || (max_dim < 3)) { depth  = 1; }
    }

    size = reply.length * 4;
    if (size != 0) {
        void * buf = Xmalloc( size );

        if ( buf == NULL ) {
            _XEatData(dpy, size);
            __glXSetError(gc, GL_OUT_OF_MEMORY);
        }
        else {
            const GLint extra = 4 - (size & 3);

            _XRead(dpy, buf, size);
            if ( extra < 4 ) {
                _XEatData(dpy, extra);
            }

            __glEmptyImage(gc, 3, width, height, depth, format, type,
                           buf, dest);
            Xfree(buf);
        }
    }
}

#define X_GLXSingle 0

NOINLINE FASTCALL GLubyte *
__glXSetupSingleRequest( struct glx_context * gc, GLint sop, GLint cmdlen )
{
    xGLXSingleReq * req;
    Display * const dpy = gc->currentDpy;

    (void) __glXFlushRenderBuffer(gc, gc->pc);
    LockDisplay(dpy);
    GetReqExtra(GLXSingle, cmdlen, req);
    req->reqType = gc->majorOpcode;
    req->contextTag = gc->currentContextTag;
    req->glxCode = sop;
    return (GLubyte *)(req) + sz_xGLXSingleReq;
}

NOINLINE FASTCALL GLubyte *
__glXSetupVendorRequest( struct glx_context * gc, GLint code, GLint vop, GLint cmdlen )
{
    xGLXVendorPrivateReq * req;
    Display * const dpy = gc->currentDpy;

    (void) __glXFlushRenderBuffer(gc, gc->pc);
    LockDisplay(dpy);
    GetReqExtra(GLXVendorPrivate, cmdlen, req);
    req->reqType = gc->majorOpcode;
    req->glxCode = code;
    req->vendorCode = vop;
    req->contextTag = gc->currentContextTag;
    return (GLubyte *)(req) + sz_xGLXVendorPrivateReq;
}

const GLuint __glXDefaultPixelStore[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 1 };

#define zero                        (__glXDefaultPixelStore+0)
#define one                         (__glXDefaultPixelStore+8)
#define default_pixel_store_1D      (__glXDefaultPixelStore+4)
#define default_pixel_store_1D_size 20
#define default_pixel_store_2D      (__glXDefaultPixelStore+4)
#define default_pixel_store_2D_size 20
#define default_pixel_store_3D      (__glXDefaultPixelStore+0)
#define default_pixel_store_3D_size 36
#define default_pixel_store_4D      (__glXDefaultPixelStore+0)
#define default_pixel_store_4D_size 36
"""

		for size in self.generic_sizes:
			self.print_generic_function(size)
		return


	def printBody(self, api):

		self.pixel_stubs = {}
		generated_stubs = []

		for func in api.functionIterateGlx():
			if func.client_handcode: continue

			# If the function is a pixel function with a certain
			# GLX protocol signature, create a fake stub function
			# for it.  For example, create a single stub function
			# that is used to implement both glTexImage1D and
			# glTexImage2D.

			if func.glx_rop != 0:
				do_it = 0
				for image in func.get_images():
					if image.img_pad_dimensions:
						do_it = 1
						break


				if do_it:
					[h, n] = hash_pixel_function(func)


					self.pixel_stubs[ func.name ] = n
					if h not in generated_stubs:
						generated_stubs.append(h)

						fake_func = glx_pixel_function_stub( func, n )
						self.printFunction(fake_func, fake_func.name)


			self.printFunction(func, func.name)
			if func.glx_sop and func.glx_vendorpriv:
				self.printFunction(func, func.glx_vendorpriv_names[0])

		self.printGetProcAddress(api)
		return

	def printGetProcAddress(self, api):
		procs = {}
		for func in api.functionIterateGlx():
			for n in func.entry_points:
				if func.has_different_protocol(n):
					procs[n] = func.static_glx_name(n)

		print """
#ifdef GLX_SHARED_GLAPI

static const struct proc_pair
{
   const char *name;
   _glapi_proc proc;
} proc_pairs[%d] = {""" % len(procs)
		names = procs.keys()
		names.sort()
		for i in xrange(len(names)):
			comma = ',' if i < len(names) - 1 else ''
			print '   { "%s", (_glapi_proc) gl%s }%s' % (names[i], procs[names[i]], comma)
		print """};

static int
__indirect_get_proc_compare(const void *key, const void *memb)
{
   const struct proc_pair *pair = (const struct proc_pair *) memb;
   return strcmp((const char *) key, pair->name);
}

_glapi_proc
__indirect_get_proc_address(const char *name)
{
   const struct proc_pair *pair;
   
   /* skip "gl" */
   name += 2;

   pair = (const struct proc_pair *) bsearch((const void *) name,
      (const void *) proc_pairs, ARRAY_SIZE(proc_pairs), sizeof(proc_pairs[0]),
      __indirect_get_proc_compare);

   return (pair) ? pair->proc : NULL;
}

#endif /* GLX_SHARED_GLAPI */
"""
		return


	def printFunction(self, func, name):
		footer = '}\n'
		if func.glx_rop == ~0:
			print 'static %s' % (func.return_type)
			print '%s( unsigned opcode, unsigned dim, %s )' % (func.name, func.get_parameter_string())
			print '{'
		else:
			if func.has_different_protocol(name):
				if func.return_type == "void":
					ret_string = ''
				else:
					ret_string = "return "

				func_name = func.static_glx_name(name)
				print '#define %s %d' % (func.opcode_vendor_name(name), func.glx_vendorpriv)
				print '%s gl%s(%s)' % (func.return_type, func_name, func.get_parameter_string())
				print '{'
				print '    struct glx_context * const gc = __glXGetCurrentContext();'
				print ''
				print '#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)'
				print '    if (gc->isDirect) {'
				print '    %sGET_DISPATCH()->%s(%s);' % (ret_string, func.name, func.get_called_parameter_string())
				print '    } else'
				print '#endif'
				print '    {'

				footer = '}\n}\n'
			else:
				print '#define %s %d' % (func.opcode_name(), func.opcode_value())

				print '%s __indirect_gl%s(%s)' % (func.return_type, name, func.get_parameter_string())
				print '{'


		if func.glx_rop != 0 or func.vectorequiv != None:
			if len(func.images):
				self.printPixelFunction(func)
			else:
				self.printRenderFunction(func)
		elif func.glx_sop != 0 or func.glx_vendorpriv != 0:
			self.printSingleFunction(func, name)
			pass
		else:
			print "/* Missing GLX protocol for %s. */" % (name)

		print footer
		return


	def print_generic_function(self, n):
		size = (n + 3) & ~3
		print """static FASTCALL NOINLINE void
generic_%u_byte( GLint rop, const void * ptr )
{
    struct glx_context * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = %u;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, %u);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
""" % (n, size + 4, size)
		return


	def common_emit_one_arg(self, p, pc, adjust, extra_offset):
		if p.is_array():
			src_ptr = p.name
		else:
			src_ptr = "&" + p.name

		if p.is_padding:
			print '(void) memset((void *)(%s + %u), 0, %s);' \
			    % (pc, p.offset + adjust, p.size_string() )
		elif not extra_offset:
			print '(void) memcpy((void *)(%s + %u), (void *)(%s), %s);' \
			    % (pc, p.offset + adjust, src_ptr, p.size_string() )
		else:
			print '(void) memcpy((void *)(%s + %u + %s), (void *)(%s), %s);' \
			    % (pc, p.offset + adjust, extra_offset, src_ptr, p.size_string() )

	def common_emit_args(self, f, pc, adjust, skip_vla):
		extra_offset = None

		for p in f.parameterIterateGlxSend( not skip_vla ):
			if p.name != f.img_reset:
				self.common_emit_one_arg(p, pc, adjust, extra_offset)
				
				if p.is_variable_length():
					temp = p.size_string()
					if extra_offset:
						extra_offset += " + %s" % (temp)
					else:
						extra_offset = temp

		return


	def pixel_emit_args(self, f, pc, large):
		"""Emit the arguments for a pixel function.  This differs from
		common_emit_args in that pixel functions may require padding
		be inserted (i.e., for the missing width field for
		TexImage1D), and they may also require a 'NULL image' flag
		be inserted before the image data."""

		if large:
			adjust = 8
		else:
			adjust = 4

		for param in f.parameterIterateGlxSend():
			if not param.is_image():
				self.common_emit_one_arg(param, pc, adjust, None)

				if f.pad_after(param):
					print '(void) memcpy((void *)(%s + %u), zero, 4);' % (pc, (param.offset + param.size()) + adjust)

			else:
				[dim, width, height, depth, extent] = param.get_dimensions()
				if f.glx_rop == ~0:
					dim_str = "dim"
				else:
					dim_str = str(dim)

				if param.is_padding:
					print '(void) memset((void *)(%s + %u), 0, %s);' \
					% (pc, (param.offset - 4) + adjust, param.size_string() )

				if param.img_null_flag:
					if large:
						print '(void) memcpy((void *)(%s + %u), zero, 4);' % (pc, (param.offset - 4) + adjust)
					else:
						print '(void) memcpy((void *)(%s + %u), (void *)((%s == NULL) ? one : zero), 4);' % (pc, (param.offset - 4) + adjust, param.name)


				pixHeaderPtr = "%s + %u" % (pc, adjust)
				pcPtr = "%s + %u" % (pc, param.offset + adjust)

				if not large:
					if param.img_send_null:
						condition = '(compsize > 0) && (%s != NULL)' % (param.name)
					else:
						condition = 'compsize > 0'

					print 'if (%s) {' % (condition)
					print '    (*gc->fillImage)(gc, %s, %s, %s, %s, %s, %s, %s, %s, %s);' % (dim_str, width, height, depth, param.img_format, param.img_type, param.name, pcPtr, pixHeaderPtr)
					print '} else {'
					print '    (void) memcpy( %s, default_pixel_store_%uD, default_pixel_store_%uD_size );' % (pixHeaderPtr, dim, dim)
					print '}'
				else:
					print '__glXSendLargeImage(gc, compsize, %s, %s, %s, %s, %s, %s, %s, %s, %s);' % (dim_str, width, height, depth, param.img_format, param.img_type, param.name, pcPtr, pixHeaderPtr)

		return


	def large_emit_begin(self, f, op_name = None):
		if not op_name:
			op_name = f.opcode_real_name()

		print 'const GLint op = %s;' % (op_name)
		print 'const GLuint cmdlenLarge = cmdlen + 4;'
		print 'GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);'
		print '(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);'
		print '(void) memcpy((void *)(pc + 4), (void *)(&op), 4);'
		return


	def common_func_print_just_start(self, f, name):
		print '    struct glx_context * const gc = __glXGetCurrentContext();'

		# The only reason that single and vendor private commands need
		# a variable called 'dpy' is becuase they use the SyncHandle
		# macro.  For whatever brain-dead reason, that macro is hard-
		# coded to use a variable called 'dpy' instead of taking a
		# parameter.

		# FIXME Simplify the logic related to skip_condition and
		# FIXME condition_list in this function.  Basically, remove
		# FIXME skip_condition, and just append the "dpy != NULL" type
		# FIXME condition to condition_list from the start.  The only
		# FIXME reason it's done in this confusing way now is to
		# FIXME minimize the diffs in the generated code.

		if not f.glx_rop:
			for p in f.parameterIterateOutputs():
				if p.is_image() and (p.img_format != "GL_COLOR_INDEX" or p.img_type != "GL_BITMAP"):
					print '    const __GLXattribute * const state = gc->client_state_private;'
					break

			print '    Display * const dpy = gc->currentDpy;'
			skip_condition = "dpy != NULL"
		elif f.can_be_large:
			skip_condition = "gc->currentDpy != NULL"
		else:
			skip_condition = None


		if f.return_type != 'void':
			print '    %s retval = (%s) 0;' % (f.return_type, f.return_type)


		if name != None and name not in f.glx_vendorpriv_names:
			print '#ifndef USE_XCB'
		self.emit_packet_size_calculation(f, 0)
		if name != None and name not in f.glx_vendorpriv_names:
			print '#endif'

		condition_list = []
		for p in f.parameterIterateCounters():
			condition_list.append( "%s >= 0" % (p.name) )
			# 'counter' parameters cannot be negative
			print "    if (%s < 0) {" % p.name
			print "        __glXSetError(gc, GL_INVALID_VALUE);"
			if f.return_type != 'void':
				print "        return 0;"
			else:
				print "        return;"
			print "    }"

		if skip_condition:
			condition_list.append( skip_condition )

		if len( condition_list ) > 0:
			if len( condition_list ) > 1:
				skip_condition = "(%s)" % (string.join( condition_list, ") && (" ))
			else:
				skip_condition = "%s" % (condition_list.pop(0))

			print '    if (__builtin_expect(%s, 1)) {' % (skip_condition)
			return 1
		else:
			return 0


	def printSingleFunction(self, f, name):
		self.common_func_print_just_start(f, name)

		if self.debug:
			print '        printf( "Enter %%s...\\n", "gl%s" );' % (f.name)

		if name not in f.glx_vendorpriv_names:

			# XCB specific:
			print '#ifdef USE_XCB'
			if self.debug:
				print '        printf("\\tUsing XCB.\\n");'
			print '        xcb_connection_t *c = XGetXCBConnection(dpy);'
			print '        (void) __glXFlushRenderBuffer(gc, gc->pc);'
			xcb_name = 'xcb_glx%s' % convertStringForXCB(name)

			iparams=[]
			extra_iparams = []
			output = None
			for p in f.parameterIterator():
				if p.is_output:
					output = p

					if p.is_image():
						if p.img_format != "GL_COLOR_INDEX" or p.img_type != "GL_BITMAP":
							extra_iparams.append("state->storePack.swapEndian")
						else:
							extra_iparams.append("0")
					
						# Hardcode this in.  lsb_first param (apparently always GL_FALSE)
						# also present in GetPolygonStipple, but taken care of above.
						if xcb_name == "xcb_glx_read_pixels": 
							extra_iparams.append("0")
				else:
					iparams.append(p.name)


			xcb_request = '%s(%s)' % (xcb_name, ", ".join(["c", "gc->currentContextTag"] + iparams + extra_iparams))

			if f.needs_reply():
				print '        %s_reply_t *reply = %s_reply(c, %s, NULL);' % (xcb_name, xcb_name, xcb_request)
				if output and f.reply_always_array:
					print '        (void)memcpy(%s, %s_data(reply), %s_data_length(reply) * sizeof(%s));' % (output.name, xcb_name, xcb_name, output.get_base_type_string())

				elif output and not f.reply_always_array:
					if not output.is_image():
						print '        if (%s_data_length(reply) == 0)' % (xcb_name)
						print '            (void)memcpy(%s, &reply->datum, sizeof(reply->datum));' % (output.name)
						print '        else'
					print '        (void)memcpy(%s, %s_data(reply), %s_data_length(reply) * sizeof(%s));' % (output.name, xcb_name, xcb_name, output.get_base_type_string())


				if f.return_type != 'void':
					print '        retval = reply->ret_val;'
				print '        free(reply);'
			else:
				print '        ' + xcb_request + ';'
			print '#else'
			# End of XCB specific.


		if f.parameters != []:
			pc_decl = "GLubyte const * pc ="
		else:
			pc_decl = "(void)"

		if name in f.glx_vendorpriv_names:
			print '        %s __glXSetupVendorRequest(gc, %s, %s, cmdlen);' % (pc_decl, f.opcode_real_name(), f.opcode_vendor_name(name))
		else:
			print '        %s __glXSetupSingleRequest(gc, %s, cmdlen);' % (pc_decl, f.opcode_name())

		self.common_emit_args(f, "pc", 0, 0)

		images = f.get_images()

		for img in images:
			if img.is_output:
				o = f.command_fixed_length() - 4
				print '        *(int32_t *)(pc + %u) = 0;' % (o)
				if img.img_format != "GL_COLOR_INDEX" or img.img_type != "GL_BITMAP":
					print '        * (int8_t *)(pc + %u) = state->storePack.swapEndian;' % (o)
		
				if f.img_reset:
					print '        * (int8_t *)(pc + %u) = %s;' % (o + 1, f.img_reset)


		return_name = ''
		if f.needs_reply():
			if f.return_type != 'void':
				return_name = " retval"
				return_str = " retval = (%s)" % (f.return_type)
			else:
				return_str = " (void)"

			got_reply = 0

			for p in f.parameterIterateOutputs():
				if p.is_image():
					[dim, w, h, d, junk] = p.get_dimensions()
					if f.dimensions_in_reply:
						print "        __glXReadPixelReply(dpy, gc, %u, 0, 0, 0, %s, %s, %s, GL_TRUE);" % (dim, p.img_format, p.img_type, p.name)
					else:
						print "        __glXReadPixelReply(dpy, gc, %u, %s, %s, %s, %s, %s, %s, GL_FALSE);" % (dim, w, h, d, p.img_format, p.img_type, p.name)

					got_reply = 1
				else:
					if f.reply_always_array:
						aa = "GL_TRUE"
					else:
						aa = "GL_FALSE"

					# gl_parameter.size() returns the size
					# of the entire data item.  If the
					# item is a fixed-size array, this is
					# the size of the whole array.  This
					# is not what __glXReadReply wants. It
					# wants the size of a single data
					# element in the reply packet.
					# Dividing by the array size (1 for
					# non-arrays) gives us this.

					s = p.size() / p.get_element_count()
					print "       %s __glXReadReply(dpy, %s, %s, %s);" % (return_str, s, p.name, aa)
					got_reply = 1


			# If a reply wasn't read to fill an output parameter,
			# read a NULL reply to get the return value.

			if not got_reply:
				print "       %s __glXReadReply(dpy, 0, NULL, GL_FALSE);" % (return_str)


		elif self.debug:
			# Only emit the extra glFinish call for functions
			# that don't already require a reply from the server.
			print '        __indirect_glFinish();'

		if self.debug:
			print '        printf( "Exit %%s.\\n", "gl%s" );' % (name)


		print '        UnlockDisplay(dpy); SyncHandle();'

		if name not in f.glx_vendorpriv_names:
			print '#endif /* USE_XCB */'

		print '    }'
		print '    return%s;' % (return_name)
		return


	def printPixelFunction(self, f):
		if self.pixel_stubs.has_key( f.name ):
			# Normally gl_function::get_parameter_string could be
			# used.  However, this call needs to have the missing
			# dimensions (e.g., a fake height value for
			# glTexImage1D) added in.

			p_string = ""
			for param in f.parameterIterateGlxSend():
				if param.is_padding:
					continue

				p_string += ", " + param.name

				if param.is_image():
					[dim, junk, junk, junk, junk] = param.get_dimensions()

				if f.pad_after(param):
					p_string += ", 1"

			print '    %s(%s, %u%s );' % (self.pixel_stubs[f.name] , f.opcode_name(), dim, p_string)
			return


		if self.common_func_print_just_start(f, None):
			trailer = "    }"
		else:
			trailer = None


		if f.can_be_large:
			print 'if (cmdlen <= gc->maxSmallRenderCommandSize) {'
			print '    if ( (gc->pc + cmdlen) > gc->bufEnd ) {'
			print '        (void) __glXFlushRenderBuffer(gc, gc->pc);'
			print '    }'

		if f.glx_rop == ~0:
			opcode = "opcode"
		else:
			opcode = f.opcode_real_name()

		print 'emit_header(gc->pc, %s, cmdlen);' % (opcode)

		self.pixel_emit_args( f, "gc->pc", 0 )
		print 'gc->pc += cmdlen;'
		print 'if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }'

		if f.can_be_large:
			print '}'
			print 'else {'

			self.large_emit_begin(f, opcode)
			self.pixel_emit_args(f, "pc", 1)

			print '}'

		if trailer: print trailer
		return


	def printRenderFunction(self, f):
		# There is a class of GL functions that take a single pointer
		# as a parameter.  This pointer points to a fixed-size chunk
		# of data, and the protocol for this functions is very
		# regular.  Since they are so regular and there are so many
		# of them, special case them with generic functions.  On
		# x86, this saves about 26KB in the libGL.so binary.

		if f.variable_length_parameter() == None and len(f.parameters) == 1:
			p = f.parameters[0]
			if p.is_pointer():
				cmdlen = f.command_fixed_length()
				if cmdlen in self.generic_sizes:
					print '    generic_%u_byte( %s, %s );' % (cmdlen, f.opcode_real_name(), p.name)
					return

		if self.common_func_print_just_start(f, None):
			trailer = "    }"
		else:
			trailer = None

		if self.debug:
			print 'printf( "Enter %%s...\\n", "gl%s" );' % (f.name)

		if f.can_be_large:
			print 'if (cmdlen <= gc->maxSmallRenderCommandSize) {'
			print '    if ( (gc->pc + cmdlen) > gc->bufEnd ) {'
			print '        (void) __glXFlushRenderBuffer(gc, gc->pc);'
			print '    }'

		print 'emit_header(gc->pc, %s, cmdlen);' % (f.opcode_real_name())

		self.common_emit_args(f, "gc->pc", 4, 0)
		print 'gc->pc += cmdlen;'
		print 'if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }'

		if f.can_be_large:
			print '}'
			print 'else {'

			self.large_emit_begin(f)
			self.common_emit_args(f, "pc", 8, 1)

			p = f.variable_length_parameter()
			print '    __glXSendLargeCommand(gc, pc, %u, %s, %s);' % (p.offset + 8, p.name, p.size_string())
			print '}'

		if self.debug:
			print '__indirect_glFinish();'
			print 'printf( "Exit %%s.\\n", "gl%s" );' % (f.name)

		if trailer: print trailer
		return


class PrintGlxProtoInit_c(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "glX_proto_send.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
(C) Copyright IBM Corporation 2004""", "PRECISION INSIGHT, IBM")
		return


	def printRealHeader(self):
		print """/**
 * \\file indirect_init.c
 * Initialize indirect rendering dispatch table.
 *
 * \\author Kevin E. Martin <kevin@precisioninsight.com>
 * \\author Brian Paul <brian@precisioninsight.com>
 * \\author Ian Romanick <idr@us.ibm.com>
 */

#include "indirect_init.h"
#include "indirect.h"
#include "glapi.h"


/**
 * No-op function used to initialize functions that have no GLX protocol
 * support.
 */
static int NoOp(void)
{
    return 0;
}

/**
 * Create and initialize a new GL dispatch table.  The table is initialized
 * with GLX indirect rendering protocol functions.
 */
struct _glapi_table * __glXNewIndirectAPI( void )
{
    struct _glapi_table *glAPI;
    GLuint entries;

    entries = _glapi_get_dispatch_table_size();
    glAPI = (struct _glapi_table *) Xmalloc(entries * sizeof(void *));

    /* first, set all entries to point to no-op functions */
    {
       int i;
       void **dispatch = (void **) glAPI;
       for (i = 0; i < entries; i++) {
          dispatch[i] = (void *) NoOp;
       }
    }

    /* now, initialize the entries we understand */"""

	def printRealFooter(self):
		print """
    return glAPI;
}
"""
		return


	def printBody(self, api):
		for [name, number] in api.categoryIterate():
			if number != None:
				preamble = '\n    /* %3u. %s */\n\n' % (int(number), name)
			else:
				preamble = '\n    /* %s */\n\n' % (name)

			for func in api.functionIterateByCategory(name):
				if func.client_supported_for_indirect():
					print '%s    glAPI->%s = __indirect_gl%s;' % (preamble, func.name, func.name)
					preamble = ''

		return


class PrintGlxProtoInit_h(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "glX_proto_send.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
(C) Copyright IBM Corporation 2004""", "PRECISION INSIGHT, IBM")
		self.header_tag = "_INDIRECT_H_"

		self.last_category = ""
		return


	def printRealHeader(self):
		print """/**
 * \\file
 * Prototypes for indirect rendering functions.
 *
 * \\author Kevin E. Martin <kevin@precisioninsight.com>
 * \\author Ian Romanick <idr@us.ibm.com>
 */
"""
		self.printFastcall()
		self.printNoinline()

		print """
#include <X11/Xfuncproto.h>
#include "glxclient.h"

extern _X_HIDDEN NOINLINE CARD32 __glXReadReply( Display *dpy, size_t size,
    void * dest, GLboolean reply_is_always_array );

extern _X_HIDDEN NOINLINE void __glXReadPixelReply( Display *dpy,
    struct glx_context * gc, unsigned max_dim, GLint width, GLint height,
    GLint depth, GLenum format, GLenum type, void * dest,
    GLboolean dimensions_in_reply );

extern _X_HIDDEN NOINLINE FASTCALL GLubyte * __glXSetupSingleRequest(
    struct glx_context * gc, GLint sop, GLint cmdlen );

extern _X_HIDDEN NOINLINE FASTCALL GLubyte * __glXSetupVendorRequest(
    struct glx_context * gc, GLint code, GLint vop, GLint cmdlen );
"""


	def printBody(self, api):
		for func in api.functionIterateGlx():
			params = func.get_parameter_string()

			print 'extern _X_HIDDEN %s __indirect_gl%s(%s);' % (func.return_type, func.name, params)

			for n in func.entry_points:
				if func.has_different_protocol(n):
					asdf = func.static_glx_name(n)
					if asdf not in func.static_entry_points:
						print 'extern _X_HIDDEN %s gl%s(%s);' % (func.return_type, asdf, params)
						# give it a easy-to-remember name
						if func.client_handcode:
							print '#define gl_dispatch_stub_%s gl%s' % (n, asdf)
					else:
						print 'GLAPI %s GLAPIENTRY gl%s(%s);' % (func.return_type, asdf, params)
						
					break

		print ''
		print '#ifdef GLX_SHARED_GLAPI'
		print 'extern _X_HIDDEN void (*__indirect_get_proc_address(const char *name))(void);'
		print '#endif'


def show_usage():
	print "Usage: %s [-f input_file_name] [-m output_mode] [-d]" % sys.argv[0]
	print "    -m output_mode   Output mode can be one of 'proto', 'init_c' or 'init_h'."
	print "    -d               Enable extra debug information in the generated code."
	sys.exit(1)


if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m:d")
	except Exception,e:
		show_usage()

	debug = 0
	mode = "proto"
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-m":
			mode = val
		elif arg == "-d":
			debug = 1

	if mode == "proto":
		printer = PrintGlxProtoStubs()
	elif mode == "init_c":
		printer = PrintGlxProtoInit_c()
	elif mode == "init_h":
		printer = PrintGlxProtoInit_h()
	else:
		show_usage()


	printer.debug = debug
	api = gl_XML.parse_GL_API( file_name, glX_XML.glx_item_factory() )

	printer.Print( api )
