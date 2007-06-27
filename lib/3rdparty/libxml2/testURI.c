/*
 * testURI.c : a small tester program for XML input.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#include "libxml.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <libxml/xmlmemory.h>
#include <libxml/uri.h>
#include <libxml/globals.h>

static const char *base = NULL;
static int escape = 0;

static void handleURI(const char *str) {
    int ret;
    xmlURIPtr uri;
    xmlChar *res = NULL, *parsed = NULL;

    uri = xmlCreateURI();

    if (base == NULL) {
	ret = xmlParseURIReference(uri, str);
	if (ret != 0)
	    printf("%s : error %d\n", str, ret);
	else {
	    xmlNormalizeURIPath(uri->path);
	    if (escape != 0) {
		parsed = xmlSaveUri(uri);
		res = xmlURIEscape(parsed);
		printf("%s\n", (char *) res);

	    } else {
		xmlPrintURI(stdout, uri);
		printf("\n");
	    }
	}
    } else {
	res = xmlBuildURI((xmlChar *)str, (xmlChar *) base);
	if (res != NULL) {
	    printf("%s\n", (char *) res);
	}
	else
	    printf("::ERROR::\n");
    }
    if (res != NULL)
	xmlFree(res);
    if (parsed != NULL)
	xmlFree(parsed);
    xmlFreeURI(uri);
}

int main(int argc, char **argv) {
    int i, arg = 1;

    if ((argc > arg) && (argv[arg] != NULL) &&
	((!strcmp(argv[arg], "-base")) || (!strcmp(argv[arg], "--base")))) {
	arg++;
	base = argv[arg];
	if (base != NULL)
	    arg++;
    }
    if ((argc > arg) && (argv[arg] != NULL) &&
	((!strcmp(argv[arg], "-escape")) || (!strcmp(argv[arg], "--escape")))) {
	arg++;
	escape++;
    }
    if (argv[arg] == NULL) {
	char str[1024];

        while (1) {
	    /*
	     * read one line in string buffer.
	     */
	    if (fgets (&str[0], sizeof (str) - 1, stdin) == NULL)
	       break;

	    /*
	     * remove the ending spaces
	     */
	    i = strlen(str);
	    while ((i > 0) &&
		   ((str[i - 1] == '\n') || (str[i - 1] == '\r') ||
		    (str[i - 1] == ' ') || (str[i - 1] == '\t'))) {
		i--;
		str[i] = 0;
	    }
	    handleURI(str);
        }
    } else {
	while (argv[arg] != NULL) {
	    handleURI(argv[arg]);
	    arg++;
	}
    }
    xmlMemoryDump();
    return(0);
}
