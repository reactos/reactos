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
import string


class glx_proto_item_factory(glX_XML.glx_item_factory):
	"""Factory to create GLX protocol oriented objects derived from gl_item."""
    
	def create_item(self, name, element, context):
		if name == "type":
			return glx_proto_type(element, context)
		else:
			return glX_XML.glx_item_factory.create_item(self, name, element, context)


class glx_proto_type(gl_XML.gl_type):
	def __init__(self, element, context):
		gl_XML.gl_type.__init__(self, element, context)

		self.glx_name = element.nsProp( "glx_name", None )
		return


class glx_print_proto(gl_XML.gl_print_base):
	def size_call(self, func, outputs_also = 0):
		"""Create C code to calculate 'compsize'.

		Creates code to calculate 'compsize'.  If the function does
		not need 'compsize' to be calculated, None will be
		returned."""
	
		compsize = None

		for param in func.parameterIterator():
			if outputs_also or not param.is_output:
				if param.is_image():
					[dim, w, h, d, junk] = param.get_dimensions()

					compsize = '__glImageSize(%s, %s, %s, %s, %s, %s)' % (w, h, d, param.img_format, param.img_type, param.img_target)
					if not param.img_send_null:
						compsize = '(%s != NULL) ? %s : 0' % (param.name, compsize)

					return compsize

				elif len(param.count_parameter_list):
					parameters = string.join( param.count_parameter_list, "," )
					compsize = "__gl%s_size(%s)" % (func.name, parameters)

					return compsize

		return None


	def emit_packet_size_calculation(self, f, bias):
		# compsize is only used in the command size calculation if
		# the function has a non-output parameter that has a non-empty
		# counter_parameter_list.

		compsize = self.size_call(f)
		if compsize:
			print '    const GLuint compsize = %s;' % (compsize)

		if bias:
			print '    const GLuint cmdlen = %s - %u;' % (f.command_length(), bias)
		else:
			print '    const GLuint cmdlen = %s;' % (f.command_length())

		#print ''
		return compsize
