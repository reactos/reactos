#!/usr/bin/python -u
import sys
import libxml2
import StringIO

# Memory debug specific
libxml2.debugMemory(1)

i = 0
while i < 5000:
    f = StringIO.StringIO("foobar")
    buf = libxml2.inputBuffer(f)
    i = i + 1

del f
del buf

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()

