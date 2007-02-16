/*
 * testRegexp.c: simple module for testing regular expressions
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <veillard@redhat.com>
 */

#include <string.h>
#include "libxml.h"
#ifdef LIBXML_REGEXP_ENABLED
#include <libxml/tree.h>
#include <libxml/xmlregexp.h>

int repeat = 0;
int debug = 0;

static void testRegexp(xmlRegexpPtr comp, const char *value) {
    int ret;

    ret = xmlRegexpExec(comp, (const xmlChar *) value);
    if (ret == 1)
	printf("%s: Ok\n", value);
    else if (ret == 0)
	printf("%s: Fail\n", value);
    else
	printf("%s: Error: %d\n", value, ret);
    if (repeat) {
	int j;
	for (j = 0;j < 999999;j++)
	    xmlRegexpExec(comp, (const xmlChar *) value);
    }
}

static void
testRegexpFile(const char *filename) {
    xmlRegexpPtr comp = NULL;
    FILE *input;
    char expression[5000];
    int len;

    input = fopen(filename, "r");
    if (input == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"Cannot open %s for reading\n", filename);
	return;
    }
    while (fgets(expression, 4500, input) != NULL) {
	len = strlen(expression);
	len--;
	while ((len >= 0) && 
	       ((expression[len] == '\n') || (expression[len] == '\t') ||
		(expression[len] == '\r') || (expression[len] == ' '))) len--;
	expression[len + 1] = 0;      
	if (len >= 0) {
	    if (expression[0] == '#')
		continue;
	    if ((expression[0] == '=') && (expression[1] == '>')) {
		char *pattern = &expression[2];

		if (comp != NULL) {
		    xmlRegFreeRegexp(comp);
		    comp = NULL;
		}
		printf("Regexp: %s\n", pattern) ;
		comp = xmlRegexpCompile((const xmlChar *) pattern);
		if (comp == NULL) {
		    printf("   failed to compile\n");
		    break;
		}
	    } else if (comp == NULL) {
		printf("Regexp: %s\n", expression) ;
		comp = xmlRegexpCompile((const xmlChar *) expression);
		if (comp == NULL) {
		    printf("   failed to compile\n");
		    break;
		}
	    } else if (comp != NULL) {
		testRegexp(comp, expression);
	    }
	}
    }
    fclose(input);
    if (comp != NULL)
	xmlRegFreeRegexp(comp);
}


static void usage(const char *name) {
    fprintf(stderr, "Usage: %s\n", name);
}

int main(int argc, char **argv) {
    xmlRegexpPtr comp = NULL;
    const char *pattern = NULL;
    char *filename = NULL;
    int i;

    xmlInitMemory();

    if (argc <= 1) {
	usage(argv[0]);
	return(1);
    }
    for (i = 1; i < argc ; i++) {
	if (!strcmp(argv[i], "-"))
	    break;

	if (argv[i][0] != '-')
	    continue;
	if ((!strcmp(argv[i], "-debug")) || (!strcmp(argv[i], "--debug"))) {
	    debug++;
	} else if ((!strcmp(argv[i], "-repeat")) ||
	         (!strcmp(argv[i], "--repeat"))) {
	    repeat++;
	} else if ((!strcmp(argv[i], "-i")) || (!strcmp(argv[i], "--input")))
	    filename = argv[++i];
        else {
	    fprintf(stderr, "Unknown option %s\n", argv[i]);
	    usage(argv[0]);
	}
    }
    if (filename != NULL) {
	testRegexpFile(filename);
    } else {
	for (i = 1; i < argc ; i++) {
	    if ((argv[i][0] != '-') || (strcmp(argv[i], "-") == 0)) {
		if (pattern == NULL) {
		    pattern = argv[i];
		    printf("Testing %s:\n", pattern);
		    comp = xmlRegexpCompile((const xmlChar *) pattern);
		    if (comp == NULL) {
			printf("   failed to compile\n");
			break;
		    }
		    if (debug)
			xmlRegexpPrint(stdout, comp);
		} else {
		    testRegexp(comp, argv[i]);
		}
	    }
	}
	xmlMemoryDump();
	if (comp != NULL)
	    xmlRegFreeRegexp(comp);
    }
    xmlCleanupParser();
    /* xmlMemoryDump(); */
    return(0);
}

#else
#include <stdio.h>
int main(int argc ATTRIBUTE_UNUSED, char **argv ATTRIBUTE_UNUSED) {
    printf("%s : Regexp support not compiled in\n", argv[0]);
    return(0);
}
#endif /* LIBXML_REGEXP_ENABLED */
