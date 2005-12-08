#include <Python.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/catalog.h>
#include <libxml/threads.h>
#include <libxml/nanoftp.h>
#include <libxml/nanohttp.h>
#include <libxml/uri.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/xinclude.h>
#include <libxml/xpointer.h>
#include <libxml/xmlunicode.h>
#include <libxml/xmlregexp.h>
#include <libxml/xmlautomata.h>
#include <libxml/xmlreader.h>
#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/relaxng.h>
#include <libxml/xmlschemas.h>
#endif

/**
 * ATTRIBUTE_UNUSED:
 *
 * Macro used to signal to GCC unused function parameters
 * Repeated here since the definition is not available when
 * compiled outside the libxml2 build tree.
 */
#ifdef __GNUC__
#ifdef ATTRIBUTE_UNUSED
#undef ATTRIBUTE_UNUSED
#endif
#ifndef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif /* ATTRIBUTE_UNUSED */
#else
#define ATTRIBUTE_UNUSED
#endif

#define PyxmlNode_Get(v) (((v) == Py_None) ? NULL : \
	(((PyxmlNode_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlNodePtr obj;
} PyxmlNode_Object;

#define PyxmlXPathContext_Get(v) (((v) == Py_None) ? NULL : \
	(((PyxmlXPathContext_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlXPathContextPtr obj;
} PyxmlXPathContext_Object;

#define PyxmlXPathParserContext_Get(v) (((v) == Py_None) ? NULL : \
	(((PyxmlXPathParserContext_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlXPathParserContextPtr obj;
} PyxmlXPathParserContext_Object;

#define PyparserCtxt_Get(v) (((v) == Py_None) ? NULL : \
        (((PyparserCtxt_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlParserCtxtPtr obj;
} PyparserCtxt_Object;

#define PyValidCtxt_Get(v) (((v) == Py_None) ? NULL : \
	(((PyValidCtxt_Object *)(v))->obj))

typedef struct {
	PyObject_HEAD
	xmlValidCtxtPtr obj;
} PyValidCtxt_Object;

#define Pycatalog_Get(v) (((v) == Py_None) ? NULL : \
        (((Pycatalog_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlCatalogPtr obj;
} Pycatalog_Object;

#ifdef LIBXML_REGEXP_ENABLED
#define PyxmlReg_Get(v) (((v) == Py_None) ? NULL : \
        (((PyxmlReg_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlRegexpPtr obj;
} PyxmlReg_Object;
#endif /* LIBXML_REGEXP_ENABLED */

#define PyxmlTextReader_Get(v) (((v) == Py_None) ? NULL : \
        (((PyxmlTextReader_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlTextReaderPtr obj;
} PyxmlTextReader_Object;

#define PyxmlTextReaderLocator_Get(v) (((v) == Py_None) ? NULL : \
        (((PyxmlTextReaderLocator_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlTextReaderLocatorPtr obj;
} PyxmlTextReaderLocator_Object;

#define PyURI_Get(v) (((v) == Py_None) ? NULL : \
	(((PyURI_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlErrorPtr obj;
} PyError_Object;

#define PyError_Get(v) (((v) == Py_None) ? NULL : \
	(((PyError_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlOutputBufferPtr obj;
} PyoutputBuffer_Object;

#define PyoutputBuffer_Get(v) (((v) == Py_None) ? NULL : \
	(((PyoutputBuffer_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlParserInputBufferPtr obj;
} PyinputBuffer_Object;

#define PyinputBuffer_Get(v) (((v) == Py_None) ? NULL : \
	(((PyinputBuffer_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlURIPtr obj;
} PyURI_Object;

/* FILE * have their own internal representation */
#define PyFile_Get(v) (((v) == Py_None) ? NULL : \
	(PyFile_Check(v) ? (PyFile_AsFile(v)) : stdout))

#ifdef LIBXML_SCHEMAS_ENABLED
typedef struct {
    PyObject_HEAD
    xmlRelaxNGPtr obj;
} PyrelaxNgSchema_Object;

#define PyrelaxNgSchema_Get(v) (((v) == Py_None) ? NULL : \
	(((PyrelaxNgSchema_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlRelaxNGParserCtxtPtr obj;
} PyrelaxNgParserCtxt_Object;

#define PyrelaxNgParserCtxt_Get(v) (((v) == Py_None) ? NULL : \
	(((PyrelaxNgParserCtxt_Object *)(v))->obj))

typedef struct {
    PyObject_HEAD
    xmlRelaxNGValidCtxtPtr obj;
} PyrelaxNgValidCtxt_Object;

#define PyrelaxNgValidCtxt_Get(v) (((v) == Py_None) ? NULL : \
	(((PyrelaxNgValidCtxt_Object *)(v))->obj))

typedef struct {
	PyObject_HEAD
	xmlSchemaPtr obj;
} PySchema_Object;

#define PySchema_Get(v) (((v) == Py_None) ? NULL : \
	(((PySchema_Object *)(v))->obj))

typedef struct {
	PyObject_HEAD
	xmlSchemaParserCtxtPtr obj;
} PySchemaParserCtxt_Object;

#define PySchemaParserCtxt_Get(v) (((v) == Py_None) ? NULL : \
	(((PySchemaParserCtxt_Object *)(v))->obj))

typedef struct {
	PyObject_HEAD
	xmlSchemaValidCtxtPtr obj;
} PySchemaValidCtxt_Object;

#define PySchemaValidCtxt_Get(v) (((v) == Py_None) ? NULL : \
	(((PySchemaValidCtxt_Object *)(v))->obj))

#endif /* LIBXML_SCHEMAS_ENABLED */

PyObject * libxml_intWrap(int val);
PyObject * libxml_longWrap(long val);
PyObject * libxml_xmlCharPtrWrap(xmlChar *str);
PyObject * libxml_constxmlCharPtrWrap(const xmlChar *str);
PyObject * libxml_charPtrWrap(char *str);
PyObject * libxml_constcharPtrWrap(const char *str);
PyObject * libxml_charPtrConstWrap(const char *str);
PyObject * libxml_xmlCharPtrConstWrap(const xmlChar *str);
PyObject * libxml_xmlDocPtrWrap(xmlDocPtr doc);
PyObject * libxml_xmlNodePtrWrap(xmlNodePtr node);
PyObject * libxml_xmlAttrPtrWrap(xmlAttrPtr attr);
PyObject * libxml_xmlNsPtrWrap(xmlNsPtr ns);
PyObject * libxml_xmlAttributePtrWrap(xmlAttributePtr ns);
PyObject * libxml_xmlElementPtrWrap(xmlElementPtr ns);
PyObject * libxml_doubleWrap(double val);
PyObject * libxml_xmlXPathContextPtrWrap(xmlXPathContextPtr ctxt);
PyObject * libxml_xmlParserCtxtPtrWrap(xmlParserCtxtPtr ctxt);
PyObject * libxml_xmlXPathParserContextPtrWrap(xmlXPathParserContextPtr ctxt);
PyObject * libxml_xmlXPathObjectPtrWrap(xmlXPathObjectPtr obj);
PyObject * libxml_xmlValidCtxtPtrWrap(xmlValidCtxtPtr valid);
PyObject * libxml_xmlCatalogPtrWrap(xmlCatalogPtr obj);
PyObject * libxml_xmlURIPtrWrap(xmlURIPtr uri);
PyObject * libxml_xmlOutputBufferPtrWrap(xmlOutputBufferPtr buffer);
PyObject * libxml_xmlParserInputBufferPtrWrap(xmlParserInputBufferPtr buffer);
#ifdef LIBXML_REGEXP_ENABLED
PyObject * libxml_xmlRegexpPtrWrap(xmlRegexpPtr regexp);
#endif /* LIBXML_REGEXP_ENABLED */
PyObject * libxml_xmlTextReaderPtrWrap(xmlTextReaderPtr reader);
PyObject * libxml_xmlTextReaderLocatorPtrWrap(xmlTextReaderLocatorPtr locator);

xmlXPathObjectPtr libxml_xmlXPathObjectPtrConvert(PyObject * obj);
#ifdef LIBXML_SCHEMAS_ENABLED
PyObject * libxml_xmlRelaxNGPtrWrap(xmlRelaxNGPtr ctxt);
PyObject * libxml_xmlRelaxNGParserCtxtPtrWrap(xmlRelaxNGParserCtxtPtr ctxt);
PyObject * libxml_xmlRelaxNGValidCtxtPtrWrap(xmlRelaxNGValidCtxtPtr valid);
PyObject * libxml_xmlSchemaPtrWrap(xmlSchemaPtr ctxt);
PyObject * libxml_xmlSchemaParserCtxtPtrWrap(xmlSchemaParserCtxtPtr ctxt);
PyObject * libxml_xmlSchemaValidCtxtPtrWrap(xmlSchemaValidCtxtPtr valid);
#endif /* LIBXML_SCHEMAS_ENABLED */
PyObject * libxml_xmlErrorPtrWrap(xmlErrorPtr error);
PyObject * libxml_xmlSchemaSetValidErrors(PyObject * self, PyObject * args);
