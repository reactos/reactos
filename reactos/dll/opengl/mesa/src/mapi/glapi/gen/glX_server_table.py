#!/bin/env python

# (C) Copyright IBM Corporation 2005, 2006
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


def log2(value):
	for i in range(0, 30):
		p = 1 << i
		if p >= value:
			return i

	return -1


def round_down_to_power_of_two(n):
	"""Returns the nearest power-of-two less than or equal to n."""

	for i in range(30, 0, -1):
		p = 1 << i
		if p <= n:
			return p

	return -1


class function_table:
	def __init__(self, name, do_size_check):
		self.name_base = name
		self.do_size_check = do_size_check


		self.max_bits = 1
		self.next_opcode_threshold = (1 << self.max_bits)
		self.max_opcode = 0

		self.functions = {}
		self.lookup_table = []
		
		# Minimum number of opcodes in a leaf node.
		self.min_op_bits = 3
		self.min_op_count = (1 << self.min_op_bits)
		return


	def append(self, opcode, func):
		self.functions[opcode] = func

		if opcode > self.max_opcode:
			self.max_opcode = opcode

			if opcode > self.next_opcode_threshold:
				bits = log2(opcode)
				if (1 << bits) <= opcode:
					bits += 1

				self.max_bits = bits
				self.next_opcode_threshold = 1 << bits
		return


	def divide_group(self, min_opcode, total):
		"""Divide the group starting min_opcode into subgroups.
		Returns a tuple containing the number of bits consumed by
		the node, the list of the children's tuple, and the number
		of entries in the final array used by this node and its
		children, and the depth of the subtree rooted at the node."""

		remaining_bits = self.max_bits - total
		next_opcode = min_opcode + (1 << remaining_bits)
		empty_children = 0
		
		for M in range(0, remaining_bits):
			op_count = 1 << (remaining_bits - M);
			child_count = 1 << M;

			empty_children = 0
			full_children = 0
			for i in range(min_opcode, next_opcode, op_count):
				used = 0
				empty = 0

				for j in range(i, i + op_count):
					if self.functions.has_key(j):
						used += 1;
					else:
						empty += 1;
						

				if empty == op_count:
					empty_children += 1

				if used == op_count:
					full_children += 1

			if (empty_children > 0) or (full_children == child_count) or (op_count <= self.min_op_count):
				break


		# If all the remaining bits are used by this node, as is the
		# case when M is 0 or remaining_bits, the node is a leaf.

		if (M == 0) or (M == remaining_bits):
			return [remaining_bits, [], 0, 0]
		else:
			children = []
			count = 1
			depth = 1
			all_children_are_nonempty_leaf_nodes = 1
			for i in range(min_opcode, next_opcode, op_count):
				n = self.divide_group(i, total + M)

				if not (n[1] == [] and not self.is_empty_leaf(i, n[0])):
					all_children_are_nonempty_leaf_nodes = 0

				children.append(n)
				count += n[2] + 1
				
				if n[3] >= depth:
					depth = n[3] + 1

			# If all of the child nodes are non-empty leaf nodes, pull
			# them up and make this node a leaf.

			if all_children_are_nonempty_leaf_nodes:
				return [remaining_bits, [], 0, 0]
			else:
				return [M, children, count, depth]


	def is_empty_leaf(self, base_opcode, M):
		for op in range(base_opcode, base_opcode + (1 << M)):
			if self.functions.has_key(op):
				return 0
				break

		return 1


	def dump_tree(self, node, base_opcode, remaining_bits, base_entry, depth):
		M = node[0]
		children = node[1]
		child_M = remaining_bits - M


		# This actually an error condition.
		if children == []:
			return

		print '    /* [%u] -> opcode range [%u, %u], node depth %u */' % (base_entry, base_opcode, base_opcode + (1 << remaining_bits), depth)
		print '    %u,' % (M)

		base_entry += (1 << M) + 1

		child_index = base_entry
		child_base_opcode = base_opcode
		for child in children:
			if child[1] == []:
				if self.is_empty_leaf(child_base_opcode, child_M):
					print '    EMPTY_LEAF,'
				else:
					# Emit the index of the next dispatch
					# function.  Then add all the
					# dispatch functions for this leaf
					# node to the dispatch function
					# lookup table.

					print '    LEAF(%u),' % (len(self.lookup_table))

					for op in range(child_base_opcode, child_base_opcode + (1 << child_M)):
						if self.functions.has_key(op):
							func = self.functions[op]
							size = func.command_fixed_length()

							if func.glx_rop != 0:
								size += 4

							size = ((size + 3) & ~3)

							if func.has_variable_size_request():
								size_name = "__glX%sReqSize" % (func.name)
							else:
								size_name = ""

							if func.glx_vendorpriv == op:
								func_name = func.glx_vendorpriv_names[0]
							else:
								func_name = func.name

							temp = [op, "__glXDisp_%s" % (func_name), "__glXDispSwap_%s" % (func_name), size, size_name]
						else:
							temp = [op, "NULL", "NULL", 0, ""]

						self.lookup_table.append(temp)
			else:
				print '    %u,' % (child_index)
				child_index += child[2]

			child_base_opcode += 1 << child_M

		print ''

		child_index = base_entry
		for child in children:
			if child[1] != []:
				self.dump_tree(child, base_opcode, remaining_bits - M, child_index, depth + 1)
				child_index += child[2]

			base_opcode += 1 << (remaining_bits - M)


	def Print(self):
		# Each dispatch table consists of two data structures.
		#
		# The first structure is an N-way tree where the opcode for
		# the function is the key.  Each node switches on a range of
		# bits from the opcode.  M bits are extracted from the opcde
		# and are used as an index to select one of the N, where
		# N = 2^M, children.
		#
		# The tree is stored as a flat array.  The first value is the
		# number of bits, M, used by the node.  For inner nodes, the
		# following 2^M values are indexes into the array for the
		# child nodes.  For leaf nodes, the followign 2^M values are
		# indexes into the second data structure.
		#
		# If an inner node's child index is 0, the child is an empty
		# leaf node.  That is, none of the opcodes selectable from
		# that child exist.  Since most of the possible opcode space
		# is unused, this allows compact data storage.
		#
		# The second data structure is an array of pairs of function
		# pointers.  Each function contains a pointer to a protocol
		# decode function and a pointer to a byte-swapped protocol
		# decode function.  Elements in this array are selected by the
		# leaf nodes of the first data structure.
		#
		# As the tree is traversed, an accumulator is kept.  This
		# accumulator counts the bits of the opcode consumed by the
		# traversal.  When accumulator + M = B, where B is the
		# maximum number of bits in an opcode, the traversal has
		# reached a leaf node.  The traversal starts with the most
		# significant bits and works down to the least significant
		# bits.
		#
		# Creation of the tree is the most complicated part.  At
		# each node the elements are divided into groups of 2^M
		# elements.  The value of M selected is the smallest possible
		# value where all of the groups are either empty or full, or
		# the groups are a preset minimum size.  If all the children
		# of a node are non-empty leaf nodes, the children are merged
		# to create a single leaf node that replaces the parent.

		tree = self.divide_group(0, 0)

		print '/*****************************************************************/'
		print '/* tree depth = %u */' % (tree[3])
		print 'static const int_fast16_t %s_dispatch_tree[%u] = {' % (self.name_base, tree[2])
		self.dump_tree(tree, 0, self.max_bits, 0, 1)
		print '};\n'
		
		# After dumping the tree, dump the function lookup table.
		
		print 'static const void *%s_function_table[%u][2] = {' % (self.name_base, len(self.lookup_table))
		index = 0
		for func in self.lookup_table:
			opcode = func[0]
			name = func[1]
			name_swap = func[2]
			
			print '    /* [% 3u] = %5u */ {%s, %s},' % (index, opcode, name, name_swap)
			
			index += 1

		print '};\n'
		
		if self.do_size_check:
			var_table = []

			print 'static const int_fast16_t %s_size_table[%u][2] = {' % (self.name_base, len(self.lookup_table))
			index = 0
			var_table = []
			for func in self.lookup_table:
				opcode = func[0]
				fixed = func[3]
				var = func[4]
				
				if var != "":
					var_offset = "%2u" % (len(var_table))
					var_table.append(var)
				else:
					var_offset = "~0"

				print '    /* [%3u] = %5u */ {%3u, %s},' % (index, opcode, fixed, var_offset)
				index += 1

				
			print '};\n'


			print 'static const gl_proto_size_func %s_size_func_table[%u] = {' % (self.name_base, len(var_table))
			for func in var_table:
				print '   %s,' % (func)
 
			print '};\n'


		print 'const struct __glXDispatchInfo %s_dispatch_info = {' % (self.name_base)
		print '    %u,' % (self.max_bits)
		print '    %s_dispatch_tree,' % (self.name_base)
		print '    %s_function_table,' % (self.name_base)
		if self.do_size_check:
			print '    %s_size_table,' % (self.name_base)
			print '    %s_size_func_table' % (self.name_base)
		else:
			print '    NULL,'
			print '    NULL'
		print '};\n'
		return


class PrintGlxDispatchTables(glX_proto_common.glx_print_proto):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)
		self.name = "glX_server_table.py (from Mesa)"
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2005, 2006", "IBM")

		self.rop_functions = function_table("Render", 1)
		self.sop_functions = function_table("Single", 0)
		self.vop_functions = function_table("VendorPriv", 0)
		return


	def printRealHeader(self):
		print '#include <inttypes.h>'
		print '#include "glxserver.h"'
		print '#include "glxext.h"'
		print '#include "indirect_dispatch.h"'
		print '#include "indirect_reqsize.h"'
		print '#include "indirect_table.h"'
		print ''
		return


	def printBody(self, api):
		for f in api.functionIterateAll():
			if not f.ignore and f.vectorequiv == None:
				if f.glx_rop != 0:
					self.rop_functions.append(f.glx_rop, f)
				if f.glx_sop != 0:
					self.sop_functions.append(f.glx_sop, f)
				if f.glx_vendorpriv != 0:
					self.vop_functions.append(f.glx_vendorpriv, f)

		self.sop_functions.Print()
		self.rop_functions.Print()
		self.vop_functions.Print()
		return


if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m")
	except Exception,e:
		show_usage()

	mode = "table_c"
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-m":
			mode = val

	if mode == "table_c":
		printer = PrintGlxDispatchTables()
	else:
		show_usage()


	api = gl_XML.parse_GL_API( file_name, glX_XML.glx_item_factory() )


	printer.Print( api )
