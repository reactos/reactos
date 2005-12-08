#!/usr/bin/python -u
import sys
import libxml2

#memory debug specific
libxml2.debugMemory(1)

#
# A document hosting the nodes returned from the extension function
#
mydoc = libxml2.newDoc("1.0")

def foo(ctx, str):
    global mydoc

    #
    # test returning a node set works as expected
    #
    parent = mydoc.newDocNode(None, 'p', None)
    mydoc.addChild(parent)
    node = mydoc.newDocText(str)
    parent.addChild(node)
    return [parent]

doc = libxml2.parseFile("tst.xml")
ctxt = doc.xpathNewContext()
libxml2.registerXPathFunction(ctxt._o, "foo", None, foo)
res = ctxt.xpathEval("foo('hello')")
if type(res) != type([]):
    print "Failed to return a nodeset"
    sys.exit(1)
if len(res) != 1:
    print "Unexpected nodeset size"
    sys.exit(1)
node = res[0]
if node.name != 'p':
    print "Unexpected nodeset element result"
    sys.exit(1)
node = node.children
if node.type != 'text':
    print "Unexpected nodeset element children type"
    sys.exit(1)
if node.content != 'hello':
    print "Unexpected nodeset element children content"
    sys.exit(1)

doc.freeDoc()
mydoc.freeDoc()
ctxt.xpathFreeContext()

#memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
