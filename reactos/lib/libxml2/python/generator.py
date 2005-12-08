#!/usr/bin/python -u
#
# generate python wrappers from the XML API description
#

functions = {}
enums = {} # { enumType: { enumConstant: enumValue } }

import os
import sys
import string

if __name__ == "__main__":
    # launched as a script
    srcPref = os.path.dirname(sys.argv[0])
else:
    # imported
    srcPref = os.path.dirname(__file__)

#######################################################################
#
#  That part if purely the API acquisition phase from the
#  XML API description
#
#######################################################################
import os
import xmllib
try:
    import sgmlop
except ImportError:
    sgmlop = None # accelerator not available

debug = 0

if sgmlop:
    class FastParser:
        """sgmlop based XML parser.  this is typically 15x faster
           than SlowParser..."""

        def __init__(self, target):

            # setup callbacks
            self.finish_starttag = target.start
            self.finish_endtag = target.end
            self.handle_data = target.data

            # activate parser
            self.parser = sgmlop.XMLParser()
            self.parser.register(self)
            self.feed = self.parser.feed
            self.entity = {
                "amp": "&", "gt": ">", "lt": "<",
                "apos": "'", "quot": '"'
                }

        def close(self):
            try:
                self.parser.close()
            finally:
                self.parser = self.feed = None # nuke circular reference

        def handle_entityref(self, entity):
            # <string> entity
            try:
                self.handle_data(self.entity[entity])
            except KeyError:
                self.handle_data("&%s;" % entity)

else:
    FastParser = None


class SlowParser(xmllib.XMLParser):
    """slow but safe standard parser, based on the XML parser in
       Python's standard library."""

    def __init__(self, target):
        self.unknown_starttag = target.start
        self.handle_data = target.data
        self.unknown_endtag = target.end
        xmllib.XMLParser.__init__(self)

def getparser(target = None):
    # get the fastest available parser, and attach it to an
    # unmarshalling object.  return both objects.
    if target is None:
        target = docParser()
    if FastParser:
        return FastParser(target), target
    return SlowParser(target), target

class docParser:
    def __init__(self):
        self._methodname = None
        self._data = []
        self.in_function = 0

    def close(self):
        if debug:
            print "close"

    def getmethodname(self):
        return self._methodname

    def data(self, text):
        if debug:
            print "data %s" % text
        self._data.append(text)

    def start(self, tag, attrs):
        if debug:
            print "start %s, %s" % (tag, attrs)
        if tag == 'function':
            self._data = []
            self.in_function = 1
            self.function = None
            self.function_cond = None
            self.function_args = []
            self.function_descr = None
            self.function_return = None
            self.function_file = None
            if attrs.has_key('name'):
                self.function = attrs['name']
            if attrs.has_key('file'):
                self.function_file = attrs['file']
        elif tag == 'cond':
            self._data = []
        elif tag == 'info':
            self._data = []
        elif tag == 'arg':
            if self.in_function == 1:
                self.function_arg_name = None
                self.function_arg_type = None
                self.function_arg_info = None
                if attrs.has_key('name'):
                    self.function_arg_name = attrs['name']
                if attrs.has_key('type'):
                    self.function_arg_type = attrs['type']
                if attrs.has_key('info'):
                    self.function_arg_info = attrs['info']
        elif tag == 'return':
            if self.in_function == 1:
                self.function_return_type = None
                self.function_return_info = None
                self.function_return_field = None
                if attrs.has_key('type'):
                    self.function_return_type = attrs['type']
                if attrs.has_key('info'):
                    self.function_return_info = attrs['info']
                if attrs.has_key('field'):
                    self.function_return_field = attrs['field']
        elif tag == 'enum':
            enum(attrs['type'],attrs['name'],attrs['value'])

    def end(self, tag):
        if debug:
            print "end %s" % tag
        if tag == 'function':
            if self.function != None:
                function(self.function, self.function_descr,
                         self.function_return, self.function_args,
                         self.function_file, self.function_cond)
                self.in_function = 0
        elif tag == 'arg':
            if self.in_function == 1:
                self.function_args.append([self.function_arg_name,
                                           self.function_arg_type,
                                           self.function_arg_info])
        elif tag == 'return':
            if self.in_function == 1:
                self.function_return = [self.function_return_type,
                                        self.function_return_info,
                                        self.function_return_field]
        elif tag == 'info':
            str = ''
            for c in self._data:
                str = str + c
            if self.in_function == 1:
                self.function_descr = str
        elif tag == 'cond':
            str = ''
            for c in self._data:
                str = str + c
            if self.in_function == 1:
                self.function_cond = str
                
                
def function(name, desc, ret, args, file, cond):
    functions[name] = (desc, ret, args, file, cond)

def enum(type, name, value):
    if not enums.has_key(type):
        enums[type] = {}
    enums[type][name] = value

#######################################################################
#
#  Some filtering rukes to drop functions/types which should not
#  be exposed as-is on the Python interface
#
#######################################################################

skipped_modules = {
    'xmlmemory': None,
    'DOCBparser': None,
    'SAX': None,
    'hash': None,
    'list': None,
    'threads': None,
#    'xpointer': None,
}
skipped_types = {
    'int *': "usually a return type",
    'xmlSAXHandlerPtr': "not the proper interface for SAX",
    'htmlSAXHandlerPtr': "not the proper interface for SAX",
    'xmlRMutexPtr': "thread specific, skipped",
    'xmlMutexPtr': "thread specific, skipped",
    'xmlGlobalStatePtr': "thread specific, skipped",
    'xmlListPtr': "internal representation not suitable for python",
    'xmlBufferPtr': "internal representation not suitable for python",
    'FILE *': None,
}

#######################################################################
#
#  Table of remapping to/from the python type or class to the C
#  counterpart.
#
#######################################################################

py_types = {
    'void': (None, None, None, None),
    'int':  ('i', None, "int", "int"),
    'long':  ('i', None, "int", "int"),
    'double':  ('d', None, "double", "double"),
    'unsigned int':  ('i', None, "int", "int"),
    'xmlChar':  ('c', None, "int", "int"),
    'unsigned char *':  ('z', None, "charPtr", "char *"),
    'char *':  ('z', None, "charPtr", "char *"),
    'const char *':  ('z', None, "charPtrConst", "const char *"),
    'xmlChar *':  ('z', None, "xmlCharPtr", "xmlChar *"),
    'const xmlChar *':  ('z', None, "xmlCharPtrConst", "const xmlChar *"),
    'xmlNodePtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const xmlNodePtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'xmlNode *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const xmlNode *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'xmlDtdPtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const xmlDtdPtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'xmlDtd *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const xmlDtd *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'xmlAttrPtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const xmlAttrPtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'xmlAttr *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const xmlAttr *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'xmlEntityPtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const xmlEntityPtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'xmlEntity *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const xmlEntity *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'xmlElementPtr':  ('O', "xmlElement", "xmlElementPtr", "xmlElementPtr"),
    'const xmlElementPtr':  ('O', "xmlElement", "xmlElementPtr", "xmlElementPtr"),
    'xmlElement *':  ('O', "xmlElement", "xmlElementPtr", "xmlElementPtr"),
    'const xmlElement *':  ('O', "xmlElement", "xmlElementPtr", "xmlElementPtr"),
    'xmlAttributePtr':  ('O', "xmlAttribute", "xmlAttributePtr", "xmlAttributePtr"),
    'const xmlAttributePtr':  ('O', "xmlAttribute", "xmlAttributePtr", "xmlAttributePtr"),
    'xmlAttribute *':  ('O', "xmlAttribute", "xmlAttributePtr", "xmlAttributePtr"),
    'const xmlAttribute *':  ('O', "xmlAttribute", "xmlAttributePtr", "xmlAttributePtr"),
    'xmlNsPtr':  ('O', "xmlNode", "xmlNsPtr", "xmlNsPtr"),
    'const xmlNsPtr':  ('O', "xmlNode", "xmlNsPtr", "xmlNsPtr"),
    'xmlNs *':  ('O', "xmlNode", "xmlNsPtr", "xmlNsPtr"),
    'const xmlNs *':  ('O', "xmlNode", "xmlNsPtr", "xmlNsPtr"),
    'xmlDocPtr':  ('O', "xmlNode", "xmlDocPtr", "xmlDocPtr"),
    'const xmlDocPtr':  ('O', "xmlNode", "xmlDocPtr", "xmlDocPtr"),
    'xmlDoc *':  ('O', "xmlNode", "xmlDocPtr", "xmlDocPtr"),
    'const xmlDoc *':  ('O', "xmlNode", "xmlDocPtr", "xmlDocPtr"),
    'htmlDocPtr':  ('O', "xmlNode", "xmlDocPtr", "xmlDocPtr"),
    'const htmlDocPtr':  ('O', "xmlNode", "xmlDocPtr", "xmlDocPtr"),
    'htmlDoc *':  ('O', "xmlNode", "xmlDocPtr", "xmlDocPtr"),
    'const htmlDoc *':  ('O', "xmlNode", "xmlDocPtr", "xmlDocPtr"),
    'htmlNodePtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const htmlNodePtr':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'htmlNode *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'const htmlNode *':  ('O', "xmlNode", "xmlNodePtr", "xmlNodePtr"),
    'xmlXPathContextPtr':  ('O', "xmlXPathContext", "xmlXPathContextPtr", "xmlXPathContextPtr"),
    'xmlXPathContext *':  ('O', "xpathContext", "xmlXPathContextPtr", "xmlXPathContextPtr"),
    'xmlXPathParserContextPtr':  ('O', "xmlXPathParserContext", "xmlXPathParserContextPtr", "xmlXPathParserContextPtr"),
    'xmlParserCtxtPtr': ('O', "parserCtxt", "xmlParserCtxtPtr", "xmlParserCtxtPtr"),
    'xmlParserCtxt *': ('O', "parserCtxt", "xmlParserCtxtPtr", "xmlParserCtxtPtr"),
    'htmlParserCtxtPtr': ('O', "parserCtxt", "xmlParserCtxtPtr", "xmlParserCtxtPtr"),
    'htmlParserCtxt *': ('O', "parserCtxt", "xmlParserCtxtPtr", "xmlParserCtxtPtr"),
    'xmlValidCtxtPtr': ('O', "ValidCtxt", "xmlValidCtxtPtr", "xmlValidCtxtPtr"),
    'xmlCatalogPtr': ('O', "catalog", "xmlCatalogPtr", "xmlCatalogPtr"),
    'FILE *': ('O', "File", "FILEPtr", "FILE *"),
    'xmlURIPtr': ('O', "URI", "xmlURIPtr", "xmlURIPtr"),
    'xmlErrorPtr': ('O', "Error", "xmlErrorPtr", "xmlErrorPtr"),
    'xmlOutputBufferPtr': ('O', "outputBuffer", "xmlOutputBufferPtr", "xmlOutputBufferPtr"),
    'xmlParserInputBufferPtr': ('O', "inputBuffer", "xmlParserInputBufferPtr", "xmlParserInputBufferPtr"),
    'xmlRegexpPtr': ('O', "xmlReg", "xmlRegexpPtr", "xmlRegexpPtr"),
    'xmlTextReaderLocatorPtr': ('O', "xmlTextReaderLocator", "xmlTextReaderLocatorPtr", "xmlTextReaderLocatorPtr"),
    'xmlTextReaderPtr': ('O', "xmlTextReader", "xmlTextReaderPtr", "xmlTextReaderPtr"),
    'xmlRelaxNGPtr': ('O', "relaxNgSchema", "xmlRelaxNGPtr", "xmlRelaxNGPtr"),
    'xmlRelaxNGParserCtxtPtr': ('O', "relaxNgParserCtxt", "xmlRelaxNGParserCtxtPtr", "xmlRelaxNGParserCtxtPtr"),
    'xmlRelaxNGValidCtxtPtr': ('O', "relaxNgValidCtxt", "xmlRelaxNGValidCtxtPtr", "xmlRelaxNGValidCtxtPtr"),
    'xmlSchemaPtr': ('O', "Schema", "xmlSchemaPtr", "xmlSchemaPtr"),
    'xmlSchemaParserCtxtPtr': ('O', "SchemaParserCtxt", "xmlSchemaParserCtxtPtr", "xmlSchemaParserCtxtPtr"),
    'xmlSchemaValidCtxtPtr': ('O', "SchemaValidCtxt", "xmlSchemaValidCtxtPtr", "xmlSchemaValidCtxtPtr"),
}

py_return_types = {
    'xmlXPathObjectPtr':  ('O', "foo", "xmlXPathObjectPtr", "xmlXPathObjectPtr"),
}

unknown_types = {}

foreign_encoding_args = (
    'htmlCreateMemoryParserCtxt',
    'htmlCtxtReadMemory',
    'htmlParseChunk',
    'htmlReadMemory',
    'xmlCreateMemoryParserCtxt',
    'xmlCtxtReadMemory',
    'xmlCtxtResetPush',
    'xmlParseChunk',
    'xmlParseMemory',
    'xmlReadMemory',
    'xmlRecoverMemory',
)

#######################################################################
#
#  This part writes the C <-> Python stubs libxml2-py.[ch] and
#  the table libxml2-export.c to add when registrering the Python module
#
#######################################################################

# Class methods which are written by hand in libxml.c but the Python-level
# code is still automatically generated (so they are not in skip_function()).
skip_impl = (
    'xmlSaveFileTo',
    'xmlSaveFormatFileTo',
)

def skip_function(name):
    if name[0:12] == "xmlXPathWrap":
        return 1
    if name == "xmlFreeParserCtxt":
        return 1
    if name == "xmlCleanupParser":
        return 1
    if name == "xmlFreeTextReader":
        return 1
#    if name[0:11] == "xmlXPathNew":
#        return 1
    # the next function is defined in libxml.c
    if name == "xmlRelaxNGFreeValidCtxt":
        return 1
    if name == "xmlFreeValidCtxt":
        return 1
    if name == "xmlSchemaFreeValidCtxt":
        return 1

#
# Those are skipped because the Const version is used of the bindings
# instead.
#
    if name == "xmlTextReaderBaseUri":
        return 1
    if name == "xmlTextReaderLocalName":
        return 1
    if name == "xmlTextReaderName":
        return 1
    if name == "xmlTextReaderNamespaceUri":
        return 1
    if name == "xmlTextReaderPrefix":
        return 1
    if name == "xmlTextReaderXmlLang":
        return 1
    if name == "xmlTextReaderValue":
        return 1
    if name == "xmlOutputBufferClose": # handled by by the superclass
        return 1
    if name == "xmlOutputBufferFlush": # handled by by the superclass
        return 1
    if name == "xmlErrMemory":
        return 1

    if name == "xmlValidBuildContentModel":
        return 1
    if name == "xmlValidateElementDecl":
        return 1
    if name == "xmlValidateAttributeDecl":
        return 1

    return 0

def print_function_wrapper(name, output, export, include):
    global py_types
    global unknown_types
    global functions
    global skipped_modules

    try:
        (desc, ret, args, file, cond) = functions[name]
    except:
        print "failed to get function %s infos"
        return

    if skipped_modules.has_key(file):
        return 0
    if skip_function(name) == 1:
        return 0
    if name in skip_impl:
	# Don't delete the function entry in the caller.
	return 1

    c_call = "";
    format=""
    format_args=""
    c_args=""
    c_return=""
    c_convert=""
    num_bufs=0
    for arg in args:
        # This should be correct
        if arg[1][0:6] == "const ":
            arg[1] = arg[1][6:]
        c_args = c_args + "    %s %s;\n" % (arg[1], arg[0])
        if py_types.has_key(arg[1]):
            (f, t, n, c) = py_types[arg[1]]
	    if (f == 'z') and (name in foreign_encoding_args) and (num_bufs == 0):
	        f = 't#'
            if f != None:
                format = format + f
            if t != None:
                format_args = format_args + ", &pyobj_%s" % (arg[0])
                c_args = c_args + "    PyObject *pyobj_%s;\n" % (arg[0])
                c_convert = c_convert + \
                   "    %s = (%s) Py%s_Get(pyobj_%s);\n" % (arg[0],
                   arg[1], t, arg[0]);
            else:
                format_args = format_args + ", &%s" % (arg[0])
	    if f == 't#':
	        format_args = format_args + ", &py_buffsize%d" % num_bufs
	        c_args = c_args + "    int py_buffsize%d;\n" % num_bufs
		num_bufs = num_bufs + 1
            if c_call != "":
                c_call = c_call + ", ";
            c_call = c_call + "%s" % (arg[0])
        else:
            if skipped_types.has_key(arg[1]):
                return 0
            if unknown_types.has_key(arg[1]):
                lst = unknown_types[arg[1]]
                lst.append(name)
            else:
                unknown_types[arg[1]] = [name]
            return -1
    if format != "":
        format = format + ":%s" % (name)

    if ret[0] == 'void':
        if file == "python_accessor":
	    if args[1][1] == "char *" or args[1][1] == "xmlChar *":
		c_call = "\n    if (%s->%s != NULL) xmlFree(%s->%s);\n" % (
		                 args[0][0], args[1][0], args[0][0], args[1][0])
		c_call = c_call + "    %s->%s = (%s)xmlStrdup((const xmlChar *)%s);\n" % (args[0][0],
		                 args[1][0], args[1][1], args[1][0])
	    else:
		c_call = "\n    %s->%s = %s;\n" % (args[0][0], args[1][0],
						   args[1][0])
        else:
            c_call = "\n    %s(%s);\n" % (name, c_call);
        ret_convert = "    Py_INCREF(Py_None);\n    return(Py_None);\n"
    elif py_types.has_key(ret[0]):
        (f, t, n, c) = py_types[ret[0]]
        c_return = "    %s c_retval;\n" % (ret[0])
        if file == "python_accessor" and ret[2] != None:
            c_call = "\n    c_retval = %s->%s;\n" % (args[0][0], ret[2])
        else:
            c_call = "\n    c_retval = %s(%s);\n" % (name, c_call);
        ret_convert = "    py_retval = libxml_%sWrap((%s) c_retval);\n" % (n,c)
        ret_convert = ret_convert + "    return(py_retval);\n"
    elif py_return_types.has_key(ret[0]):
        (f, t, n, c) = py_return_types[ret[0]]
        c_return = "    %s c_retval;\n" % (ret[0])
        c_call = "\n    c_retval = %s(%s);\n" % (name, c_call);
        ret_convert = "    py_retval = libxml_%sWrap((%s) c_retval);\n" % (n,c)
        ret_convert = ret_convert + "    return(py_retval);\n"
    else:
        if skipped_types.has_key(ret[0]):
            return 0
        if unknown_types.has_key(ret[0]):
            lst = unknown_types[ret[0]]
            lst.append(name)
        else:
            unknown_types[ret[0]] = [name]
        return -1

    if cond != None and cond != "":
        include.write("#if %s\n" % cond)
        export.write("#if %s\n" % cond)
        output.write("#if %s\n" % cond)

    include.write("PyObject * ")
    include.write("libxml_%s(PyObject *self, PyObject *args);\n" % (name));

    export.write("    { (char *)\"%s\", libxml_%s, METH_VARARGS, NULL },\n" %
                 (name, name))

    if file == "python":
        # Those have been manually generated
	if cond != None and cond != "":
	    include.write("#endif\n");
	    export.write("#endif\n");
	    output.write("#endif\n");
        return 1
    if file == "python_accessor" and ret[0] != "void" and ret[2] is None:
        # Those have been manually generated
	if cond != None and cond != "":
	    include.write("#endif\n");
	    export.write("#endif\n");
	    output.write("#endif\n");
        return 1

    output.write("PyObject *\n")
    output.write("libxml_%s(PyObject *self ATTRIBUTE_UNUSED," % (name))
    output.write(" PyObject *args")
    if format == "":
	output.write(" ATTRIBUTE_UNUSED")
    output.write(") {\n")
    if ret[0] != 'void':
        output.write("    PyObject *py_retval;\n")
    if c_return != "":
        output.write(c_return)
    if c_args != "":
        output.write(c_args)
    if format != "":
        output.write("\n    if (!PyArg_ParseTuple(args, (char *)\"%s\"%s))\n" %
                     (format, format_args))
        output.write("        return(NULL);\n")
    if c_convert != "":
        output.write(c_convert)
                                                              
    output.write(c_call)
    output.write(ret_convert)
    output.write("}\n\n")
    if cond != None and cond != "":
        include.write("#endif /* %s */\n" % cond)
        export.write("#endif /* %s */\n" % cond)
        output.write("#endif /* %s */\n" % cond)
    return 1

def buildStubs():
    global py_types
    global py_return_types
    global unknown_types

    try:
	f = open(os.path.join(srcPref,"libxml2-api.xml"))
	data = f.read()
	(parser, target)  = getparser()
	parser.feed(data)
	parser.close()
    except IOError, msg:
	try:
	    f = open(os.path.join(srcPref,"..","doc","libxml2-api.xml"))
	    data = f.read()
	    (parser, target)  = getparser()
	    parser.feed(data)
	    parser.close()
	except IOError, msg:
	    print file, ":", msg
	    sys.exit(1)

    n = len(functions.keys())
    print "Found %d functions in libxml2-api.xml" % (n)

    py_types['pythonObject'] = ('O', "pythonObject", "pythonObject", "pythonObject")
    try:
	f = open(os.path.join(srcPref,"libxml2-python-api.xml"))
	data = f.read()
	(parser, target)  = getparser()
	parser.feed(data)
	parser.close()
    except IOError, msg:
	print file, ":", msg


    print "Found %d functions in libxml2-python-api.xml" % (
	  len(functions.keys()) - n)
    nb_wrap = 0
    failed = 0
    skipped = 0

    include = open("libxml2-py.h", "w")
    include.write("/* Generated */\n\n")
    export = open("libxml2-export.c", "w")
    export.write("/* Generated */\n\n")
    wrapper = open("libxml2-py.c", "w")
    wrapper.write("/* Generated */\n\n")
    wrapper.write("#include <Python.h>\n")
    wrapper.write("#include <libxml/xmlversion.h>\n")
    wrapper.write("#include <libxml/tree.h>\n")
    wrapper.write("#include <libxml/xmlschemastypes.h>\n")
    wrapper.write("#include \"libxml_wrap.h\"\n")
    wrapper.write("#include \"libxml2-py.h\"\n\n")
    for function in functions.keys():
	ret = print_function_wrapper(function, wrapper, export, include)
	if ret < 0:
	    failed = failed + 1
	    del functions[function]
	if ret == 0:
	    skipped = skipped + 1
	    del functions[function]
	if ret == 1:
	    nb_wrap = nb_wrap + 1
    include.close()
    export.close()
    wrapper.close()

    print "Generated %d wrapper functions, %d failed, %d skipped\n" % (nb_wrap,
							      failed, skipped);
    print "Missing type converters: "
    for type in unknown_types.keys():
	print "%s:%d " % (type, len(unknown_types[type])),
    print

#######################################################################
#
#  This part writes part of the Python front-end classes based on
#  mapping rules between types and classes and also based on function
#  renaming to get consistent function names at the Python level
#
#######################################################################

#
# The type automatically remapped to generated classes
#
classes_type = {
    "xmlNodePtr": ("._o", "xmlNode(_obj=%s)", "xmlNode"),
    "xmlNode *": ("._o", "xmlNode(_obj=%s)", "xmlNode"),
    "xmlDocPtr": ("._o", "xmlDoc(_obj=%s)", "xmlDoc"),
    "xmlDocPtr *": ("._o", "xmlDoc(_obj=%s)", "xmlDoc"),
    "htmlDocPtr": ("._o", "xmlDoc(_obj=%s)", "xmlDoc"),
    "htmlxmlDocPtr *": ("._o", "xmlDoc(_obj=%s)", "xmlDoc"),
    "xmlAttrPtr": ("._o", "xmlAttr(_obj=%s)", "xmlAttr"),
    "xmlAttr *": ("._o", "xmlAttr(_obj=%s)", "xmlAttr"),
    "xmlNsPtr": ("._o", "xmlNs(_obj=%s)", "xmlNs"),
    "xmlNs *": ("._o", "xmlNs(_obj=%s)", "xmlNs"),
    "xmlDtdPtr": ("._o", "xmlDtd(_obj=%s)", "xmlDtd"),
    "xmlDtd *": ("._o", "xmlDtd(_obj=%s)", "xmlDtd"),
    "xmlEntityPtr": ("._o", "xmlEntity(_obj=%s)", "xmlEntity"),
    "xmlEntity *": ("._o", "xmlEntity(_obj=%s)", "xmlEntity"),
    "xmlElementPtr": ("._o", "xmlElement(_obj=%s)", "xmlElement"),
    "xmlElement *": ("._o", "xmlElement(_obj=%s)", "xmlElement"),
    "xmlAttributePtr": ("._o", "xmlAttribute(_obj=%s)", "xmlAttribute"),
    "xmlAttribute *": ("._o", "xmlAttribute(_obj=%s)", "xmlAttribute"),
    "xmlXPathContextPtr": ("._o", "xpathContext(_obj=%s)", "xpathContext"),
    "xmlXPathContext *": ("._o", "xpathContext(_obj=%s)", "xpathContext"),
    "xmlXPathParserContext *": ("._o", "xpathParserContext(_obj=%s)", "xpathParserContext"),
    "xmlXPathParserContextPtr": ("._o", "xpathParserContext(_obj=%s)", "xpathParserContext"),
    "xmlParserCtxtPtr": ("._o", "parserCtxt(_obj=%s)", "parserCtxt"),
    "xmlParserCtxt *": ("._o", "parserCtxt(_obj=%s)", "parserCtxt"),
    "htmlParserCtxtPtr": ("._o", "parserCtxt(_obj=%s)", "parserCtxt"),
    "htmlParserCtxt *": ("._o", "parserCtxt(_obj=%s)", "parserCtxt"),
    "xmlValidCtxtPtr": ("._o", "ValidCtxt(_obj=%s)", "ValidCtxt"),
    "xmlCatalogPtr": ("._o", "catalog(_obj=%s)", "catalog"),
    "xmlURIPtr": ("._o", "URI(_obj=%s)", "URI"),
    "xmlErrorPtr": ("._o", "Error(_obj=%s)", "Error"),
    "xmlOutputBufferPtr": ("._o", "outputBuffer(_obj=%s)", "outputBuffer"),
    "xmlParserInputBufferPtr": ("._o", "inputBuffer(_obj=%s)", "inputBuffer"),
    "xmlRegexpPtr": ("._o", "xmlReg(_obj=%s)", "xmlReg"),
    "xmlTextReaderLocatorPtr": ("._o", "xmlTextReaderLocator(_obj=%s)", "xmlTextReaderLocator"),
    "xmlTextReaderPtr": ("._o", "xmlTextReader(_obj=%s)", "xmlTextReader"),
    'xmlRelaxNGPtr': ('._o', "relaxNgSchema(_obj=%s)", "relaxNgSchema"),
    'xmlRelaxNGParserCtxtPtr': ('._o', "relaxNgParserCtxt(_obj=%s)", "relaxNgParserCtxt"),
    'xmlRelaxNGValidCtxtPtr': ('._o', "relaxNgValidCtxt(_obj=%s)", "relaxNgValidCtxt"),
    'xmlSchemaPtr': ("._o", "Schema(_obj=%s)", "Schema"),
    'xmlSchemaParserCtxtPtr': ("._o", "SchemaParserCtxt(_obj=%s)", "SchemaParserCtxt"),
    'xmlSchemaValidCtxtPtr': ("._o", "SchemaValidCtxt(_obj=%s)", "SchemaValidCtxt"),
}

converter_type = {
    "xmlXPathObjectPtr": "xpathObjectRet(%s)",
}

primary_classes = ["xmlNode", "xmlDoc"]

classes_ancestor = {
    "xmlNode" : "xmlCore",
    "xmlDtd" : "xmlNode",
    "xmlDoc" : "xmlNode",
    "xmlAttr" : "xmlNode",
    "xmlNs" : "xmlNode",
    "xmlEntity" : "xmlNode",
    "xmlElement" : "xmlNode",
    "xmlAttribute" : "xmlNode",
    "outputBuffer": "ioWriteWrapper",
    "inputBuffer": "ioReadWrapper",
    "parserCtxt": "parserCtxtCore",
    "xmlTextReader": "xmlTextReaderCore",
    "ValidCtxt": "ValidCtxtCore",
    "SchemaValidCtxt": "SchemaValidCtxtCore",
    "relaxNgValidCtxt": "relaxNgValidCtxtCore",
}
classes_destructors = {
    "parserCtxt": "xmlFreeParserCtxt",
    "catalog": "xmlFreeCatalog",
    "URI": "xmlFreeURI",
#    "outputBuffer": "xmlOutputBufferClose",
    "inputBuffer": "xmlFreeParserInputBuffer",
    "xmlReg": "xmlRegFreeRegexp",
    "xmlTextReader": "xmlFreeTextReader",
    "relaxNgSchema": "xmlRelaxNGFree",
    "relaxNgParserCtxt": "xmlRelaxNGFreeParserCtxt",
    "relaxNgValidCtxt": "xmlRelaxNGFreeValidCtxt",
	"Schema": "xmlSchemaFree",
	"SchemaParserCtxt": "xmlSchemaFreeParserCtxt",
	"SchemaValidCtxt": "xmlSchemaFreeValidCtxt",
        "ValidCtxt": "xmlFreeValidCtxt",
}

functions_noexcept = {
    "xmlHasProp": 1,
    "xmlHasNsProp": 1,
    "xmlDocSetRootElement": 1,
    "xmlNodeGetNs": 1,
    "xmlNodeGetNsDefs": 1,
}

reference_keepers = {
    "xmlTextReader": [('inputBuffer', 'input')],
    "relaxNgValidCtxt": [('relaxNgSchema', 'schema')],
	"SchemaValidCtxt": [('Schema', 'schema')],
}

function_classes = {}

function_classes["None"] = []

def nameFixup(name, classe, type, file):
    listname = classe + "List"
    ll = len(listname)
    l = len(classe)
    if name[0:l] == listname:
        func = name[l:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:12] == "xmlParserGet" and file == "python_accessor":
        func = name[12:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:12] == "xmlParserSet" and file == "python_accessor":
        func = name[12:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:10] == "xmlNodeGet" and file == "python_accessor":
        func = name[10:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:9] == "xmlURIGet" and file == "python_accessor":
        func = name[9:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:9] == "xmlURISet" and file == "python_accessor":
        func = name[6:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:11] == "xmlErrorGet" and file == "python_accessor":
        func = name[11:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:17] == "xmlXPathParserGet" and file == "python_accessor":
        func = name[17:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:11] == "xmlXPathGet" and file == "python_accessor":
        func = name[11:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:11] == "xmlXPathSet" and file == "python_accessor":
        func = name[8:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:15] == "xmlOutputBuffer" and file != "python":
        func = name[15:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:20] == "xmlParserInputBuffer" and file != "python":
        func = name[20:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:9] == "xmlRegexp" and file == "xmlregexp":
        func = "regexp" + name[9:]
    elif name[0:6] == "xmlReg" and file == "xmlregexp":
        func = "regexp" + name[6:]
    elif name[0:20] == "xmlTextReaderLocator" and file == "xmlreader":
        func = name[20:]
    elif name[0:18] == "xmlTextReaderConst" and file == "xmlreader":
        func = name[18:]
    elif name[0:13] == "xmlTextReader" and file == "xmlreader":
        func = name[13:]
    elif name[0:12] == "xmlReaderNew" and file == "xmlreader":
        func = name[9:]
    elif name[0:11] == "xmlACatalog":
        func = name[11:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:l] == classe:
        func = name[l:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:7] == "libxml_":
        func = name[7:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:6] == "xmlGet":
        func = name[6:]
        func = string.lower(func[0:1]) + func[1:]
    elif name[0:3] == "xml":
        func = name[3:]
        func = string.lower(func[0:1]) + func[1:]
    else:
        func = name
    if func[0:5] == "xPath":
        func = "xpath" + func[5:]
    elif func[0:4] == "xPtr":
        func = "xpointer" + func[4:]
    elif func[0:8] == "xInclude":
        func = "xinclude" + func[8:]
    elif func[0:2] == "iD":
        func = "ID" + func[2:]
    elif func[0:3] == "uRI":
        func = "URI" + func[3:]
    elif func[0:4] == "uTF8":
        func = "UTF8" + func[4:]
    elif func[0:3] == 'sAX':
        func = "SAX" + func[3:]
    return func


def functionCompare(info1, info2):
    (index1, func1, name1, ret1, args1, file1) = info1
    (index2, func2, name2, ret2, args2, file2) = info2
    if file1 == file2:
        if func1 < func2:
            return -1
        if func1 > func2:
            return 1
    if file1 == "python_accessor":
        return -1
    if file2 == "python_accessor":
        return 1
    if file1 < file2:
        return -1
    if file1 > file2:
        return 1
    return 0

def writeDoc(name, args, indent, output):
     if functions[name][0] is None or functions[name][0] == "":
         return
     val = functions[name][0]
     val = string.replace(val, "NULL", "None");
     output.write(indent)
     output.write('"""')
     while len(val) > 60:
         str = val[0:60]
         i = string.rfind(str, " ");
         if i < 0:
             i = 60
         str = val[0:i]
         val = val[i:]
         output.write(str)
         output.write('\n  ');
         output.write(indent)
     output.write(val);
     output.write(' """\n')

def buildWrappers():
    global ctypes
    global py_types
    global py_return_types
    global unknown_types
    global functions
    global function_classes
    global classes_type
    global classes_list
    global converter_type
    global primary_classes
    global converter_type
    global classes_ancestor
    global converter_type
    global primary_classes
    global classes_ancestor
    global classes_destructors
    global functions_noexcept

    for type in classes_type.keys():
	function_classes[classes_type[type][2]] = []

    #
    # Build the list of C types to look for ordered to start
    # with primary classes
    #
    ctypes = []
    classes_list = []
    ctypes_processed = {}
    classes_processed = {}
    for classe in primary_classes:
	classes_list.append(classe)
	classes_processed[classe] = ()
	for type in classes_type.keys():
	    tinfo = classes_type[type]
	    if tinfo[2] == classe:
		ctypes.append(type)
		ctypes_processed[type] = ()
    for type in classes_type.keys():
	if ctypes_processed.has_key(type):
	    continue
	tinfo = classes_type[type]
	if not classes_processed.has_key(tinfo[2]):
	    classes_list.append(tinfo[2])
	    classes_processed[tinfo[2]] = ()
	    
	ctypes.append(type)
	ctypes_processed[type] = ()

    for name in functions.keys():
	found = 0;
	(desc, ret, args, file, cond) = functions[name]
	for type in ctypes:
	    classe = classes_type[type][2]

	    if name[0:3] == "xml" and len(args) >= 1 and args[0][1] == type:
		found = 1
		func = nameFixup(name, classe, type, file)
		info = (0, func, name, ret, args, file)
		function_classes[classe].append(info)
	    elif name[0:3] == "xml" and len(args) >= 2 and args[1][1] == type \
	        and file != "python_accessor":
		found = 1
		func = nameFixup(name, classe, type, file)
		info = (1, func, name, ret, args, file)
		function_classes[classe].append(info)
	    elif name[0:4] == "html" and len(args) >= 1 and args[0][1] == type:
		found = 1
		func = nameFixup(name, classe, type, file)
		info = (0, func, name, ret, args, file)
		function_classes[classe].append(info)
	    elif name[0:4] == "html" and len(args) >= 2 and args[1][1] == type \
	        and file != "python_accessor":
		found = 1
		func = nameFixup(name, classe, type, file)
		info = (1, func, name, ret, args, file)
		function_classes[classe].append(info)
	if found == 1:
	    continue
	if name[0:8] == "xmlXPath":
	    continue
	if name[0:6] == "xmlStr":
	    continue
	if name[0:10] == "xmlCharStr":
	    continue
	func = nameFixup(name, "None", file, file)
	info = (0, func, name, ret, args, file)
	function_classes['None'].append(info)
   
    classes = open("libxml2class.py", "w")
    txt = open("libxml2class.txt", "w")
    txt.write("          Generated Classes for libxml2-python\n\n")

    txt.write("#\n# Global functions of the module\n#\n\n")
    if function_classes.has_key("None"):
	flist = function_classes["None"]
	flist.sort(functionCompare)
	oldfile = ""
	for info in flist:
	    (index, func, name, ret, args, file) = info
	    if file != oldfile:
		classes.write("#\n# Functions from module %s\n#\n\n" % file)
		txt.write("\n# functions from module %s\n" % file)
		oldfile = file
	    classes.write("def %s(" % func)
	    txt.write("%s()\n" % func);
	    n = 0
	    for arg in args:
		if n != 0:
		    classes.write(", ")
		classes.write("%s" % arg[0])
		n = n + 1
	    classes.write("):\n")
	    writeDoc(name, args, '    ', classes);

	    for arg in args:
		if classes_type.has_key(arg[1]):
		    classes.write("    if %s is None: %s__o = None\n" %
				  (arg[0], arg[0]))
		    classes.write("    else: %s__o = %s%s\n" %
				  (arg[0], arg[0], classes_type[arg[1]][0]))
	    if ret[0] != "void":
		classes.write("    ret = ");
	    else:
		classes.write("    ");
	    classes.write("libxml2mod.%s(" % name)
	    n = 0
	    for arg in args:
		if n != 0:
		    classes.write(", ");
		classes.write("%s" % arg[0])
		if classes_type.has_key(arg[1]):
		    classes.write("__o");
		n = n + 1
	    classes.write(")\n");
	    if ret[0] != "void":
		if classes_type.has_key(ret[0]):
		    #
		    # Raise an exception
		    #
		    if functions_noexcept.has_key(name):
		        classes.write("    if ret is None:return None\n");
		    elif string.find(name, "URI") >= 0:
			classes.write(
			"    if ret is None:raise uriError('%s() failed')\n"
			              % (name))
		    elif string.find(name, "XPath") >= 0:
			classes.write(
			"    if ret is None:raise xpathError('%s() failed')\n"
			              % (name))
		    elif string.find(name, "Parse") >= 0:
			classes.write(
			"    if ret is None:raise parserError('%s() failed')\n"
			              % (name))
		    else:
			classes.write(
			"    if ret is None:raise treeError('%s() failed')\n"
			              % (name))
		    classes.write("    return ");
		    classes.write(classes_type[ret[0]][1] % ("ret"));
		    classes.write("\n");
		else:
		    classes.write("    return ret\n");
	    classes.write("\n");

    txt.write("\n\n#\n# Set of classes of the module\n#\n\n")
    for classname in classes_list:
	if classname == "None":
	    pass
	else:
	    if classes_ancestor.has_key(classname):
		txt.write("\n\nClass %s(%s)\n" % (classname,
			  classes_ancestor[classname]))
		classes.write("class %s(%s):\n" % (classname,
			      classes_ancestor[classname]))
		classes.write("    def __init__(self, _obj=None):\n")
		if classes_ancestor[classname] == "xmlCore" or \
		   classes_ancestor[classname] == "xmlNode":
		    classes.write("        if type(_obj).__name__ != ")
		    classes.write("'PyCObject':\n")
		    classes.write("            raise TypeError, ")
		    classes.write("'%s needs a PyCObject argument'\n" % \
		                classname)
		if reference_keepers.has_key(classname):
		    rlist = reference_keepers[classname]
		    for ref in rlist:
		        classes.write("        self.%s = None\n" % ref[1])
		classes.write("        self._o = _obj\n")
		classes.write("        %s.__init__(self, _obj=_obj)\n\n" % (
			      classes_ancestor[classname]))
		if classes_ancestor[classname] == "xmlCore" or \
		   classes_ancestor[classname] == "xmlNode":
		    classes.write("    def __repr__(self):\n")
		    format = "<%s (%%s) object at 0x%%x>" % (classname)
		    classes.write("        return \"%s\" %% (self.name, id (self))\n\n" % (
				  format))
	    else:
		txt.write("Class %s()\n" % (classname))
		classes.write("class %s:\n" % (classname))
		classes.write("    def __init__(self, _obj=None):\n")
		if reference_keepers.has_key(classname):
		    list = reference_keepers[classname]
		    for ref in list:
		        classes.write("        self.%s = None\n" % ref[1])
		classes.write("        if _obj != None:self._o = _obj;return\n")
		classes.write("        self._o = None\n\n");
	    destruct=None
	    if classes_destructors.has_key(classname):
		classes.write("    def __del__(self):\n")
		classes.write("        if self._o != None:\n")
		classes.write("            libxml2mod.%s(self._o)\n" %
			      classes_destructors[classname]);
		classes.write("        self._o = None\n\n");
		destruct=classes_destructors[classname]
	    flist = function_classes[classname]
	    flist.sort(functionCompare)
	    oldfile = ""
	    for info in flist:
		(index, func, name, ret, args, file) = info
		#
		# Do not provide as method the destructors for the class
		# to avoid double free
		#
		if name == destruct:
		    continue;
		if file != oldfile:
		    if file == "python_accessor":
			classes.write("    # accessors for %s\n" % (classname))
			txt.write("    # accessors\n")
		    else:
			classes.write("    #\n")
			classes.write("    # %s functions from module %s\n" % (
				      classname, file))
			txt.write("\n    # functions from module %s\n" % file)
			classes.write("    #\n\n")
		oldfile = file
		classes.write("    def %s(self" % func)
		txt.write("    %s()\n" % func);
		n = 0
		for arg in args:
		    if n != index:
			classes.write(", %s" % arg[0])
		    n = n + 1
		classes.write("):\n")
		writeDoc(name, args, '        ', classes);
		n = 0
		for arg in args:
		    if classes_type.has_key(arg[1]):
			if n != index:
			    classes.write("        if %s is None: %s__o = None\n" %
					  (arg[0], arg[0]))
			    classes.write("        else: %s__o = %s%s\n" %
					  (arg[0], arg[0], classes_type[arg[1]][0]))
		    n = n + 1
		if ret[0] != "void":
		    classes.write("        ret = ");
		else:
		    classes.write("        ");
		classes.write("libxml2mod.%s(" % name)
		n = 0
		for arg in args:
		    if n != 0:
			classes.write(", ");
		    if n != index:
			classes.write("%s" % arg[0])
			if classes_type.has_key(arg[1]):
			    classes.write("__o");
		    else:
			classes.write("self");
			if classes_type.has_key(arg[1]):
			    classes.write(classes_type[arg[1]][0])
		    n = n + 1
		classes.write(")\n");
		if ret[0] != "void":
		    if classes_type.has_key(ret[0]):
			#
			# Raise an exception
			#
			if functions_noexcept.has_key(name):
			    classes.write(
			        "        if ret is None:return None\n");
			elif string.find(name, "URI") >= 0:
			    classes.write(
		    "        if ret is None:raise uriError('%s() failed')\n"
					  % (name))
			elif string.find(name, "XPath") >= 0:
			    classes.write(
		    "        if ret is None:raise xpathError('%s() failed')\n"
					  % (name))
			elif string.find(name, "Parse") >= 0:
			    classes.write(
		    "        if ret is None:raise parserError('%s() failed')\n"
					  % (name))
			else:
			    classes.write(
		    "        if ret is None:raise treeError('%s() failed')\n"
					  % (name))

			#
			# generate the returned class wrapper for the object
			#
			classes.write("        __tmp = ");
			classes.write(classes_type[ret[0]][1] % ("ret"));
			classes.write("\n");

                        #
			# Sometime one need to keep references of the source
			# class in the returned class object.
			# See reference_keepers for the list
			#
			tclass = classes_type[ret[0]][2]
			if reference_keepers.has_key(tclass):
			    list = reference_keepers[tclass]
			    for pref in list:
				if pref[0] == classname:
				    classes.write("        __tmp.%s = self\n" %
						  pref[1])
			#
			# return the class
			#
			classes.write("        return __tmp\n");
		    elif converter_type.has_key(ret[0]):
			#
			# Raise an exception
			#
			if functions_noexcept.has_key(name):
			    classes.write(
			        "        if ret is None:return None");
			elif string.find(name, "URI") >= 0:
			    classes.write(
		    "        if ret is None:raise uriError('%s() failed')\n"
					  % (name))
			elif string.find(name, "XPath") >= 0:
			    classes.write(
		    "        if ret is None:raise xpathError('%s() failed')\n"
					  % (name))
			elif string.find(name, "Parse") >= 0:
			    classes.write(
		    "        if ret is None:raise parserError('%s() failed')\n"
					  % (name))
			else:
			    classes.write(
		    "        if ret is None:raise treeError('%s() failed')\n"
					  % (name))
			classes.write("        return ");
			classes.write(converter_type[ret[0]] % ("ret"));
			classes.write("\n");
		    else:
			classes.write("        return ret\n");
		classes.write("\n");

    #
    # Generate enum constants
    #
    for type,enum in enums.items():
        classes.write("# %s\n" % type)
        items = enum.items()
        items.sort(lambda i1,i2: cmp(long(i1[1]),long(i2[1])))
        for name,value in items:
            classes.write("%s = %s\n" % (name,value))
        classes.write("\n");

    txt.close()
    classes.close()

buildStubs()
buildWrappers()
