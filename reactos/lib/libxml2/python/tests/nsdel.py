#!/usr/bin/python -u
#
# this test exercise the XPath basic engine, parser, etc, and
# allows to detect memory leaks
#
import sys
import libxml2

instance="""<?xml version="1.0"?>
<tag xmlns:foo='urn:foo' xmlns:bar='urn:bar' xmlns:baz='urn:baz' />"""

def namespaceDefs(node):
    n = node.nsDefs()
    while n:
        yield n
        n = n.next

def checkNamespaceDefs(node, count):
    nsList = list(namespaceDefs(node))
    #print nsList
    if len(nsList) != count :
        raise Exception, "Error: saw %d namespace declarations.  Expected %d" % (len(nsList), count)
    
# Memory debug specific
libxml2.debugMemory(1)

# Remove single namespace
doc = libxml2.parseDoc(instance)
node = doc.getRootElement()
checkNamespaceDefs(node, 3)
ns = node.removeNsDef('urn:bar')
checkNamespaceDefs(node, 2)
ns.freeNsList()
doc.freeDoc()

# Remove all namespaces
doc = libxml2.parseDoc(instance)
node = doc.getRootElement()
checkNamespaceDefs(node, 3)
ns = node.removeNsDef(None)
checkNamespaceDefs(node, 0)
ns.freeNsList()
doc.freeDoc()

# Remove a namespace refered to by a child
doc = libxml2.newDoc("1.0")
root = doc.newChild(None, "root", None)
namespace = root.newNs("http://example.com/sample", "s")
child = root.newChild(namespace, "child", None)
root.removeNsDef("http://example.com/sample")
doc.reconciliateNs(root)
namespace.freeNsList()
doc.serialize() # This should not segfault
doc.freeDoc()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
