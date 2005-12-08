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

dtd="""<!ELEMENT foo EMPTY>"""
valid="""<?xml version="1.0"?>
<foo></foo>"""

invalid="""<?xml version="1.0"?>
<foo><bar/></foo>"""

dtd = libxml2.parseDTD(None, 'test.dtd')
ctxt = libxml2.newValidCtxt()
e = ErrorHandler()
ctxt.setValidityErrorHandler(e.handler, e.handler, ARG)

# Test valid document
doc = libxml2.parseDoc(valid)
ret = doc.validateDtd(ctxt, dtd)
if ret != 1 or e.errors:
    print "error doing DTD validation"
    sys.exit(1)
doc.freeDoc()

# Test invalid document
doc = libxml2.parseDoc(invalid)
ret = doc.validateDtd(ctxt, dtd)
if ret != 0 or not e.errors:
    print "Error: document supposed to be invalid"
doc.freeDoc()

dtd.freeDtd()
del dtd
del ctxt

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()

