/**
 * section: Parsing
 * synopsis: Parse and validate an XML file to a tree and free the result
 * purpose: Create a parser context for an XML file, then parse and validate
 *          the file, creating a tree, check the validation result
 *          and xmlFreeDoc() to free the resulting tree.
 * usage: parse2 test2.xml
 * test: parse2 test2.xml
 * author: Daniel Veillard
 * copy: see Copyright for the status of this software.
 */

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

/**
 * exampleFunc:
 * @filename: a filename or an URL
 *
 * Parse and validate the resource and free the resulting tree
 */
static void
exampleFunc(const char *filename) {
    xmlParserCtxtPtr ctxt; /* the parser context */
    xmlDocPtr doc; /* the resulting document tree */

    /* create a parser context */
    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL) {
        fprintf(stderr, "Failed to allocate parser context\n");
	return;
    }
    /* parse the file, activating the DTD validation option */
    doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_DTDVALID);
    /* check if parsing suceeded */
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse %s\n", filename);
    } else {
	/* check if validation suceeded */
        if (ctxt->valid == 0)
	    fprintf(stderr, "Failed to validate %s\n", filename);
	/* free up the resulting document */
	xmlFreeDoc(doc);
    }
    /* free up the parser context */
    xmlFreeParserCtxt(ctxt);
}

int main(int argc, char **argv) {
    if (argc != 2)
        return(1);

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    exampleFunc(argv[1]);

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
    return(0);
}
