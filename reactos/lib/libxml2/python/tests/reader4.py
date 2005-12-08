#!/usr/bin/python -u
#
# this tests the basic APIs of the XmlTextReader interface
#
import libxml2
import StringIO
import sys

# Memory debug specific
libxml2.debugMemory(1)

def tst_reader(s):
    f = StringIO.StringIO(s)
    input = libxml2.inputBuffer(f)
    reader = input.newTextReader("tst")
    res = ""
    while reader.Read():
        res=res + "%s (%s) [%s] %d\n" % (reader.NodeType(),reader.Name(),
				      reader.Value(), reader.IsEmptyElement())
        if reader.NodeType() == 1: # Element
            while reader.MoveToNextAttribute():
                res = res + "-- %s (%s) [%s]\n" % (reader.NodeType(),
						   reader.Name(),reader.Value())
    return res
    
expect="""1 (test) [None] 0
1 (b) [None] 1
1 (c) [None] 1
15 (test) [None] 0
"""

res = tst_reader("""<test><b/><c/></test>""")

if res != expect:
    print "Did not get the expected error message:"
    print res
    sys.exit(1)

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
