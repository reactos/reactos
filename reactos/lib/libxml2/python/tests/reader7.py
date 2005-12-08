#!/usr/bin/python -u
#
# this tests the entities substitutions with the XmlTextReader interface
#
import sys
import StringIO
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

result = ""
def processNode(reader):
    global result

    result = result + "%d %d %s %d\n" % (reader.Depth(), reader.NodeType(),
			   reader.Name(), reader.IsEmptyElement())

#
# Parse a document testing the readerForxxx API
#
docstr="""<foo>
<label>some text</label>
<item>100</item>
</foo>"""
expect="""0 1 foo 0
1 14 #text 0
1 1 label 0
2 3 #text 0
1 15 label 0
1 14 #text 0
1 1 item 0
2 3 #text 0
1 15 item 0
1 14 #text 0
0 15 foo 0
"""
result = ""

reader = libxml2.readerForDoc(docstr, "test1", None, 0)
ret = reader.Read()
while ret == 1:
    processNode(reader)
    ret = reader.Read()

if ret != 0:
    print "Error parsing the document test1"
    sys.exit(1)

if result != expect:
    print "Unexpected result for test1"
    print result
    sys.exit(1)

#
# Reuse the reader for another document testing the ReaderNewxxx API
#
docstr="""<foo>
<label>some text</label>
<item>1000</item>
</foo>"""
expect="""0 1 foo 0
1 14 #text 0
1 1 label 0
2 3 #text 0
1 15 label 0
1 14 #text 0
1 1 item 0
2 3 #text 0
1 15 item 0
1 14 #text 0
0 15 foo 0
"""
result = ""

reader.NewDoc(docstr, "test2", None, 0)
ret = reader.Read()
while ret == 1:
    processNode(reader)
    ret = reader.Read()

if ret != 0:
    print "Error parsing the document test2"
    sys.exit(1)

if result != expect:
    print "Unexpected result for test2"
    print result
    sys.exit(1)

#
# cleanup
#
del reader

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
