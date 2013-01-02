/*
 * attributes.c: Implementation of the XSLT attributes handling
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXSLT
#include "libxslt.h"

#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_NAN_H
#include <nan.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/uri.h>
#include <libxml/parserInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "attributes.h"
#include "namespaces.h"
#include "templates.h"
#include "imports.h"
#include "transform.h"
#include "preproc.h"

#define WITH_XSLT_DEBUG_ATTRIBUTES
#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_ATTRIBUTES
#endif

/*
 * TODO: merge attribute sets from different import precedence.
 *       all this should be precomputed just before the transformation
 *       starts or at first hit with a cache in the context.
 *       The simple way for now would be to not allow redefinition of
 *       attributes once generated in the output tree, possibly costlier.
 */

/*
 * Useful macros
 */
#ifdef IS_BLANK
#undef IS_BLANK
#endif

#define IS_BLANK(c) (((c) == 0x20) || ((c) == 0x09) || ((c) == 0xA) ||	\
                     ((c) == 0x0D))

#define IS_BLANK_NODE(n)						\
    (((n)->type == XML_TEXT_NODE) && (xsltIsBlank((n)->content)))


/*
 * The in-memory structure corresponding to an XSLT Attribute in
 * an attribute set
 */


typedef struct _xsltAttrElem xsltAttrElem;
typedef xsltAttrElem *xsltAttrElemPtr;
struct _xsltAttrElem {
    struct _xsltAttrElem *next;/* chained list */
    xmlNodePtr attr;	/* the xsl:attribute definition */
    const xmlChar *set; /* or the attribute set */
    const xmlChar *ns;  /* and its namespace */
};

/************************************************************************
 *									*
 *			XSLT Attribute handling				*
 *									*
 ************************************************************************/

/**
 * xsltNewAttrElem:
 * @attr:  the new xsl:attribute node
 *
 * Create a new XSLT AttrElem
 *
 * Returns the newly allocated xsltAttrElemPtr or NULL in case of error
 */
static xsltAttrElemPtr
xsltNewAttrElem(xmlNodePtr attr) {
    xsltAttrElemPtr cur;

    cur = (xsltAttrElemPtr) xmlMalloc(sizeof(xsltAttrElem));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewAttrElem : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltAttrElem));
    cur->attr = attr;
    return(cur);
}

/**
 * xsltFreeAttrElem:
 * @attr:  an XSLT AttrElem
 *
 * Free up the memory allocated by @attr
 */
static void
xsltFreeAttrElem(xsltAttrElemPtr attr) {
    xmlFree(attr);
}

/**
 * xsltFreeAttrElemList:
 * @list:  an XSLT AttrElem list
 *
 * Free up the memory allocated by @list
 */
static void
xsltFreeAttrElemList(xsltAttrElemPtr list) {
    xsltAttrElemPtr next;
    
    while (list != NULL) {
	next = list->next;
	xsltFreeAttrElem(list);
	list = next;
    }
}

#ifdef XSLT_REFACTORED
    /*
    * This was moved to xsltParseStylesheetAttributeSet().
    */
#else
/**
 * xsltAddAttrElemList:
 * @list:  an XSLT AttrElem list
 * @attr:  the new xsl:attribute node
 *
 * Add the new attribute to the list.
 *
 * Returns the new list pointer
 */
static xsltAttrElemPtr
xsltAddAttrElemList(xsltAttrElemPtr list, xmlNodePtr attr) {
    xsltAttrElemPtr next, cur;

    if (attr == NULL)
	return(list);
    if (list == NULL)
	return(xsltNewAttrElem(attr));
    cur = list;
    while (cur != NULL) {	
	next = cur->next;
	if (cur->attr == attr)
	    return(cur);
	if (cur->next == NULL) {
	    cur->next = xsltNewAttrElem(attr);
	    return(list);
	}
	cur = next;
    }
    return(list);
}
#endif /* XSLT_REFACTORED */

/**
 * xsltMergeAttrElemList:
 * @list:  an XSLT AttrElem list
 * @old:  another XSLT AttrElem list
 *
 * Add all the attributes from list @old to list @list,
 * but drop redefinition of existing values.
 *
 * Returns the new list pointer
 */
static xsltAttrElemPtr
xsltMergeAttrElemList(xsltStylesheetPtr style,
		      xsltAttrElemPtr list, xsltAttrElemPtr old) {
    xsltAttrElemPtr cur;
    int add;

    while (old != NULL) {
	if ((old->attr == NULL) && (old->set == NULL)) {
	    old = old->next;
	    continue;
	}
	/*
	 * Check that the attribute is not yet in the list
	 */
	cur = list;
	add = 1;
	while (cur != NULL) {
	    if ((cur->attr == NULL) && (cur->set == NULL)) {
		if (cur->next == NULL)
		    break;
		cur = cur->next;
		continue;
	    }
	    if ((cur->set != NULL) && (cur->set == old->set)) {
		add = 0;
		break;
	    }
	    if (cur->set != NULL) {
		if (cur->next == NULL)
		    break;
		cur = cur->next;
		continue;
	    }
	    if (old->set != NULL) {
		if (cur->next == NULL)
		    break;
		cur = cur->next;
		continue;
	    }
	    if (cur->attr == old->attr) {
		xsltGenericError(xsltGenericErrorContext,
	     "xsl:attribute-set : use-attribute-sets recursion detected\n");
		return(list);
	    }
	    if (cur->next == NULL)
		break;
            cur = cur->next;
	}

	if (add == 1) {
	    /*
	    * Changed to use the string-dict, rather than duplicating
	    * @set and @ns; this fixes bug #340400.
	    */
	    if (cur == NULL) {
		list = xsltNewAttrElem(old->attr);
		if (old->set != NULL) {
		    list->set = xmlDictLookup(style->dict, old->set, -1);
		    if (old->ns != NULL)
			list->ns = xmlDictLookup(style->dict, old->ns, -1);
		}
	    } else if (add) {
		cur->next = xsltNewAttrElem(old->attr);
		if (old->set != NULL) {
		    cur->next->set = xmlDictLookup(style->dict, old->set, -1);
		    if (old->ns != NULL)
			cur->next->ns = xmlDictLookup(style->dict, old->ns, -1);
		}
	    }
	}

	old = old->next;
    }
    return(list);
}

/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltParseStylesheetAttributeSet:
 * @style:  the XSLT stylesheet
 * @cur:  the "attribute-set" element
 *
 * parse an XSLT stylesheet attribute-set element
 */

void
xsltParseStylesheetAttributeSet(xsltStylesheetPtr style, xmlNodePtr cur) {
    const xmlChar *ncname;
    const xmlChar *prefix;
    xmlChar *value;
    xmlNodePtr child;
    xsltAttrElemPtr attrItems;

    if ((cur == NULL) || (style == NULL))
	return;

    value = xmlGetNsProp(cur, (const xmlChar *)"name", NULL);
    if (value == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:attribute-set : name is missing\n");
	return;
    }

    ncname = xsltSplitQName(style->dict, value, &prefix);
    xmlFree(value);
    value = NULL;

    if (style->attributeSets == NULL) {
#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
	xsltGenericDebug(xsltGenericDebugContext,
	    "creating attribute set table\n");
#endif
	style->attributeSets = xmlHashCreate(10);
    }
    if (style->attributeSets == NULL)
	return;

    attrItems = xmlHashLookup2(style->attributeSets, ncname, prefix);

    /*
    * Parse the content. Only xsl:attribute elements are allowed.
    */
    child = cur->children;
    while (child != NULL) {
	/*
	* Report invalid nodes.
	*/
	if ((child->type != XML_ELEMENT_NODE) ||
	    (child->ns == NULL) ||
	    (! IS_XSLT_ELEM(child)))
	{
	    if (child->type == XML_ELEMENT_NODE)
		xsltTransformError(NULL, style, child,
			"xsl:attribute-set : unexpected child %s\n",
		                 child->name);
	    else
		xsltTransformError(NULL, style, child,
			"xsl:attribute-set : child of unexpected type\n");
	} else if (!IS_XSLT_NAME(child, "attribute")) {
	    xsltTransformError(NULL, style, child,
		"xsl:attribute-set : unexpected child xsl:%s\n",
		child->name);
	} else {
#ifdef XSLT_REFACTORED
	    xsltAttrElemPtr nextAttr, curAttr;

	    /*
	    * Process xsl:attribute
	    * ---------------------
	    */

#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
	    xsltGenericDebug(xsltGenericDebugContext,
		"add attribute to list %s\n", ncname);
#endif
	    /*
	    * The following was taken over from
	    * xsltAddAttrElemList().
	    */
	    if (attrItems == NULL) {
		attrItems = xsltNewAttrElem(child);
	    } else {
		curAttr = attrItems;
		while (curAttr != NULL) {
		    nextAttr = curAttr->next;
		    if (curAttr->attr == child) {
			/*
			* URGENT TODO: Can somebody explain
			*  why attrItems is set to curAttr
			*  here? Is this somehow related to
			*  avoidance of recursions?
			*/
			attrItems = curAttr;
			goto next_child;
		    }
		    if (curAttr->next == NULL)			
			curAttr->next = xsltNewAttrElem(child);
		    curAttr = nextAttr;
		}
	    }
	    /*
	    * Parse the xsl:attribute and its content.
	    */
	    xsltParseAnyXSLTElem(XSLT_CCTXT(style), child);
#else
#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
	    xsltGenericDebug(xsltGenericDebugContext,
		"add attribute to list %s\n", ncname);
#endif
	    /*
	    * OLD behaviour:
	    */
	    attrItems = xsltAddAttrElemList(attrItems, child);
#endif
	}

#ifdef XSLT_REFACTORED
next_child:
#endif
	child = child->next;
    }

    /*
    * Process attribue "use-attribute-sets".
    */
    /* TODO check recursion */    
    value = xmlGetNsProp(cur, (const xmlChar *)"use-attribute-sets",
	NULL);
    if (value != NULL) {
	const xmlChar *curval, *endval;
	curval = value;
	while (*curval != 0) {
	    while (IS_BLANK(*curval)) curval++;
	    if (*curval == 0)
		break;
	    endval = curval;
	    while ((*endval != 0) && (!IS_BLANK(*endval))) endval++;
	    curval = xmlDictLookup(style->dict, curval, endval - curval);
	    if (curval) {
		const xmlChar *ncname2 = NULL;
		const xmlChar *prefix2 = NULL;
		xsltAttrElemPtr refAttrItems;
		
#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
		xsltGenericDebug(xsltGenericDebugContext,
		    "xsl:attribute-set : %s adds use %s\n", ncname, curval);
#endif
		ncname2 = xsltSplitQName(style->dict, curval, &prefix2);
		refAttrItems = xsltNewAttrElem(NULL);
		if (refAttrItems != NULL) {
		    refAttrItems->set = ncname2;
		    refAttrItems->ns = prefix2;
		    attrItems = xsltMergeAttrElemList(style,
			attrItems, refAttrItems);
		    xsltFreeAttrElem(refAttrItems);
		}
	    }
	    curval = endval;
	}
	xmlFree(value);
	value = NULL;
    }

    /*
     * Update the value
     */
    /*
    * TODO: Why is this dummy entry needed.?
    */
    if (attrItems == NULL)
	attrItems = xsltNewAttrElem(NULL);
    xmlHashUpdateEntry2(style->attributeSets, ncname, prefix, attrItems, NULL);
#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
    xsltGenericDebug(xsltGenericDebugContext,
	"updated attribute list %s\n", ncname);
#endif
}

/**
 * xsltGetSAS:
 * @style:  the XSLT stylesheet
 * @name:  the attribute list name
 * @ns:  the attribute list namespace
 *
 * lookup an attribute set based on the style cascade
 *
 * Returns the attribute set or NULL
 */
static xsltAttrElemPtr
xsltGetSAS(xsltStylesheetPtr style, const xmlChar *name, const xmlChar *ns) {
    xsltAttrElemPtr values;

    while (style != NULL) {
	values = xmlHashLookup2(style->attributeSets, name, ns);
	if (values != NULL)
	    return(values);
	style = xsltNextImport(style);
    }
    return(NULL);
}

/**
 * xsltResolveSASCallback,:
 * @style:  the XSLT stylesheet
 *
 * resolve the references in an attribute set.
 */
static void
xsltResolveSASCallback(xsltAttrElemPtr values, xsltStylesheetPtr style,
	               const xmlChar *name, const xmlChar *ns,
		       ATTRIBUTE_UNUSED const xmlChar *ignored) {
    xsltAttrElemPtr tmp;
    xsltAttrElemPtr refs;

    tmp = values;
    while (tmp != NULL) {
	if (tmp->set != NULL) {
	    /*
	     * Check against cycles !
	     */
	    if ((xmlStrEqual(name, tmp->set)) && (xmlStrEqual(ns, tmp->ns))) {
		xsltGenericError(xsltGenericErrorContext,
     "xsl:attribute-set : use-attribute-sets recursion detected on %s\n",
                                 name);
	    } else {
#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
		xsltGenericDebug(xsltGenericDebugContext,
			"Importing attribute list %s\n", tmp->set);
#endif

		refs = xsltGetSAS(style, tmp->set, tmp->ns);
		if (refs == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
     "xsl:attribute-set : use-attribute-sets %s reference missing %s\n",
				     name, tmp->set);
		} else {
		    /*
		     * recurse first for cleanup
		     */
		    xsltResolveSASCallback(refs, style, name, ns, NULL);
		    /*
		     * Then merge
		     */
		    xsltMergeAttrElemList(style, values, refs);
		    /*
		     * Then suppress the reference
		     */
		    tmp->set = NULL;
		    tmp->ns = NULL;
		}
	    }
	}
	tmp = tmp->next;
    }
}

/**
 * xsltMergeSASCallback,:
 * @style:  the XSLT stylesheet
 *
 * Merge an attribute set from an imported stylesheet.
 */
static void
xsltMergeSASCallback(xsltAttrElemPtr values, xsltStylesheetPtr style,
	               const xmlChar *name, const xmlChar *ns,
		       ATTRIBUTE_UNUSED const xmlChar *ignored) {
    int ret;
    xsltAttrElemPtr topSet;

    ret = xmlHashAddEntry2(style->attributeSets, name, ns, values);
    if (ret < 0) {
	/*
	 * Add failed, this attribute set can be removed.
	 */
#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
	xsltGenericDebug(xsltGenericDebugContext,
		"attribute set %s present already in top stylesheet"
		" - merging\n", name);
#endif
	topSet = xmlHashLookup2(style->attributeSets, name, ns);
	if (topSet==NULL) {
	    xsltGenericError(xsltGenericErrorContext,
	        "xsl:attribute-set : logic error merging from imports for"
		" attribute-set %s\n", name);
	} else {
	    topSet = xsltMergeAttrElemList(style, topSet, values);
	    xmlHashUpdateEntry2(style->attributeSets, name, ns, topSet, NULL);
	}
	xsltFreeAttrElemList(values);
#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
    } else {
	xsltGenericDebug(xsltGenericDebugContext,
		"attribute set %s moved to top stylesheet\n",
		         name);
#endif
    }
}

/**
 * xsltResolveStylesheetAttributeSet:
 * @style:  the XSLT stylesheet
 *
 * resolve the references between attribute sets.
 */
void
xsltResolveStylesheetAttributeSet(xsltStylesheetPtr style) {
    xsltStylesheetPtr cur;

#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
    xsltGenericDebug(xsltGenericDebugContext,
	    "Resolving attribute sets references\n");
#endif
    /*
     * First aggregate all the attribute sets definitions from the imports
     */
    cur = xsltNextImport(style);
    while (cur != NULL) {
	if (cur->attributeSets != NULL) {
	    if (style->attributeSets == NULL) {
#ifdef WITH_XSLT_DEBUG_ATTRIBUTES
		xsltGenericDebug(xsltGenericDebugContext,
		    "creating attribute set table\n");
#endif
		style->attributeSets = xmlHashCreate(10);
	    }
	    xmlHashScanFull(cur->attributeSets, 
		(xmlHashScannerFull) xsltMergeSASCallback, style);
	    /*
	     * the attribute lists have either been migrated to style
	     * or freed directly in xsltMergeSASCallback()
	     */
	    xmlHashFree(cur->attributeSets, NULL);
	    cur->attributeSets = NULL;
	}
	cur = xsltNextImport(cur);
    }

    /*
     * Then resolve all the references and computes the resulting sets
     */
    if (style->attributeSets != NULL) {
	xmlHashScanFull(style->attributeSets, 
		(xmlHashScannerFull) xsltResolveSASCallback, style);
    }
}

/**
 * xsltAttributeInternal:
 * @ctxt:  a XSLT process context
 * @node:  the current node in the source tree
 * @inst:  the xsl:attribute element
 * @comp:  precomputed information
 * @fromAttributeSet:  the attribute comes from an attribute-set
 *
 * Process the xslt attribute node on the source node
 */
static void
xsltAttributeInternal(xsltTransformContextPtr ctxt,
		      xmlNodePtr contextNode,
                      xmlNodePtr inst,
		      xsltStylePreCompPtr castedComp,
                      int fromAttributeSet)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemAttributePtr comp =
	(xsltStyleItemAttributePtr) castedComp;   
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    xmlNodePtr targetElem;
    xmlChar *prop = NULL;    
    const xmlChar *name = NULL, *prefix = NULL, *nsName = NULL;
    xmlChar *value = NULL;
    xmlNsPtr ns = NULL;
    xmlAttrPtr attr;    

    if ((ctxt == NULL) || (contextNode == NULL) || (inst == NULL))
        return;

    /* 
    * A comp->has_name == 0 indicates that we need to skip this instruction,
    * since it was evaluated to be invalid already during compilation.
    */
    if (!comp->has_name)
        return;
    /*
    * BIG NOTE: This previously used xsltGetSpecialNamespace() and
    *  xsltGetNamespace(), but since both are not appropriate, we
    *  will process namespace lookup here to avoid adding yet another
    *  ns-lookup function to namespaces.c.
    */
    /*
    * SPEC XSLT 1.0: Error cases:
    * - Creating nodes other than text nodes during the instantiation of
    *   the content of the xsl:attribute element; implementations may
    *   either signal the error or ignore the offending nodes."
    */

    if (comp == NULL) {
        xsltTransformError(ctxt, NULL, inst,
	    "Internal error in xsltAttributeInternal(): "
	    "The XSLT 'attribute' instruction was not compiled.\n");
        return;
    }
    /*
    * TODO: Shouldn't ctxt->insert == NULL be treated as an internal error?
    *   So report an internal error?
    */
    if (ctxt->insert == NULL)
        return;    
    /*
    * SPEC XSLT 1.0:
    *  "Adding an attribute to a node that is not an element;
    *  implementations may either signal the error or ignore the attribute."
    *
    * TODO: I think we should signal such errors in the future, and maybe
    *  provide an option to ignore such errors.
    */
    targetElem = ctxt->insert;
    if (targetElem->type != XML_ELEMENT_NODE)
	return;
    
    /*
    * SPEC XSLT 1.0:
    * "Adding an attribute to an element after children have been added
    *  to it; implementations may either signal the error or ignore the
    *  attribute."
    *
    * TODO: We should decide whether not to report such errors or
    *  to ignore them; note that we *ignore* if the parent is not an
    *  element, but here we report an error.
    */
    if (targetElem->children != NULL) {
	/*
	* NOTE: Ah! This seems to be intended to support streamed
	*  result generation!.
	*/
        xsltTransformError(ctxt, NULL, inst,
	    "xsl:attribute: Cannot add attributes to an "
	    "element if children have been already added "
	    "to the element.\n");
        return;
    }

    /*
    * Process the name
    * ----------------
    */    

#ifdef WITH_DEBUGGER
    if (ctxt->debugStatus != XSLT_DEBUG_NONE)
        xslHandleDebugger(inst, contextNode, NULL, ctxt);
#endif

    if (comp->name == NULL) {
	/* TODO: fix attr acquisition wrt to the XSLT namespace */
        prop = xsltEvalAttrValueTemplate(ctxt, inst,
	    (const xmlChar *) "name", XSLT_NAMESPACE);
        if (prop == NULL) {
            xsltTransformError(ctxt, NULL, inst,
		"xsl:attribute: The attribute 'name' is missing.\n");
            goto error;
        }
	if (xmlValidateQName(prop, 0)) {
	    xsltTransformError(ctxt, NULL, inst,
		"xsl:attribute: The effective name '%s' is not a "
		"valid QName.\n", prop);
	    /* we fall through to catch any further errors, if possible */
	}
	name = xsltSplitQName(ctxt->dict, prop, &prefix);
	xmlFree(prop);

	/*
	* Reject a prefix of "xmlns".
	*/
	if ((prefix != NULL) &&
	    (!xmlStrncasecmp(prefix, (xmlChar *) "xmlns", 5)))
	{
#ifdef WITH_XSLT_DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		"xsltAttribute: xmlns prefix forbidden\n");
#endif
	    /*
	    * SPEC XSLT 1.0:
	    *  "It is an error if the string that results from instantiating
	    *  the attribute value template is not a QName or is the string
	    *  xmlns. An XSLT processor may signal the error; if it does not
	    *  signal the error, it must recover by not adding the attribute
	    *  to the result tree."
	    * TODO: Decide which way to go here.
	    */
	    goto error;
	}

    } else {
	/*
	* The "name" value was static.
	*/
#ifdef XSLT_REFACTORED
	prefix = comp->nsPrefix;
	name = comp->name;
#else
	name = xsltSplitQName(ctxt->dict, comp->name, &prefix);
#endif
    }
    
    /*
    * Process namespace semantics
    * ---------------------------
    *
    * Evaluate the namespace name.
    */
    if (comp->has_ns) {
	/*
	* The "namespace" attribute was existent.
	*/
	if (comp->ns != NULL) {
	    /*
	    * No AVT; just plain text for the namespace name.
	    */
	    if (comp->ns[0] != 0)
		nsName = comp->ns;
	} else {
	    xmlChar *tmpNsName;
	    /*
	    * Eval the AVT.
	    */
	    /* TODO: check attr acquisition wrt to the XSLT namespace */
	    tmpNsName = xsltEvalAttrValueTemplate(ctxt, inst,
		(const xmlChar *) "namespace", XSLT_NAMESPACE);	
	    /*
	    * This fixes bug #302020: The AVT might also evaluate to the 
	    * empty string; this means that the empty string also indicates
	    * "no namespace".
	    * SPEC XSLT 1.0:
	    *  "If the string is empty, then the expanded-name of the
	    *  attribute has a null namespace URI."
	    */
	    if ((tmpNsName != NULL) && (tmpNsName[0] != 0))
		nsName = xmlDictLookup(ctxt->dict, BAD_CAST tmpNsName, -1);
	    xmlFree(tmpNsName);		
	};	    
    } else if (prefix != NULL) {
	/*
	* SPEC XSLT 1.0:
	*  "If the namespace attribute is not present, then the QName is
	*  expanded into an expanded-name using the namespace declarations
	*  in effect for the xsl:attribute element, *not* including any
	*  default namespace declaration."
	*/	
	ns = xmlSearchNs(inst->doc, inst, prefix);
	if (ns == NULL) {
	    /*
	    * Note that this is treated as an error now (checked with
	    *  Saxon, Xalan-J and MSXML).
	    */
	    xsltTransformError(ctxt, NULL, inst,
		"xsl:attribute: The QName '%s:%s' has no "
		"namespace binding in scope in the stylesheet; "
		"this is an error, since the namespace was not "
		"specified by the instruction itself.\n", prefix, name);
	} else
	    nsName = ns->href;	
    }

    if (fromAttributeSet) {
	/*
	* This tries to ensure that xsl:attribute(s) coming
	* from an xsl:attribute-set won't override attribute of
	* literal result elements or of explicit xsl:attribute(s).
	* URGENT TODO: This might be buggy, since it will miss to
	*  overwrite two equal attributes both from attribute sets.
	*/
	attr = xmlHasNsProp(targetElem, name, nsName);
	if (attr != NULL)
	    return;
    }

    /*
    * Find/create a matching ns-decl in the result tree.
    */
    ns = NULL;
    
#if 0
    if (0) {	
	/*
	* OPTIMIZE TODO: How do we know if we are adding to a
	*  fragment or to the result tree?
	*
	* If we are adding to a result tree fragment (i.e., not to the
	* actual result tree), we'll don't bother searching for the
	* ns-decl, but just store it in the dummy-doc of the result
	* tree fragment.
	*/
	if (nsName != NULL) {
	    /*
	    * TODO: Get the doc of @targetElem.
	    */
	    ns = xsltTreeAcquireStoredNs(some doc, nsName, prefix);
	}
    }
#endif

    if (nsName != NULL) {	
	/*
	* Something about ns-prefixes:
	* SPEC XSLT 1.0:
	*  "XSLT processors may make use of the prefix of the QName specified
	*  in the name attribute when selecting the prefix used for outputting
	*  the created attribute as XML; however, they are not required to do
	*  so and, if the prefix is xmlns, they must not do so"
	*/
	/*
	* xsl:attribute can produce a scenario where the prefix is NULL,
	* so generate a prefix.
	*/
	if (prefix == NULL) {
	    xmlChar *pref = xmlStrdup(BAD_CAST "ns_1");

	    ns = xsltGetSpecialNamespace(ctxt, inst, nsName, BAD_CAST pref,
		targetElem);

	    xmlFree(pref);
	} else {
	    ns = xsltGetSpecialNamespace(ctxt, inst, nsName, prefix,
		targetElem);
	}
	if (ns == NULL) {
	    xsltTransformError(ctxt, NULL, inst,
		"Namespace fixup error: Failed to acquire an in-scope "
		"namespace binding for the generated attribute '{%s}%s'.\n",
		nsName, name);
	    goto error;
	}
    }
    /*
    * Construction of the value
    * -------------------------
    */
    if (inst->children == NULL) {
	/*
	* No content.
	* TODO: Do we need to put the empty string in ?
	*/
	attr = xmlSetNsProp(ctxt->insert, ns, name, (const xmlChar *) "");
    } else if ((inst->children->next == NULL) && 
	    ((inst->children->type == XML_TEXT_NODE) ||
	     (inst->children->type == XML_CDATA_SECTION_NODE)))
    {
	xmlNodePtr copyTxt;
	
	/*
	* xmlSetNsProp() will take care of duplicates.
	*/
	attr = xmlSetNsProp(ctxt->insert, ns, name, NULL);
	if (attr == NULL) /* TODO: report error ? */
	    goto error;
	/*
	* This was taken over from xsltCopyText() (transform.c).
	*/
	if (ctxt->internalized &&
	    (ctxt->insert->doc != NULL) &&
	    (ctxt->insert->doc->dict == ctxt->dict))
	{
	    copyTxt = xmlNewText(NULL);
	    if (copyTxt == NULL) /* TODO: report error */
		goto error;
	    /*
	    * This is a safe scenario where we don't need to lookup
	    * the dict.
	    */
	    copyTxt->content = inst->children->content;
	    /*
	    * Copy "disable-output-escaping" information.
	    * TODO: Does this have any effect for attribute values
	    *  anyway?
	    */
	    if (inst->children->name == xmlStringTextNoenc)
		copyTxt->name = xmlStringTextNoenc;
	} else {
	    /*
	    * Copy the value.
	    */
	    copyTxt = xmlNewText(inst->children->content);
	    if (copyTxt == NULL) /* TODO: report error */
		goto error;	    	    
	}
	attr->children = attr->last = copyTxt;
	copyTxt->parent = (xmlNodePtr) attr;
	copyTxt->doc = attr->doc;
	/*
	* Copy "disable-output-escaping" information.
	* TODO: Does this have any effect for attribute values
	*  anyway?
	*/
	if (inst->children->name == xmlStringTextNoenc)
	    copyTxt->name = xmlStringTextNoenc;	

        /*
         * since we create the attribute without content IDness must be
         * asserted as a second step
         */
        if ((copyTxt->content != NULL) &&
            (xmlIsID(attr->doc, attr->parent, attr)))
            xmlAddID(NULL, attr->doc, copyTxt->content, attr);
    } else {
	/*
	* The sequence constructor might be complex, so instantiate it.
	*/
	value = xsltEvalTemplateString(ctxt, contextNode, inst);
	if (value != NULL) {
	    attr = xmlSetNsProp(ctxt->insert, ns, name, value);
	    xmlFree(value);
	} else {
	    /*
	    * TODO: Do we have to add the empty string to the attr?
	    * TODO: Does a  value of NULL indicate an
	    *  error in xsltEvalTemplateString() ?
	    */
	    attr = xmlSetNsProp(ctxt->insert, ns, name,
		(const xmlChar *) "");
	}
    }

error:
    return;    
}

/**
 * xsltAttribute:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt attribute node
 * @comp:  precomputed information
 *
 * Process the xslt attribute node on the source node
 */
void
xsltAttribute(xsltTransformContextPtr ctxt, xmlNodePtr node,
	      xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xsltAttributeInternal(ctxt, node, inst, comp, 0);
}

/**
 * xsltApplyAttributeSet:
 * @ctxt:  the XSLT stylesheet
 * @node:  the node in the source tree.
 * @inst:  the attribute node "xsl:use-attribute-sets"
 * @attrSets:  the list of QNames of the attribute-sets to be applied
 *
 * Apply the xsl:use-attribute-sets.
 * If @attrSets is NULL, then @inst will be used to exctract this
 * value.
 * If both, @attrSets and @inst, are NULL, then this will do nothing.
 */
void
xsltApplyAttributeSet(xsltTransformContextPtr ctxt, xmlNodePtr node,
                      xmlNodePtr inst,
                      const xmlChar *attrSets)
{
    const xmlChar *ncname = NULL;
    const xmlChar *prefix = NULL;    
    const xmlChar *curstr, *endstr;
    xsltAttrElemPtr attrs;
    xsltStylesheetPtr style;    

    if (attrSets == NULL) {
	if (inst == NULL)
	    return;
	else {
	    /*
	    * Extract the value from @inst.
	    */
	    if (inst->type == XML_ATTRIBUTE_NODE) {
		if ( ((xmlAttrPtr) inst)->children != NULL)
		    attrSets = ((xmlAttrPtr) inst)->children->content;
		
	    }
	    if (attrSets == NULL) {
		/*
		* TODO: Return an error?
		*/
		return;
	    }
	}
    }
    /*
    * Parse/apply the list of QNames.
    */
    curstr = attrSets;
    while (*curstr != 0) {
        while (IS_BLANK(*curstr))
            curstr++;
        if (*curstr == 0)
            break;
        endstr = curstr;
        while ((*endstr != 0) && (!IS_BLANK(*endstr)))
            endstr++;
        curstr = xmlDictLookup(ctxt->dict, curstr, endstr - curstr);
        if (curstr) {
	    /*
	    * TODO: Validate the QName.
	    */

#ifdef WITH_XSLT_DEBUG_curstrUTES
            xsltGenericDebug(xsltGenericDebugContext,
                             "apply curstrute set %s\n", curstr);
#endif
            ncname = xsltSplitQName(ctxt->dict, curstr, &prefix);

            style = ctxt->style;

#ifdef WITH_DEBUGGER
            if ((style != NULL) &&
		(style->attributeSets != NULL) &&
		(ctxt->debugStatus != XSLT_DEBUG_NONE))
	    {
                attrs =
                    xmlHashLookup2(style->attributeSets, ncname, prefix);
                if ((attrs != NULL) && (attrs->attr != NULL))
                    xslHandleDebugger(attrs->attr->parent, node, NULL,
			ctxt);
            }
#endif
	    /*
	    * Lookup the referenced curstrute-set.
	    */
            while (style != NULL) {
                attrs =
                    xmlHashLookup2(style->attributeSets, ncname, prefix);
                while (attrs != NULL) {
                    if (attrs->attr != NULL) {
                        xsltAttributeInternal(ctxt, node, attrs->attr,
			    attrs->attr->psvi, 1);
                    }
                    attrs = attrs->next;
                }
                style = xsltNextImport(style);
            }
        }
        curstr = endstr;
    }
}

/**
 * xsltFreeAttributeSetsHashes:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by attribute sets
 */
void
xsltFreeAttributeSetsHashes(xsltStylesheetPtr style) {
    if (style->attributeSets != NULL)
	xmlHashFree((xmlHashTablePtr) style->attributeSets,
		    (xmlHashDeallocator) xsltFreeAttrElemList);
    style->attributeSets = NULL;
}
