#!/usr/bin/python -u
import libxml2
import sys

ARG = 'test string'

class ErrorHandler:

    def __init__(self):
        self.errors = []

    def handler(self, msg, data):
        if data != ARG:
            raise Exception, "Error handler did not receive correct argument"
        self.errors.append(msg)

# Memory debug specific
libxml2.debugMemory(1)

schema="""<?xml version="1.0" encoding="iso-8859-1"?>
<schema xmlns = "http://www.w3.org/2001/XMLSchema">
	<element name = "Customer">
		<complexType>
			<sequence>
				<element name = "FirstName" type = "string" />
				<element name = "MiddleInitial" type = "string" />
				<element name = "LastName" type = "string" />
			</sequence>
			<attribute name = "customerID" type = "integer" />
		</complexType>
	</element>
</schema>"""

valid="""<?xml version="1.0" encoding="iso-8859-1"?>
<Customer customerID = "24332">
	<FirstName>Raymond</FirstName>
	<MiddleInitial>G</MiddleInitial>
	<LastName>Bayliss</LastName>
</Customer>	
"""

invalid="""<?xml version="1.0" encoding="iso-8859-1"?>
<Customer customerID = "24332">
	<MiddleInitial>G</MiddleInitial>
	<LastName>Bayliss</LastName>
</Customer>	
"""

e = ErrorHandler()
ctxt_parser = libxml2.schemaNewMemParserCtxt(schema, len(schema))
ctxt_schema = ctxt_parser.schemaParse()
ctxt_valid  = ctxt_schema.schemaNewValidCtxt()
ctxt_valid.setValidityErrorHandler(e.handler, e.handler, ARG)

# Test valid document
doc = libxml2.parseDoc(valid)
ret = doc.schemaValidateDoc(ctxt_valid)
if ret != 0 or e.errors:
    print "error doing schema validation"
    sys.exit(1)
doc.freeDoc()

# Test invalid document
doc = libxml2.parseDoc(invalid)
ret = doc.schemaValidateDoc(ctxt_valid)
if ret == 0 or not e.errors:
    print "Error: document supposer to be schema invalid"
    sys.exit(1)
doc.freeDoc()

del ctxt_parser
del ctxt_schema
del ctxt_valid
libxml2.schemaCleanupTypes()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()

