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

import gl_XML, glX_XML, glX_proto_common, license
import sys, getopt


class glx_doc_item_factory(glX_proto_common.glx_proto_item_factory):
	"""Factory to create GLX protocol documentation oriented objects derived from glItem."""
    
	def create_item(self, name, element, context):
		if name == "parameter":
			return glx_doc_parameter(element, context)
		else:
			return glX_proto_common.glx_proto_item_factory.create_item(self, name, element, context)


class glx_doc_parameter(gl_XML.gl_parameter):
	def packet_type(self, type_dict):
		"""Get the type string for the packet header
		
		GLX protocol documentation uses type names like CARD32,
		FLOAT64, LISTofCARD8, and ENUM.  This function converts the
		type of the parameter to one of these names."""

		list_of = ""
		if self.is_array():
			list_of = "LISTof"

		t_name = self.get_base_type_string()
		if not type_dict.has_key( t_name ):
			type_name = "CARD8"
		else:
			type_name = type_dict[ t_name ]

		return "%s%s" % (list_of, type_name)


	def packet_size(self):
		p = None
		s = self.size()
		if s == 0:
			a_prod = "n"
			b_prod = self.p_type.size

			if not self.count_parameter_list and self.counter:
				a_prod = self.counter
			elif self.count_parameter_list and not self.counter or self.is_output:
				pass
			elif self.count_parameter_list and self.counter:
				b_prod = self.counter
			else:
				raise RuntimeError("Parameter '%s' to function '%s' has size 0." % (self.name, self.context.name))

			ss = "%s*%s" % (a_prod, b_prod)

			return [ss, p]
		else:
			if s % 4 != 0:
				p = "p"

			return [str(s), p]

class PrintGlxProtoText(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)
		self.license = ""


	def printHeader(self):
		return


	def body_size(self, f):
		# At some point, refactor this function and
		# glXFunction::command_payload_length.

		size = 0;
		size_str = ""
		pad_str = ""
		plus = ""
		for p in f.parameterIterateGlxSend():
			[s, pad] = p.packet_size()
			try: 
				size += int(s)
			except Exception,e:
				size_str += "%s%s" % (plus, s)
				plus = "+"

			if pad != None:
				pad_str = pad

		return [size, size_str, pad_str]


	def print_render_header(self, f):
		[size, size_str, pad_str] = self.body_size(f)
		size += 4;

		if size_str == "":
			s = "%u" % ((size + 3) & ~3)
		elif pad_str != "":
			s = "%u+%s+%s" % (size, size_str, pad_str)
		else:
			s = "%u+%s" % (size, size_str)

		print '            2        %-15s rendering command length' % (s)
		print '            2        %-4u            rendering command opcode' % (f.glx_rop)
		return


	def print_single_header(self, f):
		[size, size_str, pad_str] = self.body_size(f)
		size = ((size + 3) / 4) + 2;

		if f.glx_vendorpriv != 0:
			size += 1

		print '            1        CARD8           opcode (X assigned)'
		print '            1        %-4u            GLX opcode (%s)' % (f.opcode_real_value(), f.opcode_real_name())

		if size_str == "":
			s = "%u" % (size)
		elif pad_str != "":
			s = "%u+((%s+%s)/4)" % (size, size_str, pad_str)
		else:
			s = "%u+((%s)/4)" % (size, size_str)

		print '            2        %-15s request length' % (s)

		if f.glx_vendorpriv != 0:
			print '            4        %-4u            vendor specific opcode' % (f.opcode_value())
			
		print '            4        GLX_CONTEXT_TAG context tag'

		return
		

	def print_reply(self, f):
		print '          =>'
		print '            1        1               reply'
		print '            1                        unused'
		print '            2        CARD16          sequence number'

		if f.output == None:
			print '            4        0               reply length'
		elif f.reply_always_array:
			print '            4        m               reply length'
		else:
			print '            4        m               reply length, m = (n == 1 ? 0 : n)'


		output = None
		for x in f.parameterIterateOutputs():
			output = x
			break


		unused = 24
		if f.return_type != 'void':
			print '            4        %-15s return value' % (f.return_type)
			unused -= 4
		elif output != None:
			print '            4                        unused'
			unused -= 4

		if output != None:
			print '            4        CARD32          n'
			unused -= 4

		if output != None:
			if not f.reply_always_array:
				print ''
				print '            if (n = 1) this follows:'
				print ''
				print '            4        CARD32          %s' % (output.name)
				print '            %-2u                       unused' % (unused - 4)
				print ''
				print '            otherwise this follows:'
				print ''

			print '            %-2u                       unused' % (unused)

			[s, pad] = output.packet_size()
			print '            %-8s %-15s %s' % (s, output.packet_type( self.type_map ), output.name)
			if pad != None:
				try:
					bytes = int(s)
					bytes = 4 - (bytes & 3)
					print '            %-8u %-15s unused' % (bytes, "")
				except Exception,e:
					print '            %-8s %-15s unused, %s=pad(%s)' % (pad, "", pad, s)
		else:
			print '            %-2u                       unused' % (unused)


	def print_body(self, f):
		for p in f.parameterIterateGlxSend():
			[s, pad] = p.packet_size()
			print '            %-8s %-15s %s' % (s, p.packet_type( self.type_map ), p.name)
			if pad != None:
				try:
					bytes = int(s)
					bytes = 4 - (bytes & 3)
					print '            %-8u %-15s unused' % (bytes, "")
				except Exception,e:
					print '            %-8s %-15s unused, %s=pad(%s)' % (pad, "", pad, s)

	def printBody(self, api):
		self.type_map = {}
		for t in api.typeIterate():
			self.type_map[ "GL" + t.name ] = t.glx_name


		# At some point this should be expanded to support pixel
		# functions, but I'm not going to lose any sleep over it now.

		for f in api.functionIterateByOffset():
			if f.client_handcode or f.server_handcode or f.vectorequiv or len(f.get_images()):
				continue


			if f.glx_rop:
				print '        %s' % (f.name)
				self.print_render_header(f)
			elif f.glx_sop or f.glx_vendorpriv:
				print '        %s' % (f.name)
				self.print_single_header(f)
			else:
				continue

			self.print_body(f)

			if f.needs_reply():
				self.print_reply(f)

			print ''
		return


if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == "-f":
			file_name = val

	api = gl_XML.parse_GL_API( file_name, glx_doc_item_factory() )

	printer = PrintGlxProtoText()
	printer.Print( api )
