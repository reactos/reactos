#!/usr/bin/python -u
#
# this tests the entities substitutions with the XmlTextReader interface
#
import sys
import StringIO
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

#
# Parse a document testing the Close() API
#
docstr="""<foo>
<label>some text</label>
<item>100</item>
</foo>"""

reader = libxml2.readerForDoc(docstr, "test1", None, 0)
ret = reader.Read()
ret = reader.Read()
ret = reader.Close()

if ret != 0:
    print "Error closing the document test1"
    sys.exit(1)

del reader

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
