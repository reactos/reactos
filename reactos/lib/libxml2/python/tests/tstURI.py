#!/usr/bin/python -u
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

uri = libxml2.parseURI("http://example.org:8088/foo/bar?query=simple#fragid")
if uri.scheme() != 'http':
    print "Error parsing URI: wrong scheme"
    sys.exit(1)
if uri.server() != 'example.org':
    print "Error parsing URI: wrong server"
    sys.exit(1)
if uri.port() != 8088:
    print "Error parsing URI: wrong port"
    sys.exit(1)
if uri.path() != '/foo/bar':
    print "Error parsing URI: wrong path"
    sys.exit(1)
if uri.query() != 'query=simple':
    print "Error parsing URI: wrong query"
    sys.exit(1)
if uri.fragment() != 'fragid':
    print "Error parsing URI: wrong query"
    sys.exit(1)
uri.setScheme("https")
uri.setPort(223)
uri.setFragment(None)
result=uri.saveUri()
if result != "https://example.org:223/foo/bar?query=simple":
    print "Error modifying or saving the URI"
uri = None

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
