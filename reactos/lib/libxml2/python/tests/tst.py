#!/usr/bin/python -u
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

doc = libxml2.parseFile("tst.xml")
if doc.name != "tst.xml":
    print "doc.name failed"
    sys.exit(1)
root = doc.children
if root.name != "doc":
    print "root.name failed"
    sys.exit(1)
child = root.children
if child.name != "foo":
    print "child.name failed"
    sys.exit(1)
doc.freeDoc()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
