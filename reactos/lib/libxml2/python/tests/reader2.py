#!/usr/bin/python -u
#
# this tests the DTD validation with the XmlTextReader interface
#
import sys
import glob
import string
import StringIO
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

err=""
expect="""../../test/valid/rss.xml:177: element rss: validity error : Element rss does not carry attribute version
</rss>
      ^
../../test/valid/xlink.xml:450: element termdef: validity error : ID dt-arc already defined
	<p><termdef id="dt-arc" term="Arc">An <ter
	                                  ^
../../test/valid/xlink.xml:530: validity error : attribute def line 199 references an unknown ID "dt-xlg"

^
"""
def callback(ctx, str):
    global err
    err = err + "%s" % (str)
libxml2.registerErrorHandler(callback, "")

valid_files = glob.glob("../../test/valid/*.x*")
valid_files.sort()
for file in valid_files:
    if string.find(file, "t8") != -1:
        continue
    reader = libxml2.newTextReaderFilename(file)
    #print "%s:" % (file)
    reader.SetParserProp(libxml2.PARSER_VALIDATE, 1)
    ret = reader.Read()
    while ret == 1:
        ret = reader.Read()
    if ret != 0:
        print "Error parsing and validating %s" % (file)
	#sys.exit(1)

if err != expect:
    print err

#
# another separate test based on Stephane Bidoul one
#
s = """
<!DOCTYPE test [
<!ELEMENT test (x,b)>
<!ELEMENT x (c)>
<!ELEMENT b (#PCDATA)>
<!ELEMENT c (#PCDATA)>
<!ENTITY x "<x><c>xxx</c></x>">
]>
<test>
    &x;
    <b>bbb</b>
</test>
"""
expect="""10,test
1,test
14,#text
1,x
1,c
3,#text
15,c
15,x
14,#text
1,b
3,#text
15,b
14,#text
15,test
"""
res=""
err=""

input = libxml2.inputBuffer(StringIO.StringIO(s))
reader = input.newTextReader("test2")
reader.SetParserProp(libxml2.PARSER_LOADDTD,1)
reader.SetParserProp(libxml2.PARSER_DEFAULTATTRS,1)
reader.SetParserProp(libxml2.PARSER_SUBST_ENTITIES,1)
reader.SetParserProp(libxml2.PARSER_VALIDATE,1)
while reader.Read() == 1:
    res = res + "%s,%s\n" % (reader.NodeType(),reader.Name())

if res != expect:
    print "test2 failed: unexpected output"
    print res
    sys.exit(1)
if err != "":
    print "test2 failed: validation error found"
    print err
    sys.exit(1)

#
# Another test for external entity parsing and validation
#

s = """<!DOCTYPE test [
<!ELEMENT test (x)>
<!ELEMENT x (#PCDATA)>
<!ENTITY e SYSTEM "tst.ent">
]>
<test>
  &e;
</test>
"""
tst_ent = """<x>hello</x>"""
expect="""10 test
1 test
14 #text
1 x
3 #text
15 x
14 #text
15 test
"""
res=""

def myResolver(URL, ID, ctxt):
    if URL == "tst.ent":
        return(StringIO.StringIO(tst_ent))
    return None

libxml2.setEntityLoader(myResolver)

input = libxml2.inputBuffer(StringIO.StringIO(s))
reader = input.newTextReader("test3")
reader.SetParserProp(libxml2.PARSER_LOADDTD,1)
reader.SetParserProp(libxml2.PARSER_DEFAULTATTRS,1)
reader.SetParserProp(libxml2.PARSER_SUBST_ENTITIES,1)
reader.SetParserProp(libxml2.PARSER_VALIDATE,1)
while reader.Read() == 1:
    res = res + "%s %s\n" % (reader.NodeType(),reader.Name())

if res != expect:
    print "test3 failed: unexpected output"
    print res
    sys.exit(1)
if err != "":
    print "test3 failed: validation error found"
    print err
    sys.exit(1)

#
# Another test for recursive entity parsing, validation, and replacement of
# entities, making sure the entity ref node doesn't show up in that case
#

s = """<!DOCTYPE test [
<!ELEMENT test (x, x)>
<!ELEMENT x (y)>
<!ELEMENT y (#PCDATA)>
<!ENTITY x "<x>&y;</x>">
<!ENTITY y "<y>yyy</y>">
]>
<test>
  &x;
  &x;
</test>"""
expect="""10 test 0
1 test 0
14 #text 1
1 x 1
1 y 2
3 #text 3
15 y 2
15 x 1
14 #text 1
1 x 1
1 y 2
3 #text 3
15 y 2
15 x 1
14 #text 1
15 test 0
"""
res=""
err=""

input = libxml2.inputBuffer(StringIO.StringIO(s))
reader = input.newTextReader("test4")
reader.SetParserProp(libxml2.PARSER_LOADDTD,1)
reader.SetParserProp(libxml2.PARSER_DEFAULTATTRS,1)
reader.SetParserProp(libxml2.PARSER_SUBST_ENTITIES,1)
reader.SetParserProp(libxml2.PARSER_VALIDATE,1)
while reader.Read() == 1:
    res = res + "%s %s %d\n" % (reader.NodeType(),reader.Name(),reader.Depth())

if res != expect:
    print "test4 failed: unexpected output"
    print res
    sys.exit(1)
if err != "":
    print "test4 failed: validation error found"
    print err
    sys.exit(1)

#
# The same test but without entity substitution this time
#

s = """<!DOCTYPE test [
<!ELEMENT test (x, x)>
<!ELEMENT x (y)>
<!ELEMENT y (#PCDATA)>
<!ENTITY x "<x>&y;</x>">
<!ENTITY y "<y>yyy</y>">
]>
<test>
  &x;
  &x;
</test>"""
expect="""10 test 0
1 test 0
14 #text 1
5 x 1
14 #text 1
5 x 1
14 #text 1
15 test 0
"""
res=""
err=""

input = libxml2.inputBuffer(StringIO.StringIO(s))
reader = input.newTextReader("test5")
reader.SetParserProp(libxml2.PARSER_VALIDATE,1)
while reader.Read() == 1:
    res = res + "%s %s %d\n" % (reader.NodeType(),reader.Name(),reader.Depth())

if res != expect:
    print "test5 failed: unexpected output"
    print res
if err != "":
    print "test5 failed: validation error found"
    print err

#
# cleanup
#
del input
del reader

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
