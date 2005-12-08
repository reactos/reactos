/**
 * section: xmlReader
 * synopsis: Show how to extract subdocuments with xmlReader
 * purpose: Demonstrate the use of xmlTextReaderPreservePattern() 
 *          to parse an XML file with the xmlReader while collecting
 *          only some subparts of the document.
 *          (Note that the XMLReader functions require libxml2 version later
 *          than 2.6.)
 * usage: reader3
 * test: reader3 > reader3.tmp ; diff reader3.tmp reader3.res ; rm reader3.tmp
 * author: Daniel Veillard
 * copy: see Copyright for the status of this software.
 */

#include <stdio.h>
#include <libxml/xmlreader.h>

#if defined(LIBXML_READER_ENABLED) && defined(LIBXML_PATTERN_ENABLED)

/**
 * streamFile:
 * @filename: the file name to parse
 *
 * Parse and print information about an XML file.
 *
 * Returns the resulting doc with just the elements preserved.
 */
static xmlDocPtr
extractFile(const char *filename, const xmlChar *pattern) {
    xmlDocPtr doc;
    xmlTextReaderPtr reader;
    int ret;

    /*
     * build an xmlReader for that file
     */
    reader = xmlReaderForFile(filename, NULL, 0);
    if (reader != NULL) {
        /*
	 * add the pattern to preserve
	 */
        if (xmlTextReaderPreservePattern(reader, pattern, NULL) < 0) {
            fprintf(stderr, "%s : failed add preserve pattern %s\n",
	            filename, (const char *) pattern);
	}
	/*
	 * Parse and traverse the tree, collecting the nodes in the process
	 */
        ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            ret = xmlTextReaderRead(reader);
        }
        if (ret != 0) {
            fprintf(stderr, "%s : failed to parse\n", filename);
	    xmlFreeTextReader(reader);
	    return(NULL);
        }
	/*
	 * get the resulting nodes
	 */
	doc = xmlTextReaderCurrentDoc(reader);
	/*
	 * Free up the reader
	 */
        xmlFreeTextReader(reader);
    } else {
        fprintf(stderr, "Unable to open %s\n", filename);
	return(NULL);
    }
    return(doc);
}

int main(int argc, char **argv) {
    const char *filename = "test3.xml";
    const char *pattern = "preserved";
    xmlDocPtr doc;

    if (argc == 3) {
        filename = argv[1];
	pattern = argv[2];
    }

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    doc = extractFile(filename, (const xmlChar *) pattern);
    if (doc != NULL) {
        /*
	 * ouptut the result.
	 */
        xmlDocDump(stdout, doc);
	/*
	 * don't forget to free up the doc
	 */
	xmlFreeDoc(doc);
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

#else
int main(void) {
    fprintf(stderr, "Reader or Pattern support not compiled in\n");
    exit(1);
}
#endif
