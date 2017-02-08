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

import gl_XML
import license
import sys, getopt, string


class glx_item_factory(gl_XML.gl_item_factory):
	"""Factory to create GLX protocol oriented objects derived from gl_item."""
    
	def create_item(self, name, element, context):
		if name == "function":
			return glx_function(element, context)
		elif name == "enum":
			return glx_enum(element, context)
		elif name == "api":
			return glx_api(self)
		else:
			return gl_XML.gl_item_factory.create_item(self, name, element, context)


class glx_enum(gl_XML.gl_enum):
	def __init__(self, element, context):
		gl_XML.gl_enum.__init__(self, element, context)
		
		self.functions = {}

		child = element.children
		while child:
			if child.type == "element" and child.name == "size":
				n = child.nsProp( "name", None )
				c = child.nsProp( "count", None )
				m = child.nsProp( "mode", None )
				
				if not c:
					c = self.default_count
				else:
					c = int(c)

				if m == "get":
					mode = 0
				else:
					mode = 1

				if not self.functions.has_key(n):
					self.functions[ n ] = [c, mode]

			child = child.next

		return


class glx_function(gl_XML.gl_function):
	def __init__(self, element, context):
		self.glx_rop = 0
		self.glx_sop = 0
		self.glx_vendorpriv = 0

		self.glx_vendorpriv_names = []

		# If this is set to true, it means that GLdouble parameters should be
		# written to the GLX protocol packet in the order they appear in the
		# prototype.  This is different from the "classic" ordering.  In the
		# classic ordering GLdoubles are written to the protocol packet first,
		# followed by non-doubles.  NV_vertex_program was the first extension
		# to break with this tradition.

		self.glx_doubles_in_order = 0

		self.vectorequiv = None
		self.output = None
		self.can_be_large = 0
		self.reply_always_array = 0
		self.dimensions_in_reply = 0
		self.img_reset = None

		self.server_handcode = 0
		self.client_handcode = 0
		self.ignore = 0

		self.count_parameter_list = []
		self.counter_list = []
		self.parameters_by_name = {}
		self.offsets_calculated = 0

		gl_XML.gl_function.__init__(self, element, context)
		return


	def process_element(self, element):
		gl_XML.gl_function.process_element(self, element)

		# If the function already has a vector equivalent set, don't
		# set it again.  This can happen if an alias to a function
		# appears after the function that it aliases.

		if not self.vectorequiv:
			self.vectorequiv = element.nsProp("vectorequiv", None)


		name = element.nsProp("name", None)
		if name == self.name:
			for param in self.parameters:
				self.parameters_by_name[ param.name ] = param
				
				if len(param.count_parameter_list):
					self.count_parameter_list.extend( param.count_parameter_list )
				
				if param.counter and param.counter not in self.counter_list:
					self.counter_list.append(param.counter)


		child = element.children
		while child:
			if child.type == "element" and child.name == "glx":
				rop = child.nsProp( 'rop', None )
				sop = child.nsProp( 'sop', None )
				vop = child.nsProp( 'vendorpriv', None )

				if rop:
					self.glx_rop = int(rop)

				if sop:
					self.glx_sop = int(sop)

				if vop:
					self.glx_vendorpriv = int(vop)
					self.glx_vendorpriv_names.append(name)

				self.img_reset = child.nsProp( 'img_reset', None )

				# The 'handcode' attribute can be one of 'true',
				# 'false', 'client', or 'server'.

				handcode = child.nsProp( 'handcode', None )
				if handcode == "false":
					self.server_handcode = 0
					self.client_handcode = 0
				elif handcode == "true":
					self.server_handcode = 1
					self.client_handcode = 1
				elif handcode == "client":
					self.server_handcode = 0
					self.client_handcode = 1
				elif handcode == "server":
					self.server_handcode = 1
					self.client_handcode = 0
				else:
					raise RuntimeError('Invalid handcode mode "%s" in function "%s".' % (handcode, self.name))

				self.ignore               = gl_XML.is_attr_true( child, 'ignore' )
				self.can_be_large         = gl_XML.is_attr_true( child, 'large' )
				self.glx_doubles_in_order = gl_XML.is_attr_true( child, 'doubles_in_order' )
				self.reply_always_array   = gl_XML.is_attr_true( child, 'always_array' )
				self.dimensions_in_reply  = gl_XML.is_attr_true( child, 'dimensions_in_reply' )

			child = child.next


		# Do some validation of the GLX protocol information.  As
		# new tests are discovered, they should be added here.

		for param in self.parameters:
			if param.is_output and self.glx_rop != 0:
				raise RuntimeError("Render / RenderLarge commands cannot have outputs (%s)." % (self.name))

		return


	def has_variable_size_request(self):
		"""Determine if the GLX request packet is variable sized.

		The GLX request packet is variable sized in several common
		situations.
		
		1. The function has a non-output parameter that is counted
		   by another parameter (e.g., the 'textures' parameter of
		   glDeleteTextures).
		   
		2. The function has a non-output parameter whose count is
		   determined by another parameter that is an enum (e.g., the
		   'params' parameter of glLightfv).
		   
		3. The function has a non-output parameter that is an
		   image.

		4. The function must be hand-coded on the server.
		"""

		if self.glx_rop == 0:
			return 0

		if self.server_handcode or self.images:
			return 1

		for param in self.parameters:
			if not param.is_output:
				if param.counter or len(param.count_parameter_list):
					return 1

		return 0


	def variable_length_parameter(self):
		for param in self.parameters:
			if not param.is_output:
				if param.counter or len(param.count_parameter_list):
					return param
				
		return None


	def calculate_offsets(self):
		if not self.offsets_calculated:
			# Calculate the offset of the first function parameter
			# in the GLX command packet.  This byte offset is
			# measured from the end of the Render / RenderLarge
			# header.  The offset for all non-pixel commends is
			# zero.  The offset for pixel commands depends on the
			# number of dimensions of the pixel data.

			if len(self.images) and not self.images[0].is_output:
				[dim, junk, junk, junk, junk] = self.images[0].get_dimensions()

				# The base size is the size of the pixel pack info
				# header used by images with the specified number
				# of dimensions.

				if dim <=  2:
					offset = 20
				elif dim <= 4:
					offset = 36
				else:
					raise RuntimeError('Invalid number of dimensions %u for parameter "%s" in function "%s".' % (dim, self.image.name, self.name))
			else:
				offset = 0

			for param in self.parameterIterateGlxSend():
				if param.img_null_flag:
					offset += 4

				if param.name != self.img_reset:
					param.offset = offset
					if not param.is_variable_length() and not param.is_client_only:
						offset += param.size()
					
				if self.pad_after( param ):
					offset += 4


			self.offsets_calculated = 1
		return


	def offset_of(self, param_name):
		self.calculate_offsets()
		return self.parameters_by_name[ param_name ].offset


	def parameterIterateGlxSend(self, include_variable_parameters = 1):
		"""Create an iterator for parameters in GLX request order."""

		# The parameter lists are usually quite short, so it's easier
		# (i.e., less code) to just generate a new list with the
		# required elements than it is to create a new iterator class.
		
		temp = [ [],  [], [] ]
		for param in self.parameters:
			if param.is_output: continue

			if param.is_variable_length():
				temp[2].append( param )
			elif not self.glx_doubles_in_order and param.is_64_bit():
				temp[0].append( param )
			else:
				temp[1].append( param )

		parameters = temp[0]
		parameters.extend( temp[1] )
		if include_variable_parameters:
			parameters.extend( temp[2] )
		return parameters.__iter__()


	def parameterIterateCounters(self):
		temp = []
		for name in self.counter_list:
			temp.append( self.parameters_by_name[ name ] )

		return temp.__iter__()


	def parameterIterateOutputs(self):
		temp = []
		for p in self.parameters:
			if p.is_output:
				temp.append( p )

		return temp


	def command_fixed_length(self):
		"""Return the length, in bytes as an integer, of the
		fixed-size portion of the command."""

		if len(self.parameters) == 0:
			return 0
		
		self.calculate_offsets()

		size = 0
		for param in self.parameterIterateGlxSend(0):
			if param.name != self.img_reset and not param.is_client_only:
				if size == 0:
					size = param.offset + param.size()
				else:
					size += param.size()

				if self.pad_after( param ):
					size += 4

		for param in self.images:
			if param.img_null_flag or param.is_output:
				size += 4

		return size


	def command_variable_length(self):
		"""Return the length, as a string, of the variable-sized
		portion of the command."""

		size_string = ""
		for p in self.parameterIterateGlxSend():
			if (not p.is_output) and (p.is_variable_length() or p.is_image()):
				# FIXME Replace the 1 in the size_string call
				# FIXME w/0 to eliminate some un-needed parnes
				# FIXME This would already be done, but it
				# FIXME adds some extra diffs to the generated
				# FIXME code.

				size_string = size_string + " + __GLX_PAD(%s)" % (p.size_string(1))

		return size_string


	def command_length(self):
		size = self.command_fixed_length()

		if self.glx_rop != 0:
			size += 4

		size = ((size + 3) & ~3)
		return "%u%s" % (size, self.command_variable_length())


	def opcode_real_value(self):
		"""Get the true numeric value of the GLX opcode
		
		Behaves similarly to opcode_value, except for
		X_GLXVendorPrivate and X_GLXVendorPrivateWithReply commands.
		In these cases the value for the GLX opcode field (i.e.,
		16 for X_GLXVendorPrivate or 17 for
		X_GLXVendorPrivateWithReply) is returned.  For other 'single'
		commands, the opcode for the command (e.g., 101 for
		X_GLsop_NewList) is returned."""

		if self.glx_vendorpriv != 0:
			if self.needs_reply():
				return 17
			else:
				return 16
		else:
			return self.opcode_value()


	def opcode_value(self):
		"""Get the unique protocol opcode for the glXFunction"""

		if (self.glx_rop == 0) and self.vectorequiv:
			equiv = self.context.functions_by_name[ self.vectorequiv ]
			self.glx_rop = equiv.glx_rop


		if self.glx_rop != 0:
			return self.glx_rop
		elif self.glx_sop != 0:
			return self.glx_sop
		elif self.glx_vendorpriv != 0:
			return self.glx_vendorpriv
		else:
			return -1
	

	def opcode_rop_basename(self):
		"""Return either the name to be used for GLX protocol enum.
		
		Returns either the name of the function or the name of the
		name of the equivalent vector (e.g., glVertex3fv for
		glVertex3f) function."""

		if self.vectorequiv == None:
			return self.name
		else:
			return self.vectorequiv


	def opcode_name(self):
		"""Get the unique protocol enum name for the glXFunction"""

		if (self.glx_rop == 0) and self.vectorequiv:
			equiv = self.context.functions_by_name[ self.vectorequiv ]
			self.glx_rop = equiv.glx_rop
			self.glx_doubles_in_order = equiv.glx_doubles_in_order


		if self.glx_rop != 0:
			return "X_GLrop_%s" % (self.opcode_rop_basename())
		elif self.glx_sop != 0:
			return "X_GLsop_%s" % (self.name)
		elif self.glx_vendorpriv != 0:
			return "X_GLvop_%s" % (self.name)
		else:
			raise RuntimeError('Function "%s" has no opcode.' % (self.name))


	def opcode_vendor_name(self, name):
		if name in self.glx_vendorpriv_names:
			return "X_GLvop_%s" % (name)
		else:
			raise RuntimeError('Function "%s" has no VendorPrivate opcode.' % (name))


	def opcode_real_name(self):
		"""Get the true protocol enum name for the GLX opcode
		
		Behaves similarly to opcode_name, except for
		X_GLXVendorPrivate and X_GLXVendorPrivateWithReply commands.
		In these cases the string 'X_GLXVendorPrivate' or
		'X_GLXVendorPrivateWithReply' is returned.  For other
		single or render commands 'X_GLsop' or 'X_GLrop' plus the
		name of the function returned."""

		if self.glx_vendorpriv != 0:
			if self.needs_reply():
				return "X_GLXVendorPrivateWithReply"
			else:
				return "X_GLXVendorPrivate"
		else:
			return self.opcode_name()


	def needs_reply(self):
		try:
			x = self._needs_reply
		except Exception, e:
			x = 0
			if self.return_type != 'void':
				x = 1

			for param in self.parameters:
				if param.is_output:
					x = 1
					break

			self._needs_reply = x

		return x


	def pad_after(self, p):
		"""Returns the name of the field inserted after the
		specified field to pad out the command header."""

		for image in self.images:
			if image.img_pad_dimensions:
				if not image.height:
					if p.name == image.width:
						return "height"
					elif p.name == image.img_xoff:
						return "yoffset"
				elif not image.extent:
					if p.name == image.depth:
						# Should this be "size4d"?
						return "extent"
					elif p.name == image.img_zoff:
						return "woffset"

		return None


	def has_different_protocol(self, name):
		"""Returns true if the named version of the function uses different protocol from the other versions.
		
		Some functions, such as glDeleteTextures and
		glDeleteTexturesEXT are functionally identical, but have
		different protocol.  This function returns true if the
		named function is an alias name and that named version uses
		different protocol from the function that is aliased.
		"""

		return (name in self.glx_vendorpriv_names) and self.glx_sop


	def static_glx_name(self, name):
		if self.has_different_protocol(name):
			for n in self.glx_vendorpriv_names:
				if n in self.static_entry_points:
					return n
				
		return self.static_name(name)


	def client_supported_for_indirect(self):
		"""Returns true if the function is supported on the client
		side for indirect rendering."""

		return not self.ignore and (self.offset != -1) and (self.glx_rop or self.glx_sop or self.glx_vendorpriv or self.vectorequiv or self.client_handcode)


class glx_function_iterator:
	"""Class to iterate over a list of glXFunctions"""

	def __init__(self, context):
		self.iterator = context.functionIterateByOffset()
		return


	def __iter__(self):
		return self


	def next(self):
		f = self.iterator.next()

		if f.client_supported_for_indirect():
			return f
		else:
			return self.next()


class glx_api(gl_XML.gl_api):
	def functionIterateGlx(self):
		return glx_function_iterator(self)

