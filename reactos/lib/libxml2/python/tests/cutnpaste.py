#!/usr/bin/python -u
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

#
# Testing XML document serialization
#
source = libxml2.parseDoc("""<?xml version="1.0"?>
<root xmlns:foo="http://example.org/foo"
      xmlns:bar="http://example.org/bar">
<include xmlns="http://example.org/include">
<fragment><foo:elem bar="tricky"/></fragment>
</include>
</root>
""")

target = libxml2.parseDoc("""<?xml version="1.0"?>
<root xmlns:foobar="http://example.org/bar"/>""")

fragment = source.xpathEval("//*[name()='fragment']")[0]
dest = target.getRootElement()

# do a cut and paste operation
fragment.unlinkNode()
dest.addChild(fragment)
# do the namespace fixup
dest.reconciliateNs(target)

# The source tree can be freed at that point
source.freeDoc()

# check the resulting tree
str = dest.serialize()
if str != """<root xmlns:foobar="http://example.org/bar" xmlns:default="http://example.org/include" xmlns:foo="http://example.org/foo"><default:fragment><foo:elem bar="tricky"/></default:fragment></root>""":
    print "reconciliateNs() failed"
    sys.exit(1)
target.freeDoc()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
