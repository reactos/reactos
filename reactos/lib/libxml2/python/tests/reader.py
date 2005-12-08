#!/usr/bin/python -u
# -*- coding: ISO-8859-1 -*-
#
# this tests the basic APIs of the XmlTextReader interface
#
import libxml2
import StringIO
import sys

# Memory debug specific
libxml2.debugMemory(1)

f = StringIO.StringIO("""<a><b b1="b1"/><c>content of c</c></a>""")
input = libxml2.inputBuffer(f)
reader = input.newTextReader("test1")
ret = reader.Read()
if ret != 1:
    print "test1: Error reading to first element"
    sys.exit(1)
if reader.Name() != "a" or reader.IsEmptyElement() != 0 or \
   reader.NodeType() != 1 or reader.HasAttributes() != 0:
    print "test1: Error reading the first element"
    sys.exit(1)
ret = reader.Read()
if ret != 1:
    print "test1: Error reading to second element"
    sys.exit(1)
if reader.Name() != "b" or reader.IsEmptyElement() != 1 or \
   reader.NodeType() != 1 or reader.HasAttributes() != 1:
    print "test1: Error reading the second element"
    sys.exit(1)
ret = reader.Read()
if ret != 1:
    print "test1: Error reading to third element"
    sys.exit(1)
if reader.Name() != "c" or reader.IsEmptyElement() != 0 or \
   reader.NodeType() != 1 or reader.HasAttributes() != 0:
    print "test1: Error reading the third element"
    sys.exit(1)
ret = reader.Read()
if ret != 1:
    print "test1: Error reading to text node"
    sys.exit(1)
if reader.Name() != "#text" or reader.IsEmptyElement() != 0 or \
   reader.NodeType() != 3 or reader.HasAttributes() != 0 or \
   reader.Value() != "content of c":
    print "test1: Error reading the text node"
    sys.exit(1)
ret = reader.Read()
if ret != 1:
    print "test1: Error reading to end of third element"
    sys.exit(1)
if reader.Name() != "c" or reader.IsEmptyElement() != 0 or \
   reader.NodeType() != 15 or reader.HasAttributes() != 0:
    print "test1: Error reading the end of third element"
    sys.exit(1)
ret = reader.Read()
if ret != 1:
    print "test1: Error reading to end of first element"
    sys.exit(1)
if reader.Name() != "a" or reader.IsEmptyElement() != 0 or \
   reader.NodeType() != 15 or reader.HasAttributes() != 0:
    print "test1: Error reading the end of first element"
    sys.exit(1)
ret = reader.Read()
if ret != 0:
    print "test1: Error reading to end of document"
    sys.exit(1)

#
# example from the XmlTextReader docs
#
f = StringIO.StringIO("""<test xmlns:dt="urn:datatypes" dt:type="int"/>""")
input = libxml2.inputBuffer(f)
reader = input.newTextReader("test2")

ret = reader.Read()
if ret != 1:
    print "Error reading test element"
    sys.exit(1)
if reader.GetAttributeNo(0) != "urn:datatypes" or \
   reader.GetAttributeNo(1) != "int" or \
   reader.GetAttributeNs("type", "urn:datatypes") != "int" or \
   reader.GetAttribute("dt:type") != "int":
    print "error reading test attributes"
    sys.exit(1)

#
# example from the XmlTextReader docs
#
f = StringIO.StringIO("""<root xmlns:a="urn:456">
<item>
<ref href="a:b"/>
</item>
</root>""")
input = libxml2.inputBuffer(f)
reader = input.newTextReader("test3")

ret = reader.Read()
while ret == 1:
    if reader.Name() == "ref":
        if reader.LookupNamespace("a") != "urn:456":
            print "error resolving namespace prefix"
            sys.exit(1)
        break
    ret = reader.Read()
if ret != 1:
    print "Error finding the ref element"
    sys.exit(1)

#
# Home made example for the various attribute access functions
#
f = StringIO.StringIO("""<testattr xmlns="urn:1" xmlns:a="urn:2" b="b" a:b="a:b"/>""")
input = libxml2.inputBuffer(f)
reader = input.newTextReader("test4")
ret = reader.Read()
if ret != 1:
    print "Error reading the testattr element"
    sys.exit(1)
#
# Attribute exploration by index
#
if reader.MoveToAttributeNo(0) != 1:
    print "Failed moveToAttribute(0)"
    sys.exit(1)
if reader.Value() != "urn:1":
    print "Failed to read attribute(0)"
    sys.exit(1)
if reader.Name() != "xmlns":
    print "Failed to read attribute(0) name"
    sys.exit(1)
if reader.MoveToAttributeNo(1) != 1:
    print "Failed moveToAttribute(1)"
    sys.exit(1)
if reader.Value() != "urn:2":
    print "Failed to read attribute(1)"
    sys.exit(1)
if reader.Name() != "xmlns:a":
    print "Failed to read attribute(1) name"
    sys.exit(1)
if reader.MoveToAttributeNo(2) != 1:
    print "Failed moveToAttribute(2)"
    sys.exit(1)
if reader.Value() != "b":
    print "Failed to read attribute(2)"
    sys.exit(1)
if reader.Name() != "b":
    print "Failed to read attribute(2) name"
    sys.exit(1)
if reader.MoveToAttributeNo(3) != 1:
    print "Failed moveToAttribute(3)"
    sys.exit(1)
if reader.Value() != "a:b":
    print "Failed to read attribute(3)"
    sys.exit(1)
if reader.Name() != "a:b":
    print "Failed to read attribute(3) name"
    sys.exit(1)
#
# Attribute exploration by name
#
if reader.MoveToAttribute("xmlns") != 1:
    print "Failed moveToAttribute('xmlns')"
    sys.exit(1)
if reader.Value() != "urn:1":
    print "Failed to read attribute('xmlns')"
    sys.exit(1)
if reader.MoveToAttribute("xmlns:a") != 1:
    print "Failed moveToAttribute('xmlns')"
    sys.exit(1)
if reader.Value() != "urn:2":
    print "Failed to read attribute('xmlns:a')"
    sys.exit(1)
if reader.MoveToAttribute("b") != 1:
    print "Failed moveToAttribute('b')"
    sys.exit(1)
if reader.Value() != "b":
    print "Failed to read attribute('b')"
    sys.exit(1)
if reader.MoveToAttribute("a:b") != 1:
    print "Failed moveToAttribute('a:b')"
    sys.exit(1)
if reader.Value() != "a:b":
    print "Failed to read attribute('a:b')"
    sys.exit(1)
if reader.MoveToAttributeNs("b", "urn:2") != 1:
    print "Failed moveToAttribute('b', 'urn:2')"
    sys.exit(1)
if reader.Value() != "a:b":
    print "Failed to read attribute('b', 'urn:2')"
    sys.exit(1)
#
# Go back and read in sequence
#
if reader.MoveToElement() != 1:
    print "Failed to move back to element"
    sys.exit(1)
if reader.MoveToFirstAttribute() != 1:
    print "Failed to move to first attribute"
    sys.exit(1)
if reader.Value() != "urn:1":
    print "Failed to read attribute(0)"
    sys.exit(1)
if reader.Name() != "xmlns":
    print "Failed to read attribute(0) name"
    sys.exit(1)
if reader.MoveToNextAttribute() != 1:
    print "Failed to move to next attribute"
    sys.exit(1)
if reader.Value() != "urn:2":
    print "Failed to read attribute(1)"
    sys.exit(1)
if reader.Name() != "xmlns:a":
    print "Failed to read attribute(1) name"
    sys.exit(1)
if reader.MoveToNextAttribute() != 1:
    print "Failed to move to next attribute"
    sys.exit(1)
if reader.Value() != "b":
    print "Failed to read attribute(2)"
    sys.exit(1)
if reader.Name() != "b":
    print "Failed to read attribute(2) name"
    sys.exit(1)
if reader.MoveToNextAttribute() != 1:
    print "Failed to move to next attribute"
    sys.exit(1)
if reader.Value() != "a:b":
    print "Failed to read attribute(3)"
    sys.exit(1)
if reader.Name() != "a:b":
    print "Failed to read attribute(3) name"
    sys.exit(1)
if reader.MoveToNextAttribute() != 0:
    print "Failed to detect last attribute"
    sys.exit(1)

    
#
# a couple of tests for namespace nodes
#
f = StringIO.StringIO("""<a xmlns="http://example.com/foo"/>""")
input = libxml2.inputBuffer(f)
reader = input.newTextReader("test6")
ret = reader.Read()
if ret != 1:
    print "test6: failed to Read()"
    sys.exit(1)
ret = reader.MoveToFirstAttribute()
if ret != 1:
    print "test6: failed to MoveToFirstAttribute()"
    sys.exit(1)
if reader.NamespaceUri() != "http://www.w3.org/2000/xmlns/" or \
   reader.LocalName() != "xmlns" or reader.Name() != "xmlns" or \
   reader.Value() != "http://example.com/foo" or reader.NodeType() != 2:
    print "test6: failed to read the namespace node"
    sys.exit(1)

f = StringIO.StringIO("""<a xmlns:prefix="http://example.com/foo"/>""")
input = libxml2.inputBuffer(f)
reader = input.newTextReader("test7")
ret = reader.Read()
if ret != 1:
    print "test7: failed to Read()"
    sys.exit(1)
ret = reader.MoveToFirstAttribute()
if ret != 1:
    print "test7: failed to MoveToFirstAttribute()"
    sys.exit(1)
if reader.NamespaceUri() != "http://www.w3.org/2000/xmlns/" or \
   reader.LocalName() != "prefix" or reader.Name() != "xmlns:prefix" or \
   reader.Value() != "http://example.com/foo" or reader.NodeType() != 2:
    print "test7: failed to read the namespace node"
    sys.exit(1)

#
# Test for a limit case:
#
f = StringIO.StringIO("""<a/>""")
input = libxml2.inputBuffer(f)
reader = input.newTextReader("test8")
ret = reader.Read()
if ret != 1:
    print "test8: failed to read the node"
    sys.exit(1)
if reader.Name() != "a" or reader.IsEmptyElement() != 1:
    print "test8: failed to analyze the node"
    sys.exit(1)
ret = reader.Read()
if ret != 0:
    print "test8: failed to detect the EOF"
    sys.exit(1)

#
# Another test provided by Stéphane Bidoul and checked with C#
#
def tst_reader(s):
    f = StringIO.StringIO(s)
    input = libxml2.inputBuffer(f)
    reader = input.newTextReader("tst")
    res = ""
    while reader.Read():
        res=res + "%s (%s) [%s] %d %d\n" % (reader.NodeType(),reader.Name(),
                                      reader.Value(), reader.IsEmptyElement(),
                                      reader.Depth())
        if reader.NodeType() == 1: # Element
            while reader.MoveToNextAttribute():
                res = res + "-- %s (%s) [%s] %d %d\n" % (reader.NodeType(),
                                       reader.Name(),reader.Value(),
                                       reader.IsEmptyElement(), reader.Depth())
    return res
    
doc="""<a><b b1="b1"/><c>content of c</c></a>"""
expect="""1 (a) [None] 0 0
1 (b) [None] 1 1
-- 2 (b1) [b1] 0 2
1 (c) [None] 0 1
3 (#text) [content of c] 0 2
15 (c) [None] 0 1
15 (a) [None] 0 0
"""
res = tst_reader(doc)
if res != expect:
    print "test5 failed"
    print res
    sys.exit(1)

doc="""<test><b/><c/></test>"""
expect="""1 (test) [None] 0 0
1 (b) [None] 1 1
1 (c) [None] 1 1
15 (test) [None] 0 0
"""
res = tst_reader(doc)
if res != expect:
    print "test9 failed"
    print res
    sys.exit(1)

doc="""<a><b>bbb</b><c>ccc</c></a>"""
expect="""1 (a) [None] 0 0
1 (b) [None] 0 1
3 (#text) [bbb] 0 2
15 (b) [None] 0 1
1 (c) [None] 0 1
3 (#text) [ccc] 0 2
15 (c) [None] 0 1
15 (a) [None] 0 0
"""
res = tst_reader(doc)
if res != expect:
    print "test10 failed"
    print res
    sys.exit(1)

doc="""<test a="a"/>"""
expect="""1 (test) [None] 1 0
-- 2 (a) [a] 0 1
"""
res = tst_reader(doc)
if res != expect:
    print "test11 failed"
    print res
    sys.exit(1)

doc="""<test><a>aaa</a><b/></test>"""
expect="""1 (test) [None] 0 0
1 (a) [None] 0 1
3 (#text) [aaa] 0 2
15 (a) [None] 0 1
1 (b) [None] 1 1
15 (test) [None] 0 0
"""
res = tst_reader(doc)
if res != expect:
    print "test12 failed"
    print res
    sys.exit(1)

doc="""<test><p></p></test>"""
expect="""1 (test) [None] 0 0
1 (p) [None] 0 1
15 (p) [None] 0 1
15 (test) [None] 0 0
"""
res = tst_reader(doc)
if res != expect:
    print "test13 failed"
    print res
    sys.exit(1)

doc="""<p></p>"""
expect="""1 (p) [None] 0 0
15 (p) [None] 0 0
"""
res = tst_reader(doc)
if res != expect:
    print "test14 failed"
    print res
    sys.exit(1)

#
# test from bug #108801 
#
doc="""<?xml version="1.0" standalone="no"?>
<!DOCTYPE article PUBLIC "-//OASIS//DTD DocBook XML V4.1.2//EN"
                  "http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd" [
]>

<article>
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
</article>
"""
expect="""10 (article) [None] 0 0
1 (article) [None] 0 0
3 (#text) [
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
] 0 1
15 (article) [None] 0 0
"""
res = tst_reader(doc)
if res != expect:
    print "test15 failed"
    print res
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
