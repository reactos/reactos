#!/usr/bin/python -u
#
# This test exercise the redirection of error messages with a
# functions defined in Python.
#
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

expect='--> I/O --> warning : --> failed to load external entity "missing.xml"\n'
err=""
def callback(ctx, str):
     global err

     err = err + "%s %s" % (ctx, str)

got_exc = 0
libxml2.registerErrorHandler(callback, "-->")
try:
    doc = libxml2.parseFile("missing.xml")
except libxml2.parserError:
    got_exc = 1

if got_exc == 0:
    print "Failed to get a parser exception"
    sys.exit(1)

if err != expect:
    print "error"
    print "received %s" %(err)
    print "expected %s" %(expect)
    sys.exit(1)

i = 10000
while i > 0:
    try:
        doc = libxml2.parseFile("missing.xml")
    except libxml2.parserError:
        got_exc = 1
    err = ""
    i = i - 1

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
