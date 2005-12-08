#!/usr/bin/python -u
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

re = libxml2.regexpCompile("a|b")
if re.regexpExec("a") != 1:
    print "error checking 'a'"
    sys.exit(1)
if re.regexpExec("b") != 1:
    print "error checking 'b'"
    sys.exit(1)
if re.regexpExec("ab") != 0:
    print "error checking 'ab'"
    sys.exit(1)
if re.regexpExec("") != 0:
    print "error checking 'ab'"
    sys.exit(1)
if re.regexpIsDeterminist() != 1:
    print "error checking determinism"
    sys.exit(1)
del re
    

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
