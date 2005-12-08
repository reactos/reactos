#!/usr/bin/python -u
import sys
import libxml2
import StringIO

def testSimpleBufferWrites():
    f = StringIO.StringIO()
    buf = libxml2.createOutputBuffer(f, "ISO-8859-1")
    buf.write(3, "foo")
    buf.writeString("bar")
    buf.close()
    
    if f.getvalue() != "foobar":
        print "Failed to save to StringIO"
        sys.exit(1)

def testSaveDocToBuffer():
    """
    Regression test for bug #154294.
    """
    input = '<foo>Hello</foo>'
    expected = '''\
<?xml version="1.0" encoding="UTF-8"?>
<foo>Hello</foo>
'''
    f = StringIO.StringIO()
    buf = libxml2.createOutputBuffer(f, 'UTF-8')
    doc = libxml2.parseDoc(input)
    doc.saveFileTo(buf, 'UTF-8')
    doc.freeDoc()
    if f.getvalue() != expected:
        print 'xmlDoc.saveFileTo() call failed.'
        print '     got: %s' % repr(f.getvalue())
        print 'expected: %s' % repr(expected)
        sys.exit(1)

def testSaveFormattedDocToBuffer():
    input = '<outer><inner>Some text</inner><inner/></outer>'
    # The formatted and non-formatted versions of the output.
    expected = ('''\
<?xml version="1.0" encoding="UTF-8"?>
<outer><inner>Some text</inner><inner/></outer>
''', '''\
<?xml version="1.0" encoding="UTF-8"?>
<outer>
  <inner>Some text</inner>
  <inner/>
</outer>
''')
    doc = libxml2.parseDoc(input)
    for i in (0, 1):
        f = StringIO.StringIO()
        buf = libxml2.createOutputBuffer(f, 'UTF-8')
        doc.saveFormatFileTo(buf, 'UTF-8', i)
        if f.getvalue() != expected[i]:
            print 'xmlDoc.saveFormatFileTo() call failed.'
            print '     got: %s' % repr(f.getvalue())
            print 'expected: %s' % repr(expected[i])
            sys.exit(1)
    doc.freeDoc()

def testSaveIntoOutputBuffer():
    """
    Similar to the previous two tests, except this time we invoke the save
    methods on the output buffer object and pass in an XML node object.
    """
    input = '<foo>Hello</foo>'
    expected = '''\
<?xml version="1.0" encoding="UTF-8"?>
<foo>Hello</foo>
'''
    f = StringIO.StringIO()
    doc = libxml2.parseDoc(input)
    buf = libxml2.createOutputBuffer(f, 'UTF-8')
    buf.saveFileTo(doc, 'UTF-8')
    if f.getvalue() != expected:
        print 'outputBuffer.saveFileTo() call failed.'
        print '     got: %s' % repr(f.getvalue())
        print 'expected: %s' % repr(expected)
        sys.exit(1)
    f = StringIO.StringIO()
    buf = libxml2.createOutputBuffer(f, 'UTF-8')
    buf.saveFormatFileTo(doc, 'UTF-8', 1)
    if f.getvalue() != expected:
        print 'outputBuffer.saveFormatFileTo() call failed.'
        print '     got: %s' % repr(f.getvalue())
        print 'expected: %s' % repr(expected)
        sys.exit(1)
    doc.freeDoc()

if __name__ == '__main__':
    # Memory debug specific
    libxml2.debugMemory(1)

    testSimpleBufferWrites()
    testSaveDocToBuffer()
    testSaveFormattedDocToBuffer()
    testSaveIntoOutputBuffer()

    libxml2.cleanupParser()
    if libxml2.debugMemory(1) == 0:
        print "OK"
    else:
        print "Memory leak %d bytes" % (libxml2.debugMemory(1))
        libxml2.dumpMemory()
