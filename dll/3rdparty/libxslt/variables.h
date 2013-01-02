/*
 * Summary: interface for the variable matching and lookup.
 * Description: interface for the variable matching and lookup.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_XSLT_VARIABLES_H__
#define __XML_XSLT_VARIABLES_H__

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xsltexports.h"
#include "xsltInternals.h"
#include "functions.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * XSLT_REGISTER_VARIABLE_LOOKUP:
 *
 * Registering macro, not general purpose at all but used in different modules.
 */

#define XSLT_REGISTER_VARIABLE_LOOKUP(ctxt)			\
    xmlXPathRegisterVariableLookup((ctxt)->xpathCtxt,		\
	       xsltXPathVariableLookup,	(void *)(ctxt));	\
    xsltRegisterAllFunctions((ctxt)->xpathCtxt);		\
    xsltRegisterAllElement(ctxt);				\
    (ctxt)->xpathCtxt->extra = ctxt

/*
 * Interfaces for the variable module.
 */

XSLTPUBFUN int XSLTCALL		
		xsltEvalGlobalVariables		(xsltTransformContextPtr ctxt);
XSLTPUBFUN int XSLTCALL		
		xsltEvalUserParams		(xsltTransformContextPtr ctxt,
						 const char **params);
XSLTPUBFUN int XSLTCALL		
    		xsltQuoteUserParams		(xsltTransformContextPtr ctxt,
						 const char **params);
XSLTPUBFUN int XSLTCALL		
		xsltEvalOneUserParam		(xsltTransformContextPtr ctxt,
    						 const xmlChar * name,
						 const xmlChar * value);
XSLTPUBFUN int XSLTCALL		
    		xsltQuoteOneUserParam		(xsltTransformContextPtr ctxt,
    						 const xmlChar * name,
						 const xmlChar * value);

XSLTPUBFUN void XSLTCALL		
    		xsltParseGlobalVariable		(xsltStylesheetPtr style,
						 xmlNodePtr cur);
XSLTPUBFUN void XSLTCALL		
    		xsltParseGlobalParam		(xsltStylesheetPtr style,
						 xmlNodePtr cur);
XSLTPUBFUN void XSLTCALL		
    		xsltParseStylesheetVariable	(xsltTransformContextPtr ctxt,
						 xmlNodePtr cur);
XSLTPUBFUN void XSLTCALL		
    		xsltParseStylesheetParam	(xsltTransformContextPtr ctxt,
						 xmlNodePtr cur);
XSLTPUBFUN xsltStackElemPtr XSLTCALL 
		xsltParseStylesheetCallerParam	(xsltTransformContextPtr ctxt,
						 xmlNodePtr cur);
XSLTPUBFUN int XSLTCALL		
    		xsltAddStackElemList		(xsltTransformContextPtr ctxt,
						 xsltStackElemPtr elems);
XSLTPUBFUN void XSLTCALL		
    		xsltFreeGlobalVariables		(xsltTransformContextPtr ctxt);
XSLTPUBFUN xmlXPathObjectPtr XSLTCALL	
		xsltVariableLookup		(xsltTransformContextPtr ctxt,
						 const xmlChar *name,
						 const xmlChar *ns_uri);
XSLTPUBFUN xmlXPathObjectPtr XSLTCALL	
		xsltXPathVariableLookup		(void *ctxt,
						 const xmlChar *name,
						 const xmlChar *ns_uri);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_VARIABLES_H__ */

