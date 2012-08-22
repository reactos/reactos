#!/usr/bin/python
#
# Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
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
"""
A parser for APIspec.
"""

class SpecError(Exception):
    """Error in the spec file."""


class Spec(object):
    """A Spec is an abstraction of the API spec."""

    def __init__(self, doc):
        self.doc = doc

        self.spec_node = doc.getRootElement()
        self.tmpl_nodes = {}
        self.api_nodes = {}
        self.impl_node = None

        # parse <apispec>
        node = self.spec_node.children
        while node:
            if node.type == "element":
                if node.name == "template":
                    self.tmpl_nodes[node.prop("name")] = node
                elif node.name == "api":
                    self.api_nodes[node.prop("name")] = node
                else:
                    raise SpecError("unexpected node %s in apispec" %
                            node.name)
            node = node.next

        # find an implementation
        for name, node in self.api_nodes.iteritems():
            if node.prop("implementation") == "true":
                self.impl_node = node
                break
        if not self.impl_node:
            raise SpecError("unable to find an implementation")

    def get_impl(self):
        """Return the implementation."""
        return API(self, self.impl_node)

    def get_api(self, name):
        """Return an API."""
        return API(self, self.api_nodes[name])


class API(object):
    """An API consists of categories and functions."""

    def __init__(self, spec, api_node):
        self.name = api_node.prop("name")
        self.is_impl = (api_node.prop("implementation") == "true")

        self.categories = []
        self.functions = []

        # parse <api>
        func_nodes = []
        node = api_node.children
        while node:
            if node.type == "element":
                if node.name == "category":
                    cat = node.prop("name")
                    self.categories.append(cat)
                elif node.name == "function":
                    func_nodes.append(node)
                else:
                    raise SpecError("unexpected node %s in api" % node.name)
            node = node.next

        # realize functions
        for func_node in func_nodes:
            tmpl_node = spec.tmpl_nodes[func_node.prop("template")]
            try:
                func = Function(tmpl_node, func_node, self.is_impl,
                                self.categories)
            except SpecError, e:
                func_name = func_node.prop("name")
                raise SpecError("failed to parse %s: %s" % (func_name, e))
            self.functions.append(func)

    def match(self, func, conversions={}):
        """Find a matching function in the API."""
        match = None
        need_conv = False
        for f in self.functions:
            matched, conv = f.match(func, conversions)
            if matched:
                match = f
                need_conv = conv
                # exact match
                if not need_conv:
                    break
        return (match, need_conv)


class Function(object):
    """Parse and realize a <template> node."""

    def __init__(self, tmpl_node, func_node, force_skip_desc=False, categories=[]):
        self.tmpl_name = tmpl_node.prop("name")
        self.direction = tmpl_node.prop("direction")

        self.name = func_node.prop("name")
        self.prefix = func_node.prop("default_prefix")
        self.is_external = (func_node.prop("external") == "true")

        if force_skip_desc:
            self._skip_desc = True
        else:
            self._skip_desc = (func_node.prop("skip_desc") == "true")

        self._categories = categories

        # these attributes decide how the template is realized
        self._gltype = func_node.prop("gltype")
        if func_node.hasProp("vector_size"):
            self._vector_size = int(func_node.prop("vector_size"))
        else:
            self._vector_size = 0
        self._expand_vector = (func_node.prop("expand_vector") == "true")

        self.return_type = "void"
        param_nodes = []

        # find <proto>
        proto_node = tmpl_node.children
        while proto_node:
            if proto_node.type == "element" and proto_node.name == "proto":
                break
            proto_node = proto_node.next
        if not proto_node:
            raise SpecError("no proto")
        # and parse it
        node = proto_node.children
        while node:
            if node.type == "element":
                if node.name == "return":
                    self.return_type = node.prop("type")
                elif node.name == "param" or node.name == "vector":
                    if self.support_node(node):
                        # make sure the node is not hidden
                        if not (self._expand_vector and
                                (node.prop("hide_if_expanded") == "true")):
                            param_nodes.append(node)
                else:
                    raise SpecError("unexpected node %s in proto" % node.name)
            node = node.next

        self._init_params(param_nodes)
        self._init_descs(tmpl_node, param_nodes)

    def __str__(self):
        return "%s %s%s(%s)" % (self.return_type, self.prefix, self.name,
                self.param_string(True))

    def _init_params(self, param_nodes):
        """Parse and initialize parameters."""
        self.params = []

        for param_node in param_nodes:
            size = self.param_node_size(param_node)
            # when no expansion, vector is just like param
            if param_node.name == "param" or not self._expand_vector:
                param = Parameter(param_node, self._gltype, size)
                self.params.append(param)
                continue

            if not size or size > param_node.lsCountNode():
                raise SpecError("could not expand %s with unknown or "
                                "mismatch sizes" % param.name)

            # expand the vector
            expanded_params = []
            child = param_node.children
            while child:
                if (child.type == "element" and child.name == "param" and
                    self.support_node(child)):
                    expanded_params.append(Parameter(child, self._gltype))
                    if len(expanded_params) == size:
                        break
                child = child.next
            # just in case that lsCountNode counts unknown nodes
            if len(expanded_params) < size:
                raise SpecError("not enough named parameters")

            self.params.extend(expanded_params)

    def _init_descs(self, tmpl_node, param_nodes):
        """Parse and initialize parameter descriptions."""
        self.checker = Checker()
        if self._skip_desc:
            return

        node = tmpl_node.children
        while node:
            if node.type == "element" and node.name == "desc":
                if self.support_node(node):
                    # parse <desc>
                    desc = Description(node, self._categories)
                    self.checker.add_desc(desc)
            node = node.next

        self.checker.validate(self, param_nodes)

    def support_node(self, node):
        """Return true if a node is in the supported category."""
        return (not node.hasProp("category") or
                node.prop("category") in self._categories)

    def get_param(self, name):
        """Return the named parameter."""
        for param in self.params:
            if param.name == name:
                return param
        return None

    def param_node_size(self, param):
        """Return the size of a vector."""
        if param.name != "vector":
            return 0

        size = param.prop("size")
        if size.isdigit():
            size = int(size)
        else:
            size = 0
        if not size:
            size = self._vector_size
            if not size and self._expand_vector:
                # return the number of named parameters
                size = param.lsCountNode()
        return size

    def param_string(self, declaration):
        """Return the C code of the parameters."""
        args = []
        if declaration:
            for param in self.params:
                sep = "" if param.type.endswith("*") else " "
                args.append("%s%s%s" % (param.type, sep, param.name))
            if not args:
                args.append("void")
        else:
            for param in self.params:
                args.append(param.name)
        return ", ".join(args)

    def match(self, other, conversions={}):
        """Return true if the functions match, probably with a conversion."""
        if (self.tmpl_name != other.tmpl_name or
            self.return_type != other.return_type or
            len(self.params) != len(other.params)):
            return (False, False)

        need_conv = False
        for i in xrange(len(self.params)):
            src = other.params[i]
            dst = self.params[i]
            if (src.is_vector != dst.is_vector or src.size != dst.size):
                return (False, False)
            if src.type != dst.type:
                if dst.base_type() in conversions.get(src.base_type(), []):
                    need_conv = True
                else:
                    # unable to convert
                    return (False, False)

        return (True, need_conv)


class Parameter(object):
    """A parameter of a function."""

    def __init__(self, param_node, gltype=None, size=0):
        self.is_vector = (param_node.name == "vector")

        self.name = param_node.prop("name")
        self.size = size

        type = param_node.prop("type")
        if gltype:
            type = type.replace("GLtype", gltype)
        elif type.find("GLtype") != -1:
            raise SpecError("parameter %s has unresolved type" % self.name)

        self.type = type

    def base_type(self):
        """Return the base GL type by stripping qualifiers."""
        return [t for t in self.type.split(" ") if t.startswith("GL")][0]


class Checker(object):
    """A checker is the collection of all descriptions on the same level.
    Descriptions of the same parameter are concatenated.
    """

    def __init__(self):
        self.switches = {}
        self.switch_constants = {}

    def add_desc(self, desc):
        """Add a description."""
        # TODO allow index to vary
        const_attrs = ["index", "error", "convert", "size_str"]
        if desc.name not in self.switches:
            self.switches[desc.name] = []
            self.switch_constants[desc.name] = {}
            for attr in const_attrs:
                self.switch_constants[desc.name][attr] = None

        # some attributes, like error code, should be the same for all descs
        consts = self.switch_constants[desc.name]
        for attr in const_attrs:
            if getattr(desc, attr) is not None:
                if (consts[attr] is not None and
                    consts[attr] != getattr(desc, attr)):
                    raise SpecError("mismatch %s for %s" % (attr, desc.name))
                consts[attr] = getattr(desc, attr)

        self.switches[desc.name].append(desc)

    def validate(self, func, param_nodes):
        """Validate the checker against a function."""
        tmp = Checker()

        for switch in self.switches.itervalues():
            valid_descs = []
            for desc in switch:
                if desc.validate(func, param_nodes):
                    valid_descs.append(desc)
            # no possible values
            if not valid_descs:
                return False
            for desc in valid_descs:
                if not desc._is_noop:
                    tmp.add_desc(desc)

        self.switches = tmp.switches
        self.switch_constants = tmp.switch_constants
        return True

    def flatten(self, name=None):
        """Return a flat list of all descriptions of the named parameter."""
        flat_list = []
        for switch in self.switches.itervalues():
            for desc in switch:
                if not name or desc.name == name:
                    flat_list.append(desc)
                flat_list.extend(desc.checker.flatten(name))
        return flat_list

    def always_check(self, name):
        """Return true if the parameter is checked in all possible pathes."""
        if name in self.switches:
            return True

        # a param is always checked if any of the switch always checks it
        for switch in self.switches.itervalues():
            # a switch always checks it if all of the descs always check it
            always = True
            for desc in switch:
                if not desc.checker.always_check(name):
                    always = False
                    break
            if always:
                return True
        return False

    def _c_switch(self, name, indent="\t"):
        """Output C switch-statement for the named parameter, for debug."""
        switch = self.switches.get(name, [])
        # make sure there are valid values
        need_switch = False
        for desc in switch:
            if desc.values:
                need_switch = True
        if not need_switch:
            return []

        stmts = []
        var = switch[0].name
        if switch[0].index >= 0:
            var += "[%d]" % switch[0].index
        stmts.append("switch (%s) { /* assume GLenum */" % var)

        for desc in switch:
            if desc.values:
                for val in desc.values:
                    stmts.append("case %s:" % val)
                for dep_name in desc.checker.switches.iterkeys():
                    dep_stmts = [indent + s for s in desc.checker._c_switch(dep_name, indent)]
                    stmts.extend(dep_stmts)
                stmts.append(indent + "break;")

        stmts.append("default:")
        stmts.append(indent + "ON_ERROR(%s);" % switch[0].error);
        stmts.append(indent + "break;")
        stmts.append("}")

        return stmts

    def dump(self, indent="\t"):
        """Dump the descriptions in C code."""
        stmts = []
        for name in self.switches.iterkeys():
            c_switch = self._c_switch(name)
            print "\n".join(c_switch)


class Description(object):
    """A description desribes a parameter and its relationship with other
    parameters.
    """

    def __init__(self, desc_node, categories=[]):
        self._categories = categories
        self._is_noop = False

        self.name = desc_node.prop("name")
        self.index = -1

        self.error = desc_node.prop("error") or "GL_INVALID_ENUM"
        # vector_size may be C code
        self.size_str = desc_node.prop("vector_size")

        self._has_enum = False
        self.values = []
        dep_nodes = []

        # parse <desc>
        valid_names = ["value", "range", "desc"]
        node = desc_node.children
        while node:
            if node.type == "element":
                if node.name in valid_names:
                    # ignore nodes that require unsupported categories
                    if (node.prop("category") and
                        node.prop("category") not in self._categories):
                        node = node.next
                        continue
                else:
                    raise SpecError("unexpected node %s in desc" % node.name)

                if node.name == "value":
                    val = node.prop("name")
                    if not self._has_enum and val.startswith("GL_"):
                        self._has_enum = True
                    self.values.append(val)
                elif node.name == "range":
                    first = int(node.prop("from"))
                    last = int(node.prop("to"))
                    base = node.prop("base") or ""
                    if not self._has_enum and base.startswith("GL_"):
                        self._has_enum = True
                    # expand range
                    for i in xrange(first, last + 1):
                        self.values.append("%s%d" % (base, i))
                else: # dependent desc
                    dep_nodes.append(node)
            node = node.next

        # default to convert if there is no enum
        self.convert = not self._has_enum
        if desc_node.hasProp("convert"):
            self.convert = (desc_node.prop("convert") == "true")

        self._init_deps(dep_nodes)

    def _init_deps(self, dep_nodes):
        """Parse and initialize dependents."""
        self.checker = Checker()

        for dep_node in dep_nodes:
            # recursion!
            dep = Description(dep_node, self._categories)
            self.checker.add_desc(dep)

    def _search_param_node(self, param_nodes, name=None):
        """Search the template parameters for the named node."""
        param_node = None
        param_index = -1

        if not name:
            name = self.name
        for node in param_nodes:
            if name == node.prop("name"):
                param_node = node
            elif node.name == "vector":
                child = node.children
                idx = 0
                while child:
                    if child.type == "element" and child.name == "param":
                        if name == child.prop("name"):
                            param_node = node
                            param_index = idx
                            break
                        idx += 1
                    child = child.next
            if param_node:
                break
        return (param_node, param_index)

    def _find_final(self, func, param_nodes):
        """Find the final parameter."""
        param = func.get_param(self.name)
        param_index = -1

        # the described param is not in the final function
        if not param:
            # search the template parameters
            node, index = self._search_param_node(param_nodes)
            if not node:
                raise SpecError("invalid desc %s in %s" %
                        (self.name, func.name))

            # a named parameter of a vector
            if index >= 0:
                param = func.get_param(node.prop("name"))
                param_index = index
            elif node.name == "vector":
                # must be an expanded vector, check its size
                if self.size_str and self.size_str.isdigit():
                    size = int(self.size_str)
                    expanded_size = func.param_node_size(node)
                    if size != expanded_size:
                        return (False, None, -1)
            # otherwise, it is a valid, but no-op, description

        return (True, param, param_index)

    def validate(self, func, param_nodes):
        """Validate a description against certain function."""
        if self.checker.switches and not self.values:
            raise SpecError("no valid values for %s" % self.name)

        valid, param, param_index = self._find_final(func, param_nodes)
        if not valid:
            return False

        # the description is valid, but the param is gone
        # mark it no-op so that it will be skipped
        if not param:
            self._is_noop = True
            return True

        if param.is_vector:
            # if param was known, this should have been done in __init__
            if self._has_enum:
                self.size_str = "1"
            # size mismatch
            if (param.size and self.size_str and self.size_str.isdigit() and
                param.size != int(self.size_str)):
                return False
        elif self.size_str:
            # only vector accepts vector_size
            raise SpecError("vector_size is invalid for %s" % param.name)

        if not self.checker.validate(func, param_nodes):
            return False

        # update the description
        self.name = param.name
        self.index = param_index

        return True


def main():
    import libxml2

    filename = "APIspec.xml"
    apinames = ["GLES1.1", "GLES2.0"]

    doc = libxml2.readFile(filename, None,
            libxml2.XML_PARSE_DTDLOAD +
            libxml2.XML_PARSE_DTDVALID +
            libxml2.XML_PARSE_NOBLANKS)

    spec = Spec(doc)
    impl = spec.get_impl()
    for apiname in apinames:
        spec.get_api(apiname)

    doc.freeDoc()

    print "%s is successfully parsed" % filename


if __name__ == "__main__":
    main()
