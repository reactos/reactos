#!/usr/bin/python -u
#
# this test exercise the XPath basic engine, parser, etc, and
# allows to detect memory leaks
#
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

doc = libxml2.parseFile("tst.xml")
if doc.name != "tst.xml":
    print "doc.name error"
    sys.exit(1);

ctxt = doc.xpathNewContext()
res = ctxt.xpathEval("//*")
if len(res) != 2:
    print "xpath query: wrong node set size"
    sys.exit(1)
if res[0].name != "doc" or res[1].name != "foo":
    print "xpath query: wrong node set value"
    sys.exit(1)
ctxt.setContextNode(res[0])
res = ctxt.xpathEval("foo")
if len(res) != 1:
    print "xpath query: wrong node set size"
    sys.exit(1)
if res[0].name != "foo":
    print "xpath query: wrong node set value"
    sys.exit(1)
doc.freeDoc()
ctxt.xpathFreeContext()
i = 1000
while i > 0:
    doc = libxml2.parseFile("tst.xml")
    ctxt = doc.xpathNewContext()
    res = ctxt.xpathEval("//*")
    doc.freeDoc()
    ctxt.xpathFreeContext()
    i = i -1
del ctxt

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
