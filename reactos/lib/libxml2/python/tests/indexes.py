#!/usr/bin/python -u
# -*- coding: ISO-8859-1 -*-
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

ctxt = None

class callback:
    def __init__(self, startd, starte, ende, delta, endd):
        self.startd = startd
        self.starte = starte
        self.ende = ende
        self.endd = endd
        self.delta = delta
        self.count = 0

    def startDocument(self):
        global ctxt
        if ctxt.byteConsumed() != self.startd:
            print "document start at wrong index: %d expecting %d\n" % (
                  ctxt.byteConsumed(), self.startd)
            sys.exit(1)

    def endDocument(self):
        global ctxt
        expect = self.ende + self.delta * (self.count - 1) + self.endd
        if ctxt.byteConsumed() != expect:
            print "document end at wrong index: %d expecting %d\n" % (
                  ctxt.byteConsumed(), expect)
            sys.exit(1)

    def startElement(self, tag, attrs):
        global ctxt
        if tag == "bar1":
            expect = self.starte + self.delta * self.count
            if ctxt.byteConsumed() != expect:
                print "element start at wrong index: %d expecting %d\n" % (
                   ctxt.byteConsumed(), expect)
                sys.exit(1)
            

    def endElement(self, tag):
        global ctxt
        if tag == "bar1":
            expect = self.ende + self.delta * self.count
            if ctxt.byteConsumed() != expect:
                print "element end at wrong index: %d expecting %d\n" % (
                      ctxt.byteConsumed(), expect)
                sys.exit(1)
            self.count = self.count + 1

    def characters(self, data):
        pass

#
# First run a pure UTF-8 test
#
handler = callback(0, 13, 27, 198, 183)
ctxt = libxml2.createPushParser(handler, "<foo>\n", 6, "test.xml")
chunk = """  <bar1>chars1</bar1>
  <bar2>chars2</bar2>
  <bar3>chars3</bar3>
  <bar4>chars4</bar4>
  <bar5>chars5</bar5>
  <bar6>&lt;s6</bar6>
  <bar7>chars7</bar7>
  <bar8>&#38;8</bar8>
  <bar9>chars9</bar9>
"""
i = 0
while i < 10000:
    ctxt.parseChunk(chunk, len(chunk), 0)
    i = i + 1
chunk = "</foo>"
ctxt.parseChunk(chunk, len(chunk), 1)
ctxt=None

#
# Then run a test relying on ISO-Latin-1
#
handler = callback(43, 57, 71, 198, 183)
chunk="""<?xml version="1.0" encoding="ISO-8859-1"?>
<foo>
"""
ctxt = libxml2.createPushParser(handler, chunk, len(chunk), "test.xml")
chunk = """  <bar1>chars1</bar1>
  <bar2>chars2</bar2>
  <bar3>chars3</bar3>
  <bar4>chàrs4</bar4>
  <bar5>chars5</bar5>
  <bar6>&lt;s6</bar6>
  <bar7>chars7</bar7>
  <bar8>&#38;8</bar8>
  <bar9>très 9</bar9>
"""
i = 0
while i < 10000:
    ctxt.parseChunk(chunk, len(chunk), 0)
    i = i + 1
chunk = "</foo>"
ctxt.parseChunk(chunk, len(chunk), 1)
ctxt=None

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
