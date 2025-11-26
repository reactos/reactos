/*
 * xinclude.c : Code to implement XInclude processing
 *
 * World Wide Web Consortium W3C Last Call Working Draft 10 November 2003
 * http://www.w3.org/TR/2003/WD-xinclude-20031110
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXML
#include "libxml.h"

#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/uri.h>
#include <libxml/xpath.h>
#include <libxml/xpointer.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlerror.h>
#include <libxml/encoding.h>

#ifdef LIBXML_XINCLUDE_ENABLED
#include <libxml/xinclude.h>

#include "private/buf.h"
#include "private/error.h"
#include "private/tree.h"
#include "private/xinclude.h"

#define XINCLUDE_MAX_DEPTH 40

/************************************************************************
 *									*
 *			XInclude context handling			*
 *									*
 ************************************************************************/

/*
 * An XInclude context
 */
typedef xmlChar *xmlURL;

typedef struct _xmlXIncludeRef xmlXIncludeRef;
typedef xmlXIncludeRef *xmlXIncludeRefPtr;
struct _xmlXIncludeRef {
    xmlChar              *URI; /* the fully resolved resource URL */
    xmlChar         *fragment; /* the fragment in the URI */
    xmlNodePtr           elem; /* the xi:include element */
    xmlNodePtr            inc; /* the included copy */
    int                   xml; /* xml or txt */
    int	             fallback; /* fallback was loaded */
    int		      emptyFb; /* flag to show fallback empty */
    int		    expanding; /* flag to detect inclusion loops */
    int		      replace; /* should the node be replaced? */
};

typedef struct _xmlXIncludeDoc xmlXIncludeDoc;
typedef xmlXIncludeDoc *xmlXIncludeDocPtr;
struct _xmlXIncludeDoc {
    xmlDocPtr             doc; /* the parsed document */
    xmlChar              *url; /* the URL */
    int             expanding; /* flag to detect inclusion loops */
};

typedef struct _xmlXIncludeTxt xmlXIncludeTxt;
typedef xmlXIncludeTxt *xmlXIncludeTxtPtr;
struct _xmlXIncludeTxt {
    xmlChar		*text; /* text string */
    xmlChar              *url; /* the URL */
};

struct _xmlXIncludeCtxt {
    xmlDocPtr             doc; /* the source document */
    int                 incNr; /* number of includes */
    int                incMax; /* size of includes tab */
    xmlXIncludeRefPtr *incTab; /* array of included references */

    int                 txtNr; /* number of unparsed documents */
    int                txtMax; /* size of unparsed documents tab */
    xmlXIncludeTxt    *txtTab; /* array of unparsed documents */

    int                 urlNr; /* number of documents stacked */
    int                urlMax; /* size of document stack */
    xmlXIncludeDoc    *urlTab; /* document stack */

    int              nbErrors; /* the number of errors detected */
    int              fatalErr; /* abort processing */
    int                legacy; /* using XINCLUDE_OLD_NS */
    int            parseFlags; /* the flags used for parsing XML documents */
    xmlChar *		 base; /* the current xml:base */

    void            *_private; /* application data */

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    unsigned long    incTotal; /* total number of processed inclusions */
#endif
    int			depth; /* recursion depth */
    int		     isStream; /* streaming mode */
};

static xmlXIncludeRefPtr
xmlXIncludeExpandNode(xmlXIncludeCtxtPtr ctxt, xmlNodePtr node);

static int
xmlXIncludeLoadNode(xmlXIncludeCtxtPtr ctxt, xmlXIncludeRefPtr ref);

static int
xmlXIncludeDoProcess(xmlXIncludeCtxtPtr ctxt, xmlNodePtr tree);


/************************************************************************
 *									*
 *			XInclude error handler				*
 *									*
 ************************************************************************/

/**
 * xmlXIncludeErrMemory:
 * @extra:  extra information
 *
 * Handle an out of memory condition
 */
static void
xmlXIncludeErrMemory(xmlXIncludeCtxtPtr ctxt, xmlNodePtr node,
                     const char *extra)
{
    if (ctxt != NULL)
	ctxt->nbErrors++;
    __xmlRaiseError(NULL, NULL, NULL, ctxt, node, XML_FROM_XINCLUDE,
                    XML_ERR_NO_MEMORY, XML_ERR_ERROR, NULL, 0,
		    extra, NULL, NULL, 0, 0,
		    "Memory allocation failed : %s\n", extra);
}

/**
 * xmlXIncludeErr:
 * @ctxt: the XInclude context
 * @node: the context node
 * @msg:  the error message
 * @extra:  extra information
 *
 * Handle an XInclude error
 */
static void LIBXML_ATTR_FORMAT(4,0)
xmlXIncludeErr(xmlXIncludeCtxtPtr ctxt, xmlNodePtr node, int error,
               const char *msg, const xmlChar *extra)
{
    if (ctxt != NULL)
	ctxt->nbErrors++;
    __xmlRaiseError(NULL, NULL, NULL, ctxt, node, XML_FROM_XINCLUDE,
                    error, XML_ERR_ERROR, NULL, 0,
		    (const char *) extra, NULL, NULL, 0, 0,
		    msg, (const char *) extra);
}

#if 0
/**
 * xmlXIncludeWarn:
 * @ctxt: the XInclude context
 * @node: the context node
 * @msg:  the error message
 * @extra:  extra information
 *
 * Emit an XInclude warning.
 */
static void LIBXML_ATTR_FORMAT(4,0)
xmlXIncludeWarn(xmlXIncludeCtxtPtr ctxt, xmlNodePtr node, int error,
               const char *msg, const xmlChar *extra)
{
    __xmlRaiseError(NULL, NULL, NULL, ctxt, node, XML_FROM_XINCLUDE,
                    error, XML_ERR_WARNING, NULL, 0,
		    (const char *) extra, NULL, NULL, 0, 0,
		    msg, (const char *) extra);
}
#endif

/**
 * xmlXIncludeGetProp:
 * @ctxt:  the XInclude context
 * @cur:  the node
 * @name:  the attribute name
 *
 * Get an XInclude attribute
 *
 * Returns the value (to be freed) or NULL if not found
 */
static xmlChar *
xmlXIncludeGetProp(xmlXIncludeCtxtPtr ctxt, xmlNodePtr cur,
                   const xmlChar *name) {
    xmlChar *ret;

    ret = xmlGetNsProp(cur, XINCLUDE_NS, name);
    if (ret != NULL)
        return(ret);
    if (ctxt->legacy != 0) {
	ret = xmlGetNsProp(cur, XINCLUDE_OLD_NS, name);
	if (ret != NULL)
	    return(ret);
    }
    ret = xmlGetProp(cur, name);
    return(ret);
}
/**
 * xmlXIncludeFreeRef:
 * @ref: the XInclude reference
 *
 * Free an XInclude reference
 */
static void
xmlXIncludeFreeRef(xmlXIncludeRefPtr ref) {
    if (ref == NULL)
	return;
    if (ref->URI != NULL)
	xmlFree(ref->URI);
    if (ref->fragment != NULL)
	xmlFree(ref->fragment);
    xmlFree(ref);
}

/**
 * xmlXIncludeNewRef:
 * @ctxt: the XInclude context
 * @URI:  the resource URI
 * @elem:  the xi:include element
 *
 * Creates a new reference within an XInclude context
 *
 * Returns the new set
 */
static xmlXIncludeRefPtr
xmlXIncludeNewRef(xmlXIncludeCtxtPtr ctxt, const xmlChar *URI,
	          xmlNodePtr elem) {
    xmlXIncludeRefPtr ret;

    ret = (xmlXIncludeRefPtr) xmlMalloc(sizeof(xmlXIncludeRef));
    if (ret == NULL) {
        xmlXIncludeErrMemory(ctxt, elem, "growing XInclude context");
	return(NULL);
    }
    memset(ret, 0, sizeof(xmlXIncludeRef));
    if (URI == NULL)
	ret->URI = NULL;
    else
	ret->URI = xmlStrdup(URI);
    ret->fragment = NULL;
    ret->elem = elem;
    ret->xml = 0;
    ret->inc = NULL;
    if (ctxt->incNr >= ctxt->incMax) {
        xmlXIncludeRefPtr *tmp;
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        size_t newSize = ctxt->incMax ? ctxt->incMax * 2 : 1;
#else
        size_t newSize = ctxt->incMax ? ctxt->incMax * 2 : 4;
#endif

        tmp = (xmlXIncludeRefPtr *) xmlRealloc(ctxt->incTab,
	             newSize * sizeof(ctxt->incTab[0]));
        if (tmp == NULL) {
	    xmlXIncludeErrMemory(ctxt, elem, "growing XInclude context");
	    xmlXIncludeFreeRef(ret);
	    return(NULL);
	}
        ctxt->incTab = tmp;
        ctxt->incMax = newSize;
    }
    ctxt->incTab[ctxt->incNr++] = ret;
    return(ret);
}

/**
 * xmlXIncludeNewContext:
 * @doc:  an XML Document
 *
 * Creates a new XInclude context
 *
 * Returns the new set
 */
xmlXIncludeCtxtPtr
xmlXIncludeNewContext(xmlDocPtr doc) {
    xmlXIncludeCtxtPtr ret;

    if (doc == NULL)
	return(NULL);
    ret = (xmlXIncludeCtxtPtr) xmlMalloc(sizeof(xmlXIncludeCtxt));
    if (ret == NULL) {
	xmlXIncludeErrMemory(NULL, (xmlNodePtr) doc,
	                     "creating XInclude context");
	return(NULL);
    }
    memset(ret, 0, sizeof(xmlXIncludeCtxt));
    ret->doc = doc;
    ret->incNr = 0;
    ret->incMax = 0;
    ret->incTab = NULL;
    ret->nbErrors = 0;
    return(ret);
}

/**
 * xmlXIncludeFreeContext:
 * @ctxt: the XInclude context
 *
 * Free an XInclude context
 */
void
xmlXIncludeFreeContext(xmlXIncludeCtxtPtr ctxt) {
    int i;

    if (ctxt == NULL)
	return;
    if (ctxt->urlTab != NULL) {
	for (i = 0; i < ctxt->urlNr; i++) {
	    xmlFreeDoc(ctxt->urlTab[i].doc);
	    xmlFree(ctxt->urlTab[i].url);
	}
	xmlFree(ctxt->urlTab);
    }
    for (i = 0;i < ctxt->incNr;i++) {
	if (ctxt->incTab[i] != NULL)
	    xmlXIncludeFreeRef(ctxt->incTab[i]);
    }
    if (ctxt->incTab != NULL)
	xmlFree(ctxt->incTab);
    if (ctxt->txtTab != NULL) {
	for (i = 0;i < ctxt->txtNr;i++) {
	    xmlFree(ctxt->txtTab[i].text);
	    xmlFree(ctxt->txtTab[i].url);
	}
	xmlFree(ctxt->txtTab);
    }
    if (ctxt->base != NULL) {
        xmlFree(ctxt->base);
    }
    xmlFree(ctxt);
}

/**
 * xmlXIncludeParseFile:
 * @ctxt:  the XInclude context
 * @URL:  the URL or file path
 *
 * parse a document for XInclude
 */
static xmlDocPtr
xmlXIncludeParseFile(xmlXIncludeCtxtPtr ctxt, const char *URL) {
    xmlDocPtr ret;
    xmlParserCtxtPtr pctxt;
    xmlParserInputPtr inputStream;

    xmlInitParser();

    pctxt = xmlNewParserCtxt();
    if (pctxt == NULL) {
	xmlXIncludeErrMemory(ctxt, NULL, "cannot allocate parser context");
	return(NULL);
    }

    /*
     * pass in the application data to the parser context.
     */
    pctxt->_private = ctxt->_private;

    /*
     * try to ensure that new documents included are actually
     * built with the same dictionary as the including document.
     */
    if ((ctxt->doc != NULL) && (ctxt->doc->dict != NULL)) {
       if (pctxt->dict != NULL)
            xmlDictFree(pctxt->dict);
	pctxt->dict = ctxt->doc->dict;
	xmlDictReference(pctxt->dict);
    }

    xmlCtxtUseOptions(pctxt, ctxt->parseFlags | XML_PARSE_DTDLOAD);

    /* Don't read from stdin. */
    if ((URL != NULL) && (strcmp(URL, "-") == 0))
        URL = "./-";

    inputStream = xmlLoadExternalEntity(URL, NULL, pctxt);
    if (inputStream == NULL) {
	xmlFreeParserCtxt(pctxt);
	return(NULL);
    }

    inputPush(pctxt, inputStream);

    if (pctxt->directory == NULL)
        pctxt->directory = xmlParserGetDirectory(URL);

    pctxt->loadsubset |= XML_DETECT_IDS;

    xmlParseDocument(pctxt);

    if (pctxt->wellFormed) {
        ret = pctxt->myDoc;
    }
    else {
        ret = NULL;
	if (pctxt->myDoc != NULL)
	    xmlFreeDoc(pctxt->myDoc);
        pctxt->myDoc = NULL;
    }
    xmlFreeParserCtxt(pctxt);

    return(ret);
}

/**
 * xmlXIncludeAddNode:
 * @ctxt:  the XInclude context
 * @cur:  the new node
 *
 * Add a new node to process to an XInclude context
 */
static xmlXIncludeRefPtr
xmlXIncludeAddNode(xmlXIncludeCtxtPtr ctxt, xmlNodePtr cur) {
    xmlXIncludeRefPtr ref;
    xmlURIPtr uri;
    xmlChar *URL;
    xmlChar *fragment = NULL;
    xmlChar *href;
    xmlChar *parse;
    xmlChar *base;
    xmlChar *URI;
    int xml = 1;
    int local = 0;


    if (ctxt == NULL)
	return(NULL);
    if (cur == NULL)
	return(NULL);

    /*
     * read the attributes
     */
    href = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_HREF);
    if (href == NULL) {
	href = xmlStrdup(BAD_CAST ""); /* @@@@ href is now optional */
	if (href == NULL)
	    return(NULL);
    }
    parse = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_PARSE);
    if (parse != NULL) {
	if (xmlStrEqual(parse, XINCLUDE_PARSE_XML))
	    xml = 1;
	else if (xmlStrEqual(parse, XINCLUDE_PARSE_TEXT))
	    xml = 0;
	else {
	    xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_PARSE_VALUE,
	                   "invalid value %s for 'parse'\n", parse);
	    if (href != NULL)
		xmlFree(href);
	    if (parse != NULL)
		xmlFree(parse);
	    return(NULL);
	}
    }

    /*
     * compute the URI
     */
    base = xmlNodeGetBase(ctxt->doc, cur);
    if (base == NULL) {
	URI = xmlBuildURI(href, ctxt->doc->URL);
    } else {
	URI = xmlBuildURI(href, base);
    }
    if (URI == NULL) {
	xmlChar *escbase;
	xmlChar *eschref;
	/*
	 * Some escaping may be needed
	 */
	escbase = xmlURIEscape(base);
	eschref = xmlURIEscape(href);
	URI = xmlBuildURI(eschref, escbase);
	if (escbase != NULL)
	    xmlFree(escbase);
	if (eschref != NULL)
	    xmlFree(eschref);
    }
    if (parse != NULL)
	xmlFree(parse);
    if (href != NULL)
	xmlFree(href);
    if (base != NULL)
	xmlFree(base);
    if (URI == NULL) {
	xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_HREF_URI,
	               "failed build URL\n", NULL);
	return(NULL);
    }
    fragment = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_PARSE_XPOINTER);

    /*
     * Check the URL and remove any fragment identifier
     */
    uri = xmlParseURI((const char *)URI);
    if (uri == NULL) {
	xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_HREF_URI,
	               "invalid value URI %s\n", URI);
	if (fragment != NULL)
	    xmlFree(fragment);
	xmlFree(URI);
	return(NULL);
    }

    if (uri->fragment != NULL) {
        if (ctxt->legacy != 0) {
	    if (fragment == NULL) {
		fragment = (xmlChar *) uri->fragment;
	    } else {
		xmlFree(uri->fragment);
	    }
	} else {
	    xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_FRAGMENT_ID,
       "Invalid fragment identifier in URI %s use the xpointer attribute\n",
                           URI);
	    if (fragment != NULL)
	        xmlFree(fragment);
	    xmlFreeURI(uri);
	    xmlFree(URI);
	    return(NULL);
	}
	uri->fragment = NULL;
    }
    URL = xmlSaveUri(uri);
    xmlFreeURI(uri);
    if (URL == NULL) {
	xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_HREF_URI,
	               "invalid value URI %s\n", URI);
	if (fragment != NULL)
	    xmlFree(fragment);
        xmlFree(URI);
	return(NULL);
    }
    xmlFree(URI);

    if (xmlStrEqual(URL, ctxt->doc->URL))
	local = 1;

    /*
     * If local and xml then we need a fragment
     */
    if ((local == 1) && (xml == 1) &&
        ((fragment == NULL) || (fragment[0] == 0))) {
	xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_RECURSION,
	               "detected a local recursion with no xpointer in %s\n",
		       URL);
        xmlFree(URL);
        xmlFree(fragment);
	return(NULL);
    }

    ref = xmlXIncludeNewRef(ctxt, URL, cur);
    xmlFree(URL);
    if (ref == NULL) {
        xmlFree(fragment);
	return(NULL);
    }
    ref->fragment = fragment;
    ref->xml = xml;
    return(ref);
}

/**
 * xmlXIncludeRecurseDoc:
 * @ctxt:  the XInclude context
 * @doc:  the new document
 * @url:  the associated URL
 *
 * The XInclude recursive nature is handled at this point.
 */
static void
xmlXIncludeRecurseDoc(xmlXIncludeCtxtPtr ctxt, xmlDocPtr doc,
	              const xmlURL url ATTRIBUTE_UNUSED) {
    xmlDocPtr oldDoc;
    xmlXIncludeRefPtr *oldIncTab;
    int oldIncMax, oldIncNr, oldIsStream;
    int i;

    oldDoc = ctxt->doc;
    oldIncMax = ctxt->incMax;
    oldIncNr = ctxt->incNr;
    oldIncTab = ctxt->incTab;
    oldIsStream = ctxt->isStream;
    ctxt->doc = doc;
    ctxt->incMax = 0;
    ctxt->incNr = 0;
    ctxt->incTab = NULL;
    ctxt->isStream = 0;

    xmlXIncludeDoProcess(ctxt, xmlDocGetRootElement(doc));

    if (ctxt->incTab != NULL) {
        for (i = 0; i < ctxt->incNr; i++)
            xmlXIncludeFreeRef(ctxt->incTab[i]);
        xmlFree(ctxt->incTab);
    }

    ctxt->doc = oldDoc;
    ctxt->incMax = oldIncMax;
    ctxt->incNr = oldIncNr;
    ctxt->incTab = oldIncTab;
    ctxt->isStream = oldIsStream;
}

/************************************************************************
 *									*
 *			Node copy with specific semantic		*
 *									*
 ************************************************************************/

/**
 * xmlXIncludeCopyNode:
 * @ctxt:  the XInclude context
 * @elem:  the element
 * @copyChildren:  copy children instead of node if true
 *
 * Make a copy of the node while expanding nested XIncludes.
 *
 * Returns a node list, not a single node.
 */
static xmlNodePtr
xmlXIncludeCopyNode(xmlXIncludeCtxtPtr ctxt, xmlNodePtr elem,
                    int copyChildren) {
    xmlNodePtr result = NULL;
    xmlNodePtr insertParent = NULL;
    xmlNodePtr insertLast = NULL;
    xmlNodePtr cur;

    if (copyChildren) {
        cur = elem->children;
        if (cur == NULL)
            return(NULL);
    } else {
        cur = elem;
    }

    while (1) {
        xmlNodePtr copy = NULL;
        int recurse = 0;

        if ((cur->type == XML_DOCUMENT_NODE) ||
            (cur->type == XML_DTD_NODE)) {
            ;
        } else if ((cur->type == XML_ELEMENT_NODE) &&
                   (cur->ns != NULL) &&
                   (xmlStrEqual(cur->name, XINCLUDE_NODE)) &&
                   ((xmlStrEqual(cur->ns->href, XINCLUDE_NS)) ||
                    (xmlStrEqual(cur->ns->href, XINCLUDE_OLD_NS)))) {
            xmlXIncludeRefPtr ref = xmlXIncludeExpandNode(ctxt, cur);

            if (ref == NULL)
                goto error;
            /*
             * TODO: Insert XML_XINCLUDE_START and XML_XINCLUDE_END nodes
             */
            if (ref->inc != NULL) {
                copy = xmlStaticCopyNodeList(ref->inc, ctxt->doc,
                                             insertParent);
                if (copy == NULL)
                    goto error;
            }
        } else {
            copy = xmlStaticCopyNode(cur, ctxt->doc, insertParent, 2);
            if (copy == NULL)
                goto error;

            recurse = (cur->type != XML_ENTITY_REF_NODE) &&
                      (cur->children != NULL);
        }

        if (copy != NULL) {
            if (result == NULL)
                result = copy;
            if (insertLast != NULL) {
                insertLast->next = copy;
                copy->prev = insertLast;
            } else if (insertParent != NULL) {
                insertParent->children = copy;
            }
            insertLast = copy;
            while (insertLast->next != NULL) {
                insertLast = insertLast->next;
            }
        }

        if (recurse) {
            cur = cur->children;
            insertParent = insertLast;
            insertLast = NULL;
            continue;
        }

        if (cur == elem)
            return(result);

        while (cur->next == NULL) {
            if (insertParent != NULL)
                insertParent->last = insertLast;
            cur = cur->parent;
            if (cur == elem)
                return(result);
            insertLast = insertParent;
            insertParent = insertParent->parent;
        }

        cur = cur->next;
    }

error:
    xmlFreeNodeList(result);
    return(NULL);
}

#ifdef LIBXML_XPTR_LOCS_ENABLED
/**
 * xmlXIncludeGetNthChild:
 * @cur:  the node
 * @no:  the child number
 *
 * Returns the @n'th element child of @cur or NULL
 */
static xmlNodePtr
xmlXIncludeGetNthChild(xmlNodePtr cur, int no) {
    int i;
    if ((cur == NULL) || (cur->type == XML_NAMESPACE_DECL))
        return(NULL);
    cur = cur->children;
    for (i = 0;i <= no;cur = cur->next) {
	if (cur == NULL)
	    return(cur);
	if ((cur->type == XML_ELEMENT_NODE) ||
	    (cur->type == XML_DOCUMENT_NODE) ||
	    (cur->type == XML_HTML_DOCUMENT_NODE)) {
	    i++;
	    if (i == no)
		break;
	}
    }
    return(cur);
}

xmlNodePtr xmlXPtrAdvanceNode(xmlNodePtr cur, int *level); /* in xpointer.c */
/**
 * xmlXIncludeCopyRange:
 * @ctxt:  the XInclude context
 * @obj:  the XPointer result from the evaluation.
 *
 * Build a node list tree copy of the XPointer result.
 *
 * Returns an xmlNodePtr list or NULL.
 *         The caller has to free the node tree.
 */
static xmlNodePtr
xmlXIncludeCopyRange(xmlXIncludeCtxtPtr ctxt, xmlXPathObjectPtr range) {
    /* pointers to generated nodes */
    xmlNodePtr list = NULL, last = NULL, listParent = NULL;
    xmlNodePtr tmp, tmp2;
    /* pointers to traversal nodes */
    xmlNodePtr start, cur, end;
    int index1, index2;
    int level = 0, lastLevel = 0, endLevel = 0, endFlag = 0;

    if ((ctxt == NULL) || (range == NULL))
	return(NULL);
    if (range->type != XPATH_RANGE)
	return(NULL);
    start = (xmlNodePtr) range->user;

    if ((start == NULL) || (start->type == XML_NAMESPACE_DECL))
	return(NULL);
    end = range->user2;
    if (end == NULL)
	return(xmlDocCopyNode(start, ctxt->doc, 1));
    if (end->type == XML_NAMESPACE_DECL)
        return(NULL);

    cur = start;
    index1 = range->index;
    index2 = range->index2;
    /*
     * level is depth of the current node under consideration
     * list is the pointer to the root of the output tree
     * listParent is a pointer to the parent of output tree (within
       the included file) in case we need to add another level
     * last is a pointer to the last node added to the output tree
     * lastLevel is the depth of last (relative to the root)
     */
    while (cur != NULL) {
	/*
	 * Check if our output tree needs a parent
	 */
	if (level < 0) {
	    while (level < 0) {
	        /* copy must include namespaces and properties */
	        tmp2 = xmlDocCopyNode(listParent, ctxt->doc, 2);
	        xmlAddChild(tmp2, list);
	        list = tmp2;
	        listParent = listParent->parent;
	        level++;
	    }
	    last = list;
	    lastLevel = 0;
	}
	/*
	 * Check whether we need to change our insertion point
	 */
	while (level < lastLevel) {
	    last = last->parent;
	    lastLevel --;
	}
	if (cur == end) {	/* Are we at the end of the range? */
	    if (cur->type == XML_TEXT_NODE) {
		const xmlChar *content = cur->content;
		int len;

		if (content == NULL) {
		    tmp = xmlNewDocTextLen(ctxt->doc, NULL, 0);
		} else {
		    len = index2;
		    if ((cur == start) && (index1 > 1)) {
			content += (index1 - 1);
			len -= (index1 - 1);
		    } else {
			len = index2;
		    }
		    tmp = xmlNewDocTextLen(ctxt->doc, content, len);
		}
		/* single sub text node selection */
		if (list == NULL)
		    return(tmp);
		/* prune and return full set */
		if (level == lastLevel)
		    xmlAddNextSibling(last, tmp);
		else
		    xmlAddChild(last, tmp);
		return(list);
	    } else {	/* ending node not a text node */
	        endLevel = level;	/* remember the level of the end node */
		endFlag = 1;
		/* last node - need to take care of properties + namespaces */
		tmp = xmlDocCopyNode(cur, ctxt->doc, 2);
		if (list == NULL) {
		    list = tmp;
		    listParent = cur->parent;
		    last = tmp;
		} else {
		    if (level == lastLevel)
			last = xmlAddNextSibling(last, tmp);
		    else {
			last = xmlAddChild(last, tmp);
			lastLevel = level;
		    }
		}

		if (index2 > 1) {
		    end = xmlXIncludeGetNthChild(cur, index2 - 1);
		    index2 = 0;
		}
		if ((cur == start) && (index1 > 1)) {
		    cur = xmlXIncludeGetNthChild(cur, index1 - 1);
		    index1 = 0;
		}  else {
		    cur = cur->children;
		}
		level++;	/* increment level to show change */
		/*
		 * Now gather the remaining nodes from cur to end
		 */
		continue;	/* while */
	    }
	} else if (cur == start) {	/* Not at the end, are we at start? */
	    if ((cur->type == XML_TEXT_NODE) ||
		(cur->type == XML_CDATA_SECTION_NODE)) {
		const xmlChar *content = cur->content;

		if (content == NULL) {
		    tmp = xmlNewDocTextLen(ctxt->doc, NULL, 0);
		} else {
		    if (index1 > 1) {
			content += (index1 - 1);
			index1 = 0;
		    }
		    tmp = xmlNewDocText(ctxt->doc, content);
		}
		last = list = tmp;
		listParent = cur->parent;
	    } else {		/* Not text node */
	        /*
		 * start of the range - need to take care of
		 * properties and namespaces
		 */
		tmp = xmlDocCopyNode(cur, ctxt->doc, 2);
		list = last = tmp;
		listParent = cur->parent;
		if (index1 > 1) {	/* Do we need to position? */
		    cur = xmlXIncludeGetNthChild(cur, index1 - 1);
		    level = lastLevel = 1;
		    index1 = 0;
		    /*
		     * Now gather the remaining nodes from cur to end
		     */
		    continue; /* while */
		}
	    }
	} else {
	    tmp = NULL;
	    switch (cur->type) {
		case XML_DTD_NODE:
		case XML_ELEMENT_DECL:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_NODE:
		    /* Do not copy DTD information */
		    break;
		case XML_ENTITY_DECL:
		    /* handle crossing entities -> stack needed */
		    break;
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
		    /* don't consider it part of the tree content */
		    break;
		case XML_ATTRIBUTE_NODE:
		    /* Humm, should not happen ! */
		    break;
		default:
		    /*
		     * Middle of the range - need to take care of
		     * properties and namespaces
		     */
		    tmp = xmlDocCopyNode(cur, ctxt->doc, 2);
		    break;
	    }
	    if (tmp != NULL) {
		if (level == lastLevel)
		    last = xmlAddNextSibling(last, tmp);
		else {
		    last = xmlAddChild(last, tmp);
		    lastLevel = level;
		}
	    }
	}
	/*
	 * Skip to next node in document order
	 */
	cur = xmlXPtrAdvanceNode(cur, &level);
	if (endFlag && (level >= endLevel))
	    break;
    }
    return(list);
}
#endif /* LIBXML_XPTR_LOCS_ENABLED */

/**
 * xmlXIncludeCopyXPointer:
 * @ctxt:  the XInclude context
 * @obj:  the XPointer result from the evaluation.
 *
 * Build a node list tree copy of the XPointer result.
 * This will drop Attributes and Namespace declarations.
 *
 * Returns an xmlNodePtr list or NULL.
 *         the caller has to free the node tree.
 */
static xmlNodePtr
xmlXIncludeCopyXPointer(xmlXIncludeCtxtPtr ctxt, xmlXPathObjectPtr obj) {
    xmlNodePtr list = NULL, last = NULL, copy;
    int i;

    if ((ctxt == NULL) || (obj == NULL))
	return(NULL);
    switch (obj->type) {
        case XPATH_NODESET: {
	    xmlNodeSetPtr set = obj->nodesetval;
	    if (set == NULL)
		return(NULL);
	    for (i = 0;i < set->nodeNr;i++) {
                xmlNodePtr node;

		if (set->nodeTab[i] == NULL)
		    continue;
		switch (set->nodeTab[i]->type) {
		    case XML_DOCUMENT_NODE:
		    case XML_HTML_DOCUMENT_NODE:
                        node = xmlDocGetRootElement(
                                (xmlDocPtr) set->nodeTab[i]);
                        if (node == NULL) {
                            xmlXIncludeErr(ctxt, set->nodeTab[i],
                                           XML_ERR_INTERNAL_ERROR,
                                           "document without root\n", NULL);
                            continue;
                        }
                        break;
		    case XML_TEXT_NODE:
		    case XML_CDATA_SECTION_NODE:
		    case XML_ELEMENT_NODE:
		    case XML_PI_NODE:
		    case XML_COMMENT_NODE:
                        node = set->nodeTab[i];
			break;
                    default:
                        xmlXIncludeErr(ctxt, set->nodeTab[i],
                                       XML_XINCLUDE_XPTR_RESULT,
                                       "invalid node type in XPtr result\n",
                                       NULL);
			continue; /* for */
		}
                /*
                 * OPTIMIZE TODO: External documents should already be
                 * expanded, so xmlDocCopyNode should work as well.
                 * xmlXIncludeCopyNode is only required for the initial
                 * document.
                 */
		copy = xmlXIncludeCopyNode(ctxt, node, 0);
                if (copy == NULL) {
                    xmlFreeNodeList(list);
                    return(NULL);
                }
		if (last == NULL) {
                    list = copy;
                } else {
                    while (last->next != NULL)
                        last = last->next;
                    copy->prev = last;
                    last->next = copy;
		}
                last = copy;
	    }
	    break;
	}
#ifdef LIBXML_XPTR_LOCS_ENABLED
	case XPATH_LOCATIONSET: {
	    xmlLocationSetPtr set = (xmlLocationSetPtr) obj->user;
	    if (set == NULL)
		return(NULL);
	    for (i = 0;i < set->locNr;i++) {
		if (last == NULL)
		    list = last = xmlXIncludeCopyXPointer(ctxt,
			                                  set->locTab[i]);
		else
		    xmlAddNextSibling(last,
			    xmlXIncludeCopyXPointer(ctxt, set->locTab[i]));
		if (last != NULL) {
		    while (last->next != NULL)
			last = last->next;
		}
	    }
	    break;
	}
	case XPATH_RANGE:
	    return(xmlXIncludeCopyRange(ctxt, obj));
	case XPATH_POINT:
	    /* points are ignored in XInclude */
	    break;
#endif
	default:
	    break;
    }
    return(list);
}
/************************************************************************
 *									*
 *			XInclude I/O handling				*
 *									*
 ************************************************************************/

typedef struct _xmlXIncludeMergeData xmlXIncludeMergeData;
typedef xmlXIncludeMergeData *xmlXIncludeMergeDataPtr;
struct _xmlXIncludeMergeData {
    xmlDocPtr doc;
    xmlXIncludeCtxtPtr ctxt;
};

/**
 * xmlXIncludeMergeOneEntity:
 * @ent: the entity
 * @doc:  the including doc
 * @name: the entity name
 *
 * Implements the merge of one entity
 */
static void
xmlXIncludeMergeEntity(void *payload, void *vdata,
	               const xmlChar *name ATTRIBUTE_UNUSED) {
    xmlEntityPtr ent = (xmlEntityPtr) payload;
    xmlXIncludeMergeDataPtr data = (xmlXIncludeMergeDataPtr) vdata;
    xmlEntityPtr ret, prev;
    xmlDocPtr doc;
    xmlXIncludeCtxtPtr ctxt;

    if ((ent == NULL) || (data == NULL))
	return;
    ctxt = data->ctxt;
    doc = data->doc;
    if ((ctxt == NULL) || (doc == NULL))
	return;
    switch (ent->etype) {
        case XML_INTERNAL_PARAMETER_ENTITY:
        case XML_EXTERNAL_PARAMETER_ENTITY:
        case XML_INTERNAL_PREDEFINED_ENTITY:
	    return;
        case XML_INTERNAL_GENERAL_ENTITY:
        case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
        case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
	    break;
    }
    ret = xmlAddDocEntity(doc, ent->name, ent->etype, ent->ExternalID,
			  ent->SystemID, ent->content);
    if (ret != NULL) {
	if (ent->URI != NULL)
	    ret->URI = xmlStrdup(ent->URI);
    } else {
	prev = xmlGetDocEntity(doc, ent->name);
	if (prev != NULL) {
	    if (ent->etype != prev->etype)
		goto error;

	    if ((ent->SystemID != NULL) && (prev->SystemID != NULL)) {
		if (!xmlStrEqual(ent->SystemID, prev->SystemID))
		    goto error;
	    } else if ((ent->ExternalID != NULL) &&
		       (prev->ExternalID != NULL)) {
		if (!xmlStrEqual(ent->ExternalID, prev->ExternalID))
		    goto error;
	    } else if ((ent->content != NULL) && (prev->content != NULL)) {
		if (!xmlStrEqual(ent->content, prev->content))
		    goto error;
	    } else {
		goto error;
	    }

	}
    }
    return;
error:
    switch (ent->etype) {
        case XML_INTERNAL_PARAMETER_ENTITY:
        case XML_EXTERNAL_PARAMETER_ENTITY:
        case XML_INTERNAL_PREDEFINED_ENTITY:
        case XML_INTERNAL_GENERAL_ENTITY:
        case XML_EXTERNAL_GENERAL_PARSED_ENTITY:
	    return;
        case XML_EXTERNAL_GENERAL_UNPARSED_ENTITY:
	    break;
    }
    xmlXIncludeErr(ctxt, (xmlNodePtr) ent, XML_XINCLUDE_ENTITY_DEF_MISMATCH,
                   "mismatch in redefinition of entity %s\n",
		   ent->name);
}

/**
 * xmlXIncludeMergeEntities:
 * @ctxt: an XInclude context
 * @doc:  the including doc
 * @from:  the included doc
 *
 * Implements the entity merge
 *
 * Returns 0 if merge succeeded, -1 if some processing failed
 */
static int
xmlXIncludeMergeEntities(xmlXIncludeCtxtPtr ctxt, xmlDocPtr doc,
	                 xmlDocPtr from) {
    xmlNodePtr cur;
    xmlDtdPtr target, source;

    if (ctxt == NULL)
	return(-1);

    if ((from == NULL) || (from->intSubset == NULL))
	return(0);

    target = doc->intSubset;
    if (target == NULL) {
	cur = xmlDocGetRootElement(doc);
	if (cur == NULL)
	    return(-1);
        target = xmlCreateIntSubset(doc, cur->name, NULL, NULL);
	if (target == NULL)
	    return(-1);
    }

    source = from->intSubset;
    if ((source != NULL) && (source->entities != NULL)) {
	xmlXIncludeMergeData data;

	data.ctxt = ctxt;
	data.doc = doc;

	xmlHashScan((xmlHashTablePtr) source->entities,
		    xmlXIncludeMergeEntity, &data);
    }
    source = from->extSubset;
    if ((source != NULL) && (source->entities != NULL)) {
	xmlXIncludeMergeData data;

	data.ctxt = ctxt;
	data.doc = doc;

	/*
	 * don't duplicate existing stuff when external subsets are the same
	 */
	if ((!xmlStrEqual(target->ExternalID, source->ExternalID)) &&
	    (!xmlStrEqual(target->SystemID, source->SystemID))) {
	    xmlHashScan((xmlHashTablePtr) source->entities,
			xmlXIncludeMergeEntity, &data);
	}
    }
    return(0);
}

/**
 * xmlXIncludeLoadDoc:
 * @ctxt:  the XInclude context
 * @url:  the associated URL
 * @ref:  an XMLXincludeRefPtr
 *
 * Load the document, and store the result in the XInclude context
 *
 * Returns 0 in case of success, -1 in case of failure
 */
static int
xmlXIncludeLoadDoc(xmlXIncludeCtxtPtr ctxt, const xmlChar *url,
                   xmlXIncludeRefPtr ref) {
    xmlXIncludeDocPtr cache;
    xmlDocPtr doc;
    xmlURIPtr uri;
    xmlChar *URL = NULL;
    xmlChar *fragment = NULL;
    int i = 0;
    int ret = -1;
    int cacheNr;
#ifdef LIBXML_XPTR_ENABLED
    int saveFlags;
#endif

    /*
     * Check the URL and remove any fragment identifier
     */
    uri = xmlParseURI((const char *)url);
    if (uri == NULL) {
	xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_HREF_URI,
		       "invalid value URI %s\n", url);
        goto error;
    }
    if (uri->fragment != NULL) {
	fragment = (xmlChar *) uri->fragment;
	uri->fragment = NULL;
    }
    if (ref->fragment != NULL) {
	if (fragment != NULL) xmlFree(fragment);
	fragment = xmlStrdup(ref->fragment);
    }
    URL = xmlSaveUri(uri);
    xmlFreeURI(uri);
    if (URL == NULL) {
        xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_HREF_URI,
                       "invalid value URI %s\n", url);
        goto error;
    }

    /*
     * Handling of references to the local document are done
     * directly through ctxt->doc.
     */
    if ((URL[0] == 0) || (URL[0] == '#') ||
	((ctxt->doc != NULL) && (xmlStrEqual(URL, ctxt->doc->URL)))) {
	doc = ctxt->doc;
        goto loaded;
    }

    /*
     * Prevent reloading the document twice.
     */
    for (i = 0; i < ctxt->urlNr; i++) {
	if (xmlStrEqual(URL, ctxt->urlTab[i].url)) {
            if (ctxt->urlTab[i].expanding) {
                xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_RECURSION,
                               "inclusion loop detected\n", NULL);
                goto error;
            }
	    doc = ctxt->urlTab[i].doc;
            if (doc == NULL)
                goto error;
	    goto loaded;
	}
    }

    /*
     * Load it.
     */
#ifdef LIBXML_XPTR_ENABLED
    /*
     * If this is an XPointer evaluation, we want to assure that
     * all entities have been resolved prior to processing the
     * referenced document
     */
    saveFlags = ctxt->parseFlags;
    if (fragment != NULL) {	/* if this is an XPointer eval */
	ctxt->parseFlags |= XML_PARSE_NOENT;
    }
#endif

    doc = xmlXIncludeParseFile(ctxt, (const char *)URL);
#ifdef LIBXML_XPTR_ENABLED
    ctxt->parseFlags = saveFlags;
#endif

    /* Also cache NULL docs */
    if (ctxt->urlNr >= ctxt->urlMax) {
        xmlXIncludeDoc *tmp;
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        size_t newSize = ctxt->urlMax ? ctxt->urlMax * 2 : 1;
#else
        size_t newSize = ctxt->urlMax ? ctxt->urlMax * 2 : 8;
#endif

        tmp = xmlRealloc(ctxt->urlTab, sizeof(xmlXIncludeDoc) * newSize);
        if (tmp == NULL) {
            xmlXIncludeErrMemory(ctxt, ref->elem,
                                 "growing XInclude URL table");
            xmlFreeDoc(doc);
            goto error;
        }
        ctxt->urlMax = newSize;
        ctxt->urlTab = tmp;
    }
    cacheNr = ctxt->urlNr++;
    cache = &ctxt->urlTab[cacheNr];
    cache->doc = doc;
    cache->url = xmlStrdup(URL);
    cache->expanding = 0;

    if (doc == NULL)
        goto error;
    /*
     * It's possible that the requested URL has been mapped to a
     * completely different location (e.g. through a catalog entry).
     * To check for this, we compare the URL with that of the doc
     * and change it if they disagree (bug 146988).
     */
   if (!xmlStrEqual(URL, doc->URL)) {
       xmlFree(URL);
       URL = xmlStrdup(doc->URL);
   }

    /*
     * Make sure we have all entities fixed up
     */
    xmlXIncludeMergeEntities(ctxt, ctxt->doc, doc);

    /*
     * We don't need the DTD anymore, free up space
    if (doc->intSubset != NULL) {
	xmlUnlinkNode((xmlNodePtr) doc->intSubset);
	xmlFreeNode((xmlNodePtr) doc->intSubset);
	doc->intSubset = NULL;
    }
    if (doc->extSubset != NULL) {
	xmlUnlinkNode((xmlNodePtr) doc->extSubset);
	xmlFreeNode((xmlNodePtr) doc->extSubset);
	doc->extSubset = NULL;
    }
     */
    cache->expanding = 1;
    xmlXIncludeRecurseDoc(ctxt, doc, URL);
    /* urlTab might be reallocated. */
    cache = &ctxt->urlTab[cacheNr];
    cache->expanding = 0;

loaded:
    if (fragment == NULL) {
	/*
	 * Add the top children list as the replacement copy.
	 */
        ref->inc = xmlDocCopyNode(xmlDocGetRootElement(doc), ctxt->doc, 1);
    }
#ifdef LIBXML_XPTR_ENABLED
    else {
	/*
	 * Computes the XPointer expression and make a copy used
	 * as the replacement copy.
	 */
	xmlXPathObjectPtr xptr;
	xmlXPathContextPtr xptrctxt;
	xmlNodeSetPtr set;

        if (ctxt->isStream && doc == ctxt->doc) {
	    xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_XPTR_FAILED,
			   "XPointer expressions not allowed in streaming"
                           " mode\n", NULL);
            goto error;
        }

	xptrctxt = xmlXPtrNewContext(doc, NULL, NULL);
	if (xptrctxt == NULL) {
	    xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_XPTR_FAILED,
			   "could not create XPointer context\n", NULL);
            goto error;
	}
	xptr = xmlXPtrEval(fragment, xptrctxt);
	if (xptr == NULL) {
	    xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_XPTR_FAILED,
			   "XPointer evaluation failed: #%s\n",
			   fragment);
	    xmlXPathFreeContext(xptrctxt);
            goto error;
	}
	switch (xptr->type) {
	    case XPATH_UNDEFINED:
	    case XPATH_BOOLEAN:
	    case XPATH_NUMBER:
	    case XPATH_STRING:
#ifdef LIBXML_XPTR_LOCS_ENABLED
	    case XPATH_POINT:
#endif
	    case XPATH_USERS:
	    case XPATH_XSLT_TREE:
		xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_XPTR_RESULT,
			       "XPointer is not a range: #%s\n",
			       fragment);
                xmlXPathFreeObject(xptr);
		xmlXPathFreeContext(xptrctxt);
                goto error;
	    case XPATH_NODESET:
	        if ((xptr->nodesetval == NULL) ||
		    (xptr->nodesetval->nodeNr <= 0)) {
                    xmlXPathFreeObject(xptr);
		    xmlXPathFreeContext(xptrctxt);
                    goto error;
		}

#ifdef LIBXML_XPTR_LOCS_ENABLED
	    case XPATH_RANGE:
	    case XPATH_LOCATIONSET:
		break;
#endif
	}
	set = xptr->nodesetval;
	if (set != NULL) {
	    for (i = 0;i < set->nodeNr;i++) {
		if (set->nodeTab[i] == NULL)
		    continue;
		switch (set->nodeTab[i]->type) {
		    case XML_ELEMENT_NODE:
		    case XML_TEXT_NODE:
		    case XML_CDATA_SECTION_NODE:
		    case XML_ENTITY_REF_NODE:
		    case XML_ENTITY_NODE:
		    case XML_PI_NODE:
		    case XML_COMMENT_NODE:
		    case XML_DOCUMENT_NODE:
		    case XML_HTML_DOCUMENT_NODE:
			continue;

		    case XML_ATTRIBUTE_NODE:
			xmlXIncludeErr(ctxt, ref->elem,
			               XML_XINCLUDE_XPTR_RESULT,
				       "XPointer selects an attribute: #%s\n",
				       fragment);
			set->nodeTab[i] = NULL;
			continue;
		    case XML_NAMESPACE_DECL:
			xmlXIncludeErr(ctxt, ref->elem,
			               XML_XINCLUDE_XPTR_RESULT,
				       "XPointer selects a namespace: #%s\n",
				       fragment);
			set->nodeTab[i] = NULL;
			continue;
		    case XML_DOCUMENT_TYPE_NODE:
		    case XML_DOCUMENT_FRAG_NODE:
		    case XML_NOTATION_NODE:
		    case XML_DTD_NODE:
		    case XML_ELEMENT_DECL:
		    case XML_ATTRIBUTE_DECL:
		    case XML_ENTITY_DECL:
		    case XML_XINCLUDE_START:
		    case XML_XINCLUDE_END:
			xmlXIncludeErr(ctxt, ref->elem,
			               XML_XINCLUDE_XPTR_RESULT,
				   "XPointer selects unexpected nodes: #%s\n",
				       fragment);
			set->nodeTab[i] = NULL;
			set->nodeTab[i] = NULL;
			continue; /* for */
		}
	    }
	}
        ref->inc = xmlXIncludeCopyXPointer(ctxt, xptr);
        xmlXPathFreeObject(xptr);
	xmlXPathFreeContext(xptrctxt);
    }
#endif

    /*
     * Do the xml:base fixup if needed
     */
    if ((doc != NULL) && (URL != NULL) &&
        (!(ctxt->parseFlags & XML_PARSE_NOBASEFIX)) &&
	(!(doc->parseFlags & XML_PARSE_NOBASEFIX))) {
	xmlNodePtr node;
	xmlChar *base;
	xmlChar *curBase;

	/*
	 * The base is only adjusted if "necessary", i.e. if the xinclude node
	 * has a base specified, or the URL is relative
	 */
	base = xmlGetNsProp(ref->elem, BAD_CAST "base", XML_XML_NAMESPACE);
	if (base == NULL) {
	    /*
	     * No xml:base on the xinclude node, so we check whether the
	     * URI base is different than (relative to) the context base
	     */
	    curBase = xmlBuildRelativeURI(URL, ctxt->base);
	    if (curBase == NULL) {	/* Error return */
	        xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_HREF_URI,
		       "trying to build relative URI from %s\n", URL);
	    } else {
		/* If the URI doesn't contain a slash, it's not relative */
	        if (!xmlStrchr(curBase, '/'))
		    xmlFree(curBase);
		else
		    base = curBase;
	    }
	}
	if (base != NULL) {	/* Adjustment may be needed */
	    node = ref->inc;
	    while (node != NULL) {
		/* Only work on element nodes */
		if (node->type == XML_ELEMENT_NODE) {
		    curBase = xmlNodeGetBase(node->doc, node);
		    /* If no current base, set it */
		    if (curBase == NULL) {
			xmlNodeSetBase(node, base);
		    } else {
			/*
			 * If the current base is the same as the
			 * URL of the document, then reset it to be
			 * the specified xml:base or the relative URI
			 */
			if (xmlStrEqual(curBase, node->doc->URL)) {
			    xmlNodeSetBase(node, base);
			} else {
			    /*
			     * If the element already has an xml:base
			     * set, then relativise it if necessary
			     */
			    xmlChar *xmlBase;
			    xmlBase = xmlGetNsProp(node,
					    BAD_CAST "base",
					    XML_XML_NAMESPACE);
			    if (xmlBase != NULL) {
				xmlChar *relBase;
				relBase = xmlBuildURI(xmlBase, base);
				if (relBase == NULL) { /* error */
				    xmlXIncludeErr(ctxt,
						ref->elem,
						XML_XINCLUDE_HREF_URI,
					"trying to rebuild base from %s\n",
						xmlBase);
				} else {
				    xmlNodeSetBase(node, relBase);
				    xmlFree(relBase);
				}
				xmlFree(xmlBase);
			    }
			}
			xmlFree(curBase);
		    }
		}
	        node = node->next;
	    }
	    xmlFree(base);
	}
    }
    ret = 0;

error:
    xmlFree(URL);
    xmlFree(fragment);
    return(ret);
}

/**
 * xmlXIncludeLoadTxt:
 * @ctxt:  the XInclude context
 * @url:  the associated URL
 * @ref:  an XMLXincludeRefPtr
 *
 * Load the content, and store the result in the XInclude context
 *
 * Returns 0 in case of success, -1 in case of failure
 */
static int
xmlXIncludeLoadTxt(xmlXIncludeCtxtPtr ctxt, const xmlChar *url,
                   xmlXIncludeRefPtr ref) {
    xmlParserInputBufferPtr buf;
    xmlNodePtr node = NULL;
    xmlURIPtr uri = NULL;
    xmlChar *URL = NULL;
    int i;
    int ret = -1;
    xmlChar *encoding = NULL;
    xmlCharEncoding enc = (xmlCharEncoding) 0;
    xmlParserCtxtPtr pctxt = NULL;
    xmlParserInputPtr inputStream = NULL;
    int len;
    const xmlChar *content;


    /* Don't read from stdin. */
    if (xmlStrcmp(url, BAD_CAST "-") == 0)
        url = BAD_CAST "./-";

    /*
     * Check the URL and remove any fragment identifier
     */
    uri = xmlParseURI((const char *)url);
    if (uri == NULL) {
	xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_HREF_URI,
	               "invalid value URI %s\n", url);
	goto error;
    }
    if (uri->fragment != NULL) {
	xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_TEXT_FRAGMENT,
	               "fragment identifier forbidden for text: %s\n",
		       (const xmlChar *) uri->fragment);
	goto error;
    }
    URL = xmlSaveUri(uri);
    if (URL == NULL) {
	xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_HREF_URI,
	               "invalid value URI %s\n", url);
	goto error;
    }

    /*
     * Handling of references to the local document are done
     * directly through ctxt->doc.
     */
    if (URL[0] == 0) {
	xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_TEXT_DOCUMENT,
		       "text serialization of document not available\n", NULL);
	goto error;
    }

    /*
     * Prevent reloading the document twice.
     */
    for (i = 0; i < ctxt->txtNr; i++) {
	if (xmlStrEqual(URL, ctxt->txtTab[i].url)) {
            node = xmlNewDocText(ctxt->doc, ctxt->txtTab[i].text);
	    goto loaded;
	}
    }

    /*
     * Try to get the encoding if available
     */
    if (ref->elem != NULL) {
	encoding = xmlGetProp(ref->elem, XINCLUDE_PARSE_ENCODING);
    }
    if (encoding != NULL) {
	/*
	 * TODO: we should not have to remap to the xmlCharEncoding
	 *       predefined set, a better interface than
	 *       xmlParserInputBufferCreateFilename should allow any
	 *       encoding supported by iconv
	 */
        enc = xmlParseCharEncoding((const char *) encoding);
	if (enc == XML_CHAR_ENCODING_ERROR) {
	    xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_UNKNOWN_ENCODING,
			   "encoding %s not supported\n", encoding);
	    goto error;
	}
    }

    /*
     * Load it.
     */
    pctxt = xmlNewParserCtxt();
    inputStream = xmlLoadExternalEntity((const char*)URL, NULL, pctxt);
    if(inputStream == NULL)
	goto error;
    buf = inputStream->buf;
    if (buf == NULL)
	goto error;
    if (buf->encoder)
	xmlCharEncCloseFunc(buf->encoder);
    buf->encoder = xmlGetCharEncodingHandler(enc);
    node = xmlNewDocText(ctxt->doc, NULL);
    if (node == NULL) {
        xmlXIncludeErrMemory(ctxt, ref->elem, NULL);
	goto error;
    }

    /*
     * Scan all chars from the resource and add the to the node
     */
    while (xmlParserInputBufferRead(buf, 4096) > 0)
        ;

    content = xmlBufContent(buf->buffer);
    len = xmlBufLength(buf->buffer);
    for (i = 0; i < len;) {
        int cur;
        int l;

        l = len - i;
        cur = xmlGetUTF8Char(&content[i], &l);
        if ((cur < 0) || (!IS_CHAR(cur))) {
            xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_INVALID_CHAR,
                           "%s contains invalid char\n", URL);
            goto error;
        }

        i += l;
    }

    xmlNodeAddContentLen(node, content, len);

    if (ctxt->txtNr >= ctxt->txtMax) {
        xmlXIncludeTxt *tmp;
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        size_t newSize = ctxt->txtMax ? ctxt->txtMax * 2 : 1;
#else
        size_t newSize = ctxt->txtMax ? ctxt->txtMax * 2 : 8;
#endif

        tmp = xmlRealloc(ctxt->txtTab, sizeof(xmlXIncludeTxt) * newSize);
        if (tmp == NULL) {
            xmlXIncludeErrMemory(ctxt, ref->elem,
                                 "growing XInclude text table");
	    goto error;
        }
        ctxt->txtMax = newSize;
        ctxt->txtTab = tmp;
    }
    ctxt->txtTab[ctxt->txtNr].text = xmlStrdup(node->content);
    ctxt->txtTab[ctxt->txtNr].url = xmlStrdup(URL);
    ctxt->txtNr++;

loaded:
    /*
     * Add the element as the replacement copy.
     */
    ref->inc = node;
    node = NULL;
    ret = 0;

error:
    xmlFreeNode(node);
    xmlFreeInputStream(inputStream);
    xmlFreeParserCtxt(pctxt);
    xmlFree(encoding);
    xmlFreeURI(uri);
    xmlFree(URL);
    return(ret);
}

/**
 * xmlXIncludeLoadFallback:
 * @ctxt:  the XInclude context
 * @fallback:  the fallback node
 * @ref:  an XMLXincludeRefPtr
 *
 * Load the content of the fallback node, and store the result
 * in the XInclude context
 *
 * Returns 0 in case of success, -1 in case of failure
 */
static int
xmlXIncludeLoadFallback(xmlXIncludeCtxtPtr ctxt, xmlNodePtr fallback,
                        xmlXIncludeRefPtr ref) {
    int ret = 0;
    int oldNbErrors;

    if ((fallback == NULL) || (fallback->type == XML_NAMESPACE_DECL) ||
        (ctxt == NULL))
	return(-1);
    if (fallback->children != NULL) {
	/*
	 * It's possible that the fallback also has 'includes'
	 * (Bug 129969), so we re-process the fallback just in case
	 */
        oldNbErrors = ctxt->nbErrors;
	ref->inc = xmlXIncludeCopyNode(ctxt, fallback, 1);
	if (ctxt->nbErrors > oldNbErrors)
	    ret = -1;
        else if (ref->inc == NULL)
            ref->emptyFb = 1;
    } else {
        ref->inc = NULL;
	ref->emptyFb = 1;	/* flag empty callback */
    }
    ref->fallback = 1;
    return(ret);
}

/************************************************************************
 *									*
 *			XInclude Processing				*
 *									*
 ************************************************************************/

/**
 * xmlXIncludeExpandNode:
 * @ctxt: an XInclude context
 * @node: an XInclude node
 *
 * If the XInclude node wasn't processed yet, create a new RefPtr,
 * add it to ctxt->incTab and load the included items.
 *
 * Returns the new or existing xmlXIncludeRefPtr, or NULL in case of error.
 */
static xmlXIncludeRefPtr
xmlXIncludeExpandNode(xmlXIncludeCtxtPtr ctxt, xmlNodePtr node) {
    xmlXIncludeRefPtr ref;
    int i;

    if (ctxt->fatalErr)
        return(NULL);
    if (ctxt->depth >= XINCLUDE_MAX_DEPTH) {
        xmlXIncludeErr(ctxt, node, XML_XINCLUDE_RECURSION,
                       "maximum recursion depth exceeded\n", NULL);
        ctxt->fatalErr = 1;
        return(NULL);
    }

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    /*
     * The XInclude engine offers no protection against exponential
     * expansion attacks similar to "billion laughs". Avoid timeouts by
     * limiting the total number of replacements when fuzzing.
     *
     * Unfortuately, a single XInclude can already result in quadratic
     * behavior:
     *
     *     <doc xmlns:xi="http://www.w3.org/2001/XInclude">
     *       <xi:include xpointer="xpointer(//e)"/>
     *       <e>
     *         <e>
     *           <e>
     *             <!-- more nested elements -->
     *           </e>
     *         </e>
     *       </e>
     *     </doc>
     */
    if (ctxt->incTotal >= 20)
        return(NULL);
    ctxt->incTotal++;
#endif

    for (i = 0; i < ctxt->incNr; i++) {
        if (ctxt->incTab[i]->elem == node) {
            if (ctxt->incTab[i]->expanding) {
                xmlXIncludeErr(ctxt, node, XML_XINCLUDE_RECURSION,
                               "inclusion loop detected\n", NULL);
                return(NULL);
            }
            return(ctxt->incTab[i]);
        }
    }

    ref = xmlXIncludeAddNode(ctxt, node);
    if (ref == NULL)
        return(NULL);
    ref->expanding = 1;
    ctxt->depth++;
    xmlXIncludeLoadNode(ctxt, ref);
    ctxt->depth--;
    ref->expanding = 0;

    return(ref);
}

/**
 * xmlXIncludeLoadNode:
 * @ctxt: an XInclude context
 * @ref: an xmlXIncludeRefPtr
 *
 * Find and load the infoset replacement for the given node.
 *
 * Returns 0 if substitution succeeded, -1 if some processing failed
 */
static int
xmlXIncludeLoadNode(xmlXIncludeCtxtPtr ctxt, xmlXIncludeRefPtr ref) {
    xmlNodePtr cur;
    xmlChar *href;
    xmlChar *parse;
    xmlChar *base;
    xmlChar *oldBase;
    xmlChar *URI;
    int xml = 1; /* default Issue 64 */
    int ret;

    if ((ctxt == NULL) || (ref == NULL))
	return(-1);
    cur = ref->elem;
    if (cur == NULL)
	return(-1);

    /*
     * read the attributes
     */
    href = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_HREF);
    if (href == NULL) {
	href = xmlStrdup(BAD_CAST ""); /* @@@@ href is now optional */
	if (href == NULL)
	    return(-1);
    }
    parse = xmlXIncludeGetProp(ctxt, cur, XINCLUDE_PARSE);
    if (parse != NULL) {
	if (xmlStrEqual(parse, XINCLUDE_PARSE_XML))
	    xml = 1;
	else if (xmlStrEqual(parse, XINCLUDE_PARSE_TEXT))
	    xml = 0;
	else {
	    xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_PARSE_VALUE,
			   "invalid value %s for 'parse'\n", parse);
	    if (href != NULL)
		xmlFree(href);
	    if (parse != NULL)
		xmlFree(parse);
	    return(-1);
	}
    }

    /*
     * compute the URI
     */
    base = xmlNodeGetBase(ctxt->doc, cur);
    if (base == NULL) {
	URI = xmlBuildURI(href, ctxt->doc->URL);
    } else {
	URI = xmlBuildURI(href, base);
    }
    if (URI == NULL) {
	xmlChar *escbase;
	xmlChar *eschref;
	/*
	 * Some escaping may be needed
	 */
	escbase = xmlURIEscape(base);
	eschref = xmlURIEscape(href);
	URI = xmlBuildURI(eschref, escbase);
	if (escbase != NULL)
	    xmlFree(escbase);
	if (eschref != NULL)
	    xmlFree(eschref);
    }
    if (URI == NULL) {
	xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_HREF_URI,
                       "failed build URL\n", NULL);
	if (parse != NULL)
	    xmlFree(parse);
	if (href != NULL)
	    xmlFree(href);
	if (base != NULL)
	    xmlFree(base);
	return(-1);
    }

    /*
     * Save the base for this include (saving the current one)
     */
    oldBase = ctxt->base;
    ctxt->base = base;

    if (xml) {
	ret = xmlXIncludeLoadDoc(ctxt, URI, ref);
	/* xmlXIncludeGetFragment(ctxt, cur, URI); */
    } else {
	ret = xmlXIncludeLoadTxt(ctxt, URI, ref);
    }

    /*
     * Restore the original base before checking for fallback
     */
    ctxt->base = oldBase;

    if (ret < 0) {
	xmlNodePtr children;

	/*
	 * Time to try a fallback if available
	 */
	children = cur->children;
	while (children != NULL) {
	    if ((children->type == XML_ELEMENT_NODE) &&
		(children->ns != NULL) &&
		(xmlStrEqual(children->name, XINCLUDE_FALLBACK)) &&
		((xmlStrEqual(children->ns->href, XINCLUDE_NS)) ||
		 (xmlStrEqual(children->ns->href, XINCLUDE_OLD_NS)))) {
		ret = xmlXIncludeLoadFallback(ctxt, children, ref);
		break;
	    }
	    children = children->next;
	}
    }
    if (ret < 0) {
	xmlXIncludeErr(ctxt, cur, XML_XINCLUDE_NO_FALLBACK,
		       "could not load %s, and no fallback was found\n",
		       URI);
    }

    /*
     * Cleanup
     */
    if (URI != NULL)
	xmlFree(URI);
    if (parse != NULL)
	xmlFree(parse);
    if (href != NULL)
	xmlFree(href);
    if (base != NULL)
	xmlFree(base);
    return(0);
}

/**
 * xmlXIncludeIncludeNode:
 * @ctxt: an XInclude context
 * @ref: an xmlXIncludeRefPtr
 *
 * Implement the infoset replacement for the given node
 *
 * Returns 0 if substitution succeeded, -1 if some processing failed
 */
static int
xmlXIncludeIncludeNode(xmlXIncludeCtxtPtr ctxt, xmlXIncludeRefPtr ref) {
    xmlNodePtr cur, end, list, tmp;

    if ((ctxt == NULL) || (ref == NULL))
	return(-1);
    cur = ref->elem;
    if ((cur == NULL) || (cur->type == XML_NAMESPACE_DECL))
	return(-1);

    list = ref->inc;
    ref->inc = NULL;
    ref->emptyFb = 0;

    /*
     * Check against the risk of generating a multi-rooted document
     */
    if ((cur->parent != NULL) &&
	(cur->parent->type != XML_ELEMENT_NODE)) {
	int nb_elem = 0;

	tmp = list;
	while (tmp != NULL) {
	    if (tmp->type == XML_ELEMENT_NODE)
		nb_elem++;
	    tmp = tmp->next;
	}
	if (nb_elem > 1) {
	    xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_MULTIPLE_ROOT,
		       "XInclude error: would result in multiple root nodes\n",
			   NULL);
            xmlFreeNodeList(list);
	    return(-1);
	}
    }

    if (ctxt->parseFlags & XML_PARSE_NOXINCNODE) {
	/*
	 * Add the list of nodes
	 */
	while (list != NULL) {
	    end = list;
	    list = list->next;

	    xmlAddPrevSibling(cur, end);
	}
        /*
         * FIXME: xmlUnlinkNode doesn't coalesce text nodes.
         */
	xmlUnlinkNode(cur);
	xmlFreeNode(cur);
    } else {
        xmlNodePtr child, next;

	/*
	 * Change the current node as an XInclude start one, and add an
	 * XInclude end one
	 */
        if (ref->fallback)
            xmlUnsetProp(cur, BAD_CAST "href");
	cur->type = XML_XINCLUDE_START;
        /* Remove fallback children */
        for (child = cur->children; child != NULL; child = next) {
            next = child->next;
            xmlUnlinkNode(child);
            xmlFreeNode(child);
        }
	end = xmlNewDocNode(cur->doc, cur->ns, cur->name, NULL);
	if (end == NULL) {
	    xmlXIncludeErr(ctxt, ref->elem, XML_XINCLUDE_BUILD_FAILED,
			   "failed to build node\n", NULL);
            xmlFreeNodeList(list);
	    return(-1);
	}
	end->type = XML_XINCLUDE_END;
	xmlAddNextSibling(cur, end);

	/*
	 * Add the list of nodes
	 */
	while (list != NULL) {
	    cur = list;
	    list = list->next;

	    xmlAddPrevSibling(end, cur);
	}
    }


    return(0);
}

/**
 * xmlXIncludeTestNode:
 * @ctxt: the XInclude processing context
 * @node: an XInclude node
 *
 * test if the node is an XInclude node
 *
 * Returns 1 true, 0 otherwise
 */
static int
xmlXIncludeTestNode(xmlXIncludeCtxtPtr ctxt, xmlNodePtr node) {
    if (node == NULL)
	return(0);
    if (node->type != XML_ELEMENT_NODE)
	return(0);
    if (node->ns == NULL)
	return(0);
    if ((xmlStrEqual(node->ns->href, XINCLUDE_NS)) ||
        (xmlStrEqual(node->ns->href, XINCLUDE_OLD_NS))) {
	if (xmlStrEqual(node->ns->href, XINCLUDE_OLD_NS)) {
	    if (ctxt->legacy == 0) {
#if 0 /* wait for the XML Core Working Group to get something stable ! */
		xmlXIncludeWarn(ctxt, node, XML_XINCLUDE_DEPRECATED_NS,
	               "Deprecated XInclude namespace found, use %s",
		                XINCLUDE_NS);
#endif
	        ctxt->legacy = 1;
	    }
	}
	if (xmlStrEqual(node->name, XINCLUDE_NODE)) {
	    xmlNodePtr child = node->children;
	    int nb_fallback = 0;

	    while (child != NULL) {
		if ((child->type == XML_ELEMENT_NODE) &&
		    (child->ns != NULL) &&
		    ((xmlStrEqual(child->ns->href, XINCLUDE_NS)) ||
		     (xmlStrEqual(child->ns->href, XINCLUDE_OLD_NS)))) {
		    if (xmlStrEqual(child->name, XINCLUDE_NODE)) {
			xmlXIncludeErr(ctxt, node,
			               XML_XINCLUDE_INCLUDE_IN_INCLUDE,
				       "%s has an 'include' child\n",
				       XINCLUDE_NODE);
			return(0);
		    }
		    if (xmlStrEqual(child->name, XINCLUDE_FALLBACK)) {
			nb_fallback++;
		    }
		}
		child = child->next;
	    }
	    if (nb_fallback > 1) {
		xmlXIncludeErr(ctxt, node, XML_XINCLUDE_FALLBACKS_IN_INCLUDE,
			       "%s has multiple fallback children\n",
		               XINCLUDE_NODE);
		return(0);
	    }
	    return(1);
	}
	if (xmlStrEqual(node->name, XINCLUDE_FALLBACK)) {
	    if ((node->parent == NULL) ||
		(node->parent->type != XML_ELEMENT_NODE) ||
		(node->parent->ns == NULL) ||
		((!xmlStrEqual(node->parent->ns->href, XINCLUDE_NS)) &&
		 (!xmlStrEqual(node->parent->ns->href, XINCLUDE_OLD_NS))) ||
		(!xmlStrEqual(node->parent->name, XINCLUDE_NODE))) {
		xmlXIncludeErr(ctxt, node,
		               XML_XINCLUDE_FALLBACK_NOT_IN_INCLUDE,
			       "%s is not the child of an 'include'\n",
			       XINCLUDE_FALLBACK);
	    }
	}
    }
    return(0);
}

/**
 * xmlXIncludeDoProcess:
 * @ctxt: the XInclude processing context
 * @tree: the top of the tree to process
 *
 * Implement the XInclude substitution on the XML document @doc
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *    or the number of substitutions done.
 */
static int
xmlXIncludeDoProcess(xmlXIncludeCtxtPtr ctxt, xmlNodePtr tree) {
    xmlXIncludeRefPtr ref;
    xmlNodePtr cur;
    int ret = 0;
    int i, start;

    if ((tree == NULL) || (tree->type == XML_NAMESPACE_DECL))
	return(-1);
    if (ctxt == NULL)
	return(-1);

    /*
     * First phase: lookup the elements in the document
     */
    start = ctxt->incNr;
    cur = tree;
    do {
	/* TODO: need to work on entities -> stack */
        if (xmlXIncludeTestNode(ctxt, cur) == 1) {
            ref = xmlXIncludeExpandNode(ctxt, cur);
            /*
             * Mark direct includes.
             */
            if (ref != NULL)
                ref->replace = 1;
        } else if ((cur->children != NULL) &&
                   ((cur->type == XML_DOCUMENT_NODE) ||
                    (cur->type == XML_ELEMENT_NODE))) {
            cur = cur->children;
            continue;
        }
        do {
            if (cur == tree)
                break;
            if (cur->next != NULL) {
                cur = cur->next;
                break;
            }
            cur = cur->parent;
        } while (cur != NULL);
    } while ((cur != NULL) && (cur != tree));

    /*
     * Second phase: extend the original document infoset.
     */
    for (i = start; i < ctxt->incNr; i++) {
	if (ctxt->incTab[i]->replace != 0) {
            if ((ctxt->incTab[i]->inc != NULL) ||
                (ctxt->incTab[i]->emptyFb != 0)) {	/* (empty fallback) */
                xmlXIncludeIncludeNode(ctxt, ctxt->incTab[i]);
            }
            ctxt->incTab[i]->replace = 0;
        } else {
            /*
             * Ignore includes which were added indirectly, for example
             * inside xi:fallback elements.
             */
            if (ctxt->incTab[i]->inc != NULL) {
                xmlFreeNodeList(ctxt->incTab[i]->inc);
                ctxt->incTab[i]->inc = NULL;
            }
        }
	ret++;
    }

    if (ctxt->isStream) {
        /*
         * incTab references nodes which will eventually be deleted in
         * streaming mode. The table is only required for XPointer
         * expressions which aren't allowed in streaming mode.
         */
        for (i = 0;i < ctxt->incNr;i++) {
            xmlXIncludeFreeRef(ctxt->incTab[i]);
        }
        ctxt->incNr = 0;
    }

    return(ret);
}

/**
 * xmlXIncludeSetFlags:
 * @ctxt:  an XInclude processing context
 * @flags: a set of xmlParserOption used for parsing XML includes
 *
 * Set the flags used for further processing of XML resources.
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int
xmlXIncludeSetFlags(xmlXIncludeCtxtPtr ctxt, int flags) {
    if (ctxt == NULL)
        return(-1);
    ctxt->parseFlags = flags;
    return(0);
}

/**
 * xmlXIncludeSetStreamingMode:
 * @ctxt:  an XInclude processing context
 * @mode:  whether streaming mode should be enabled
 *
 * In streaming mode, XPointer expressions aren't allowed.
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int
xmlXIncludeSetStreamingMode(xmlXIncludeCtxtPtr ctxt, int mode) {
    if (ctxt == NULL)
        return(-1);
    ctxt->isStream = !!mode;
    return(0);
}

/**
 * xmlXIncludeProcessTreeFlagsData:
 * @tree: an XML node
 * @flags: a set of xmlParserOption used for parsing XML includes
 * @data: application data that will be passed to the parser context
 *        in the _private field of the parser context(s)
 *
 * Implement the XInclude substitution on the XML node @tree
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *    or the number of substitutions done.
 */

int
xmlXIncludeProcessTreeFlagsData(xmlNodePtr tree, int flags, void *data) {
    xmlXIncludeCtxtPtr ctxt;
    int ret = 0;

    if ((tree == NULL) || (tree->type == XML_NAMESPACE_DECL) ||
        (tree->doc == NULL))
        return(-1);

    ctxt = xmlXIncludeNewContext(tree->doc);
    if (ctxt == NULL)
        return(-1);
    ctxt->_private = data;
    ctxt->base = xmlStrdup((xmlChar *)tree->doc->URL);
    xmlXIncludeSetFlags(ctxt, flags);
    ret = xmlXIncludeDoProcess(ctxt, tree);
    if ((ret >= 0) && (ctxt->nbErrors > 0))
        ret = -1;

    xmlXIncludeFreeContext(ctxt);
    return(ret);
}

/**
 * xmlXIncludeProcessFlagsData:
 * @doc: an XML document
 * @flags: a set of xmlParserOption used for parsing XML includes
 * @data: application data that will be passed to the parser context
 *        in the _private field of the parser context(s)
 *
 * Implement the XInclude substitution on the XML document @doc
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *    or the number of substitutions done.
 */
int
xmlXIncludeProcessFlagsData(xmlDocPtr doc, int flags, void *data) {
    xmlNodePtr tree;

    if (doc == NULL)
	return(-1);
    tree = xmlDocGetRootElement(doc);
    if (tree == NULL)
	return(-1);
    return(xmlXIncludeProcessTreeFlagsData(tree, flags, data));
}

/**
 * xmlXIncludeProcessFlags:
 * @doc: an XML document
 * @flags: a set of xmlParserOption used for parsing XML includes
 *
 * Implement the XInclude substitution on the XML document @doc
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *    or the number of substitutions done.
 */
int
xmlXIncludeProcessFlags(xmlDocPtr doc, int flags) {
    return xmlXIncludeProcessFlagsData(doc, flags, NULL);
}

/**
 * xmlXIncludeProcess:
 * @doc: an XML document
 *
 * Implement the XInclude substitution on the XML document @doc
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *    or the number of substitutions done.
 */
int
xmlXIncludeProcess(xmlDocPtr doc) {
    return(xmlXIncludeProcessFlags(doc, 0));
}

/**
 * xmlXIncludeProcessTreeFlags:
 * @tree: a node in an XML document
 * @flags: a set of xmlParserOption used for parsing XML includes
 *
 * Implement the XInclude substitution for the given subtree
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *    or the number of substitutions done.
 */
int
xmlXIncludeProcessTreeFlags(xmlNodePtr tree, int flags) {
    xmlXIncludeCtxtPtr ctxt;
    int ret = 0;

    if ((tree == NULL) || (tree->type == XML_NAMESPACE_DECL) ||
        (tree->doc == NULL))
	return(-1);
    ctxt = xmlXIncludeNewContext(tree->doc);
    if (ctxt == NULL)
	return(-1);
    ctxt->base = xmlNodeGetBase(tree->doc, tree);
    xmlXIncludeSetFlags(ctxt, flags);
    ret = xmlXIncludeDoProcess(ctxt, tree);
    if ((ret >= 0) && (ctxt->nbErrors > 0))
	ret = -1;

    xmlXIncludeFreeContext(ctxt);
    return(ret);
}

/**
 * xmlXIncludeProcessTree:
 * @tree: a node in an XML document
 *
 * Implement the XInclude substitution for the given subtree
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *    or the number of substitutions done.
 */
int
xmlXIncludeProcessTree(xmlNodePtr tree) {
    return(xmlXIncludeProcessTreeFlags(tree, 0));
}

/**
 * xmlXIncludeProcessNode:
 * @ctxt: an existing XInclude context
 * @node: a node in an XML document
 *
 * Implement the XInclude substitution for the given subtree reusing
 * the information and data coming from the given context.
 *
 * Returns 0 if no substitution were done, -1 if some processing failed
 *    or the number of substitutions done.
 */
int
xmlXIncludeProcessNode(xmlXIncludeCtxtPtr ctxt, xmlNodePtr node) {
    int ret = 0;

    if ((node == NULL) || (node->type == XML_NAMESPACE_DECL) ||
        (node->doc == NULL) || (ctxt == NULL))
	return(-1);
    ret = xmlXIncludeDoProcess(ctxt, node);
    if ((ret >= 0) && (ctxt->nbErrors > 0))
	ret = -1;
    return(ret);
}

#else /* !LIBXML_XINCLUDE_ENABLED */
#endif
