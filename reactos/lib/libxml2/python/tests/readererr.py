#!/usr/bin/python -u
#
# this tests the basic APIs of the XmlTextReader interface
#
import libxml2
import StringIO
import sys

# Memory debug specific
libxml2.debugMemory(1)

expect="""--> (3) test1:1:xmlns: URI foo is not absolute
--> (4) test1:1:Opening and ending tag mismatch: c line 0 and a
"""
err=""
def myErrorHandler(arg,msg,severity,locator):
    global err
    err = err + "%s (%d) %s:%d:%s" % (arg,severity,locator.BaseURI(),locator.LineNumber(),msg)

f = StringIO.StringIO("""<a xmlns="foo"><b b1="b1"/><c>content of c</a>""")
input = libxml2.inputBuffer(f)
reader = input.newTextReader("test1")
reader.SetErrorHandler(myErrorHandler,"-->")
while reader.Read() == 1:
    pass

if err != expect:
    print "error"
    print "received %s" %(err)
    print "expected %s" %(expect)
    sys.exit(1)

reader.SetErrorHandler(None,None)
if reader.GetErrorHandler() != (None,None):
    print "GetErrorHandler failed"
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
