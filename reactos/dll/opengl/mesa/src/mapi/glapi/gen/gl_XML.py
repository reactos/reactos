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

import libxml2
import re, sys, string
import typeexpr


def parse_GL_API( file_name, factory = None ):
	doc = libxml2.readFile( file_name, None, libxml2.XML_PARSE_XINCLUDE + libxml2.XML_PARSE_NOBLANKS + libxml2.XML_PARSE_DTDVALID + libxml2.XML_PARSE_DTDATTR + libxml2.XML_PARSE_DTDLOAD + libxml2.XML_PARSE_NOENT )
	ret = doc.xincludeProcess()

	if not factory:
		factory = gl_item_factory()

	api = factory.create_item( "api", None, None )
	api.process_element( doc )

	# After the XML has been processed, we need to go back and assign
	# dispatch offsets to the functions that request that their offsets
	# be assigned by the scripts.  Typically this means all functions
	# that are not part of the ABI.

	for func in api.functionIterateByCategory():
		if func.assign_offset:
			func.offset = api.next_offset;
			api.next_offset += 1

	doc.freeDoc()

	return api


def is_attr_true( element, name ):
	"""Read a name value from an element's attributes.
	
	The value read from the attribute list must be either 'true' or
	'false'.  If the value is 'false', zero will be returned.  If the
	value is 'true', non-zero will be returned.  An exception will be
	raised for any other value."""

	value = element.nsProp( name, None )
	if value == "true":
		return 1
	elif value == "false":
		return 0
	else:
		raise RuntimeError('Invalid value "%s" for boolean "%s".' % (value, name))


class gl_print_base:
	"""Base class of all API pretty-printers.

	In the model-view-controller pattern, this is the view.  Any derived
	class will want to over-ride the printBody, printRealHader, and
	printRealFooter methods.  Some derived classes may want to over-ride
	printHeader and printFooter, or even Print (though this is unlikely).
	"""

	def __init__(self):
		# Name of the script that is generating the output file.
		# Every derived class should set this to the name of its
		# source file.

		self.name = "a"


		# License on the *generated* source file.  This may differ
		# from the license on the script that is generating the file.
		# Every derived class should set this to some reasonable
		# value.
		#
		# See license.py for an example of a reasonable value.

		self.license = "The license for this file is unspecified."

		
		# The header_tag is the name of the C preprocessor define
		# used to prevent multiple inclusion.  Typically only
		# generated C header files need this to be set.  Setting it
		# causes code to be generated automatically in printHeader
		# and printFooter.

		self.header_tag = None

		
		# List of file-private defines that must be undefined at the
		# end of the file.  This can be used in header files to define
		# names for use in the file, then undefine them at the end of
		# the header file.

		self.undef_list = []
		return


	def Print(self, api):
		self.printHeader()
		self.printBody(api)
		self.printFooter()
		return


	def printHeader(self):
		"""Print the header associated with all files and call the printRealHeader method."""

		print '/* DO NOT EDIT - This file generated automatically by %s script */' \
			% (self.name)
		print ''
		print '/*'
		print ' * ' + self.license.replace('\n', '\n * ')
		print ' */'
		print ''
		if self.header_tag:
		    print '#if !defined( %s )' % (self.header_tag)
		    print '#  define %s' % (self.header_tag)
		    print ''
		self.printRealHeader();
		return


	def printFooter(self):
		"""Print the header associated with all files and call the printRealFooter method."""

		self.printRealFooter()

		if self.undef_list:
			print ''
			for u in self.undef_list:
				print "#  undef %s" % (u)

		if self.header_tag:
			print ''
			print '#endif /* !defined( %s ) */' % (self.header_tag)


	def printRealHeader(self):
		"""Print the "real" header for the created file.

		In the base class, this function is empty.  All derived
		classes should over-ride this function."""
		return


	def printRealFooter(self):
		"""Print the "real" footer for the created file.

		In the base class, this function is empty.  All derived
		classes should over-ride this function."""
		return


	def printPure(self):
		"""Conditionally define `PURE' function attribute.

		Conditionally defines a preprocessor macro `PURE' that wraps
		GCC's `pure' function attribute.  The conditional code can be
		easilly adapted to other compilers that support a similar
		feature.

		The name is also added to the file's undef_list.
		"""
		self.undef_list.append("PURE")
		print """#  if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define PURE __attribute__((pure))
#  else
#    define PURE
#  endif"""
		return


	def printFastcall(self):
		"""Conditionally define `FASTCALL' function attribute.

		Conditionally defines a preprocessor macro `FASTCALL' that
		wraps GCC's `fastcall' function attribute.  The conditional
		code can be easilly adapted to other compilers that support a
		similar feature.

		The name is also added to the file's undef_list.
		"""

		self.undef_list.append("FASTCALL")
		print """#  if defined(__i386__) && defined(__GNUC__) && !defined(__CYGWIN__) && !defined(__MINGW32__)
#    define FASTCALL __attribute__((fastcall))
#  else
#    define FASTCALL
#  endif"""
		return


	def printVisibility(self, S, s):
		"""Conditionally define visibility function attribute.

		Conditionally defines a preprocessor macro name S that wraps
		GCC's visibility function attribute.  The visibility used is
		the parameter s.  The conditional code can be easilly adapted
		to other compilers that support a similar feature.

		The name is also added to the file's undef_list.
		"""

		self.undef_list.append(S)
		print """#  if (defined(__GNUC__) && !defined(__CYGWIN__) && !defined(__MINGW32__)) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590) && defined(__ELF__))
#    define %s  __attribute__((visibility("%s")))
#  else
#    define %s
#  endif""" % (S, s, S)
		return


	def printNoinline(self):
		"""Conditionally define `NOINLINE' function attribute.

		Conditionally defines a preprocessor macro `NOINLINE' that
		wraps GCC's `noinline' function attribute.  The conditional
		code can be easilly adapted to other compilers that support a
		similar feature.

		The name is also added to the file's undef_list.
		"""

		self.undef_list.append("NOINLINE")
		print """#  if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define NOINLINE __attribute__((noinline))
#  else
#    define NOINLINE
#  endif"""
		return


def real_function_name(element):
	name = element.nsProp( "name", None )
	alias = element.nsProp( "alias", None )
	
	if alias:
		return alias
	else:
		return name


def real_category_name(c):
	if re.compile("[1-9][0-9]*[.][0-9]+").match(c):
		return "GL_VERSION_" + c.replace(".", "_")
	else:
		return c


def classify_category(name, number):
	"""Based on the category name and number, select a numerical class for it.
	
	Categories are divided into four classes numbered 0 through 3.  The
	classes are:

		0. Core GL versions, sorted by version number.
		1. ARB extensions, sorted by extension number.
		2. Non-ARB extensions, sorted by extension number.
		3. Un-numbered extensions, sorted by extension name.
	"""

	try:
		core_version = float(name)
	except Exception,e:
		core_version = 0.0

	if core_version > 0.0:
		cat_type = 0
		key = name
	elif name.startswith("GL_ARB_") or name.startswith("GLX_ARB_") or name.startswith("WGL_ARB_"):
		cat_type = 1
		key = int(number)
	else:
		if number != None:
			cat_type = 2
			key = int(number)
		else:
			cat_type = 3
			key = name


	return [cat_type, key]


def create_parameter_string(parameters, include_names):
	"""Create a parameter string from a list of gl_parameters."""

	list = []
	for p in parameters:
		if p.is_padding:
			continue

		if include_names:
			list.append( p.string() )
		else:
			list.append( p.type_string() )

	if len(list) == 0: list = ["void"]

	return string.join(list, ", ")


class gl_item:
	def __init__(self, element, context):
		self.context = context
		self.name = element.nsProp( "name", None )
		self.category = real_category_name( element.parent.nsProp( "name", None ) )
		return


class gl_type( gl_item ):
	def __init__(self, element, context):
		gl_item.__init__(self, element, context)
		self.size = int( element.nsProp( "size", None ), 0 )

		te = typeexpr.type_expression( None )
		tn = typeexpr.type_node()
		tn.size = int( element.nsProp( "size", None ), 0 )
		tn.integer = not is_attr_true( element, "float" )
		tn.unsigned = is_attr_true( element, "unsigned" )
		tn.name = "GL" + self.name
		te.set_base_type_node( tn )

		self.type_expr = te
		return
	

	def get_type_expression(self):
		return self.type_expr


class gl_enum( gl_item ):
	def __init__(self, element, context):
		gl_item.__init__(self, element, context)
		self.value = int( element.nsProp( "value", None ), 0 )

		temp = element.nsProp( "count", None )
		if not temp or temp == "?":
			self.default_count = -1
		else:
			try:
				c = int(temp)
			except Exception,e:
				raise RuntimeError('Invalid count value "%s" for enum "%s" in function "%s" when an integer was expected.' % (temp, self.name, n))

			self.default_count = c

		return


	def priority(self):
		"""Calculate a 'priority' for this enum name.
		
		When an enum is looked up by number, there may be many
		possible names, but only one is the 'prefered' name.  The
		priority is used to select which name is the 'best'.

		Highest precedence is given to core GL name.  ARB extension
		names have the next highest, followed by EXT extension names.
		Vendor extension names are the lowest.
		"""

		if self.name.endswith( "_BIT" ):
			bias = 1
		else:
			bias = 0

		if self.category.startswith( "GL_VERSION_" ):
			priority = 0
		elif self.category.startswith( "GL_ARB_" ):
			priority = 2
		elif self.category.startswith( "GL_EXT_" ):
			priority = 4
		else:
			priority = 6

		return priority + bias



class gl_parameter:
	def __init__(self, element, context):
		self.name = element.nsProp( "name", None )

		ts = element.nsProp( "type", None )
		self.type_expr = typeexpr.type_expression( ts, context )

		temp = element.nsProp( "variable_param", None )
		if temp:
			self.count_parameter_list = temp.split( ' ' )
		else:
			self.count_parameter_list = []

		# The count tag can be either a numeric string or the name of
		# a variable.  If it is the name of a variable, the int(c)
		# statement will throw an exception, and the except block will
		# take over.

		c = element.nsProp( "count", None )
		try: 
			count = int(c)
			self.count = count
			self.counter = None
		except Exception,e:
			count = 1
			self.count = 0
			self.counter = c
		
		self.count_scale = int(element.nsProp( "count_scale", None ))

		elements = (count * self.count_scale)
		if elements == 1:
			elements = 0

		#if ts == "GLdouble":
		#	print '/* stack size -> %s = %u (before)*/' % (self.name, self.type_expr.get_stack_size())
		#	print '/* # elements = %u */' % (elements)
		self.type_expr.set_elements( elements )
		#if ts == "GLdouble":
		#	print '/* stack size -> %s = %u (after) */' % (self.name, self.type_expr.get_stack_size())

		self.is_client_only = is_attr_true( element, 'client_only' )
		self.is_counter     = is_attr_true( element, 'counter' )
		self.is_output      = is_attr_true( element, 'output' )


		# Pixel data has special parameters.

		self.width      = element.nsProp('img_width',  None)
		self.height     = element.nsProp('img_height', None)
		self.depth      = element.nsProp('img_depth',  None)
		self.extent     = element.nsProp('img_extent', None)

		self.img_xoff   = element.nsProp('img_xoff',   None)
		self.img_yoff   = element.nsProp('img_yoff',   None)
		self.img_zoff   = element.nsProp('img_zoff',   None)
		self.img_woff   = element.nsProp('img_woff',   None)

		self.img_format = element.nsProp('img_format', None)
		self.img_type   = element.nsProp('img_type',   None)
		self.img_target = element.nsProp('img_target', None)

		self.img_pad_dimensions = is_attr_true( element, 'img_pad_dimensions' )
		self.img_null_flag      = is_attr_true( element, 'img_null_flag' )
		self.img_send_null      = is_attr_true( element, 'img_send_null' )

		self.is_padding = is_attr_true( element, 'padding' )
		return


	def compatible(self, other):
		return 1


	def is_array(self):
		return self.is_pointer()


	def is_pointer(self):
		return self.type_expr.is_pointer()


	def is_image(self):
		if self.width:
			return 1
		else:
			return 0


	def is_variable_length(self):
		return len(self.count_parameter_list) or self.counter


	def is_64_bit(self):
		count = self.type_expr.get_element_count()
		if count:
			if (self.size() / count) == 8:
				return 1
		else:
			if self.size() == 8:
				return 1

		return 0


	def string(self):
		return self.type_expr.original_string + " " + self.name


	def type_string(self):
		return self.type_expr.original_string


	def get_base_type_string(self):
		return self.type_expr.get_base_name()


	def get_dimensions(self):
		if not self.width:
			return [ 0, "0", "0", "0", "0" ]

		dim = 1
		w = self.width
		h = "1"
		d = "1"
		e = "1"

		if self.height:
			dim = 2
			h = self.height

		if self.depth:
			dim = 3
			d = self.depth

		if self.extent:
			dim = 4
			e = self.extent

		return [ dim, w, h, d, e ]


	def get_stack_size(self):
		return self.type_expr.get_stack_size()


	def size(self):
		if self.is_image():
			return 0
		else:
			return self.type_expr.get_element_size()


	def get_element_count(self):
		c = self.type_expr.get_element_count()
		if c == 0:
			return 1

		return c


	def size_string(self, use_parens = 1):
		s = self.size()
		if self.counter or self.count_parameter_list:
			list = [ "compsize" ]

			if self.counter and self.count_parameter_list:
				list.append( self.counter )
			elif self.counter:
				list = [ self.counter ]

			if s > 1:
				list.append( str(s) )

			if len(list) > 1 and use_parens :
				return "(%s)" % (string.join(list, " * "))
			else:
				return string.join(list, " * ")

		elif self.is_image():
			return "compsize"
		else:
			return str(s)


	def format_string(self):
		if self.type_expr.original_string == "GLenum":
			return "0x%x"
		else:
			return self.type_expr.format_string()



class gl_function( gl_item ):
	def __init__(self, element, context):
		self.context = context
		self.name = None

		self.entry_points = []
		self.return_type = "void"
		self.parameters = []
		self.offset = -1
		self.initialized = 0
		self.images = []

		self.assign_offset = 0

		self.static_entry_points = []

		# Track the parameter string (for the function prototype)
		# for each entry-point.  This is done because some functions
		# change their prototype slightly when promoted from extension
		# to ARB extension to core.  glTexImage3DEXT and glTexImage3D
		# are good examples of this.  Scripts that need to generate
		# code for these differing aliases need to real prototype
		# for each entry-point.  Otherwise, they may generate code
		# that won't compile.

		self.entry_point_parameters = {}

		self.process_element( element )

		return

	
	def process_element(self, element):
		name = element.nsProp( "name", None )
		alias = element.nsProp( "alias", None )

		if is_attr_true(element, "static_dispatch"):
			self.static_entry_points.append(name)

		self.entry_points.append( name )
		if alias:
			true_name = alias
		else:
			true_name = name

			# Only try to set the offset when a non-alias
			# entry-point is being processes.

			offset = element.nsProp( "offset", None )
			if offset:
				try:
					o = int( offset )
					self.offset = o
				except Exception, e:
					self.offset = -1
					if offset == "assign":
						self.assign_offset = 1


		if not self.name:
			self.name = true_name
		elif self.name != true_name:
			raise RuntimeError("Function true name redefined.  Was %s, now %s." % (self.name, true_name))


		# There are two possible cases.  The first time an entry-point
		# with data is seen, self.initialized will be 0.  On that
		# pass, we just fill in the data.  The next time an
		# entry-point with data is seen, self.initialized will be 1.
		# On that pass we have to make that the new values match the
		# valuse from the previous entry-point.

		parameters = []
		return_type = "void"
		child = element.children
		while child:
			if child.type == "element":
				if child.name == "return":
					return_type = child.nsProp( "type", None )
				elif child.name == "param":
					param = self.context.factory.create_item( "parameter", child, self.context)
					parameters.append( param )

			child = child.next


		if self.initialized:
			if self.return_type != return_type:
				raise RuntimeError( "Return type changed in %s.  Was %s, now %s." % (name, self.return_type, return_type))

			if len(parameters) != len(self.parameters):
				raise RuntimeError( "Parameter count mismatch in %s.  Was %d, now %d." % (name, len(self.parameters), len(parameters)))

			for j in range(0, len(parameters)):
				p1 = parameters[j]
				p2 = self.parameters[j]
				if not p1.compatible( p2 ):
					raise RuntimeError( 'Parameter type mismatch in %s.  "%s" was "%s", now "%s".' % (name, p2.name, p2.type_expr.original_string, p1.type_expr.original_string))


		if true_name == name or not self.initialized:
			self.return_type = return_type
			self.parameters = parameters

			for param in self.parameters:
				if param.is_image():
					self.images.append( param )

		if element.children:
			self.initialized = 1
			self.entry_point_parameters[name] = parameters
		else:
			self.entry_point_parameters[name] = []

		return

	def filter_entry_points(self, entry_point_list):
		"""Filter out entry points not in entry_point_list."""
		if not self.initialized:
			raise RuntimeError('%s is not initialized yet' % self.name)

		entry_points = []
		for ent in self.entry_points:
			if ent not in entry_point_list:
				if ent in self.static_entry_points:
					self.static_entry_points.remove(ent)
				self.entry_point_parameters.pop(ent)
			else:
				entry_points.append(ent)

		if not entry_points:
			raise RuntimeError('%s has no entry point after filtering' % self.name)

		self.entry_points = entry_points
		if self.name not in entry_points:
			# use the first remaining entry point
			self.name = entry_points[0]
			self.parameters = self.entry_point_parameters[entry_points[0]]

	def get_images(self):
		"""Return potentially empty list of input images."""
		return self.images


	def parameterIterator(self):
		return self.parameters.__iter__();


	def get_parameter_string(self, entrypoint = None):
		if entrypoint:
			params = self.entry_point_parameters[ entrypoint ]
		else:
			params = self.parameters
		
		return create_parameter_string( params, 1 )

	def get_called_parameter_string(self):
		p_string = ""
		comma = ""

		for p in self.parameterIterator():
			p_string = p_string + comma + p.name
			comma = ", "

		return p_string


	def is_abi(self):
		return (self.offset >= 0 and not self.assign_offset)

	def is_static_entry_point(self, name):
		return name in self.static_entry_points

	def dispatch_name(self):
		if self.name in self.static_entry_points:
			return self.name
		else:
			return "_dispatch_stub_%u" % (self.offset)

	def static_name(self, name):
		if name in self.static_entry_points:
			return name
		else:
			return "_dispatch_stub_%u" % (self.offset)


class gl_item_factory:
	"""Factory to create objects derived from gl_item."""

	def create_item(self, item_name, element, context):
		if item_name == "function":
			return gl_function(element, context)
		if item_name == "type":
			return gl_type(element, context)
		elif item_name == "enum":
			return gl_enum(element, context)
		elif item_name == "parameter":
			return gl_parameter(element, context)
		elif item_name == "api":
			return gl_api(self)
		else:
			return None


class gl_api:
	def __init__(self, factory):
		self.functions_by_name = {}
		self.enums_by_name = {}
		self.types_by_name = {}

		self.category_dict = {}
		self.categories = [{}, {}, {}, {}]

		self.factory = factory

		self.next_offset = 0

		typeexpr.create_initial_types()
		return

	def filter_functions(self, entry_point_list):
		"""Filter out entry points not in entry_point_list."""
		functions_by_name = {}
		for func in self.functions_by_name.itervalues():
			entry_points = [ent for ent in func.entry_points if ent in entry_point_list]
			if entry_points:
				func.filter_entry_points(entry_points)
				functions_by_name[func.name] = func

		self.functions_by_name = functions_by_name

	def process_element(self, doc):
		element = doc.children
		while element.type != "element" or element.name != "OpenGLAPI":
			element = element.next

		if element:
			self.process_OpenGLAPI(element)
		return


	def process_OpenGLAPI(self, element):
		child = element.children
		while child:
			if child.type == "element":
				if child.name == "category":
					self.process_category( child )
				elif child.name == "OpenGLAPI":
					self.process_OpenGLAPI( child )

			child = child.next

		return


	def process_category(self, cat):
		cat_name = cat.nsProp( "name", None )
		cat_number = cat.nsProp( "number", None )

		[cat_type, key] = classify_category(cat_name, cat_number)
		self.categories[cat_type][key] = [cat_name, cat_number]

		child = cat.children
		while child:
			if child.type == "element":
				if child.name == "function":
					func_name = real_function_name( child )

					temp_name = child.nsProp( "name", None )
					self.category_dict[ temp_name ] = [cat_name, cat_number]

					if self.functions_by_name.has_key( func_name ):
						func = self.functions_by_name[ func_name ]
						func.process_element( child )
					else:
						func = self.factory.create_item( "function", child, self )
						self.functions_by_name[ func_name ] = func

					if func.offset >= self.next_offset:
						self.next_offset = func.offset + 1


				elif child.name == "enum":
					enum = self.factory.create_item( "enum", child, self )
					self.enums_by_name[ enum.name ] = enum
				elif child.name == "type":
					t = self.factory.create_item( "type", child, self )
					self.types_by_name[ "GL" + t.name ] = t


			child = child.next

		return


	def functionIterateByCategory(self, cat = None):
		"""Iterate over functions by category.
		
		If cat is None, all known functions are iterated in category
		order.  See classify_category for details of the ordering.
		Within a category, functions are sorted by name.  If cat is
		not None, then only functions in that category are iterated.
		"""
		lists = [{}, {}, {}, {}]

		for func in self.functionIterateAll():
			[cat_name, cat_number] = self.category_dict[func.name]

			if (cat == None) or (cat == cat_name):
				[func_cat_type, key] = classify_category(cat_name, cat_number)

				if not lists[func_cat_type].has_key(key):
					lists[func_cat_type][key] = {}

				lists[func_cat_type][key][func.name] = func


		functions = []
		for func_cat_type in range(0,4):
			keys = lists[func_cat_type].keys()
			keys.sort()

			for key in keys:
				names = lists[func_cat_type][key].keys()
				names.sort()

				for name in names:
					functions.append(lists[func_cat_type][key][name])

		return functions.__iter__()


	def functionIterateByOffset(self):
		max_offset = -1
		for func in self.functions_by_name.itervalues():
			if func.offset > max_offset:
				max_offset = func.offset


		temp = [None for i in range(0, max_offset + 1)]
		for func in self.functions_by_name.itervalues():
			if func.offset != -1:
				temp[ func.offset ] = func


		list = []
		for i in range(0, max_offset + 1):
			if temp[i]:
				list.append(temp[i])

		return list.__iter__();


	def functionIterateAll(self):
		return self.functions_by_name.itervalues()


	def enumIterateByName(self):
		keys = self.enums_by_name.keys()
		keys.sort()
		
		list = []
		for enum in keys:
			list.append( self.enums_by_name[ enum ] )

		return list.__iter__()


	def categoryIterate(self):
		"""Iterate over categories.
		
		Iterate over all known categories in the order specified by
		classify_category.  Each iterated value is a tuple of the
		name and number (which may be None) of the category.
		"""

		list = []
		for cat_type in range(0,4):
			keys = self.categories[cat_type].keys()
			keys.sort()
			
			for key in keys:
				list.append(self.categories[cat_type][key])
				
		return list.__iter__()


	def get_category_for_name( self, name ):
		if self.category_dict.has_key(name):
			return self.category_dict[name]
		else:
			return ["<unknown category>", None]


	def typeIterate(self):
		return self.types_by_name.itervalues()


	def find_type( self, type_name ):
		if type_name in self.types_by_name:
			return self.types_by_name[ type_name ].type_expr
		else:
			print "Unable to find base type matching \"%s\"." % (type_name)
			return None
