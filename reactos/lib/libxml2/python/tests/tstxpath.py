#!/usr/bin/python -u
import sys
import libxml2

#memory debug specific
libxml2.debugMemory(1)

called = ""

def foo(ctx, x):
    global called

    #
    # test that access to the XPath evaluation contexts
    #
    pctxt = libxml2.xpathParserContext(_obj=ctx)
    ctxt = pctxt.context()
    called = ctxt.function()
    return x + 1

def bar(ctxt, x):
    return "%d" % (x + 2)

doc = libxml2.parseFile("tst.xml")
ctxt = doc.xpathNewContext()
res = ctxt.xpathEval("//*")
if len(res) != 2:
    print "xpath query: wrong node set size"
    sys.exit(1)
if res[0].name != "doc" or res[1].name != "foo":
    print "xpath query: wrong node set value"
    sys.exit(1)
libxml2.registerXPathFunction(ctxt._o, "foo", None, foo)
libxml2.registerXPathFunction(ctxt._o, "bar", None, bar)
i = 10000
while i > 0:
    res = ctxt.xpathEval("foo(1)")
    if res != 2:
        print "xpath extension failure"
        sys.exit(1)
    i = i - 1
i = 10000
while i > 0:
    res = ctxt.xpathEval("bar(1)")
    if res != "3":
        print "xpath extension failure got %s expecting '3'"
        sys.exit(1)
    i = i - 1
doc.freeDoc()
ctxt.xpathFreeContext()

if called != "foo":
    print "xpath function: failed to access the context"
    print "xpath function: %s" % (called)
    sys.exit(1)

#memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
