#!/usr/bin/python -u
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

ctxt = libxml2.createFileParserCtxt("valid.xml")
ctxt.validate(1)
ctxt.parseDocument()
doc = ctxt.doc()
valid = ctxt.isValid()

if doc.name != "valid.xml":
    print "doc.name failed"
    sys.exit(1)
root = doc.children
if root.name != "doc":
    print "root.name failed"
    sys.exit(1)
if valid != 1:
    print "validity chec failed"
    sys.exit(1)
doc.freeDoc()

i = 1000
while i > 0:
    ctxt = libxml2.createFileParserCtxt("valid.xml")
    ctxt.validate(1)
    ctxt.parseDocument()
    doc = ctxt.doc()
    valid = ctxt.isValid()
    doc.freeDoc()
    if valid != 1:
        print "validity check failed"
        sys.exit(1)
    i = i - 1

#desactivate error messages from the validation
def noerr(ctx, str):
    pass

libxml2.registerErrorHandler(noerr, None)

ctxt = libxml2.createFileParserCtxt("invalid.xml")
ctxt.validate(1)
ctxt.parseDocument()
doc = ctxt.doc()
valid = ctxt.isValid()
if doc.name != "invalid.xml":
    print "doc.name failed"
    sys.exit(1)
root = doc.children
if root.name != "doc":
    print "root.name failed"
    sys.exit(1)
if valid != 0:
    print "validity chec failed"
    sys.exit(1)
doc.freeDoc()

i = 1000
while i > 0:
    ctxt = libxml2.createFileParserCtxt("invalid.xml")
    ctxt.validate(1)
    ctxt.parseDocument()
    doc = ctxt.doc()
    valid = ctxt.isValid()
    doc.freeDoc()
    if valid != 0:
        print "validity check failed"
        sys.exit(1)
    i = i - 1
del ctxt

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
