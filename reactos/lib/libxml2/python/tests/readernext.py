#!/usr/bin/python -u
# -*- coding: ISO-8859-1 -*-
#
# this tests the next API of the XmlTextReader interface
#
import libxml2
import StringIO
import sys

# Memory debug specific
libxml2.debugMemory(1)

f = StringIO.StringIO("""<a><b><c /></b><d>content of d</d></a>""")
input = libxml2.inputBuffer(f)
reader = input.newTextReader("test_next")
ret = reader.Read()
if ret != 1:
    print "test_next: Error reading to first element"
    sys.exit(1)
if reader.Name() != "a" or reader.IsEmptyElement() != 0 or \
   reader.NodeType() != 1 or reader.HasAttributes() != 0:
    print "test_next: Error reading the first element"
    sys.exit(1)
ret = reader.Read()
if ret != 1:
    print "test_next: Error reading to second element"
    sys.exit(1)
if reader.Name() != "b" or reader.IsEmptyElement() != 0 or \
   reader.NodeType() != 1 or reader.HasAttributes() != 0:
    print "test_next: Error reading the second element"
    sys.exit(1)
ret = reader.Read()
if ret != 1:
    print "test_next: Error reading to third element"
    sys.exit(1)
if reader.Name() != "c" or reader.NodeType() != 1 or \
   reader.HasAttributes() != 0:
    print "test_next: Error reading the third element"
    sys.exit(1)
ret = reader.Read()
if ret != 1:
    print "test_next: Error reading to end of third element"
    sys.exit(1)
if reader.Name() != "b" or reader.NodeType() != 15:
    print "test_next: Error reading to end of second element"
    sys.exit(1)
ret = reader.Next()
if ret != 1:
    print "test_next: Error moving to third element"
    sys.exit(1)
if reader.Name() != "d" or reader.IsEmptyElement() != 0 or \
   reader.NodeType() != 1 or reader.HasAttributes() != 0:
    print "test_next: Error reading third element"
    sys.exit(1)
ret = reader.Next()
if ret != 1:
    print "test_next: Error reading to end of first element"
    sys.exit(1)
if reader.Name() != "a" or reader.IsEmptyElement() != 0 or \
   reader.NodeType() != 15 or reader.HasAttributes() != 0:
    print "test_next: Error reading the end of first element"
    sys.exit(1)
ret = reader.Read()
if ret != 0:
    print "test_next: Error reading to end of document"
    sys.exit(1)

#
# cleanup for memory allocation counting
#
del f
del input
del reader

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
