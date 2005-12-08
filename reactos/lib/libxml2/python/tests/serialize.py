#!/usr/bin/python -u
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

#
# Testing XML document serialization
#
doc = libxml2.parseDoc("""<root><foo>hello</foo></root>""")
str = doc.serialize()
if str != """<?xml version="1.0"?>
<root><foo>hello</foo></root>
""":
   print "error serializing XML document 1"
   sys.exit(1)
str = doc.serialize("iso-8859-1")
if str != """<?xml version="1.0" encoding="iso-8859-1"?>
<root><foo>hello</foo></root>
""":
   print "error serializing XML document 2"
   sys.exit(1)
str = doc.serialize(format=1)
if str != """<?xml version="1.0"?>
<root>
  <foo>hello</foo>
</root>
""":
   print "error serializing XML document 3"
   sys.exit(1)
str = doc.serialize("iso-8859-1", 1)
if str != """<?xml version="1.0" encoding="iso-8859-1"?>
<root>
  <foo>hello</foo>
</root>
""":
   print "error serializing XML document 4"
   sys.exit(1)

#
# Test serializing a subnode
#
root = doc.getRootElement()
str = root.serialize()
if str != """<root><foo>hello</foo></root>""":
   print "error serializing XML root 1"
   sys.exit(1)
str = root.serialize("iso-8859-1")
if str != """<root><foo>hello</foo></root>""":
   print "error serializing XML root 2"
   sys.exit(1)
str = root.serialize(format=1)
if str != """<root>
  <foo>hello</foo>
</root>""":
   print "error serializing XML root 3"
   sys.exit(1)
str = root.serialize("iso-8859-1", 1)
if str != """<root>
  <foo>hello</foo>
</root>""":
   print "error serializing XML root 4"
   sys.exit(1)
doc.freeDoc()

#
# Testing HTML document serialization
#
doc = libxml2.htmlParseDoc("""<html><head><title>Hello</title><body><p>hello</body></html>""", None)
str = doc.serialize()
if str != """<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">
<html><head><title>Hello</title></head><body><p>hello</p></body></html>
""":
   print "error serializing HTML document 1"
   sys.exit(1)
str = doc.serialize("ISO-8859-1")
if str != """<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">
<html><head><meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1"><title>Hello</title></head><body><p>hello</p></body></html>
""":
   print "error serializing HTML document 2"
   sys.exit(1)
str = doc.serialize(format=1)
if str != """<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<title>Hello</title>
</head>
<body><p>hello</p></body>
</html>
""":
   print "error serializing HTML document 3"
   sys.exit(1)
str = doc.serialize("iso-8859-1", 1)
if str != """<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<title>Hello</title>
</head>
<body><p>hello</p></body>
</html>
""":
   print "error serializing HTML document 4"
   sys.exit(1)

#
# Test serializing a subnode
#
doc.htmlSetMetaEncoding(None)
root = doc.getRootElement()
str = root.serialize()
if str != """<html><head><title>Hello</title></head><body><p>hello</p></body></html>""":
   print "error serializing HTML root 1"
   sys.exit(1)
str = root.serialize("ISO-8859-1")
if str != """<html><head><meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1"><title>Hello</title></head><body><p>hello</p></body></html>""":
   print "error serializing HTML root 2"
   sys.exit(1)
str = root.serialize(format=1)
if str != """<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<title>Hello</title>
</head>
<body><p>hello</p></body>
</html>""":
   print "error serializing HTML root 3"
   sys.exit(1)
str = root.serialize("iso-8859-1", 1)
if str != """<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<title>Hello</title>
</head>
<body><p>hello</p></body>
</html>""":
   print "error serializing HTML root 4"
   sys.exit(1)

doc.freeDoc()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
