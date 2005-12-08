# -*- coding: iso-8859-1 -*-
""" A SAX2 driver for libxml2, on top of it's XmlReader API

USAGE
    # put this file (drv_libxml2.py) in PYTHONPATH
    import xml.sax
    reader = xml.sax.make_parser(["drv_libxml2"])
    # ...and the rest is standard python sax.

CAVEATS
    - Lexical handlers are supported, except for start/endEntity
      (waiting for XmlReader.ResolveEntity) and start/endDTD
    - Error callbacks are not exactly synchronous, they tend
      to be invoked before the corresponding content callback,
      because the underlying reader interface parses
      data by chunks of 512 bytes
    
TODO
    - search for TODO
    - some ErrorHandler events (warning)
    - some ContentHandler events (setDocumentLocator, skippedEntity)
    - EntityResolver (using libxml2.?)
    - DTDHandler (if/when libxml2 exposes such node types)
    - DeclHandler (if/when libxml2 exposes such node types)
    - property_xml_string?
    - feature_string_interning?
    - Incremental parser
    - additional performance tuning:
      - one might cache callbacks to avoid some name lookups
      - one might implement a smarter way to pass attributes to startElement
        (some kind of lazy evaluation?)
      - there might be room for improvement in start/endPrefixMapping
      - other?

"""

__author__  = u"Stéphane Bidoul <sbi@skynet.be>"
__version__ = "0.3"

import codecs
from types import StringType, UnicodeType
StringTypes = (StringType,UnicodeType)

from xml.sax._exceptions import *
from xml.sax import xmlreader, saxutils
from xml.sax.handler import \
     feature_namespaces, \
     feature_namespace_prefixes, \
     feature_string_interning, \
     feature_validation, \
     feature_external_ges, \
     feature_external_pes, \
     property_lexical_handler, \
     property_declaration_handler, \
     property_dom_node, \
     property_xml_string

# libxml2 returns strings as UTF8
_decoder = codecs.lookup("utf8")[1]
def _d(s):
    if s is None:
        return s
    else:
        return _decoder(s)[0]

try:
    import libxml2
except ImportError, e:
    raise SAXReaderNotAvailable("libxml2 not available: " \
                                "import error was: %s" % e)

class Locator(xmlreader.Locator):
    """SAX Locator adapter for libxml2.xmlTextReaderLocator"""

    def __init__(self,locator):
        self.__locator = locator

    def getColumnNumber(self):
        "Return the column number where the current event ends."
        return -1

    def getLineNumber(self):
        "Return the line number where the current event ends."
        return self.__locator.LineNumber()

    def getPublicId(self):
        "Return the public identifier for the current event."
        return None

    def getSystemId(self):
        "Return the system identifier for the current event."
        return self.__locator.BaseURI()

class LibXml2Reader(xmlreader.XMLReader):

    def __init__(self):
        xmlreader.XMLReader.__init__(self)
        # features
        self.__ns = 0
        self.__nspfx = 0
        self.__validate = 0
        self.__extparams = 1
        # parsing flag
        self.__parsing = 0
        # additional handlers
        self.__lex_handler = None
        self.__decl_handler = None
        # error messages accumulator
        self.__errors = None

    def _errorHandler(self,arg,msg,severity,locator):
        if self.__errors is None:
            self.__errors = []
        self.__errors.append((severity,
                              SAXParseException(msg,None,
                                                Locator(locator))))

    def _reportErrors(self,fatal):
        for severity,exception in self.__errors:
            if severity in (libxml2.PARSER_SEVERITY_VALIDITY_WARNING,
                            libxml2.PARSER_SEVERITY_WARNING):
                self._err_handler.warning(exception)
            else:
                # when fatal is set, the parse will stop;
                # we consider that the last error reported
                # is the fatal one.
                if fatal and exception is self.__errors[-1][1]:
                    self._err_handler.fatalError(exception)
                else:
                    self._err_handler.error(exception)
        self.__errors = None

    def parse(self, source):
        self.__parsing = 1
        try:
            # prepare source and create reader
            if type(source) in StringTypes:
                reader = libxml2.newTextReaderFilename(source)
            else:
                source = saxutils.prepare_input_source(source)
                input = libxml2.inputBuffer(source.getByteStream())
                reader = input.newTextReader(source.getSystemId())
            reader.SetErrorHandler(self._errorHandler,None)
            # configure reader
            if self.__extparams:
                reader.SetParserProp(libxml2.PARSER_LOADDTD,1)
                reader.SetParserProp(libxml2.PARSER_DEFAULTATTRS,1)
                reader.SetParserProp(libxml2.PARSER_SUBST_ENTITIES,1)
                reader.SetParserProp(libxml2.PARSER_VALIDATE,self.__validate)
            else:
                reader.SetParserProp(libxml2.PARSER_LOADDTD, 0)
            # we reuse attribute maps (for a slight performance gain)
            if self.__ns:
                attributesNSImpl = xmlreader.AttributesNSImpl({},{})
            else:
                attributesImpl = xmlreader.AttributesImpl({})
            # prefixes to pop (for endPrefixMapping)
            prefixes = []
            # start loop
            self._cont_handler.startDocument()
            while 1:
                r = reader.Read()
                # check for errors
                if r == 1:
                    if not self.__errors is None:
                        self._reportErrors(0)
                elif r == 0:
                    if not self.__errors is None:
                        self._reportErrors(0)
                    break # end of parse
                else:
                    if not self.__errors is None:
                        self._reportErrors(1)
                    else:
                        self._err_handler.fatalError(\
                            SAXException("Read failed (no details available)"))
                    break # fatal parse error
                # get node type
                nodeType = reader.NodeType()
                # Element
                if nodeType == 1: 
                    if self.__ns:
                        eltName = (_d(reader.NamespaceUri()),\
                                   _d(reader.LocalName()))
                        eltQName = _d(reader.Name())
                        attributesNSImpl._attrs = attrs = {}
                        attributesNSImpl._qnames = qnames = {}
                        newPrefixes = []
                        while reader.MoveToNextAttribute():
                            qname = _d(reader.Name())
                            value = _d(reader.Value())
                            if qname.startswith("xmlns"):
                                if len(qname) > 5:
                                    newPrefix = qname[6:]
                                else:
                                    newPrefix = None
                                newPrefixes.append(newPrefix)
                                self._cont_handler.startPrefixMapping(\
                                    newPrefix,value)
                                if not self.__nspfx:
                                    continue # don't report xmlns attribute
                            attName = (_d(reader.NamespaceUri()),
                                       _d(reader.LocalName()))
                            qnames[attName] = qname
                            attrs[attName] = value
                        reader.MoveToElement()
                        self._cont_handler.startElementNS( \
                            eltName,eltQName,attributesNSImpl) 
                        if reader.IsEmptyElement():
                            self._cont_handler.endElementNS(eltName,eltQName)
                            for newPrefix in newPrefixes:
                                self._cont_handler.endPrefixMapping(newPrefix)
                        else:
                            prefixes.append(newPrefixes)
                    else:
                        eltName = _d(reader.Name())
                        attributesImpl._attrs = attrs = {}
                        while reader.MoveToNextAttribute():
                            attName = _d(reader.Name())
                            attrs[attName] = _d(reader.Value())
                        reader.MoveToElement()
                        self._cont_handler.startElement( \
                            eltName,attributesImpl)
                        if reader.IsEmptyElement():
                            self._cont_handler.endElement(eltName)
                # EndElement
                elif nodeType == 15: 
                    if self.__ns:
                        self._cont_handler.endElementNS( \
                             (_d(reader.NamespaceUri()),_d(reader.LocalName())),
                             _d(reader.Name()))
                        for prefix in prefixes.pop():
                            self._cont_handler.endPrefixMapping(prefix)
                    else:
                        self._cont_handler.endElement(_d(reader.Name()))
                # Text
                elif nodeType == 3: 
                    self._cont_handler.characters(_d(reader.Value()))
                # Whitespace
                elif nodeType == 13: 
                    self._cont_handler.ignorableWhitespace(_d(reader.Value()))
                # SignificantWhitespace
                elif nodeType == 14:
                    self._cont_handler.characters(_d(reader.Value()))
                # CDATA
                elif nodeType == 4:
                    if not self.__lex_handler is None:
                        self.__lex_handler.startCDATA()
                    self._cont_handler.characters(_d(reader.Value()))
                    if not self.__lex_handler is None:
                        self.__lex_handler.endCDATA()
                # EntityReference
                elif nodeType == 5:
                    if not self.__lex_handler is None:
                        self.startEntity(_d(reader.Name()))
                    reader.ResolveEntity()
                # EndEntity
                elif nodeType == 16:
                    if not self.__lex_handler is None:
                        self.endEntity(_d(reader.Name()))
                # ProcessingInstruction
                elif nodeType == 7: 
                    self._cont_handler.processingInstruction( \
                        _d(reader.Name()),_d(reader.Value()))
                # Comment
                elif nodeType == 8:
                    if not self.__lex_handler is None:
                        self.__lex_handler.comment(_d(reader.Value()))
                # DocumentType
                elif nodeType == 10:
                    #if not self.__lex_handler is None:
                    #    self.__lex_handler.startDTD()
                    pass # TODO (how to detect endDTD? on first non-dtd event?)
                # XmlDeclaration
                elif nodeType == 17:
                    pass # TODO
                # Entity
                elif nodeType == 6:
                    pass # TODO (entity decl)
                # Notation (decl)
                elif nodeType == 12:
                    pass # TODO
                # Attribute (never in this loop)
                #elif nodeType == 2: 
                #    pass
                # Document (not exposed)
                #elif nodeType == 9: 
                #    pass
                # DocumentFragment (never returned by XmlReader)
                #elif nodeType == 11:
                #    pass
                # None
                #elif nodeType == 0:
                #    pass
                # -
                else:
                    raise SAXException("Unexpected node type %d" % nodeType)
            if r == 0:
                self._cont_handler.endDocument()
            reader.Close()
        finally:
            self.__parsing = 0

    def setDTDHandler(self, handler):
        # TODO (when supported, the inherited method works just fine)
        raise SAXNotSupportedException("DTDHandler not supported")

    def setEntityResolver(self, resolver):
        # TODO (when supported, the inherited method works just fine)
        raise SAXNotSupportedException("EntityResolver not supported")

    def getFeature(self, name):
        if name == feature_namespaces:
            return self.__ns
        elif name == feature_namespace_prefixes:
            return self.__nspfx
        elif name == feature_validation:
            return self.__validate
        elif name == feature_external_ges:
            return 1 # TODO (does that relate to PARSER_LOADDTD)?
        elif name == feature_external_pes:
            return self.__extparams
        else:
            raise SAXNotRecognizedException("Feature '%s' not recognized" % \
                                            name)

    def setFeature(self, name, state):
        if self.__parsing:
            raise SAXNotSupportedException("Cannot set feature %s " \
                                           "while parsing" % name)
        if name == feature_namespaces:
            self.__ns = state
        elif name == feature_namespace_prefixes:
            self.__nspfx = state
        elif name == feature_validation:
            self.__validate = state
        elif name == feature_external_ges:
            if state == 0:
                # TODO (does that relate to PARSER_LOADDTD)?
                raise SAXNotSupportedException("Feature '%s' not supported" % \
                                               name)
        elif name == feature_external_pes:
            self.__extparams = state
        else:
            raise SAXNotRecognizedException("Feature '%s' not recognized" % \
                                            name)

    def getProperty(self, name):
        if name == property_lexical_handler:
            return self.__lex_handler
        elif name == property_declaration_handler:
            return self.__decl_handler
        else:
            raise SAXNotRecognizedException("Property '%s' not recognized" % \
                                            name)

    def setProperty(self, name, value):     
        if name == property_lexical_handler:
            self.__lex_handler = value
        elif name == property_declaration_handler:
            # TODO: remove if/when libxml2 supports dtd events
            raise SAXNotSupportedException("Property '%s' not supported" % \
                                           name)
            self.__decl_handler = value
        else:
            raise SAXNotRecognizedException("Property '%s' not recognized" % \
                                            name)

def create_parser():
    return LibXml2Reader()

