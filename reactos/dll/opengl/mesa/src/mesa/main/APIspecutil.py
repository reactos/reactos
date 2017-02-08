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
Minimal apiutil.py interface for use by es_generator.py.
"""

import sys
import libxml2

import APIspec

__spec = {}
__functions = {}
__aliases = {}

def _ParseXML(filename, apiname):
    conversions = {
        # from           to
        'GLfloat':  [ 'GLdouble' ],
        'GLclampf': [ 'GLclampd' ],
        'GLubyte':  [ 'GLfloat', 'GLdouble' ],
        'GLint':    [ 'GLfloat', 'GLdouble' ],
        'GLfixed':  [ 'GLfloat', 'GLdouble' ],
        'GLclampx': [ 'GLclampf', 'GLclampd' ],
    }

    doc = libxml2.readFile(filename, None,
            libxml2.XML_PARSE_DTDLOAD +
            libxml2.XML_PARSE_DTDVALID +
            libxml2.XML_PARSE_NOBLANKS)
    spec = APIspec.Spec(doc)
    impl = spec.get_impl()
    api = spec.get_api(apiname)
    doc.freeDoc()

    __spec["impl"] = impl
    __spec["api"] = api

    for func in api.functions:
        alias, need_conv = impl.match(func, conversions)
        if not alias:
            # external functions are manually dispatched
            if not func.is_external:
                print >>sys.stderr, "Error: unable to dispatch %s" % func.name
            alias = func
            need_conv = False

        __functions[func.name] = func
        __aliases[func.name] = (alias, need_conv)


def AllSpecials(notused=None):
    """Return a list of all external functions in the API."""
    api = __spec["api"]

    specials = []
    for func in api.functions:
        if func.is_external:
            specials.append(func.name)

    return specials


def GetAllFunctions(filename, api):
    """Return sorted list of all functions in the API."""
    if not __spec:
        _ParseXML(filename, api)

    api = __spec["api"]
    names = []
    for func in api.functions:
        names.append(func.name)
    names.sort()
    return names


def ReturnType(funcname):
    """Return the C return type of named function."""
    func = __functions[funcname]
    return func.return_type


def Properties(funcname):
    """Return list of properties of the named GL function."""
    func = __functions[funcname]
    return [func.direction]


def _ValidValues(func, param):
    """Return the valid values of a parameter."""
    valid_values = []
    switch = func.checker.switches.get(param.name, [])
    for desc in switch:
        # no dependent vector
        if not desc.checker.switches:
            for val in desc.values:
                valid_values.append((val, None, None, [], desc.error, None))
            continue

        items = desc.checker.switches.items()
        if len(items) > 1:
            print >>sys.stderr, "%s: more than one parameter depend on %s" % \
                    (func.name, desc.name)
        dep_name, dep_switch = items[0]

        for dep_desc in dep_switch:
            if dep_desc.index >= 0 and dep_desc.index != 0:
                print >>sys.stderr, "%s: not first element of a vector" % func.name
            if dep_desc.checker.switches:
                print >>sys.stderr, "%s: deep nested dependence" % func.name

            convert = None if dep_desc.convert else "noconvert"
            for val in desc.values:
                valid_values.append((val, dep_desc.size_str, dep_desc.name,
                                     dep_desc.values, dep_desc.error, convert))
    return valid_values


def _Conversion(func, src_param):
    """Return the destination type of the conversion, or None."""
    alias, need_conv = __aliases[func.name]
    if need_conv:
        dst_param = alias.get_param(src_param.name)
        if src_param.type == dst_param.type:
            need_conv = False
    if not need_conv:
        return (None, "none")

    converts = { True: 0, False: 0 }

    # In Fogx, for example,  pname may be GL_FOG_DENSITY/GL_FOG_START/GL_FOG_END
    # or GL_FOG_MODE.  In the former three cases, param is not checked and the
    # default is to convert.
    if not func.checker.always_check(src_param.name):
        converts[True] += 1

    for desc in func.checker.flatten(src_param.name):
        converts[desc.convert] += 1
        if converts[True] and converts[False]:
            break

    # it should be "never", "sometimes", and "always"...
    if converts[False]:
        if converts[True]:
            conversion = "some"
        else:
            conversion = "none"
    else:
        conversion = "all"

    return (dst_param.base_type(), conversion)


def _MaxVecSize(func, param):
    """Return the largest possible size of a vector."""
    if not param.is_vector:
        return 0
    if param.size:
        return param.size

    # need to look at all descriptions
    size = 0
    for desc in func.checker.flatten(param.name):
        if desc.size_str and desc.size_str.isdigit():
            s = int(desc.size_str)
            if s > size:
                size = s
    if not size:
        need_conv = __aliases[func.name][1]
        if need_conv:
            print >>sys.stderr, \
                    "Error: unable to dicide the max size of %s in %s" % \
                    (param.name, func.name)
    return size


def _ParameterTuple(func, param):
    """Return a parameter tuple.

    [0] -- parameter name
    [1] -- parameter type
    [2] -- max vector size or 0
    [3] -- dest type the parameter converts to, or None
    [4] -- valid values
    [5] -- how often does the conversion happen

    """
    vec_size = _MaxVecSize(func, param)
    dst_type, conversion = _Conversion(func, param)
    valid_values = _ValidValues(func, param)

    return (param.name, param.type, vec_size, dst_type, valid_values, conversion)


def Parameters(funcname):
    """Return list of tuples of function parameters."""
    func = __functions[funcname]
    params = []
    for param in func.params:
        params.append(_ParameterTuple(func, param))

    return params


def FunctionPrefix(funcname):
    """Return function specific prefix."""
    func = __functions[funcname]

    return func.prefix


def FindParamIndex(params, paramname):
    """Find the index of a named parameter."""
    for i in xrange(len(params)):
        if params[i][0] == paramname:
            return i
    return None


def MakeDeclarationString(params):
    """Return a C-style parameter declaration string."""
    string = []
    for p in params:
        sep = "" if p[1].endswith("*") else " "
        string.append("%s%s%s" % (p[1], sep, p[0]))
    if not string:
        return "void"
    return ", ".join(string)


def AliasPrefix(funcname):
    """Return the prefix of the function the named function is an alias of."""
    alias = __aliases[funcname][0]
    return alias.prefix


def Alias(funcname):
    """Return the name of the function the named function is an alias of."""
    alias, need_conv = __aliases[funcname]
    return alias.name if not need_conv else None


def ConversionFunction(funcname):
    """Return the name of the function the named function converts to."""
    alias, need_conv = __aliases[funcname]
    return alias.name if need_conv else None


def Categories(funcname):
    """Return all the categories of the named GL function."""
    api = __spec["api"]
    return [api.name]
