#!/usr/bin/python -u
import libxml2
import sys

# Memory debug specific
libxml2.debugMemory(1)

doc = libxml2.newDoc("1.0")
comment = doc.newDocComment("This is a generated document")
doc.addChild(comment)
pi = libxml2.newPI("test", "PI content")
doc.addChild(pi)
root = doc.newChild(None, "doc", None)
ns = root.newNs("http://example.com/doc", "my")
root.setNs(ns)
elem = root.newChild(None, "foo", "bar")
elem.setBase("http://example.com/imgs")
elem.setProp("img", "image.gif")
doc.saveFile("tmp.xml")
doc.freeDoc()

doc = libxml2.parseFile("tmp.xml")
comment = doc.children
if comment.type != "comment" or \
   comment.content != "This is a generated document":
   print "error rereading comment"
   sys.exit(1)
pi = comment.next
if pi.type != "pi" or pi.name != "test" or pi.content != "PI content":
   print "error rereading PI"
   sys.exit(1)
root = pi.next
if root.name != "doc":
   print "error rereading root"
   sys.exit(1)
ns = root.ns()
if ns.name != "my" or ns.content != "http://example.com/doc":
   print "error rereading namespace"
   sys.exit(1)
elem = root.children
if elem.name != "foo":
   print "error rereading elem"
   sys.exit(1)
if elem.getBase(None) != "http://example.com/imgs":
   print "error rereading base"
   sys.exit(1)
if elem.prop("img") != "image.gif":
   print "error rereading property"
   sys.exit(1)

doc.freeDoc()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
