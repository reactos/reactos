import libxml2mod
import types

# The root of all libxml2 errors.
class libxmlError(Exception): pass

#
# Errors raised by the wrappers when some tree handling failed.
#
class treeError(libxmlError):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class parserError(libxmlError):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class uriError(libxmlError):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class xpathError(libxmlError):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class ioWrapper:
    def __init__(self, _obj):
        self.__io = _obj
        self._o = None

    def io_close(self):
        if self.__io == None:
            return(-1)
        self.__io.close()
        self.__io = None
        return(0)

    def io_flush(self):
        if self.__io == None:
            return(-1)
        self.__io.flush()
        return(0)

    def io_read(self, len = -1):
        if self.__io == None:
            return(-1)
        if len < 0:
            return(self.__io.read())
        return(self.__io.read(len))

    def io_write(self, str, len = -1):
        if self.__io == None:
            return(-1)
        if len < 0:
            return(self.__io.write(str))
        return(self.__io.write(str, len))

class ioReadWrapper(ioWrapper):
    def __init__(self, _obj, enc = ""):
        ioWrapper.__init__(self, _obj)
        self._o = libxml2mod.xmlCreateInputBuffer(self, enc)

    def __del__(self):
        print "__del__"
        self.io_close()
        if self._o != None:
            libxml2mod.xmlFreeParserInputBuffer(self._o)
        self._o = None

    def close(self):
        self.io_close()
        if self._o != None:
            libxml2mod.xmlFreeParserInputBuffer(self._o)
        self._o = None

class ioWriteWrapper(ioWrapper):
    def __init__(self, _obj, enc = ""):
#        print "ioWriteWrapper.__init__", _obj
        if type(_obj) == type(''):
            print "write io from a string"
            self.o = None
        elif type(_obj) == types.InstanceType:
            print "write io from instance of %s" % (_obj.__class__)
            ioWrapper.__init__(self, _obj)
            self._o = libxml2mod.xmlCreateOutputBuffer(self, enc)
        else:
            file = libxml2mod.outputBufferGetPythonFile(_obj)
            if file != None:
                ioWrapper.__init__(self, file)
            else:
                ioWrapper.__init__(self, _obj)
            self._o = _obj

    def __del__(self):
#        print "__del__"
        self.io_close()
        if self._o != None:
            libxml2mod.xmlOutputBufferClose(self._o)
        self._o = None

    def flush(self):
        self.io_flush()
        if self._o != None:
            libxml2mod.xmlOutputBufferClose(self._o)
        self._o = None

    def close(self):
        self.io_flush()
        if self._o != None:
            libxml2mod.xmlOutputBufferClose(self._o)
        self._o = None

#
# Example of a class to handle SAX events
#
class SAXCallback:
    """Base class for SAX handlers"""
    def startDocument(self):
        """called at the start of the document"""
        pass

    def endDocument(self):
        """called at the end of the document"""
        pass

    def startElement(self, tag, attrs):
        """called at the start of every element, tag is the name of
           the element, attrs is a dictionary of the element's attributes"""
        pass

    def endElement(self, tag):
        """called at the start of every element, tag is the name of
           the element"""
        pass

    def characters(self, data):
        """called when character data have been read, data is the string
           containing the data, multiple consecutive characters() callback
           are possible."""
        pass

    def cdataBlock(self, data):
        """called when CDATA section have been read, data is the string
           containing the data, multiple consecutive cdataBlock() callback
           are possible."""
        pass

    def reference(self, name):
        """called when an entity reference has been found"""
        pass

    def ignorableWhitespace(self, data):
        """called when potentially ignorable white spaces have been found"""
        pass

    def processingInstruction(self, target, data):
        """called when a PI has been found, target contains the PI name and
           data is the associated data in the PI"""
        pass

    def comment(self, content):
        """called when a comment has been found, content contains the comment"""
        pass

    def externalSubset(self, name, externalID, systemID):
        """called when a DOCTYPE declaration has been found, name is the
           DTD name and externalID, systemID are the DTD public and system
           identifier for that DTd if available"""
        pass

    def internalSubset(self, name, externalID, systemID):
        """called when a DOCTYPE declaration has been found, name is the
           DTD name and externalID, systemID are the DTD public and system
           identifier for that DTD if available"""
        pass

    def entityDecl(self, name, type, externalID, systemID, content):
        """called when an ENTITY declaration has been found, name is the
           entity name and externalID, systemID are the entity public and
           system identifier for that entity if available, type indicates
           the entity type, and content reports it's string content"""
        pass

    def notationDecl(self, name, externalID, systemID):
        """called when an NOTATION declaration has been found, name is the
           notation name and externalID, systemID are the notation public and
           system identifier for that notation if available"""
        pass

    def attributeDecl(self, elem, name, type, defi, defaultValue, nameList):
        """called when an ATTRIBUTE definition has been found"""
        pass

    def elementDecl(self, name, type, content):
        """called when an ELEMENT definition has been found"""
        pass

    def entityDecl(self, name, publicId, systemID, notationName):
        """called when an unparsed ENTITY declaration has been found,
           name is the entity name and publicId,, systemID are the entity
           public and system identifier for that entity if available,
           and notationName indicate the associated NOTATION"""
        pass

    def warning(self, msg):
        print msg

    def error(self, msg):
        raise parserError(msg)

    def fatalError(self, msg):
        raise parserError(msg)

#
# This class is the ancestor of all the Node classes. It provides
# the basic functionalities shared by all nodes (and handle
# gracefylly the exception), like name, navigation in the tree,
# doc reference, content access and serializing to a string or URI
#
class xmlCore:
    def __init__(self, _obj=None):
        if _obj != None: 
            self._o = _obj;
            return
        self._o = None
    def __str__(self):
        return self.serialize()
    def get_parent(self):
        ret = libxml2mod.parent(self._o)
        if ret == None:
            return None
        return xmlNode(_obj=ret)
    def get_children(self):
        ret = libxml2mod.children(self._o)
        if ret == None:
            return None
        return xmlNode(_obj=ret)
    def get_last(self):
        ret = libxml2mod.last(self._o)
        if ret == None:
            return None
        return xmlNode(_obj=ret)
    def get_next(self):
        ret = libxml2mod.next(self._o)
        if ret == None:
            return None
        return xmlNode(_obj=ret)
    def get_properties(self):
        ret = libxml2mod.properties(self._o)
        if ret == None:
            return None
        return xmlAttr(_obj=ret)
    def get_prev(self):
        ret = libxml2mod.prev(self._o)
        if ret == None:
            return None
        return xmlNode(_obj=ret)
    def get_content(self):
        return libxml2mod.xmlNodeGetContent(self._o)
    getContent = get_content  # why is this duplicate naming needed ?
    def get_name(self):
        return libxml2mod.name(self._o)
    def get_type(self):
        return libxml2mod.type(self._o)
    def get_doc(self):
        ret = libxml2mod.doc(self._o)
        if ret == None:
            if self.type in ["document_xml", "document_html"]:
                return xmlDoc(_obj=self._o)
            else:
                return None
        return xmlDoc(_obj=ret)
    #
    # Those are common attributes to nearly all type of nodes
    # defined as python2 properties
    # 
    import sys
    if float(sys.version[0:3]) < 2.2:
        def __getattr__(self, attr):
            if attr == "parent":
                ret = libxml2mod.parent(self._o)
                if ret == None:
                    return None
                return xmlNode(_obj=ret)
            elif attr == "properties":
                ret = libxml2mod.properties(self._o)
                if ret == None:
                    return None
                return xmlAttr(_obj=ret)
            elif attr == "children":
                ret = libxml2mod.children(self._o)
                if ret == None:
                    return None
                return xmlNode(_obj=ret)
            elif attr == "last":
                ret = libxml2mod.last(self._o)
                if ret == None:
                    return None
                return xmlNode(_obj=ret)
            elif attr == "next":
                ret = libxml2mod.next(self._o)
                if ret == None:
                    return None
                return xmlNode(_obj=ret)
            elif attr == "prev":
                ret = libxml2mod.prev(self._o)
                if ret == None:
                    return None
                return xmlNode(_obj=ret)
            elif attr == "content":
                return libxml2mod.xmlNodeGetContent(self._o)
            elif attr == "name":
                return libxml2mod.name(self._o)
            elif attr == "type":
                return libxml2mod.type(self._o)
            elif attr == "doc":
                ret = libxml2mod.doc(self._o)
                if ret == None:
                    if self.type == "document_xml" or self.type == "document_html":
                        return xmlDoc(_obj=self._o)
                    else:
                        return None
                return xmlDoc(_obj=ret)
            raise AttributeError,attr
    else:
        parent = property(get_parent, None, None, "Parent node")
        children = property(get_children, None, None, "First child node")
        last = property(get_last, None, None, "Last sibling node")
        next = property(get_next, None, None, "Next sibling node")
        prev = property(get_prev, None, None, "Previous sibling node")
        properties = property(get_properties, None, None, "List of properies")
        content = property(get_content, None, None, "Content of this node")
        name = property(get_name, None, None, "Node name")
        type = property(get_type, None, None, "Node type")
        doc = property(get_doc, None, None, "The document this node belongs to")

    #
    # Serialization routines, the optional arguments have the following
    # meaning:
    #     encoding: string to ask saving in a specific encoding
    #     indent: if 1 the serializer is asked to indent the output
    #
    def serialize(self, encoding = None, format = 0):
        return libxml2mod.serializeNode(self._o, encoding, format)
    def saveTo(self, file, encoding = None, format = 0):
        return libxml2mod.saveNodeTo(self._o, file, encoding, format)
            
    #
    # Canonicalization routines:
    #
    #   nodes: the node set (tuple or list) to be included in the
    #     canonized image or None if all document nodes should be
    #     included.
    #   exclusive: the exclusive flag (0 - non-exclusive
    #     canonicalization; otherwise - exclusive canonicalization)
    #   prefixes: the list of inclusive namespace prefixes (strings),
    #     or None if there is no inclusive namespaces (only for
    #     exclusive canonicalization, ignored otherwise)
    #   with_comments: include comments in the result (!=0) or not
    #     (==0)
    def c14nMemory(self,
                   nodes=None,
                   exclusive=0,
                   prefixes=None,
                   with_comments=0):
        if nodes:
            nodes = map(lambda n: n._o, nodes)
        return libxml2mod.xmlC14NDocDumpMemory(
            self.get_doc()._o,
            nodes,
            exclusive != 0,
            prefixes,
            with_comments != 0)
    def c14nSaveTo(self,
                   file,
                   nodes=None,
                   exclusive=0,
                   prefixes=None,
                   with_comments=0):
        if nodes:
            nodes = map(lambda n: n._o, nodes)
        return libxml2mod.xmlC14NDocSaveTo(
            self.get_doc()._o,
            nodes,
            exclusive != 0,
            prefixes,
            with_comments != 0,
            file)

    #
    # Selecting nodes using XPath, a bit slow because the context
    # is allocated/freed every time but convenient.
    #
    def xpathEval(self, expr):
        doc = self.doc
        if doc == None:
            return None
        ctxt = doc.xpathNewContext()
        ctxt.setContextNode(self)
        res = ctxt.xpathEval(expr)
        ctxt.xpathFreeContext()
        return res

#    #
#    # Selecting nodes using XPath, faster because the context
#    # is allocated just once per xmlDoc.
#    #
#    # Removed: DV memleaks c.f. #126735
#    #
#    def xpathEval2(self, expr):
#        doc = self.doc
#        if doc == None:
#            return None
#        try:
#            doc._ctxt.setContextNode(self)
#        except:
#            doc._ctxt = doc.xpathNewContext()
#            doc._ctxt.setContextNode(self)
#        res = doc._ctxt.xpathEval(expr)
#        return res
    def xpathEval2(self, expr):
        return self.xpathEval(expr)

    # Remove namespaces
    def removeNsDef(self, href):
        """
        Remove a namespace definition from a node.  If href is None,
        remove all of the ns definitions on that node.  The removed
        namespaces are returned as a linked list.

        Note: If any child nodes referred to the removed namespaces,
        they will be left with dangling links.  You should call
        renciliateNs() to fix those pointers.

        Note: This method does not free memory taken by the ns
        definitions.  You will need to free it manually with the
        freeNsList() method on the returns xmlNs object.
        """

        ret = libxml2mod.xmlNodeRemoveNsDef(self._o, href)
        if ret is None:return None
        __tmp = xmlNs(_obj=ret)
        return __tmp

    # support for python2 iterators
    def walk_depth_first(self):
        return xmlCoreDepthFirstItertor(self)
    def walk_breadth_first(self):
        return xmlCoreBreadthFirstItertor(self)
    __iter__ = walk_depth_first

    def free(self):
        try:
            self.doc._ctxt.xpathFreeContext()
        except:
            pass
        libxml2mod.xmlFreeDoc(self._o)


#
# implements the depth-first iterator for libxml2 DOM tree
#
class xmlCoreDepthFirstItertor:
    def __init__(self, node):
        self.node = node
        self.parents = []
    def __iter__(self):
        return self
    def next(self):
        while 1:
            if self.node:
                ret = self.node
                self.parents.append(self.node)
                self.node = self.node.children
                return ret
            try:
                parent = self.parents.pop()
            except IndexError:
                raise StopIteration
            self.node = parent.next

#
# implements the breadth-first iterator for libxml2 DOM tree
#
class xmlCoreBreadthFirstItertor:
    def __init__(self, node):
        self.node = node
        self.parents = []
    def __iter__(self):
        return self
    def next(self):
        while 1:
            if self.node:
                ret = self.node
                self.parents.append(self.node)
                self.node = self.node.next
                return ret
            try:
                parent = self.parents.pop()
            except IndexError:
                raise StopIteration
            self.node = parent.children

#
# converters to present a nicer view of the XPath returns
#
def nodeWrap(o):
    # TODO try to cast to the most appropriate node class
    name = libxml2mod.type(o)
    if name == "element" or name == "text":
        return xmlNode(_obj=o)
    if name == "attribute":
        return xmlAttr(_obj=o)
    if name[0:8] == "document":
        return xmlDoc(_obj=o)
    if name == "namespace":
        return xmlNs(_obj=o)
    if name == "elem_decl":
        return xmlElement(_obj=o)
    if name == "attribute_decl":
        return xmlAttribute(_obj=o)
    if name == "entity_decl":
        return xmlEntity(_obj=o)
    if name == "dtd":
        return xmlDtd(_obj=o)
    return xmlNode(_obj=o)

def xpathObjectRet(o):
    if type(o) == type([]) or type(o) == type(()):
        ret = map(lambda x: nodeWrap(x), o)
        return ret
    return o

#
# register an XPath function
#
def registerXPathFunction(ctxt, name, ns_uri, f):
    ret = libxml2mod.xmlRegisterXPathFunction(ctxt, name, ns_uri, f)

#
# For the xmlTextReader parser configuration
#
PARSER_LOADDTD=1
PARSER_DEFAULTATTRS=2
PARSER_VALIDATE=3
PARSER_SUBST_ENTITIES=4

#
# For the error callback severities
#
PARSER_SEVERITY_VALIDITY_WARNING=1
PARSER_SEVERITY_VALIDITY_ERROR=2
PARSER_SEVERITY_WARNING=3
PARSER_SEVERITY_ERROR=4

#
# register the libxml2 error handler
#
def registerErrorHandler(f, ctx):
    """Register a Python written function to for error reporting.
       The function is called back as f(ctx, error). """
    import sys
    if not sys.modules.has_key('libxslt'):
        # normal behaviour when libxslt is not imported
        ret = libxml2mod.xmlRegisterErrorHandler(f,ctx)
    else:
        # when libxslt is already imported, one must
        # use libxst's error handler instead
        import libxslt
        ret = libxslt.registerErrorHandler(f,ctx)
    return ret

class parserCtxtCore:

    def __init__(self, _obj=None):
        if _obj != None: 
            self._o = _obj;
            return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeParserCtxt(self._o)
        self._o = None

    def setErrorHandler(self,f,arg):
        """Register an error handler that will be called back as
           f(arg,msg,severity,reserved).
           
           @reserved is currently always None."""
        libxml2mod.xmlParserCtxtSetErrorHandler(self._o,f,arg)

    def getErrorHandler(self):
        """Return (f,arg) as previously registered with setErrorHandler
           or (None,None)."""
        return libxml2mod.xmlParserCtxtGetErrorHandler(self._o)

    def addLocalCatalog(self, uri):
        """Register a local catalog with the parser"""
        return libxml2mod.addLocalCatalog(self._o, uri)
    

class ValidCtxtCore:

    def __init__(self, *args, **kw):
        pass

    def setValidityErrorHandler(self, err_func, warn_func, arg=None):
        """
        Register error and warning handlers for DTD validation.
        These will be called back as f(msg,arg)
        """
        libxml2mod.xmlSetValidErrors(self._o, err_func, warn_func, arg)
    

class SchemaValidCtxtCore:

    def __init__(self, *args, **kw):
        pass

    def setValidityErrorHandler(self, err_func, warn_func, arg=None):
        """
        Register error and warning handlers for Schema validation.
        These will be called back as f(msg,arg)
        """
        libxml2mod.xmlSchemaSetValidErrors(self._o, err_func, warn_func, arg)


class relaxNgValidCtxtCore:

    def __init__(self, *args, **kw):
        pass

    def setValidityErrorHandler(self, err_func, warn_func, arg=None):
        """
        Register error and warning handlers for RelaxNG validation.
        These will be called back as f(msg,arg)
        """
        libxml2mod.xmlRelaxNGSetValidErrors(self._o, err_func, warn_func, arg)

    
def _xmlTextReaderErrorFunc((f,arg),msg,severity,locator):
    """Intermediate callback to wrap the locator"""
    return f(arg,msg,severity,xmlTextReaderLocator(locator))

class xmlTextReaderCore:

    def __init__(self, _obj=None):
        self.input = None
        if _obj != None:self._o = _obj;return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeTextReader(self._o)
        self._o = None

    def SetErrorHandler(self,f,arg):
        """Register an error handler that will be called back as
           f(arg,msg,severity,locator)."""
        if f is None:
            libxml2mod.xmlTextReaderSetErrorHandler(\
                self._o,None,None)
        else:
            libxml2mod.xmlTextReaderSetErrorHandler(\
                self._o,_xmlTextReaderErrorFunc,(f,arg))

    def GetErrorHandler(self):
        """Return (f,arg) as previously registered with setErrorHandler
           or (None,None)."""
        f,arg = libxml2mod.xmlTextReaderGetErrorHandler(self._o)
        if f is None:
            return None,None
        else:
            # assert f is _xmlTextReaderErrorFunc
            return arg

#
# The cleanup now goes though a wrappe in libxml.c
#
def cleanupParser():
    libxml2mod.xmlPythonCleanupParser()

# WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
#
# Everything before this line comes from libxml.py 
# Everything after this line is automatically generated
#
# WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

