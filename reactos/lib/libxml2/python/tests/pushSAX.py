#!/usr/bin/python -u
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

log = ""

class callback:
    def startDocument(self):
        global log
        log = log + "startDocument:"

    def endDocument(self):
        global log
        log = log + "endDocument:"

    def startElement(self, tag, attrs):
        global log
        log = log + "startElement %s %s:" % (tag, attrs)

    def endElement(self, tag):
        global log
        log = log + "endElement %s:" % (tag)

    def characters(self, data):
        global log
        log = log + "characters: %s:" % (data)

    def warning(self, msg):
        global log
        log = log + "warning: %s:" % (msg)

    def error(self, msg):
        global log
        log = log + "error: %s:" % (msg)

    def fatalError(self, msg):
        global log
        log = log + "fatalError: %s:" % (msg)

handler = callback()

ctxt = libxml2.createPushParser(handler, "<foo", 4, "test.xml")
chunk = " url='tst'>b"
ctxt.parseChunk(chunk, len(chunk), 0)
chunk = "ar</foo>"
ctxt.parseChunk(chunk, len(chunk), 1)
ctxt=None

reference = "startDocument:startElement foo {'url': 'tst'}:characters: bar:endElement foo:endDocument:"
if log != reference:
    print "Error got: %s" % log
    print "Exprected: %s" % reference
    sys.exit(1)

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
