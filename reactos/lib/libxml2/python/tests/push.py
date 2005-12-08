#!/usr/bin/python -u
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

ctxt = libxml2.createPushParser(None, "<foo", 4, "test.xml")
ctxt.parseChunk("/>", 2, 1)
doc = ctxt.doc()
ctxt=None
if doc.name != "test.xml":
    print "document name error"
    sys.exit(1)
root = doc.children
if root.name != "foo":
    print "root element name error"
    sys.exit(1)
doc.freeDoc()
i = 10000
while i > 0:
    ctxt = libxml2.createPushParser(None, "<foo", 4, "test.xml")
    ctxt.parseChunk("/>", 2, 1)
    doc = ctxt.doc()
    doc.freeDoc()
    i = i -1
ctxt=None

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
