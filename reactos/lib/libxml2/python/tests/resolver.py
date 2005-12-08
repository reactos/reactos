#!/usr/bin/python -u
import sys
import libxml2
import StringIO

# Memory debug specific
libxml2.debugMemory(1)

def myResolver(URL, ID, ctxt):
    return(StringIO.StringIO("<foo/>"))

libxml2.setEntityLoader(myResolver)

doc = libxml2.parseFile("doesnotexist.xml")
root = doc.children
if root.name != "foo":
    print "root element name error"
    sys.exit(1)
doc.freeDoc()

i = 0
while i < 5000:
    doc = libxml2.parseFile("doesnotexist.xml")
    root = doc.children
    if root.name != "foo":
        print "root element name error"
        sys.exit(1)
    doc.freeDoc()
    i = i + 1


# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()

