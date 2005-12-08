#!/usr/bin/python -u
#
# this tests the entities substitutions with the XmlTextReader interface
#
import sys
import StringIO
import libxml2

schema="""<element name="foo" xmlns="http://relaxng.org/ns/structure/1.0"
         datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
  <oneOrMore>
    <element name="label">
      <text/>
    </element>
    <optional>
      <element name="opt">
        <empty/>
      </element>
    </optional>
    <element name="item">
      <data type="byte"/>
    </element>
  </oneOrMore>
</element>
"""
# Memory debug specific
libxml2.debugMemory(1)

#
# Parse the Relax NG Schemas
# 
rngp = libxml2.relaxNGNewMemParserCtxt(schema, len(schema))
rngs = rngp.relaxNGParse()
del rngp

#
# Parse and validate the correct document
#
docstr="""<foo>
<label>some text</label>
<item>100</item>
</foo>"""

f = StringIO.StringIO(docstr)
input = libxml2.inputBuffer(f)
reader = input.newTextReader("correct")
reader.RelaxNGSetSchema(rngs)
ret = reader.Read()
while ret == 1:
    ret = reader.Read()

if ret != 0:
    print "Error parsing the document"
    sys.exit(1)

if reader.IsValid() != 1:
    print "Document failed to validate"
    sys.exit(1)

#
# Parse and validate the incorrect document
#
docstr="""<foo>
<label>some text</label>
<item>1000</item>
</foo>"""

err=""
# RNG errors are not as good as before , TODO
#expect="""RNG validity error: file error line 3 element text
#Type byte doesn't allow value '1000'
#RNG validity error: file error line 3 element text
#Error validating datatype byte
#RNG validity error: file error line 3 element text
#Element item failed to validate content
#"""
expect="""Type byte doesn't allow value '1000'
Error validating datatype byte
Element item failed to validate content
"""

def callback(ctx, str):
    global err
    err = err + "%s" % (str)
libxml2.registerErrorHandler(callback, "")

f = StringIO.StringIO(docstr)
input = libxml2.inputBuffer(f)
reader = input.newTextReader("error")
reader.RelaxNGSetSchema(rngs)
ret = reader.Read()
while ret == 1:
    ret = reader.Read()

if ret != 0:
    print "Error parsing the document"
    sys.exit(1)

if reader.IsValid() != 0:
    print "Document failed to detect the validation error"
    sys.exit(1)

if err != expect:
    print "Did not get the expected error message:"
    print err
    sys.exit(1)

#
# cleanup
#
del f
del input
del reader
del rngs
libxml2.relaxNGCleanupTypes()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
