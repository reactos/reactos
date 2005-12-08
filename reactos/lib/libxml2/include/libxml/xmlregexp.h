/*
 * Summary: regular expressions handling
 * Description: basic API for libxml regular expressions handling used
 *              for XML Schemas and validation.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_REGEXP_H__
#define __XML_REGEXP_H__

#include <libxml/xmlversion.h>

#ifdef LIBXML_REGEXP_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * xmlRegexpPtr:
 *
 * A libxml regular expression, they can actually be far more complex
 * thank the POSIX regex expressions.
 */
typedef struct _xmlRegexp xmlRegexp;
typedef xmlRegexp *xmlRegexpPtr;

/**
 * xmlRegExecCtxtPtr:
 *
 * A libxml progressive regular expression evaluation context
 */
typedef struct _xmlRegExecCtxt xmlRegExecCtxt;
typedef xmlRegExecCtxt *xmlRegExecCtxtPtr;

#ifdef __cplusplus
}
#endif 
#include <libxml/tree.h>
#ifdef __cplusplus
extern "C" {
#endif

/*
 * The POSIX like API
 */
XMLPUBFUN xmlRegexpPtr XMLCALL
		    xmlRegexpCompile	(const xmlChar *regexp);
XMLPUBFUN void XMLCALL			 xmlRegFreeRegexp(xmlRegexpPtr regexp);
XMLPUBFUN int XMLCALL			
		    xmlRegexpExec	(xmlRegexpPtr comp,
					 const xmlChar *value);
XMLPUBFUN void XMLCALL			
    		    xmlRegexpPrint	(FILE *output,
					 xmlRegexpPtr regexp);
XMLPUBFUN int XMLCALL			
		    xmlRegexpIsDeterminist(xmlRegexpPtr comp);

/*
 * Callback function when doing a transition in the automata
 */
typedef void (*xmlRegExecCallbacks) (xmlRegExecCtxtPtr exec,
	                             const xmlChar *token,
				     void *transdata,
				     void *inputdata);

/*
 * The progressive API
 */
XMLPUBFUN xmlRegExecCtxtPtr XMLCALL	
    		    xmlRegNewExecCtxt	(xmlRegexpPtr comp,
					 xmlRegExecCallbacks callback,
					 void *data);
XMLPUBFUN void XMLCALL			
		    xmlRegFreeExecCtxt	(xmlRegExecCtxtPtr exec);
XMLPUBFUN int XMLCALL			
    		    xmlRegExecPushString(xmlRegExecCtxtPtr exec,
					 const xmlChar *value,
					 void *data);
XMLPUBFUN int XMLCALL			
		    xmlRegExecPushString2(xmlRegExecCtxtPtr exec,
					 const xmlChar *value,
					 const xmlChar *value2,
					 void *data);

XMLPUBFUN int XMLCALL
		    xmlRegExecNextValues(xmlRegExecCtxtPtr exec,
		    			 int *nbval,
		    			 int *nbneg,
					 xmlChar **values,
					 int *terminal);
XMLPUBFUN int XMLCALL
		    xmlRegExecErrInfo	(xmlRegExecCtxtPtr exec,
		    			 const xmlChar **string,
					 int *nbval,
		    			 int *nbneg,
					 xmlChar **values,
					 int *terminal);
#ifdef __cplusplus
}
#endif 

#endif /* LIBXML_REGEXP_ENABLED */

#endif /*__XML_REGEXP_H__ */
