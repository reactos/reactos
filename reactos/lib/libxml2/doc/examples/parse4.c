/**
 * section: Parsing
 * synopsis: Parse an XML document chunk by chunk to a tree and free it
 * purpose: Demonstrate the use of xmlCreatePushParserCtxt() and
 *          xmlParseChunk() to read an XML file progressively
 *          into a tree and and xmlFreeDoc() to free the resulting tree
 * usage: parse4 test3.xml
 * test: parse4 test3.xml
 * author: Daniel Veillard
 * copy: see Copyright for the status of this software.
 */

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef LIBXML_PUSH_ENABLED
static FILE *desc;

/**
 * readPacket:
 * @mem: array to store the packet
 * @size: the packet size
 *
 * read at most @size bytes from the document and store it in @mem
 *
 * Returns the number of bytes read
 */
static int
readPacket(char *mem, int size) {
    int res;

    res = fread(mem, 1, size, desc);
    return(res);
}

/**
 * example4Func:
 * @filename: a filename or an URL
 *
 * Parse the resource and free the resulting tree
 */
static void
example4Func(const char *filename) {
    xmlParserCtxtPtr ctxt;
    char chars[4];
    xmlDocPtr doc; /* the resulting document tree */
    int res;

    /*
     * Read a few first byte to check the input used for the
     * encoding detection at the parser level.
     */
    res = readPacket(chars, 4);
    if (res <= 0) {
        fprintf(stderr, "Failed to parse %s\n", filename);
	return;
    }

    /*
     * Create a progressive parsing context, the 2 first arguments
     * are not used since we want to build a tree and not use a SAX
     * parsing interface. We also pass the first bytes of the document
     * to allow encoding detection when creating the parser but this
     * is optional.
     */
    ctxt = xmlCreatePushParserCtxt(NULL, NULL,
                                   chars, res, filename);
    if (ctxt == NULL) {
        fprintf(stderr, "Failed to create parser context !\n");
	return;
    }

    /*
     * loop on the input getting the document data, of course 4 bytes
     * at a time is not realistic but allows to verify testing on small
     * documents.
     */
    while ((res = readPacket(chars, 4)) > 0) {
        xmlParseChunk(ctxt, chars, res, 0);
    }

    /*
     * there is no more input, indicate the parsing is finished.
     */
    xmlParseChunk(ctxt, chars, 0, 1);

    /*
     * collect the document back and if it was wellformed
     * and destroy the parser context.
     */
    doc = ctxt->myDoc;
    res = ctxt->wellFormed;
    xmlFreeParserCtxt(ctxt);

    if (!res) {
        fprintf(stderr, "Failed to parse %s\n", filename);
    }

    /*
     * since we don't use the document, destroy it now.
     */
    xmlFreeDoc(doc);
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

    /*
     * simulate a progressive parsing using the input file.
     */
    desc = fopen(argv[1], "rb");
    if (desc != NULL) {
	example4Func(argv[1]);
	fclose(desc);
    } else {
        fprintf(stderr, "Failed to parse %s\n", argv[1]);
    }

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
#else /* ! LIBXML_PUSH_ENABLED */
int main(int argc, char **argv) {
    fprintf(stderr, "Library not compiled with push parser support\n");
    return(1);
}
#endif
