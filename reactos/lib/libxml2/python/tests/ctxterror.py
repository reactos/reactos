#!/usr/bin/python -u
#
# This test exercise the redirection of error messages with a
# functions defined in Python.
#
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

expect="""--> (3) xmlns: URI foo is not absolute
--> (4) Opening and ending tag mismatch: x line 0 and y
"""

err=""
def callback(arg,msg,severity,reserved):
    global err
    err = err + "%s (%d) %s" % (arg,severity,msg)

s = """<x xmlns="foo"></y>"""

parserCtxt = libxml2.createPushParser(None,"",0,"test.xml")
parserCtxt.setErrorHandler(callback, "-->")
if parserCtxt.getErrorHandler() != (callback,"-->"):
    print "getErrorHandler failed"
    sys.exit(1)
parserCtxt.parseChunk(s,len(s),1)
doc = parserCtxt.doc()
doc.freeDoc()
parserCtxt = None

if err != expect:
    print "error"
    print "received %s" %(err)
    print "expected %s" %(expect)
    sys.exit(1)

i = 10000
while i > 0:
    parserCtxt = libxml2.createPushParser(None,"",0,"test.xml")
    parserCtxt.setErrorHandler(callback, "-->")
    parserCtxt.parseChunk(s,len(s),1)
    doc = parserCtxt.doc()
    doc.freeDoc()
    parserCtxt = None
    err = ""
    i = i - 1

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
