#!/usr/bin/python -u
import libxml2
import sys

ARG = 'test string'

class ErrorHandler:

    def __init__(self):
        self.errors = []

    def handler(self, msg, data):
        if data != ARG:
            raise Exception, "Error handler did not receive correct argument"
        self.errors.append(msg)

# Memory debug specific
libxml2.debugMemory(1)

schema="""<?xml version="1.0"?>
<element name="foo"
         xmlns="http://relaxng.org/ns/structure/1.0"
         xmlns:a="http://relaxng.org/ns/annotation/1.0"
         xmlns:ex1="http://www.example.com/n1"
         xmlns:ex2="http://www.example.com/n2">
  <a:documentation>A foo element.</a:documentation>
  <element name="ex1:bar1">
    <empty/>
  </element>
  <element name="ex2:bar2">
    <empty/>
  </element>
</element>
"""

valid="""<?xml version="1.0"?>
<foo><pre1:bar1 xmlns:pre1="http://www.example.com/n1"/><pre2:bar2 xmlns:pre2="http://www.example.com/n2"/></foo>"""

invalid="""<?xml version="1.0"?>
<foo><pre1:bar1 xmlns:pre1="http://www.example.com/n1">bad</pre1:bar1><pre2:bar2 xmlns:pre2="http://www.example.com/n2"/></foo>"""

rngp = libxml2.relaxNGNewMemParserCtxt(schema, len(schema))
rngs = rngp.relaxNGParse()
ctxt = rngs.relaxNGNewValidCtxt()
e = ErrorHandler()
ctxt.setValidityErrorHandler(e.handler, e.handler, ARG)

# Test valid document
doc = libxml2.parseDoc(valid)
ret = doc.relaxNGValidateDoc(ctxt)
if ret != 0 or e.errors:
    print "error doing RelaxNG validation"
    sys.exit(1)
doc.freeDoc()

# Test invalid document
doc = libxml2.parseDoc(invalid)
ret = doc.relaxNGValidateDoc(ctxt)
if ret == 0 or not e.errors:
    print "Error: document supposed to be RelaxNG invalid"
    sys.exit(1)
doc.freeDoc()

del rngp
del rngs
del ctxt
libxml2.relaxNGCleanupTypes()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()

