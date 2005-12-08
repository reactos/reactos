/**
 * section: Parsing
 * synopsis: Parse an XML document in memory to a tree and free it
 * purpose: Demonstrate the use of xmlReadMemory() to read an XML file
 *          into a tree and and xmlFreeDoc() to free the resulting tree
 * usage: parse3
 * test: parse3
 * author: Daniel Veillard
 * copy: see Copyright for the status of this software.
 */

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

static const char *document = "<doc/>";

/**
 * example3Func:
 * @content: the content of the document
 * @length: the length in bytes
 *
 * Parse the in memory document and free the resulting tree
 */
static void
example3Func(const char *content, int length) {
    xmlDocPtr doc; /* the resulting document tree */

    /*
     * The document being in memory, it have no base per RFC 2396,
     * and the "noname.xml" argument will serve as its base.
     */
    doc = xmlReadMemory(content, length, "noname.xml", NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse document\n");
	return;
    }
    xmlFreeDoc(doc);
}

int main(void) {
    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    example3Func(document, 6);

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
